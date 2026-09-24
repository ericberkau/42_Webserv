/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   networking.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/31 22:44:52 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/24 22:00:20 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "classes/Client.class.hpp"
#include "config.hpp"
#include <optional>
#include <poll.h>
#include <unordered_map>
#include <vector>

constexpr int		FOREVER = -1;
constexpr int		MAX_PEND_CONNECT_QUEUE = 32;
constexpr size_t	MSG_SIZE = 4096;


enum class SocketType {
	INVALID = -1,
	LISTENER,
	CLIENT,
	CGI_PIPE
};

struct PendingCgi {
	pid_t	pid;
	int		stdoutFd;
	int		clientFd;
};

struct RequestResult {
	std::optional<std::string>	response;
	std::optional<PendingCgi>	cgi;

	/* constructors */
	/* for immediate response */
	explicit RequestResult(const std::string& response)
		: response(response), cgi(std::nullopt)
	{}

	/* for CGI */
	explicit RequestResult(const PendingCgi& cgi)
		: response(std::nullopt), cgi(cgi)
	{}
};

struct FdContext {
	SocketType				type;
	const ServerConfig*		server;
	std::optional<Client>	client;
	int						owningClientFd;
	pid_t					pid;


	/* constructors */
	/* for listerner */
	explicit FdContext(const ServerConfig* serverConfig)
		:	type(SocketType::LISTENER),
			server(serverConfig),
			client(std::nullopt),
			owningClientFd(-1),
			pid(-1)
	{
		if (serverConfig == nullptr)
			throw std::logic_error("server must not be empty in FD context");
	}

	/* for client */
	explicit FdContext(
		const ServerConfig* serverConfig,
		Client&& client
	)
		:	type(SocketType::CLIENT),
			server(serverConfig),
			client(std::move(client)),
			owningClientFd(-1),
			pid(-1)
	{
		if (serverConfig == nullptr)
			throw std::logic_error("server must not be empty in FD context");
	}

	/* for CGI pipe */
	explicit FdContext(const PendingCgi& cgi)
		:	type(SocketType::CGI_PIPE),
			server(nullptr),
			client(std::nullopt),
			owningClientFd(cgi.clientFd),
			pid(cgi.pid)
	{}
};
