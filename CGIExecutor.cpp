/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 13:00:52 by skomyshe          #+#    #+#             */
/*   Updated: 2026/06/15 14:00:20 by skomyshe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIExecutor.hpp"

CGIEnv &CGIExecutor::getEnv()
{
    return _env;
}

std::string CGIExecutor::execute(const std::string &scriptPath)
{
    int pipefd[2];

    if (pipe(pipefd) == -1)
        throw std::runtime_error("pipe failed");

    pid_t pid = fork();

    if (pid < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        throw std::runtime_error("fork failed");
    }

    if (pid == 0)
    {
        if(dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }

        close(pipefd[0]);
        close(pipefd[1]);
        
        char **envp = _env.buildEnvp();

        char *argv[] =
            {
                const_cast<char *>(scriptPath.c_str()),
                NULL};

        // char *argv[] =
        //     {
        //         &scriptPath[0],
        //         NULL};

        // std::vector<char *> argv;
        // argv.push_back(const_cast<char *>(scriptPath.c_str()));
        // argv.push_back(NULL);

        execve(argv[0], argv, envp);

        CGIEnv::freeEnvp(envp);
                
        perror("execve");
        _exit(EXIT_FAILURE);
    }

    close(pipefd[1]);

    std::string output;

    char buffer[1024];
    ssize_t bytes;

    while ((bytes = read(pipefd[0], buffer, sizeof(buffer))) > 0)
    {
        output.append(buffer, bytes);
    }

    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
    {
        int exitCode = WEXITSTATUS(status);

        if (exitCode != 0)
            throw std::runtime_error("CGI exited with error");
    }
    else if (WIFSIGNALED(status))
    {
        throw std::runtime_error("CGI terminated by signal");
    }

    return output;
}
