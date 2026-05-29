/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.class.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:23:06 by eberkau           #+#    #+#             */
/*   Updated: 2026/05/29 02:21:05 by eberkau          ###   ########.fr       */
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
		size_t				getOutProgress() const;
		
		void	advanceOutProgress(ssize_t bytes);
		void	appendToInBuf(const char* input, size_t size);
		void	appendToOutBuf(const std::string& input);
		void	clearInBuf();
		void	clearOutBuf();
		bool	hasPendingOutput() const;


	private:

		Client(void) = delete;
		Client(const Client&) = delete;
		Client& operator=(const Client&) = delete;

		int			_fd;
		std::string	_inputBuffer; // TODO: limit size later
		size_t		_inputReadProgress;
		std::string	_outputBuffer;
		size_t		_outputWriteProgress;
		sockaddr_in	_addr;

		// int			_requestParserState;
		// int			_timeoutInfo

};
