/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cgi.class.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/03 23:14:29 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/13 13:50:40 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/server/Server.class.hpp"
#include "classes/HTTP/RequestHandler.class.hpp"
#include <csignal>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

void Server::_registerCgi(CgiContext&& cgiInfo) {
	const CgiContext cgiPreserve(cgiInfo, false);
	
	try {
		CgiContext cgiEmptyBuffers(cgiInfo, false);

		_registerCgiPipe(std::move(cgiInfo), POLLOUT);
		_registerCgiPipe(std::move(cgiEmptyBuffers), POLLIN);

		write(1, "\033[0;32mCGI registered -> input: \033[0m", 37);
		char buf[12];
		auto [iPtr, ec1] = std::to_chars(buf, buf + 12, cgiPreserve.writePipeFd);
		write(1, buf, iPtr - buf);
		write(1, "\033[0;32m, output: \033[0m", 22);
		auto [oPtr, ec2] = std::to_chars(buf, buf + 12, cgiPreserve.readPipeFd);
		write(1, buf, oPtr - buf);
		write(1, "\n", 2);
	}
	catch (const std::logic_error&) {
		_actOnFailedCgiRegistration(cgiPreserve);
		_cleanupCompleteCgi(cgiPreserve);
		throw;
	} catch (...) {
		_actOnFailedCgiRegistration(cgiPreserve);
		_cleanupCompleteCgi(cgiPreserve);
		std::cerr << "Runtime warning: failed to register CGI pipe FD context -> killed CGI process" << std::endl;
	}
}

void Server::_actOnFailedCgiRegistration(const CgiContext& cgi) {
	Client& owningClient = _fdCtx.at(cgi.clientFd).client.value();

	ResponseBuilder rb;
	rb.generateErrorPage(500, owningClient.getServer());

	cgi.responseSlot->response = rb.buildString();
	cgi.responseSlot->ready = true;

	owningClient.flushReadyResponses();

	if (owningClient.hasPendingOutput()) {
		size_t clientEntryIdx = _findPollEntryIdxByFd(cgi.clientFd);
		if (clientEntryIdx == _pollMonitoring.size())
			throw std::logic_error("owning client fd not found");

		_pollMonitoring[clientEntryIdx].events |= POLLOUT;
	}
}

void Server::_registerCgiPipe(CgiContext&& cgiInfo, const short events) {
	if (events != POLLIN && events != POLLOUT)
		throw std::runtime_error("tried to register CGI pipe with unsupported poll event");

	const int pipeFd = (events == POLLIN ? cgiInfo.readPipeFd : cgiInfo.writePipeFd);

	const auto emplResult = _fdCtx.try_emplace(pipeFd, std::move(cgiInfo));
	bool inserted = emplResult.second;

	if (!inserted)
		throw std::logic_error("attempted to register CGI pipe with duplicate file descriptor");

	pollfd cgiPipe = {};
	cgiPipe.fd = pipeFd;
	cgiPipe.events = events;
	_pollMonitoring.push_back(cgiPipe);
}

void Server::_handleCgiInput(CgiContext& cgi, size_t& idx) {
	if (cgi.writeProgress == cgi.input.size()) {
		_disconnectFd(idx);
		return;
	}

	const size_t bytesRemaining = cgi.input.size() - cgi.writeProgress;
	const size_t nBytesToWrite = (bytesRemaining > MSG_SIZE ? MSG_SIZE : bytesRemaining);

	ssize_t written = write(
						cgi.writePipeFd,
						cgi.input.c_str() + cgi.writeProgress,
						nBytesToWrite
					);

	if (written <= 0) {
		std::cerr << "Runtime warning: write() to CGI child process failed, CGI process stopped for client: " << cgi.clientFd << std::endl;
		_cleanupCompleteCgi(cgi);
		return;
	} else
		cgi.writeProgress += written;

	if (cgi.writeProgress == cgi.input.size())
		_disconnectFd(idx);
}

void Server::_handleCgiOutput(CgiContext& cgi, size_t& idx) {
	if (!(_pollMonitoring[idx].revents & (POLLIN | POLLHUP)))
		return;

	auto it = _fdCtx.find(cgi.clientFd);
	if (it == _fdCtx.end()) {
		_cleanupCompleteCgi(cgi);
		return;
	}
	Client& owningClient = it->second.client.value();

	size_t clientEntryIdx = _findPollEntryIdxByFd(cgi.clientFd);
	if (clientEntryIdx == _pollMonitoring.size())
		throw std::logic_error("owning client fd not found");

	if (cgi.aborted) {
		ResponseBuilder rb;
		rb.generateErrorPage(504, owningClient.getServer());

		cgi.responseSlot->response = rb.buildString();
		cgi.responseSlot->ready = true;

		owningClient.flushReadyResponses();

		if (owningClient.hasPendingOutput())
			_pollMonitoring[clientEntryIdx].events |= POLLOUT;

		_cleanupCompleteCgi(cgi);
		return;
	}

	char buffer[MSG_SIZE];

	ssize_t bytes = read(cgi.readPipeFd, buffer, sizeof(buffer));

	if (bytes > 0) {
		cgi.output.append(buffer, bytes);
		return;
	}

	if (bytes == 0) {
		int status = 0;

		if (waitpid(cgi.pid, &status, WNOHANG) == 0) {
			kill(cgi.pid, SIGKILL);
			waitpid(cgi.pid, &status, 0);
		}

		if (WEXITSTATUS(status) != 0) {
			ResponseBuilder rb;
			rb.generateErrorPage(502, owningClient.getServer());

			cgi.responseSlot->response = rb.buildString();
		} else
			cgi.responseSlot->response = RequestHandler::buildResponseFromCgiOutput(cgi.output, owningClient.getServer());

		cgi.responseSlot->ready = true;

		owningClient.flushReadyResponses();

		if (owningClient.hasPendingOutput())
			_pollMonitoring[clientEntryIdx].events |= POLLOUT;

		_disconnectFd(idx);
		return;
	}

	perror("read");
	_cleanupCompleteCgi(cgi);
}

void Server::_cleanupCompleteCgi(const CgiContext& cgiInfo) {
	const pid_t pid = cgiInfo.pid;
	const int readPipe = cgiInfo.readPipeFd;
	const int writePipe = cgiInfo.writePipeFd;
	const bool aborted = cgiInfo.aborted;

	_fdCtx.erase(readPipe);
	_fdCtx.erase(writePipe);

	for (size_t i = 0; i < _pollMonitoring.size();) {
		if (_pollMonitoring[i].fd == readPipe ||
			_pollMonitoring[i].fd == writePipe)
		{
			_pollMonitoring[i] = _pollMonitoring.back();
			_pollMonitoring.pop_back();
			continue;
		}

		++i;
	}

	if (!aborted)
		kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);

	close(readPipe);
	close(writePipe);
}
