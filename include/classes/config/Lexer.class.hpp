/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.class.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 23:11:41 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/30 00:25:21 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "token.hpp"
#include <vector>

class Lexer {

	public:

		explicit Lexer(const std::string& source);
		~Lexer(void) = default;

		std::vector<Token>	tokenize(void);


	private:

		void	_advance(void);
		char	_current(void) const;
		bool	_eof(void) const;
		char	_peek(void) const;
		Token	_readWord(void);
		void	_skipComment(void);
		void	_skipWhitespace(void);

		std::string	_source;
		size_t		_currentPos;
		size_t		_line;
		size_t		_column;

		/* disallow copying or moving */
		Lexer(const Lexer&) = delete;
		Lexer&	operator=(const Lexer&) = delete;
		Lexer(Lexer&&) = delete;
		Lexer&	operator=(Lexer&&) = delete;

};
