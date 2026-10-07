/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.location.class.cpp                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 22:39:18 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/07 18:57:46 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Parser.class.hpp"

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
	else if (directive.lexeme == "client_max_body_size")
		_parseClientMaxBodySize(location);
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

void Parser::_parsePathSetting(std::string& configOption) {
	const Token& directive = _current();
	_advance();

	_expect(TokenType::WORD, "expected path after '" + directive.lexeme + "'");
	configOption = _current().lexeme;
	_advance();

	_expect(TokenType::SEMICOLON, "expected ';' after " + directive.lexeme + " path");
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

static
HttpMethod parseHttpMethod(const Token& token) {
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

		location.allowedMethods.insert(parseHttpMethod(_current()));
		_advance();
	}
	_advance();
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
