/*
 * output.c
 *
 * by Gary Wong <gtw@gnu.org>, 1998, 1999, 2000, 2001, 2002, 2003.
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
 * $Id: output.c,v 1.4 2014/07/29 15:19:44 plm Exp $
 */

#include "config.h"
#include "output.h"
#include "backgammon.h"

#include <sys/types.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#ifdef WIN32
#include <io.h>
#endif

#if USE_GTK_UNDEF
#undef USE_GTK
#endif

#if USE_GTK
#include "gtkgame.h"
#endif

int cOutputDisabled;
int cOutputPostponed;
int foutput_on;

extern void
output_initialize(void)
{
    cOutputDisabled = FALSE;
    cOutputPostponed = FALSE;
    foutput_on = TRUE;
}

/* Write a string to stdout/status bar/popup window */
extern void
output(const char *sz)
{

    if (cOutputDisabled || !foutput_on)
        return;

#if USE_GTK
    if (fX) {
        GTKOutput(sz);
        return;
    }
#endif
    fprintf(stdout, "%s", sz);
    if (!isatty(STDOUT_FILENO))
        fflush(stdout);

}

/* Write a string to stdout/status bar/popup window, and append \n */
extern void
outputl(const char *sz)
{


    if (cOutputDisabled || !foutput_on)
        return;

#if USE_GTK
    if (fX) {
        char *szOut = g_strdup_printf("%s\n", sz);
        GTKOutput(szOut);
        g_free(szOut);
        return;
    }
#endif
    g_print("%s\n", sz);
    if (!isatty(STDOUT_FILENO))
        fflush(stdout);
}

/* Write a character to stdout/status bar/popup window */
extern void
outputc(const char ch)
{

    char sz[2];
    sz[0] = ch;
    sz[1] = '\0';

    output(sz);
}

/* Write a string to stdout/status bar/popup window, printf style */
extern void
outputf(const char *sz, ...)
{

    va_list val;

    va_start(val, sz);
    outputv(sz, val);
    va_end(val);
}

/* Write a string to stdout/status bar/popup window, vprintf style */
extern void
outputv(const char *sz, va_list val)
{

    char *szFormatted;
    if (cOutputDisabled || !foutput_on)
        return;
    szFormatted = g_strdup_vprintf(sz, val);
    output(szFormatted);
    g_free(szFormatted);
}

/* Write an error message, perror() style */
extern void
outputerr(const char *sz)
{

    /* FIXME we probably shouldn't convert the charset of strerror() - yuck! */

    outputerrf("%s: %s", sz, strerror(errno));
}

/* Write an error message, fprintf() style */
extern void
outputerrf(const char *sz, ...)
{

    va_list val;

    va_start(val, sz);
    outputerrv(sz, val);
    va_end(val);
}

/* Write an error message, vfprintf() style */
extern void
outputerrv(const char *sz, va_list val)
{

    char *szFormatted;
    szFormatted = g_strdup_vprintf(sz, val);

#if USE_GTK
    if (fX)
        GTKOutputErr(szFormatted);
#endif
    fprintf(stderr, "%s", szFormatted);
    if (!isatty(STDOUT_FILENO))
        fflush(stdout);
    putc('\n', stderr);
    g_free(szFormatted);
}

/* Signifies that all output for the current command is complete */
extern void
outputx(void)
{

    if (cOutputDisabled || cOutputPostponed || !foutput_on)
        return;

#if USE_GTK
    if (fX)
        GTKOutputX();
#endif
}

/* Signifies that subsequent output is for a new command */
extern void
outputnew(void)
{

    if (cOutputDisabled || !foutput_on)
        return;

#if USE_GTK
    if (fX)
        GTKOutputNew();
#endif
}

/* Disable output */
extern void
outputoff(void)
{

    cOutputDisabled++;
}

/* Enable output */
extern void
outputon(void)
{

    g_assert(cOutputDisabled);

    cOutputDisabled--;
}

/* Temporarily disable outputx() calls */
extern void
outputpostpone(void)
{

    cOutputPostponed++;
}

/* Re-enable outputx() calls */
extern void
outputresume(void)
{

    g_assert(cOutputPostponed);

    if (!--cOutputPostponed) {
        outputx();
    }
}


