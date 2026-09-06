#include "ResponseBuilder.hpp"

ResponseBuilder::ResponseBuilder() : _httpVersion("HTTP/1.1"), _statusCode(200), _statusMessage("OK") {}
ResponseBuilder::~ResponseBuilder() {}

// Dictionary lookup for standard HTTP text statuses required by the protocol
std::string ResponseBuilder::getStandardStatusMessage(int code) const
{
	switch (code)
	{
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 411: return "Length Required";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		default:  return "Unknown Status";
	}
}

void ResponseBuilder::setStatus(int code)
{
	_statusCode = code;
	_statusMessage = getStandardStatusMessage(code);
}

void ResponseBuilder::setStatus(int code, const std::string& customMessage)
{
	_statusCode = code;
	_statusMessage = customMessage;
}

void ResponseBuilder::addHeader(const std::string& key, const std::string& value)
{
	_headers[key] = value;
}

void ResponseBuilder::setBody(const std::string& body)
{
	_body = body;
	setContentLength(_body.size()); // Automatically sync the header size!
}

void ResponseBuilder::setHttpVersion(const std::string& version)
{
	_httpVersion = version;
}

void ResponseBuilder::setContentType(const std::string& contentType)
{
	addHeader("Content-Type", contentType);
}

void ResponseBuilder::setContentLength(size_t length)
{
	std::stringstream ss;
	ss << length;
	addHeader("Content-Length", ss.str());
}

// Generates a standard default fallback HTML view if no custom error file is found
void ResponseBuilder::generateDefaultErrorPage(int code)
{
	setStatus(code);
		
	std::stringstream bodyStream;
	bodyStream << "<html><head><title>" << _statusCode << " " << _statusMessage << "</title></head>"
			   << "<body bgcolor=\"white\"><center><h1>" << _statusCode << " " << _statusMessage << "</h1></center>"
			   << "<hr><center>webserv/1.0</center></body></html>";
			   
	setContentType("text/html");
	setBody(bodyStream.str());
}

void ResponseBuilder::generateCustomErrorPage(int code, const ServerConfig& config)
{
	std::map<int, std::string>::const_iterator it = config.errorPages.find(code);
	
	if (it != config.errorPages.end())
	{
		std::string filePath = it->second;
		std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
		
		if (file.is_open())
		{
			std::stringstream ss;
			ss << file.rdbuf();
			file.close();
			
			setStatus(code);
			setContentTypeFromExtension(filePath);
			setBody(ss.str());
			return;
		}
	}
	// default if custom file fails
	generateDefaultErrorPage(code);
}


// Sticher: Merges everything into the exact byte packet the network socket needs
std::string ResponseBuilder::buildString() const
{
	std::stringstream packet;

	// 1. Status Line (Ex: HTTP/1.1 200 OK\r\n)
	packet << _httpVersion << " " << _statusCode << " " << _statusMessage << "\r\n";

	// 2. Headers Block (Ex: Content-Length: 42\r\n)
	std::map<std::string, std::string>::const_iterator it;
	for (it = _headers.begin(); it != _headers.end(); ++it)
	{
		packet << it->first << ": " << it->second << "\r\n";
	}

	// 3. The Mandatory Blank Line marking the end of headers
	packet << "\r\n";

	// 4. The Payload Body (if any exists)
	packet << _body;

	return packet.str();
}

void ResponseBuilder::clear()
{
	_httpVersion = "HTTP/1.1";
	_statusCode = 200;
	_statusMessage = "OK";
	_headers.clear();
	_body.clear();
}

void ResponseBuilder::setContentTypeFromExtension(const std::string& physicalPath)
{
	size_t dotPos = physicalPath.find_last_of('.');
		
	// if there is no dot or extension, defaulting safely to raw octet stream/binary
	if (dotPos == std::string::npos || dotPos == physicalPath.size() - 1)
	{
		setContentType("application/octet-stream");
		return;
	}

	std::string ext = physicalPath.substr(dotPos + 1);
	// Convert to lowercase just in case (Ex: .PNG -> .png)
	for (size_t i = 0; i < ext.size(); ++i)
	{
		ext[i] = std::tolower(static_cast<unsigned char>(ext[i]));
	}

	// map common file types standard web browsers expect
	if (ext == "html" || ext == "htm") setContentType("text/html");
	else if (ext == "css")			   setContentType("text/css");
	else if (ext == "js")				setContentType("application/javascript");
	else if (ext == "png")			   setContentType("image/png");
	else if (ext == "jpg" || ext == "jpeg") setContentType("image/jpeg");
	else if (ext == "gif")			   setContentType("image/gif");
	else if (ext == "txt")			   setContentType("text/plain");
	else if (ext == "json")			  setContentType("application/json");
	else								 setContentType("application/octet-stream");
}

