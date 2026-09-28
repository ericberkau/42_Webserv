/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 23:02:21 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/28 02:40:58 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Server.class.hpp"
#include "classes/RequestHandler.class.hpp"
#include "classes/ResponseBuilder.class.hpp"
#include "signals.hpp"
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <unistd.h>
#include <sys/wait.h>

Server::Server(const Config& config) : _config(config) {}

Server::~Server(void) {
	for (auto& [fd, ctx] : _fdCtx) {
		if (ctx.type == SocketType::CGI_PIPE && ctx.pid > 0) {
			kill(ctx.pid, SIGKILL);
			waitpid(ctx.pid, NULL, 0);
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
				continue;

			_handleSocketEvent(i);
			--ready;
		}

		// updatePolloutMonitoring(pollfdArr, clients); // FIXME: maybe do the updating later when data is queued/fully written to avoid looping
	}

	std::cout << "\nServer shutting down ..." << std::endl;
}

void Server::_setupListeners(void) {
	for (const ServerConfig& serverCfg : _config.servers) {
		for (const ListenDirective& listenCfg : serverCfg.listen) {
			int listenerFd = _setupListeningSocket(listenCfg);

			bool inserted = false;
			try {
				const auto result = _fdCtx.emplace(listenerFd, FdContext(&serverCfg));
				inserted = result.second;
			} catch (...) {
				close(listenerFd);
				throw;
			}

			if (!inserted)
				throw std::logic_error("attempted to register duplicate listener file descriptor");
		}
	}
}

int Server::_setupListeningSocket(const ListenDirective& listenCfg) {
	auto resolvedAddr = std::unique_ptr<addrinfo, void(*)(addrinfo*)>(
							resolveAddress(listenCfg.host, listenCfg.port),
							freeaddrinfo
						);
	sockaddr_in* serverAddr = reinterpret_cast<sockaddr_in*>(resolvedAddr->ai_addr);

	uint32_t ip = ntohl(serverAddr->sin_addr.s_addr);
	uint16_t port = ntohs(serverAddr->sin_port);

	if (_isDuplicateAddress(ip, port)) {
		throw std::runtime_error(
			"duplicate listen address '"
			+ listenCfg.host + ":" + std::to_string(listenCfg.port)
			+ "'"
		);
	}

	int serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd < 0) {
		perror("socket");
		throw std::runtime_error("socket() failed");
	}

	const int enableSetting = true;
	setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &enableSetting, sizeof(enableSetting)); // TODO: check return value

	const int currentFlags = fcntl(serverFd, F_GETFL);
	if (currentFlags == -1 || fcntl(serverFd, F_SETFL, currentFlags | O_NONBLOCK) == -1) {
		close(serverFd);
		perror("fcntl");
		throw std::runtime_error("fcntl() failed to configure server socket");
	}

	if (0 != bind(serverFd, resolvedAddr->ai_addr, resolvedAddr->ai_addrlen)) {
		close(serverFd);
		perror("bind");
		throw std::runtime_error("bind() failed");
	}
	if (0 != listen(serverFd, MAX_PEND_CONNECT_QUEUE)) {
		close(serverFd);
		perror("listen");
		throw std::runtime_error("listen() failed");
	}

	pollfd serverPoll;
	serverPoll.fd = serverFd;
	serverPoll.events = POLLIN;
	_pollfdArr.push_back(serverPoll); // FIXME: might fail, consider this in cleanup

	return serverFd;
}

bool Server::_isDuplicateAddress(uint32_t host, uint16_t port) {
	return !_usedAddresses.insert(std::make_pair(host, port)).second;
}

