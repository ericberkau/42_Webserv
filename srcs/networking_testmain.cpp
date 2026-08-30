/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   networking_testmain.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:21:49 by eberkau           #+#    #+#             */
/*   Updated: 2026/05/20 23:12:31 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// FIXME: this is a testmain for experimenting and needs to be removed or the code included in the actual main function

#include "classes/Client.class.hpp"
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <unordered_map>

// constexpr int FOREVER = -1;

void serverLoop();

int main()
{
	serverLoop();

	// int serverFd = socket(AF_INET, SOCK_STREAM, 0);
	// if (serverFd < 0) {
	// 	perror("socket");
	// 	return 1;
	// }
	// const int settingEnable = true;
	// setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &settingEnable, sizeof(settingEnable));

	// sockaddr_in serverAddr = {};
	// serverAddr.sin_family = AF_INET;
	// serverAddr.sin_port = htons(8080);
	// serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// if (0 != bind(serverFd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr))) {
	// 	perror("bind");
	// 	return 1;
	// }

	// if (0 != listen(serverFd, 32)) {
	// 	perror("listen");
	// 	return 1;
	// }

	// int currentFlags = fcntl(serverFd, F_GETFL);
	// fcntl(serverFd, F_SETFL, currentFlags | O_NONBLOCK);

	// pollfd serverPoll;
	// serverPoll.fd = serverFd;
	// serverPoll.events = POLLIN;

	// std::vector<pollfd> pollfdArr;
	// pollfdArr.push_back(serverPoll);

	// std::unordered_map<int, Client> clients;

	// while (true) {
	// 	std::cout << "listening" << std::endl;

	// 	int ready = poll(pollfdArr.data(), pollfdArr.size(), FOREVER);

	// 	for (size_t i = 0; i < pollfdArr.size() && ready > 0; ++i) {
	// 		if (pollfdArr[i].fd == serverFd && pollfdArr[i].revents & POLLIN) {
	// 			sockaddr_in clientAddr = {};
	// 			socklen_t clientLen = sizeof(clientAddr);
				
	// 			int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
	// 			if (clientFd == -1) {
	// 				perror("accept");
	// 				return 1;
	// 			}
	// 			currentFlags = fcntl(clientFd, F_GETFL);
	// 			fcntl(clientFd, F_SETFL, currentFlags | O_NONBLOCK);

	// 			std::cout << "client accepted" << std::endl;
				
	// 			pollfd clientPoll;
	// 			clientPoll.fd = clientFd;
	// 			clientPoll.events = POLLIN;
				
	// 			clients.try_emplace(clientFd, clientFd, clientAddr);
	// 			pollfdArr.push_back(clientPoll);
	// 			std::cout << "client registered: " << clientFd << std::endl;
	// 			--ready;
	// 		}
	// 		else if (pollfdArr[i].fd != serverFd && pollfdArr[i].revents & POLLIN) {
	// 			char msg[64];
	// 			if (0 == recv(pollfdArr[i].fd, &msg, 64, 0)) {
	// 				std::cout << pollfdArr[i].fd << " disconnected" << std::endl;
	// 				clients.erase(pollfdArr[i].fd);
	// 				pollfdArr[i] = pollfdArr.back();
	// 				pollfdArr.pop_back();
	// 				--i;
	// 				continue;
	// 			}

	// 			std::cout << "Message arrived from: " << pollfdArr[i].fd << " -> " << msg << std::endl;
	// 			bzero(&msg, 64);
	// 			--ready;
	// 		}
	// 	}
	// }


	
	// const std::string str("Hello World!");
	// testUser.appendToOutBuf(str, sizeof(str));
	// send(testUser.getFd(), &(testUser.getOutBuf()), sizeof(testUser.getOutBuf()), 0);
	
	// sleep(10);

	// char backMsg[32];
	// recv(testUser.getFd(), &backMsg, 32, 0);

	// testUser.appendToInBuf(backMsg, sizeof(backMsg));

	// std::cout << backMsg << std::endl;
	// std::cout << testUser.getInBuf() << std::endl;

	// send(testUser.getFd(), "okayyy lets go\n", 32, 0);
	// bzero(&backMsg, 32);
	// recv(testUser.getFd(), &backMsg, 32, 0);
	// std::cout << backMsg << std::endl;

	return 0;
}
