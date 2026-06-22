/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/11 14:57:27 by eberkau           #+#    #+#             */
/*   Updated: 2026/06/22 16:10:28 by djanardh         ###   ########.fr       */
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
		"GET /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n");

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

	return 0;
}

