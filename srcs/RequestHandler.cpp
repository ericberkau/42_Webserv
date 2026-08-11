/* ************************************************************************** */
/*																			*/
/*														:::	  ::::::::   */
/*   RequestHandler.cpp								 :+:	  :+:	:+:   */
/*													+:+ +:+		 +:+	 */
/*   By: djanardh <djanardh@student.42heilbronn.	+#+  +:+	   +#+		*/
/*												+#+#+#+#+#+   +#+		   */
/*   Created: 2026/08/07 19:28:12 by djanardh		  #+#	#+#			 */
/*   Updated: 2026/08/11 16:17:07 by djanardh		 ###   ########.fr	   */
/*																			*/
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "ResponseBuilder.hpp"
#include <sys/stat.h> // for checking if files exist
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio> // for std::remove
#include <cstdlib>

RequestHandler::RequestHandler(const ServerConfig& config) : _config(config) {}
RequestHandler::~RequestHandler() {}

// Matching the Location Block (Prefix Matching). If a user requests /images/avatar.png, and we 
// have location configurations for / and /images, the router must choose the longest, most specific match (/images)

const LocationConfig* RequestHandler::matchLocation(const std::string& uri) const
{
	const LocationConfig* bestMatch = NULL;
	size_t longestMatchLength = 0;

	for (size_t i = 0; i < _config.locations.size(); ++i) {
		const LocationConfig& loc = _config.locations[i];
		
		// prefix match evaluation
		if (uri.rfind(loc.path, 0) == 0) {
			if (loc.path.size() > longestMatchLength) {
				longestMatchLength = loc.path.size();
				bestMatch = &loc;
			}
		}
	}
	return bestMatch; // returns NULL if no location blocks match, falling back to server default root
}

// checking the 405 Method Not Allowed Rule. If a location block specifies that only GET is allowed, 
// any incoming POST or DELETE request must halt immediately
void RequestHandler::verifyMethodAllowed(const std::string& method, const LocationConfig* location) const
{
	if (!location) return; // if no location block matches, default global rules apply

	// if allowedMethods is empty, assume all methods are permitted by default
	if (location->allowedMethods.empty()) return;

	if (location->allowedMethods.find(method) == location->allowedMethods.end()) {
		// throw a handler exception that the Response Builder will catch to turn into a 405 response!
		throw std::runtime_error("405"); 
	}
}

// translating the URI to a Real File Path on Your System (to serve a file, we combine the configuration's designated root 
// folder directory with the client's requested trailing URI string path)
std::string RequestHandler::translateToPhysicalPath(const std::string& uri, const LocationConfig* location) const
{
	std::string baseRoot = location ? location->root : _config.root;
	std::string path = baseRoot + uri;

	// check if the resulting path targets a directory. If it does, try to append the index file name
	struct stat pathStat;
	if (stat(path.c_str(), &pathStat) == 0) {
		if (S_ISDIR(pathStat.st_mode)) {
			std::string indexFile = location ? location->index : _config.index;
			if (indexFile.empty()) indexFile = "index.html"; // Standard fallback
			
			if (path.back() == '/') path += indexFile;
			else					path += "/" + indexFile;
		}
	}
	return path;
}

// attempts to open a file on our hard drive. If it exists, it reads the content and builds a 200 OK response
// If it doesn't exist, it throws a 404 Not Found error
std::string RequestHandler::handleGet(const std::string& physicalPath, const LocationConfig* location)
{
	(void)location; // Unused for now, but useful later if you add autoindex

	// attempt to open the requested file in binary read mode
	std::ifstream file(physicalPath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("404"); // file does not exist -> 404 Not Found
	}

	// read the entire file content into a string stream
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string fileContent = buffer.str();
	file.close();

	// determine the length of the file content
	std::stringstream lengthStream;
	lengthStream << fileContent.size();
	std::string contentLength = lengthStream.str();

	// build a simple standard HTTP response
	std::string response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: " + contentLength + "\r\n"
		"Content-Type: text/html\r\n" // defaulting to HTML for simplicity
		"Connection: keep-alive\r\n"
		"\r\n" + 
		fileContent;

	return response;
}

// this function takes the raw body data sent by the client and saves it as a new file on your hard drive
// if successful, it returns a 201 Created response.
std::string RequestHandler::handlePost(const std::string& physicalPath, const HttpRequestContainer& request)
{
	// Attempt to create/open a file in binary write mode (overwrites if it exists)
	std::ofstream file(physicalPath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("500"); // internal Server Error (e.g., directory doesn't exist)
	}

	// write the request body directly into the file
	file.write(request.getBody().data(), request.getBody().size());
	file.close();

	// prepare a confirmation body payload
	std::string bodyMessage = "<h1>File Created Successfully</h1>";
	std::stringstream lengthStream;
	lengthStream << bodyMessage.size();

	// build the 201 Created response
	std::string response = 
		"HTTP/1.1 201 Created\r\n"
		"Content-Length: " + lengthStream.str() + "\r\n"
		"Content-Type: text/html\r\n"
		"Location: " + request.getUri() + "\r\n"
		"\r\n" + 
		bodyMessage;

	return response;
}

