/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 12:30:19 by skomyshe          #+#    #+#             */
/*   Updated: 2026/06/04 13:45:52 by skomyshe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIEXECUTOR_HTTP
#define CGIEXECUTOR_HTTP

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

class CGIExecutor
{
public:
    std::string execute(const std::string& scriptPath);
};

// class CGIExecutor
// {
// private:
//     std::map<std::string, std::string> env;

//     char **buildEnvp();

// public:
//     std::string execute(
//         const Request &reqiest,
//         const LocationConfig &location);
// };

#endif