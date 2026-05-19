/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   networking_testmain.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:21:49 by eberkau           #+#    #+#             */
/*   Updated: 2026/05/20 00:10:18 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// FIXME: this is a testmain for experimenting and needs to be removed or the code included in the actual main function

#include "classes/Client.class.hpp"
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
#include <iostream>
#include <unistd.h>

int main() {

	int serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd < 0) {
		perror("socket");
		return 1;
	}

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8080);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (0 != bind(serverFd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr))) {
		perror("bind");
		return 1;
	}

	if (0 != listen(serverFd, 32)) {
		perror("listen");
		return 1;
	}

	std::cout << "listening" << std::endl;

	sockaddr_in clientAddr = {};
	socklen_t clientLen = sizeof(clientAddr);
	int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
	if (clientFd == -1) {
		perror("accept");
		return 1;
	}
	std::cout << "client accepted" << std::endl;

	Client testUser(clientFd, clientAddr);

	
	const std::string str("Hello World!");
	testUser.appendToOutBuf(str, sizeof(str));
	send(testUser.getFd(), &(testUser.getOutBuf()), sizeof(testUser.getOutBuf()), 0);
	
	sleep(10);

	char backMsg[32];
	recv(testUser.getFd(), &backMsg, 32, 0);

	testUser.appendToInBuf(backMsg, sizeof(backMsg));

	std::cout << backMsg << std::endl;
	std::cout << testUser.getInBuf() << std::endl;

	send(testUser.getFd(), "okayyy lets go\n", 32, 0);
	bzero(&backMsg, 32);
	recv(testUser.getFd(), &backMsg, 32, 0);
	std::cout << backMsg << std::endl;

	return 0;
}
