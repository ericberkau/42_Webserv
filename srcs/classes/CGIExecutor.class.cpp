/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.class.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 13:00:52 by skomyshe          #+#    #+#             */
/*   Updated: 2026/10/06 20:16:09 by skomyshe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/CGIExecutor.class.hpp"
#include <fcntl.h>

CGIEnv& CGIExecutor::getEnv()
{
	return _env;
}

CgiContext CGIExecutor::start(const std::string& scriptPath)
{
	int stdinPipeFd[2];
	int stdoutPipeFd[2];

	if (pipe(stdinPipeFd) == -1)
		throw std::runtime_error("pipe failed");

	if (pipe(stdoutPipeFd) == -1)
		throw std::runtime_error("pipe failed");

	pid_t pid = fork();

	if (pid < 0)
	{
		close(stdinPipeFd[0]);
		close(stdinPipeFd[1]);

		close(stdoutPipeFd[0]);
		close(stdoutPipeFd[1]);
		throw std::runtime_error("fork failed");
	}

	if (pid == 0)
		_handleChild(stdinPipeFd, stdoutPipeFd, scriptPath);

	close(stdinPipeFd[0]);
	close(stdoutPipeFd[1]);

	CgiContext cgiInfo;
	cgiInfo.pid = pid;
	cgiInfo.readPipeFd = stdoutPipeFd[0];
	cgiInfo.writePipeFd = stdinPipeFd[1];

	return cgiInfo;
}

void CGIExecutor::_handleChild(int* stdinPipeFd, int* stdoutPipeFd, const std::string& scriptPath)
{
	close(stdinPipeFd[1]);
	close(stdoutPipeFd[0]);

	if (dup2(stdinPipeFd[0], STDIN_FILENO) == -1)
	{
		close(stdinPipeFd[0]);
		close(stdoutPipeFd[1]);
		perror("dup2");
		_exit(EXIT_FAILURE);
	}
	close(stdinPipeFd[0]);

	if (dup2(stdoutPipeFd[1], STDOUT_FILENO) == -1)
	{
		close(stdoutPipeFd[1]);
		perror("dup2");
		_exit(EXIT_FAILURE);
	}
	close(stdoutPipeFd[1]);

	char** envp = _env.buildEnvp();

	char* argv[] = {const_cast<char*>(scriptPath.c_str()), NULL};

	execve(argv[0], argv, envp);

	CGIEnv::freeEnvp(envp);

	perror("execve");
	_exit(EXIT_FAILURE);
}
