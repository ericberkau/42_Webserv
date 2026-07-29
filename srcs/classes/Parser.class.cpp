/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/29 16:35:46 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/30 00:13:50 by eberkau          ###   ########.fr       */
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
}

bool Parser::_check(TokenType type) const {
	return _current().type == type;
}

// bool Parser::_match(TokenType type) {
// 	if (!_check(type))
// 		return false;

// 	_advance();
// 	return true;
// } // FIXME: check if needed

void Parser::_expect(TokenType type, const std::string& msg) {
	if (_check(type)) {
		_advance();
		return;
	}

	std::ostringstream oss;
	oss << "Parse error at "
		<< _current().line << ':' << _current().column
		<< ": " << msg;

	throw std::runtime_error(oss.str());
}

Config Parser::_parseConfig(void) {
	Config config;

	while (!_atEnd())
		config.servers.push_back(_parseServer());

	return config;
}

ServerConfig Parser::_parseServer(void) {
	_expect(TokenType::WORD, "expected 'server'");

	// if ()
	// TODO: continue here
}

void Parser::_parseServerDirective(ServerConfig& server) {

}

void Parser::_parseListen(ServerConfig& server) {

}
