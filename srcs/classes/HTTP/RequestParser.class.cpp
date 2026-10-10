/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.class.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 11:04:16 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/10 21:22:21 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/HTTP/RequestHandler.class.hpp"
#include "classes/HTTP/RequestParser.class.hpp"
#include "stringUtils.hpp"
#include <sstream>

static const size_t MAX_LINE_LENGTH = 8192; // longest single request/header/chunk-size line
static const size_t MAX_HEADER_BYTES = 32768; // all header lines together

HttpParseException::HttpParseException(int statusCode)
	: _statusCode(statusCode) {}

HttpParseException::~HttpParseException() {}

int HttpParseException::getStatusCode() const
{
	return _statusCode;
}

bool HttpParseException::isFatal() const
{
	return (
		_statusCode == 400 ||
		_statusCode == 411 ||
		_statusCode == 413 ||
		_statusCode == 414 ||
		_statusCode == 431
	);
}

RequestParser::RequestParser() : _state(PARSE_REQUEST_LINE), _expectedBodyBytes(0), _chunkSizeParsed(false), _maxBodySize(0), _headerBytes(0) {}

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
		throw HttpParseException(400); // Bad Request: malformed request line

	// Check there's nothing left over (Ex. extra token after version)
	std::string extra;
	if (iss >> extra)
		throw HttpParseException(400); // Bad Request: malformed request line

	if (!_isSupportedMethod(method)) 
	{
		// Check if it's a valid standard HTTP method we just don't support
		if (_isKnownHttpMethod(method))
			throw HttpParseException(405); // Method Not Allowed

		throw HttpParseException(400);
	}

	if (uri.empty())
		throw HttpParseException(400);

	// Look for forbidden control characters/spaces
	for (char c : uri)
	{
		if (!(c > 32 && c < 127))
			throw HttpParseException(400); // Bad Request: illegal characters in URI
	}

	// Check for relative directory traversal backtracks ("..")
	if (uri.find("..") != std::string::npos)
		throw HttpParseException(400); // Bad Request: Directory traversal attempt detected

	// Validate form (Origin-form, Absolute-form)
	if (uri[0] != '/')
	{
		// If it doesn't start with '/', check if it is a valid absolute URI (starts with http:// or https://)
		if (uri.rfind("http://", 0) != 0 && uri.rfind("https://", 0) != 0)
			throw HttpParseException(400); // Bad Request: unsupported URI format
	}

	if (version != "HTTP/1.1" && version != "HTTP/1.0")
		throw HttpParseException(400); // Bad Request: unsupported HTTP version

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
		throw HttpParseException(400); // Bad Request: malformed header line
	
	if (key.empty())
		throw HttpParseException(400); // Bad Request: empty header name
	
	if (key.back() == ' ' || key.back() == '\t')
		throw HttpParseException(400); // Bad Request: space before colon in header

	// Check header name for illegal characters
	for (char c : key)
	{
		if (!_isValidHeaderKeyChar(static_cast<unsigned char>(c)))
			throw HttpParseException(400); // Bad Request: illegal character in header name
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
			throw HttpParseException(400); // Bad Request: conflicting Content-Length values
		// exact duplicate: existing value is already correctly stored, nothing more to do
		return;
	}

	if (lowerKey == "host" && !request.getHeader("host").empty())
		throw HttpParseException(400); // Bad Request: duplicate Host header
		
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
	_maxBodySize = 0;
	_headerBytes = 0;
}

void RequestParser::discardBuffer()
{
	_buffer.clear();
}

// helper function: String-To-Size Integer Converter (avoiding std::stoi coz it throws uncaught 
// C++ exceptions on text and doesn't handle unsigned size_t safely for packet boundaries)

static size_t parseContentLength(const std::string& s)
{
	if (s.empty())
		throw HttpParseException(400); // Bad Request: Empty Content-Length

	size_t result = 0;
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(s[i])))
			throw HttpParseException(400); // Bad Request: Invalid character in Content-Length
		
		size_t next = result * 10 + (s[i] - '0');
		// overflow guard
		if (next < result)
			throw HttpParseException(400); // Bad Request: Content-Length integer overflow
		result = next;
	}
	return result;
}

// helper func.: Hexadecimal-To-Size Converter (For Chunked Mode) (Chunk sizes are sent as hex values (like A for 10 bytes))
static size_t parseHexSize(const std::string& s)
{
	if (s.empty())
		throw HttpParseException(400); // Bad Request: Empty chunk size

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
			throw HttpParseException(400); // Bad Request: Invalid Hex in Chunk Size

		size_t next = result * 16 + value;
		if (next < result)
			throw HttpParseException(400); // Bad Request: Chunk size overflow
		result = next;
	}
	return result;
}

void RequestParser::feed(const std::string& chunk, HttpRequestContainer& request, const ServerConfig* config)
{
	try
	{
		_buffer += chunk; // Save the newly arrived data bytes to persistent cache

		// Keep processing as long as there are full lines or readable blocks in the buffer
		while (true)
		{
			if (_state == PARSE_REQUEST_LINE || _state == PARSE_HEADERS)
				_parseRequestHeader(request, config);

			if (_state == PARSE_BODY)
				_parseBody(request);

			if (_state == PARSE_COMPLETE || _buffer.empty())
				break;
		}
			
	}
	catch (const NeedMoreInput&)
	{
		return;
	}
}

