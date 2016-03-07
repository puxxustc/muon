/*
 * utils.h - some util functions
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

#ifndef UTILS_H
#define UTILS_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

extern int runas(const char *user);
extern int daemonize(const char *pidfile, const char *logfile);
extern int route(const char *tunif, const char *server, int ipv4, int ipv6);
#ifdef TARGET_LINUX
extern int ifconfig(const char *tunif, int mtu, const char *address, const char *address6);
extern int nat(const char *address, int on);
#endif
#ifdef TARGET_DARWIN
extern int ifconfig(const char *tunif, int mtu, const char *address, const char *peer, const char *address6);
#endif

#endif // UTILS_H
