/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_loop.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/20 22:02:11 by eberkau           #+#    #+#             */
/*   Updated: 2026/06/01 01:52:06 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "networking.hpp"
// #include "classes/Client.class.hpp"
// #include <arpa/inet.h>
// #include <fcntl.h>
// #include <poll.h>
// #include <iostream>

// constexpr int FOREVER = -1;
// constexpr int MAX_PEND_CONNECT_QUEUE = 32;

static int setupListeningSocket(std::vector<pollfd>& pollfdArr) {
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
		case socket_type::LISTENER:
			handleListenerEvents(pollfdArr, idx, fdCtx);
			break;

		case socket_type::CLIENT:
			handleClientEvents(pollfdArr, idx, fdCtx);
			break;

		case socket_type::CGI_PIPE:
			// TODO
			break;

		default:
			break;
	}
}

void serverLoop(void) {
	std::vector<pollfd> pollfdArr;

	const int serverFd = setupListeningSocket(pollfdArr);

	std::unordered_map<int, FdContext> fdCtx;

	fdCtx.emplace(serverFd, FdContext(socket_type::LISTENER));

	if (0 != listen(serverFd, MAX_PEND_CONNECT_QUEUE)) {
		perror("listen");
		throw 1;
	}
	// FIXME: maybe wrap above code in subfunction later to register multiple listeners in a loop

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