void RequestParser::_parseRequestHeader(HttpRequestContainer& request, const ServerConfig* config)
{
	// Look for the standard network line break (\r\n)
	size_t lineEnd = _buffer.find("\r\n");
	if (lineEnd == std::string::npos)
	{
		if (_buffer.size() > MAX_LINE_LENGTH)
			throw HttpParseException(431); // Request Header Fields Too Large
		throw NeedMoreInput{}; // Incomplete line, stop and wait for more network read ticks
	}

	if (lineEnd > MAX_LINE_LENGTH)
		throw HttpParseException(431);

	_headerBytes += lineEnd + 2;
	if (_headerBytes > MAX_HEADER_BYTES)
		throw HttpParseException(431);

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
				throw HttpParseException(400); // Bad Request: Missing mandatory Host header
			_maxBodySize = RequestHandler::bodyLimitFor(*config, RequestHandler::matchLocation(*config, request.getUri(), request.getMethod()));
			_state = PARSE_BODY; // Do not break; immediately fall through to try parsing the body out of remaining buffer
		}
		else
			_parseHeaderLine(line, request);
	}
}

void RequestParser::_parseBody(HttpRequestContainer& request)
{
	std::string transferEncoding = request.getHeader("transfer-encoding");
	std::string contentLengthStr = request.getHeader("content-length");
	HttpMethod method = request.getMethod();

	// If it's a POST request and BOTH size headers are missing, reject it
	if (method == HttpMethod::POST && transferEncoding.empty() && contentLengthStr.empty())
		throw HttpParseException(411); // Length Required

	// CASE 1: If 'Transfer-Encoding: chunked' is there in the header
	if (transferEncoding.find("chunked") != std::string::npos)
		_parseChunked(request);
	// CASE 2: If 'Content-Length' exists in the header
	else if (!contentLengthStr.empty())
		_parseLengthBasedBody(request, contentLengthStr);
	// CASE 3: If neither header exists, No payload body requested (Standard GET/DELETE operations)
	else
		_state = PARSE_COMPLETE;
}

void RequestParser::_parseLengthBasedBody(HttpRequestContainer& request, const std::string& contentLengthStr)
{
	size_t contentLength = parseContentLength(contentLengthStr);

	// Enforce Max Body Limit up front (413 Payload Too Large) before consuming bytes
	if (contentLength > _maxBodySize)
		throw HttpParseException(413);

	// Check if the current cache holds the requested frame bytes fully
	if (_buffer.size() < contentLength)
		throw NeedMoreInput{}; // Incomplete payload frame block, yield control back to recv()

	request.setBody(_buffer.substr(0, contentLength));
	_buffer.erase(0, contentLength);
	_state = PARSE_COMPLETE;
}

void RequestParser::_parseChunked(HttpRequestContainer& request)
{
	while (true)
	{
		// Phase A: We need to parse the next hex chunk size line
		if (!_chunkSizeParsed)
			_parseChunkSize(request);
		// Phase B: We have the size, now consume the raw chunk data payload
		else
		{
			bool done = _parseChunkBody(request);
			if (done)
				break;
		}
	}
}

void RequestParser::_parseChunkSize(HttpRequestContainer& request)
{
	size_t lineEnd = _buffer.find("\r\n");
	if (lineEnd == std::string::npos)
	{
		if (_buffer.size() > MAX_LINE_LENGTH)
			throw HttpParseException(400); // Bad Request: chunk size line too long
		throw NeedMoreInput{}; // Wait for full hex line to arrive over the socket
	}

	std::string sizeLine = _buffer.substr(0, lineEnd);
	_buffer.erase(0, lineEnd + 2);

	// Handle optional chunk extensions if present (anything after ';')
	size_t semiColonPos = sizeLine.find(';');
	if (semiColonPos != std::string::npos)
		sizeLine = sizeLine.substr(0, semiColonPos);

	sizeLine = trim(sizeLine);
	_expectedBodyBytes = parseHexSize(sizeLine);
	if (_expectedBodyBytes > _maxBodySize - request.getBody().size())
		throw HttpParseException(413); // Payload Too Large

	_chunkSizeParsed = true;
}

bool RequestParser::_parseChunkBody(HttpRequestContainer& request)
{
	// A chunk size of 0 implies the terminating block (0\r\n\r\n)
	if (_expectedBodyBytes == 0)
	{
		// Make sure the trailing \r\n after the 0 chunk is also in our buffer
		if (_buffer.size() < 2)
			throw NeedMoreInput{}; 
		if (_buffer.substr(0, 2) != "\r\n")
			throw HttpParseException(400); // Bad Request: Malformed chunked terminal line

		_buffer.erase(0, 2); // Discard trailing newline
		_state = PARSE_COMPLETE;
		return true;
	}

	// We need expected_bytes PLUS the 2 trailing \r\n characters attached to every chunk payload
	if (_buffer.size() < _expectedBodyBytes + 2)
		throw NeedMoreInput{}; // Stream data chunk incomplete, return control to socket poll loop

	if (_buffer.substr(_expectedBodyBytes, 2) != "\r\n")
		throw HttpParseException(400); // Bad Request: Missing CRLF at end of chunk payload

	// Append data chunk to body container accumulating payload totals
	std::string newBody;
	newBody.reserve(request.getBody().size() + _expectedBodyBytes);
	newBody.assign(request.getBody());
	newBody.append(_buffer, 0, _expectedBodyBytes);
	request.setBody(std::move(newBody));

	if (request.getBody().size() > _maxBodySize)
		throw HttpParseException(413); // Payload Too Large"

	_buffer.erase(0, _expectedBodyBytes + 2);
	
	// Toggle flag back to prepare loop parsing for next structural chunk line
	_chunkSizeParsed = false;

	return false;
}
