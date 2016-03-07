/*
 * log.h
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

#ifndef LOG_H
#define LOG_H


#include <stdio.h>
extern void __log(FILE *stream, const char *format, ...);
extern void __err(const char *msg);
#define LOG(format, ...)  do{__log(stdout, format, ##__VA_ARGS__);}while(0)
#define ERROR(msg)  do{__err(msg);}while(0)


#endif // LOG_H
