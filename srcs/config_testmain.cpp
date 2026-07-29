/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_testmain.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/08 18:07:00 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/29 14:29:05 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// #include "config.hpp"

#include "classes/Lexer.class.hpp"
#include <iostream>

static const char* tokenTypeName(TokenType type) {
	switch (type) {
		case TokenType::WORD:
			return "WORD";
		case TokenType::BRACE_LEFT:
			return "BRACE_LEFT";
		case TokenType::BRACE_RIGHT:
			return "BRACE_RIGHT";
		case TokenType::SEMICOLON:
			return "SEMICOLON";
		case TokenType::END:
			return "END";
	}
	return "UNKNOWN";
}

int main() {
	std::string input = "server {\n"
						" listen 127.0.0.1:8080;\n"
						"# this is a comment and should not show up {\n"
						"# for real} \n"
						" root ./www;\n"
						"}\n";
	Lexer lexer(input);
	std::vector<Token> tokens = lexer.tokenize();

	for (const Token& token : tokens) {
		std::cout
			<< tokenTypeName(token.type)
			<< " \"" << token.lexeme << "\""
			<< " (" << token.line << ':' << token.column << ")" << '\n';
	}
}
