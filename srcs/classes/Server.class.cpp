/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 23:02:21 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/08 19:14:23 by eberkau          ###   ########.fr       */
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

Server::Server(const Config& config) : _config(config) {}

Server::~Server(void) {
	for (auto& [fd, ctx] : _fdCtx)
		close(fd);
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
				const auto result = _fdCtx.emplace(listenerFd, FdContext(SocketType::LISTENER, &serverCfg));
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
		throw 1;
	}

	const int enableSetting = true;
	setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &enableSetting, sizeof(enableSetting)); // TODO: check return value

	const int currentFlags = fcntl(serverFd, F_GETFL);
	fcntl(serverFd, F_SETFL, currentFlags | O_NONBLOCK); // TODO: check return value

	if (0 != bind(serverFd, resolvedAddr->ai_addr, resolvedAddr->ai_addrlen)) {
		close(serverFd);
		perror("bind");
		throw 1;
	}
	if (0 != listen(serverFd, MAX_PEND_CONNECT_QUEUE)) {
		close(serverFd);
		perror("listen");
		throw 1;
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

void Server::_handleSocketEvent(size_t& idx) {
	FdContext& currFd = _fdCtx.at(_pollfdArr[idx].fd);

	switch (currFd.type) {
		case SocketType::LISTENER:
			_handleListenerEvents(idx);
			break;

		case SocketType::CLIENT:
			_handleClientEvents(idx);
			break;

		case SocketType::CGI_PIPE:
			// TODO
			break;

		default:
			break;
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
			perror("accept()");
			throw 1;
		}

		const int currentFlags = fcntl(newClientFd, F_GETFL); // TODO: check return value for fail
		fcntl(newClientFd, F_SETFL, currentFlags | O_NONBLOCK);

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
										SocketType::CLIENT,
										relatedServer,
										Client(
											newClientFd,
											newClientAddr
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
	if (_pollfdArr[idx].revents & POLLOUT && idx == originalIdx)
		_handleOutgoingData(idx);
}

// void Server::_handleIncomingData(size_t& idx) {
// 	char msg[MSG_SIZE];
// 	Client& currClient = _fdCtx.at(_pollfdArr[idx].fd).client.value();
// 	ssize_t receivedBytes = -1;

// 	while (!g_shutdownRequested) {
// 		receivedBytes = recv(currClient.getFd(), msg, MSG_SIZE, 0);
// 		if (receivedBytes == -1) {
// 			if (errno == EAGAIN || errno == EWOULDBLOCK)
// 				break;
// 			else if (errno == EPIPE || errno == ECONNRESET) {
// 				_disconnectClient(idx);
// 				return;
// 			}
// 			perror("recv()");
// 			throw 1;
// 		}
// 		else if (receivedBytes == 0) {
// 			_disconnectClient(idx);
// 			return;
// 		}

// 		currClient.appendToInBuf(msg, receivedBytes);
// 	}

// 	_processClientInput(currClient);

// 	std::cout << "Message arrived from " << _pollfdArr[idx].fd << " -> " << currClient.getInBuf() << std::endl; // BUG: remove later
// 	currClient.appendToOutBuf(std::string("\nYou just sent -> " + currClient.getInBuf() + "\n")); // BUG: remove later
// 	_pollfdArr[idx].events |= POLLOUT; // BUG: remove later
// 	currClient.clearInBuf(); // BUG: remove later

// }


void Server::_handleIncomingData(size_t& idx) {
	char msg[MSG_SIZE];
	Client& currClient = _fdCtx.at(_pollfdArr[idx].fd).client.value();
	ssize_t receivedBytes = -1;

	while (!g_shutdownRequested) {
		receivedBytes = recv(currClient.getFd(), msg, MSG_SIZE, 0);
		if (receivedBytes == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else if (errno == EPIPE || errno == ECONNRESET) {
				_disconnectClient(idx);
				return;
			}
			perror("recv()");
			throw 1;
		}
		else if (receivedBytes == 0) {
			_disconnectClient(idx);
			return;
		}

		currClient.appendToInBuf(msg, receivedBytes);
	}

	std::cout << "Message arrived from " << _pollfdArr[idx].fd << " -> " << currClient.getInBuf() << std::endl; // BUG: remove later

	_processClientInput(currClient);

	// check if a response packet was successfully queued
	if (currClient.hasPendingOutput()) {
		_pollfdArr[idx].events |= POLLOUT; // Tell poll() we are ready to write to the socket
	}
}

void Server::_processClientInput(Client& client) {
	/**
	 * //TODO: check if enough
	 * Feed client input buffer into HTTP parser.
	 * Extract complete requests.
	 * Call _handleRequest() for each request.
	 */
	client.requestParser.feed(
		client.getInBuf(),
		client.httpRequest,
		_fdCtx.at(client.getFd()).server
	);
	client.clearInBuf();

	if (client.requestParser.getState() == RequestParser::PARSE_COMPLETE)
		_handleRequest(client);
}

void Server::_handleRequest(Client& client/*, HttpRequest*/) {
	/**
	 * //TODO: check if enough
	 * Match request against ServerConfig.
	 * Find location block.
	 * Generate response.
	 * Queue response into client output buffer.
	 */
	RequestHandler reqHandler(_fdCtx.at(client.getFd()).server);

	const std::string response = reqHandler.processRequest(client.httpRequest);

	client.appendToOutBuf(response);
	client.requestParser.reset();
}

void Server::_handleOutgoingData(size_t& idx) {
	Client& currClient = _fdCtx.at(_pollfdArr[idx].fd).client.value();
	ssize_t sentBytes = -1;

	while (!g_shutdownRequested) {
		if (!currClient.hasPendingOutput()) {
			currClient.clearOutBuf();
			_pollfdArr[idx].events &= ~POLLOUT;
			break;
		}

		sentBytes = send(currClient.getFd(),
						currClient.getOutBuf().data() + currClient.getOutProgress(),
						currClient.getOutBuf().size() - currClient.getOutProgress(),
						MSG_NOSIGNAL);

		if (sentBytes == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else if (errno == EPIPE || errno == ECONNRESET) {
				_disconnectClient(idx);
				return;
			}
			perror("send()");
			throw 1;
		}
		else if (sentBytes == 0) {
			std::cerr << "Sent empty output to client " << currClient.getFd() << std::endl;
		}

		currClient.advanceOutProgress(sentBytes);
	}
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
