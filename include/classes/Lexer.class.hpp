/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.class.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 23:11:41 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/28 17:12:01 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../token.hpp"
#include <string>
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

		void	_skipComment(void);
		void	_skipWhitespace(void);

		Token	_readWord(void);

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
