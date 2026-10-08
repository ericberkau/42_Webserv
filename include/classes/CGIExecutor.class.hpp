/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.class.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 12:30:19 by skomyshe          #+#    #+#             */
/*   Updated: 2026/10/08 07:00:27 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIEXECUTOR_HTTP
#define CGIEXECUTOR_HTTP

#include "CGIEnv.class.hpp"
#include "networking.hpp"
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
	public:

		CGIEnv&		getEnv();

		CgiContext	start(const std::string& scriptPath, const std::string& handlerPath);


	private:

		void	_handleChild(int* stdinPipeFd, int* stdoutPipeFd, const std::string& scriptPath, const std::string& handlerPath);

		CGIEnv	_env;

};

#endif