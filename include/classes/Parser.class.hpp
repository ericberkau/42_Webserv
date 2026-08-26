/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.class.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/29 16:09:13 by eberkau           #+#    #+#             */
/*   Updated: 2026/08/27 00:16:42 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "config.hpp"
#include "token.hpp"

class Parser {

	public:

		explicit Parser(const std::vector<Token>& tokens);
		~Parser(void) = default;

		Config	parse(void);


	private:

		void			_advance(void);
		bool			_atEnd(void) const;
		const Token&	_current(void) const;
		// const Token&	_previous(void) const; // FIXME: check if needed

		bool	_check(TokenType type) const;
		// bool	_match(TokenType type); // FIXME: check if needed
		void	_expect(TokenType type, const std::string& msg);
		void	_expect(const std::string& lexeme, const std::string& msg);

		Config			_parseConfig(void);

		ServerConfig	_parseServer(void);
		void			_parseServerDirective(ServerConfig& server);
		void			_parseListen(ServerConfig& server);
		void			_parseClientMaxBodySize(ServerConfig& server);

		LocationConfig	_parseLocation(void);
		void			_parseLocationDirective(LocationConfig& location);
		// void			_parseRoot(LocationConfig& location); // FIXME: delete
		// void			_parseIndex(LocationConfig& location); // FIXME: delete
		void			_parseAllowMethods(LocationConfig& location);
		void			_parseReturn(LocationConfig& location);
		void			_parseBoolSetting(bool& configOption);
		void			_parsePathSetting(std::string& configOption);

		const std::vector<Token>&	_tokens;
		size_t						_currentPos;

		/* disallow copying or moving */
		Parser(const Parser&) = delete;
		Parser&	operator=(const Parser&) = delete;
		Parser(Parser&&) = delete;
		Parser&	operator=(Parser&&) = delete;

};
