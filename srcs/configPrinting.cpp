/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configPrinting.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/10 21:16:10 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/10 22:27:17 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config.hpp"
#include "stringUtils.hpp"
#include <iostream>

static
void printLocation(const LocationConfig& loc) {
	std::cout
		<< "\nlocation "
		<< loc.path
		<< '\n';

	std::cout
		<< "  root: "
		<< loc.root
		<< '\n';

	std::cout
		<< "  index: "
		<< loc.index
		<< '\n';

	std::cout
		<< "  autoindex: "
		<< (loc.autoIndex ? "on" : "off")
		<< '\n';

	std::cout
		<< "  client_max_body_size: "
		<< (!loc.clientMaxBodySize.has_value()
				? ""
				: (loc.clientMaxBodySize.value() == 0
					? "unlimited"
					: std::to_string(loc.clientMaxBodySize.value())
				)
			)
		<< '\n';

	std::cout
		<< "  upload_enable: "
		<< (loc.uploadEnabled ? "on" : "off")
		<< '\n';

	std::cout
		<< "  upload_path: "
		<< loc.uploadPath
		<< '\n';

	std::cout
		<< "  methods: ";

	for (const HttpMethod& m : loc.allowedMethods) {
		std::cout
			<< methodToString(m)
			<< ' ';
	}

	std::cout << '\n';

	if (loc.hasRedirect) {
		std::cout
			<< "  redirect: "
			<< loc.redirectStatusCode
			<< " -> "
			<< loc.redirectTarget
			<< '\n';
	}

	if (!loc.cgiHandlers.empty()) {
		std::cout << "  cgi:\n";

		for (auto it = loc.cgiHandlers.begin();
			it != loc.cgiHandlers.end();
			++it)
		{
			std::cout
				<< "    "
				<< it->first
				<< " -> "
				<< it->second
				<< '\n';
		}
	}
}

static
void printServer(const ServerConfig& server, size_t serverIndex) {
	std::cout
		<< "SERVER #" << serverIndex + 1
		<< "\n-------------------------\n";

	for (std::size_t j = 0; j < server.listen.size(); ++j) {
		std::cout
			<< "listen: "
			<< server.listen[j].host
			<< ':'
			<< server.listen[j].port
			<< '\n';
	}

	std::cout
		<< "client_max_body_size: "
		<< server.clientMaxBodySize
		<< '\n';

	std::cout
		<< "global root: "
		<< server.root
		<< '\n';

	std::cout << "\nerror pages:\n";

	for (auto it = server.errorPages.begin();
		it != server.errorPages.end();
		++it)
	{
		std::cout
			<< "  "
			<< it->first
			<< " -> "
			<< it->second
			<< '\n';
	}

	std::cout << "\nlocations:\n";

	for (std::size_t k = 0; k < server.locations.size(); ++k)
		printLocation(server.locations[k]);
}

void printConfig(const Config& config) {
	for (std::size_t i = 0; i < config.servers.size(); ++i) {
		printServer(config.servers[i], i);

		std::cout << "\n=========================\n\n";
	}
}
