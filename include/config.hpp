/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/08 18:11:25 by eberkau           #+#    #+#             */
/*   Updated: 2026/08/07 17:30:51 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <vector>

// struct s_config {
// 	std::list<std::string>	portsToListen;
// 	size_t					clientReqMaxSize;
// 	std::list<std::string>	acceptedHttpsMethods;
// 	std::string				rootedDir;
// 	bool					dirListing;
// 	bool					uploadAllowed;
// 	// defaultErrorPages;
// };

// using t_config = s_config;

struct ListenDirective {
	std::string		host;
	std::uint16_t	port;
};

struct ServerConfig {
	std::vector<ListenDirective>	listen;
	size_t							clientMaxBodySize;

	ServerConfig(void) : clientMaxBodySize(1024 * 1024) {} // default 1 MiB
};

struct Config {
	std::vector<ServerConfig>	servers;
};
