/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 15:15:37 by djanardh          #+#    #+#             */
/*   Updated: 2026/06/24 01:46:08 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include "HttpRequestContainer.hpp"
#include <string>
#include <stdexcept>
#include <vector>

class HttpParseException : public std::exception
{
	private:
		int _statusCode;
		std::string _reason;

	public:
		HttpParseException(int statusCode, const std::string& reason);
		virtual ~HttpParseException() throw();
		virtual const char* what() const throw();
		int getStatusCode() const;
};

class RequestParser
{
	public:
		enum ParsingState 
		{ 
			PARSE_REQUEST_LINE, // or CHOOSE_REQUEST_LINE(?)
			PARSE_HEADERS, 
			PARSE_BODY, 
			PARSE_COMPLETE 
		};
		
	private:
		ParsingState _state;
		std::string _buffer;
		void parseRequestLine(const std::string& line, HttpRequestContainer& request); // Splits "METHOD URI VERSION" and validates each piece
		bool isValidHeaderKeyChar(unsigned char c);
		void parseHeaderLine(const std::string& line, HttpRequestContainer& request); // Splits "Key: Value" and adds it to the request
		bool isSupportedMethod(const std::string& method);
		bool isKnownHttpMethod(const std::string& method);
		
	public:
		RequestParser();
		~RequestParser();
		// So far only parsing one complete request string in one shot.
		// (No partial/incremental handling yet - that comes after
		// body parsing is added and then this should be hooked up to real recv() data later)
		// HttpRequestContainer parse(const std::string& raw);
		
		// Feeds raw data coming out of recv() piece by piece
		void feed(const std::string& chunk, HttpRequestContainer& request);

		// Getters for the server loop to know when a request is ready
		ParsingState getState() const;
		void reset();
};

#endif
