/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serverLoop.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/20 22:02:11 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/01 22:27:36 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "networking.hpp"
#include "config.hpp"
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <set>
#include <unistd.h>

// constexpr int FOREVER = -1; // FIXME: remove
// constexpr int MAX_PEND_CONNECT_QUEUE = 32; // FIXME: remove

static bool isDuplicateAddress(
	std::set<std::pair<uint32_t, uint16_t>>& usedAddresses,
	uint32_t host,
	uint16_t port
) {
	return !usedAddresses.insert(std::make_pair(host, port)).second;
}

static int setupListeningSocket(
	const ListenDirective& listenCfg,
	std::vector<pollfd>& pollfdArr,
	std::set<std::pair<uint32_t, uint16_t>>& usedAddresses
) {
	auto resolvedAddr = std::unique_ptr<addrinfo, void(*)(addrinfo*)>(
							resolveAddress(listenCfg.host, listenCfg.port),
							freeaddrinfo
						);
	sockaddr_in* serverAddr = reinterpret_cast<sockaddr_in*>(resolvedAddr->ai_addr);

	uint32_t ip = ntohl(serverAddr->sin_addr.s_addr);
	uint16_t port = ntohs(serverAddr->sin_port);

	if (isDuplicateAddress(usedAddresses, ip, port)) {
		throw std::runtime_error(
			"duplicate listen address '"
			+ listenCfg.host + ":" + std::to_string(listenCfg.port)
			+ "'"
		);
	}

	int serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd < 0) {
		perror("socket");
		throw 1; // FIXME: clean open listening sockets before exiting
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
	pollfdArr.push_back(serverPoll); // FIXME: might fail, consider this in cleanup

	return serverFd;
}

// static void updatePolloutMonitoring(std::vector<pollfd>& pollfdArr, std::unordered_map<int, Client>& clients) {
// 	for (size_t i = 1; i < pollfdArr.size(); ++i) {
// 		if (clients.at(pollfdArr[i].fd).hasPendingOutput())
// 			pollfdArr[i].events |= POLLOUT;
// 		else
// 			pollfdArr[i].events &= ~POLLOUT;
// 	}
// }

static void handleListenerEvents(
	std::vector<pollfd>& pollfdArr,
	size_t& idx,
	std::unordered_map<int, FdContext>& fdCtx
) {
	if (pollfdArr[idx].revents & POLLIN)
		handlePendConnects(pollfdArr[idx].fd, pollfdArr, fdCtx);
}

static void handleClientEvents(
	std::vector<pollfd>& pollfdArr,
	size_t& idx,
	std::unordered_map<int, FdContext>& fdCtx
) {
	size_t originalIdx = idx;

	if (pollfdArr[idx].revents & POLLIN)
		handleIncomingData(pollfdArr, idx, fdCtx);

	/** At this point, the client might have disconnected (idx modified in subfunction)
	 * with still pending outgoing data, originalIdx checks if a diconnect occured. */
	if (pollfdArr[idx].revents & POLLOUT && idx == originalIdx)
		handleOutgoingData(pollfdArr, idx, fdCtx);
}

static void handleSingleSocket(
	std::vector<pollfd>& pollfdArr,
	size_t& idx,
	std::unordered_map<int, FdContext>& fdCtx
) {
	FdContext& currFd = fdCtx.at(pollfdArr[idx].fd);

	switch (currFd.type) {
		case SocketType::LISTENER:
			handleListenerEvents(pollfdArr, idx, fdCtx);
			break;

		case SocketType::CLIENT:
			handleClientEvents(pollfdArr, idx, fdCtx);
			break;

		case SocketType::CGI_PIPE:
			// TODO
			break;

		default:
			break;
	}
}

void serverLoop(const Config& config) {
	std::vector<pollfd> pollfdArr;
	std::unordered_map<int, FdContext> fdCtx;
	std::set<std::pair<uint32_t, uint16_t>> usedAddresses;
	int listenerFd = -1;

	try {
		for (const ServerConfig& serverCfg : config.servers) {
			for (const ListenDirective& listenCfg : serverCfg.listen) {
				listenerFd = setupListeningSocket(listenCfg, pollfdArr, usedAddresses);

				fdCtx.emplace(listenerFd, FdContext(SocketType::LISTENER, &serverCfg));
				listenerFd = -1;
			}
		}
	} catch (const std::exception& e) {
		for (auto& [fd, ctx] : fdCtx) {
			close(fd);
		}
		if (listenerFd != -1)
			close(listenerFd);

		throw;
	}

	while (true) {
		std::cout << "listening" << std::endl;

		int ready = poll(pollfdArr.data(), pollfdArr.size(), FOREVER);

		for (size_t i = 0; i < pollfdArr.size() && ready > 0; ++i) {
			if (!pollfdArr[i].revents)
				continue;

			handleSingleSocket(pollfdArr, i, fdCtx);
			--ready;
		}

		// updatePolloutMonitoring(pollfdArr, clients); // FIXME: maybe do the updating later when data is queued/fully written to avoid looping
	}
}
