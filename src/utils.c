/*
 * utils.c - some util functions
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

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "utils.h"

int setnonblock(int fd)
{
	int flags;
	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		return -1;
	}
	if (-1 == fcntl(fd, F_SETFL, flags | O_NONBLOCK))
	{
		return -1;
	}
	return 0;
}

int runas(const char *user)
{
	struct passwd *pw_ent = NULL;

	if (user != NULL)
	{
		pw_ent = getpwnam(user);
	}

	if (pw_ent != NULL)
	{
		if (setegid(pw_ent->pw_gid) != 0)
		{
			return -1;
		}
		if (seteuid(pw_ent->pw_uid) != 0)
		{
			return -1;
		}
	}

	return 0;
}

int daemonize(const char *pidfile, const char *logfile)
{
	pid_t pid;

	pid = fork();
	if (pid < 0)
	{
		fprintf(stderr, "fork: %s\n", strerror(errno));
		return -1;
	}

	if (pid > 0)
	{
		FILE *fp = fopen(pidfile, "w");
		if (fp == NULL)
		{
			fprintf(stderr, "can not open pidfile: %s\n", pidfile);
		}
		else
		{
			fprintf(fp, "%d", pid);
			fclose(fp);
		}
		exit(EXIT_SUCCESS);
	}

	umask(0);

	if (setsid() < 0)
	{
		fprintf(stderr, "setsid: %s\n", strerror(errno));
		return -1;
	}

	fclose(stdin);
	FILE *fp;
	fp = freopen(logfile, "w", stdout);
	if (fp == NULL)
	{
		fprintf(stderr, "freopen: %s\n", strerror(errno));
		return -1;
	}
	fp = freopen(logfile, "w", stderr);
	if (fp == NULL)
	{
		fprintf(stderr, "freopen: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int shell(const char *script)
{
	return system(script);
}
