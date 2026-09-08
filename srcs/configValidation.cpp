/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configValidation.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 01:40:46 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/08 19:33:09 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config.hpp"
#include <set>
#include <stdexcept>

addrinfo* resolveAddress(const std::string& host, uint16_t port) {
	addrinfo* result = nullptr;
	const std::string portString = std::to_string(port);

	addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	const int resolutionStatus = getaddrinfo(
									host.c_str(),
									portString.c_str(),
									&hints,
									&result
								);

	if (resolutionStatus != 0) {
		throw std::runtime_error(
			"cannot resolve address '"
			+ host + ":" + portString
			+ "': "
			+ gai_strerror(resolutionStatus)
		);
	}

	return result;
}

static
void validateAddressResolution(const ListenDirective& listen) {
	addrinfo* result = resolveAddress(listen.host, listen.port);
	freeaddrinfo(result);
}

static
void validateLocation(const LocationConfig& location) {
	// if (location.root.empty()) // FIXME: discuss whether redirect-only locations (return/cgi) require root
	// 	throw std::runtime_error("location '" + location.path + "' has no root directive");

	if (!location.uploadPath.empty() && !location.uploadEnabled)
		throw std::runtime_error("location '" + location.path + "' specifies upload path but upload_enable is set to 'off'");

	// if (location.hasRedirect && !location.cgiHandlers.empty()) // FIXME: discuss whether desired
	// 	throw std::runtime_error("location '" + location.path + "' combines redirect and cgi directives (return overwrites CGI redirections)");
}

static
void validateServer(const ServerConfig& server) {
	if (server.listen.empty())
		throw std::runtime_error("server block must contain at least one listen directive");

	for (const ListenDirective& listen : server.listen) {
		validateAddressResolution(listen);
	}

	std::set<std::string> allPaths;

	for (const LocationConfig& location : server.locations) {
		if (!allPaths.insert(location.path).second)
			throw std::runtime_error("duplicate location directive path '" + location.path + "'");

		validateLocation(location);
	}
}

void validateConfig(const Config& config) {
	if (config.servers.empty())
		throw std::runtime_error("configuration contains no server blocks");

	for (const ServerConfig& server :  config.servers) {
		validateServer(server);
	}
}
