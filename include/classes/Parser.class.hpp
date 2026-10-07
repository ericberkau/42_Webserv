/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.class.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/29 16:09:13 by eberkau           #+#    #+#             */
/*   Updated: 2026/10/07 17:57:03 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "config.hpp"
#include "parserUtils.hpp"
#include "token.hpp"

class Parser {

	public:

		explicit Parser(const std::vector<Token>& tokens);
		~Parser(void) = default;

		Config	parse(void);


	private:

		/* token traversal*/
		void			_advance(void);
		bool			_atEnd(void) const;
		const Token&	_current(void) const;

		/* parsing foundation */
		bool			_check(TokenType type) const;
		void			_expect(TokenType type, const std::string& msg);
		void			_expect(const std::string& lexeme, const std::string& msg);
		Config			_parseConfig(void);

		/* server parsing */
		ServerConfig	_parseServer(void);
		void			_parseServerDirective(ServerConfig& server);
		void			_parseErrorPage(ServerConfig& server);
		void			_parseListen(ServerConfig& server);

		/* location parsing */
		LocationConfig	_parseLocation(void);
		void			_parseLocationDirective(LocationConfig& location);
		void			_parseAllowMethods(LocationConfig& location);
		void			_parseBoolSetting(bool& configOption);
		void			_parseCgi(LocationConfig& location);
		void			_parsePathSetting(std::string& configOption);
		void			_parseReturn(LocationConfig& location);

		template<typename ConfigType>
		void			_parseClientMaxBodySize(ConfigType& targetConfig);

		/* parser state */
		const std::vector<Token>&	_tokens;
		size_t						_currentPos;

		/* disallow copying or moving */
		Parser(const Parser&) = delete;
		Parser&	operator=(const Parser&) = delete;
		Parser(Parser&&) = delete;
		Parser&	operator=(Parser&&) = delete;

};

template<typename ConfigType>
void Parser::_parseClientMaxBodySize(ConfigType& targetConfig) {
	_advance();

	_expect(TokenType::WORD, "expected size value after 'client_max_body_size'");
	const Token& sizeToken = _current();
	_advance();

	_expect(TokenType::SEMICOLON, "expected ';' after client_max_body_size value");
	_advance();

	const size_t size = parseSizeValue(sizeToken);

	// if (size > 1024 * 1024 * 10)
	// 		error(sizeToken, "size exceeds limit (size <= 10 MiB)"); // FIXME: 10 MiB as upper client body limit?

	targetConfig.clientMaxBodySize = size;
}
