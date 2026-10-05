/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.client.class.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/03 23:13:08 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/05 04:59:10 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Server.class.hpp"
#include "signals.hpp"
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
// #include "classes/RequestHandler.class.hpp"
// #include "classes/ResponseBuilder.class.hpp"
// #include <memory>
// #include <netdb.h>
// #include <sys/wait.h>
// #include <variant>

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

void Server::_handleIncomingData(size_t& idx) {
	char msg[MSG_SIZE];
	Client& currClient = _fdCtx.at(_pollfdArr[idx].fd).client.value();

	ssize_t receivedBytes = recv(currClient.getFd(), msg, MSG_SIZE, 0);

	if (receivedBytes == -1) {
		perror("recv");
		throw std::runtime_error("recv() failed");
	} else if (receivedBytes == 0) {
		_disconnectFd(idx);
		return;
	}

	currClient.appendToInBuf(msg, receivedBytes);

	std::cout << "Message arrived from " << _pollfdArr[idx].fd << " -> " << currClient.getInBuf() << std::endl; // BUG: remove later

	std::queue<RequestResult> reqResults = currClient.processInput();

	while (!reqResults.empty()) {
		RequestResult& singleResult = reqResults.front();
		
		if (std::holds_alternative<std::string>(singleResult.result)) {
			singleResult.responseSlot->response = std::move(std::get<std::string>(singleResult.result));
			singleResult.responseSlot->ready = true;
		}
		else {
			CgiContext& cgi = std::get<CgiContext>(singleResult.result);

			cgi.clientFd = currClient.getFd();
			cgi.responseSlot = singleResult.responseSlot;

			_registerCgi(cgi);
		}

		reqResults.pop();
	}
	currClient.flushReadyResponses();

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
			_disconnectFd(idx);
			return;
		}

		currClient.clearOutBuf();
		_pollfdArr[idx].events &= ~POLLOUT;
	}
}
