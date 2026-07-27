/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Lexer.class.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 23:11:41 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/27 23:32:46 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../token.hpp"
#include <string>
#include <vector>

class Lexer {

	public:

		explicit Lexer(const std::string& input);
		~Lexer(void) = default;

		std::vector<Token>	tokenize(void);


	private:

		bool	_eof(void) const;
		char	_current(void) const;
		char	_peek(void) const;
		void	_advance(void);

		void	_skipWhitespace(void);
		void	_skipComment(void);
		
		Token	_readWord(void);

		std::string	_source;
		size_t		_current;
		size_t		_line;
		size_t		_column;
	
		Lexer(const Lexer&) = delete;
		Lexer&	operator=(const Lexer&) = delete;
		Lexer(Lexer&&) = delete;
		Lexer&	operator=(Lexer&&) = delete;
	
};
