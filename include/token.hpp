/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   token.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/27 23:10:32 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/27 23:15:43 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>

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
	size_t		column;
};
