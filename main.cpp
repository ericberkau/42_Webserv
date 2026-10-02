/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/20 18:39:09 by skomyshe          #+#    #+#             */
/*   Updated: 2026/09/27 21:48:55 by skomyshe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIExecutor.hpp"

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <csignal>

volatile sig_atomic_t g_stop = 0;

void handleSigint(int)
{
    g_stop = 1;
    // printf("hello");
}
int main()
{
    signal(SIGINT, handleSigint);

    CGIExecutor cgi;

    try
    {
        std::string result = cgi.execute("./slow.py", "");
        std::cout << result << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

// int main()
// {
//     pid_t pid = fork();
//     if (pid == 0)
//         std::cout << "Child\n";
//     else
//         std::cout << "Parent\n";
// }

// int main()
// {
//    char *argv[] = {
//     (char *)"/bin/ls",
//     (char *)"-l",
//     NULL
//    };

//    execve(argv[0], argv, NULL);
// }

// int main()
// {
//    int fd[2];
//    pipe(fd);

//    pid_t pid = fork();

//    if (pid == 0)
//    {
// 	  dup2(fd[1], STDOUT_FILENO);
//       close(fd[0]);
//       close(fd[1]);

//       std::cout << "Hello from child\n";
//    }
//    else
//    {
//       close(fd[1]);
//       char buffer[1024];

//       int n = read(fd[0], buffer, sizeof(buffer));

//       std::cout << "Perant received: ";
//       std::cout.write(buffer, n);
//    }
// }

// int main()
// {
//    int fd[2];
//    if (pipe(fd) == -1)
//    {
//       perror("pipe");
//       return 1;
//    }

//    pid_t pid = fork();

//    if (pid == -1)
//    {
//       perror("fork");
//       return 1;
//    }

//    if (pid == 0)
//    {
//       dup2(fd[1], STDOUT_FILENO);

//       close(fd[0]);
//       close(fd[1]);

//       char *argv[] = {
//           (char *)"./hello.py",
//           NULL};

//       char *envp[] = {
//           (char *)"REQUEST_METHOD=GET",
//           (char *)"QUERY_STRING=name=hello",
//           (char *)"SCRIPT_NAME=/hello.py",
//          //  (char *)"REQUEST_METHOD=GET",
//          //  (char *)"QUERY_STRING=name=hello",
//          //  (char *)"CONTENT_LENGTH=0",
//          //  (char *)"CONTENT_TYPE=text/plain",
//          //  (char *)"SCRIPT_NAME=/hello.py",
//          //  (char *)"SERVER_PROTOCOL=HTTP/1.1",
//          //  (char *)"SERVER_NAME=localhost",
//           NULL};
//       execve("./hello.py", argv, envp);
//       perror("execve");
//    }
//    else
//    {
//       close(fd[1]);
//       char buffer[1024];
//       ssize_t bytesRead = read(fd[0], buffer, sizeof(buffer) - 1);
//       if (bytesRead > 0)
//       {
//          buffer[bytesRead] = '\0';

//          std::cout << "CGI OUTPUT:\n";
//          std::cout << buffer << std::endl;
//       }
//       close(fd[0]);
//       waitpid(pid, NULL, 0);
//    }
//    return 0;
// }

// int main()
// {
//    CGIExecutor cgi;

//    cgi.getEnv().set("REQUEST_METHOD", "GET");
//    cgi.getEnv().set("QUERY_STRING", "name=mehdi");
//    cgi.getEnv().set("SCRIPT_NAME", "/cgi-bin/hello.py");

//    std::string result = cgi.execute("./hello.py", "");

//    std::cout << "CGI OUTPUT:\n";
//    std::cout << result << std::endl;
// }

// int main()
// {
//     CGIExecutor cgi;

//     std::string body = "name=mehdi&age=42";

//     cgi.getEnv().set("REQUEST_METHOD", "POST");
//     // cgi.getEnv().set("CONTENT_LENGTH", "18");
//     cgi.getEnv().set("SCRIPT_NAME", "/cgi-bin/post.py");
//     cgi.getEnv().set(
//         "CONTENT_LENGTH",
//         std::to_string(body.size()));

//     std::string result =
//         cgi.execute("./post.py", body);

//     std::cout << result << std::endl;
// }

// int main()
// {
//     CGIExecutor cgi;

//     try
//     {
//         std::string result = cgi.execute("./slow.py", "");
//         std::cout << result << std::endl;
//     }
//     catch (const std::exception &e)
//     {
//         std::cerr << e.what() << std::endl;
//     }
//     // std::string result =
//     //     cgi.execute("./slow.py", "");

//     // std::cout << result << std::endl;
// }

// int main()
// {
//     CGIExecutor cgi;

//     try
//     {
//         std::string result = cgi.execute("./error.py", "");

//         std::cout << result << std::endl;
//     }
//     catch (const std::exception &e)
//     {
//         std::cerr << "CGI failed: " << e.what() << std::endl;
//     }
// }

// int main()
// {
//     CGIExecutor cgi;

//     std::string result =
//         cgi.execute("./partial.py", "");

//     std::cout << result << std::endl;
// }