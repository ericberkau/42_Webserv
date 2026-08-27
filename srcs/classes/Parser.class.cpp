/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/29 16:35:46 by eberkau           #+#    #+#             */
/*   Updated: 2026/08/27 22:35:15 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Parser.class.hpp"
#include <sstream>

Parser::Parser(const std::vector<Token>& tokens) : _tokens(tokens), _currentPos(0) {}

Config Parser::parse(void) {
	return _parseConfig();
}


/* ------- private member functions ------- */

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

bool Parser::_check(TokenType type) const {
	return _current().type == type;
}

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

void Parser::_expect(const std::string& keyword, const std::string& errMsg) {
	if (!_check(TokenType::WORD) || _current().lexeme != keyword)
		error(_current(), errMsg);
}


/* ------- actual parsing functions ------- */
// TODO: check where duplicates are still accepted and can overwrite each other (work-in-progress)

Config Parser::_parseConfig(void) {
	Config config;

	while (!_atEnd())
		config.servers.push_back(_parseServer());

	// TODO: add config verification?
	return config;
}

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

static bool isDuplicate(
	const std::vector<ListenDirective>& existing,
	const ListenDirective& newListen)
{
	for (const ListenDirective& entry : existing) {
		if (entry.host == newListen.host && entry.port == newListen.port)
			return true;
	}
	return false;
}

static bool containsOnlyDigits(const std::string& str) {
	if (str.empty())
		return false;

	for (char c : str) {
		if (!std::isdigit(static_cast<unsigned char>(c)))
			return false;
	}
	return true;
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

static size_t parseSizeValue(const Token& token) {
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

LocationConfig Parser::_parseLocation(void) {
	_advance();

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

	if (directive.lexeme == "root")
		_parsePathSetting(location.root);
	else if (directive.lexeme == "index")
		_parsePathSetting(location.index);
	else if (directive.lexeme == "autoindex")
		_parseBoolSetting(location.autoIndex);
	else if (directive.lexeme == "allow_methods")
		_parseAllowMethods(location);
	else if (directive.lexeme == "return")
		_parseReturn(location);
	else if (directive.lexeme == "upload_enable")
		_parseBoolSetting(location.uploadEnabled);
	else if (directive.lexeme == "upload_path")
		_parsePathSetting(location.uploadPath);
	else if (directive.lexeme == "cgi")
		_parseCgi(location);
	else
		error(directive, "unknown location directive '" + directive.lexeme + "'");
}

static HttpMethod parseHttpMethod(const Token& token) {
	if (token.lexeme == "GET")
		return HttpMethod::GET;
	else if (token.lexeme == "POST")
		return HttpMethod::POST;
	else if (token.lexeme == "DELETE")
		return HttpMethod::DELETE;
	else
		error(token, "unknown HTTP method '" + token.lexeme + "'");
}

void Parser::_parseAllowMethods(LocationConfig& location) {
		location.allowedMethods.clear(); // override constructor default
	_advance();

	if (_check(TokenType::SEMICOLON))
		error(_current(), "'allow_methods' requires at least one method");

	while (!_check(TokenType::SEMICOLON)) {
		_expect(TokenType::WORD, "expected allowed HTTP method");

		location.allowedMethods.push_back(parseHttpMethod(_current()));
		_advance();
	}
	_advance();
}

static int parseStatusCode(const Token& codeToken, int minCode, int maxCode) {
	if (!containsOnlyDigits(codeToken.lexeme))
		error(codeToken, "expected numeric redirect status code");

	int code;

	try {
		code = std::stoi(codeToken.lexeme);
	} catch (...) {
		error(codeToken, "invalid status code");
	}

	if (code < minCode || code > maxCode)
		error(codeToken, "status code must be between " + std::to_string(minCode) + " and " + std::to_string(maxCode));

	return code;
}

void Parser::_parseReturn(LocationConfig& location) {
	if (location.hasRedirect)
		error(_current(), "duplicate return directive");
	_advance();

	_expect(TokenType::WORD, "expected redirect status code");
	location.redirectStatusCode = parseStatusCode(_current(), 300, 399);
	location.hasRedirect = true;
	_advance();

	_expect(TokenType::WORD, "expected redirect target");
	location.redirectTarget = _current().lexeme;
	_advance();

	_expect(TokenType::SEMICOLON, "expected ';' after redirect target");
	_advance();
}

void Parser::_parseBoolSetting(bool& configOption) {
	const Token& directive = _current();
	_advance();

	_expect(TokenType::WORD, "expected 'on' or 'off' after '" + directive.lexeme + "'");
	const std::string& setting = _current().lexeme;

	if (setting == "on")
		configOption = true;
	else if (setting == "off")
		configOption = false;
	else
		error(_current(), "unknown " + directive.lexeme + " setting, must be 'on' or 'off'");

	_advance();

	_expect(TokenType::SEMICOLON, "expected ';' after " + directive.lexeme + " setting");
	_advance();
}

void Parser::_parsePathSetting(std::string& configOption) {
	const Token& directive = _current();
	_advance();

	_expect(TokenType::WORD, "expected path after '" + directive.lexeme + "'");
	configOption = _current().lexeme;
	_advance();

	_expect(TokenType::SEMICOLON, "expected ';' after " + directive.lexeme + " path");
	_advance();
}

void Parser::_parseCgi(LocationConfig& location) {
	_advance();

	_expect(TokenType::WORD, "expected file extension after 'cgi'");
	const std::string& extension = _current().lexeme;

	if (extension.size() < 2 || extension[0] != '.')
		error(_current(), "invalid file extension: " + extension);
	if (location.cgiHandlers.count(extension))
		error(_current(), "duplicate 'cgi' directive for file extension '" + extension + "'");

	_advance();

	_expect(TokenType::WORD, "expected CGI executable path after file extension");
	const std::string& executable = _current().lexeme;
	_advance();

	_expect(TokenType::SEMICOLON, "expected ';' after CGI executable path");
	_advance();

	location.cgiHandlers[extension] = executable;
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
