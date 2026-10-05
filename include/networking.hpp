/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   networking.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/31 22:44:52 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/05 17:46:41 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "classes/Client.class.hpp"
#include "config.hpp"
#include <list>
#include <optional>
#include <poll.h>
#include <unordered_map>
#include <variant>
#include <vector>

constexpr int					POLL_TIMEOUT_MS = 10000;
constexpr std::chrono::seconds	IDLE_CHECK_INTERVAL(10);
constexpr std::chrono::seconds	CLIENT_IDLE_TIMEOUT(30);
constexpr int					MAX_PEND_CONNECT_QUEUE = 32;
constexpr size_t				MSG_SIZE = 4096;

enum class SocketType {
	INVALID = -1,
	LISTENER,
	CLIENT,
	CGI_PIPE
};

struct CgiContext {
	pid_t		pid;
	int			readPipeFd;
	int			writePipeFd;
	int			clientFd;
	size_t		writeProgress;
	std::string	input;
	std::string	output;
	std::list<ResponseSlot>::iterator	responseSlot;

	/* constructors */
	/* default */
	CgiContext(void)
		:	pid(-1),
			readPipeFd(-1),
			writePipeFd(-1),
			clientFd(-1),
			writeProgress(0)
	{}

	/* variable copy constructor (copy buffers -> yes/no) */
	explicit CgiContext(const CgiContext& other, bool copyBuffers)
		:	pid(other.pid),
			readPipeFd(other.readPipeFd),
			writePipeFd(other.writePipeFd),
			clientFd(other.clientFd),
			writeProgress(other.writeProgress),
			responseSlot(other.responseSlot)
	{
		if (copyBuffers) {
			input = other.input;
			output = other.output;
		}
	}
};

struct ResponseSlot {
	bool		ready = false;
	std::string	response;
};

struct RequestResult {
	std::variant<std::string, CgiContext>	result;
	std::list<ResponseSlot>::iterator		responseSlot;

	/* constructors */
	/* for immediate response */
	explicit RequestResult(std::string&& response)
		: result(std::move(response))
	{}

	/* for CGI */
	explicit RequestResult(CgiContext&& cgi)
		: result(std::move(cgi))
	{}
};

struct FdContext {
	SocketType					type;
	const ServerConfig*			server;
	std::optional<Client>		client;
	std::optional<CgiContext>	cgi;


	/* constructors */
	/* for listerner */
	explicit FdContext(const ServerConfig* serverConfig)
		:	type(SocketType::LISTENER),
			server(serverConfig),
			client(std::nullopt),
			cgi(std::nullopt)
	{
		if (serverConfig == nullptr)
			throw std::logic_error("server must not be empty in FD context");
	}

	/* for client */
	explicit FdContext(
		const ServerConfig* serverConfig,
		Client&& _client
	)
		:	type(SocketType::CLIENT),
			server(serverConfig),
			client(std::move(_client)),
			cgi(std::nullopt)
	{
		if (serverConfig == nullptr)
			throw std::logic_error("server must not be empty in FD context");
	}

	/* for CGI pipe */
	explicit FdContext(CgiContext&& _cgi)
		:	type(SocketType::CGI_PIPE),
			server(nullptr),
			client(std::nullopt),
			cgi(std::move(_cgi))
	{}
};
