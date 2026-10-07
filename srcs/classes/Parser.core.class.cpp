/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.core.class.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/29 16:35:46 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/07 18:47:05 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Parser.class.hpp"
#include "classes/parserUtils.hpp"


/* ------- public member functions ------- */

Parser::Parser(const std::vector<Token>& tokens) : _tokens(tokens), _currentPos(0) {}

Config Parser::parse(void) {
	return _parseConfig();
}


/* ------- token traversal ------- */

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


/* ------- parsing foundation ------- */

bool Parser::_check(TokenType type) const {
	return _current().type == type;
}

void Parser::_expect(TokenType type, const std::string& errMsg) {
	if (!_check(type))
		error(_current(), errMsg);
}

void Parser::_expect(const std::string& keyword, const std::string& errMsg) {
	if (!_check(TokenType::WORD) || _current().lexeme != keyword)
		error(_current(), errMsg);
}

Config Parser::_parseConfig(void) {
	Config config;
	
	while (!_atEnd())
		config.servers.push_back(_parseServer());
	
	// TODO: add config verification?
	return config;
}
// TODO: check where duplicates are still accepted and can overwrite each other (work-in-progress)
