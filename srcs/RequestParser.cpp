/* ************************************************************************** */
/*																			*/
/*														:::	  ::::::::   */
/*   RequestParser.cpp								  :+:	  :+:	:+:   */
/*													+:+ +:+		 +:+	 */
/*   By: djanardh <djanardh@student.42heilbronn.	+#+  +:+	   +#+		*/
/*												+#+#+#+#+#+   +#+		   */
/*   Created: 2026/06/22 15:14:18 by djanardh		  #+#	#+#			 */
/*   Updated: 2026/06/22 19:27:38 by djanardh		 ###   ########.fr	   */
/*																			*/
/* ************************************************************************** */

#include "RequestParser.hpp"
#include <sstream>

HttpParseException::HttpParseException(int statusCode, const std::string& reason)
	: _statusCode(statusCode), _reason(reason) {}

HttpParseException::~HttpParseException() throw() {}

const char* HttpParseException::what() const throw()
{
	return _reason.c_str();
}

int HttpParseException::getStatusCode() const
{
	return _statusCode;
}

// Helpers

// HTTP header values can have leading/trailing whitespace around the
// colon, Ex. "Host:	example.com". We trim spaces and tabs
static std::string trim(const std::string& s)
{
	size_t start = s.find_first_not_of(" \t");
	if (start == std::string::npos) // no text found, only spaces & tabs
		return "";
	size_t end = s.find_last_not_of(" \t");
	return s.substr(start, end - start + 1); // 2nd arg = length
}

// Splits a line on the first occurrence of a delimiter character.
// Used for splitting "Key: Value" on the first ':'.
static bool splitOnce(const std::string& s, char delim, std::string& left, std::string& right)
{
	size_t pos = s.find(delim);
	if (pos == std::string::npos)
		return false;
	left = s.substr(0, pos);
	right = s.substr(pos + 1);
	return true;
}

// RequestParser

bool RequestParser::isSupportedMethod(const std::string& method)
{
	// Subject requires at least GET, POST, DELETE
	// We still need to RECOGNIZE other valid HTTP methods (so we can
	// return 405 Method Not Allowed instead of 400 Bad Request), but
	// we only fully support these three
	return method == "GET" || method == "POST" || method == "DELETE";
}

bool RequestParser::isKnownHttpMethod(const std::string& method) 
{
	static const std::vector<std::string> knownMethods = {
		"GET", "POST", "DELETE", "PUT", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"
	};
	return std::find(knownMethods.begin(), knownMethods.end(), method) != knownMethods.end();
}

void RequestParser::parseRequestLine(const std::string& line, HttpRequestContainer& request)
{
	// Request line looks like: "GET /index.html HTTP/1.1"
	// Exactly two spaces separate exactly three tokens. Anything else
	// (missing piece, extra spaces) is a malformed request -> 400.
	std::istringstream iss(line);
	std::string method, uri, version;

	if (!(iss >> method >> uri >> version))
		throw HttpParseException(400, "Bad Request: malformed request line");

	// Check there's nothing left over (Ex. extra token after version)
	std::string extra;
	if (iss >> extra)
		throw HttpParseException(400, "Bad Request: malformed request line");

	if (!isSupportedMethod(method)) 
	{
		// Check if it's a valid standard HTTP method we just don't support
		if (isKnownHttpMethod(method)) {
			throw HttpParseException(405, "Method Not Allowed");
		}

		// If it's completely malformed (contains lowercase letters or invalid chars)
		for (char c : method) {
			if (!std::isupper(static_cast<unsigned char>(c))) {
				throw HttpParseException(400, "Bad Request: invalid method token");
			}
		}

		// It's all-caps but completely random (e.g., "MYCUSTOMMETHOD")
		throw HttpParseException(405, "Method Not Allowed"); 
	}

	if (uri.empty())
		throw HttpParseException(400, "Bad Request: empty URI");

	// Look for forbidden control characters/spaces
	for (char c : uri)
	{
		if (!(c > 32 && c < 127))
			throw HttpParseException(400, "Bad Request: illegal characters in URI");
	}

	// Validate form (Origin-form, Absolute-form)
	if (uri[0] != '/') {
		// If it doesn't start with '/', check if it is a valid absolute URI (starts with http:// or https://)
		if (uri.rfind("http://", 0) != 0 && uri.rfind("https://", 0) != 0) {
			throw HttpParseException(400, "Bad Request: unsupported URI format");
		}
	}

	if (version != "HTTP/1.1" && version != "HTTP/1.0")
		throw HttpParseException(400, "Bad Request: unsupported HTTP version");

	request.setMethod(method);
	request.setUri(uri);
	request.setVersion(version);
}

void RequestParser::parseHeaderLine(const std::string& line, HttpRequestContainer& request)
{
	std::string key, value;
	if (!splitOnce(line, ':', key, value))
		throw HttpParseException(400, "Bad Request: malformed header line");
	
	if (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
		throw HttpParseException(400, "Bad Request: space before colon in header");

	key = trim(key);
	value = trim(value);

	if (key.empty())
		throw HttpParseException(400, "Bad Request: empty header name");

	request.addHeader(key, value);
}

HttpRequestContainer RequestParser::parse(const std::string& raw)
{
	HttpRequestContainer request;

	// Headers are separated by CRLF ("\r\n"), and the header block ends
	// with a blank line (CRLF CRLF). For now we assume the whole
	// request (request line + headers) is already in `raw`. ACTUAL
	// partial-data handling should be done later
	size_t headerEnd = raw.find("\r\n\r\n"); // Stores the index of the very first character of that (entire) match (the first \r)
	if (headerEnd == std::string::npos) // npos is a special constant in C++ that means 'not found'.
		throw HttpParseException(400, "Bad Request: no end of headers found");

	std::string headerBlock = raw.substr(0, headerEnd);
	std::string rest = raw.substr(headerEnd + 4); // skip past "\r\n\r\n"

	// Split headerBlock into lines on "\r\n"
	std::istringstream stream(headerBlock);
	std::string line;
	bool firstLine = true;

	while (std::getline(stream, line)) // getline() by default reads until it hits '\n'
	{
		// getline on "\r\n"-separated text leaves a trailing '\r' so strip it
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1); // chops it off

		if (firstLine)
		{
			parseRequestLine(line, request);
			firstLine = false;
		}
		else
		{
			parseHeaderLine(line, request);
		}
	}

	// NO Content-Length / chunked handling yet!!
	// Whatever is left after the headers, we just store as-is for now
	request.setBody(rest);

	return request;
}
