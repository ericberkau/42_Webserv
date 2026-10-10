/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.client.class.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/03 23:13:08 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/10 03:00:29 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/server/Server.class.hpp"
#include "signals.hpp"
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

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

		if (fcntl(newClientFd, F_SETFL, O_NONBLOCK) == -1 ||
			fcntl(newClientFd, F_SETFD, FD_CLOEXEC) == -1) 
		{
			close(newClientFd);
			perror("fcntl");
			throw std::runtime_error("fcntl() failed to configure client socket");
		};

		// std::cout << "Client accepted" << std::endl; // BUG: remove?
		write(1, "Client accepted\n", 17);

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

		// std::cout << "Client registered: " << newClientFd << std::endl; // BUG: remove?
		write(1, "Client registered: ", 20);
		char buf[12];
		auto [p, ec] = std::to_chars(buf, buf + 12, newClientFd);
		write(1, buf, p - buf);
		write(1, "\n", 2);
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

	// std::cout << "Message arrived from " << _pollfdArr[idx].fd << " -> " << currClient.getInBuf() << std::endl; // BUG: remove later
	// write(1, "Message arrived from ", 22);
	// char buf[12];
	// auto [p, ec] = std::to_chars(buf, buf + 12, _pollfdArr[idx].fd);
	// write(1, buf, p - buf);
	// write(1, " -> ", 5);
	// write(1, "\n", 2);
	// write(1, currClient.getInBuf().c_str(), currClient.getInBuf().size() + 1);
	// write(1, "\n", 2);


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

			_registerCgi(std::move(cgi));
		}

		reqResults.pop();
	}
	currClient.flushReadyResponses();

	if (currClient.hasPendingOutput())
		_pollfdArr[idx].events |= POLLOUT;

	if (currClient.shouldDisconnect())
		_pollfdArr[idx].events &= ~POLLIN;

	currClient.updateActivity();
}

void Server::_handleOutgoingData(size_t& idx) {
	Client& currClient = _fdCtx.at(_pollfdArr[idx].fd).client.value();

	if (!currClient.hasPendingOutput()) {
		if (currClient.shouldDisconnect() && currClient.queueEmpty())
			_disconnectFd(idx);

		return;
	}

	const size_t bytesRemaining = currClient.getOutBuf().size() - currClient.getOutProgress();
	const size_t nBytesToWrite = (bytesRemaining > MSG_SIZE ? MSG_SIZE : bytesRemaining);

	ssize_t sentBytes = send(
					currClient.getFd(),
					currClient.getOutBuf().data() + currClient.getOutProgress(),
					nBytesToWrite,
					MSG_NOSIGNAL
				);

	// std::cout << "Response sent to client " << currClient.getFd() << " -> " << currClient.getOutBuf() << std::endl; // BUG: for debugging, remove later
	// write(1, "Response sent to ", 18);
	// char buf[12];
	// auto [p, ec] = std::to_chars(buf, buf + 12, currClient.getFd());
	// write(1, buf, p - buf);
	// write(1, " -> ", 5);
	// write(1, "\n", 2);
	// write(1, currClient.getOutBuf().c_str(), nBytesToWrite + 1);
	// write(1, "\n", 2);

	if (sentBytes == -1) {
		perror("send");
		throw std::runtime_error("send() failed");
	}
	else if (sentBytes == 0) {
		std::cerr << "Logic warning: Sent empty output to client " << currClient.getFd() << std::endl;
	}

	currClient.advanceOutProgress(sentBytes);

	if (!currClient.hasPendingOutput()) {
		if (currClient.shouldDisconnect() && currClient.queueEmpty()) {
			_disconnectFd(idx);
			return;
		}

		currClient.clearOutBuf();
		_pollfdArr[idx].events &= ~POLLOUT;
	}

	currClient.updateActivity();
}
