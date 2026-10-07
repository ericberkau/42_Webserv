/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/08 18:11:25 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/07 17:38:54 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <netdb.h>
#include <cstdint>
#include <set>
#include <optional>

enum class HttpMethod {
	INVALID = -1,
	GET,
	POST,
	DELETE
};

struct LocationConfig {
	std::string				path;
	std::string 			root;
	std::string				index;
	bool					autoIndex;
	std::optional<size_t>	clientMaxBodySize;
	std::set<HttpMethod>	allowedMethods;

	bool		uploadEnabled;
	std::string	uploadPath;

	bool		hasRedirect;
	int			redirectStatusCode;
	std::string	redirectTarget;

	std::map<std::string, std::string>	cgiHandlers;
	
	/* default inits */
	LocationConfig(void)
		:	autoIndex(false),
			clientMaxBodySize(std::nullopt),
			uploadEnabled(false),
			hasRedirect(false),
			redirectStatusCode(0)
	{
		allowedMethods.insert(HttpMethod::GET); // FIXME: allow default method(s)? discuss
	}
};

struct ListenDirective {
	std::string	host;
	uint16_t	port;
};

struct ServerConfig {
	std::vector<ListenDirective>	listen;
	size_t							clientMaxBodySize;
	std::string						root;
	std::vector<LocationConfig>		locations;
	std::map<int, std::string>		errorPages;

	ServerConfig(void) : clientMaxBodySize(1024 * 1024) {} // default body size: 1 MiB
};

struct Config {
	std::vector<ServerConfig>	servers;
};

addrinfo*	resolveAddress(const std::string& host, uint16_t port);
void		validateConfig(const Config& config);
