/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.class.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 23:02:10 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/01 23:17:12 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "config.hpp"
#include "networking.hpp"
#include <vector>
#include <unordered_map>
#include <set>
#include <poll.h>

class Server {

	public:

		explicit Server(const Config& config);
		~Server(void) = default;

		void	run(void);


	private:

		/* event handling */
		void	_handleSingleSocket(size_t idx);

		/* server state */
		const Config&							_config;
		std::vector<pollfd>						_pollFdArr;
		std::unordered_map<int, FdContext>		_fdCtx;
		std::set<std::pair<uint32_t, uint16_t>>	_usedAddresses;

		/* disallow copying or moving */
		Server(const Server&) = delete;
		Server&	operator=(const Server&) = delete;
		Server(Server&&) = delete;
		Server&	operator=(Server&&) = delete;

};
