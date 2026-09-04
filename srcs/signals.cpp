/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signals.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 19:09:11 by eberkau           #+#    #+#             */
/*   Updated: 2026/09/04 19:17:18 by eberkau          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "signals.hpp"

volatile sig_atomic_t g_shutdownRequested = false;

void handleShutdownSignal(int signum) {
	g_shutdownRequested = true;
	(void)signum;
}
