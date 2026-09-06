/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 16:05:34 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/06 18:20:14 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSEBUILDER_HPP
#define RESPONSEBUILDER_HPP

#include <sstream>
#include <string>
#include <map>
#include <fstream>
#include "ConfigStructures.hpp"

class ResponseBuilder
{
	private:
		std::string _httpVersion;
		int	_statusCode;
		std::string _statusMessage;
		std::map<std::string, std::string> _headers;
		std::string _body;

		// Internal helper to get standard HTTP status messages automatically
		std::string getStandardStatusMessage(int code) const;

	public:
		ResponseBuilder();
		~ResponseBuilder();

		// Setters
		void setStatus(int code); // Automatically looks up the text message
		void setStatus(int code, const std::string& customMessage);
		void addHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);
		void setHttpVersion(const std::string& version);

		// Helpers  for common headers
		void setContentType(const std::string& contentType);
		void setContentLength(size_t length);

		// Generates a proper HTML page for error codes (404, 405, etc.)
		void generateDefaultErrorPage(int code);
		void generateCustomErrorPage(int code, const ServerConfig& config);

		// stitches everything into the raw string packet
		std::string buildString() const;

		// Clears the builder state so it can be reused on keep-alive loops
		void clear();

		void setContentTypeFromExtension(const std::string& physicalPath);
};

#endif
