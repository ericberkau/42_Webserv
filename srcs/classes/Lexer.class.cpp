/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.class.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 16:48:53 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/28 18:05:10 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Lexer.class.hpp"

Lexer::Lexer(const std::string& source)
	: _source(source), _currentPos(0), _line(1), _column(1)
{}

std::vector<Token> Lexer::tokenize(void) {
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

void Lexer::_advance(void) {
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

char Lexer::_current(void) const {
	return _source[_currentPos];
}

bool Lexer::_eof(void) const {
	return _currentPos >= _source.size();
}

char Lexer::_peek(void) const {
	if (_currentPos + 1 >= _source.size())
		return '\0';

	return _source[_currentPos + 1];
}

void Lexer::_skipComment(void) {
	while (!_eof() && _current() != '\n')
		_advance();
}

static bool isWhitespace(char c) {
	return std::isspace(static_cast<unsigned char>(c));
}

void Lexer::_skipWhitespace(void) {
	while (!_eof() && isWhitespace(_current()))
		_advance();
}

static bool isDelimiter(char c) {
	return isWhitespace(c)
		|| c == '{'
		|| c == '}'
		|| c == ';'
		|| c == '#';
}

Token Lexer::_readWord(void) {
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
