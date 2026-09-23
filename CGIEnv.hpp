/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIEnv.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/09 20:26:46 by skomyshe          #+#    #+#             */
/*   Updated: 2026/06/09 20:46:27 by skomyshe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIENV_HPP
#define CGIENV_HPP

#include <map>
#include <string>

class CGIEnv
{
private:
    std::map<std::string, std::string> _env;

public:
    void set(
        const std::string &key,
        const std::string &value);

    char **buildEnvp() const;
    
    static void freeEnvp(char **envp);
};

#endif