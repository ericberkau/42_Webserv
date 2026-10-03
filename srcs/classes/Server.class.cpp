/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 23:02:21 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/03 23:31:27 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Server.class.hpp"
#include "signals.hpp"
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
// #include "classes/RequestHandler.class.hpp"
// #include "classes/ResponseBuilder.class.hpp"
// #include <fcntl.h>
// #include <memory>
// #include <netdb.h>
// #include <variant>

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

	while (!g_shutdownRequested) {
		std::cout << "listening" << std::endl;

		int ready = poll(_pollfdArr.data(), _pollfdArr.size(), FOREVER);
		if (ready == -1) {
			if (errno == EINTR)
				continue;

			perror("poll");
			throw std::runtime_error("poll() failed");
		}

		for (size_t i = 0; i < _pollfdArr.size() && ready > 0; ++i) {
			if (!_pollfdArr[i].revents)
				continue; // TODO: maybe check for client timeouts for long idle times

			_handleSocketEvent(i);
			--ready;
		}

		// updatePolloutMonitoring(pollfdArr, clients); // FIXME: maybe do the updating later when data is queued/fully written to avoid looping
	}

	std::cout << "\nServer shutting down ..." << std::endl;
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

	printf("POLLERR: %i\nPOLLHUP: %i\n",
		revents & POLLERR ? 1 : 0,
		revents & POLLHUP ? 1 : 0
	); // BUG: for debugging, remove later!

	if (revents & POLLERR) {
		_cleanupCompleteCgi(cgi);
		return;
	}

	if (revents & (POLLIN | POLLHUP))
		_handleCgiOutput(cgi, idx);
	else if (revents & POLLOUT)
		_handleCgiInput(cgi, idx);
}

size_t Server::_findPollEntryIdxByFd(int fd) {
	for (size_t i = 0; i < _pollfdArr.size(); ++i) {
		if (_pollfdArr[i].fd == fd)
			return i;
	}
	return _pollfdArr.size();
}

void Server::_disconnectFd(size_t& idx) {
	const int fd = _pollfdArr[idx].fd;
	
	_fdCtx.erase(fd);
	close(fd);
	
	_pollfdArr[idx] = _pollfdArr.back();
	_pollfdArr.pop_back();
	--idx;

	std::cout << "FD " << fd << " disconnected" << std::endl;
}

