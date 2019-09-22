/*
 * timer.c
 * by Jon Kinsey, 2003
 *
 * Accurate win32 timer or default timer
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 3 or later of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: timer.c,v 1.17 2013/06/16 02:16:21 mdpetch Exp $
 */

#include "config.h"
#include <time.h>
#include <backgammon.h>
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef WIN32
#include "windows.h"

static double perFreq = 0;

static int
setup_timer()
{
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) {    /* Timer not supported */
        return 0;
    } else {
        perFreq = ((double) freq.QuadPart) / 1000;
        return 1;
    }
}

double
get_time()
{                               /* Return elapsed time in milliseconds */
    LARGE_INTEGER timer;

    if (!perFreq) {
        if (!setup_timer())
            return clock() / 1000.0;
    }

    QueryPerformanceCounter(&timer);
    return timer.QuadPart / perFreq;
}

#else

extern double
get_time(void)
{                               /* Return elapsed time in milliseconds */
    struct timeval tv;
    gettimeofday(&tv, 0);

    return 1000.0 * tv.tv_sec + 0.001 * tv.tv_usec;
}

#endif
