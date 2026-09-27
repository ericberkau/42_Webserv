/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StringUtils.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 15:38:37 by djanardh          #+#    #+#             */
/*   Updated: 2026/09/27 19:19:28 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP

#include <string>
#include <cctype> // for std::tolower
#include "config.hpp"

std::string	ft_toLower(const std::string& s);
std::string	methodToString(HttpMethod method);

#endif