/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequestContainer.hpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 15:15:43 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/05 17:28:08 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_CONTAINER_HPP
#define HTTP_REQUEST_CONTAINER_HPP

#include <string>
#include <map>

// This class is only a container for the result of parsing.
class HttpRequestContainer
{
	private:
		std::string _method;
		std::string _uri;
		std::string _version;
		std::map<std::string, std::string> _headers; // keys stored lowercase
		std::string _body;
		
	public:
		HttpRequestContainer();
		~HttpRequestContainer();

		// Setters
		void setMethod(const std::string& method);
		void setUri(const std::string& uri);
		void setVersion(const std::string& version);
		void addHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);

		// Getters
		const std::string& getMethod() const;
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
