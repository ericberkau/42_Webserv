# Webserv

_This project has been created as part of the 42 curriculum by eberkau, skomyshe, djanardh._

## Description
Webserv is an HTTP server written in C++.

The goal of this project is to understand how web servers work by implementing one from scratch, taking Nginx as inspiration. The server handles HTTP requests, processes them according to a configuration file, and sends appropriate HTTP responses to clients.

The project covers topics such as:

TCP/IP socket programming

Non-blocking I/O

Multiple client connections

Configuration and server setup

HTTP request and response handling

HTTP methods such as GET, POST, and DELETE

CGI execution

Error handling

## Instructions

### _Requirements_
C++ compiler supporting C++17

make

### _Compilation_
Clone the repository and run:

`make`

This creates the webserv executable.

### _Execution_
Run the server:

`./webserv`

Without a second argument, basic.conf runs by default.

The server can be run with a configuration file of choice too:

`./webserv configs/example.conf`

The server will start listening on the ports defined in the configuration file.

You can then connect to it using a web browser or a tool such as curl:

`curl http://localhost:8080`

To remove object files:

`make clean`

To remove all generated files:

`make fclean`

To recompile everything:

`make re`

## Features
Multiple server configurations

Multiple listening ports

HTTP/1.1 request handling

GET, POST, and DELETE

Static file serving

Custom error pages

Redirects

File uploads

CGI support

Non-blocking sockets

Multiple simultaneous clients

Configurable routes and server settings

## Resources
HTTP/1.1 — RFC 9112

[https://www.rfc-editor.org/rfc/rfc2616](https://datatracker.ietf.org/doc/html/rfc9112)

HTTP Semantics — RFC 9110

[https://www.rfc-editor.org/rfc/rfc9110](https://datatracker.ietf.org/doc/html/rfc9110)

CGI Digital Guide - What is CGI?

https://www.ionos.com/digitalguide/websites/web-development/what-is-a-cgi/

CGI book - CGI Programming on the World Wide Web

https://www.oreilly.com/library/view/cgi-programming-on/9781565921689/04_chapter-01.html