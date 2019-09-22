/*
 * openurl.c
 *
 * by Jørn Thyssen <jthyssen@dk.ibm.com>, 2002.
 * (after inspiration from osr.cc from fibs2html <fibs2html.sourceforge.net>
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
 * $Id: openurl.c,v 1.28 2013/07/22 23:04:00 mdpetch Exp $
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "backgammon.h"
#include "openurl.h"
#ifdef WIN32
#include "windows.h"
#include "shellapi.h"
#else
#include <string.h>
#endif                          /* WIN32 */
static gchar *web_browser = NULL;

extern const gchar *
get_web_browser(void)
{
    const gchar *pch;
#ifdef WIN32
    if (!web_browser || !*web_browser)
        return ("");
#endif
    if (web_browser && *web_browser)
        return web_browser;
    if ((pch = g_getenv("BROWSER")) == NULL) {
#ifdef __APPLE__
        pch = "open";
#else
        pch = OPEN_URL_PROG;
#endif
    }
    return pch;
}


extern char *
set_web_browser(const char *sz)
{
    g_free(web_browser);
    web_browser = g_strdup(sz ? sz : "");
    return web_browser;
}

extern void
OpenURL(const char *szURL)
{
    const gchar *browser = get_web_browser();
    gchar *commandString;
    GError *error = NULL;
    if (!(browser) || !(*browser)) {
#ifdef WIN32
        int win_error;
        gchar *url = g_filename_to_uri(szURL, NULL, NULL);
        win_error = (int) ShellExecute(NULL, TEXT("open"), url ? url : szURL, NULL, ".\\", SW_SHOWNORMAL);
        if (win_error < 33)
            outputerrf(_("Failed to perform default action on " "%s. Error code was %d"), url, win_error);
        g_free(url);
        return;
#endif
    }
    commandString = g_strdup_printf("'%s' '%s'", browser, szURL);
    if (!g_spawn_command_line_async(commandString, &error)) {
        outputerrf(_("Browser couldn't open file (%s): %s\n"), commandString, error->message);
        g_error_free(error);
    }
    return;
}
