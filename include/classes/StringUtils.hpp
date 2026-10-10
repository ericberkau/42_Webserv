/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StringUtils.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 15:38:37 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/10 02:31:11 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP

#include "config.hpp"
#include <cctype> // for std::tolower
#include <string>

std::string	ft_toLower(const std::string& s);
std::string	methodToString(HttpMethod method);

#endif