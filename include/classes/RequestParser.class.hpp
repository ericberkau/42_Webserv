/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.class.hpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 10:56:06 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/06 22:32:41 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include "HttpRequestContainer.class.hpp"
#include "../config.hpp"
#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>

class HttpParseException : public std::exception
{
	public:
	
		HttpParseException(int statusCode, const std::string& reason);
		
		virtual				~HttpParseException() throw();
		virtual const char*	what() const throw();
		
		int		getStatusCode() const;
		bool	isFatal() const;
		
	private:
	
		int	_statusCode;
		std::string	_reason;

};

class RequestParser
{		
	public:
	
		enum ParsingState 
		{ 
			PARSE_REQUEST_LINE,
			PARSE_HEADERS, 
			PARSE_BODY, 
			PARSE_COMPLETE 
		};
		
		RequestParser();
		~RequestParser() = default;
		
		// Feeds raw data coming out of recv() piece by piece
		void	feed(const std::string& chunk, HttpRequestContainer& request, const ServerConfig* config);

		// Getters for the server loop to know when a request is ready
		ParsingState	getState() const;
		void			reset();
		void			discardBuffer();
		
	private:
	
		ParsingState	_state;
		std::string		_buffer;
		size_t			_expectedBodyBytes; // Stores Content-Length or current chunk size
		bool			_chunkSizeParsed; // Tracks if we are waiting for a chunk size line or chunk data bytes
		
		void	_parseRequestLine(const std::string& line, HttpRequestContainer& request); // Splits "METHOD URI VERSION" and validates each piece
		bool	_isValidHeaderKeyChar(unsigned char c);
		void	_parseHeaderLine(const std::string& line, HttpRequestContainer& request); // Splits "Key: Value" and adds it to the request
		bool	_isSupportedMethod(const std::string& method);
		bool	_isKnownHttpMethod(const std::string& method);
		
};

#endif
