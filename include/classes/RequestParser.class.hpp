/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.class.hpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/07 10:56:06 by djanardh          #+#    #+#             */
/*   Updated: 2026/10/10 02:40:21 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include "config.hpp"
#include "HttpRequestContainer.class.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

struct NeedMoreInput {};

class HttpParseException : public std::exception
{
	public:
	
		HttpParseException(int statusCode);
		
		virtual	~HttpParseException() throw(); // FIXME: talk about structure
		// virtual const char*	what() const throw(); // TODO: remove?
		
		int		getStatusCode() const;
		bool	isFatal() const;
		
	private:
	
		int	_statusCode;
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
		void			discardBuffer();
		ParsingState	getState() const;
		void			reset();


	private:
	
		ParsingState	_state;
		std::string		_buffer;
		size_t			_expectedBodyBytes; // Stores Content-Length or current chunk size
		bool			_chunkSizeParsed; // Tracks if we are waiting for a chunk size line or chunk data bytes
		size_t			_maxBodySize;
		size_t			_headerBytes;
		
		bool	_isKnownHttpMethod(const std::string& method);
		bool	_isSupportedMethod(const std::string& method);
		bool	_isValidHeaderKeyChar(unsigned char c);

		void	_parseBody(HttpRequestContainer& request);
		void	_parseChunked(HttpRequestContainer& request);
		bool	_parseChunkBody(HttpRequestContainer& request);
		void	_parseChunkSize(HttpRequestContainer& request);
		void	_parseHeaderLine(const std::string& line, HttpRequestContainer& request); // Splits "Key: Value" and adds it to the request
		void	_parseLengthBasedBody(HttpRequestContainer& request, const std::string& contentLengthStr);
		void	_parseRequestHeader(HttpRequestContainer& request, const ServerConfig* config);
		void	_parseRequestLine(const std::string& line, HttpRequestContainer& request); // Splits "METHOD URI VERSION" and validates each piece

};

#endif
