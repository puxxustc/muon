/*
 * log.c
 *
 * Copyright (C) 2014 - 2016, Xiaoxiao <i@xiaoxiao.im>
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log.h"

void __log(FILE *stream, const char *format, ...)
{
    time_t now = time(NULL);
    char timestr[20];
    strftime(timestr, 20, "%y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(stream, "[%s] ", timestr);

    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
    putchar('\n');
    fflush(stream);
}

void __err(const char *msg)
{
    __log(stderr, "%s: %s", msg, strerror(errno));
}

