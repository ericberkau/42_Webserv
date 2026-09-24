/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.class.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 12:30:19 by skomyshe          #+#    #+#             */
/*   Updated: 2026/09/24 18:50:04 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIEXECUTOR_HTTP
#define CGIEXECUTOR_HTTP

#include "CGIEnv.class.hpp"
#include <iostream>
#include <string>
#include <map>
#include <unistd.h>
#include <sys/wait.h>
#include <ctime>
#include <signal.h>
#include <poll.h>

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

#endif