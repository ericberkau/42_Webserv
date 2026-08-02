/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/01 18:02:19 by djanardh          #+#    #+#             */
/*   Updated: 2026/08/02 12:54:42 by djanardh         ###   ########.fr       */
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
            else                    path += "/" + indexFile;
        }
    }
    return path;
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

