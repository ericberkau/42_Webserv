/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIEnv.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 12:31:01 by skomyshe          #+#    #+#             */
/*   Updated: 2026/10/10 02:31:39 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/CGIEnv.class.hpp"
#include "classes/CGIExecutor.class.hpp"
#include <cstring>

void CGIEnv::set(const std::string& key,
				const std::string& value)
{
	_env[key] = value;
}

char** CGIEnv::buildEnvp() const
{
	char** envp = new char*[_env.size() + 1];

	size_t i = 0;

	for (auto it = _env.begin();
		it != _env.end();
		++it)
	{
		std::string entry = it->first + "=" + it->second;

		envp[i] = new char[entry.size() + 1];
		std::strcpy(envp[i], entry.data());

		++i;
	}

	envp[i] = NULL;

	return envp;
}

void CGIEnv::freeEnvp(char** envp)
{
	if (!envp)
		return;

	for (size_t i = 0; envp[i]; ++i)
		delete[] envp[i];

	delete[] envp;
}