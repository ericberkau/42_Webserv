/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.class.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 11:04:16 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/13 16:33:51 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/RequestParser.class.hpp"
#include <sstream>
#include "classes/StringUtils.hpp"

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

RequestParser::RequestParser() : _state(PARSE_REQUEST_LINE), _expectedBodyBytes(0), _chunkSizeParsed(false) {}

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

bool RequestParser::_isSupportedMethod(const std::string& method)
{
	// Subject requires at least GET, POST, DELETE
	// We still need to RECOGNIZE other valid HTTP methods (so we can
	// return 405 Method Not Allowed instead of 400 Bad Request), but
	// we only fully support these three
	return method == "GET" || method == "POST" || method == "DELETE";
}

bool RequestParser::_isKnownHttpMethod(const std::string& method) 
{
	static const std::vector<std::string> knownMethods = {
		"GET", "POST", "DELETE", "PUT", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"
	};
	return (std::find(knownMethods.begin(), knownMethods.end(), method) != knownMethods.end());
}

static HttpMethod convertStrToMethod(const std::string& str)
{
	if (str == "GET")
		return HttpMethod::GET;
	else if (str == "POST")
		return HttpMethod::POST;
	else if (str == "DELETE")
		return HttpMethod::DELETE;
	return HttpMethod::INVALID;
}

void RequestParser::_parseRequestLine(const std::string& line, HttpRequestContainer& request)
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

	if (!_isSupportedMethod(method)) 
	{
		// Check if it's a valid standard HTTP method we just don't support
		if (_isKnownHttpMethod(method))
			throw HttpParseException(405, "Method Not Allowed");

		// If it's completely malformed (contains lowercase letters or invalid chars)
		for (char c : method)
		{
			if (!std::isupper(static_cast<unsigned char>(c)))
				throw HttpParseException(400, "Bad Request: invalid method token");
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

	// Check for relative directory traversal backtracks ("..")
	if (uri.find("..") != std::string::npos)
		throw HttpParseException(400, "Bad Request: Directory traversal attempt detected");

	// Validate form (Origin-form, Absolute-form)
	if (uri[0] != '/')
	{
		// If it doesn't start with '/', check if it is a valid absolute URI (starts with http:// or https://)
		if (uri.rfind("http://", 0) != 0 && uri.rfind("https://", 0) != 0)
			throw HttpParseException(400, "Bad Request: unsupported URI format");
	}

	if (version != "HTTP/1.1" && version != "HTTP/1.0")
		throw HttpParseException(400, "Bad Request: unsupported HTTP version");

	std::string path = uri;
	std::string queryString;
	size_t qPos = uri.find('?');
	if (qPos != std::string::npos)
	{
		path = uri.substr(0, qPos);
		queryString = uri.substr(qPos + 1);
	}

	request.setMethod(convertStrToMethod(method));
	request.setUri(path);
	request.setQueryString(queryString);
	request.setVersion(version);
}

bool RequestParser::_isValidHeaderKeyChar(unsigned char c)
{
	if (c <= 32 || c >= 127)
		return false; // No control characters or spaces
	// Forbidden separators in HTTP header fields
	std::string separators = "()<>@,;:\\\"/[]?={}";
	return separators.find(c) == std::string::npos;
}

void RequestParser::_parseHeaderLine(const std::string& line, HttpRequestContainer& request)
{
	std::string key, value;
	if (!splitOnce(line, ':', key, value))
		throw HttpParseException(400, "Bad Request: malformed header line");
	
	if (key.empty())
		throw HttpParseException(400, "Bad Request: empty header name");
	
	if (key.back() == ' ' || key.back() == '\t')
		throw HttpParseException(400, "Bad Request: space before colon in header");

	// Check header name for illegal characters
	for (char c : key)
	{
		if (!_isValidHeaderKeyChar(static_cast<unsigned char>(c)))
			throw HttpParseException(400, "Bad Request: illegal character in header name");
	}

	std::string lowerKey = ft_toLower(key);
	value = trim(value);

	// duplicate is fine only if it's an exact repeat of the same value, anything else is a 400
	if (lowerKey == "content-length")
	{
		std::string existing = request.getHeader("content-length");
		if (existing.empty())
			request.addHeader(key, value);
		else if (existing != value)
			throw HttpParseException(400, "Bad Request: conflicting Content-Length values");
		// exact duplicate: existing value is already correctly stored, nothing more to do
		return;
	}

	if (lowerKey == "host" && !request.getHeader("host").empty())
		throw HttpParseException(400, "Bad Request: duplicate Host header");
		
	request.addHeader(key, value);
}

RequestParser::ParsingState RequestParser::getState() const
{ 
	return _state;
}

void RequestParser::reset()
{ 
	_state = PARSE_REQUEST_LINE;
	_expectedBodyBytes = 0;
	_chunkSizeParsed = false;
	// _buffer.clear();
}

void RequestParser::discardBuffer()
{
	_buffer.clear();
}

// helper function: Safe String-To-Size Integer Converter (avoiding std::stoi coz it throws uncaught 
// C++ exceptions on text and doesn't handle unsigned size_t safely for packet boundaries)

static size_t parseContentLength(const std::string& s)
{
	if (s.empty())
		throw HttpParseException(400, "Bad Request: Empty Content-Length");

	size_t result = 0;
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(s[i])))
			throw HttpParseException(400, "Bad Request: Invalid character in Content-Length");
		
		size_t next = result * 10 + (s[i] - '0');
		// Simple overflow guard
		if (next < result)
			throw HttpParseException(400, "Bad Request: Content-Length integer overflow");
		result = next;
	}
	return result;
}

