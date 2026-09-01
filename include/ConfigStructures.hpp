/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigStructures.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/01 17:58:03 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/01 15:00:37 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGSTRUCTURES_HPP
#define CONFIGSTRUCTURES_HPP

#include <string>
#include <vector>
#include <set>

// Represents a "location" block inside a server configuration block
struct LocationConfig
{
	std::string path;				 // Ex: "/images" or "/cgi-bin"
	std::string root;				 // Ex: "/var/www/html/images"
	std::set<std::string> allowedMethods; // Ex: { "GET", "POST" }
	std::string index;				// Ex: "index.html"
	bool autoindex;				   // Directory listing enabled?
	std::string redirect;			 // Optional HTTP redirection URL
		
	LocationConfig() : autoindex(false) {}
};

// Represents a "server" block
struct ServerConfig
{
	std::string host;
	int port;
	std::string serverName;
	std::string root;				 // Default fallback root
	std::string index;				// Default fallback index file
	size_t clientMaxBodySize;
	std::map<int, std::string> errorPages;
	std::vector<LocationConfig> locations;
		
	ServerConfig() : port(80), clientMaxBodySize(1000000) {} // Default 1MB
};

#endif
