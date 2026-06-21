#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include "HttpRequest.hpp"
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
		static void parseRequestLine(const std::string& line, HttpRequest& request);

		// Splits "Key: Value" and adds it to the request
		static void parseHeaderLine(const std::string& line, HttpRequest& request);

		// Only these are supported for now as per the subject's requirements
		static bool isSupportedMethod(const std::string& method);

	public:
		// So far only parsing one complete request string in one shot.
		// (No partial/incremental handling yet - that comes after
		// body parsing is added and then this should be hooked up to real recv() data later)
		static HttpRequest parse(const std::string& raw);

};

#endif
