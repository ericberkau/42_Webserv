/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parserUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 23:12:53 by eberkau           #+#    #+#             */
/*   Updated: 2026/08/27 23:16:24 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "token.hpp"
#include <sstream>

[[noreturn]]
void error(const Token& token, const std::string& msg) {
	std::ostringstream oss;
	oss << "Parse error at "
		<< token.line << ':' << token.column
		<< ": " << msg;

	throw std::runtime_error(oss.str());
}

bool containsOnlyDigits(const std::string& str) {
	if (str.empty())
		return false;

	for (char c : str) {
		if (!std::isdigit(static_cast<unsigned char>(c)))
			return false;
	}
	return true;
}

int parseStatusCode(const Token& codeToken, int minCode, int maxCode) {
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
