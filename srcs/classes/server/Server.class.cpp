/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 23:02:21 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/10 22:02:18 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/server/Server.class.hpp"
#include "signals.hpp"
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

Server::Server(const Config& config) : _config(config) {}

Server::~Server(void) {
	for (auto& [fd, ctx] : _fdCtx) {
		if (ctx.type == SocketType::CGI_PIPE && ctx.cgi->pid > 0) {
			kill(ctx.cgi->pid, SIGKILL);
			waitpid(ctx.cgi->pid, NULL, 0);
		}

		close(fd);
	}
}

void Server::run(void) {
	_setupListeners();

	_lastTimeoutCheck = Clock::now();
	size_t dots = 0;

	std::cout << "\033[0;34mThe server is starting, welcome!\033[0m\n" << std::endl;

	while (!g_shutdownRequested) {
		write(1, "\033[0;36mlistening\033[0m .....", 22 + (dots++ % 5));
		write(1, "\n", 2);

		int ready = poll(_pollfdArr.data(), _pollfdArr.size(), POLL_TIMEOUT_MS);
		if (ready == -1) {
			if (errno == EINTR)
				continue;

			perror("poll");
			throw std::runtime_error("poll() failed");
		}

		_handleTimeouts();

		for (size_t i = 0; i < _pollfdArr.size() && ready > 0; ++i) {
			if (!_pollfdArr[i].revents)
				continue;

			_handleSocketEvent(i);
			--ready;
		}
	}

	std::cout << "\n\n\033[0;31mServer shutting down ...\033[0m" << std::endl;
}

void Server::_handleTimeouts(void) {
	const Clock::time_point now = Clock::now();

	if (now - _lastTimeoutCheck < TIMEOUT_CHECK_INTERVAL)
		return;

	for (auto it = _fdCtx.begin(); it != _fdCtx.end(); ++it) {
		if (it->second.type == SocketType::CLIENT) {
			Client& client = it->second.client.value();

			if (now - client.getLastActivity() >= CLIENT_IDLE_TIMEOUT)
				_handleClientTimeout(client);
		}
		else if (it->second.type == SocketType::CGI_PIPE &&
			it->first == it->second.cgi->readPipeFd)
		{
			CgiContext& cgi = it->second.cgi.value();

			if (!cgi.aborted && now - cgi.startTime >= CGI_TIMEOUT) {
				cgi.aborted = true;
				kill(cgi.pid, SIGKILL);
			}
		}
	}

	_lastTimeoutCheck = now;
}

void Server::_handleClientTimeout(Client& client) {
	client.markForDisconnect();
	
	size_t clientEntryIdx = _findPollEntryIdxByFd(client.getFd());
	if (clientEntryIdx == _pollfdArr.size())
		throw std::logic_error("timed-out client fd not found");

	_pollfdArr[clientEntryIdx].events &= ~POLLIN;

	if (client.queueEmpty())
		_pollfdArr[clientEntryIdx].events |= POLLOUT;
}

void Server::_checkIdleClients(const Clock::time_point now) {
	for (auto it = _fdCtx.begin(); it != _fdCtx.end(); ++it) {
		if (it->second.type != SocketType::CLIENT)
			continue;

		Client& client = it->second.client.value();

		if (now - client.getLastActivity() < CLIENT_IDLE_TIMEOUT)
			continue;

		client.markForDisconnect();
		
		size_t clientEntryIdx = _findPollEntryIdxByFd(client.getFd());
		_pollfdArr[clientEntryIdx].events &= ~POLLIN;

		if (client.queueEmpty())
			_pollfdArr[clientEntryIdx].events |= POLLOUT;
	}
}

void Server::_handleSocketEvent(size_t& idx) {
	if (_pollfdArr[idx].revents & POLLNVAL)
		throw std::logic_error("poll() monitored invalid file descriptor");

	FdContext& currFd = _fdCtx.at(_pollfdArr[idx].fd);

	switch (currFd.type) {
		case SocketType::LISTENER:
			_handleListenerEvents(idx);
			break;

		case SocketType::CLIENT:
			_handleClientEvents(idx);
			break;

		case SocketType::CGI_PIPE:
			_handleCgiEvents(idx);
			break;

		default:
			throw std::logic_error("event for unknown socket type");
	}
}

void Server::_handleListenerEvents(size_t idx) {
	const short revents = _pollfdArr[idx].revents;

	if(revents & (POLLERR | POLLHUP))
		throw std::runtime_error("listening socket failed");

	if (_pollfdArr[idx].revents & POLLIN)
		_handlePendingConnections(_pollfdArr[idx].fd);
}


void Server::_handleClientEvents(size_t& idx) {
	const short revents = _pollfdArr[idx].revents;

	if(revents & (POLLERR | POLLHUP)) {
		_disconnectFd(idx);
		return;
	}

	size_t originalIdx = idx;

	if (revents & POLLIN)
		_handleIncomingData(idx);

	/** At this point, the client might have disconnected (idx modified in subfunction)
	 * with still pending outgoing data, originalIdx checks if a diconnect occured.
	 * */
	if (idx != originalIdx)
		return;

	if (revents & POLLOUT)
		_handleOutgoingData(idx);
}

void Server::_handleCgiEvents(size_t& idx) {
	const short revents = _pollfdArr[idx].revents;
	CgiContext& cgi = _fdCtx.at(_pollfdArr[idx].fd).cgi.value();

	if (revents & POLLERR) {
		_cleanupCompleteCgi(cgi);
		return;
	}

	if (revents & (POLLIN | POLLHUP))
		_handleCgiOutput(cgi, idx);
	else if (revents & POLLOUT)
		_handleCgiInput(cgi, idx);
}

size_t Server::_findPollEntryIdxByFd(int fd) const {
	for (size_t i = 0; i < _pollfdArr.size(); ++i) {
		if (_pollfdArr[i].fd == fd)
			return i;
	}
	return _pollfdArr.size();
}

void Server::_disconnectFd(size_t& idx) {
	const int fd = _pollfdArr[idx].fd;
	
	_fdCtx.erase(fd);
	
	_pollfdArr[idx] = _pollfdArr.back();
	_pollfdArr.pop_back();
	--idx;

	close(fd);

	write(1, "\033[0;31mFD\033[0m ", 15);
	char buf[12];
	auto [p, ec] = std::to_chars(buf, buf + 12, fd);
	write(1, buf, p - buf);
	write(1, " \033[0;31mdisconnected\033[0m\n", 26);
}
