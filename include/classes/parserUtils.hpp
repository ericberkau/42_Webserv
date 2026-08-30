/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parserUtils.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 23:13:20 by eberkau           #+#    #+#             */
/*   Updated: 2026/08/27 23:19:27 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

[[noreturn]] void	error(const Token& token, const std::string& msg);
bool				containsOnlyDigits(const std::string& str);
int 				parseStatusCode(const Token& codeToken, int minCode, int maxCode);