// helper func.: Safe Hexadecimal-To-Size Converter (For Chunked Mode) (Chunk sizes are sent as hex values (like A for 10 bytes))
static size_t parseHexSize(const std::string& s)
{
	if (s.empty())
		throw HttpParseException(400, "Bad Request: Empty chunk size");

	size_t result = 0;
	for (size_t i = 0; i < s.size(); ++i)
	{
		char c = std::tolower(static_cast<unsigned char>(s[i]));
		int value;
		if (c >= '0' && c <= '9')
			value = c - '0';
		else if (c >= 'a' && c <= 'f')
			value = c - 'a' + 10;
		else
			throw HttpParseException(400, "Bad Request: Invalid Hex in Chunk Size");

		size_t next = result * 16 + value;
		if (next < result)
			throw HttpParseException(400, "Bad Request: Chunk size overflow");
		result = next;
	}
	return result;
}

void RequestParser::feed(const std::string& chunk, HttpRequestContainer& request, const ServerConfig* config)
{
	_buffer += chunk; // Save the newly arrived data bytes to persistent cache

	// Keep processing as long as there are full lines or readable blocks in the buffer
	while (true)
	{
		if (_state == PARSE_REQUEST_LINE || _state == PARSE_HEADERS)
		{
			if ((_state == PARSE_REQUEST_LINE || _state == PARSE_HEADERS) && _buffer.size() > 8192)
				throw HttpParseException(431, "Request Header Fields Too Large");

			// Look for the standard network line break (\r\n)
			size_t lineEnd = _buffer.find("\r\n");
			if (lineEnd == std::string::npos)
				return; // Incomplete line, stop and wait for more network read ticks

			std::string line = _buffer.substr(0, lineEnd);
			_buffer.erase(0, lineEnd + 2); // remove the processed line off our buffer cache

			if (_state == PARSE_REQUEST_LINE)
			{
				_parseRequestLine(line, request);
				_state = PARSE_HEADERS;
			}
			else if (_state == PARSE_HEADERS)
			{
				// If we hit an empty line, the headers block is over
				if (line.empty())
				{
					// HTTP/1.1 Mandate: Check for the required Host header
					if (request.getHeader("host").empty())
						throw HttpParseException(400, "Bad Request: Missing mandatory Host header");
					_state = PARSE_BODY; // Do not break; immediately fall through to try parsing the body out of remaining buffer
				}
				else
					_parseHeaderLine(line, request);
			}
		}
		
		if (_state == PARSE_BODY)
		{
			std::string transferEncoding = request.getHeader("transfer-encoding");
			std::string contentLengthStr = request.getHeader("content-length");
			HttpMethod method = request.getMethod();

			// HTTP Specification Requirement: 411 Length Required
			// If it's a POST request and BOTH size headers are missing, reject it
			if (method == HttpMethod::POST && transferEncoding.empty() && contentLengthStr.empty())
				throw HttpParseException(411, "Length Required");

			// CASE 1: If 'Transfer-Encoding: chunked' is there in the header
			if (transferEncoding.find("chunked") != std::string::npos)
			{
				while (true)
				{
					// Phase A: We need to parse the next hex chunk size line
					if (!_chunkSizeParsed)
					{
						size_t lineEnd = _buffer.find("\r\n");
						if (lineEnd == std::string::npos)
							return; // Wait for full hex line to arrive over the socket

						std::string sizeLine = _buffer.substr(0, lineEnd);
						_buffer.erase(0, lineEnd + 2);

						// Handle optional chunk extensions if present (anything after ';')
						size_t semiColonPos = sizeLine.find(';');
						if (semiColonPos != std::string::npos)
							sizeLine = sizeLine.substr(0, semiColonPos);
						
						sizeLine = trim(sizeLine);
						_expectedBodyBytes = parseHexSize(sizeLine);
						if (_expectedBodyBytes > config->clientMaxBodySize - request.getBody().size())
							throw HttpParseException(413, "Payload Too Large");
						_chunkSizeParsed = true;
					}

					// Phase B: We have the size, now consume the raw chunk data payload
					if (_chunkSizeParsed)
					{
						// A chunk size of 0 implies the terminating block (0\r\n\r\n)
						if (_expectedBodyBytes == 0)
						{
							// Make sure the trailing \r\n after the 0 chunk is also in our buffer
							if (_buffer.size() < 2)
								return; 
							if (_buffer.substr(0, 2) != "\r\n")
								throw HttpParseException(400, "Bad Request: Malformed chunked terminal line");
							
							_buffer.erase(0, 2); // Discard trailing newline
							_state = PARSE_COMPLETE;
							break;
						}

						// We need expected_bytes PLUS the 2 trailing \r\n characters attached to every chunk payload
						if (_buffer.size() < _expectedBodyBytes + 2)
							return; // Stream data chunk incomplete, return control to socket poll loop

						// Extract clean chunk bytes data (ignoring the padding CRLF ending)
						std::string chunkData = _buffer.substr(0, _expectedBodyBytes);
						
						if (_buffer.substr(_expectedBodyBytes, 2) != "\r\n")
							throw HttpParseException(400, "Bad Request: Missing CRLF at end of chunk payload");

						// Append data chunk to body container accumulating payload totals
						request.setBody(request.getBody() + chunkData);
						
						// Enforce Max Body Limit (Webserv client_max_body_size requirement)
						// DON'T FORGET TO replace 50000000 with the variable from the config structs file!
						if (request.getBody().size() > config->clientMaxBodySize)
							throw HttpParseException(413, "Payload Too Large");

						// Wipe out processed data segment from system read cache
						_buffer.erase(0, _expectedBodyBytes + 2);
						
						// Toggle flag back to prepare loop parsing for next structural chunk line
						_chunkSizeParsed = false;
					}
				}
			}
			// CASE 2: If 'Content-Length' exists in the header
			else if (!contentLengthStr.empty())
			{
				size_t contentLength = parseContentLength(contentLengthStr);

				// Enforce Max Body Limit up front (413 Payload Too Large) before consuming bytes
				if (contentLength > config->clientMaxBodySize) //Map to configuration rules later
					throw HttpParseException(413, "Payload Too Large");

				// Check if the current cache holds the requested frame bytes fully
				if (_buffer.size() < contentLength)
					return; // Incomplete payload frame block, yield control back to recv()

				// Populate request storage safely and clear the stream sequence
				request.setBody(_buffer.substr(0, contentLength));
				_buffer.erase(0, contentLength);
				_state = PARSE_COMPLETE;
			}
			// CASE 3: If neither header exists, No payload body requested (Standard GET/DELETE operations)
			else
			{
				_state = PARSE_COMPLETE;
			}
		}

		if (_state == PARSE_COMPLETE || _buffer.empty())
			break;
	}
}
