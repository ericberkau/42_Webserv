/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/08 18:11:25 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/23 20:31:15 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <list>
#include <string>
#include <vector>

struct s_config {
	std::list<std::string>	portsToListen;
	size_t					clientReqMaxSize;
	std::list<std::string>	acceptedHttpsMethods;
	std::string				rootedDir;
	bool					dirListing;
	bool					uploadAllowed;
	// defaultErrorPages;
};

enum class TokenType {
	WORD,
	BRACE_LEFT,
	BRACE_RIGHT,
	SEMICOLON,
	END
};

struct Token {
	TokenType	type;
	std::string	lexeme;
	size_t		line;
	size_t		col;
};

class Lexer {

	public:

		explicit Lexer(const std::string& input);
		~Lexer(void);

		std::vector<Token>	tokenize(void);
		
		
	private:

		Lexer(const Lexer&) = delete;
		Lexer&	operator=(const Lexer&) = delete;
		Lexer(Lexer&&) = delete;
		Lexer&	operator=(Lexer&&) = delete;

		
		
};

using t_config = s_config;
