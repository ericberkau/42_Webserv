#include "HttpRequest.hpp"
#include <iostream>
#include <cctype> // for tolower

HttpRequest::HttpRequest() {}

void HttpRequest::setMethod(const std::string& method) { _method = method; }
void HttpRequest::setUri(const std::string& uri) { _uri = uri; }
void HttpRequest::setVersion(const std::string& version) { _version = version; }
void HttpRequest::setBody(const std::string& body) { _body = body; }

// Lowercase the header key before storing, so lookups are case-insensitive.
// HTTP headers are case-insensitive by spec (Host, host, HOST are the same header).
static std::string toLower(const std::string& s)
{
	std::string result = s;
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
	return result;
}

void HttpRequest::addHeader(const std::string& key, const std::string& value)
{
	_headers[toLower(key)] = value;
}

const std::string& HttpRequest::getMethod() const { return _method; }
const std::string& HttpRequest::getUri() const { return _uri; }
const std::string& HttpRequest::getVersion() const { return _version; }
const std::string& HttpRequest::getBody() const { return _body; }

std::string HttpRequest::getHeader(const std::string& key) const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find(toLower(key));
	if (it == _headers.end())
		return "";
	return it->second;
}

void HttpRequest::print() const
{
	std::cout << "Method:  " << _method << std::endl;
	std::cout << "URI:     " << _uri << std::endl;
	std::cout << "Version: " << _version << std::endl;
	std::cout << "Headers:" << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); ++it)
	{
		std::cout << "  " << it->first << ": " << it->second << std::endl;
	}
	std::cout << "Body (" << _body.size() << " bytes): " << _body << std::endl;
}
