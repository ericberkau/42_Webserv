/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.class.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 13:00:52 by skomyshe          #+#    #+#             */
/*   Updated: 2026/09/25 01:23:05 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/CGIExecutor.class.hpp"

CGIEnv &CGIExecutor::getEnv()
{
	return _env;
}

std::string CGIExecutor::execute(const std::string &scriptPath, const std::string &body)
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

	if (write(stdinPipeFd[1], body.c_str(), body.size()) == -1)
	{
		close(stdinPipeFd[1]);
		close(stdoutPipeFd[0]);
		throw std::runtime_error("write failed");
	}

	close(stdinPipeFd[1]);

	std::string output;
	char buffer[1024];

	struct pollfd pfd;
	pfd.fd = stdoutPipeFd[0];
	pfd.events = POLLIN;

	int status = 0;
	bool childExited = false;

	time_t start = time(NULL);

	while (true)
	{
		if (time(NULL) - start >= 5)
		{
			kill(pid, SIGKILL);
			waitpid(pid, &status, 0);

			close(stdoutPipeFd[0]);

			std::cerr << "Captured before timeout:\n";
			std::cerr << output << std::endl;

			throw std::runtime_error("CGI timeout");
		}
		int ret = poll(&pfd, 1, 100);

		if (ret == -1)
		{
			close(stdoutPipeFd[0]);
			throw std::runtime_error("poll failed");
		}

		if (ret > 0)
		{
			if (pfd.revents & POLLERR)
			{
				close(stdoutPipeFd[0]);
				throw std::runtime_error("poll error");
			}

			if (pfd.revents & (POLLIN | POLLHUP))
			{

				ssize_t bytes = read(stdoutPipeFd[0], buffer, sizeof(buffer));

				if (bytes > 0)
					output.append(buffer, bytes);

				if (pfd.revents & POLLHUP)
					break;
			}
		}

		if (!childExited)
		{
			pid_t result = waitpid(pid, &status, WNOHANG);

			if (result == pid)
				childExited = true;
			else if (result == -1)
			{
				close(stdoutPipeFd[0]);
				throw std::runtime_error("waitpid failed");
			}
		}
	}

	close(stdoutPipeFd[0]);

	if (!childExited)
	{
		if (waitpid(pid, &status, 0) == -1)
			throw std::runtime_error("waitpid failed");
	}

	if (WIFEXITED(status))
	{
		if (WEXITSTATUS(status) != 0)
			throw std::runtime_error("CGI exited with error");
	}
	else if (WIFSIGNALED(status))
	{
		throw std::runtime_error("CGI terminated by signal");
	}

	return output;
}

void CGIExecutor::_handleChild(int* stdinPipeFd, int* stdoutPipeFd, const std::string& scriptPath)
{
	if (dup2(stdinPipeFd[0], STDIN_FILENO) == -1)
	{
		perror("dup2");
		_exit(EXIT_FAILURE);
	}

	if (dup2(stdoutPipeFd[1], STDOUT_FILENO) == -1)
	{
		perror("dup2");
		_exit(EXIT_FAILURE);
	}

	close(stdinPipeFd[0]);
	close(stdinPipeFd[1]);

	close(stdoutPipeFd[0]);
	close(stdoutPipeFd[1]);

	char **envp = _env.buildEnvp();

	char *argv[] = {const_cast<char *>(scriptPath.c_str()), NULL};

	execve(argv[0], argv, envp);

	CGIEnv::freeEnvp(envp);

	perror("execve");
	_exit(EXIT_FAILURE);
}
