/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 13:00:52 by skomyshe          #+#    #+#             */
/*   Updated: 2026/06/22 16:57:22 by skomyshe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIExecutor.hpp"

CGIEnv &CGIExecutor::getEnv()
{
    return _env;
}

std::string CGIExecutor::execute(const std::string &scriptPath, const std::string &body)
{
    int stdinpipefd[2];
    int stdoutpipefd[2];

    if (pipe(stdinpipefd) == -1)
        throw std::runtime_error("pipe failed");

    if (pipe(stdoutpipefd) == -1)
        throw std::runtime_error("pipe failed");

    pid_t pid = fork();

    if (pid < 0)
    {
        close(stdinpipefd[0]);
        close(stdinpipefd[1]);

        close(stdoutpipefd[0]);
        close(stdoutpipefd[1]);
        throw std::runtime_error("fork failed");
    }

    if (pid == 0)
    {
        if (dup2(stdinpipefd[0], STDIN_FILENO) == -1)
        {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }

        if (dup2(stdoutpipefd[1], STDOUT_FILENO) == -1)
        {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }

        close(stdinpipefd[0]);
        close(stdinpipefd[1]);

        close(stdoutpipefd[0]);
        close(stdoutpipefd[1]);

        char **envp = _env.buildEnvp();

        char *argv[] =
            {
                const_cast<char *>(scriptPath.data()),
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

    close(stdinpipefd[0]);
    close(stdoutpipefd[1]);

    write(
        stdinpipefd[1],
        body.c_str(),
        body.size());

    if (!body.empty())
    {
        write(stdinpipefd[1], body.data(), body.size());
    }

    close(stdinpipefd[1]);

    std::string output;

    char buffer[1024];
    ssize_t bytes;

    while ((bytes = read(stdoutpipefd[0], buffer, sizeof(buffer))) > 0)
    {
        output.append(buffer, bytes);
    }

    close(stdoutpipefd[0]);

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
