/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_loop.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/20 22:02:11 by eberkau           #+#    #+#             */
/*   Updated: 2026/05/21 00:03:49 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Client.class.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>

constexpr int FOREVER = -1;

int setupListeningSocket(std::vector<pollfd>& pollfdArr) {
	int serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd < 0) {
		perror("socket");
		throw 1;
	}

	const int enableSetting = true;
	setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &enableSetting, sizeof(enableSetting));
	const int currentFlags = fcntl(serverFd, F_GETFL);
	fcntl(serverFd, F_SETFL, currentFlags | O_NONBLOCK);

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8080);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (0 != bind(serverFd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr))) {
		perror("bind");
		throw 1;
	}

	pollfd serverPoll;
	serverPoll.fd = serverFd;
	serverPoll.events = POLLIN;
	pollfdArr.push_back(serverPoll);

	return serverFd;
}

void handlePendConnects(
	int serverFd,
	std::vector<pollfd>& pollfdArr,
	std::unordered_map<int, Client>& clients
);
void handleIncomingData(
	std::vector<pollfd>& pollfdArr,
	size_t& idx,
	std::unordered_map<int, Client>& clients
);

void serverLoop() {
	std::vector<pollfd> pollfdArr;

	const int serverFd = setupListeningSocket(pollfdArr);

	std::unordered_map<int, Client> clients;

	if (0 != listen(serverFd, 32)) {
		perror("listen");
		throw 1;
	}

	while (true) {
		std::cout << "listening" << std::endl;

		int ready = poll(pollfdArr.data(), pollfdArr.size(), FOREVER);

		for (size_t i = 0; i < pollfdArr.size() && ready > 0; ++i) {
			if (pollfdArr[i].fd == serverFd && pollfdArr[i].revents & POLLIN) {
				handlePendConnects(serverFd, pollfdArr, clients);
				--ready;
			}
			else if (pollfdArr[i].fd != serverFd && pollfdArr[i].revents & POLLIN) {
				handleIncomingData(pollfdArr, i, clients);
				--ready;
			}
		}
	}
}
