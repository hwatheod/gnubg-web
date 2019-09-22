/*
 * file.c
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
 * $Id: file.c,v 1.31 2014/03/09 23:34:17 plm Exp $
 */

#include "config.h"
#include "backgammon.h"
#include <glib/gstdio.h>
#include "file.h"
#include <stdlib.h>

ExportFormat export_format[] = {
    {EXPORT_SGF, ".sgf", N_("GNU Backgammon File"), "sgf", {TRUE, TRUE, TRUE}
     }
    ,                           /*must be the first element */
    {EXPORT_HTML, ".html", "HTML", "html", {TRUE, TRUE, TRUE}
     }
    ,
    {EXPORT_GAM, ".gam", N_("Jellyfish Game"), "gam", {FALSE, TRUE, FALSE}
     }
    ,
    {EXPORT_MAT, ".mat", N_("Jellyfish Match"), "mat", {TRUE, FALSE, FALSE}
     }
    ,
    {EXPORT_POS, ".pos", N_("Jellyfish Position"), "pos", {FALSE, FALSE, TRUE}
     }
    ,
    {EXPORT_LATEX, ".tex", "LaTeX", "latex", {TRUE, TRUE, FALSE}
     }
    ,
#if HAVE_PANGOCAIRO
    {EXPORT_PDF, ".pdf", "PDF", "pdf", {TRUE, TRUE, TRUE}
     }
    ,
#endif
    {EXPORT_TEXT, ".txt", N_("Plain Text"), "text", {TRUE, TRUE, TRUE}
     }
    ,
#if HAVE_LIBPNG
    {EXPORT_PNG, ".png", "PNG", "png", {FALSE, FALSE, TRUE}
     }
    ,
#endif
#if HAVE_PANGOCAIRO
    {EXPORT_PS, ".ps", "PostScript", "ps", {TRUE, TRUE, TRUE}
     }
    ,
#endif
    {EXPORT_SNOWIETXT, ".txt", N_("Snowie Text"), "snowietxt", {TRUE, TRUE, TRUE}
     }
    ,
#if HAVE_PANGOCAIRO
    {EXPORT_SVG, ".svg", "SVG", "svg", {FALSE, FALSE, TRUE}
     }
    ,
#endif
};

ImportFormat import_format[] = {
    {IMPORT_SGF, ".sgf", N_("GNU Backgammon File"), "sgf"}
    ,                           /*must be the first element */
    {IMPORT_SGG, ".sgg", N_("GamesGrid Save Game"), "sgg"}
    ,
    {IMPORT_MAT, ".mat", N_("Jellyfish Match"), "mat"}
    ,
    {IMPORT_OLDMOVES, ".fibs", N_("FIBS oldmoves format"), "oldmoves"}
    ,
    {IMPORT_POS, ".pos", N_("Jellyfish Position"), "pos"}
    ,
    {IMPORT_SNOWIETXT, ".txt", N_("Snowie Text"), "snowietxt"}
    ,
    {IMPORT_TMG, ".tmg", N_("TrueMoneyGames"), "tmg"}
    ,
    {IMPORT_EMPIRE, ".gam", N_("GammonEmpire Game"), "empire"}
    ,
    {IMPORT_PARTY, ".gam", N_("PartyGammon Game"), "party"}
    ,
    {IMPORT_BGROOM, ".bgf", N_("BGRoom Game"), "bgroom"}
    ,
    {N_IMPORT_TYPES, NULL, N_("Unknown file format"), NULL}
};

typedef struct _FileHelper {
    FILE *fp;
    size_t dataRead;
    unsigned int dataPos;
    char *data;
} FileHelper;

/* Data structures and functions for getting file type data */

static FileHelper *
OpenFileHelper(const char *filename)
{
    FileHelper *fh;
    if (!filename || !g_file_test(filename, G_FILE_TEST_EXISTS) || g_file_test(filename, G_FILE_TEST_IS_DIR))
        return NULL;            /* File not found */

    fh = g_new(FileHelper, 1);
    fh->fp = g_fopen(filename, "r");
    if (!fh->fp) {              /* Failed to open file */
        g_free(fh);
        return NULL;
    }
    fh->dataRead = 0;
    fh->dataPos = 0;
    fh->data = NULL;
    return fh;
}

static void
CloseFileHelper( /*lint -e{818} */ FileHelper * fh)
{
    fclose(fh->fp);
    free(fh->data);
}

static void
fhReset(FileHelper * fh)
{                               /* Reset data pointer to start of file */
    fh->dataPos = 0;
}

