/*
 * export.h
 *
 * by Jørn Thyssen  <jthyssen@dk.ibm.com>, 2002.
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
 * $Id: export.h,v 1.38 2013/06/16 02:16:11 mdpetch Exp $
 */

#include "render.h"
#include "list.h"
#include <glib.h>

#ifndef EXPORT_H
#define EXPORT_H

#define EXPORT_CUBE_ACTUAL   7
#define EXPORT_CUBE_MISSED   8
#define EXPORT_CUBE_CLOSE    9


typedef enum _htmlexporttype {
    HTML_EXPORT_TYPE_GNU,
    HTML_EXPORT_TYPE_BBS,
    HTML_EXPORT_TYPE_FIBS2HTML,
    NUM_HTML_EXPORT_TYPES
} htmlexporttype;

typedef enum _htmlexportcss {
    HTML_EXPORT_CSS_HEAD,
    HTML_EXPORT_CSS_INLINE,
    HTML_EXPORT_CSS_EXTERNAL,
    NUM_HTML_EXPORT_CSS
} htmlexportcss;

extern const char *aszHTMLExportType[];
extern const char *aszHTMLExportCSS[];
extern const char *aszHTMLExportCSSCommand[];

typedef struct _exportsetup {

    int fIncludeAnnotation;
    int fIncludeAnalysis;
    int fIncludeStatistics;
    int fIncludeMatchInfo;

    /* display board: 0 (never), 1 (every move), 2 (every second move) etc */

    int fDisplayBoard;

    int fSide;                  /* 0, 1, or -1 for both players */

    /* moves */

    unsigned int nMoves;        /* show at most nMoves */
    int fMovesDetailProb;       /* show detailed probabilities */
    int afMovesParameters[2];   /* detailed parameters */
    int afMovesDisplay[7];      /* display moves */

    /* cube */

    int fCubeDetailProb;        /* show detailed probabilities */
    int afCubeParameters[2];    /* detailed parameters */
    int afCubeDisplay[10];      /* display moves */

    /* FIXME: add format specific options */

    /* For example, frames/non frames for HTML. */

    char *szHTMLPictureURL;
    htmlexporttype het;
    char *szHTMLExtension;
    htmlexportcss hecss;

    /* sizes */
    int nPNGSize;
    int nHtmlSize;

} exportsetup;

extern exportsetup exsExport;

extern char *filename_from_iGame(const char *szBase, const int iGame);
extern int WritePNG(const char *sz, unsigned char *puch,
                    unsigned int nStride, unsigned int nSizeX, unsigned int nSizeY);

#if USE_BOARD3D
void GenerateImage3d(const char *szName, unsigned int nSize, unsigned int nSizeX, unsigned int nSizeY);
#endif

extern void TextAnalysis(GString * gsz, const matchstate * pms, moverecord * pmr);
extern void TextPrologue(GString * gsz, const matchstate * pms, const int iGame);
extern void TextBoardHeader(GString * gsz, const matchstate * pms, const int iGame, const int iMove);
#endif
