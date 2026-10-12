/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.class.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 10:55:03 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/11 23:32:27 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "classes/CGI/CGIEnv.class.hpp"
#include "classes/CGI/CGIExecutor.class.hpp"
#include "classes/HTTP/ResponseBuilder.class.hpp"
#include "classes/HTTP/RequestParser.class.hpp"
#include "config.hpp"
#include "HttpRequestContainer.class.hpp"
#include "networking.hpp"
#include "stringUtils.hpp"
#include <cctype>
#include <cstdio> // for std::remove
#include <cstdlib>
#include <ctime> // time()
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h> // for checking if files exist
#include <sys/types.h>

class RequestHandler
{
	public:
	
		RequestHandler(const ServerConfig* config);
		~RequestHandler();

		static std::string				buildResponseFromCgiOutput(const std::string& cgiOutput, const ServerConfig& config);
		static size_t					bodyLimitFor(const ServerConfig& config, const LocationConfig* location);
		static const LocationConfig*	matchLocation(const ServerConfig& config, const std::string& uri, HttpMethod method);

		RequestResult					processRequest(const HttpRequestContainer& request);
		
	private:
	
		const ServerConfig*	_config;

		// Internal steps for routing
		const LocationConfig*	_matchLocation(const std::string& uri, HttpMethod method) const;
		bool					_isCgiRequest(const std::string& physicalPath, const LocationConfig* location) const;
		std::string				_translateToPhysicalPath(const std::string& uri, const LocationConfig* location) const;
		void					_verifyMethodAllowed(const HttpMethod& method, const LocationConfig* location) const;

		std::string	_handleDelete(const std::string& physicalPath);
		CgiContext	_handleCgi(const std::string& physicalPath, const HttpRequestContainer& request, const LocationConfig* location);
		std::string	_handleGet(const std::string& physicalPath, const LocationConfig* location, const std::string& uri);
		std::string	_handlePost(const HttpRequestContainer& request, const LocationConfig* location);
		std::string	_handleRedirect(const LocationConfig* location) const;

};

#endif