static void
fhDataGetChar(FileHelper * fh)
{
    size_t numRead;
    if (fh->dataPos < fh->dataRead)
        return;

#define BLOCK_SIZE 1024
#define MAX_READ_SIZE 5000
    fh->data = realloc(fh->data, fh->dataRead + BLOCK_SIZE);
    if (fh->dataRead > MAX_READ_SIZE)
        numRead = 0;            /* Too big - should have worked things out by now! */
    else
        numRead = fread(fh->data + fh->dataRead, 1, BLOCK_SIZE, fh->fp);
    if (numRead < BLOCK_SIZE) {
        (fh->data + fh->dataRead)[numRead] = '\0';
        numRead++;
    }
    fh->dataRead += numRead;
}

static char
fhPeekNextChar(FileHelper * fh)
{
    fhDataGetChar(fh);
    return fh->data[fh->dataPos];
}

static char
fhReadNextChar(FileHelper * fh)
{
    fhDataGetChar(fh);
    return fh->data[fh->dataPos++];
}

static int
fhPeekNextIsWS(FileHelper * fh)
{
    char c = fhPeekNextChar(fh);
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

static void
fhSkipWS(FileHelper * fh)
{
    while (fhPeekNextIsWS(fh))
        (void) fhReadNextChar(fh);
}

static int
fhSkipToEOL(FileHelper * fh)
{
    char c;
    do {
        c = fhReadNextChar(fh);
        if (c == '\n')
            return TRUE;
    } while (c != '\0');

    return FALSE;
}

static int
fhReadString(FileHelper * fh, const char *str)
{                               /* Check file has str next */
    while (*str) {
        if (fhReadNextChar(fh) != *str)
            return FALSE;
        str++;
    }
    return TRUE;
}

static int
fhReadStringNC(FileHelper * fh, const char *str)
{                               /* Check file has str next (ignoring case) */
    while (*str) {
        char c = fhReadNextChar(fh);
        if (g_ascii_tolower(c) != *str)
            return FALSE;
        str++;
    }
    return TRUE;
}

static int
fhPeekStringNC(FileHelper * fh, const char *str)
{                               /* Check file has str next (ignoring case) but don't move */
    unsigned int pos = fh->dataPos;
    int ret = TRUE;
    while (*str) {
        char c = fhReadNextChar(fh);
        if (g_ascii_tolower(c) != *str) {
            ret = FALSE;
            break;
        }
        str++;
    }
    fh->dataPos = pos;
    return ret;
}

static int
fhReadNumber(FileHelper * fh)
{                               /* Check file has str next */
    int anyNumbers = FALSE;
    do {
        char c = fhPeekNextChar(fh);
        if (!g_ascii_isdigit(c))
            return anyNumbers;
        anyNumbers = TRUE;
    } while (fhReadNextChar(fh) != '\0');

    return TRUE;
}

static int
fhReadAnyAlphNumString(FileHelper * fh)
{
    char c = fhPeekNextChar(fh);
    if (!g_ascii_isalnum(c))
        return FALSE;
    do {
        c = fhPeekNextChar(fh);
        if (!g_ascii_isalnum(c) && c != '_')
            return fhPeekNextIsWS(fh);
    } while (fhReadNextChar(fh) != '\0');
    return TRUE;
}

static int
IsSGFFile(FileHelper * fh)
{
    const char *elements[] = { "(", ";", "FF", "[", "4", "]", "GM", "[", "6", "]", "" };
    const char **test = elements;

    fhReset(fh);
    while (**test) {
        fhSkipWS(fh);
        if (!fhReadString(fh, *test))
            return FALSE;
        test++;
    }
    return TRUE;
}

static int
IsSGGFile(FileHelper * fh)
{
    fhReset(fh);
    fhSkipWS(fh);

    if (fhReadAnyAlphNumString(fh)) {
        fhSkipWS(fh);
        if (!fhReadString(fh, "vs."))
            return FALSE;
        fhSkipWS(fh);
        if (fhReadAnyAlphNumString(fh))
            return TRUE;
    }
    return FALSE;
}

static int
IsMATFile(FileHelper * fh)
{
    fhReset(fh);
    do {
        char c;
        fhSkipWS(fh);
        c = fhPeekNextChar(fh);

        /* XG-style comment at the top of a mat file.
         * Backgammon NJ can add so many of them that the real MAT pattern
         * further below is pushed beyond the horizon of our probing. */
        /* FIXME ? Check for all comments (';' and anything after that) */
        if (c == ';') {
            fhReadNextChar(fh);
            if (fhReadNextChar(fh) == ' ' && fhReadNextChar(fh) == '[')
                return TRUE;
        } else if (g_ascii_isdigit(c)) {
            if (fhReadNumber(fh)) {
                fhSkipWS(fh);
                if (fhReadStringNC(fh, "point")) {
                    fhSkipWS(fh);
                    if (fhReadStringNC(fh, "match"))
                        return TRUE;
                }
            }
            return FALSE;
        }
    } while (fhSkipToEOL(fh));
    return FALSE;
}

static int
IsTMGFile(FileHelper * fh)
{
    fhReset(fh);
    do {
        fhSkipWS(fh);
        if (fhPeekStringNC(fh, "game")) {
            (void) fhReadStringNC(fh, "game");
            fhSkipWS(fh);
            return fhReadNumber(fh)
                && (fhPeekNextChar(fh) == ':');
        }
        (void) fhReadAnyAlphNumString(fh);
        if (fhPeekNextChar(fh) != ':')
            return FALSE;
    } while (fhSkipToEOL(fh));

    return FALSE;
}

static int
IsTXTFile(FileHelper * fh)
{
    fhReset(fh);
    fhSkipWS(fh);
    if (fhReadNumber(fh) && fhReadNextChar(fh) == ';') {
        fhSkipWS(fh);
        if (fhReadNumber(fh) && fhReadNextChar(fh) == ';')
            return TRUE;
    }
    return FALSE;
}

static int
IsJFPFile(FileHelper * fh)
{
    char firstbyte;

    fhReset(fh);

    firstbyte = fhReadNextChar(fh);
    if (firstbyte < 124 || firstbyte > 126)
        return FALSE;

    if ((fhReadNextChar(fh) == '\0'))
        return TRUE;
    else
        return FALSE;
}

static int
IsGAMFile(FileHelper * fh)
{
    fhReset(fh);
    fhSkipWS(fh);

    if (fhReadAnyAlphNumString(fh)) {
        fhSkipWS(fh);
        if (fhReadAnyAlphNumString(fh)) {
            fhSkipWS(fh);
            return fhReadString(fh, "1)");
        }
    }

    return FALSE;
}

static int
IsPARFile(FileHelper * fh)
{
    fhReset(fh);
    fhSkipWS(fh);

    if (fhReadStringNC(fh, "boardid=")) {
        (void) fhSkipToEOL(fh);
        if (fhReadStringNC(fh, "creator="))
            return TRUE;
    }

    return FALSE;
}

static int
IsBGRFile(FileHelper * fh)
{
    fhReset(fh);
    fhSkipWS(fh);

    if (fhReadStringNC(fh, "bgf version")) {
        fhSkipWS(fh);
        if (!fhReadString(fh, "1.2"))
            printf("Unexpected version of bgroom file!\n");

        return TRUE;
    }

    return FALSE;
}

extern FilePreviewData *
ReadFilePreview(const char *filename)
{
    FilePreviewData *fpd;
    FileHelper *fh = OpenFileHelper(filename);
    if (!fh)
        return NULL;

    fpd = g_new0(FilePreviewData, 1);
    fpd->type = N_IMPORT_TYPES;

    if (IsSGFFile(fh))
        fpd->type = IMPORT_SGF;
    else if (IsSGGFile(fh))
        fpd->type = IMPORT_SGG;
    else if (IsTXTFile(fh))
        fpd->type = IMPORT_SNOWIETXT;
    else if (IsTMGFile(fh))
        fpd->type = IMPORT_TMG;
    else if (IsMATFile(fh))
        fpd->type = IMPORT_MAT;
    else if (IsJFPFile(fh))
        fpd->type = IMPORT_POS;
    else if (IsGAMFile(fh))
        fpd->type = IMPORT_EMPIRE;
    else if (IsPARFile(fh))
        fpd->type = IMPORT_PARTY;
    else if (IsBGRFile(fh))
        fpd->type = IMPORT_BGROOM;

    CloseFileHelper(fh);
    return fpd;
}

extern char *
GetFilename(int CheckForCurrent, ExportType type)
{
    char *sz;
    time_t t;

    if (CheckForCurrent && szCurrentFileName && *szCurrentFileName)
        sz = g_strdup_printf("%s%s", szCurrentFileName, export_format[type].extension);
    else {
        char tstr[15];

        if (mi.nYear)
            sprintf(tstr, "%04u-%02u-%02u", mi.nYear, mi.nMonth, mi.nDay);
        else {
            t = time(NULL);
            if (strftime(tstr, 14, "%Y-%m-%d-%H%M", localtime(&t)) == 0)
                *tstr = '\0';
        }
        sz = g_strdup_printf("%s-%s_%dp_%s.sgf", ap[0].szName, ap[1].szName, ms.nMatchTo, tstr);
    }
    return sz;
}
