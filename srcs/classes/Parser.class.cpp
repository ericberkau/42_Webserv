/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/29 16:35:46 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/30 15:55:13 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Parser.class.hpp"
#include <sstream>

Parser::Parser(const std::vector<Token>& tokens)
	: _tokens(tokens), _currentPos(0)
{}

Config Parser::parse(void) {
	return _parseConfig();
}


/* private member functions */

void Parser::_advance(void) {
	if (!_atEnd())
		++_currentPos;
}

bool Parser::_atEnd(void) const {
	return _current().type == TokenType::END;
}

const Token& Parser::_current(void) const {
	return _tokens[_currentPos];
}

const Token& Parser::_previous(void) const {
	if (_currentPos > 0)
		return _tokens[_currentPos - 1];
	return _tokens.back();
} // FIXME: check if needed

bool Parser::_check(TokenType type) const {
	return _current().type == type;
}

// bool Parser::_match(TokenType type) {
// 	if (!_check(type))
// 		return false;

// 	_advance();
// 	return true;
// } // FIXME: check if needed

[[noreturn]]
static void error(const Token& token, const std::string& msg) {
	std::ostringstream oss;
	oss << "Parse error at "
		<< token.line << ':' << token.column
		<< ": " << msg;

	throw std::runtime_error(oss.str());
}

void Parser::_expect(TokenType type, const std::string& errMsg) {
	if (!_check(type))
		error(_current(), errMsg);
}

void Parser::_expect(const std::string& lexeme, const std::string& errMsg) {
	if (!_check(TokenType::WORD) || _current().lexeme != lexeme)
		error(_current(), errMsg);
}

Config Parser::_parseConfig(void) {
	Config config;

	while (!_atEnd())
		config.servers.push_back(_parseServer());

	return config;
}

ServerConfig Parser::_parseServer(void) {
	_expect("server", "expected 'server'");
	_advance();
	_expect(TokenType::BRACE_LEFT, "expected '{' after 'server'");
	_advance();

	ServerConfig server;

	while (!_check(TokenType::BRACE_RIGHT)) {
		if (_atEnd())
			error(_current(), "expected '}' after server block");

		_parseServerDirective(server);
	}
	_advance();

	return server;
}

void Parser::_parseServerDirective(ServerConfig& server) {
	_expect(TokenType::WORD, "expected server directive");
	const Token& directive = _current();

	if (directive.lexeme == "listen") {
		_advance();
		_parseListen(server);
	}
	else
		error(directive, "unknown server directive '" + directive.lexeme + "'");
}

void Parser::_parseListen(ServerConfig& server) {
	_expect(TokenType::WORD, "expected address after 'listen'");

	const Token& addressToken = _current();
	_advance();
	
	_expect(TokenType::SEMICOLON, "expected ';' after listen directive");
	_advance();

	//TODO
}
