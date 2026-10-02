/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIExecutor.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 13:00:52 by skomyshe          #+#    #+#             */
/*   Updated: 2026/09/27 23:02:31 by skomyshe         ###   ########.fr       */
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
    {
        close(stdinpipefd[0]);
        close(stdinpipefd[1]);
        throw std::runtime_error("pipe failed");
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        close(stdinpipefd[0]);
        close(stdinpipefd[1]);

        close(stdoutpipefd[0]);
        close(stdoutpipefd[1]);
        throw std::runtime_error("fork failed");
    }

    int status = 0;

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
                const_cast<char *>(scriptPath.c_str()),
                NULL};

        execve(argv[0], argv, envp);

        CGIEnv::freeEnvp(envp);

        perror("execve");
        _exit(EXIT_FAILURE);
    }

    close(stdinpipefd[0]);
    close(stdoutpipefd[1]);

    if (write(stdinpipefd[1], body.c_str(), body.size()) == -1)
    {
        close(stdinpipefd[1]);
        close(stdoutpipefd[0]);

        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);

        throw std::runtime_error("write failed");
    }

    close(stdinpipefd[1]);

    std::string output;
    char buffer[1024];

    struct pollfd pfd;
    pfd.fd = stdoutpipefd[0];
    pfd.events = POLLIN;

    bool childExited = false;

    time_t start = time(NULL);

    while (true)
    {
        if (time(NULL) - start >= 5)
        {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);

            close(stdoutpipefd[0]);

            std::cerr << "Captured before timeout:\n";
            std::cerr << output << std::endl;

            throw std::runtime_error("CGI timeout");
        }
        int ret = poll(&pfd, 1, 100);

        if (ret == -1)
        {
            close(stdoutpipefd[0]);

            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);

            throw std::runtime_error("poll failed");
        }

        if (ret > 0)
        {
            if (pfd.revents & POLLERR)
            {
                close(stdoutpipefd[0]);

                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);

                throw std::runtime_error("poll error");
            }

            if (pfd.revents & (POLLIN | POLLHUP))
            {

                // ssize_t bytes = read(stdoutpipefd[0], buffer, sizeof(buffer));

                // if (bytes > 0)
                //     output.append(buffer, bytes);

                // if (pfd.revents & POLLHUP)
                //     break;
                ssize_t bytes = read(stdoutpipefd[0], buffer, sizeof(buffer));

                if (bytes > 0)
                {
                    output.append(buffer, bytes);
                }
                else if (bytes == 0)
                {
                    break;
                }
                else
                {
                    close(stdoutpipefd[0]);

                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0);

                    throw std::runtime_error("read failed");
                }
            }
        }

        if (!childExited)
        {
            pid_t result = waitpid(pid, &status, WNOHANG);

            if (result == pid)
                childExited = true;
            else if (result == -1)
            {
                close(stdoutpipefd[0]);
                throw std::runtime_error("waitpid failed");
            }
        }
    }

    close(stdoutpipefd[0]);

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
