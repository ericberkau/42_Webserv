/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigLexer.class.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 16:48:53 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/10 02:59:07 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/config/ConfigLexer.class.hpp"

ConfigLexer::ConfigLexer(const std::string& source)
	: _source(source), _currentPos(0), _line(1), _column(1)
{}

std::vector<Token> ConfigLexer::tokenize(void) {
	std::vector<Token> tokens;

	while (!_eof()) {
		_skipWhitespace();

		if (_eof())
			break;

		if (_current() == '#') {
			_skipComment();
			continue;
		}

		const size_t currLine = _line;
		const size_t currColumn = _column;

		if (_current() == '{') {
			tokens.push_back(Token{TokenType::BRACE_LEFT, "{", currLine, currColumn});
			_advance();
		}
		else if (_current() == '}') {
			tokens.push_back(Token{TokenType::BRACE_RIGHT, "}", currLine, currColumn});
			_advance();
		}
		else if (_current() == ';') {
			tokens.push_back(Token{TokenType::SEMICOLON, ";", currLine, currColumn});
			_advance();
		}
		else
			tokens.push_back(_readWord());
	}
	tokens.push_back(Token{TokenType::END, "", _line, _column});

	return tokens;
}


/* private member functions */

void ConfigLexer::_advance(void) {
	if (_eof())
		return;

	if (_current() == '\n') {
		++_line;
		_column = 1;
	}
	else
		++_column;

	++_currentPos;
}

char ConfigLexer::_current(void) const {
	return _source[_currentPos];
}

bool ConfigLexer::_eof(void) const {
	return _currentPos >= _source.size();
}

char ConfigLexer::_peek(void) const {
	if (_currentPos + 1 >= _source.size())
		return '\0';

	return _source[_currentPos + 1];
}

static bool isWhitespace(char c) {
	return std::isspace(static_cast<unsigned char>(c));
}

static bool isDelimiter(char c) {
	return isWhitespace(c)
		|| c == '{'
		|| c == '}'
		|| c == ';'
		|| c == '#';
}

Token ConfigLexer::_readWord(void) {
	const size_t startPos = _currentPos;
	const size_t startLine = _line;
	const size_t startCol = _column;

	while (!_eof() && !isDelimiter(_current()))
		_advance();

	return Token{
		TokenType::WORD,
		_source.substr(startPos, _currentPos - startPos),
		startLine,
		startCol
	};
}

void ConfigLexer::_skipComment(void) {
	while (!_eof() && _current() != '\n')
		_advance();
}

void ConfigLexer::_skipWhitespace(void) {
	while (!_eof() && isWhitespace(_current()))
		_advance();
}