void Server::_registerCgiPipe(const PendingCgi& cgiInfo) {
	bool inserted = false;
	bool duplicate = false;
	try {
		const auto emplResult = _fdCtx.emplace(cgiInfo.pipeFd, FdContext(cgiInfo));
		inserted = emplResult.second;

		if (!inserted) {
			duplicate = true;
			throw std::logic_error("attempted to register CGI pipe with duplicate file descriptor");
		}

		pollfd cgiPipe;
		cgiPipe.fd = cgiInfo.pipeFd;
		cgiPipe.events = POLLIN;
		_pollfdArr.push_back(cgiPipe);
	} catch (...) {
		close(cgiInfo.pipeFd);
		kill(cgiInfo.pid, SIGKILL);
		waitpid(cgiInfo.pid, NULL, 0);
		if (inserted)
			_fdCtx.erase(cgiInfo.pipeFd);

		if (duplicate)
			throw;

		std::cerr << "Runtime warning: failed to register CGI pipe FD context -> killed CGI process" << std::endl; // FIXME: maybe send error 500 to client and disconnect?
	}
}

void Server::_handleSocketEvent(size_t& idx) {
	FdContext& currFd = _fdCtx.at(_pollfdArr[idx].fd);

	if (_pollfdArr[idx].revents & (POLLERR | POLLHUP | POLLNVAL)) {
		switch (currFd.type) {
			case SocketType::LISTENER:
				throw std::runtime_error("listening socket failed");

			case SocketType::CLIENT:
				_disconnectClient(idx);
				return;

			case SocketType::CGI_PIPE:
				// BUG: don't disconnect, pipe may have remaining bytes
				return;

			default:
				throw std::logic_error("unknown socket type failed");
		}
	}

	switch (currFd.type) {
		case SocketType::LISTENER:
			_handleListenerEvents(idx);
			break;

		case SocketType::CLIENT:
			_handleClientEvents(idx);
			break;

		case SocketType::CGI_PIPE:
			_handleCgiOutput(idx);
			break;

		default:
			throw std::logic_error("event for unknown socket type");
	}
}

void Server::_handleListenerEvents(size_t idx) {
	if (_pollfdArr[idx].revents & POLLIN)
		_handlePendingConnections(_pollfdArr[idx].fd);
}

void Server::_handlePendingConnections(int listeningSocket) {
	while (!g_shutdownRequested) {
		sockaddr_in newClientAddr = {};
		socklen_t newClientLen = sizeof(newClientAddr);
		
		const int newClientFd = accept(listeningSocket, reinterpret_cast<sockaddr*>(&newClientAddr), &newClientLen);
		if (newClientFd == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			perror("accept");
			throw std::runtime_error("accept() failed");
		}

		const int currentFlags = fcntl(newClientFd, F_GETFL); // TODO: check return value for fail
		if (currentFlags == -1 || fcntl(newClientFd, F_SETFL, currentFlags | O_NONBLOCK) == -1) {
			close(newClientFd);
			perror("fcntl");
			throw std::runtime_error("fcntl() failed to configure client socket");
		};

		std::cout << "Client accepted" << std::endl;

		pollfd newClientPoll;
		newClientPoll.fd = newClientFd;
		newClientPoll.events = POLLIN;
		_pollfdArr.push_back(newClientPoll);

		const ServerConfig*	relatedServer = _fdCtx.at(listeningSocket).server;

		bool inserted = false;
		try {
			const auto result = _fdCtx.emplace(
									newClientFd,
									FdContext(
										relatedServer,
										Client(
											newClientFd,
											newClientAddr,
											relatedServer
										)
									)
								);

			inserted = result.second;
		} catch (...) {
			close(newClientFd);
			throw;
		}

		if (!inserted)
			throw std::logic_error("attempted to register duplicate client file descriptor");

		std::cout << "Client registered: " << newClientFd << std::endl;
	}
}

void Server::_handleClientEvents(size_t& idx) {
	size_t originalIdx = idx;

	if (_pollfdArr[idx].revents & POLLIN)
		_handleIncomingData(idx);

	/** At this point, the client might have disconnected (idx modified in subfunction)
	 * with still pending outgoing data, originalIdx checks if a diconnect occured.
	 * */
	if (idx != originalIdx)
		return;

	if (_pollfdArr[idx].revents & POLLOUT)
		_handleOutgoingData(idx);
}

