/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.class.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:23:06 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/09 20:19:47 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "classes/RequestParser.class.hpp"
#include "classes/HttpRequestContainer.class.hpp"
#include <arpa/inet.h>
#include <string>

class Client {

	public:

		Client(int fd, const sockaddr_in& addr, const ServerConfig* server);
		Client(Client&& other) noexcept;
		Client& operator=(Client&& other) noexcept;
		~Client(void);

		int							getFd(void) const;
		const std::string&			getInBuf(void) const;
		const std::string&			getOutBuf(void) const;
		size_t						getOutProgress(void) const;
		RequestParser::ParsingState	getRequestState(void) const;
		const HttpRequestContainer&	getRequest(void) const;
		const ServerConfig*			getServer(void) const;

		void	advanceOutProgress(ssize_t bytes);
		void	appendToInBuf(const char* input, size_t size);
		void	appendToOutBuf(const std::string& input);
		void	clearInBuf(void);
		void	clearOutBuf(void);
		bool	hasPendingOutput(void) const;
		void	processInputBuffer(void);
		void	resetRequestState(void);


	private:

		int					_fd;
		std::string			_inputBuffer; // TODO: limit size later
		size_t				_inputReadProgress; // FIXME: not needed at all?
		std::string			_outputBuffer;
		size_t				_outputWriteProgress;
		sockaddr_in			_addr;
		const ServerConfig*	_server;

		RequestParser			_requestParser;
		HttpRequestContainer	_httpRequest;

		/* diallow copying */
		Client(const Client&) = delete;
		Client& operator=(const Client&) = delete;

};
