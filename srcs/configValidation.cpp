/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configValidation.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 01:40:46 by eberkau           #+#    #+#             */
/*   Updated: 2026/08/28 02:31:47 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config.hpp"
#include <set>
#include <stdexcept>

static
void validateLocation(const LocationConfig& location) {
	if (location.root.empty()) // FIXME: discuss whether redirect-only locations (return/cgi) require root
		throw std::runtime_error("location '" + location.path + "' has no root directive");

	if (!location.uploadPath.empty() && !location.uploadEnabled)
		throw std::runtime_error("location '" + location.path + "' specifies upload path but upload_enable is set to 'off'");

	// if (location.hasRedirect && !location.cgiHandlers.empty()) // FIXME: discuss whether desired
	// 	throw std::runtime_error("location '" + location.path + "' combines redirect and cgi directives (return overwrites CGI redirections)");
}

static
void validateServer(const ServerConfig& server) {
	if (server.listen.empty())
		throw std::runtime_error("server block must contain at least one listen directive");

	std::set<std::string> pathSet;

	for (const LocationConfig& location : server.locations) {
		if (!pathSet.insert(location.path).second)
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
