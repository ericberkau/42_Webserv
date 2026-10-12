/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.class.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:23:20 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/11 23:33:03 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "classes/client/Client.class.hpp"
#include "classes/HTTP/RequestHandler.class.hpp"
#include "classes/HTTP/ResponseBuilder.class.hpp"
#include "signals.hpp"
#include <iostream>
#include <queue>
#include <unistd.h>

Client::Client(int fd, const sockaddr_in& addr, const ServerConfig* server)
	:	_fd(fd),
		_outputWriteProgress(0),
		_addr(addr),
		_server(server),
		_pendingDisconnect(false),
		_lastActivity(Clock::now())
{
	// std::clog << "[Client] constructor called" << std::endl;

	if (!server)
		throw std::logic_error("Client constructed with nullptr to ServerConfig");
}

Client::Client(Client&& other) noexcept
	:	_fd(other._fd),
		_inputBuffer(std::move(other._inputBuffer)),
		_outputBuffer(std::move(other._outputBuffer)),
		_outputWriteProgress(other._outputWriteProgress),
		_addr(other._addr),
		_server(other._server),
		_pendingDisconnect(other._pendingDisconnect),
		_lastActivity(other._lastActivity),
		_requestParser(std::move(other._requestParser)),
		_httpRequest(std::move(other._httpRequest))
{
	// std::clog << "[Client] move constructor called" << std::endl;

	other._fd = -1;
}

Client& Client::operator=(Client&& other) noexcept {
	// std::clog << "[Client] move operator called" << std::endl;

	if (this != &other) {
		if (_fd >= 0)
			close(this->_fd);

		_fd = other._fd;
		_inputBuffer = std::move(other._inputBuffer);
		_outputBuffer = std::move(other._outputBuffer);
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
	// std::clog << "[Client] destructor called on fd: " << this->_fd << std::endl;

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
	return this->_outputBuffer;
}

size_t Client::getOutProgress(void) const {
	return this->_outputWriteProgress;
}

const ServerConfig& Client::getServer(void) const {
	return *this->_server;
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
}

void Client::clearInBuf(void) {
	_inputBuffer.clear();
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
		_appendToOutBuf(std::move(_responseQueue.front().response));

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
		rb.generateErrorPage(e.getStatusCode(), *_server);

		slot->response = rb.buildString();
		slot->ready = true;

		_resetRequestState();
		_requestParser.discardBuffer();
		clearInBuf();
		if (e.isFatal())
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

void Client::_appendToOutBuf(const std::string& input) {
	_outputBuffer.append(input);
}
