/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.class.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 10:55:03 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/07 11:09:17 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "HttpRequestContainer.class.hpp"
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
		std::string processRequest(const HttpRequestContainer& request);
		
	private:
	
		const ServerConfig* _config;

		// Internal steps for routing
		const LocationConfig* _matchLocation(const std::string& uri) const;
		std::string _translateToPhysicalPath(const std::string& uri, const LocationConfig* location) const;
		void _verifyMethodAllowed(const HttpMethod& method, const LocationConfig* location) const;

		std::string _handleGet(const std::string& physicalPath, const LocationConfig* location);
		std::string _handlePost(const std::string& physicalPath, const HttpRequestContainer& request);
		std::string _handleDelete(const std::string& physicalPath);

};

#endif
