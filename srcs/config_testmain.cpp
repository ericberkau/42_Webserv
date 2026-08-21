/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_testmain.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/08 18:07:00 by eberkau           #+#    #+#             */
/*   Updated: 2026/08/22 00:57:32 by eberkau          ###   ########.fr       */
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




/* testing parser */
#include "classes/Lexer.class.hpp"
#include "classes/Parser.class.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

static void printConfig(const Config& config)
{
	for (size_t i = 0; i < config.servers.size(); ++i)
	{
		const ServerConfig& server = config.servers[i];

		std::cout << "SERVER #" << i << '\n';

		for (size_t j = 0; j < server.listen.size(); ++j)
		{
			std::cout
				<< "  listen: "
				<< server.listen[j].host
				<< ':'
				<< server.listen[j].port
				<< '\n';
		}

		std::cout
			<< "  client_max_body_size: "
			<< server.clientMaxBodySize
			<< '\n';

		for (size_t j = 0; j < server.locations.size(); ++j)
		{
			const LocationConfig& location =
				server.locations[j];

			std::cout
				<< "  location "
				<< location.path
				<< '\n';

			std::cout
				<< "    root: "
				<< location.root
				<< '\n';

			std::cout
				<< "    autoindex: "
				<< (location.autoIndex ? "on" : "off")
				<< '\n';
		}

		std::cout << '\n';
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
		if (argc != 2)
		{
			std::cerr << "Usage: "
			          << argv[0]
			          << " <config_file>\n";
			return 1;
		}

		const std::string input = readFile(argv[1]);

		Lexer lexer(input);
		std::vector<Token> tokens = lexer.tokenize();

		Parser parser(tokens);
		Config config = parser.parse();

		std::cout << "Config parsed successfully.\n";

		printConfig(config);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}

	return 0;
}
