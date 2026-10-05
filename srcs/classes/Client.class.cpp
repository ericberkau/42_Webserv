/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:23:20 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/05 18:50:27 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/Client.class.hpp"
#include "classes/RequestHandler.class.hpp"
#include "classes/ResponseBuilder.class.hpp"
#include "signals.hpp"
#include <iostream>
#include <unistd.h>
#include <queue>

Client::Client(int fd, const sockaddr_in& addr, const ServerConfig* server)
	:	_fd(fd),
		_inputReadProgress(0),
		_outputWriteProgress(0),
		_addr(addr),
		_server(server),
		_pendingDisconnect(false),
		_lastActivity(Clock::now())
{
	std::clog << "[Client] constructor called" << std::endl; // FIXME: for debugging, remove later

	if (!server)
		throw std::logic_error("Client constructed with nullptr to ServerConfig");
}

Client::Client(Client&& other) noexcept
	:	_fd(other._fd),
		_inputBuffer(std::move(other._inputBuffer)),
		_inputReadProgress(other._inputReadProgress), // FIXME: not needed -> remove
		_outputBuffer(std::move(other._outputBuffer)),
		_outputWriteProgress(other._outputWriteProgress),
		_addr(other._addr),
		_server(other._server),
		_pendingDisconnect(other._pendingDisconnect),
		_lastActivity(other._lastActivity),
		_requestParser(std::move(other._requestParser)),
		_httpRequest(std::move(other._httpRequest))
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
		_inputReadProgress = other._inputReadProgress; // FIXME: not needed -> remove
		_outputWriteProgress = other._outputWriteProgress;
		_addr = other._addr;
		_server = other._server;
		_pendingDisconnect = other._pendingDisconnect;
		_lastActivity = other._lastActivity;
		_requestParser = std::move(other._requestParser);
		_httpRequest = std::move(other._httpRequest);

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

bool Client::hasPendingOutput(void) const {
	return !this->_outputBuffer.empty() && this->_outputWriteProgress < this->_outputBuffer.size();
}

bool Client::queueEmpty(void) const {
	return this->_responseQueue.empty();
}

Clock::time_point Client::getLastActivity(void) const {
	return this->_lastActivity;
}

bool Client::shouldDisconnect(void) const {
	return this->_pendingDisconnect;
}

void Client::advanceOutProgress(ssize_t bytes) {
	this->_outputWriteProgress += bytes;
}

void Client::appendToInBuf(const char* input, size_t size) {
	_inputBuffer.append(input, size);
	_inputReadProgress += size;
} // BUG: unused? -> remove

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

void Client::flushReadyResponses(void) {
	while (!g_shutdownRequested &&
		!_responseQueue.empty() &&
		_responseQueue.front().ready)
	{
		appendToOutBuf(std::move(_responseQueue.front().response));

		_responseQueue.pop_front();
	}
}

void Client::markForDisconnect(void) {
	this->_pendingDisconnect = true;
}

void Client::updateActivity(void) {
	this->_lastActivity = Clock::now();
}

std::queue<RequestResult> Client::processInput(void) {
	std::queue<RequestResult> results;

	try {
		_requestParser.feed(_inputBuffer, _httpRequest, _server);
		clearInBuf();

		while (_requestParser.getState() == RequestParser::PARSE_COMPLETE) {
			results.push(_handleRequest());
			_requestParser.feed("", _httpRequest, _server);
		}
	}
	catch (const HttpParseException& e) {
		std::list<ResponseSlot>::iterator slot = _reserveResponseSlot();
	
		ResponseBuilder rb;

		rb.generateCustomErrorPage(e.getStatusCode(), *_server);

		slot->response = rb.buildString();
		slot->ready = true;

		_resetRequestState();
		_requestParser.discardBuffer();
		clearInBuf();
		markForDisconnect();
	}

	return results;
}

RequestResult Client::_handleRequest(void) {
	std::list<ResponseSlot>::iterator slot = _reserveResponseSlot();

	RequestHandler requestHandler(_server);

	RequestResult result = requestHandler.processRequest(_httpRequest);
	result.responseSlot = slot;

	_resetRequestState();

	return result;
}

void Client::_resetRequestState(void) {
	_requestParser.reset();
	_httpRequest.clear();
}

std::list<ResponseSlot>::iterator Client::_reserveResponseSlot(void) {
	return _responseQueue.emplace(_responseQueue.end());
}
