/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.class.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 10:55:03 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/07 10:56:55 by djanardh         ###   ########.fr       */
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
	private:
		const ServerConfig* _config;

		// Internal steps for routing
		const LocationConfig* matchLocation(const std::string& uri) const;
		std::string translateToPhysicalPath(const std::string& uri, const LocationConfig* location) const;
		void verifyMethodAllowed(const HttpMethod& method, const LocationConfig* location) const;

		std::string handleGet(const std::string& physicalPath, const LocationConfig* location);
		std::string handlePost(const std::string& physicalPath, const HttpRequestContainer& request);
		std::string handleDelete(const std::string& physicalPath);

	public:
		RequestHandler(const ServerConfig* config);
		~RequestHandler();

		// execution entry point
		std::string processRequest(const HttpRequestContainer& request);
};

#endif
