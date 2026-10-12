/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.class.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 11:04:08 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/13 01:00:31 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/HTTP/RequestHandler.class.hpp"

RequestHandler::RequestHandler(const ServerConfig* config) : _config(config) {}
RequestHandler::~RequestHandler() {}

static bool isExtensionLocation(const LocationConfig& loc)
{
	return !loc.path.empty() && loc.path[0] == '*'; // e.g. "*.bla"
}

// longest prefix match (your old logic, just using a config argument)
static const LocationConfig* matchPrefixLocation(const ServerConfig& config, const std::string& uri)
{
	const LocationConfig* bestMatch = NULL;
	size_t longestMatchLength = 0;

	for (size_t i = 0; i < config.locations.size(); ++i)
	{
		const LocationConfig& loc = config.locations[i];

		if (isExtensionLocation(loc) || uri.rfind(loc.path, 0) != 0)
			continue;

		bool boundary = loc.path.empty()
			|| loc.path.back() == '/'
			|| uri.size() == loc.path.size()
			|| uri[loc.path.size()] == '/';

		if (boundary && loc.path.size() > longestMatchLength)
		{
			longestMatchLength = loc.path.size();
			bestMatch = &loc;
		}
	}
	return bestMatch;
}

static const LocationConfig* matchExtensionLocation(const ServerConfig& config, const std::string& uri, HttpMethod method)
{
	const LocationConfig* best = NULL;
	size_t bestLen = 0;

	for (size_t i = 0; i < config.locations.size(); ++i)
	{
		const LocationConfig& loc = config.locations[i];
		if (!isExtensionLocation(loc))
			continue;

		// an extension location only claims requests whose method it allows
		if (!loc.allowedMethods.empty() && loc.allowedMethods.find(method) == loc.allowedMethods.end())
			continue;

		std::string suffix = loc.path.substr(1);
		if (uri.size() >= suffix.size()
			&& uri.compare(uri.size() - suffix.size(), suffix.size(), suffix) == 0
			&& suffix.size() > bestLen)
		{
			best = &loc;
			bestLen = suffix.size();
		}
	}
	return best;
}

// extension location prioritised over any prefix location
const LocationConfig* RequestHandler::matchLocation(const ServerConfig& config, const std::string& uri, HttpMethod method)
{
	const LocationConfig* ext = matchExtensionLocation(config, uri, method);
	return ext ? ext : matchPrefixLocation(config, uri);
}

const LocationConfig* RequestHandler::_matchLocation(const std::string& uri, HttpMethod method) const
{
	return matchLocation(*_config, uri, method);
}

// location value if set, otherwise  server value
size_t RequestHandler::bodyLimitFor(const ServerConfig& config, const LocationConfig* location)
{
	if (location)
		return location->clientMaxBodySize.value_or(config.clientMaxBodySize);
	return config.clientMaxBodySize;
}

bool RequestHandler::_isCgiRequest(const std::string& physicalPath, const LocationConfig* location) const
{
	if (!location || location->cgiHandlers.empty())
		return false;

	size_t dotPos = physicalPath.find_last_of('.');
	if (dotPos == std::string::npos)
		return false;

	std::string ext = physicalPath.substr(dotPos);
	return location->cgiHandlers.find(ext) != location->cgiHandlers.end();
}

// checking the 405 Method Not Allowed Rule. If a location block specifies that only GET is allowed, 
// any incoming POST or DELETE request must halt immediately
void RequestHandler::_verifyMethodAllowed(const HttpMethod& method, const LocationConfig* location) const
{
	if (!location)
		return; // if no location block matches, default global rules apply

	// if allowedMethods is empty, assume all methods are permitted by default
	if (location->allowedMethods.empty())
		return;

	if (location->allowedMethods.find(method) == location->allowedMethods.end())
		throw std::runtime_error("405"); 
}

