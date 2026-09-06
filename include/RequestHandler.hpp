/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/02 12:54:09 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/06 14:58:36 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "HttpRequestContainer.hpp"
#include "ConfigStructures.hpp"
#include <string>
#include <sys/types.h>
#include <dirent.h>

class RequestHandler
{
private:
	const ServerConfig& _config;

	// Internal steps for routing
	const LocationConfig* matchLocation(const std::string& uri) const;
	std::string translateToPhysicalPath(const std::string& uri, const LocationConfig* location) const;
	void verifyMethodAllowed(const std::string& method, const LocationConfig* location) const;

	std::string handleGet(const std::string& physicalPath, const LocationConfig* location);
	std::string handlePost(const std::string& physicalPath, const HttpRequestContainer& request);
	std::string handleDelete(const std::string& physicalPath);

public:
	RequestHandler(const ServerConfig& config);
	~RequestHandler();

	// execution entry point
	std::string processRequest(const HttpRequestContainer& request);
};

#endif
