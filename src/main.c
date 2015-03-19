/*
 * main.c
 *
 * Copyright (C) 2014 - 2015, Xiaoxiao <i@xiaoxiao.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "conf.h"
#include "utils.h"
#include "vpn.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

static void signal_cb(int signo)
{
	(void)signo;
	vpn_stop();
}

int main(int argc, char **argv)
{
	conf_t conf;

	if (parse_args(argc, argv, &conf) != 0)
	{
		return EXIT_FAILURE;
	}

	// Daemonize
	if (conf.daemon)
	{
		if (daemonize(conf.pidfile, conf.logfile) != 0)
		{
			return EXIT_FAILURE;
		}
	}

	// 注册 signal handle
#ifdef HAVE_SIGACTION
	struct sigaction sa;
	sa.sa_handler = signal_cb;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
#else
	signal(SIGINT, signal_cb);
	signal(SIGTERM, signal_cb);
	signal(SIGHUP, signal_cb);
#endif

	if (vpn_init(&conf) != 0)
	{
		return EXIT_FAILURE;
	}

	return vpn_run();
}
