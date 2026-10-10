/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequestContainer.class.hpp                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 10:54:11 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/10 02:41:39 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_CONTAINER_HPP
#define HTTP_REQUEST_CONTAINER_HPP

#include "config.hpp"
#include <map>
#include <string>

// This class is only a container for the result of parsing
class HttpRequestContainer
{
	public:
	
		HttpRequestContainer();
		~HttpRequestContainer() = default;

		// Setters
		void	addHeader(const std::string& key, const std::string& value);
		void	setBody(std::string body);
		void	setMethod(const HttpMethod& method);
		void	setQueryString(const std::string& query);
		void	setUri(const std::string& uri);
		void	setVersion(const std::string& version);

		// Getters
		const std::string&	getBody() const;
		const HttpMethod&	getMethod() const;
		const std::string&	getQueryString() const;
		const std::string&	getUri() const;
		const std::string&	getVersion() const;

		// Returns "" if header not found. Header lookup is case-insensitive
		// acc. to RFC, so we normalize keys to lowercase when storing them
		std::string									getHeader(const std::string& key) const;
		const std::map<std::string, std::string>&	getHeaders() const;
		// void										appendBody(const std::string& data); // TODO: remove?

		void	clear();

		// For debugging to print everything that is parsed
		void	print() const;
		
	private:
	
		HttpMethod							_method;
		std::string							_uri;
		std::string							_version;
		std::map<std::string, std::string>	_headers; // keys stored lowercase
		std::string							_body;
		std::string							_queryString;
		
};

#endif
