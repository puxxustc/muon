/*
 * timer.h - simple timer
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

#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include "log.h"
#include "timer.h"

#define TIMER_MAX 4

static int count = 0;

static struct
{
    int interval;   // 时间间隔
    void (*cb)();   // 回调函数
    uint64_t last;  // 定时器上次触发时间
} timer[TIMER_MAX];


uint64_t timer_now(void)
{
#ifdef CLOCK_MONOTONIC_COARSE
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}


int timer_set(void (*cb)(), int interval)
{
    uint64_t now = timer_now();
    if (count >= TIMER_MAX)
    {
        return -1;
    }
    else
    {
        timer[count].interval = interval;
        timer[count].cb = cb;
        timer[count].last = now;
        count++;
        return 0;
    }
}


void timer_tick(void)
{
    uint64_t now = timer_now();
    for (int i = 0; i < count; i++)
    {
        if (now >= timer[i].last + timer[i].interval)
        {
            timer[i].last = now;
            (timer[i].cb)();
        }
    }
}
