/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 23:02:21 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/03 01:12:31 by skomyshe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Server.class.hpp"
#include "classes/RequestHandler.class.hpp"
#include "classes/ResponseBuilder.class.hpp"
#include "signals.hpp"
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <sys/wait.h>
#include <unistd.h>
#include <variant>

Server::Server(const Config& config) : _config(config) {}

Server::~Server(void) {
	for (auto& [fd, ctx] : _fdCtx) {
		if (ctx.type == SocketType::CGI_PIPE && ctx.cgi->pid > 0) {
			kill(ctx.cgi->pid, SIGKILL);
			waitpid(ctx.cgi->pid, NULL, 0);
		}

		close(fd);
	}
}

void Server::run(void) {
	_setupListeners();

	while (!g_shutdownRequested) {
		std::cout << "listening" << std::endl;

		int ready = poll(_pollfdArr.data(), _pollfdArr.size(), FOREVER);
		if (ready == -1) {
			if (errno == EINTR)
				continue;

			perror("poll");
			throw std::runtime_error("poll() failed");
		}

		for (size_t i = 0; i < _pollfdArr.size() && ready > 0; ++i) {
			if (!_pollfdArr[i].revents)
				continue;

			_handleSocketEvent(i);
			--ready;
		}

		// updatePolloutMonitoring(pollfdArr, clients); // FIXME: maybe do the updating later when data is queued/fully written to avoid looping
	}

	std::cout << "\nServer shutting down ..." << std::endl;
}

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

void Server::_registerCgi(const CgiContext& cgiInfo) {
	try {
		_registerCgiPipe(cgiInfo, POLLIN);
		_registerCgiPipe(cgiInfo, POLLOUT);
	} catch (const std::logic_error&) {
		_abortCgiRegistration(cgiInfo);
		throw;
	} catch (...) {
		_abortCgiRegistration(cgiInfo);
		std::cerr << "Runtime warning: failed to register CGI pipe FD context -> killed CGI process" << std::endl; // FIXME: maybe send error 500 to client and disconnect?
	}
}

void Server::_registerCgiPipe(const CgiContext& cgiInfo, const short events) {
	if (events != POLLIN && events != POLLOUT)
		throw std::runtime_error("tried to register CGI pipe with unsupported poll event");

	const int pipeFd = (events == POLLIN ? cgiInfo.readPipeFd : cgiInfo.writePipeFd);

	const auto emplResult = _fdCtx.emplace(pipeFd, FdContext(cgiInfo));
	bool inserted = emplResult.second;

	if (!inserted)
		throw std::logic_error("attempted to register CGI pipe with duplicate file descriptor");

	pollfd cgiPipe = {};
	cgiPipe.fd = pipeFd;
	cgiPipe.events = events;
	_pollfdArr.push_back(cgiPipe);
}

void Server::_abortCgiRegistration(const CgiContext& cgiInfo) {
	_fdCtx.erase(cgiInfo.readPipeFd);
	_fdCtx.erase(cgiInfo.writePipeFd);

	_pollfdArr.erase(
		std::remove_if(
			_pollfdArr.begin(),
			_pollfdArr.end(),
			[&](const pollfd& pfd) {
				return pfd.fd == cgiInfo.readPipeFd ||
					pfd.fd == cgiInfo.writePipeFd;
			}),
		_pollfdArr.end());

	close(cgiInfo.readPipeFd);
	close(cgiInfo.writePipeFd);
	kill(cgiInfo.pid, SIGKILL);
	waitpid(cgiInfo.pid, NULL, 0);
}

void Server::_handleSocketEvent(size_t& idx) {
	const short revents = _pollfdArr[idx].revents;
	if (revents & POLLNVAL)
		throw std::logic_error("poll() monitored invalid file descriptor");

	FdContext& currFd = _fdCtx.at(_pollfdArr[idx].fd);

	if (revents & (POLLERR | POLLHUP)) {
		switch (currFd.type) {
			case SocketType::LISTENER:
				throw std::runtime_error("listening socket failed");

			case SocketType::CLIENT:
				_disconnectFd(idx);
				return;

			case SocketType::CGI_PIPE:
				printf("POLLERR: %i\nPOLLHUP: %i\n",
					revents & POLLERR ? 1 : 0,
					revents & POLLHUP ? 1 : 0
				); // BUG: for debugging, remove later!

				if (revents & POLLERR)
					_disconnectFd(idx);
				else
					_handleCgiOutput(idx);

				return;

			default:
				throw std::logic_error("unknown socket type failed");
		}
	}

	switch (currFd.type) {
		case SocketType::LISTENER:
			_handleListenerEvents(idx);
			break;

		case SocketType::CLIENT:
			_handleClientEvents(idx);
			break;

		case SocketType::CGI_PIPE:
			_handleCgiOutput(idx);
			break;

		default:
			throw std::logic_error("event for unknown socket type");
	}
}

