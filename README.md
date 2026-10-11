# Webserv

_This project has been created as part of the 42 curriculum by djanardh, eberkau and skomyshe._

## Description

**webserv** is a custom HTTP/1.1 web server implemented in C++.

The goal of the project is to understand the fundamentals of web servers by reimplementing core networking and HTTP functionality from scratch, without relying on existing web server frameworks, but taking Nginx as inspiration. The project covers topics such as socket programming, event-driven I/O, HTTP request parsing, response generation, configuration handling, process management, and CGI execution.

The server uses non-blocking sockets together with the `poll()` system call to handle multiple simultaneous client connections within a single event loop.

Main features include:

* HTTP/1.1 request handling
* Non-blocking TCP networking
* Multiple virtual servers
* Configurable listening ports and host addresses
* Location-based routing
* Static file serving
* Custom error pages
* CGI support
* Request method handling
* Connection management

## Instructions

### _Requirements_

* Linux or macOS
* C++17 compatible compiler
* GNU Make

### _Compilation_

Clone the repository and run:

```bash
make
```

This produces the executable:

```bash
./webserv
```

### _Execution_

Run the server by executing the generated binary. If a configuration file is provided, the server will use the settings defined in that file. If no configuration file is specified, the server falls back to a default basic working configuration located at `configs/basic.conf`.

```bash
./webserv [file.conf]
```

Example:

```bash
./webserv configs/example.conf
```

The server will start listening on the addresses and ports defined in the configuration file.

To remove object files:

```bash
make clean
```

To remove all generated files:

```bash
make fclean
```

To recompile everything:

```bash
make re
```

### _Testing_

Example request using curl:

```bash
curl http://localhost:8080/
```

Example load test using Siege:

```bash
siege -c 50 -t 30S http://localhost:8080
```


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

Timeouts for idle clients and running CGI processes

## Project Structure

```text
.
├── configs/
├── html/
├── include/
├── srcs/
├── tests/
├── Makefile
└── README.md
```

## Resources

### Networking

* Beej's Guide to Network Programming
  https://beej.us/guide/bgnet/

* Linux `poll(2)` manual page
  https://linux.die.net/man/2/poll

* Linux socket programming documentation
  https://linux.die.net/man/7/socket

### HTTP

* RFC 9110 — HTTP Semantics
  https://www.rfc-editor.org/rfc/rfc9110

* RFC 9112 — HTTP/1.1
  https://www.rfc-editor.org/rfc/rfc9112

* Mozilla HTTP Documentation
  https://developer.mozilla.org/en-US/docs/Web/HTTP

### CGI

* RFC 3875 — CGI Specification
  https://www.rfc-editor.org/rfc/rfc3875

* CGI Digital Guide - What is CGI?
  https://www.ionos.com/digitalguide/websites/web-development/what-is-a-cgi/

* CGI book - CGI Programming on the World Wide Web
  https://www.oreilly.com/library/view/cgi-programming-on/9781565921689/04_chapter-01.html

### AI Usage

AI tools were used as an auxiliary learning and review resource during development.

Typical use cases included:

* Clarification of networking concepts (`poll`, sockets, non-blocking I/O, TCP behavior)
* Discussion of C++ design decisions and class structure
* Review of implementation approaches and edge cases
* Identification of potential bugs and resource-management issues
* Generation of testing scripts using Python

All architectural decisions, implementation work, debugging, testing, and final code were performed by the project authors.
