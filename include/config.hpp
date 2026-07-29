/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/08 18:11:25 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/29 16:05:41 by eberkau          ###   ########.fr       */
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
	std::string		ip;
	std::uint16_t	port;
};

struct ServerConfig {
	std::vector<ListenDirective>	listen;
};

struct Config {
	std::vector<ServerConfig>	servers;
};
