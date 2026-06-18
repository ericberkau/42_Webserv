/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 12:30:19 by skomyshe          #+#    #+#             */
/*   Updated: 2026/06/15 15:44:08 by skomyshe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIEXECUTOR_HTTP
#define CGIEXECUTOR_HTTP

#include "CGIEnv.hpp"
#include <iostream>
#include <string>
#include <map>
#include <unistd.h>
#include <sys/wait.h>

class CGIExecutor
{
private:
    CGIEnv _env;

public:
    CGIEnv &getEnv();
    std::string execute(
        const std::string &scriptPath,
        const std::string &body);
};

// class CGIExecutor
// {
// private:
//     std::map<std::string, std::string> env;

//     char **buildEnvp();

// public:
//     std::string execute(
//         const std::string &key,
//         const std::string &value);
// };

#endif