// checks if a file exists, and if it does, deletes it using the standard C++ std::remove utility
// if successful, it returns a 200 OK or 204 No Content response.
std::string RequestHandler::handleDelete(const std::string& physicalPath)
{
	// check if the file exists before attempting to delete it
	struct stat pathStat;
	if (stat(physicalPath.c_str(), &pathStat) != 0) {
		throw std::runtime_error("404");
	}

	// enforce that we cannot delete a whole directory folder directly via DELETE
	if (S_ISDIR(pathStat.st_mode)) {
		throw std::runtime_error("400"); // Bad Request
	}

	// delete the file using standard system call
	if (std::remove(physicalPath.c_str()) != 0) {
		throw std::runtime_error("500"); // Internal Server Error (e.g., permission denied)
	}

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


// std::string RequestHandler::processRequest(const HttpRequestContainer& request)
// {
// 	try {
// 		// 1. Find configuration block matches
// 		const LocationConfig* matchedLoc = matchLocation(request.getUri());

// 		// 2. Validate routing method permissions (Throws 405 if blocked)
// 		verifyMethodAllowed(request.getMethod(), matchedLoc);

// 		// 3. Convert URI string to a real system file address path
// 		std::string physicalPath = translateToPhysicalPath(request.getUri(), matchedLoc);

// 		// 4. Branch execution to chosen handler based on HTTP Method type
// 		if (request.getMethod() == "GET") {
// 			return handleGet(physicalPath, matchedLoc);
// 		} 
// 		else if (request.getMethod() == "POST") {
// 			return handlePost(physicalPath, request);
// 		} 
// 		else if (request.getMethod() == "DELETE") {
// 			return handleDelete(physicalPath);
// 		}
		
// 		throw std::runtime_error("501"); // Not Implemented fallback guard
// 	}
// 	catch (const std::runtime_error& errorMsg) {
// 		// This is where your upcoming Response Builder takes over to format an Error Response packet!
// 		std::string status = errorMsg.what();
// 		std::cout << "Handling Error Status Code: " << status << "\n";
// 		return "HTTP/1.1 " + status + " Error\r\n\r\n"; // Temporary mock string
// 	}
// }

// Helper utility to safely convert strings to numbers for status code conversion
static int stringToInt(const std::string& s)
{
	std::stringstream ss(s);
	int num;
	ss >> num;
	return num;
}

std::string RequestHandler::processRequest(const HttpRequestContainer& request)
{
	ResponseBuilder response; // instantiate a clean ResponseBuilder container

	try {
		// 1. Find configuration block matches
		const LocationConfig* matchedLoc = matchLocation(request.getUri());

		// 2. Validate routing method permissions (Throws a runtime_error with status string if blocked)
		verifyMethodAllowed(request.getMethod(), matchedLoc);

		// 3. Convert abstract URI string to a real system file address path
		std::string physicalPath = translateToPhysicalPath(request.getUri(), matchedLoc);

		// 4. Branch execution to the matching handler based on HTTP Method type
		if (request.getMethod() == "GET") {
			return handleGet(physicalPath, matchedLoc);
		} 
		else if (request.getMethod() == "POST") {
			return handlePost(physicalPath, request);
		} 
		else if (request.getMethod() == "DELETE") {
			return handleDelete(physicalPath);
		}
		
		throw std::runtime_error("501"); // Not Implemented fallback guard
	}
	catch (const std::runtime_error& errorMsg) {
		// handles structural runtime codes thrown by the execution handlers (e.g. "404", "405")
		int statusCode = stringToInt(errorMsg.what());
		if (statusCode == 0) statusCode = 500; // Fallback to Internal Error if conversion fails

		response.generateDefaultErrorPage(statusCode);
		return response.buildString(); // Returns a beautifully compiled HTTP error packet
	}
	catch (const HttpParseException& parseError) {
		// captures bad request inputs coming out of your initial Parser stages
		response.generateDefaultErrorPage(parseError.getStatusCode());
		return response.buildString();
	}
	catch (...) {
		// global safety net catch-all for any completely unexpected system failure
		response.generateDefaultErrorPage(500);
		return response.buildString();
	}
}