void Server::_handleListenerEvents(size_t idx) {
	if (_pollfdArr[idx].revents & POLLIN)
		_handlePendingConnections(_pollfdArr[idx].fd);
}

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

void Server::_handleClientEvents(size_t& idx) {
	size_t originalIdx = idx;

	if (_pollfdArr[idx].revents & POLLIN)
		_handleIncomingData(idx);

	/** At this point, the client might have disconnected (idx modified in subfunction)
	 * with still pending outgoing data, originalIdx checks if a diconnect occured.
	 * */
	if (idx != originalIdx)
		return;

	if (_pollfdArr[idx].revents & POLLOUT)
		_handleOutgoingData(idx);
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
		
		if (std::holds_alternative<std::string>(singleResult.result))
			currClient.appendToOutBuf(std::get<std::string>(singleResult.result)); // FIXME: consider adding a transaction queue to keep proper request<->response order grouping
		else {
			CgiContext& cgi = std::get<CgiContext>(singleResult.result);

			cgi.clientFd = currClient.getFd();
			_registerCgi(cgi);
		}

		reqResults.pop();
	}

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

void Server::_handleCgiOutput(size_t& idx) {
	if (!(_pollfdArr[idx].revents & (POLLIN | POLLHUP)))
		return;

	printf("Error in cgi fetcher? -> "); // BUG: for debugging, remove later!
	FdContext& currFdCtx = _fdCtx.at(_pollfdArr[idx].fd);
	printf("no for CGI\n"); // BUG: for debugging, remove later!
	CgiContext& currCgi = currFdCtx.cgi.value();

	auto it = _fdCtx.find(currCgi.clientFd);
	if (it == _fdCtx.end()) {
		_disconnectFd(idx);
		return;
	}
	Client& owningClient = it->second.client.value();

	char buffer[MSG_SIZE];

	ssize_t bytes = read(_pollfdArr[idx].fd, buffer, sizeof(buffer));

	printf("bytes: %zd\n", bytes); // BUG: for debugging, remove later!
	// printf("buffer: %s\n", buffer); // BUG: for debugging, remove later!

	if (bytes > 0) {
		currCgi.output.append(buffer, bytes);
		return;
	}

	if (bytes == 0) {
		owningClient.appendToOutBuf(
			RequestHandler::buildResponseFromCgiOutput(currCgi.output)
		);

		size_t clientIdx = _findPollEntryIdxByFd(currCgi.clientFd);
		if (clientIdx == _pollfdArr.size())
			throw std::logic_error("owning client fd not found");

		_pollfdArr[clientIdx].events |= POLLOUT;

		_disconnectFd(idx);
		return;
	}

	perror("read");
	_disconnectFd(idx);
}

void Server::_disconnectFd(size_t& idx) {
	const int fd = _pollfdArr[idx].fd;
	const FdContext& currCtx = _fdCtx.at(fd);
	
	if (currCtx.type == SocketType::CGI_PIPE) {
		pid_t result = waitpid(currCtx.cgi->pid, NULL, WNOHANG);
		if (result == 0) {
			kill(currCtx.cgi->pid, SIGKILL);
			waitpid(currCtx.cgi->pid, NULL, 0);
		}
	}
	
	_fdCtx.erase(fd);
	close(fd);
	
	_pollfdArr[idx] = _pollfdArr.back();
	_pollfdArr.pop_back();
	--idx;

	std::cout << "FD " << fd << " disconnected" << std::endl;
}

size_t Server::_findPollEntryIdxByFd(int fd) {
	for (size_t i = 0; i < _pollfdArr.size(); ++i) {
		if (_pollfdArr[i].fd == fd)
			return i;
	}
	return _pollfdArr.size();
}
