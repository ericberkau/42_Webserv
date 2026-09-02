/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.class.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:23:06 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/02 01:57:12 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <arpa/inet.h>
#include <string>

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

		int			_fd;
		std::string	_inputBuffer; // TODO: limit size later
		size_t		_inputReadProgress;
		std::string	_outputBuffer;
		size_t		_outputWriteProgress;
		sockaddr_in	_addr;

		/* diallow copying */
		Client(const Client&) = delete;
		Client& operator=(const Client&) = delete;

		// int			_requestParserState;
		// int			_timeoutInfo

};
