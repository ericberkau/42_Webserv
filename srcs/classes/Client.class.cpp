/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:23:20 by eberkau           #+#    #+#             */
/*   Updated: 2026/05/20 00:00:46 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Client.class.hpp"
#include <unistd.h>

Client::Client(int fd, sockaddr_in addr) : _fd(fd), _readProgress(0), _writeProgress(0), _addr(addr) {}

Client::Client(Client&& other) noexcept
	:	_fd(other._fd),
		_inputBuf(std::move(other._inputBuf)),
		_outputBuf(std::move(other._outputBuf)),
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
		_inputBuf = std::move(other._inputBuf);
		_outputBuf = std::move(other._outputBuf);
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

int Client::getFd(void) {
	return this->_fd;
}

const std::string& Client::getOutBuf() {
	return this->_outputBuf; // FIXME: + current writeProgr maybe?
}

const std::string& Client::getInBuf() {
	return this->_inputBuf;
}

void Client::appendToInBuf(const std::string& input, size_t size) {
	_inputBuf.append(input);
	_readProgress += size;
}

void Client::appendToOutBuf(const std::string& input, size_t size) {
	_outputBuf.append(input);
	_writeProgress += size;
}

void Client::clearInBuf(void) {
	_inputBuf.clear();
	_readProgress = 0;
}

void Client::clearOutBuf() {
	_outputBuf.clear();
	_writeProgress = 0;
}
