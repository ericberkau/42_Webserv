/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   networking.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/31 22:44:52 by eberkau           #+#    #+#             */
/*   Updated: 2026/08/30 19:44:30 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "classes/Client.class.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>

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
	explicit FdContext(SocketType t) : type(t), client(std::nullopt) {}
	explicit FdContext(SocketType t, Client&& c) : type(t), client(std::move(c)) {}

	SocketType				type;
	std::optional<Client>	client;
};


/* functions */

void	handlePendConnects(int serverFd, std::vector<pollfd>& pollfdArr, std::unordered_map<int, FdContext>& fdCtx);
void	handleIncomingData(std::vector<pollfd>& pollfdArr, size_t& idx, std::unordered_map<int, FdContext>& fdCtx);
void	handleOutgoingData(std::vector<pollfd>& pollfdArr, size_t& idx, std::unordered_map<int, FdContext>& fdCtx);

