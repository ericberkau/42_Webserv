/* ************************************************************************** */
/*																			*/
/*														:::	  ::::::::   */
/*   main.cpp										   :+:	  :+:	:+:   */
/*													+:+ +:+		 +:+	 */
/*   By: djanardh <djanardh@student.42heilbronn.	+#+  +:+	   +#+		*/
/*												+#+#+#+#+#+   +#+		   */
/*   Created: 2026/05/11 14:57:27 by eberkau		   #+#	#+#			 */
/*   Updated: 2026/06/24 01:31:42 by djanardh		 ###   ########.fr	   */
/*																			*/
/* ************************************************************************** */

#include "RequestParser.hpp"
#include <iostream>

// Helper to run test cases and print the result
// static void runTest(const std::string& name, const std::string& raw)
// {
// 	std::cout << "=== " << name << " ===" << std::endl;
// 	try
// 	{
// 		HttpRequestContainer req = RequestParser::parse(raw);
// 		std::cout << "OK, parsed:" << std::endl;
// 		req.print();
// 	}
// 	catch (const HttpParseException& e)
// 	{
// 		std::cout << "REJECTED -> status " << e.getStatusCode()
// 				   << " (" << e.what() << ")" << std::endl;
// 	}
// 	std::cout << std::endl;
// }

static void runTest(const std::string& testName, const std::string& rawRequest)
{
	std::cout << "Running: " << testName << "\n";
	try
	{
		RequestParser parser; // Create a living instance
		HttpRequestContainer request;

		// Feed the data into the instance state machine
		parser.feed(rawRequest, request);

		if (parser.getState() == RequestParser::PARSE_COMPLETE)
		{
			std::cout << "Parsing success!\n";
			request.print();
		} 
		else
		{
			std::cout << "Request is incomplete (waiting for more chunks).\n";
		}
	}
	catch (const HttpParseException& e)
	{
		std::cout << "Caught expected/unexpected error: " << e.what() << "\n";
	}
}


