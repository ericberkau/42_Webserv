/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/08 18:11:25 by eberkau           #+#    #+#             */
/*   Updated: 2026/07/08 18:28:41 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <list>
#include <string>

struct s_config {
	std::list<std::string>	portsToListen;
	size_t					clientReqMaxSize;
	std::list<std::string>	acceptedHttpsMethods;
	std::string				rootedDir;
	bool					dirListing;
	bool					uploadAllowed;
	// defaultErrorPages;
};

using t_config = s_config;
