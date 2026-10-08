/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequestContainer.class.cpp                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 11:03:59 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/08 05:33:55 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/HttpRequestContainer.class.hpp"
#include <iostream>
#include "classes/StringUtils.hpp"

HttpRequestContainer::HttpRequestContainer() : _method(HttpMethod::INVALID) {}

void HttpRequestContainer::setMethod(const HttpMethod& method)
{
	_method = method;
}

void HttpRequestContainer::setUri(const std::string& uri)
{
	_uri = uri;
}

void HttpRequestContainer::setVersion(const std::string& version)
{
	_version = version;
}
void HttpRequestContainer::setBody(std::string body)
{
	_body = std::move(body);
}

void HttpRequestContainer::setQueryString(const std::string& query)
{
	_queryString = query;
}

void HttpRequestContainer::addHeader(const std::string& key, const std::string& value)
{
	// Lowercase the header key before storing
	// HTTP headers are case-insensitive by spec (Host, host, HOST are the same header)
	std::string lowerKey = ft_toLower(key); 
		
	// If the header already exists append the new value with a comma
	if (_headers.find(lowerKey) != _headers.end())
		_headers[lowerKey] += ", " + value;
	else
		_headers[lowerKey] = value;
}

const HttpMethod& HttpRequestContainer::getMethod() const
{ 
	return _method; 
}

const std::string& HttpRequestContainer::getUri() const
{
	return _uri;
}

const std::string& HttpRequestContainer::getVersion() const
{
	return _version;
}

const std::string& HttpRequestContainer::getBody() const
{
	return _body;
}

const std::string& HttpRequestContainer::getQueryString() const
{
	return _queryString;
}

std::string HttpRequestContainer::getHeader(const std::string& key) const
{
	// ft_toLower(key): to handle whatever is typed when a header is looked up
	std::map<std::string, std::string>::const_iterator it = _headers.find(ft_toLower(key));
	if (it == _headers.end())
		return "";
	return it->second;
}

const std::map<std::string, std::string>& HttpRequestContainer::getHeaders() const
{
	return _headers;
}

// void HttpRequestContainer::appendBody(const std::string& data)
// {
// 	_body += data;
// }

void HttpRequestContainer::clear()
{
	_method = HttpMethod::INVALID;
	_uri.clear();
	_version.clear();
	_queryString.clear();
	_body.clear();
	_headers.clear();
}

void HttpRequestContainer::print() const
{
	// std::cout << "Method:  " << _method << std::endl;
	std::cout << "URI:	 " << _uri << std::endl;
	std::cout << "Version: " << _version << std::endl;
	std::cout << "Headers:" << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); ++it)
	{
		std::cout << "  " << it->first << ": " << it->second << std::endl;
	}
	std::cout << "Body (" << _body.size() << " bytes): " << _body << std::endl;
}
