/*
 * util.c
 *
 * by Christian Anthon 2007
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
 * $Id: util.c,v 1.36 2015/02/08 15:44:43 plm Exp $
 */

#include "config.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "common.h"

char *prefsdir = NULL;
char *datadir = NULL;
char *pkg_datadir = NULL;
char *docdir = NULL;

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>

extern void
PrintSystemError(const char *message)
{
    LPVOID lpMsgBuf;
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) & lpMsgBuf, 0, NULL) != 0) {
        g_print("** Windows error while %s **\n", message);
        g_print(": %s", (LPCTSTR) lpMsgBuf);

        if (LocalFree(lpMsgBuf) != NULL)
            g_print("LocalFree() failed\n");
    }
}
#else
extern void
PrintSystemError(const char *message)
{
    printf("Unknown system error while %s!\n", message);
}
#endif

extern char *
getDataDir(void)
{
    if (!datadir) {
#ifndef WIN32
        datadir = g_strdup(AC_DATADIR);
#else
        char buf[FILENAME_MAX];
        if (GetModuleFileName(NULL, buf, sizeof(buf)) != 0) {
            char *p1 = strrchr(buf, '/'), *p2 = strrchr(buf, '\\');
            int pos1 = (p1 != NULL) ? (int) (p1 - buf) : -1;
            int pos2 = (p2 != NULL) ? (int) (p2 - buf) : -1;
            int pos = MAX(pos1, pos2);
            if (pos > 0)
                buf[pos] = '\0';
            datadir = g_strdup(buf);
        }
#endif
    }
    return datadir;
}

extern char *
getPkgDataDir(void)
{
    if (!pkg_datadir)
#ifndef WIN32
        pkg_datadir = g_strdup(AC_PKGDATADIR);
#else
        pkg_datadir = g_build_filename(getDataDir(), NULL);
#endif
    return pkg_datadir;
}

extern char *
getDocDir(void)
{
    if (!docdir)
#ifndef WIN32
        docdir = g_strdup(AC_DOCDIR);
#else
        docdir = g_build_filename(getDataDir(), "doc", NULL);
#endif
    return docdir;
}


void
PrintError(const char *str)
{
    g_printerr("%s: %s", str, strerror(errno));
}

/* Non-Ansi compliant function */
#ifdef __STRICT_ANSI__
FILE *fdopen(int, const char *);
#endif

/* Temporary version of g_file_open_tmp() as glib version doesn't work
 * although this code is copied from glib... */
#ifndef WIN32
#define TEMP_g_file_open_tmp g_file_open_tmp
#else
int
TEMP_g_mkstemp(char *tmpl)
{
    int count, fd;
    static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const int NLETTERS = sizeof(letters) - 1;
    glong value;
    GTimeVal tv;
    static int counter = 0;
    /* This is where the Xs start.  */
    char *XXXXXX = &tmpl[strlen(tmpl) - 6];

    /* Get some more or less random data.  */
    g_get_current_time(&tv);
    value = (tv.tv_usec ^ tv.tv_sec) + counter++;

    for (count = 0; count < 100; value += 7777, ++count) {
        glong v = value;

        /* Fill in the random bits.  */
        XXXXXX[0] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[1] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[2] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[3] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[4] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[5] = letters[v % NLETTERS];

        fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL | O_BINARY, 0600);

        if (fd >= 0)
            return fd;
        else if (errno != EEXIST)
            /* Any other error will apply also to other names we might
             *  try, and there are 2^32 or so of them, so give up now.
             */
            return -1;
    }

    /* We got out of the loop because we ran out of combinations to try.  */
    return -1;
}

int
TEMP_g_file_open_tmp(const char *tmpl, char **name_used, GError ** UNUSED(pError))
{
    const char *sep = "";
    char test;
    const char *tmpdir = g_get_tmp_dir();

    if (tmpl == NULL)
        tmpl = ".XXXXXX";

    test = tmpdir[strlen(tmpdir) - 1];
    if (!G_IS_DIR_SEPARATOR(test))
        sep = G_DIR_SEPARATOR_S;

    *name_used = g_strconcat(tmpdir, sep, tmpl, NULL);
    return TEMP_g_mkstemp(*name_used);
}

/* End of temporary copy of glib code, remove when glib version works on windows... */
#endif

extern FILE *
GetTemporaryFile(const char *nameTemplate, char **retName)
{
    FILE *pf;
    int tmpd = TEMP_g_file_open_tmp(nameTemplate, retName, NULL);

    if (tmpd < 0) {
        PrintError("creating temporary file");
        return NULL;
    }

    pf = fdopen(tmpd, "wb+");

    if (pf == NULL) {
        g_free(retName);
        PrintError("opening temporary file");
        return NULL;
    } else
        return pf;
}
