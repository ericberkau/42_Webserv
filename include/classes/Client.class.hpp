/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.class.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:23:06 by eberkau           #+#    #+#             */
/*   Updated: 2026/05/29 01:04:17 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <arpa/inet.h>

class Client {

	public:

		Client(int fd, sockaddr_in addr);
		Client(Client&& other) noexcept;
		Client& operator=(Client&& other) noexcept;
		~Client();

		int					getFd() const;
		const std::string&	getInBuf() const;
		const std::string&	getOutBuf() const;
		
		void	appendToInBuf(const char* input, size_t size);
		void	appendToOutBuf(const char* input, size_t size);
		void	clearInBuf();
		void	clearOutBuf();
		bool	hasPendingOutput() const;


	private:

		Client(void) = delete;
		Client(const Client&) = delete;
		Client& operator=(const Client&) = delete;

		int			_fd;
		std::string	_inputBuffer; // TODO: limit size later
		size_t		_readProgress;
		std::string	_outputBuffer;
		size_t		_writeProgress;
		sockaddr_in	_addr;

		// int			_requestParserState;
		// int			_timeoutInfo

};
