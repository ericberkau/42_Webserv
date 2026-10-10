/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/08 18:11:25 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/10 19:45:34 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <cstdint>
#include <map>
#include <netdb.h>
#include <optional>
#include <set>
#include <string>
#include <vector>

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
	bool					autoIndex = false;
	std::optional<size_t>	clientMaxBodySize = std::nullopt;
	std::set<HttpMethod>	allowedMethods;

	bool		uploadEnabled = false;
	std::string	uploadPath;

	bool		hasRedirect = false;
	int			redirectStatusCode = 0;
	std::string	redirectTarget;

	std::map<std::string, std::string>	cgiHandlers;


	/* default inits */
	LocationConfig(void) {
		allowedMethods.insert(HttpMethod::GET);
	}
};

struct ListenDirective {
	std::string	host;
	uint16_t	port = 0;
};

struct ServerConfig {
	std::vector<ListenDirective>	listen;
	size_t							clientMaxBodySize = 1024 * 1024; // default body size: 1 MiB
	std::string						root;
	std::vector<LocationConfig>		locations;
	std::map<int, std::string>		errorPages;
};

struct Config {
	std::vector<ServerConfig>	servers;
};

addrinfo*	resolveAddress(const std::string& host, uint16_t port);
void		validateConfig(const Config& config);
