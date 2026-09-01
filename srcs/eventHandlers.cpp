/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   eventHandlers.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/20 23:21:39 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/01 22:31:48 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "networking.hpp"
#include <iostream>
#include <fcntl.h>

// constexpr size_t MSG_SIZE = 8; // FIXME: provide proper size later

void handlePendConnects(
	int listeningSocket,
	std::vector<pollfd>& pollfdArr,
	std::unordered_map<int, FdContext>& fdCtx
) {
	while (true) {
		sockaddr_in newClientAddr = {};
		socklen_t newClientLen = sizeof(newClientAddr);
		
		const int newClientFd = accept(listeningSocket, reinterpret_cast<sockaddr*>(&newClientAddr), &newClientLen);
		if (newClientFd == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			perror("accept()");
			throw 1;
		}

		const int currentFlags = fcntl(newClientFd, F_GETFL);
		fcntl(newClientFd, F_SETFL, currentFlags | O_NONBLOCK);

		std::cout << "Client accepted" << std::endl;

		pollfd newClientPoll;
		newClientPoll.fd = newClientFd;
		newClientPoll.events = POLLIN;
		pollfdArr.push_back(newClientPoll);

		const ServerConfig*	relatedServer = fdCtx.at(listeningSocket).server;

		const auto result = fdCtx.emplace(
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

		if (!result.second)
			throw std::logic_error("attempted to register duplicate client file descriptor");

		std::cout << "Client registered: " << newClientFd << std::endl;
	}
}

static void disconnectClient(
	std::vector<pollfd>& pollfdArr,
	size_t& idx,
	std::unordered_map<int, FdContext>& fdCtx
) {
	std::cout << "Client " << pollfdArr[idx].fd << " disconnected" << std::endl;

	fdCtx.erase(pollfdArr[idx].fd);

	pollfdArr[idx] = pollfdArr.back();
	pollfdArr.pop_back();
	--idx;
}

void handleIncomingData(
	std::vector<pollfd>& pollfdArr,
	size_t& idx,
	std::unordered_map<int, FdContext>& fdCtx
) {
	char msg[MSG_SIZE];
	Client& currClient = fdCtx.at(pollfdArr[idx].fd).client.value();
	ssize_t receivedBytes = -1;

	while (true) {
		receivedBytes = recv(currClient.getFd(), msg, MSG_SIZE, 0);
		if (receivedBytes == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			perror("recv()");
			throw 1;
		}
		else if (receivedBytes == 0) {
			disconnectClient(pollfdArr, idx, fdCtx);
			return;
		}

		currClient.appendToInBuf(msg, receivedBytes);
	}

	std::cout << "Message arrived from " << pollfdArr[idx].fd << " -> " << currClient.getInBuf() << std::endl; // FIXME: remove later
	currClient.appendToOutBuf(std::string("\nYou just sent -> " + currClient.getInBuf() + "\n")); // FIXME: remove later
	pollfdArr[idx].events |= POLLOUT; // FIXME: remove later
	currClient.clearInBuf(); // FIXME: remove later
}

void handleOutgoingData(
	std::vector<pollfd>& pollfdArr,
	size_t& idx,
	std::unordered_map<int, FdContext>& fdCtx
) {
	Client& currClient = fdCtx.at(pollfdArr[idx].fd).client.value();
	ssize_t sentBytes = -1;

	while (true) {
		if (!currClient.hasPendingOutput()) {
			currClient.clearOutBuf();
			pollfdArr[idx].events &= ~POLLOUT;
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
				disconnectClient(pollfdArr, idx, fdCtx);
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
