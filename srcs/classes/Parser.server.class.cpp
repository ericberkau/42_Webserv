/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.server.class.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 22:50:20 by eberkau           #+#    #+#             */
/*   Updated: 2026/08/27 23:52:39 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Parser.class.hpp"
#include "classes/parserUtils.hpp"

ServerConfig Parser::_parseServer(void) {
	_expect("server", "expected 'server'"); // FIXME: might change, if _parseConfig() gets an if-elseif chain to account for 'events' as well
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

	if (directive.lexeme == "listen")
		_parseListen(server);
	else if (directive.lexeme == "client_max_body_size")
		_parseClientMaxBodySize(server);
	else if (directive.lexeme == "location")
		server.locations.push_back(_parseLocation());
	else if (directive.lexeme == "error_page")
		_parseErrorPage(server);
	else
		error(directive, "unknown server directive '" + directive.lexeme + "'");
}

static
bool isDuplicate(
	const std::vector<ListenDirective>& existing,
	const ListenDirective& newListen)
{
	for (const ListenDirective& entry : existing) {
		if (entry.host == newListen.host && entry.port == newListen.port)
			return true;
	}
	return false;
}

void Parser::_parseListen(ServerConfig& server) {
	_advance();

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
	if (!containsOnlyDigits(portStr))
		error(addressToken, "port must be numeric");

	unsigned long port;

	try {
		port = std::stoul(portStr);
	} catch (...) {
		error(addressToken, "invalid port number");
	}

	if (port == 0 || port > 65535)
		error(addressToken, "port out of range");

	ListenDirective newListen;

	newListen.host = host;
	newListen.port = static_cast<std::uint16_t>(port);

	if (isDuplicate(server.listen, newListen))
		error(addressToken, "duplicate listen address: " + addressToken.lexeme);

	server.listen.push_back(newListen);
}

static
size_t parseSizeValue(const Token& token) {
	if (!containsOnlyDigits(token.lexeme))
		error(token, "max body size must be a positive integer");

	try {
		return std::stoull(token.lexeme);
	} catch (...) {
		error(token, "invalid body size value: " + token.lexeme);
	}
}

void Parser::_parseClientMaxBodySize(ServerConfig& server) {
		_advance();

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

void Parser::_parseErrorPage(ServerConfig& server) {
	_advance();

	_expect(TokenType::WORD, "expected HTTP status code after 'error_page'");
	const Token& codeToken = _current();
	const int code = parseStatusCode(codeToken, 100, 599);

	if (server.errorPages.count(code))
		error(codeToken, "duplicate error page definition for status " + codeToken.lexeme);

	_advance();

	_expect(TokenType::WORD, "expected error page path");
	const std::string& path = _current().lexeme;
	_advance();

	_expect(TokenType::SEMICOLON, "expected ';' after error page path");
	_advance();

	server.errorPages[code] = path;
}