int main()
{
	// --- Valid cases ---

	runTest("Simple GET",
		"GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n");

	runTest("GET with multiple headers + query string",
		"GET /search?q=cats HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"User-Agent: test-client\r\n"
		"Accept: */*\r\n"
		"\r\n");

	runTest("POST with a body",
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Length: 11\r\n"
		"\r\n"
		"hello world");

	runTest("Header value with extra spaces around colon",
		"GET / HTTP/1.1\r\n"
		"Host:	localhost\r\n"
		"\r\n");

	// --- Deliberately broken cases (this is the telnet-style poking,
	//	 just hardcoded instead of typed by hand each time) ---

	runTest("Header key:value pair with space BEFORE the colon",
		"GET / HTTP/1.1\r\n"
		"Host   :	localhost\r\n"
		"\r\n");
	
	runTest("Missing HTTP version",
		"GET /\r\n"
		"Host: localhost\r\n"
		"\r\n");

	runTest("Lowercase method",
		"get / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n");

	runTest("URI missing leading slash",
		"GET index.html HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n");

	runTest("Malformed header (no colon)",
		"GET / HTTP/1.1\r\n"
		"Host localhost\r\n"
		"\r\n");

	runTest("No blank line to end headers",
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n");

	runTest("Bad HTTP version string",
		"GET / HTTP/9.9\r\n"
		"Host: localhost\r\n"
		"\r\n");

	// --- Tests after completing header and request line parsing (I hope?) 23/06/26---

	runTest("Valid Absolute URI Form",
		"GET http://localhost:8080/index.html HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"\r\n");

	runTest("Valid Empty Header Value",
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"X-Custom-Empty-Flag:\r\n"
		"\r\n");
		
	runTest("Invalid URI with a physical control character (Tab)",
		"GET /my\tpath HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n");

	runTest("Invalid Method Triage (Valid token format but unsupported)",
		"PUT /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n");

	runTest("Invalid Method Triage",
		"G@T /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n");

	runTest("Invalid Space Before Colon (Tab character variant)",
		"GET / HTTP/1.1\r\n"
		"Host\t: localhost\r\n"
		"\r\n");


	// runTest("Invalid Request-Line exceeding 8000 octets",
	// 	"GET /" + std::string(8005, 'a') + " HTTP/1.1\r\nHost: localhost\r\n\r\n");



	// --- Advanced Body Parsing Fragmented Tests ---

	// 1. Content-Length: Arriving in two split network packets
	{
		std::cout << "Running: Content-Length Fragmented Packet Test\n";
		RequestParser parser;
		HttpRequestContainer request;

		// Packet 1: Contains request-line, headers, and only the FIRST half of the body
		std::string packet1 = "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\n\r\nhello";
		parser.feed(packet1, request);
		
		if (parser.getState() != RequestParser::PARSE_BODY)
			std::cout << "Error: State should be PARSE_BODY while waiting for rest of data\n";

		// Packet 2: The remaining data bytes finally arrive
		std::string packet2 = "world";
		parser.feed(packet2, request);

		if (parser.getState() == RequestParser::PARSE_COMPLETE && request.getBody() == "helloworld")
			std::cout << "Success: Fragmented Content-Length parsed correctly!\n";
		else
			std::cout << "Error: Fragmented Content-Length failed. Body was: " << request.getBody() << "\n";
	}

	// 2. Chunked Transfer Encoding: Arriving sequentially 
	{
		std::cout << "Running: Valid Chunked Stream Test\n";
		RequestParser parser;
		HttpRequestContainer request;

		std::string chunkedRequest = 
			"POST /stream HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"5\r\n"
			"Hello\r\n"
			"6\r\n"
			" World\r\n"
			"0\r\n\r\n"; // Terminating block

		parser.feed(chunkedRequest, request);

		if (parser.getState() == RequestParser::PARSE_COMPLETE && request.getBody() == "Hello World")
			std::cout << "Success: Chunked body decoded perfectly!\n";
		else
			std::cout << "Error: Chunked stream failed. Body was: " << request.getBody() << "\n";
	}

	// 3. Chunked Transfer Encoding: Split directly in the middle of a hex size line
	{
		std::cout << "Running: Fragmented Chunked Line Test\n";
		RequestParser parser;
		HttpRequestContainer request;

		// Feed headers and the start of a chunk size line, but cut it off!
		parser.feed("POST / HTTP/1.1\r\nHost: local\r\nTransfer-Encoding: chunked\r\n\r\n4", request);
		
		// Feed the rest of the size line and its payload
		parser.feed("\r\ntest\r\n0\r\n\r\n", request);

		if (parser.getState() == RequestParser::PARSE_COMPLETE && request.getBody() == "test")
			std::cout << "Success: Fragmented chunked sizes handled gracefully!\n";
		else
			std::cout << "Error: Fragmented chunked parser failed.\n";
	}

	// 4. Missing Length on POST Request (411 Check)
	{
		std::cout << "Running: POST Missing Length Headers Test (411 Check)\n";
		try
		{
			RequestParser parser;
			HttpRequestContainer request;
			parser.feed("POST /submit HTTP/1.1\r\nHost: localhost\r\n\r\n", request);
			std::cout << "Error: Server accepted a POST request with no body size declarations!\n";
		}
		catch (const HttpParseException& e)
		{
			if (e.getStatusCode() == 411)
				std::cout << "Success: Correctly rejected with 411 Length Required\n";
			else
				std::cout << "Error: Threw wrong code " << e.getStatusCode() << "\n";
		}
	}

	// 5. Max Body Limit Breach (413 Check)
	{
		std::cout << "Running: Payload Too Large Test (413 Check)\n";
		try
		{
			RequestParser parser;
			HttpRequestContainer request;
			// Tell the parser 60 Million bytes are coming (Our limit is 50 Million)
			parser.feed("POST /big-upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 60000000\r\n\r\n", request);
			std::cout << "Error: Allowed a size metadata overflow!\n";
		}
		catch (const HttpParseException& e)
		{
			if (e.getStatusCode() == 413)
				std::cout << "Success: Correctly blocked oversized payload with 413\n";
			else
				std::cout << "Error: Threw wrong code " << e.getStatusCode() << "\n";
		}
	}


	return 0;
}

