/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/11 14:57:27 by eberkau           #+#    #+#             */
/*   Updated: 2026/06/23 21:15:00 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestParser.hpp"
#include <iostream>

// Helper to run one test case and print the result.
static void runTest(const std::string& name, const std::string& raw)
{
	std::cout << "=== " << name << " ===" << std::endl;
	try
	{
		HttpRequestContainer req = RequestParser::parse(raw);
		std::cout << "OK, parsed:" << std::endl;
		req.print();
	}
	catch (const HttpParseException& e)
	{
		std::cout << "REJECTED -> status " << e.getStatusCode()
				   << " (" << e.what() << ")" << std::endl;
	}
	std::cout << std::endl;
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
		"Host:    localhost\r\n"
		"\r\n");

	// --- Deliberately broken cases (this is the telnet-style poking,
	//     just hardcoded instead of typed by hand each time) ---

	runTest("Header key:value pair with space BEFORE the colon",
		"GET / HTTP/1.1\r\n"
		"Host   :    localhost\r\n"
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

	return 0;
}

