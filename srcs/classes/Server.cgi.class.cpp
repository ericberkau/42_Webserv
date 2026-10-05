/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cgi.class.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/03 23:14:29 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/05 05:58:34 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Server.class.hpp"
#include "classes/RequestHandler.class.hpp"
#include <csignal>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
// #include "classes/ResponseBuilder.class.hpp"
// #include <fcntl.h>
// #include <memory>
// #include <netdb.h>
// #include <variant>

void Server::_registerCgi(CgiContext&& cgiInfo) {
	CgiContext cgiPreserve(cgiInfo, false);
	
	try {
		CgiContext cgiEmptyBuffers(cgiInfo, false);

		_registerCgiPipe(std::move(cgiInfo), POLLOUT);
		_registerCgiPipe(std::move(cgiEmptyBuffers), POLLIN);
	} catch (const std::logic_error&) {
		_cleanupCompleteCgi(cgiPreserve);
		throw;
	} catch (...) {
		_cleanupCompleteCgi(cgiPreserve);
		std::cerr << "Runtime warning: failed to register CGI pipe FD context -> killed CGI process" << std::endl; // FIXME: maybe send error 500 to client and disconnect?
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
	_pollfdArr.push_back(cgiPipe);
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
	if (!(_pollfdArr[idx].revents & (POLLIN | POLLHUP)))
		return;

	auto it = _fdCtx.find(cgi.clientFd);
	if (it == _fdCtx.end()) {
		_cleanupCompleteCgi(cgi);
		return;
	}
	Client& owningClient = it->second.client.value();

	char buffer[MSG_SIZE];

	ssize_t bytes = read(cgi.readPipeFd, buffer, sizeof(buffer));

	printf("bytes: %zd\n", bytes); // BUG: for debugging, remove later!
	// printf("buffer: %s\n", buffer); // BUG: for debugging, remove later!

	if (bytes > 0) {
		cgi.output.append(buffer, bytes);
		return;
	}

	if (bytes == 0) {
		cgi.responseSlot->response = RequestHandler::buildResponseFromCgiOutput(cgi.output);
		cgi.responseSlot->ready = true;

		owningClient.flushReadyResponses();

		size_t clientIdx = _findPollEntryIdxByFd(cgi.clientFd);
		if (clientIdx == _pollfdArr.size())
			throw std::logic_error("owning client fd not found");

		if (owningClient.hasPendingOutput())
			_pollfdArr[clientIdx].events |= POLLOUT;

		if (waitpid(cgi.pid, NULL, WNOHANG) == 0) {
			kill(cgi.pid, SIGKILL);
			waitpid(cgi.pid, NULL, 0);
		}
		_disconnectFd(idx);
		return;
	}

	perror("read");
	_cleanupCompleteCgi(cgi);
}

void Server::_cleanupCompleteCgi(const CgiContext& cgiInfo) {
	_fdCtx.erase(cgiInfo.readPipeFd);
	_fdCtx.erase(cgiInfo.writePipeFd);

	for (size_t i = 0; i < _pollfdArr.size();) {
		if (_pollfdArr[i].fd == cgiInfo.readPipeFd ||
			_pollfdArr[i].fd == cgiInfo.writePipeFd)
		{
			_pollfdArr[i] = _pollfdArr.back();
			_pollfdArr.pop_back();
			continue;
		}

		++i;
	}

	close(cgiInfo.readPipeFd);
	close(cgiInfo.writePipeFd);
	kill(cgiInfo.pid, SIGKILL);
	waitpid(cgiInfo.pid, NULL, 0);
}