void Server::_handleIncomingData(size_t& idx) {
	char msg[MSG_SIZE];
	Client& currClient = _fdCtx.at(_pollfdArr[idx].fd).client.value();

	ssize_t receivedBytes = recv(currClient.getFd(), msg, MSG_SIZE, 0);

	if (receivedBytes == -1) {
		perror("recv");
		throw std::runtime_error("recv() failed");
	} else if (receivedBytes == 0) {
		_disconnectClient(idx);
		return;
	}

	currClient.appendToInBuf(msg, receivedBytes);

	std::cout << "Message arrived from " << _pollfdArr[idx].fd << " -> " << currClient.getInBuf() << std::endl; // BUG: remove later

	std::queue<RequestResult> reqResults = currClient.processInput();

	while (!reqResults.empty()) {
		RequestResult& singleResult = reqResults.front();
		
		if (singleResult.response)
			currClient.appendToOutBuf(singleResult.response.value()); // FIXME: consider adding a transaction queue to keep proper request<->response order grouping
		else if (singleResult.cgi) {
			singleResult.cgi->clientFd = currClient.getFd(); // FIXME: maybe remove clientFd from this struct and only add in FdContext?

			_registerCgiPipe(singleResult.cgi.value());
		}

		reqResults.pop();
	}

	if (currClient.hasPendingOutput())
		_pollfdArr[idx].events |= POLLOUT;

	if (currClient.shouldDisconnect())
		_pollfdArr[idx].events &= ~POLLIN;
}

void Server::_handleOutgoingData(size_t& idx) {
	Client& currClient = _fdCtx.at(_pollfdArr[idx].fd).client.value();

	ssize_t sentBytes = send(
					currClient.getFd(),
					currClient.getOutBuf().data() + currClient.getOutProgress(),
					currClient.getOutBuf().size() - currClient.getOutProgress(),
					MSG_NOSIGNAL
				);

	if (sentBytes == -1) {
		perror("send");
		throw std::runtime_error("send() failed");
	}
	else if (sentBytes == 0) {
		std::cerr << "Logic warning: Sent empty output to client " << currClient.getFd() << std::endl;
	}

	currClient.advanceOutProgress(sentBytes);

	if (!currClient.hasPendingOutput()) {
		if (currClient.shouldDisconnect()) {
			_disconnectClient(idx);
			return;
		}

		currClient.clearOutBuf();
		_pollfdArr[idx].events &= ~POLLOUT;
	}
}

void Server::_handleCgiOutput(size_t& idx) {
	if (!(_pollfdArr[idx].revents & (POLLIN | POLLHUP)))
		return;

	FdContext& currCgi = _fdCtx.at(_pollfdArr[idx].fd);
	Client& owningClient = _fdCtx.at(currCgi.owningClientFd).client.value();

	char buffer[MSG_SIZE];
	std::string output;

	while (true) {
		ssize_t bytes = read(_pollfdArr[idx].fd, buffer, sizeof(buffer));

		if (bytes > 0)
			output.append(buffer, bytes);
		else if (bytes == 0) {
			owningClient.appendToOutBuf(
				RequestHandler::buildResponseFromCgiOutput(output)
			);

			size_t clientIdx = _findPollEntryIdxByFd(currCgi.owningClientFd);
			if (clientIdx == _pollfdArr.size())
				throw std::logic_error("owning client fd not found");

			_pollfdArr[clientIdx].events |= POLLOUT;

			_disconnectClient(idx);
			return;
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;

			perror("read");
			_disconnectClient(idx);
			return;
		}
	}
	// FIXME: change to incremental reading instead of draining
}

void Server::_disconnectClient(size_t& idx) {
	std::cout << "Client " << _pollfdArr[idx].fd << " disconnected" << std::endl;

	const int fd = _pollfdArr[idx].fd;
	_fdCtx.erase(fd);
	close(fd);

	_pollfdArr[idx] = _pollfdArr.back();
	_pollfdArr.pop_back();
	--idx;
}

size_t Server::_findPollEntryIdxByFd(int fd) {
	for (size_t i = 0; i < _pollfdArr.size(); ++i) {
		if (_pollfdArr[i].fd == fd)
			return i;
	}
	return _pollfdArr.size();
}
