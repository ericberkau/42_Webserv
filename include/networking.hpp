/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   networking.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/31 22:44:52 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/04 19:36:43 by eberkau          ###   ########.fr       */
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
constexpr size_t	MSG_SIZE = 8; // FIXME: provide proper size later


enum class SocketType {
	INVALID = -1, // FIXME: evaluate if still needed
	LISTENER,
	CLIENT,
	CGI_PIPE
};

struct FdContext {
	explicit FdContext(
		SocketType t,
		const ServerConfig* s = NULL
	)
		: type(t), server(s), client(std::nullopt)
	{}

	explicit FdContext(
		SocketType t,
		const ServerConfig* s,
		Client&& c
	)
		: type(t), server(s), client(std::move(c))
	{}

	SocketType				type;
	const ServerConfig*		server;
	std::optional<Client>	client;
};
