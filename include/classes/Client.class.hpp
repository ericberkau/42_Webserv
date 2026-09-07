/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.class.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/19 23:23:06 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/07 10:56:00 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "classes/RequestParser.class.hpp"
#include "classes/HttpRequestContainer.class.hpp"
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

		RequestParser			requestParser;
		HttpRequestContainer	httpRequest;


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
