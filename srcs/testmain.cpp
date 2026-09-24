/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   testmain.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/08 18:07:00 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/08 20:57:55 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// #include "config.hpp"

// /* testing lexer */
// #include "classes/Lexer.class.hpp"
// #include <iostream>

// static const char* tokenTypeName(TokenType type) {
// 	switch (type) {
// 		case TokenType::WORD:
// 			return "WORD";
// 		case TokenType::BRACE_LEFT:
// 			return "BRACE_LEFT";
// 		case TokenType::BRACE_RIGHT:
// 			return "BRACE_RIGHT";
// 		case TokenType::SEMICOLON:
// 			return "SEMICOLON";
// 		case TokenType::END:
// 			return "END";
// 	}
// 	return "UNKNOWN";
// }

// int main() {
// 	std::string input = "server {\n"
// 						" listen 127.0.0.1:8080;\n"
// 						"# this is a comment and should not show up {\n"
// 						"# for real} \n"
// 						" root ./www;\n"
// 						"}\n";
// 	Lexer lexer(input);
// 	std::vector<Token> tokens = lexer.tokenize();

// 	for (const Token& token : tokens) {
// 		std::cout
// 			<< tokenTypeName(token.type)
// 			<< " \"" << token.lexeme << "\""
// 			<< " (" << token.line << ':' << token.column << ")" << '\n';
// 	}
// }




/* extended parser testing */
#include "classes/Lexer.class.hpp"
#include "classes/Parser.class.hpp"
#include "classes/Server.class.hpp"
#include "signals.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

static std::string methodToString(HttpMethod method)
{
	switch (method)
	{
		case HttpMethod::GET:
			return "GET";

		case HttpMethod::POST:
			return "POST";

		case HttpMethod::DELETE:
			return "DELETE";

		case HttpMethod::INVALID:
			return "INVALID";
	}

	return "UNKNOWN";
}

static void printLocation(const LocationConfig& loc) {
	std::cout
		<< "\nlocation "
		<< loc.path
		<< '\n';

	std::cout
		<< "  root: "
		<< loc.root
		<< '\n';

	std::cout
		<< "  index: "
		<< loc.index
		<< '\n';

	std::cout
		<< "  autoindex: "
		<< (loc.autoIndex ? "on" : "off")
		<< '\n';

	std::cout
		<< "  upload_enable: "
		<< (loc.uploadEnabled ? "on" : "off")
		<< '\n';

	std::cout
		<< "  upload_path: "
		<< loc.uploadPath
		<< '\n';

	std::cout
		<< "  methods: ";

	for (const HttpMethod& m : loc.allowedMethods)
	{
		std::cout
			<< methodToString(m)
			<< ' ';
	}

	std::cout << '\n';

	if (loc.hasRedirect)
	{
		std::cout
			<< "  redirect: "
			<< loc.redirectStatusCode
			<< " -> "
			<< loc.redirectTarget
			<< '\n';
	}

	if (!loc.cgiHandlers.empty())
	{
		std::cout << "  cgi:\n";

		for (std::map<
				std::string,
				std::string>::const_iterator
			it = loc.cgiHandlers.begin();
			it != loc.cgiHandlers.end();
			++it)
		{
			std::cout
				<< "    "
				<< it->first
				<< " -> "
				<< it->second
				<< '\n';
		}
	}
}

static void printServer(const ServerConfig& server, size_t serverIndex) {
	std::cout
		<< "SERVER #" << serverIndex + 1
		<< "\n-------------------------\n";

	for (std::size_t j = 0; j < server.listen.size(); ++j)
	{
		std::cout
			<< "listen: "
			<< server.listen[j].host
			<< ':'
			<< server.listen[j].port
			<< '\n';
	}

	std::cout
		<< "client_max_body_size: "
		<< server.clientMaxBodySize
		<< '\n';

	std::cout
		<< "global root: "
		<< server.root
		<< '\n';

	std::cout << "\nerror pages:\n";

	for (std::map<int, std::string>::const_iterator
		it = server.errorPages.begin();
		it != server.errorPages.end();
		++it)
	{
		std::cout
			<< "  "
			<< it->first
			<< " -> "
			<< it->second
			<< '\n';
	}

	std::cout << "\nlocations:\n";

	for (std::size_t k = 0;
		k < server.locations.size();
		++k)
	{
		printLocation(server.locations[k]);
	}

}

static void printConfig(const Config& config) {
	for (std::size_t i = 0; i < config.servers.size(); ++i)
	{
		printServer(config.servers[i], i);

		std::cout << "\n=========================\n\n";
	}
}

static std::string readFile(const std::string& path)
{
	std::ifstream file(path.c_str());

	if (!file)
		throw std::runtime_error("could not open config file");

	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

int main(int argc, char** argv)
{
	try
	{
		std::string source;
		
		if (argc > 2)
		{
			std::cerr << "Usage: "
			          << argv[0]
			          << " [config_file]\n";
			return 1;
		}
		else if (argc == 2)
			source = argv[1];
		else
			source = "configs/basic.conf";

		const std::string input = readFile(source);

		Lexer lexer(input);

		std::vector<Token> tokens = lexer.tokenize();

		std::cout
			<< "\n== Token count: "
			<< tokens.size()
			<< " ==\n";

		Parser parser(tokens);

		Config config = parser.parse();
		std::cout << "\n===== CONFIG PARSED SUCCESSFULLY =====\n\n";

		validateConfig(config);
		std::cout << "===== CONFIG VALIDATED SUCCESSFULLY =====\n\n\n";

		printConfig(config);
		std::cout << "******************************\n" << std::endl;

		/* server testing */
		signal(SIGINT, handleShutdownSignal);
		signal(SIGTERM, handleShutdownSignal);

		Server server(config);
		server.run();
	}
	catch (const std::exception& e)
	{
		std::cerr
			<< "ERROR: "
			<< e.what()
			<< '\n';

		return 1;
	}

	return 0;
}
