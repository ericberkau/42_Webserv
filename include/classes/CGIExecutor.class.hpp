/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.class.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 12:30:19 by skomyshe          #+#    #+#             */
/*   Updated: 2026/10/02 22:36:47 by skomyshe         ###   ########.fr       */
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

		CgiContext	start(const std::string& scriptPath);


	private:

		void	_handleChild(int* stdinPipeFd, int* stdoutPipeFd, const std::string& scriptPath);

		CGIEnv	_env;

};

#endif