/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.class.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 23:02:10 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/02 01:59:23 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "config.hpp"
#include "networking.hpp"
#include <poll.h>
#include <set>
#include <unordered_map>
#include <vector>

class Server {

	public:

		explicit Server(const Config& config);
		~Server(void);

		void	run(void);


	private:

		/* setup */
		void	_setupListeners(void);
		int		_setupListeningSocket(const ListenDirective& listenCfg);
		bool	_isDuplicateAddress(uint32_t host, uint16_t port);

		/* event handling */
		void	_disconnectClient(size_t& idx);
		void	_handleClientEvents(size_t& idx);
		void	_handleIncomingData(size_t& idx);
		void	_handleListenerEvents(size_t idx);
		void	_handleOutgoingData(size_t& idx);
		void	_handlePendingConnections(int listeningSocket);
		void	_handleSocketEvent(size_t& idx);

		/* server state */
		const Config&							_config;
		std::vector<pollfd>						_pollfdArr;
		std::unordered_map<int, FdContext>		_fdCtx;
		std::set<std::pair<uint32_t, uint16_t>>	_usedAddresses;

		/* disallow copying or moving */
		Server(const Server&) = delete;
		Server&	operator=(const Server&) = delete;
		Server(Server&&) = delete;
		Server&	operator=(Server&&) = delete;

};
