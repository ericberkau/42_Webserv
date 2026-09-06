/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequestContainer.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 15:15:43 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/06 19:25:01 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_CONTAINER_HPP
#define HTTP_REQUEST_CONTAINER_HPP

#include "config.hpp"
#include <string>
#include <map>

// This class is only a container for the result of parsing
class HttpRequestContainer
{
	private:
		HttpMethod _method;
		std::string _uri;
		std::string _version;
		std::map<std::string, std::string> _headers; // keys stored lowercase
		std::string _body;
		
	public:
		HttpRequestContainer();
		~HttpRequestContainer();

		// Setters
		void setMethod(const HttpMethod& method);
		void setUri(const std::string& uri);
		void setVersion(const std::string& version);
		void addHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);

		// Getters
		const HttpMethod& getMethod() const;
		const std::string& getUri() const;
		const std::string& getVersion() const;
		const std::string& getBody() const;

		// Returns "" if header not found. Header lookup is case-insensitive
		// acc. to RFC, so we normalize keys to lowercase when storing them
		std::string getHeader(const std::string& key) const;

		// For debugging to print everything that is parsed
		void print() const;
};

#endif
