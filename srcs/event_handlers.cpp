/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   event_handlers.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/20 23:21:39 by eberkau           #+#    #+#             */
/*   Updated: 2026/05/21 00:09:07 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Client.class.hpp"
#include <arpa/inet.h>
#include <iostream>
#include <fcntl.h>
#include <poll.h>

constexpr size_t MSG_SIZE = 64;

void handlePendConnects(
	int listeningSocket,
	std::vector<pollfd>& pollfdArr,
	std::unordered_map<int, Client>& clients
) {
	while (true) {
		sockaddr_in clientAddr = {};
		socklen_t clientLen = sizeof(clientAddr);
		
		const int clientFd = accept(listeningSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
		if (clientFd == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			perror("accept");
			throw 1;
		}
		const int currentFlags = fcntl(clientFd, F_GETFL);
		fcntl(clientFd, F_SETFL, currentFlags | O_NONBLOCK);

		std::cout << "client accepted" << std::endl;
		
		pollfd clientPoll;
		clientPoll.fd = clientFd;
		clientPoll.events = POLLIN;
		pollfdArr.push_back(clientPoll);

		clients.try_emplace(clientFd, clientFd, clientAddr);

		std::cout << "client registered: " << clientFd << std::endl;
	}
}

void handleIncomingData(
	std::vector<pollfd>& pollfdArr,
	size_t& idx,
	std::unordered_map<int, Client>& clients
) {
	char msg[MSG_SIZE];
	bzero(&msg, MSG_SIZE);

	if (0 == recv(pollfdArr[idx].fd, &msg, MSG_SIZE, 0)) {
		std::cout << pollfdArr[idx].fd << " disconnected" << std::endl;
		
		clients.erase(pollfdArr[idx].fd);
		
		pollfdArr[idx] = pollfdArr.back();
		pollfdArr.pop_back();
		--idx;

		return;
	}

	std::cout << "Message arrived from " << pollfdArr[idx].fd << " -> " << msg << std::endl;
	bzero(&msg, MSG_SIZE);
}
