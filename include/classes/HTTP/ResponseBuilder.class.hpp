/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.class.hpp                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 10:58:56 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/12 02:07:21 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSEBUILDER_HPP
#define RESPONSEBUILDER_HPP

#include "config.hpp"
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h> 

class ResponseBuilder
{
	public:
	
		ResponseBuilder();
		~ResponseBuilder();

		// Setters
		void	addHeader(const std::string& key, const std::string& value);
		void	setBody(const std::string& body);
		void	setHttpVersion(const std::string& version);
		void	setStatus(int code); // Automatically looks up the text message
		void	setStatus(int code, const std::string& customMessage);

		// Helpers  for common headers
		void	setContentLength(size_t length);
		void	setContentType(const std::string& contentType);

		// Generates a proper HTML page for error codes
		void	generateErrorPage(int code, const ServerConfig& config);

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
		void		_generateDefaultErrorPage(int code);
		std::string	_getStandardStatusMessage(int code) const;

};

#endif
