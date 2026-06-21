#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>

// This class is only a container for the result of parsing.
class HttpRequest
{
	private:
		std::string _method;
		std::string _uri;
		std::string _version;
		std::map<std::string, std::string> _headers; // keys stored lowercase
		std::string _body;
		
	public:
		HttpRequest();

		void setMethod(const std::string& method);
		void setUri(const std::string& uri);
		void setVersion(const std::string& version);
		void addHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);

		const std::string& getMethod() const;
		const std::string& getUri() const;
		const std::string& getVersion() const;
		const std::string& getBody() const;

		// Returns "" if header not found. Header lookup is case-insensitive
		// acc. to RFC, so we normalize keys to lowercase when storing them.
		std::string getHeader(const std::string& key) const;

		// For debugging to print everything that is parsed
		void print() const;

};

#endif
