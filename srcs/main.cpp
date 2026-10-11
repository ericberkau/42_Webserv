/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/08 18:07:00 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/11 19:46:00 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/config/ConfigLexer.class.hpp"
#include "classes/config/ConfigParser.class.hpp"
#include "classes/server/Server.class.hpp"
#include "signals.hpp"
#include "stringUtils.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

void printConfig(const Config& config);

static
std::string readFile(const std::string& path) {
	std::ifstream file(path.c_str());

	if (!file)
		throw std::runtime_error("could not open config file");

	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

int main(int argc, char** argv) {
	try {
		std::string source;
		
		if (argc > 2) {
			std::cerr << "Usage: "
			          << argv[0]
			          << " [config_file]\n";
			return 1;
		}
		else if (argc == 2)
			source = argv[1];
		else
			source = "configs/basic.conf";

		std::cout << "\033[0;33m\nConfig file: '" << source << "'\033[0m" << std::endl;

		const std::string input = readFile(source);

		ConfigLexer lexer(input);
		std::vector<Token> tokens = lexer.tokenize();

		// std::cout
		// 	<< "\033[0;33m\n== Token count: "
		// 	<< tokens.size()
		// 	<< " ==\n\033[0m";

		ConfigParser parser(tokens);
		Config config = parser.parse();

		std::cout << "\033[0;33m\n===== CONFIG PARSED SUCCESSFULLY =====\n\033[0m";

		validateConfig(config);

		std::cout << "\033[0;33m\n===== CONFIG VALIDATED SUCCESSFULLY =====\n\n\n\033[0m";

		// printConfig(config);

		std::cout << "******************************\n" << std::endl;

		signal(SIGINT, handleShutdownSignal);
		signal(SIGTERM, handleShutdownSignal);

		Server server(config);
		server.run();
	}
	catch (const std::exception& e) {
		std::cerr
			<< "ERROR: "
			<< e.what()
			<< '\n';

		return 1;
	}

	return 0;
}
