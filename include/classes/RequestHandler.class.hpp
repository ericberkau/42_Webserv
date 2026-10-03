/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.class.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 10:55:03 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/04 00:30:36 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "HttpRequestContainer.class.hpp"
#include "networking.hpp"
#include "../config.hpp"
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <ctime> // time()
#include <cctype>
#include "classes/ResponseBuilder.class.hpp"
#include "classes/RequestParser.class.hpp"
#include <sys/stat.h> // for checking if files exist
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio> // for std::remove
#include <cstdlib>
#include "classes/StringUtils.hpp"
#include "classes/CGIEnv.class.hpp"
#include "classes/CGIExecutor.class.hpp"

class RequestHandler
{
	public:
	
		RequestHandler(const ServerConfig* config);
		~RequestHandler();

		RequestResult		processRequest(const HttpRequestContainer& request);
		static std::string	buildResponseFromCgiOutput(const std::string& cgiOutput);
		
	private:
	
		const ServerConfig*	_config;

		// Internal steps for routing
		const LocationConfig*	_matchLocation(const std::string& uri) const;
		std::string				_translateToPhysicalPath(const std::string& uri, const LocationConfig* location) const;
		bool					_isCgiRequest(const std::string& physicalPath, const LocationConfig* location) const;
		void					_verifyMethodAllowed(const HttpMethod& method, const LocationConfig* location) const;

		std::string	_handleGet(const std::string& physicalPath, const LocationConfig* location, const std::string& uri);
		std::string	_handlePost(const HttpRequestContainer& request, const LocationConfig* location);
		std::string	_handleDelete(const std::string& physicalPath);
		CgiContext	_handleCgi(const std::string& physicalPath, const HttpRequestContainer& request);
		std::string	_handleRedirect(const LocationConfig* location) const;

};

#endif
