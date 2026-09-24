/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIEnv.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 12:31:01 by skomyshe          #+#    #+#             */
/*   Updated: 2026/09/23 16:09:58 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/CGIExecutor.class.hpp"
#include "classes/CGIEnv.class.hpp"
#include <cstring>

void CGIEnv::set(const std::string &key,
                 const std::string &value)
{
    _env[key] = value;
}

char **CGIEnv::buildEnvp() const
{
    char **envp = new char *[_env.size() + 1];

    size_t i = 0;

    for (std::map<std::string, std::string>::const_iterator it = _env.begin();
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

void CGIEnv::freeEnvp(char **envp)
{
    if (!envp)
        return;

    for (size_t i = 0; envp[i]; ++i)
        delete[] envp[i];

    delete[] envp;
}