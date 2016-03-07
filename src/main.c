/*
 * main.c
 *
 * Copyright (C) 2014 - 2016, Xiaoxiao <i@pxx.io>
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
#include <sys/wait.h>
#include <unistd.h>
#include "conf.h"
#include "log.h"
#include "utils.h"
#include "vpn.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

// role == 0 -> master, role == 1 -> worker
static int role;

static void signal_cb(int signo)
{
    (void)signo;
    if (role == 1)
    {
        vpn_stop();
    }
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

    while (1)
    {
        int cpid = fork();
        if (cpid < 0)
        {
            LOG("failed to spawn worker process");
        }
        else if (cpid == 0)
        {
            // worker
            role = 1;
            vpn_run();
            return 0;
        }
        else
        {
            // master
            int status;
            wait(&status);
            if (WIFEXITED(status))
            {
                return 0;
            }
            else
            {
                LOG("worker terminated unexpectedly, respawn woker");
            }
        }
    }
}
