/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.class.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 10:55:03 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/25 22:00:04 by eberkau          ###   ########.fr       */
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

class RequestHandler
{
	public:
	
		RequestHandler(const ServerConfig* config);
		~RequestHandler();

		// execution entry point
		RequestResult processRequest(const HttpRequestContainer& request);
		
	private:
	
		const ServerConfig* _config;

		// Internal steps for routing
		const LocationConfig* _matchLocation(const std::string& uri) const;
		std::string _translateToPhysicalPath(const std::string& uri, const LocationConfig* location) const;
		bool _isCgiRequest(const std::string& physicalPath, const LocationConfig* location) const;
		void _verifyMethodAllowed(const HttpMethod& method, const LocationConfig* location) const;

		std::string _handleGet(const std::string& physicalPath, const LocationConfig* location);
		std::string _handlePost(const std::string& physicalPath, const HttpRequestContainer& request);
		std::string _handleDelete(const std::string& physicalPath);
		std::string _buildResponseFromCgiOutput(const std::string& cgiOutput);
		PendingCgi _handleCgi(const std::string& physicalPath, const HttpRequestContainer& request);
		std::string _handleRedirect(const LocationConfig* location) const;

};

#endif
