/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parserUtils.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 23:13:20 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/07 16:02:21 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "token.hpp"
#include <string>

[[noreturn]] void	error(const Token& token, const std::string& msg);
bool				containsOnlyDigits(const std::string& str);
size_t				parseSizeValue(const Token& token);
int 				parseStatusCode(const Token& codeToken, int minCode, int maxCode);