std::string RequestHandler::_translateToPhysicalPath(const std::string& uri, const LocationConfig* location) const
{
	// extension location says how to handle a file; the directory location says where it lives
	if (location && isExtensionLocation(*location) && location->root.empty())
		location = matchPrefixLocation(*_config, uri);
		
	std::string baseRoot = "html";
	bool stripPrefix = false;

	if (location && !location->root.empty())
	{
		baseRoot = location->root;
		stripPrefix = !isExtensionLocation(*location); // "*.bla" is not a URL prefix
	}
	else if (!_config->root.empty())
		baseRoot = _config->root;

	std::string rest = uri;
	if (stripPrefix && uri.compare(0, location->path.size(), location->path) == 0)
	{
		rest = uri.substr(location->path.size());
		if (rest.empty() || rest[0] != '/')
			rest = "/" + rest;
	}
	std::string path = baseRoot + rest;

	struct stat pathStat;
	if (stat(path.c_str(), &pathStat) == 0)
	{
		// Check if the path targets a directory folder on disk
		if (S_ISDIR(pathStat.st_mode))
		{
			// extract the index configuration if it exists
			std::string indexFile = location ? location->index : "";

			// do file lookup if the config explicitly specified an index file
			if (!indexFile.empty())
			{
				std::string fullIndexPath = path;
				if (fullIndexPath.back() == '/') 
					fullIndexPath += indexFile;
				else							 
					fullIndexPath += "/" + indexFile;

				// Check if the configured index file actually exists on the hard drive
				struct stat indexStat;
				if (stat(fullIndexPath.c_str(), &indexStat) == 0)
					return fullIndexPath; // Match found, route directly to this file
			}
		}
	}
	// If no index file was specified or if the configured file doesn't exist on disk
	// leave the path pointing to the folder directory. 
	// handleGet block will evaluate 'autoIndex' from here
	return path;
}


