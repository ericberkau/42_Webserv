/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:23:20 by eberkau           #+#    #+#             */
/*   Updated: 2026/05/29 01:04:26 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Client.class.hpp"
#include <unistd.h>

Client::Client(int fd, sockaddr_in addr) : _fd(fd), _readProgress(0), _writeProgress(0), _addr(addr) {}

Client::Client(Client&& other) noexcept
	:	_fd(other._fd),
		_inputBuffer(std::move(other._inputBuffer)),
		_outputBuffer(std::move(other._outputBuffer)),
		_writeProgress(other._writeProgress),
		_addr(other._addr)
{
    other._fd = -1;
}

Client& Client::operator=(Client&& other) noexcept {
	if (this != &other) {
		if (_fd >= 0)
			close(this->_fd); // FIXME: potential fail -> handle?

		_fd = other._fd;
		_inputBuffer = std::move(other._inputBuffer);
		_outputBuffer = std::move(other._outputBuffer);
		_writeProgress = other._writeProgress;
		_addr = other._addr;
		
		other._fd = -1;
	}

	return *this;
}

Client::~Client(void) {
	if (_fd >= 0)
		close(this->_fd);
}

int Client::getFd(void) const {
	return this->_fd;
}

const std::string& Client::getOutBuf(void) const {
	return this->_outputBuffer; // FIXME: + current writeProgr maybe?
}

const std::string& Client::getInBuf(void) const {
	return this->_inputBuffer;
}

bool Client::hasPendingOutput(void) const {
	return this->_writeProgress < this->_outputBuffer.size();
}

void Client::appendToInBuf(const char* input, size_t size) {
	_inputBuffer.append(input, size);
	_readProgress += size;
}

void Client::appendToOutBuf(const char* input, size_t size) {
	_outputBuffer.append(input, size);
	_writeProgress += size;
}

void Client::clearInBuf(void) {
	_inputBuffer.clear();
	_readProgress = 0;
}

void Client::clearOutBuf(void) {
	_outputBuffer.clear();
	_writeProgress = 0;
}
