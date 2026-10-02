/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   networking.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/31 22:44:52 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/02 03:23:59 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "classes/Client.class.hpp"
#include "config.hpp"
#include <optional>
#include <poll.h>
#include <unordered_map>
#include <variant>
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

struct CgiContext {
	pid_t		pid;
	int			pipeFd;
	int			clientFd;
	std::string	output;
};

struct RequestResult {
	std::variant<std::string, CgiContext>	result;

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
		Client&& client
	)
		:	type(SocketType::CLIENT),
			server(serverConfig),
			client(std::move(client)),
			cgi(std::nullopt)
	{
		if (serverConfig == nullptr)
			throw std::logic_error("server must not be empty in FD context");
	}

	/* for CGI pipe */
	explicit FdContext(const CgiContext& cgi)
		:	type(SocketType::CGI_PIPE),
			server(nullptr),
			client(std::nullopt),
			cgi(std::move(cgi))
	{}
};