// attempts to open a file on our hard drive. If it exists, it reads the content and builds a 200 OK response
// If it doesn't exist, it throws a 404 Not Found error
std::string RequestHandler::_handleGet(const std::string& physicalPath, const LocationConfig* location, const std::string& uri)
{
	ResponseBuilder response;

	struct stat pathStat;
	if (stat(physicalPath.c_str(), &pathStat) != 0)
		throw std::runtime_error("404"); // Path doesn't exist at all -> 404

	// CASE 1: The path is a directory (meaning index.html was missing)
	if (S_ISDIR(pathStat.st_mode))
	{
		if (uri.empty() || uri.back() != '/')
		{
			response.setStatus(301);
			response.addHeader("Location", uri + "/");
			response.setBody("");
			return response.buildString();
		}
		
		// if location is NULL, autoIndex defaults to false
		bool isAutoindexEnabled = location ? location->autoIndex : false;
		if (!isAutoindexEnabled)
			throw std::runtime_error("403"); // If autoindex is turned off and we have no valid index file path, deny access

		// Opening directory folder stream
		DIR* dir = opendir(physicalPath.c_str());
		if (!dir)
			throw std::runtime_error("500"); // Internal Server Error

		std::stringstream html;
		html << "<html><head><title>Index of " << (location ? location->path : "/") << "</title></head>";
		html << "<body><h1>Index of " << (location ? location->path : "/") << "</h1><hr><pre>";

		struct dirent* entry;
		// Loop through every file and item inside the folder
		while ((entry = readdir(dir)) != NULL)
		{
			std::string name = entry->d_name;
			
			// Skip hidden current directory "." references to look clean
			if (name == ".")
				continue;

			// Generate an HTML hyperlink anchor block for each asset
			html << "<a href=\"" << name << "\">" << name << "</a>\n";
		}
		closedir(dir);

		html << "</pre><hr></body></html>";

		response.setStatus(200);
		response.setContentType("text/html");
		response.setBody(html.str());
		return response.buildString();
	}

	// CASE 2: path is a normal standalone file
	std::ifstream file(physicalPath.c_str(), std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("404");

	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string fileContent = buffer.str();
	file.close();

	response.setStatus(200);
	response.setContentTypeFromExtension(physicalPath); // Automatically select MIME type
	response.setBody(fileContent);

	return response.buildString();
}

static std::string sanitizeFilename(const std::string& raw)
{
	std::string out;
	for (size_t i = 0; i < raw.size(); ++i)
	{
		unsigned char c = raw[i];
		if (std::isalnum(c) || c == '.' || c == '_' || c == '-')
			out += c;
	}
	while (!out.empty() && out[0] == '.')
		out.erase(0, 1);
	return out;
}

static std::string generateUploadName()
{
	static unsigned long counter = 0;
	std::ostringstream ss;
	ss << "upload_" << time(NULL) << "_" << counter++;
	return ss.str();
}

std::string RequestHandler::_handlePost(const HttpRequestContainer& request, const LocationConfig* location)
{
	if (!location)
		throw std::runtime_error("403");

	if (!location->uploadEnabled)
	{
		// POST is allowed here (method check passed), but no upload storage is configured:
		// accept the request and discard the body
		std::string bodyMessage = "<h1>POST Received</h1>";
		std::stringstream lengthStream;
		lengthStream << bodyMessage.size();

		return "HTTP/1.1 200 OK\r\n"
			"Content-Length: " + lengthStream.str() + "\r\n"
			"Content-Type: text/html\r\n"
			"\r\n" + bodyMessage;
	}

	struct stat st;
	if (stat(location->uploadPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
		throw std::runtime_error("500");

	const std::string& uri = request.getUri();
	std::string name = sanitizeFilename(uri.substr(uri.find_last_of('/') + 1));
	if (name.empty())
		name = generateUploadName();

	std::string target = location->uploadPath;
	if (target.back() != '/')
		target += '/';
	target += name;

	if (stat(target.c_str(), &st) == 0)
		throw std::runtime_error("409");

	std::ofstream file(target.c_str(), std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("500");
	file.write(request.getBody().data(), request.getBody().size());
	file.close();

	std::string base = location->path;
	if (base.empty() || base.back() != '/')
		base += '/';

	std::string bodyMessage = "<h1>File Created Successfully</h1>";
	std::stringstream lengthStream;
	lengthStream << bodyMessage.size();

	return "HTTP/1.1 201 Created\r\n"
		"Content-Length: " + lengthStream.str() + "\r\n"
		"Content-Type: text/html\r\n"
		"Location: " + base + name + "\r\n"
		"\r\n" + bodyMessage;
}

// checks if a file exists, and if it does, deletes it using the standard C++ std::remove utility
// if successful, it returns a 200 OK or 204 No Content response
std::string RequestHandler::_handleDelete(const std::string& physicalPath)
{
	// check if the file exists before attempting to delete it
	struct stat pathStat;
	if (stat(physicalPath.c_str(), &pathStat) != 0)
		throw std::runtime_error("404");

	// enforce that we cannot delete a whole directory folder directly via DELETE
	if (S_ISDIR(pathStat.st_mode))
		throw std::runtime_error("400"); // Bad Request

	// delete the file using standard system call
	if (std::remove(physicalPath.c_str()) != 0)
		throw std::runtime_error("500"); // Internal Server Error (e.g., permission denied)

	// prepare a confirmation payload
	std::string bodyMessage = "<h1>File Deleted Successfully</h1>";
	std::stringstream lengthStream;
	lengthStream << bodyMessage.size();

	// build the 200 OK response
	std::string response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: " + lengthStream.str() + "\r\n"
		"Content-Type: text/html\r\n"
		"\r\n" + 
		bodyMessage;

	return response;
}

std::string RequestHandler::buildResponseFromCgiOutput(const std::string& cgiOutput, const ServerConfig& config)
{
	ResponseBuilder response;

	if (cgiOutput.empty()) // handler produced nothing: crashed, missing script ???
	{
		response.generateErrorPage(502, config);
		return response.buildString();
	}

	size_t headerEnd = cgiOutput.find("\r\n\r\n");
	size_t sepLen = 4;
	if (headerEnd == std::string::npos)
	{
		headerEnd = cgiOutput.find("\n\n"); // some scripts only emit \n
		sepLen = 2;
	}

	std::string headerBlock = (headerEnd == std::string::npos) ? "" : cgiOutput.substr(0, headerEnd);
	std::string body = (headerEnd == std::string::npos) ? cgiOutput : cgiOutput.substr(headerEnd + sepLen);

	int statusCode = 200;
	std::istringstream headerStream(headerBlock);
	std::string line;
	while (std::getline(headerStream, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			continue;

		size_t pos = line.find(':');
		if (pos == std::string::npos)
			continue;

		std::string key = line.substr(0, pos);
		std::string value = line.substr(pos + 1);
		size_t start = value.find_first_not_of(' ');
		value = (start == std::string::npos) ? "" : value.substr(start);

		if (key == "Status")
			std::istringstream(value) >> statusCode;
		else
			response.addHeader(key, value);
	}

	response.setStatus(statusCode);
	response.setBody(body);
	return response.buildString();
}

static std::string toUpperWithUnderscores(const std::string& key)
{
	std::string result = key;
	for (size_t i = 0; i < result.size(); ++i)
	{
		if (result[i] == '-')
			result[i] = '_';
		else
			result[i] = std::toupper(static_cast<unsigned char>(result[i]));
	}
	return result;
}

CgiContext RequestHandler::_handleCgi(const std::string& physicalPath, const HttpRequestContainer& request, const LocationConfig* location)
{
	// which program handles this file type?
	size_t dot = physicalPath.find_last_of('.');
	if (!location || dot == std::string::npos)
		throw std::runtime_error("500");

	std::map<std::string, std::string>::const_iterator it =
		location->cgiHandlers.find(physicalPath.substr(dot));
	if (it == location->cgiHandlers.end())
		throw std::runtime_error("500");
	const std::string& handler = it->second;

	// A "*.ext" location sends every matching URL to the handler (handler decides what the URL means)
	// A cgi directive inside a normal location runs files from that directory, so the file must exist
	if (!isExtensionLocation(*location))
	{
		struct stat pathStat;
		if (stat(physicalPath.c_str(), &pathStat) != 0 || S_ISDIR(pathStat.st_mode))
			throw std::runtime_error("404");
	}

	// configured handler must be runnable, otherwise server misconfiguration?
	if (access(handler.c_str(), X_OK) != 0)
		throw std::runtime_error("500");

	CGIExecutor executor;
	CGIEnv& env = executor.getEnv();

	env.set("REQUEST_METHOD", methodToString(request.getMethod()));
	env.set("SCRIPT_FILENAME", physicalPath);
	env.set("SCRIPT_NAME", request.getUri());
	env.set("QUERY_STRING", request.getQueryString());
	env.set("SERVER_PROTOCOL", request.getVersion());
	env.set("GATEWAY_INTERFACE", "CGI/1.1");
	env.set("REDIRECT_STATUS", "200");

	std::string requestUri = request.getUri();
	
	if (!request.getQueryString().empty())
		requestUri += "?" + request.getQueryString();

	env.set("REQUEST_URI", requestUri);
	env.set("PATH_INFO", request.getUri());

	if (!request.getHeader("content-length").empty() || !request.getHeader("transfer-encoding").empty())
		env.set("CONTENT_LENGTH", std::to_string(request.getBody().size()));

	std::string contentType = request.getHeader("content-type");
	if (!contentType.empty())
		env.set("CONTENT_TYPE", contentType);

	for (const auto& [key, value] : request.getHeaders())
	{
		// already handled above, or no longer true (CGI gets an unchunked body)
		if (key == "content-length" || key == "content-type" || key == "transfer-encoding")
			continue;
		env.set("HTTP_" + toUpperWithUnderscores(key), value);
	}

	CgiContext cgiInfo;
	try
	{
		cgiInfo = executor.start(physicalPath, handler);
		cgiInfo.input = request.getBody();
	} 
	catch (const std::exception&)
	{
		throw std::runtime_error("500");
	}

	return cgiInfo;
}

std::string RequestHandler::_handleRedirect(const LocationConfig* location) const
{
	ResponseBuilder response;
	response.setStatus(location->redirectStatusCode);
	response.addHeader("Location", location->redirectTarget);

	std::string body = "<html><head><title>" + std::to_string(location->redirectStatusCode) + " Redirect</title></head>"
					"<body>Redirecting to <a href=\"" + location->redirectTarget + "\">" + location->redirectTarget + "</a></body></html>";
	response.setContentType("text/html");
	response.setBody(body);

	return response.buildString();
}

// for status code conversion
static int stringToInt(const std::string& s)
{
	std::stringstream ss(s);
	int num;
	ss >> num;
	return num;
}

RequestResult RequestHandler::processRequest(const HttpRequestContainer& request)
{
	ResponseBuilder response;

	try
	{
		// Find configuration block matches
		const LocationConfig* matchedLoc = _matchLocation(request.getUri(), request.getMethod());

		if (matchedLoc && matchedLoc->hasRedirect)
			return RequestResult(_handleRedirect(matchedLoc));

		// Validate routing method permissions
		_verifyMethodAllowed(request.getMethod(), matchedLoc);

		// Convert abstract URI string to a real system file address path
		std::string physicalPath = _translateToPhysicalPath(request.getUri(), matchedLoc);

		if (_isCgiRequest(physicalPath, matchedLoc))
			return RequestResult(_handleCgi(physicalPath, request, matchedLoc));
		else if (request.getMethod() == HttpMethod::GET)
			return RequestResult(_handleGet(physicalPath, matchedLoc, request.getUri()));
		else if (request.getMethod() == HttpMethod::POST)
			return RequestResult(_handlePost(request, matchedLoc));
		else if (request.getMethod() == HttpMethod::DELETE)
			return RequestResult(_handleDelete(physicalPath));

		throw std::runtime_error("501"); // Not Implemented fallback guard if INVALID
	}
	catch (const std::runtime_error& errorMsg)
	{
		// handles structural runtime codes thrown by the execution handlers (e.g. "404", "405")
		int statusCode = stringToInt(errorMsg.what());
		if (statusCode == 0)
			statusCode = 500; // Fallback to Internal Error if conversion fails

		response.generateErrorPage(statusCode, *_config);
		return RequestResult(response.buildString());
	}
	catch (...)
	{
		// global safety net catch-all for any unexpected system failure
		response.generateErrorPage(500, *_config);
		return RequestResult(response.buildString());
	}
}
