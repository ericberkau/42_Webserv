/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIEnv.class.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/09 20:26:46 by skomyshe          #+#    #+#             */
/*   Updated: 2026/09/25 01:13:01 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIENV_HPP
#define CGIENV_HPP

#include <map>
#include <string>

class CGIEnv
{
	public:

		void		set(const std::string &key, const std::string &value);
		char		**buildEnvp() const;
		static void	freeEnvp(char **envp);


	private:

		std::map<std::string, std::string>	_env;

};

#endif