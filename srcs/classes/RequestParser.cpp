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

#include "classes/RequestParser.hpp"
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

RequestParser::RequestParser() : _state(PARSE_REQUEST_LINE), _expectedBodyBytes(0), _chunkSizeParsed(false) {}

RequestParser::~RequestParser() {}

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

// bool RequestParser::isKnownHttpMethod(const std::string& method) 
// {
// 	static const std::vector<std::string> knownMethods = {
// 		"GET", "POST", "DELETE", "PUT", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"
// 	};
// 	return (std::find(knownMethods.begin(), knownMethods.end(), method) != knownMethods.end());
// }

// if u get an error, try this
bool RequestParser::isKnownHttpMethod(const std::string& method) 
{
	// Use a static raw array instead of an initializer-list vector
		static const std::string localArray[] = {
		"GET", "POST", "DELETE", "PUT", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"
	};
		static const std::vector<std::string> knownMethods(localArray, localArray + sizeof(localArray) / sizeof(localArray[0]));

		return std::find(knownMethods.begin(), knownMethods.end(), method) != knownMethods.end();
}

static
HttpMethod convertStrToMethod(const std::string& str) {
	if (str == "GET")
		return HttpMethod::GET;
	else if (str == "POST")
		return HttpMethod::POST;
	else if (str == "DELETE")
		return HttpMethod::DELETE;
	return HttpMethod::INVALID;
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
		if (isKnownHttpMethod(method))
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

	// Validate form (Origin-form, Absolute-form)
	if (uri[0] != '/')
	{
		// If it doesn't start with '/', check if it is a valid absolute URI (starts with http:// or https://)
		if (uri.rfind("http://", 0) != 0 && uri.rfind("https://", 0) != 0)
			throw HttpParseException(400, "Bad Request: unsupported URI format");
	}

	if (version != "HTTP/1.1" && version != "HTTP/1.0")
		throw HttpParseException(400, "Bad Request: unsupported HTTP version");

	request.setMethod(convertStrToMethod(method));
	request.setUri(uri);
	request.setVersion(version);
}

bool RequestParser::isValidHeaderKeyChar(unsigned char c)
{
	if (c <= 32 || c >= 127) return false; // No control characters or spaces
	// Forbidden separators in HTTP header fields
	std::string separators = "()<>@,;:\\\"/[]?={}";
	return separators.find(c) == std::string::npos;
}

void RequestParser::parseHeaderLine(const std::string& line, HttpRequestContainer& request)
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
		if (!isValidHeaderKeyChar(static_cast<unsigned char>(c)))
			throw HttpParseException(400, "Bad Request: illegal character in header name");
	}

	value = trim(value);
	request.addHeader(key, value);
}

RequestParser::ParsingState RequestParser::getState() const
{ 
	return _state;
}

void RequestParser::reset()
{ 
	_state = PARSE_REQUEST_LINE;
	_buffer.clear();
	_expectedBodyBytes = 0;
	_chunkSizeParsed = false;
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
			// Look for the standard network line break (\r\n)
			size_t lineEnd = _buffer.find("\r\n");
			if (lineEnd == std::string::npos)
				return; // Incomplete line, stop and wait for more network read ticks

			std::string line = _buffer.substr(0, lineEnd);
			_buffer.erase(0, lineEnd + 2); // remove the processed line off our buffer cache

			if (_state == PARSE_REQUEST_LINE)
			{
				parseRequestLine(line, request);
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
					parseHeaderLine(line, request);
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

// HttpRequestContainer RequestParser::parse(const std::string& raw)
// {
// 	HttpRequestContainer request;

// 	// Headers are separated by CRLF ("\r\n"), and the header block ends
// 	// with a blank line (CRLF CRLF). For now we assume the whole
// 	// request (request line + headers) is already in `raw`. ACTUAL
// 	// partial-data handling should be done later
// 	size_t headerEnd = raw.find("\r\n\r\n"); // Stores the index of the very first character of that (entire) match (the first \r)
// 	if (headerEnd == std::string::npos) // npos is a special constant in C++ that means 'not found'.
// 		throw HttpParseException(400, "Bad Request: no end of headers found");

// 	std::string headerBlock = raw.substr(0, headerEnd);
// 	std::string rest = raw.substr(headerEnd + 4); // skip past "\r\n\r\n"

// 	// Split headerBlock into lines on "\r\n"
// 	std::istringstream stream(headerBlock);
// 	std::string line;
// 	bool firstLine = true;

// 	while (std::getline(stream, line)) // getline() by default reads until it hits '\n'
// 	{
// 		// getline on "\r\n"-separated text leaves a trailing '\r' so strip it
// 		if (!line.empty() && line[line.size() - 1] == '\r')
// 			line.erase(line.size() - 1); // chops it off
// 		if (firstLine)
// 		{
// 			parseRequestLine(line, request);
// 			firstLine = false;
// 		}
// 		else
// 			parseHeaderLine(line, request);
// 	}

// 	// NO Content-Length / chunked handling yet!!
// 	// Whatever is left after the headers, we just store as-is for now
// 	request.setBody(rest);

// 	return request;
// }
