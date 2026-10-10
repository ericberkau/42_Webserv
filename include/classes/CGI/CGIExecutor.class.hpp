/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.class.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 12:30:19 by skomyshe          #+#    #+#             */
/*   Updated: 2026/10/10 02:36:34 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIEXECUTOR_HTTP
#define CGIEXECUTOR_HTTP

#include "CGIEnv.class.hpp"
#include "networking.hpp"
#include <ctime>
#include <iostream>
#include <map>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

class CGIExecutor
{
	public:

		CGIEnv&		getEnv();
		CgiContext	start(const std::string& scriptPath, const std::string& handlerPath);


	private:

		void	_handleChild(int* stdinPipeFd, int* stdoutPipeFd, const std::string& scriptPath, const std::string& handlerPath);

		CGIEnv	_env;

};

#endif