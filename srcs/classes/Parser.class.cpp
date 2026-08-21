/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/29 16:35:46 by eberkau           #+#    #+#             */
/*   Updated: 2026/08/22 00:27:01 by eberkau          ###   ########.fr       */
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
	} else if (directive.lexeme == "client_max_body_size") {
		_advance();
		_parseClientMaxBodySize(server);
	} else if (directive.lexeme == "location") {
		_advance();
		server.locations.push_back(_parseLocation());
	}
	else
		error(directive, "unknown server directive '" + directive.lexeme + "'");
}

static bool isDigits(const std::string& str) {
	if (str.empty())
		return false;

	for (char c : str) {
		if (!std::isdigit(static_cast<unsigned char>(c)))
			return false;
	}
	return true;
}

void Parser::_parseListen(ServerConfig& server) {
	_expect(TokenType::WORD, "expected address after 'listen'");

	const Token& addressToken = _current();
	_advance();
	
	_expect(TokenType::SEMICOLON, "expected ';' after listening address");
	_advance();

	const std::string& address = addressToken.lexeme;
	const std::size_t colonPos = address.find(':');

	if (colonPos == std::string::npos)
		error(addressToken, "listen must be in '<host>:<port>' format");

	const std::string host = address.substr(0, colonPos);
	const std::string portStr = address.substr(colonPos + 1);

	if (host.empty())
		error(addressToken, "host cannot be empty");
	if (!isDigits(portStr))
		error(addressToken, "port must be numeric");

	unsigned long port;

	try {
		port = std::stoul(portStr);
	} catch (const std::exception&) {
		error(addressToken, "invalid port number");
	}

	if (port == 0 || port > 65535)
		error(addressToken, "port out of range");

	ListenDirective listen;

	listen.host = host;
	listen.port = static_cast<std::uint16_t>(port);

	server.listen.push_back(listen);
}

static size_t parseSizeValue(const Token& token) {
	if (!isDigits(token.lexeme))
		error(token, "size must be a positive integer");

	try {
		return std::stoull(token.lexeme);
	} catch (const std::exception&) {
		error(token, "invalid size value");
	}
}

void Parser::_parseClientMaxBodySize(ServerConfig& server) {
	_expect(TokenType::WORD, "expected size value after 'client_max_body_size'");

	const Token& sizeToken = _current();
	_advance();

	_expect(TokenType::SEMICOLON, "expected ';' after client_max_body_size value");
	_advance();

	const size_t size = parseSizeValue(sizeToken);

	if (size == 0 || size > 1024 * 1024 * 10)
			error(sizeToken, "size exceeds limits (0 < size <= 10 MiB)"); // FIXME: 10 MiB as upper client body limit?

	server.clientMaxBodySize = size;
}

LocationConfig Parser::_parseLocation(void) {
	_expect(TokenType::WORD, "expected path after 'location'");

	LocationConfig location;
	location.path = _current().lexeme;
	_advance();

	_expect(TokenType::BRACE_LEFT, "expected '{' after location path");
	_advance();

	while (!_check(TokenType::BRACE_RIGHT)) {
		if (_atEnd())
			error(_current(), "expected '}' after location block");

		_parseLocationDirective(location);
	}
	_advance();

	return location;
}

void Parser::_parseLocationDirective(LocationConfig& location) {
	_expect(TokenType::WORD, "expected location directive");

	const Token& directive = _current();

	if (directive.lexeme == "root") {
		_advance();
		_parseRoot(location);
	} else if (directive.lexeme == "autoindex") {
		_advance();
		_parseAutoIndex(location);
	}
	else
		error(directive, "unknown location directive '" + directive.lexeme + "'");
}

void Parser::_parseRoot(LocationConfig& location) {
	_expect(TokenType::WORD, "expected path after 'root'");
	location.root = _current().lexeme;
	_advance();

	_expect(TokenType::SEMICOLON, "expected ';' after root path");
	_advance();
}

void Parser::_parseAutoIndex(LocationConfig& location) {
	_expect(TokenType::WORD, "expected 'on' or 'off'");
	const std::string& setting = _current().lexeme;

	if (setting == "on")
		location.autoIndex = true;
	else if (setting == "off")
		location.autoIndex = false;
	else
		error(_current(), "unknown autoindex setting, must be 'on' or 'off'");

	_advance();
	_expect(TokenType::SEMICOLON, "expected ';' after autoindex setting");
	_advance();
}
