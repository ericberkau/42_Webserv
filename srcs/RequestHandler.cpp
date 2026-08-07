/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/07 19:28:12 by djanardh          #+#    #+#             */
/*   Updated: 2026/08/07 19:28:14 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include <sys/stat.h> // For checking if files exist
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio> // Required for std::remove

RequestHandler::RequestHandler(const ServerConfig& config) : _config(config) {}
RequestHandler::~RequestHandler() {}

// Matching the Location Block (Prefix Matching). If a user requests /images/avatar.png, and you 
// have location configurations for / and /images, your router must choose the longest, most specific match (/images).

const LocationConfig* RequestHandler::matchLocation(const std::string& uri) const {
	const LocationConfig* bestMatch = NULL;
	size_t longestMatchLength = 0;

	for (size_t i = 0; i < _config.locations.size(); ++i) {
		const LocationConfig& loc = _config.locations[i];
		
		// Simple prefix match evaluation
		if (uri.rfind(loc.path, 0) == 0) {
			if (loc.path.size() > longestMatchLength) {
				longestMatchLength = loc.path.size();
				bestMatch = &loc;
			}
		}
	}
	return bestMatch; // Returns NULL if no location blocks match, falling back to server default root
}

// Checking the 405 Method Not Allowed Rule. If a location block specifies that only GET is allowed, 
// any incoming POST or DELETE request must halt immediately
void RequestHandler::verifyMethodAllowed(const std::string& method, const LocationConfig* location) const {
	if (!location) return; // If no location block matches, default global rules apply

	// If allowedMethods is empty, assume all methods are permitted by default
	if (location->allowedMethods.empty()) return;

	if (location->allowedMethods.find(method) == location->allowedMethods.end()) {
		// Throw a handler exception that the Response Builder will catch to turn into a 405 response!
		throw std::runtime_error("405"); 
	}
}

// Translating the URI to a Real File Path on Your System: To serve a file, you combine the configuration's designated root 
// folder directory with the client's requested trailing URI string path.
std::string RequestHandler::translateToPhysicalPath(const std::string& uri, const LocationConfig* location) const {
	std::string baseRoot = location ? location->root : _config.root;
	std::string path = baseRoot + uri;

	// Check if the resulting path targets a directory. If it does, try to append the index file name
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

// This function attempts to open a file on your hard drive. If it exists, it reads the content and builds a 200 OK response
// If it doesn't exist, it throws a 404 Not Found error.
std::string RequestHandler::handleGet(const std::string& physicalPath, const LocationConfig* location) {
	(void)location; // Unused for now, but useful later if you add autoindex

	// Attempt to open the requested file in binary read mode
	std::ifstream file(physicalPath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("404"); // File does not exist -> 404 Not Found
	}

	// Read the entire file content into a string stream
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string fileContent = buffer.str();
	file.close();

	// Determine the length of the file content
	std::stringstream lengthStream;
	lengthStream << fileContent.size();
	std::string contentLength = lengthStream.str();

	// Build a simple standard HTTP response
	std::string response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: " + contentLength + "\r\n"
		"Content-Type: text/html\r\n" // Defaulting to HTML for simplicity
		"Connection: keep-alive\r\n"
		"\r\n" + 
		fileContent;

	return response;
}

// This function takes the raw body data sent by the client and saves it as a new file on your hard drive
// If successful, it returns a 201 Created response.
std::string RequestHandler::handlePost(const std::string& physicalPath, const HttpRequestContainer& request) {
	// Attempt to create/open a file in binary write mode (overwrites if it exists)
	std::ofstream file(physicalPath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("500"); // Internal Server Error (e.g., directory doesn't exist)
	}

	// Write the request body directly into the file
	file.write(request.getBody().data(), request.getBody().size());
	file.close();

	// Prepare a confirmation body payload
	std::string bodyMessage = "<h1>File Created Successfully</h1>";
	std::stringstream lengthStream;
	lengthStream << bodyMessage.size();

	// Build the 201 Created response
	std::string response = 
		"HTTP/1.1 201 Created\r\n"
		"Content-Length: " + lengthStream.str() + "\r\n"
		"Content-Type: text/html\r\n"
		"Location: " + request.getUri() + "\r\n"
		"\r\n" + 
		bodyMessage;

	return response;
}

// This function checks if a file exists, and if it does, deletes it using the standard C++ std::remove utility
//  If successful, it returns a 200 OK or 204 No Content response.
std::string RequestHandler::handleDelete(const std::string& physicalPath) {
	// Check if the file exists before attempting to delete it
	struct stat pathStat;
	if (stat(physicalPath.c_str(), &pathStat) != 0) {
		throw std::runtime_error("404"); // File doesn't exist -> 404 Not Found
	}

	// Enforce that you cannot delete a whole directory folder directly via DELETE
	if (S_ISDIR(pathStat.st_mode)) {
		throw std::runtime_error("400"); // Bad Request
	}

	// Delete the file using standard system call
	if (std::remove(physicalPath.c_str()) != 0) {
		throw std::runtime_error("500"); // Internal Server Error (e.g., permission denied)
	}

	// Prepare a confirmation payload
	std::string bodyMessage = "<h1>File Deleted Successfully</h1>";
	std::stringstream lengthStream;
	lengthStream << bodyMessage.size();

	// Build the 200 OK response
	std::string response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: " + lengthStream.str() + "\r\n"
		"Content-Type: text/html\r\n"
		"\r\n" + 
		bodyMessage;

	return response;
}


std::string RequestHandler::processRequest(const HttpRequestContainer& request) {
	try {
		// 1. Find configuration block matches
		const LocationConfig* matchedLoc = matchLocation(request.getUri());

		// 2. Validate routing method permissions (Throws 405 if blocked)
		verifyMethodAllowed(request.getMethod(), matchedLoc);

		// 3. Convert URI string to a real system file address path
		std::string physicalPath = translateToPhysicalPath(request.getUri(), matchedLoc);

		// 4. Branch execution to chosen handler based on HTTP Method type
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
		// This is where your upcoming Response Builder takes over to format an Error Response packet!
		std::string status = errorMsg.what();
		std::cout << "Handling Error Status Code: " << status << "\n";
		return "HTTP/1.1 " + status + " Error\r\n\r\n"; // Temporary mock string
	}
}

