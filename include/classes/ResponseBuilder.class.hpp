/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.class.hpp                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 10:58:56 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/27 19:19:21 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSEBUILDER_HPP
#define RESPONSEBUILDER_HPP

#include <sstream>
#include <string>
#include <map>
#include <fstream>
#include "config.hpp"
#include <sys/stat.h> 

class ResponseBuilder
{
	public:
	
		ResponseBuilder();
		~ResponseBuilder();

		// Setters
		void	setStatus(int code); // Automatically looks up the text message
		void	setStatus(int code, const std::string& customMessage);
		void	addHeader(const std::string& key, const std::string& value);
		void	setBody(const std::string& body);
		void	setHttpVersion(const std::string& version);

		// Helpers  for common headers
		void	setContentType(const std::string& contentType);
		void	setContentLength(size_t length);

		// Generates a proper HTML page for error codes
		void	generateDefaultErrorPage(int code);
		void	generateCustomErrorPage(int code, const ServerConfig& config);

		std::string	buildString() const;

		// Clears the builder state so it can be reused on keep-alive loops (not currently(10 Sep) needed)
		void	clear();

		void	setContentTypeFromExtension(const std::string& physicalPath);
		
	private:
	
		std::string							_httpVersion;
		int									_statusCode;
		std::string							_statusMessage;
		std::map<std::string, std::string>	_headers;
		std::string							_body;

		// Internal helper to get standard HTTP status messages
		std::string	_getStandardStatusMessage(int code) const;
};

#endif
