/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StringUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 15:40:12 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/10 03:01:22 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/HTTP/StringUtils.hpp"

std::string ft_toLower(const std::string& s)
{
	std::string result = s;
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
	return result;
}

std::string methodToString(HttpMethod method)
{
	switch (method)
	{
		case HttpMethod::GET:
			return "GET";

		case HttpMethod::POST:
			return "POST";

		case HttpMethod::DELETE:
			return "DELETE";

		case HttpMethod::INVALID:
			return "INVALID";
	}

	return "UNKNOWN";
}