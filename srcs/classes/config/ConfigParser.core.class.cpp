/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.core.class.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/29 16:35:46 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/10 02:59:20 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/config/ConfigParser.class.hpp"
#include "classes/config/configParserUtils.hpp"


/* ------- public member functions ------- */

ConfigParser::ConfigParser(const std::vector<Token>& tokens) : _tokens(tokens), _currentPos(0) {}

Config ConfigParser::parse(void) {
	return _parseConfig();
}


/* ------- token traversal ------- */

void ConfigParser::_advance(void) {
	if (!_atEnd())
		++_currentPos;
}

bool ConfigParser::_atEnd(void) const {
	return _current().type == TokenType::END;
}

const Token& ConfigParser::_current(void) const {
	return _tokens[_currentPos];
}


/* ------- parsing foundation ------- */

bool ConfigParser::_check(TokenType type) const {
	return _current().type == type;
}

void ConfigParser::_expect(TokenType type, const std::string& errMsg) {
	if (!_check(type))
		error(_current(), errMsg);
}

void ConfigParser::_expect(const std::string& keyword, const std::string& errMsg) {
	if (!_check(TokenType::WORD) || _current().lexeme != keyword)
		error(_current(), errMsg);
}

Config ConfigParser::_parseConfig(void) {
	Config config;
	
	while (!_atEnd())
		config.servers.push_back(_parseServer());
	
	return config;
}
