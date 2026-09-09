/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:23:20 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/09 21:55:50 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Client.class.hpp"
#include "classes/RequestHandler.class.hpp"
#include <iostream>
#include <unistd.h>

Client::Client(int fd, const sockaddr_in& addr, const ServerConfig* server)
	: _fd(fd), _inputReadProgress(0), _outputWriteProgress(0), _addr(addr), _server(server)
{
	std::clog << "[Client] constructor called" << std::endl; // FIXME: for debugging, remove later

	if (!server)
		throw std::logic_error("Client constructed with nullptr to ServerConfig");
}

Client::Client(Client&& other) noexcept
	:	_fd(other._fd),
		_inputBuffer(std::move(other._inputBuffer)),
		_outputBuffer(std::move(other._outputBuffer)),
		_outputWriteProgress(other._outputWriteProgress),
		_addr(other._addr),
		_server(other._server)
{
	std::clog << "[Client] move constructor called" << std::endl; // FIXME: for debugging, remove later

    other._fd = -1;
}

Client& Client::operator=(Client&& other) noexcept {
	std::clog << "[Client] move operator called" << std::endl; // FIXME: for debugging, remove later

	if (this != &other) {
		if (_fd >= 0)
			close(this->_fd); // FIXME: potential fail -> handle?

		_fd = other._fd;
		_inputBuffer = std::move(other._inputBuffer);
		_outputBuffer = std::move(other._outputBuffer);
		_outputWriteProgress = other._outputWriteProgress;
		_addr = other._addr;
		_server = other._server;
		
		other._fd = -1;
	}

	return *this;
}

Client::~Client(void) {
	std::clog << "[Client] destructor called on fd: " << this->_fd << std::endl; // FIXME: for debugging, remove later

	if (_fd >= 0) {
		close(this->_fd);
		_fd = -1;
	}
}

int Client::getFd(void) const {
	return this->_fd;
}

const std::string& Client::getInBuf(void) const {
	return this->_inputBuffer;
}

const std::string& Client::getOutBuf(void) const {
	return this->_outputBuffer; // FIXME: + current writeProgr maybe?
}

size_t Client::getOutProgress(void) const {
	return this->_outputWriteProgress;
}

void Client::advanceOutProgress(ssize_t bytes) {
	this->_outputWriteProgress += bytes;
}

void Client::appendToInBuf(const char* input, size_t size) {
	_inputBuffer.append(input, size);
	_inputReadProgress += size;
}

void Client::appendToOutBuf(const std::string& input) {
	_outputBuffer.append(input);
}

void Client::clearInBuf(void) {
	_inputBuffer.clear();
	_inputReadProgress = 0;
}

void Client::clearOutBuf(void) {
	_outputBuffer.clear();
	_outputWriteProgress = 0;
}

bool Client::hasPendingOutput(void) const {
	return !this->_outputBuffer.empty() && this->_outputWriteProgress < this->_outputBuffer.size();
}

void Client::processInput(void) {
	_requestParser.feed(_inputBuffer, _httpRequest, _server);
	clearInBuf();

	if (_requestParser.getState() == RequestParser::PARSE_COMPLETE)
		_handleRequest();
}

void Client::_handleRequest(void) {
	RequestHandler requestHandler(_server);

	const std::string response = requestHandler.processRequest(_httpRequest);

	appendToOutBuf(response);

	_resetRequestState();
}

void Client::_resetRequestState(void) {
	_requestParser.reset();
	// TODO: maybe reset HttpRequestContainer "_httpRequest" as well?
}
