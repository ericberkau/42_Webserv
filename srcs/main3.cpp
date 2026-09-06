#include "RequestParser.hpp"
#include "RequestHandler.hpp"
#include "ResponseBuilder.hpp"
#include "ConfigStructures.hpp"
#include <iostream>
#include <sys/stat.h>

// The helper function that simulates network packets coming in
void simulateNetworkEvent(const std::string& testName, const ServerConfig& config, const std::string& rawInput)
{
	std::cout << "==================================================\n";
	std::cout << "TEST: " << testName << "\n";
	std::cout << "==================================================\n";

	RequestParser parser;
	HttpRequestContainer request;
	RequestHandler handler(config);
	std::string finalNetworkOutput;

	try
	{
		// Step 1: Feed data chunks into your parser state machine
		parser.feed(rawInput, request);

		// Step 2: If parsing finishes successfully, pass it to the handler
		if (parser.getState() == RequestParser::PARSE_COMPLETE)
		{
			finalNetworkOutput = handler.processRequest(request);
		}
		else
		{
			ResponseBuilder rb;
			rb.generateDefaultErrorPage(400); 
			finalNetworkOutput = rb.buildString();
		}
	}
	catch (const HttpParseException& e)
	{
		ResponseBuilder rb;
		rb.generateDefaultErrorPage(e.getStatusCode(), config);
		finalNetworkOutput = rb.buildString();
	}
	catch (...)
	{
		ResponseBuilder rb;
		rb.generateDefaultErrorPage(500);
		finalNetworkOutput = rb.buildString();
	}

	// Print the raw HTTP network packet that goes back to the browser
	std::cout << finalNetworkOutput << "\n";
}

int main()
{
	// 1. Setup a basic server configuration layout
	ServerConfig config;
	config.root = "."; // Points to the current directory
	config.index = "this_index_does_not_exist.html"; // Force it to miss index files
	config.errorPages[404] = "/custom_404.html"; // Custom error page mapping

	// 2. Setup a strict route that only allows GET operations
	LocationConfig apiRoute;
	apiRoute.path = "/api";
	apiRoute.root = "./public_html/api";
	apiRoute.allowedMethods.insert("GET");
	config.locations.push_back(apiRoute);

	// 3. Create a location block with autoindex explicitly TURNED ON
	LocationConfig listableRoute;
	listableRoute.path = "/files";
	listableRoute.root = "."; // Points to your current project folder
	listableRoute.allowedMethods.insert("GET");
	listableRoute.autoindex = true; //TURN AUTOINDEX ON!
	config.locations.push_back(listableRoute);


	// --- EXECUTE THE TEST LIFE CYCLES ---

	// TEST A: Malformed input test (Parser Exception -> 400 Bad Request)
	simulateNetworkEvent(
		"Malformed Method (Lowercase Token)", 
		config, 
		"get /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
	);

	// TEST B: Space before colon check (Parser Exception -> 400 Bad Request)
	simulateNetworkEvent(
		"Space Before Colon Violation", 
		config, 
		"GET /index.html HTTP/1.1\r\nHost : localhost\r\n\r\n"
	);

	// TEST C: Routing Violation (Handler rule exception -> 405 Method Not Allowed)
	simulateNetworkEvent(
		"Route Method Restriction (POSTing to GET-only /api)", 
		config, 
		"POST /api HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello"
	);

	// TEST D: Triggering Directory Listing (Autoindex Engine -> 200 OK with HTML Links)
	simulateNetworkEvent(
		"Directory Listing (Autoindex Enabled)", 
		config, 
		"GET /files/ HTTP/1.1\r\nHost: localhost\r\n\r\n"
	);

	return 0;
}
