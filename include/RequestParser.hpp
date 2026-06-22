/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: djanardh <djanardh@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 15:15:37 by djanardh          #+#    #+#             */
/*   Updated: 2026/06/22 16:10:16 by djanardh         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include "HttpRequestContainer.hpp"
#include <string>
#include <stdexcept>

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
	private:
		// Splits "METHOD URI VERSION" and validates each piece
		static void parseRequestLine(const std::string& line, HttpRequestContainer& request);

		// Splits "Key: Value" and adds it to the request
		static void parseHeaderLine(const std::string& line, HttpRequestContainer& request);

		static bool isSupportedMethod(const std::string& method);

	public:
		// So far only parsing one complete request string in one shot.
		// (No partial/incremental handling yet - that comes after
		// body parsing is added and then this should be hooked up to real recv() data later)
		static HttpRequestContainer parse(const std::string& raw);

};

#endif
