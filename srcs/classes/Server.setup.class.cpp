/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.setup.class.cpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/03 23:11:44 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/03 23:32:04 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Server.class.hpp"
#include <fcntl.h>
#include <memory>
#include <unistd.h>
// #include "classes/RequestHandler.class.hpp"
// #include "classes/ResponseBuilder.class.hpp"
// #include "signals.hpp"
// #include <iostream>
// #include <netdb.h>
// #include <sys/wait.h>
// #include <variant>

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
