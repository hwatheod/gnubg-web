/*
 * html.c
 *
 * by Joern Thyssen  <jthyssen@dk.ibm.com>, 2002
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
 * $Id: html.c,v 1.235 2014/02/12 21:12:36 plm Exp $
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "analysis.h"
#include "backgammon.h"
#include "drawboard.h"
#include "format.h"
#include "export.h"
#include "eval.h"
#include "positionid.h"
#include "matchid.h"
#include "formatgs.h"
#include "relational.h"

#include <glib.h>
#include <glib/gstdio.h>
#ifdef WIN32
#include <io.h>
#endif
#include "util.h"

typedef enum _stylesheetclass {
    CLASS_MOVETABLE,
    CLASS_MOVEHEADER,
    CLASS_MOVENUMBER,
    CLASS_MOVEPLY,
    CLASS_MOVEMOVE,
    CLASS_MOVEEQUITY,
    CLASS_MOVETHEMOVE,
    CLASS_MOVEODD,
    CLASS_BLUNDER,
    CLASS_JOKER,
    CLASS_STATTABLE,
    CLASS_STATTABLEHEADER,
    CLASS_RESULT,
    CLASS_TINY,
    CLASS_CUBEDECISION,
    CLASS_CUBEDECISIONHEADER,
    CLASS_COMMENT,
    CLASS_COMMENTHEADER,
    CLASS_NUMBER,
    CLASS_FONT_FAMILY,
    CLASS_BLOCK,
    CLASS_PERCENT,
    CLASS_POSITIONID,
    CLASS_CUBE_EQUITY,
    CLASS_CUBE_ACTION,
    CLASS_CUBE_PLY,
    CLASS_CUBE_PROBABILITIES,
    CLASS_CUBE_CUBELESS_TEXT,
    CLASS_BOARD_IMG_HEADER,
    CLASS_BOARD_IMG,
    NUM_CLASSES
} stylesheetclass;

static const char *aaszStyleSheetClasses[NUM_CLASSES][2] = {
    {"movetable", "background-color: #ddddee"},
    {"moveheader", "background-color: #89d0e2; padding: 0.5em"},
    {"movenumber", "width: 2em; text-align: right"},
    {"moveply", "width: 5em; text-align: center"},
    {"movemove", "width: 20em; text-align: left"},

    {"moveequity", "width: 10em; text-align: left"},
    {"movethemove", "background-color: #ffffcc"},
    {"moveodd", "background-color: #d0d0d0"},
    {"blunder", "background-color: red; color: yellow"},
    {"joker", "background-color: red; color: yellow"},

    {"stattable",
     "text-align: left; width: 40em; background-color: #fff2cc; " "border: 0px; padding: 0px"},
    {"stattableheader", "background-color: #d15b34"},
    {"result",
     "background-color: yellow; font-weight: bold; text-align: center; " "color: black; width: 40em; padding: 0.2em"},
    {"tiny", "font-size: 25%"},
    {"cubedecision", "background-color: #ddddee; text-align: left;"},
    {"cubedecisionheader",
     "background-color: #89d0e2; text-align: center; padding: 0.5em"},
    {"comment", "background-color: #ccffcc; width: 39.5em; padding: 0.5em"},
    {"commentheader",
     "background-color: #6f9915; font-weight: bold; text-align: center; " "width: 40em; padding: 0.25em"},
    {"number",
     "text-align: center; font-weight: bold; font-size: 60%; " "font-family: sans-serif"},
    {"fontfamily", "font-family: sans-serif"},
    {"block", "display: block"},
    {"percent", "text-align: right"},
    {"positionid", "font-size: 75%; color: #787878"},
    {"cubeequity", "font-weight: bold"},
    {"cubeaction", "color: red"},
    {"cubeply", "font-weight: bold"},
    {"cubeprobs", "font-weight: bold"},
    {"cubecubelesstext", "font-style: italic"},
    {"boardimagetop", "vertical-align: bottom"},
    {"boardimage", "vertical-align: top"}
};

const char *aszHTMLExportType[NUM_HTML_EXPORT_TYPES] = {
    "gnu",
    "bbs",
    "fibs2html"
};

const char *aszHTMLExportCSS[NUM_HTML_EXPORT_CSS] = {
    N_("in <head>"),
    N_("inline (inside tags)"),
    N_("external file (\"gnubg.css\")")
};

const char *aszHTMLExportCSSCommand[NUM_HTML_EXPORT_CSS] = {
    "head", "inline", "external"
};



/* text for links on html page */

static const char *aszLinkText[] = {
    N_("[First Game]"),
    N_("[Previous Game]"),
    N_("[Next Game]"),
    N_("[Last Game]")
};

static const char *bullet = "&bull; ";

static void
WriteStyleSheet(FILE * pf, const htmlexportcss hecss)
{

    int i;

    if (hecss == HTML_EXPORT_CSS_HEAD)
        fputs("<style type=\"text/css\">\n", pf);
    else if (hecss == HTML_EXPORT_CSS_EXTERNAL)
        /* write come comments in the file */

        fputs("\n"
              "/* CSS Stylesheet for " VERSION_STRING " */\n"
              "/* $Id: html.c,v 1.235 2014/02/12 21:12:36 plm Exp $ */\n", pf);

    fputs("/* This file is distributed as a part of the "
          "GNU Backgammon program. */\n"
          "/* Copying and distribution of verbatim and modified "
          "versions of this file */\n"
          "/* is permitted in any medium provided the copyright notice "
          "and this */\n" "/* permission notice are preserved. */\n\n", pf);

    for (i = 0; i < NUM_CLASSES; ++i)
        fprintf(pf, ".%s { %s }\n", aaszStyleSheetClasses[i][0], aaszStyleSheetClasses[i][1]);


    if (hecss == HTML_EXPORT_CSS_HEAD)
        fputs("</style>\n", pf);
    else if (hecss == HTML_EXPORT_CSS_EXTERNAL)
        fputs("\n" "/* end of file */\n", pf);

}

static char *
GetStyle(const stylesheetclass ssc, const htmlexportcss hecss)
{

    static char sz[200];

    switch (hecss) {
    case HTML_EXPORT_CSS_INLINE:
        sprintf(sz, "style=\"%s\"", aaszStyleSheetClasses[ssc][1]);
        break;
    case HTML_EXPORT_CSS_EXTERNAL:
    case HTML_EXPORT_CSS_HEAD:
        sprintf(sz, "class=\"%s\"", aaszStyleSheetClasses[ssc][0]);
        break;
    default:
        strcpy(sz, "");
        break;
    }

    return sz;

}


static char *
GetStyleGeneral(const htmlexportcss hecss, ...)
{

    static char sz[2048];
    va_list val;
    stylesheetclass ssc;
    int i = 0;
    int j;

    va_start(val, hecss);

    switch (hecss) {
    case HTML_EXPORT_CSS_INLINE:
        strcpy(sz, "style=\"");
        break;
    case HTML_EXPORT_CSS_EXTERNAL:
    case HTML_EXPORT_CSS_HEAD:
        strcpy(sz, "class=\"");
        break;
    default:
        strcpy(sz, "");
        break;
    }

    while ((j = va_arg(val, int)) > -1) {

        ssc = (stylesheetclass) j;

        switch (hecss) {
        case HTML_EXPORT_CSS_INLINE:
            if (i)
                strcat(sz, "; ");
            strcat(sz, aaszStyleSheetClasses[ssc][1]);
            break;
        case HTML_EXPORT_CSS_EXTERNAL:
        case HTML_EXPORT_CSS_HEAD:
            if (i)
                strcat(sz, " ");
            strcat(sz, aaszStyleSheetClasses[ssc][0]);
            break;
        default:
            break;
        }

        ++i;

    }

    va_end(val);

    strcat(sz, "\"");

    return sz;

}


static void
printRolloutTable(FILE * pf,
                  char asz[][1024],
                  float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                  float aarStdDev[][NUM_ROLLOUT_OUTPUTS],
                  const cubeinfo aci[], const int cci, const int fCubeful, const int fHeader, const htmlexportcss hecss)
{

    int ici;

    fputs("<table>\n", pf);

    if (fHeader) {

        fputs("<tr>", pf);

        if (asz)
            fputs("<td>&nbsp;</td>", pf);

        fprintf(pf,
                "<td>%s</td>"
                "<td>%s</td>"
                "<td>%s</td>"
                "<td>&nbsp;</td>"
                "<td>%s</td>"
                "<td>%s</td>"
                "<td>%s</td>"
                "<td>%s</td>", _("Win"), _("W g"), _("W bg"), _("Lose"), _("L g"), _("L bg"), _("Cubeless"));

        if (fCubeful)
            fprintf(pf, "<td>%s</td>", _("Cubeful"));

        fputs("</tr>\n", pf);

    }

    for (ici = 0; ici < cci; ici++) {

        fputs("<tr>", pf);

        /* output */

        if (asz)
            fprintf(pf, "<td>%s</td>", asz[ici]);

        fprintf(pf, "<td %s>%s</td>", GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarOutput[ici][OUTPUT_WIN]));
        fprintf(pf, "<td %s>%s</td>", GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarOutput[ici][OUTPUT_WINGAMMON]));
        fprintf(pf, "<td %s>%s</td>",
                GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarOutput[ici][OUTPUT_WINBACKGAMMON]));

        fputs("<td>-</td>", pf);

        fprintf(pf, "<td %s>%s</td>", GetStyle(CLASS_PERCENT, hecss), OutputPercent(1.0f - aarOutput[ici][OUTPUT_WIN]));
        fprintf(pf, "<td %s>%s</td>", GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarOutput[ici][OUTPUT_LOSEGAMMON]));
        fprintf(pf, "<td %s>%s</td>",
                GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarOutput[ici][OUTPUT_LOSEBACKGAMMON]));

        fprintf(pf, "<td %s>%s</td>",
                GetStyle(CLASS_PERCENT, hecss),
                OutputEquityScale(aarOutput[ici][OUTPUT_EQUITY], &aci[ici], &aci[0], TRUE));

        if (fCubeful)
            fprintf(pf, "<td %s>%s</td>",
                    GetStyle(CLASS_PERCENT, hecss), OutputMWC(aarOutput[ici][OUTPUT_CUBEFUL_EQUITY], &aci[0], TRUE));

        fputs("</tr>\n", pf);

        /* stddev */

        fputs("<tr>", pf);

        if (asz)
            fprintf(pf, "<td>%s</td>", _("Standard error"));

        fprintf(pf, "<td %s>%s</td>", GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarStdDev[ici][OUTPUT_WIN]));
        fprintf(pf, "<td %s>%s</td>", GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarStdDev[ici][OUTPUT_WINGAMMON]));
        fprintf(pf, "<td %s>%s</td>",
                GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarStdDev[ici][OUTPUT_WINBACKGAMMON]));

        fputs("<td>-</td>", pf);

        fprintf(pf, "<td %s>%s</td>", GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarStdDev[ici][OUTPUT_WIN]));
        fprintf(pf, "<td %s>%s</td>", GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarStdDev[ici][OUTPUT_LOSEGAMMON]));
        fprintf(pf, "<td %s>%s</td>",
                GetStyle(CLASS_PERCENT, hecss), OutputPercent(aarStdDev[ici][OUTPUT_LOSEBACKGAMMON]));

        fprintf(pf, "<td %s>%s</td>",
                GetStyle(CLASS_PERCENT, hecss),
                OutputEquityScale(aarStdDev[ici][OUTPUT_EQUITY], &aci[ici], &aci[0], FALSE));

        if (fCubeful)
            fprintf(pf, "<td %s>%s</td>",
                    GetStyle(CLASS_PERCENT, hecss), OutputMWC(aarStdDev[ici][OUTPUT_CUBEFUL_EQUITY], &aci[0], FALSE));

        fputs("</tr>\n", pf);

    }

    fputs("</table>\n", pf);

}


static void
printStatTableHeader(FILE * pf, const htmlexportcss hecss, const char *header)
{

    fprintf(pf, "<tr %s>\n" "<th colspan=\"3\" style=\"text-align: center\">", GetStyle(CLASS_STATTABLEHEADER, hecss));
    fputs(header, pf);
    fputs("</th>\n</tr>\n", pf);

}


static void
printStatTableRow(FILE * pf, const char *format1, const char *format2, ...)
{

    va_list val;
    char *sz;
    size_t l = 100 + strlen(format1) + 2 * strlen(format2);

    va_start(val, format2);

    sprintf(sz = (char *) malloc(l),
            "<tr>\n" "<td>%s</td>\n" "<td>%s</td>\n" "<td>%s</td>\n" "</tr>\n", format1, format2, format2);

    vfprintf(pf, sz, val);

    free(sz);

    va_end(val);

}

/*
 * Print img tag.
 *
 * Input:
 *    pf : write to file
 *    szImageDir: path (URI) to images
 *    szImage: the image to print
 *    szExtension: extension of the image (e.g. gif or png)
 *
 */

static void
printImageClass(FILE * pf, const char *szImageDir, const char *szImage,
                const char *szExtension, const char *szAlt,
                const htmlexportcss hecss, const htmlexporttype het, const stylesheetclass ssc)
{

    fprintf(pf, "<img src=\"%s%s%s.%s\" %s alt=\"%s\" />",
            (szImageDir) ? szImageDir : "",
            (!szImageDir || szImageDir[strlen(szImageDir) - 1] == '/') ? "" : "/",
            szImage, szExtension,
            (het == HTML_EXPORT_TYPE_GNU || (het == HTML_EXPORT_TYPE_BBS && ssc != CLASS_BLOCK)) ?
            GetStyle(ssc, hecss) : "", (szAlt) ? szAlt : "");

}

/*
 * Print img tag.
 *
 * Input:
 *    pf : write to file
 *    szImageDir: path (URI) to images
 *    szImage: the image to print
 *    szExtension: extension of the image (e.g. gif or png)
 *
 */

static void
printImage(FILE * pf, const char *szImageDir, const char *szImage,
           const char *szExtension, const char *szAlt, const htmlexportcss hecss, const htmlexporttype het)
{

    printImageClass(pf, szImageDir, szImage, szExtension, szAlt, hecss, het, CLASS_BLOCK);

}


/*
 * print image for a point
 *
 * Input:
 *    pf : write to file
 *    szImageDir: path (URI) to images
 *    szImage: the image to print
 *    szExtension: extension of the image (e.g. gif or png)
 *    anBoard: the board
 *    iPoint: the point to draw
 *    fColor: color
 *    fUp: upper half or lower half of board
 *
 */

static void
printPointBBS(FILE * pf, const char *szImageDir, const char *szExtension,
              int iPoint0, int iPoint1, const int fColor, const int fUp, const htmlexportcss hecss)
{

    char sz[100];
    char szAlt[100];
    const char *aasz[2][2] = { {"dd", "dn"},
    {"ud", "up"}
    };

    if (iPoint0) {

        /* player 0 owns the point */

        sprintf(sz, "p_%s_w_%d", aasz[fUp][fColor], iPoint0);
        sprintf(szAlt, "%1xX", iPoint0);

    } else if (iPoint1) {

        /* player 1 owns the point */

        sprintf(sz, "p_%s_b_%d", aasz[fUp][fColor], iPoint1);
        sprintf(szAlt, "%1xO", iPoint1);

    } else {
        /* empty point */
        sprintf(sz, "p_%s_0", aasz[fUp][fColor]);
        sprintf(szAlt, "&nbsp;'");
    }

    printImageClass(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

}


static void
printHTMLBoardBBS(FILE * pf, matchstate * pms, int fTurn,
                  const char *szImageDir, const char *szExtension, const htmlexportcss hecss)
{

    TanBoard anBoard;
    unsigned int anPips[2];
    int acOff[2];
    int i, j;
    char sz[1024];

    memcpy(anBoard, pms->anBoard, sizeof(anBoard));

    if (pms->fMove)
        SwapSides(anBoard);
    PipCount((ConstTanBoard) anBoard, anPips);

    for (i = 0; i < 2; i++) {
        acOff[i] = 15;
        for (j = 0; j < 25; j++)
            acOff[i] -= anBoard[i][j];
    }

    PipCount((ConstTanBoard) anBoard, anPips);

    /* Begin table  and print for player 0 */
    fprintf(pf,
            "<table style=\"page-break-inside: avoid\"><tr><th align=\"left\">%s</th><th align=\"right\">%u</th></tr>",
            ap[0].szName, anPips[1]);

    /* avoid page break when printing */
    fputs("<tr><td align=\"center\" colspan=\"2\">", pf);

    /* 
     * Top row
     */

    printImageClass(pf, szImageDir, fTurn ? "n_high" : "n_low",
                    szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG_HEADER);
    fputs("<br />\n", pf);

    /* chequers off */

    sprintf(sz, "o_w_%d", acOff[1]);
    printImageClass(pf, szImageDir, sz, szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

    /* player 0's inner board */

    for (i = 0; i < 6; i++)
        printPointBBS(pf, szImageDir, szExtension, anBoard[1][i], anBoard[0][23 - i], !(i % 2), TRUE, hecss);

    /* player 0's chequers on the bar */

    sprintf(sz, "b_up_%d", anBoard[0][24]);
    printImageClass(pf, szImageDir, sz, szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

    /* player 0's outer board */

    for (i = 0; i < 6; i++)
        printPointBBS(pf, szImageDir, szExtension, anBoard[1][i + 6], anBoard[0][17 - i], !(i % 2), TRUE, hecss);

    /* player 0 owning cube */

    if (!pms->fCubeOwner) {
        sprintf(sz, "c_up_%d", pms->nCube);
        printImageClass(pf, szImageDir, sz, szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);
    } else
        printImageClass(pf, szImageDir, "c_up_0", szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

    fputs("<br />\n", pf);

    /* end of first row */

    /*
     * center row (dice)
     */

    if (pms->anDice[0]) {

        /* dice rolled */

        sprintf(sz, "b_center%d%d%s",
                (pms->anDice[0] < pms->anDice[1]) ?
                pms->anDice[0] : pms->anDice[1],
                (pms->anDice[0] < pms->anDice[1]) ? pms->anDice[1] : pms->anDice[0], pms->fMove ? "right" : "left");
        printImageClass(pf, szImageDir, sz, szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

    } else
        /* no dice rolled */
        printImageClass(pf, szImageDir, "b_center", szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

    /* center cube */

    if (pms->fCubeOwner == -1)
        printImageClass(pf, szImageDir, "c_center", szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);
    else
        printImageClass(pf, szImageDir, "c_blank", szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

    fputs("<br />\n", pf);

    /* end of center row */

    /*
     * Bottom row
     */

    /* player 1's chequers off */

    sprintf(sz, "o_b_%d", acOff[0]);
    printImageClass(pf, szImageDir, sz, szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

    /* player 1's inner board */

    for (i = 0; i < 6; i++)
        printPointBBS(pf, szImageDir, szExtension, anBoard[1][23 - i], anBoard[0][i], (i % 2), FALSE, hecss);

    /* player 1's chequers on the bar */

    sprintf(sz, "b_dn_%d", anBoard[1][24]);
    printImageClass(pf, szImageDir, sz, szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

    /* player 1's outer board */

    for (i = 0; i < 6; i++)
        printPointBBS(pf, szImageDir, szExtension, anBoard[1][17 - i], anBoard[0][i + 6], (i % 2), FALSE, hecss);

    /* player 1 owning cube */

    if (pms->fCubeOwner == 1) {
        sprintf(sz, "c_dn_%d", pms->nCube);
        printImageClass(pf, szImageDir, sz, szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);
    } else
        printImageClass(pf, szImageDir, "c_dn_0", szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

    fputs("<br />\n", pf);

    /* point numbers */

    printImageClass(pf, szImageDir, fTurn ? "n_low" : "n_high",
                    szExtension, NULL, hecss, HTML_EXPORT_TYPE_BBS, CLASS_BOARD_IMG);

    fputs("</td></tr>\n", pf);

    fprintf(pf,
            "<tr><th align=\"left\">%s</th><th align=\"right\">%u</th><th align=\"center\" colspan=\"2\"></th></tr>",
            ap[1].szName, anPips[0]);

    /* pip counts */

    fputs("<tr><td align=\"center\" colspan=\"2\">", pf);


    /* position ID Player 1 and end of table */

    fprintf(pf, "<span %s>", GetStyle(CLASS_POSITIONID, hecss));

    fprintf(pf, "%s <tt>%s</tt> %s <tt>%s</tt><br /></span></td></tr></table>\n",
            _("Position ID:"), PositionID((ConstTanBoard) pms->anBoard), _("Match ID:"), MatchIDFromMatchState(pms));

}


/*
 * print image for a point
 *
 * Input:
 *    pf : write to file
 *    szImageDir: path (URI) to images
 *    szImage: the image to print
 *    szExtension: extension of the image (e.g. gif or png)
 *    anBoard: the board
 *    iPoint: the point to draw
 *    fColor: color
 *    fUp: upper half or lower half of board
 *
 */

static void
printPointF2H(FILE * pf, const char *szImageDir, const char *szExtension,
              int iPoint0, int iPoint1, const int fColor, const int fUp, const htmlexportcss hecss)
{

    char sz[100];
    char szAlt[100];

    if (iPoint0) {

        /* player 0 owns the point */

        sprintf(sz, "b-%c%c-o%d", fColor ? 'g' : 'y', fUp ? 'd' : 'u', (iPoint0 >= 11) ? 11 : iPoint0);
        sprintf(szAlt, "%1xX", iPoint0);

    } else if (iPoint1) {

        /* player 1 owns the point */

        sprintf(sz, "b-%c%c-x%d", fColor ? 'g' : 'y', fUp ? 'd' : 'u', (iPoint1 >= 11) ? 11 : iPoint1);
        sprintf(szAlt, "%1xO", iPoint1);

    } else {
        /* empty point */
        sprintf(sz, "b-%c%c", fColor ? 'g' : 'y', fUp ? 'd' : 'u');
        sprintf(szAlt, "&nbsp;'");
    }

    printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_FIBS2HTML);

}


static void
printHTMLBoardF2H(FILE * pf, matchstate * pms, int fTurn,
                  const char *szImageDir, const char *szExtension, const htmlexportcss hecss)
{

    char sz[100];
    char szAlt[100];
    int fColor;
    int i;

    const char *aszDieO[] = { "b-odie1", "b-odie2", "b-odie3",
        "b-odie4", "b-odie5", "b-odie6"
    };
    const char *aszDieX[] = { "b-xdie1", "b-xdie2", "b-xdie3",
        "b-xdie4", "b-xdie5", "b-xdie6"
    };

    TanBoard anBoard;
    unsigned int anPips[2];

    memcpy(anBoard, pms->anBoard, sizeof(anBoard));

    if (pms->fMove)
        SwapSides(anBoard);

    /* top line with board numbers */

    fprintf(pf, "<p>\n");
    printImage(pf, szImageDir, "b-indent", szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
    sprintf(sz, "b-%stop%s", fTurn ? "hi" : "lo", fClockwise ? "" : "r");
    printImage(pf, szImageDir, sz, szExtension,
               fTurn ? "+-13-14-15-16-17-18-+---+-19-20-21-22-23-24-+" :
               "+-12-11-10--9--8--7-+---+--6--5--4--3--2--1-+", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
    fprintf(pf, "<br />\n");

    /* cube image */

    if (pms->fDoubled) {

        /* questionmark with cube (if player 0 is doubled) */

        if (!pms->fTurn) {
            sprintf(sz, "b-dup%d", 2 * pms->nCube);
            printImage(pf, szImageDir, sz, szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

        } else
            printImage(pf, szImageDir, "b-indent", szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

    } else {

        /* display arrow */

        if (pms->fCubeOwner == -1 || pms->fCubeOwner)
            printImage(pf, szImageDir, fTurn ? "b-topdn" : "b-topup",
                       szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        else {
            sprintf(sz, "%s%d", fTurn ? "b-tdn" : "b-tup", pms->nCube);
            printImage(pf, szImageDir, sz, szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        }

    }

    /* display left border */

    printImage(pf, szImageDir, "b-left", szExtension, "|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

    /* display player 0's outer quadrant */

    fColor = 1;

    for (i = 0; i < 6; i++) {

        printPointF2H(pf, szImageDir, szExtension, anBoard[1][11 - i], anBoard[0][12 + i], fColor, TRUE, hecss);

        fColor = !fColor;

    }

    /* display bar */

    if (anBoard[0][24]) {

        sprintf(sz, "b-bar-x%d", (anBoard[0][24] > 4) ? 4 : anBoard[0][24]);
        sprintf(szAlt, "|%1X&nbsp;|", anBoard[0][24]);
        printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_FIBS2HTML);

    } else
        printImage(pf, szImageDir, "b-bar", szExtension, "|&nbsp;&nbsp;&nbsp;|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

    /* display player 0's home quadrant */

    fColor = 1;

    for (i = 0; i < 6; i++) {

        printPointF2H(pf, szImageDir, szExtension, anBoard[1][5 - i], anBoard[0][18 + i], fColor, TRUE, hecss);

        fColor = !fColor;

    }

    /* right border */

    printImage(pf, szImageDir, "b-right", szExtension, "|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

    fprintf(pf, "<br />\n");

    /* center of board */

    if (pms->anDice[0] && pms->anDice[1]) {

        printImage(pf, szImageDir, "b-midin", szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir, "b-midl", szExtension, "|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

        printImage(pf, szImageDir,
                   fTurn ? "b-midg" : "b-midg2", szExtension,
                   "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                   "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir,
                   fTurn ? "b-midc" : aszDieO[pms->anDice[0] - 1],
                   szExtension, "|&nbsp;&nbsp;&nbsp;|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir,
                   fTurn ? "b-midg2" : aszDieO[pms->anDice[1] - 1],
                   szExtension, "|&nbsp;&nbsp;&nbsp;|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir,
                   fTurn ? aszDieX[pms->anDice[0] - 1] : "b-midg2",
                   szExtension, "|&nbsp;&nbsp;&nbsp;|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir,
                   fTurn ? aszDieX[pms->anDice[1] - 1] : "b-midc",
                   szExtension, "|&nbsp;&nbsp;&nbsp;|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir, fTurn ? "b-midg2" : "b-midg", szExtension,
                   "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                   "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir, "b-midr", szExtension, "|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

        fprintf(pf, "<br />\n");

    } else {

        if (pms->fDoubled)
            printImage(pf, szImageDir, "b-indent", szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        else
            printImage(pf, szImageDir, "b-midin", szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

        printImage(pf, szImageDir, "b-midl", szExtension, "|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir, "b-midg", szExtension,
                   "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                   "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir, "b-midc", szExtension, "|&nbsp;&nbsp;&nbsp;|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir, "b-midg", szExtension,
                   "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                   "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        printImage(pf, szImageDir, "b-midr", szExtension, "|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

        fprintf(pf, "<br />\n");

    }

    /* cube image */

    if (pms->fDoubled) {

        /* questionmark with cube (if player 0 is doubled) */

        if (pms->fTurn) {
            sprintf(sz, "b-ddn%d", 2 * pms->nCube);
            printImage(pf, szImageDir, sz, szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

        } else
            printImage(pf, szImageDir, "b-indent", szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

    } else {

        /* display cube */

        if (pms->fCubeOwner == -1 || !pms->fCubeOwner)
            printImage(pf, szImageDir, fTurn ? "b-botdn" : "b-botup",
                       szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        else {
            sprintf(sz, "%s%d", fTurn ? "b-bdn" : "b-bup", pms->nCube);
            printImage(pf, szImageDir, sz, szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
        }

    }

    printImage(pf, szImageDir, "b-left", szExtension, "|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

    /* display player 1's outer quadrant */

    fColor = 0;

    for (i = 0; i < 6; i++) {

        printPointF2H(pf, szImageDir, szExtension, anBoard[1][12 + i], anBoard[0][11 - i], fColor, FALSE, hecss);

        fColor = !fColor;

    }

    /* display bar */

    if (anBoard[1][24]) {

        sprintf(sz, "b-bar-o%d", (anBoard[1][24] > 4) ? 4 : anBoard[1][24]);
        sprintf(szAlt, "|&nbsp;%1dO|", anBoard[1][24]);
        printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_FIBS2HTML);

    } else
        printImage(pf, szImageDir, "b-bar", szExtension, "|&nbsp;&nbsp;&nbsp;|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

    /* display player 1's outer quadrant */

    fColor = 0;

    for (i = 0; i < 6; i++) {

        printPointF2H(pf, szImageDir, szExtension, anBoard[1][18 + i], anBoard[0][5 - i], fColor, FALSE, hecss);

        fColor = !fColor;

    }

    /* right border */

    printImage(pf, szImageDir, "b-right", szExtension, "|", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
    fprintf(pf, "<br />\n");

    /* bottom */



    printImage(pf, szImageDir, "b-indent", szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
    sprintf(sz, "b-%sbot%s", fTurn ? "lo" : "hi", fClockwise ? "" : "r");
    printImage(pf, szImageDir, sz, szExtension,
               fTurn ?
               "+-12-11-10--9--8--7-+---+--6--5--4--3--2--1-+" :
               "+-13-14-15-16-17-18-+---+-19-20-21-22-23-24-+", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
    fprintf(pf, "<br />\n");

    /* pip counts */

    printImage(pf, szImageDir, "b-indent", szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);

    PipCount((ConstTanBoard) anBoard, anPips);
    fprintf(pf, _("Pip counts: %s %d, %s %d<br />\n"), ap[0].szName, anPips[1], ap[1].szName, anPips[0]);

    /* position ID */

    printImage(pf, szImageDir, "b-indent", szExtension, "", hecss, HTML_EXPORT_TYPE_FIBS2HTML);
    fprintf(pf, "<span %s>", GetStyle(CLASS_POSITIONID, hecss));

    fprintf(pf, _("Position ID: <tt>%s</tt> Match ID: <tt>%s</tt><br />\n"),
            PositionID((ConstTanBoard) pms->anBoard), MatchIDFromMatchState(pms));

    fprintf(pf, "</span>");

    fprintf(pf, "</p>\n");

}



/*
 * print image for a point
 *
 * Input:
 *    pf : write to file
 *    szImageDir: path (URI) to images
 *    szImage: the image to print
 *    szExtension: extension of the image (e.g. gif or png)
 *    anBoard: the board
 *    iPoint: the point to draw
 *    fColor: color
 *    fUp: upper half or lower half of board
 *
 */

static void
printPointGNU(FILE * pf, const char *szImageDir, const char *szExtension,
              int iPoint0, int iPoint1, const int fColor, const int fUp, const htmlexportcss hecss)
{

    char sz[100];
    char szAlt[100];

    if (iPoint0) {

        /* player 0 owns the point */

        sprintf(sz, "b-%c%c-x%d", fColor ? 'g' : 'r', fUp ? 'd' : 'u', iPoint0);
        sprintf(szAlt, "%1xX", iPoint0);

    } else if (iPoint1) {

        /* player 1 owns the point */

        sprintf(sz, "b-%c%c-o%d", fColor ? 'g' : 'r', fUp ? 'd' : 'u', iPoint1);
        sprintf(szAlt, "%1xO", iPoint1);

    } else {
        /* empty point */
        sprintf(sz, "b-%c%c", fColor ? 'g' : 'r', fUp ? 'd' : 'u');
        sprintf(szAlt, "&nbsp;'");
    }

    printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_GNU);

}


static void
printHTMLBoardGNU(FILE * pf, matchstate * pms, int fTurn,
                  const char *szImageDir, const char *szExtension, const htmlexportcss hecss)
{

    char sz[1000];
    char szAlt[1000];
    int i, j;

    TanBoard anBoard;
    unsigned int anPips[2];
    int acOff[2];

    memcpy(anBoard, pms->anBoard, sizeof(anBoard));

    if (pms->fMove)
        SwapSides(anBoard);

    for (i = 0; i < 2; i++) {
        acOff[i] = 15;
        for (j = 0; j < 25; j++)
            acOff[i] -= anBoard[i][j];
    }

    /* top line with board numbers */

    fputs("<table cellpadding=\"0\" border=\"0\" cellspacing=\"0\""
          " style=\"margin: 0; padding: 0; border: 0\">\n", pf);

    fputs("<tr>", pf);
    fputs("<td colspan=\"15\">", pf);

    sprintf(sz, "b-%stop%s", fTurn ? "hi" : "lo", fClockwise ? "" : "r");
    printImage(pf, szImageDir, sz, szExtension,
               fClockwise ?
               (fTurn ? "+-24-23-22-21-20-19-+---+-18-17-16-15-14-13-+" :
                "+--1--2--3--4--5--6-+---+--7--8--9-10-11-12-+") :
               (fTurn ? "+-13-14-15-16-17-18-+---+-19-20-21-22-23-24-+" :
                "+-12-11-10--9--8--7-+---+--6--5--4--3--2--1-+"), hecss, HTML_EXPORT_TYPE_GNU);

    fputs("</td></tr>\n", pf);

    /* display left bearoff tray */

    fputs("<tr>", pf);


    fputs("<td rowspan=\"2\">", pf);
    if (fClockwise)
        /* Use roff as loff not generated (and probably not needed) */
        sprintf(sz, "b-roff-x%d", acOff[1]);
    else
        strcpy(sz, "b-loff-x0");
    printImage(pf, szImageDir, sz, szExtension, "|", hecss, HTML_EXPORT_TYPE_GNU);
    fputs("</td>", pf);

    /* display player 0's outer quadrant */

    for (i = 0; i < 6; i++) {
        fputs("<td rowspan=\"2\">", pf);
        if (fClockwise)
            printPointGNU(pf, szImageDir, szExtension, anBoard[1][i], anBoard[0][23 - i], !(i % 2), TRUE, hecss);
        else
            printPointGNU(pf, szImageDir, szExtension, anBoard[1][11 - i], anBoard[0][12 + i], !(i % 2), TRUE, hecss);
        fputs("</td>", pf);
    }


    /* display cube */

    fputs("<td>", pf);

    if (!pms->fCubeOwner) {
        sprintf(sz, "b-ct-%d", pms->nCube);
        sprintf(szAlt, "|%2d&nbsp;|", pms->nCube);
    } else {
        strcpy(sz, "b-ct");
        strcpy(szAlt, "|&nbsp;&nbsp;&nbsp;|");
    }

    printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_GNU);

    fputs("</td>", pf);


    /* display player 0's home quadrant */

    for (i = 0; i < 6; i++) {
        fputs("<td rowspan=\"2\">", pf);
        if (fClockwise)
            printPointGNU(pf, szImageDir, szExtension, anBoard[1][i + 6], anBoard[0][17 - i], !(i % 2), TRUE, hecss);
        else
            printPointGNU(pf, szImageDir, szExtension, anBoard[1][5 - i], anBoard[0][18 + i], !(i % 2), TRUE, hecss);
        fputs("</td>", pf);
    }


    /* right bearoff tray */

    fputs("<td rowspan=\"2\">", pf);
    if (!fClockwise)
        sprintf(sz, "b-roff-x%d", acOff[1]);
    else
        strcpy(sz, "b-roff-x0");
    printImage(pf, szImageDir, sz, szExtension, "|", hecss, HTML_EXPORT_TYPE_GNU);
    fputs("</td>", pf);

    fputs("</tr>\n", pf);


    /* display bar */

    fputs("<tr>", pf);
    fputs("<td>", pf);

    sprintf(sz, "b-bar-o%d", anBoard[1][24]);
    if (anBoard[1][24])
        sprintf(szAlt,
                "|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                "|&nbsp;%1XX|"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;|",
                anBoard[1][24]);
    else
        strcpy(szAlt,
               "|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
               "|&nbsp;&nbsp;&nbsp;|"
               "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;|");
    printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_GNU);

    fputs("</td>", pf);
    fputs("</tr>\n", pf);


    /* center of board */

    fputs("<tr>", pf);

    /* left part of bar */

    fputs("<td>", pf);
    if (fClockwise)
        printImage(pf, szImageDir, "b-midlb", szExtension, "|", hecss, HTML_EXPORT_TYPE_GNU);
    else
        printImage(pf, szImageDir, fTurn ? "b-midlb-o" : "b-midlb-x", szExtension, "|", hecss, HTML_EXPORT_TYPE_GNU);

    fputs("</td>", pf);

    /* center of board */

    fputs("<td colspan=\"6\">", pf);

    if (!pms->fMove && pms->anDice[0] && pms->anDice[1]) {

        /* player has rolled the dice */

        sprintf(sz, "b-midl-x%d%d", pms->anDice[0], pms->anDice[1]);
        sprintf(szAlt, "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;%d&nbsp;%d&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;",
                pms->anDice[0], pms->anDice[1]);

        printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_GNU);

    } else if (!pms->fMove && pms->fDoubled) {

        /* player 0 has doubled */

        sprintf(sz, "b-midl-c%d", 2 * pms->nCube);
        sprintf(szAlt, "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[%d]&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;",
                2 * pms->nCube);
        printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_GNU);

    } else {

        /* player 0 on roll */

        printImage(pf, szImageDir, "b-midl", szExtension,
                   "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;",
                   hecss, HTML_EXPORT_TYPE_GNU);

    }

    fputs("</td>", pf);

    /* centered cube */

    if (pms->fCubeOwner == -1 && !pms->fDoubled) {
        sprintf(sz, "b-midc-%d", pms->nCube);
        sprintf(szAlt, "|%2d&nbsp;|", pms->nCube);
    } else {
        strcpy(sz, "b-midc");
        strcpy(szAlt, "|&nbsp;&nbsp;&nbsp;|");
    }

    fputs("<td>", pf);
    printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_GNU);
    fputs("</td>", pf);

    /* player 1 */

    fputs("<td colspan=\"6\">", pf);

    if (pms->fMove && pms->anDice[0] && pms->anDice[1]) {

        /* player 1 has rolled the dice */

        sprintf(sz, "b-midr-o%d%d", pms->anDice[0], pms->anDice[1]);
        sprintf(szAlt, "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;%d&nbsp;%d&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;",
                pms->anDice[0], pms->anDice[1]);

        printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_GNU);

    } else if (pms->fMove && pms->fDoubled) {

        /* player 1 has doubled */

        sprintf(sz, "b-midr-c%d", 2 * pms->nCube);
        sprintf(szAlt, "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[%d]&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;",
                2 * pms->nCube);

        printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_GNU);

    } else {

        /* player 1 on roll */

        printImage(pf, szImageDir, "b-midr", szExtension,
                   "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;",
                   hecss, HTML_EXPORT_TYPE_GNU);

    }

    fputs("</td>", pf);

    /* right part of bar */

    fputs("<td>", pf);
    if (!fClockwise)
        printImage(pf, szImageDir, "b-midrb", szExtension, "|", hecss, HTML_EXPORT_TYPE_GNU);
    else
        printImage(pf, szImageDir, fTurn ? "b-midrb-o" : "b-midrb-x", szExtension, "|", hecss, HTML_EXPORT_TYPE_GNU);
    fputs("</td>", pf);

    fputs("</tr>\n", pf);


    /* display left bearoff tray */

    fputs("<tr>", pf);

    fputs("<td rowspan=\"2\">", pf);
    if (fClockwise)
        /* Use roff as loff not generated (and probably not needed) */
        sprintf(sz, "b-roff-o%d", acOff[0]);
    else
        strcpy(sz, "b-loff-o0");
    printImage(pf, szImageDir, sz, szExtension, "|", hecss, HTML_EXPORT_TYPE_GNU);
    fputs("</td>", pf);

    /* display player 1's outer quadrant */

    for (i = 0; i < 6; i++) {
        fputs("<td rowspan=\"2\">", pf);
        if (fClockwise)
            printPointGNU(pf, szImageDir, szExtension, anBoard[1][23 - i], anBoard[0][i], i % 2, FALSE, hecss);
        else
            printPointGNU(pf, szImageDir, szExtension, anBoard[1][12 + i], anBoard[0][11 - i], i % 2, FALSE, hecss);
        fputs("</td>", pf);
    }


    /* display bar */

    fputs("<td>", pf);

    sprintf(sz, "b-bar-x%d", anBoard[0][24]);
    if (pms->fCubeOwner == 1)
        sprintf(szAlt, "|%2d&nbsp;|", pms->nCube);
    else
        strcpy(szAlt, "|&nbsp;&nbsp;&nbsp;|");

    printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_GNU);

    fputs("</td>", pf);


    /* display player 1's outer quadrant */

    for (i = 0; i < 6; i++) {
        fputs("<td rowspan=\"2\">", pf);
        if (fClockwise)
            printPointGNU(pf, szImageDir, szExtension, anBoard[1][17 - i], anBoard[0][i + 6], i % 2, FALSE, hecss);
        else
            printPointGNU(pf, szImageDir, szExtension, anBoard[1][18 + i], anBoard[0][5 - i], i % 2, FALSE, hecss);
        fputs("</td>", pf);
    }


    /* right bearoff tray */

    fputs("<td rowspan=\"2\">", pf);
    if (!fClockwise)
        sprintf(sz, "b-roff-o%d", acOff[0]);
    else
        strcpy(sz, "b-roff-o0");
    printImage(pf, szImageDir, sz, szExtension, "|", hecss, HTML_EXPORT_TYPE_GNU);
    fputs("</td>", pf);

    fputs("</tr>\n", pf);


    /* display cube */

    fputs("<tr>", pf);
    fputs("<td>", pf);

    if (pms->fCubeOwner == 1)
        sprintf(sz, "b-cb-%d", pms->nCube);
    else
        strcpy(sz, "b-cb");

    if (anBoard[0][24])
        /* text for alt tag - bottom bar piece */
        sprintf(szAlt,
                "|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                "|&nbsp;%1XO|"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;|",
                anBoard[0][24]);
    else
        strcpy(szAlt, "");

    printImage(pf, szImageDir, sz, szExtension, szAlt, hecss, HTML_EXPORT_TYPE_GNU);

    fputs("</td>", pf);
    fputs("</tr>\n", pf);


    /* bottom */

    fputs("<tr>", pf);
    fputs("<td colspan=\"15\">", pf);
    sprintf(sz, "b-%sbot%s", fTurn ? "lo" : "hi", fClockwise ? "" : "r");
    printImage(pf, szImageDir, sz, szExtension,
               fClockwise ?
               (fTurn ? "+--1--2--3--4--5--6-+---+--7--8--9-10-11-12-+" :
                "+-24-23-22-21-20-19-+---+-18-17-16-15-14-13-+") :
               (fTurn ? "+-12-11-10--9--8--7-+---+--6--5--4--3--2--1-+" :
                "+-13-14-15-16-17-18-+---+-19-20-21-22-23-24-+"), hecss, HTML_EXPORT_TYPE_GNU);
    fputs("</td>", pf);
    fputs("</tr>", pf);

    fputs("</table>\n\n", pf);

    /* pip counts */

    fputs("<p>", pf);

    PipCount((ConstTanBoard) anBoard, anPips);
    fprintf(pf, _("Pip counts: %s %d, %s %d<br />\n"), ap[0].szName, anPips[1], ap[1].szName, anPips[0]);

    /* position ID */

    fprintf(pf, "<span %s>", GetStyle(CLASS_POSITIONID, hecss));

    fprintf(pf, _("Position ID: <tt>%s</tt> Match ID: <tt>%s</tt><br />\n"),
            PositionID((ConstTanBoard) pms->anBoard), MatchIDFromMatchState(pms));

    fprintf(pf, "</span>");

    fputs("</p>\n", pf);

}


static void
printHTMLBoard(FILE * pf, matchstate * pms, int fTurn,
               const char *szImageDir, const char *szExtension, const htmlexporttype het, const htmlexportcss hecss)
{

    fputs("\n<!--  Board -->\n\n", pf);

    switch (het) {
    case HTML_EXPORT_TYPE_FIBS2HTML:
        printHTMLBoardF2H(pf, pms, fTurn, szImageDir, szExtension, hecss);
        break;
    case HTML_EXPORT_TYPE_BBS:
        printHTMLBoardBBS(pf, pms, fTurn, szImageDir, szExtension, hecss);
        break;
    case HTML_EXPORT_TYPE_GNU:
        printHTMLBoardGNU(pf, pms, fTurn, szImageDir, szExtension, hecss);
        break;
    default:
        printf(_("unknown board type\n"));
        break;
    }

    fputs("\n<!-- End Board -->\n\n", pf);

}


/*
 * Print html header for board: move or cube decision 
 *
 * Input:
 *   pf: output file
 *   ms: current match state
 *   iMove: move no.
 *
 */

static void
HTMLBoardHeader(FILE * pf, const matchstate * pms,
                const htmlexporttype UNUSED(het),
                const htmlexportcss UNUSED(hecss), const int iGame, const int iMove, const int fHR)
{

    fputs("\n<!-- Header -->\n\n", pf);

    if (fHR)
        fputs("<hr />\n", pf);

    fputs("<p>", pf);

    if (iMove >= 0) {
        fprintf(pf, "<b>" "<a name=\"game%d.move%d\">", iGame + 1, iMove + 1);
        fprintf(pf, _("Move number %d:"), iMove + 1);
        fputs("</a></b>", pf);
    }

    if (pms->fResigned)

        /* resignation */

        fprintf(pf,
                ngettext(" %s resigns %d point",
                         " %s resigns %d points",
                         pms->fResigned * pms->nCube), ap[pms->fTurn].szName, pms->fResigned * pms->nCube);

    else if (pms->anDice[0] && pms->anDice[1])

        /* chequer play decision */

        fprintf(pf, _(" %s to play %d%d"), ap[pms->fMove].szName, pms->anDice[0], pms->anDice[1]
            );

    else if (pms->fDoubled)

        /* take decision */

        fprintf(pf, _(" %s doubles to %d"), ap[!pms->fTurn].szName, pms->nCube * 2);

    else
        /* cube decision */

        fprintf(pf, _(" %s on roll, cube decision?"), ap[pms->fMove].szName);

    fputs("</p>\n", pf);

    fputs("\n<!-- End Header -->\n\n", pf);

}


/*
 * Print html header: dtd, head etc.
 *
 * Input:
 *   pf: output file
 *   ms: current match state
 *
 */

static void
HTMLPrologue(FILE * pf, const matchstate * pms,
             const int iGame, char *aszLinks[4], const htmlexporttype UNUSED(het), const htmlexportcss hecss)
{
    char szTitle[100];

    int i;
    int fFirst;

    /* DTD */

    sprintf(szTitle,
            ngettext("The score (after %d game) is: %s %d, %s %d",
                     "The score (after %d games) is: %s %d, %s %d",
                     pms->cGames), pms->cGames, ap[0].szName, pms->anScore[0], ap[1].szName, pms->anScore[1]);

    if (pms->nMatchTo > 0)
        sprintf(strchr(szTitle, 0),
                ngettext(" (match to %d point%s)",
                         " (match to %d points%s)",
                         pms->nMatchTo),
                pms->nMatchTo,
                pms->fCrawford ? _(", Crawford game") : (pms->fPostCrawford ? _(", post-Crawford play") : ""));

    fprintf(pf,
            "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' "
            "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
            "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" "
            "lang=\"en\">\n"
            "<head>\n"
            "<meta name=\"generator\" content=\"%s\" />\n"
            "<meta http-equiv=\"Content-Type\" "
            "content=\"text/html; charset=%s\" />\n"
            "<meta name=\"keywords\" content=\"%s, %s, %s\" />\n"
            "<meta name=\"description\" "
            "content=\"",
            VERSION_STRING,
            GNUBG_CHARSET, ap[0].szName, ap[1].szName, (pms->nMatchTo) ? _("match play") : _("money game"));

    fprintf(pf, _("%s (analysed by %s)"), szTitle, VERSION_STRING);

    fprintf(pf, "\" />\n" "<title>%s</title>\n", szTitle);

    if (hecss == HTML_EXPORT_CSS_HEAD)
        WriteStyleSheet(pf, hecss);
    else if (hecss == HTML_EXPORT_CSS_EXTERNAL)
        fputs("<link title=\"CSS stylesheet\" rel=\"stylesheet\" " "href=\"gnubg.css\" type=\"text/css\" />\n", pf);

    fprintf(pf, "</head>\n" "\n" "<body %s>\n" "<h1>", GetStyle(CLASS_FONT_FAMILY, hecss));

    fprintf(pf, _("Game number %d"), iGame + 1);

    fprintf(pf, "</h1>\n" "<h2>%s</h2>\n", szTitle);

    /* add links to other games */

    fFirst = TRUE;

    for (i = 0; i < 4; i++)
        if (aszLinks && aszLinks[i]) {
            if (fFirst) {
                fprintf(pf, "<hr />\n");
                fputs("<p>\n", pf);
                fFirst = FALSE;
            }
            fprintf(pf, "<a href=\"%s\">%s</a>\n", aszLinks[i], gettext(aszLinkText[i]));
        }

    if (!fFirst)
        fputs("</p>\n", pf);

}


/*
 * Print html header: dtd, head etc.
 *
 * Input:
 *   pf: output file
 *   ms: current match state
 *
 */

static void
HTMLEpilogue(FILE * pf, const matchstate * UNUSED(pms), char *aszLinks[4], const htmlexportcss UNUSED(hecss))
{

    time_t t;
    int fFirst;
    int i;

    const char szVersion[] = "$Revision: 1.235 $";
    int iMajor, iMinor;

    iMajor = atoi(strchr(szVersion, ' '));
    iMinor = atoi(strchr(szVersion, '.') + 1);

    fputs("\n<!-- Epilogue -->\n\n", pf);

    /* add links to other games */

    fFirst = TRUE;

    for (i = 0; i < 4; i++)
        if (aszLinks && aszLinks[i]) {
            if (fFirst) {
                fprintf(pf, "<hr />\n");
                fputs("<p>\n", pf);
                fFirst = FALSE;
            }
            fprintf(pf, "<a href=\"%s\">%s</a>\n", aszLinks[i], aszLinkText[i]);
        }


    if (!fFirst)
        fputs("</p>\n", pf);

    time(&t);

    fputs("<hr />\n" "<address>", pf);

    fprintf(pf,
            _("Output generated %s by "
              "<a href=\"http://www.gnu.org/software/gnubg/\">%s</a>"), ctime(&t), VERSION_STRING);

    fputs(" ", pf);

    fprintf(pf, _("(HTML Export version %d.%d)"), iMajor, iMinor);

    fprintf(pf,
            "</address>\n"
            "<p>\n"
            "<a href=\"http://validator.w3.org/check/referer\">"
            "<img style=\"border: 0\" width=\"88\" height=\"31\" "
            "src=\"http://www.w3.org/Icons/valid-xhtml10\" "
            "alt=\"%s\" /></a>\n"
            "<a href=\"http://jigsaw.w3.org/css-validator/check/referer\">"
            "<img style=\"border: 0\" width=\"88\" height=\"31\" "
            "src=\"http://jigsaw.w3.org/css-validator/images/vcss\" "
            "alt=\"%s\" />" "</a>\n" "</p>\n" "</body>\n" "</html>\n", _("Valid XHTML 1.0!"), _("Valid CSS!"));


}



/*
 * Print html header: dtd, head etc.
 *
 * Input:
 *   pf: output file
 *   ms: current match state
 *
 */

static void
HTMLEpilogueComment(FILE * pf)
{

    time_t t;

    const char szVersion[] = "$Revision: 1.235 $";
    int iMajor, iMinor;
    char *pc;

    iMajor = atoi(strchr(szVersion, ' '));
    iMinor = atoi(strchr(szVersion, '.') + 1);

    time(&t);

    pc = ctime(&t);
    if ((pc = strchr(pc, '\n')))
        *pc = 0;

    fputs("\n<!-- Epilogue -->\n\n", pf);

    fprintf(pf, _("<!-- Output generated %s by %s " "(http://www.gnu.org/software/gnubg/) "), pc, VERSION_STRING);

    fputs(" ", pf);

    fprintf(pf, _("(HTML Export version %d.%d) -->"), iMajor, iMinor);


}

/*
 * Print cube analysis
 *
 * Input:
 *  pf: output file
 *  arDouble: equitites for cube decisions
 *  fPlayer: player who doubled
 *  esDouble: eval setup
 *  pci: cubeinfo
 *  fDouble: double/no double
 *  fTake: take/drop
 *
 */

static void
HTMLPrintCubeAnalysisTable(FILE * pf,
                           float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                           float aarStdDev[2][NUM_ROLLOUT_OUTPUTS],
                           int UNUSED(fPlayer),
                           evalsetup * pes, cubeinfo * pci,
                           int fDouble, int fTake, skilltype stDouble, skilltype stTake, const htmlexportcss hecss)
{
    const char *aszCube[] = {
        NULL,
        N_("No double"),
        N_("Double, take"),
        N_("Double, pass")
    };
    float arDouble[4];

    int i;
    int ai[3];
    float r;

    int fActual, fClose, fMissed;
    int fDisplay;
    int fAnno = FALSE;

    cubedecision cd;

    /* check if cube analysis should be printed */

    if (pes->et == EVAL_NONE)
        return;                 /* no evaluation */

    cd = FindCubeDecision(arDouble, aarOutput, pci);

    fActual = fDouble > 0;
    fClose = isCloseCubedecision(arDouble);
    fMissed = fDouble > -1 && isMissedDouble(arDouble, aarOutput, fDouble, pci);

    fDisplay =
        (fActual && exsExport.afCubeDisplay[EXPORT_CUBE_ACTUAL]) ||
        (fClose && exsExport.afCubeDisplay[EXPORT_CUBE_CLOSE]) ||
        (fMissed && exsExport.afCubeDisplay[EXPORT_CUBE_MISSED]) ||
        (exsExport.afCubeDisplay[stDouble]) || (exsExport.afCubeDisplay[stTake]);

    if (!fDisplay)
        return;

    fputs("\n<!-- Cube Analysis -->\n\n", pf);


    /* print alerts */

    if (fMissed) {

        fAnno = TRUE;

        /* missed double */

        fprintf(pf, "<p><span %s>%s (%s)!",
                GetStyle(CLASS_BLUNDER, hecss),
                _("Alert: missed double"),
                OutputEquityDiff(arDouble[OUTPUT_NODOUBLE],
                                 (arDouble[OUTPUT_TAKE] >
                                  arDouble[OUTPUT_DROP]) ? arDouble[OUTPUT_DROP] : arDouble[OUTPUT_TAKE], pci));

        if (badSkill(stDouble))
            fprintf(pf, " [%s]", gettext(aszSkillType[stDouble]));

        fprintf(pf, "</span></p>\n");

    }

    r = arDouble[OUTPUT_TAKE] - arDouble[OUTPUT_DROP];

    if (fTake > 0 && r > 0.0f) {

        fAnno = TRUE;

        /* wrong take */

        fprintf(pf, "<p><span %s>%s (%s)!",
                GetStyle(CLASS_BLUNDER, hecss),
                _("Alert: wrong take"), OutputEquityDiff(arDouble[OUTPUT_DROP], arDouble[OUTPUT_TAKE], pci));

        if (badSkill(stTake))
            fprintf(pf, " [%s]", gettext(aszSkillType[stTake]));

        fprintf(pf, "</span></p>\n");

    }

    r = arDouble[OUTPUT_DROP] - arDouble[OUTPUT_TAKE];

    if (fDouble > 0 && !fTake && r > 0.0f) {

        fAnno = TRUE;

        /* wrong pass */

        fprintf(pf, "<p><span %s>%s (%s)!",
                GetStyle(CLASS_BLUNDER, hecss),
                _("Alert: wrong pass"), OutputEquityDiff(arDouble[OUTPUT_TAKE], arDouble[OUTPUT_DROP], pci));

        if (badSkill(stTake))
            fprintf(pf, " [%s]", gettext(aszSkillType[stTake]));

        fprintf(pf, "</span></p>\n");

    }


    if (arDouble[OUTPUT_TAKE] > arDouble[OUTPUT_DROP])
        r = arDouble[OUTPUT_NODOUBLE] - arDouble[OUTPUT_DROP];
    else
        r = arDouble[OUTPUT_NODOUBLE] - arDouble[OUTPUT_TAKE];

    if (fDouble > 0 && fTake < 0 && r > 0.0f) {

        fAnno = TRUE;

        /* wrong double */

        fprintf(pf, "<p><span %s>%s (%s)!",
                GetStyle(CLASS_BLUNDER, hecss),
                _("Alert: wrong double"),
                OutputEquityDiff((arDouble[OUTPUT_TAKE] >
                                  arDouble[OUTPUT_DROP]) ?
                                 arDouble[OUTPUT_DROP] : arDouble[OUTPUT_TAKE], arDouble[OUTPUT_NODOUBLE], pci));

        if (badSkill(stDouble))
            fprintf(pf, " [%s]", gettext(aszSkillType[stDouble]));

        fprintf(pf, "</span></p>\n");

    }

    if ((badSkill(stDouble) || badSkill(stTake)) && !fAnno) {

        if (badSkill(stDouble)) {
            fprintf(pf, "<p><span %s>", GetStyle(CLASS_BLUNDER, hecss));
            fprintf(pf, _("Alert: double decision marked %s"), gettext(aszSkillType[stDouble]));
            fputs("</span></p>\n", pf);
        }

        if (badSkill(stTake)) {
            fprintf(pf, "<p><span %s>", GetStyle(CLASS_BLUNDER, hecss));
            fprintf(pf, _("Alert: take decision marked %s"), gettext(aszSkillType[stTake]));
            fputs("</span></p>\n", pf);
        }

    }

    /* print table */

    fprintf(pf, "<table %s>\n", GetStyle(CLASS_CUBEDECISION, hecss));

    /* header */

    fprintf(pf,
            "<tr><th colspan=\"4\" %s>%s</th></tr>\n", GetStyle(CLASS_CUBEDECISIONHEADER, hecss), _("Cube decision"));

    /* ply & cubeless equity */

    /* FIXME: about parameters if exsExport.afCubeParameters */

    fprintf(pf, "<tr>");

    /* ply */

    fprintf(pf, "<td colspan=\"2\">" "<span %s>", GetStyle(CLASS_CUBE_PLY, hecss));

    switch (pes->et) {
    case EVAL_NONE:
        fputs(_("n/a"), pf);
        break;
    case EVAL_EVAL:
        fprintf(pf, _("%d-ply"), pes->ec.nPlies);
        break;
    case EVAL_ROLLOUT:
        fputs(_("Rollout"), pf);
        break;
    }

    /* cubeless equity */

    if (pci->nMatchTo)
        fprintf(pf,
                "</span> %s</td>"
                "<td %s>%s</td>"
                "<td %s>(%s: <span %s>%s</span>)</td>\n",
                (!pci->nMatchTo || (pci->nMatchTo && !fOutputMWC)) ?
                _("cubeless equity") : _("cubeless MWC"),
                GetStyle(CLASS_CUBE_EQUITY, hecss),
                OutputEquity(aarOutput[0][OUTPUT_EQUITY], pci, TRUE),
                GetStyle(CLASS_CUBE_CUBELESS_TEXT, hecss),
                _("Money"), GetStyle(CLASS_CUBE_EQUITY, hecss), OutputMoneyEquity(aarOutput[0], TRUE));
    else
        fprintf(pf, " cubeless equity</td><td>%s</td><td>&nbsp;</td>\n", OutputMoneyEquity(aarOutput[0], TRUE));


    fprintf(pf, "</tr>\n");

    /* percentages */

    if (exsExport.fCubeDetailProb && pes->et == EVAL_EVAL) {

        fprintf(pf, "<tr><td>&nbsp;</td>" "<td colspan=\"3\" %s>", GetStyle(CLASS_CUBE_PROBABILITIES, hecss));

        fputs(OutputPercents(aarOutput[0], TRUE), pf);

        fputs("</td></tr>\n", pf);

    }

    /* equities */

    fprintf(pf, "<tr><td colspan=\"4\">%s</td></tr>\n", _("Cubeful equities:"));

    /* evaluate parameters */

    if (pes->et == EVAL_EVAL && exsExport.afCubeParameters[0]) {

        fputs("<tr><td>&nbsp;</td>" "<td colspan=\"3\">", pf);
        fputs(OutputEvalContext(&pes->ec, FALSE), pf);
        fputs("</td></tr>\n", pf);

    }

    getCubeDecisionOrdering(ai, arDouble, aarOutput, pci);

    for (i = 0; i < 3; i++) {

        fprintf(pf, "<tr><td>%d.</td><td>%s</td>", i + 1, gettext(aszCube[ai[i]]));

        fprintf(pf, "<td %s>%s</td>", GetStyle(CLASS_CUBE_EQUITY, hecss), OutputEquity(arDouble[ai[i]], pci, TRUE));

        if (i)
            fprintf(pf, "<td>%s</td>", OutputEquityDiff(arDouble[ai[i]], arDouble[OUTPUT_OPTIMAL], pci));
        else
            fputs("<td>&nbsp;</td>", pf);

        fputs("</tr>\n", pf);

    }

    /* cube decision */

    fprintf(pf,
            "<tr><td colspan=\"2\">%s</td>"
            "<td colspan=\"2\" %s>%s",
            _("Proper cube action:"), GetStyle(CLASS_CUBE_ACTION, hecss), GetCubeRecommendation(cd));

    if ((r = getPercent(cd, arDouble)) >= 0.0)
        fprintf(pf, " (%.1f%%)", 100.0f * r);



    fprintf(pf, "</td></tr>\n");

    /* rollout details */

    if (pes->et == EVAL_ROLLOUT && (exsExport.fCubeDetailProb || exsExport.afCubeParameters[1])) {

        fprintf(pf, "<tr><th colspan=\"4\">%s</th></tr>\n", _("Rollout details"));

    }

    if (pes->et == EVAL_ROLLOUT && exsExport.fCubeDetailProb) {

        char asz[2][1024];
        cubeinfo aci[2];

        for (i = 0; i < 2; i++) {

            memcpy(&aci[i], pci, sizeof(cubeinfo));

            if (i) {
                aci[i].fCubeOwner = !pci->fMove;
                aci[i].nCube *= 2;
            }

            FormatCubePosition(asz[i], &aci[i]);

        }

        fputs("<tr>" "<td colspan=\"4\">", pf);

        printRolloutTable(pf, asz, aarOutput, aarStdDev, aci, 2, pes->rc.fCubeful, TRUE, hecss);

        fputs("</td></tr>\n", pf);

    }

    if (pes->et == EVAL_ROLLOUT && exsExport.afCubeParameters[1]) {

        char *sz = g_strdup(OutputRolloutContext(NULL, &pes->rc));
        char *pcS = sz, *pcE;

        while ((pcE = strstr(pcS, "\n"))) {

            *pcE = 0;
            fprintf(pf, "<tr><td colspan=\"4\">%s</td></tr>\n", pcS);
            pcS = pcE + 1;

        }

        g_free(sz);

    }

    fprintf(pf, "</table>\n");

    fputs("<p>&nbsp;</p>\n", pf);

    fputs("\n<!-- End Cube Analysis -->\n\n", pf);

}



/*
 * Wrapper for print cube analysis
 *
 * Input:
 *  pf: output file
 *  pms: match state
 *  pmr: current move record
 *  szImageDir: URI to images
 *  szExtension: extension of images
 *  fTake: take/drop
 *
 */

static void
HTMLPrintCubeAnalysis(FILE * pf, matchstate * pms, moverecord * pmr,
                      const char *UNUSED(szImageDir), const char *UNUSED(szExtension),
                      const htmlexporttype UNUSED(het), const htmlexportcss hecss)
{

    cubeinfo ci;
    /* we need to remember the double type to be able to do the right
     * thing for beavers and racoons */
    static doubletype dt = DT_NORMAL;

    GetMatchStateCubeInfo(&ci, pms);

    switch (pmr->mt) {

    case MOVE_NORMAL:

        /* cube analysis from move */

        HTMLPrintCubeAnalysisTable(pf,
                                   pmr->CubeDecPtr->aarOutput, pmr->CubeDecPtr->aarStdDev,
                                   pmr->fPlayer,
                                   &pmr->CubeDecPtr->esDouble, &ci, FALSE, -1, pmr->stCube, SKILL_NONE, hecss);
        dt = DT_NORMAL;

        break;

    case MOVE_DOUBLE:

        dt = DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);
        if (dt != DT_NORMAL) {
            fprintf(pf, "<p><span %s> Cannot analyse doubles nor raccoons!</span></p>\n",
                    GetStyle(CLASS_BLUNDER, hecss));
            break;
        }
        HTMLPrintCubeAnalysisTable(pf,
                                   pmr->CubeDecPtr->aarOutput,
                                   pmr->CubeDecPtr->aarStdDev,
                                   pmr->fPlayer,
                                   &pmr->CubeDecPtr->esDouble, &ci, TRUE, -1, pmr->stCube, SKILL_NONE, hecss);

        break;

    case MOVE_TAKE:
    case MOVE_DROP:

        /* cube analysis from double, {take, drop, beaver} */

        if (dt != DT_NORMAL) {
            dt = DT_NORMAL;
            fprintf(pf, "<p><span %s> Cannot analyse doubles nor raccoons!</span></p>\n",
                    GetStyle(CLASS_BLUNDER, hecss));
            break;
        }
        HTMLPrintCubeAnalysisTable(pf, pmr->CubeDecPtr->aarOutput, pmr->CubeDecPtr->aarStdDev, pmr->fPlayer, &pmr->CubeDecPtr->esDouble, &ci, TRUE, pmr->mt == MOVE_TAKE, SKILL_NONE,   /* FIXME: skill from prev. cube */
                                   pmr->stCube, hecss);

        break;

    default:

        g_assert_not_reached();

    }

    return;

}


/*
 * Print move analysis
 *
 * Input:
 *  pf: output file
 *  pms: match state
 *  pmr: current move record
 *  szImageDir: URI to images
 *  szExtension: extension of images
 *
 */

static void
HTMLPrintMoveAnalysis(FILE * pf, matchstate * pms, moverecord * pmr,
                      const char *UNUSED(szImageDir), const char *UNUSED(szExtension),
                      const htmlexporttype UNUSED(het), const htmlexportcss hecss)
{

    char sz[64];
    unsigned int i;
    float rEq, rEqTop;

    cubeinfo ci;

    GetMatchStateCubeInfo(&ci, pms);

    /* check if move should be printed */

    if (!exsExport.afMovesDisplay[pmr->n.stMove])
        return;

    fputs("\n<!-- Move Analysis -->\n\n", pf);

    /* print alerts */

    if (badSkill(pmr->n.stMove) && pmr->n.iMove < pmr->ml.cMoves) {

        /* blunder or error */

        fprintf(pf, "<p><span %s>", GetStyle(CLASS_BLUNDER, hecss));
        fprintf(pf, _("Alert: %s move"), gettext(aszSkillType[pmr->n.stMove]));

        if (!pms->nMatchTo || (pms->nMatchTo && !fOutputMWC))
            fprintf(pf, " (%+7.3f)</span></p>\n", pmr->ml.amMoves[pmr->n.iMove].rScore - pmr->ml.amMoves[0].rScore);
        else
            fprintf(pf, " (%+6.3f%%)</span></p>\n",
                    100.0f *
                    eq2mwc(pmr->ml.amMoves[pmr->n.iMove].rScore, &ci) -
                    100.0f * eq2mwc(pmr->ml.amMoves[0].rScore, &ci));

    }

    if (pmr->lt != LUCK_NONE) {

        /* joker */

        fprintf(pf, "<p><span %s>", GetStyle(CLASS_JOKER, hecss));
        fprintf(pf, _("Alert: %s roll!"), gettext(aszLuckType[pmr->lt]));

        if (!pms->nMatchTo || (pms->nMatchTo && !fOutputMWC))
            fprintf(pf, " (%+7.3f)</span></p>\n", pmr->rLuck);
        else
            fprintf(pf, " (%+6.3f%%)</span></p>\n", 100.0f * eq2mwc(pmr->rLuck, &ci) - 100.0f * eq2mwc(0.0f, &ci));

    }


    /* table header */

    fprintf(pf,
            "<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" %s>\n" "<tr>\n", GetStyle(CLASS_MOVETABLE, hecss));
    fprintf(pf, "<th %s colspan=\"2\">%s</th>\n",
            GetStyleGeneral(hecss, CLASS_MOVEHEADER, CLASS_MOVENUMBER, -1), _("#"));
    fprintf(pf, "<th %s>%s</th>\n", GetStyleGeneral(hecss, CLASS_MOVEHEADER, CLASS_MOVEPLY, -1), _("Ply"));
    fprintf(pf, "<th %s>%s</th>\n", GetStyleGeneral(hecss, CLASS_MOVEHEADER, CLASS_MOVEMOVE, -1), _("Move"));
    fprintf(pf,
            "<th %s>%s</th>\n" "</tr>\n",
            GetStyleGeneral(hecss, CLASS_MOVEHEADER, CLASS_MOVEEQUITY, -1),
            (!pms->nMatchTo || (pms->nMatchTo && !fOutputMWC)) ? _("Equity") : _("MWC"));


    if (pmr->ml.cMoves) {

        for (i = 0; i < pmr->ml.cMoves; i++) {

            if (i >= exsExport.nMoves && i != pmr->n.iMove)
                continue;

            if (i == pmr->n.iMove)
                fprintf(pf, "<tr %s>\n", GetStyle(CLASS_MOVETHEMOVE, hecss));
            else if (i % 2)
                fprintf(pf, "<tr %s>\n", GetStyle(CLASS_MOVEODD, hecss));
            else
                fputs("<tr>\n", pf);

            /* selected move or not */

            fprintf(pf, "<td %s>%s</td>\n", GetStyle(CLASS_MOVENUMBER, hecss), (i == pmr->n.iMove) ? bullet : "&nbsp;");

            /* move no */

            if (i != pmr->n.iMove || i != pmr->ml.cMoves - 1 || pmr->ml.cMoves == 1 || i < exsExport.nMoves)
                fprintf(pf, "<td %s>%d</td>\n", GetStyle(CLASS_MOVENUMBER, hecss), i + 1);
            else
                fprintf(pf, "<td %s>\?\?</td>\n", GetStyle(CLASS_MOVENUMBER, hecss));

            /* ply */

            switch (pmr->ml.amMoves[i].esMove.et) {
            case EVAL_NONE:
                fprintf(pf, "<td %s>n/a</td>\n", GetStyle(CLASS_MOVEPLY, hecss));
                break;
            case EVAL_EVAL:
                fprintf(pf, "<td %s>%u</td>\n", GetStyle(CLASS_MOVEPLY, hecss), pmr->ml.amMoves[i].esMove.ec.nPlies);
                break;
            case EVAL_ROLLOUT:
                fprintf(pf, "<td %s>R</td>\n", GetStyle(CLASS_MOVEPLY, hecss));
                break;
            }

            /* move */

            fprintf(pf,
                    "<td %s>%s</td>\n",
                    GetStyle(CLASS_MOVEMOVE, hecss),
                    FormatMove(sz, (ConstTanBoard) pms->anBoard, pmr->ml.amMoves[i].anMove));

            /* equity */

            rEq = pmr->ml.amMoves[i].rScore;
            rEqTop = pmr->ml.amMoves[0].rScore;

            if (i)
                fprintf(pf,
                        "<td %s>%s (%s)</td>\n",
                        GetStyle(CLASS_MOVEEQUITY, hecss),
                        OutputEquity(rEq, &ci, TRUE), OutputEquityDiff(rEq, rEqTop, &ci));
            else
                fprintf(pf, "<td %s>%s</td>\n", GetStyle(CLASS_MOVEEQUITY, hecss), OutputEquity(rEq, &ci, TRUE));

            /* end row */

            fprintf(pf, "</tr>\n");

            /*
             * print row with detailed probabilities 
             */

            if (exsExport.fMovesDetailProb) {

                float *ar = pmr->ml.amMoves[i].arEvalMove;

                /* percentages */

                if (i == pmr->n.iMove)
                    fprintf(pf, "<tr %s>\n", GetStyle(CLASS_MOVETHEMOVE, hecss));
                else if (i % 2)
                    fprintf(pf, "<tr %s>\n", GetStyle(CLASS_MOVEODD, hecss));
                else
                    fputs("<tr>\n", pf);

                fputs("<td colspan=\"3\">&nbsp;</td>\n", pf);


                fputs("<td>", pf);


                switch (pmr->ml.amMoves[i].esMove.et) {
                case EVAL_EVAL:
                    fputs(OutputPercents(ar, TRUE), pf);
                    break;
                case EVAL_ROLLOUT:
                    printRolloutTable(pf, NULL, (float (*)[NUM_ROLLOUT_OUTPUTS])
                                      pmr->ml.amMoves[i].arEvalMove, (float (*)[NUM_ROLLOUT_OUTPUTS])
                                      pmr->ml.amMoves[i].arEvalStdDev,
                                      &ci, 1, pmr->ml.amMoves[i].esMove.rc.fCubeful, FALSE, hecss);
                    break;
                default:
                    break;
                }

                fputs("</td>\n", pf);

                fputs("<td>&nbsp;</td>\n", pf);

                fputs("</tr>\n", pf);
            }

            /*
             * Write row with move parameters 
             */

            if (exsExport.afMovesParameters[pmr->ml.amMoves[i].esMove.et - 1]) {

                evalsetup *pes = &pmr->ml.amMoves[i].esMove;

                switch (pes->et) {
                case EVAL_EVAL:

                    if (i == pmr->n.iMove)
                        fprintf(pf, "<tr %s>\n", GetStyle(CLASS_MOVETHEMOVE, hecss));
                    else if (i % 2)
                        fprintf(pf, "<tr %s>\n", GetStyle(CLASS_MOVEODD, hecss));
                    else
                        fputs("<tr>\n", pf);

                    fprintf(pf, "<td colspan=\"3\">&nbsp;</td>\n");

                    fputs("<td>", pf);
                    fputs(OutputEvalContext(&pes->ec, TRUE), pf);
                    fputs("</td>\n", pf);

                    fputs("<td>&nbsp;</td>\n", pf);

                    fputs("</tr>\n", pf);

                    break;

                case EVAL_ROLLOUT:
                    {
                        char *sz = g_strdup(OutputRolloutContext(NULL, &pes->rc));
                        char *pcS = sz, *pcE;

                        while ((pcE = strstr(pcS, "\n"))) {

                            *pcE = 0;

                            if (i == pmr->n.iMove)
                                fprintf(pf, "<tr %s>\n", GetStyle(CLASS_MOVETHEMOVE, hecss));
                            else if (i % 2)
                                fprintf(pf, "<tr %s>\n", GetStyle(CLASS_MOVEODD, hecss));
                            else
                                fputs("<tr>\n", pf);

                            fprintf(pf, "<td colspan=\"3\">&nbsp;</td>\n");

                            fputs("<td>", pf);
                            fputs(pcS, pf);
                            fputs("</td>\n", pf);

                            fputs("<td>&nbsp;</td>\n", pf);

                            fputs("</tr>\n", pf);

                            pcS = pcE + 1;

                        }

                        g_free(sz);

                        break;

                    }
                default:

                    break;

                }

            }


        }

    } else {

        if (pmr->n.anMove[0] >= 0)
            /* no movelist saved */
            fprintf(pf,
                    "<tr %s><td>&nbsp;</td><td>&nbsp;</td>"
                    "<td>&nbsp;</td><td>%s</td><td>&nbsp;</td></tr>\n",
                    GetStyle(CLASS_MOVETHEMOVE, hecss), FormatMove(sz, (ConstTanBoard) pms->anBoard, pmr->n.anMove));
        else
            /* no legal moves */
            /* FIXME: output equity?? */
            fprintf(pf,
                    "<tr %s><td>&nbsp;</td><td>&nbsp;</td>"
                    "<td>&nbsp;</td><td>%s</td><td>&nbsp;</td></tr>\n",
                    GetStyle(CLASS_MOVETHEMOVE, hecss), _("Cannot move"));

    }


    fprintf(pf, "</table>\n");

    fputs("\n<!-- End Move Analysis -->\n\n", pf);

    return;

}





/*
 * Print cube analysis
 *
 * Input:
 *  pf: output file
 *  pms: match state
 *  pmr: current move record
 *  szImageDir: URI to images
 *  szExtension: extension of images
 *
 */

static void
HTMLAnalysis(FILE * pf, matchstate * pms, moverecord * pmr,
             const char *szImageDir, const char *szExtension, const htmlexporttype het, const htmlexportcss hecss)
{

    char sz[1024];

    switch (pmr->mt) {

    case MOVE_NORMAL:

        fprintf(pf, "<p>");

        if (het == HTML_EXPORT_TYPE_FIBS2HTML)
            printImage(pf, szImageDir, "b-indent", szExtension, "", hecss, het);

        if (pmr->n.anMove[0] >= 0)
            fprintf(pf,
                    _("%s%s moves %s"), bullet,
                    ap[pmr->fPlayer].szName, FormatMove(sz, (ConstTanBoard) pms->anBoard, pmr->n.anMove));
        else if (!pmr->ml.cMoves)
            fprintf(pf, _("%s%s cannot move"), bullet, ap[pmr->fPlayer].szName);

        fputs("</p>\n", pf);

        /* HTMLRollAlert ( pf, pms, pmr, szImageDir, szExtension ); */

        if (exsExport.fIncludeAnalysis) {
            HTMLPrintCubeAnalysis(pf, pms, pmr, szImageDir, szExtension, het, hecss);

            HTMLPrintMoveAnalysis(pf, pms, pmr, szImageDir, szExtension, het, hecss);
        }

        break;

    case MOVE_TAKE:
    case MOVE_DROP:
    case MOVE_DOUBLE:

        fprintf(pf, "<p>");

        if (het == HTML_EXPORT_TYPE_FIBS2HTML)
            printImage(pf, szImageDir, "b-indent", szExtension, "", hecss, het);

        if (pmr->mt == MOVE_DOUBLE)
            fprintf(pf, "%s%s doubles</p>\n", bullet, ap[pmr->fPlayer].szName);
        else
            fprintf(pf,
                    "%s%s %s</p>\n", bullet,
                    ap[pmr->fPlayer].szName, (pmr->mt == MOVE_TAKE) ? _("accepts") : _("rejects"));

        if (exsExport.fIncludeAnalysis)
            HTMLPrintCubeAnalysis(pf, pms, pmr, szImageDir, szExtension, het, hecss);

        break;

    default:
        break;

    }

}


/*
 * Dump statcontext
 *
 * Input:
 *   pf: output file
 *   statcontext to dump
 *
 */

static void
HTMLDumpStatcontext(FILE * pf, const statcontext * psc,
                    int nMatchTo, const int iGame, const htmlexportcss hecss, const gchar * header)
{

    fprintf(pf, "\n<!-- %s Statistics -->\n\n", (iGame >= 0) ? "Game" : "Match");

    fprintf(pf, "<table %s>\n", GetStyle(CLASS_STATTABLE, hecss));

    printStatTableHeader(pf, hecss, header);

    fprintf(pf,
            "<tr %s>\n"
            "<th>%s</th><th>%s</th><th>%s</th>\n"
            "</tr>\n", GetStyle(CLASS_STATTABLEHEADER, hecss), _("Player"), ap[0].szName, ap[1].szName);

    /* checker play */

    if (psc->fMoves) {

        GList *list = formatGS(psc, nMatchTo, FORMATGS_CHEQUER);
        GList *pl;

        printStatTableHeader(pf, hecss, _("Checker play statistics"));

        for (pl = g_list_first(list); pl; pl = g_list_next(pl)) {

            char **aasz = pl->data;

            printStatTableRow(pf, aasz[0], "%s", aasz[1], aasz[2]);

        }

        freeGS(list);

    }

    /* dice stat */

    if (psc->fDice) {

        GList *list = formatGS(psc, nMatchTo, FORMATGS_LUCK);
        GList *pl;

        printStatTableHeader(pf, hecss, _("Luck statistics"));

        for (pl = g_list_first(list); pl; pl = g_list_next(pl)) {

            char **aasz = pl->data;

            printStatTableRow(pf, aasz[0], "%s", aasz[1], aasz[2]);

        }

        freeGS(list);

    }

    /* cube statistics */

    if (psc->fCube) {

        GList *list = formatGS(psc, nMatchTo, FORMATGS_CUBE);
        GList *pl;

        printStatTableHeader(pf, hecss, _("Cube statistics"));

        for (pl = g_list_first(list); pl; pl = g_list_next(pl)) {

            char **aasz = pl->data;

            printStatTableRow(pf, aasz[0], "%s", aasz[1], aasz[2]);

        }

        freeGS(list);

    }

    /* overall rating */

    {

        GList *list = formatGS(psc, nMatchTo, FORMATGS_OVERALL);
        GList *pl;

        printStatTableHeader(pf, hecss, _("Overall statistics"));

        for (pl = g_list_first(list); pl; pl = g_list_next(pl)) {

            char **aasz = pl->data;

            printStatTableRow(pf, aasz[0], "%s", aasz[1], aasz[2]);

        }

        freeGS(list);

    }


    fprintf(pf, "</table>\n");

    fprintf(pf, "\n<!-- End %s Statistics -->\n\n", (iGame >= 0) ? "Game" : "Match");

}


static void
HTMLPrintComment(FILE * pf, const moverecord * pmr, const htmlexportcss hecss)
{

    char *sz = pmr->sz;

    if (sz) {


        fputs("<!-- Annotation -->\n\n", pf);

        fprintf(pf, "<br />\n" "<div %s>", GetStyle(CLASS_COMMENTHEADER, hecss));
        fputs(_("Annotation"), pf);
        fputs("</div>\n", pf);

        fprintf(pf, "<div %s>", GetStyle(CLASS_COMMENT, hecss));

        while (*sz) {

            if (*sz == '\n')
                fputs("<br />\n", pf);
            else
                fputc(*sz, pf);

            sz++;

        }

        fputs("</div>\n\n", pf);

        fputs("<!-- End Annotation -->\n\n", pf);


    }


}


static void
HTMLPrintMI(FILE * pf, const char *szTitle, const char *sz)
{

    gchar **ppch;
    gchar *pchToken;
    int i;

    if (!sz || !*sz)
        return;

    fprintf(pf, "<tr valign=\"top\">" "<td style=\"padding-right: 2em\">%s</td><td>", szTitle);

    ppch = g_strsplit(sz, "\n", -1);
    for (i = 0; (pchToken = ppch[i]); ++i) {
        if (i)
            fputs("<br />\n", pf);
        fputs(pchToken, pf);
    }

    g_strfreev(ppch);

    fputs("</td></tr>\n", pf);

}

static void
HTMLMatchInfo(FILE * pf, const matchinfo * pmi, const htmlexportcss UNUSED(hecss))
{

    int i;
    char sz[80];
    struct tm tmx;

    if (!pmi->nYear &&
        !pmi->pchRating[0] && !pmi->pchRating[1] &&
        !pmi->pchEvent && !pmi->pchRound && !pmi->pchPlace && !pmi->pchAnnotator && !pmi->pchComment)
        /* no match information -- don't print anything */
        return;

    fputs("\n<!-- Match Information -->\n\n", pf);

    fputs("<hr />", pf);

    fprintf(pf, "<h2>%s</h2>\n", _("Match Information"));

    fputs("<table border=\"0\">\n", pf);

    /* ratings */

    for (i = 0; i < 2; ++i)
        if (pmi->pchRating[i]) {
            gchar *pch = g_strdup_printf(_("%s's rating"), ap[i].szName);
            HTMLPrintMI(pf, pch, pmi->pchRating[i] ? pmi->pchRating[i] : _("n/a"));
            g_free(pch);
        }

    /* date */

    if (pmi->nYear) {

        tmx.tm_year = pmi->nYear - 1900;
        tmx.tm_mon = pmi->nMonth - 1;
        tmx.tm_mday = pmi->nDay;
        strftime(sz, sizeof(sz), "%B %d, %Y", &tmx);
        HTMLPrintMI(pf, _("Date"), sz);
    }

    /* event, round, place and annotator */

    HTMLPrintMI(pf, _("Event"), pmi->pchEvent);
    HTMLPrintMI(pf, _("Round"), pmi->pchRound);
    HTMLPrintMI(pf, _("Place"), pmi->pchPlace);
    HTMLPrintMI(pf, _("Annotator"), pmi->pchAnnotator);
    HTMLPrintMI(pf, _("Comment"), pmi->pchComment);

    fputs("</table>\n", pf);

    fputs("\n<!-- End Match Information -->\n\n", pf);

}

/*
 * Export a game in HTML
 *
 * Input:
 *   pf: output file
 *   plGame: list of moverecords for the current game
 *
 */

static void
ExportGameHTML(FILE * pf, listOLD * plGame, const char *szImageDir,
               const char *szExtension,
               const htmlexporttype het,
               const htmlexportcss hecss, const int iGame, const int fLastGame, char *aszLinks[4])
{
    listOLD *pl;
    moverecord *pmr;
    matchstate msExport;
    matchstate msOrig;
    int iMove = 0;
    statcontext *psc = NULL;
    static statcontext scTotal;
    xmovegameinfo *pmgi = NULL;
    listOLD *pl_hint = NULL;
    statcontext *psc_rel = NULL;

    msOrig.nMatchTo = 0;

    if (!iGame)
        IniStatcontext(&scTotal);

    updateStatisticsGame(plGame);

    if (game_is_last(plGame))
        pl_hint = game_add_pmr_hint(plGame);

    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext) {

        pmr = pl->p;

        FixMatchState(&msExport, pmr);

        switch (pmr->mt) {

        case MOVE_GAMEINFO:

            ApplyMoveRecord(&msExport, plGame, pmr);

            HTMLPrologue(pf, &msExport, iGame, aszLinks, het, hecss);

            if (exsExport.fIncludeMatchInfo)
                HTMLMatchInfo(pf, &mi, hecss);

            msOrig = msExport;
            pmgi = &pmr->g;

            psc = &pmr->g.sc;

            AddStatcontext(psc, &scTotal);

            /* FIXME: game introduction */
            break;

        case MOVE_NORMAL:

            if (pmr->fPlayer != msExport.fMove) {
                SwapSides(msExport.anBoard);
                msExport.fMove = pmr->fPlayer;
            }

            msExport.fTurn = msExport.fMove = pmr->fPlayer;
            msExport.anDice[0] = pmr->anDice[0];
            msExport.anDice[1] = pmr->anDice[1];

            HTMLBoardHeader(pf, &msExport, het, hecss, iGame, iMove, TRUE);

            printHTMLBoard(pf, &msExport, msExport.fTurn, szImageDir, szExtension, het, hecss);
            HTMLAnalysis(pf, &msExport, pmr, szImageDir, szExtension, het, hecss);

            iMove++;

            break;

        case MOVE_DOUBLE:
        case MOVE_TAKE:
        case MOVE_DROP:

            HTMLBoardHeader(pf, &msExport, het, hecss, iGame, iMove, TRUE);

            printHTMLBoard(pf, &msExport, msExport.fTurn, szImageDir, szExtension, het, hecss);

            HTMLAnalysis(pf, &msExport, pmr, szImageDir, szExtension, het, hecss);

            iMove++;

            break;


        default:

            break;

        }

        if (exsExport.fIncludeAnnotation)
            HTMLPrintComment(pf, pmr, hecss);

        ApplyMoveRecord(&msExport, plGame, pmr);

    }

    if (pl_hint)
        game_remove_pmr_hint(pl_hint);

    if (pmgi && pmgi->fWinner != -1) {

        /* print game result */

        fprintf(pf,
                ngettext("<p %s>%s wins %d point</p>\n",
                         "<p %s>%s wins %d points</p>\n",
                         pmgi->nPoints), GetStyle(CLASS_RESULT, hecss), ap[pmgi->fWinner].szName, pmgi->nPoints);
    }

    if (psc) {
        char *header = g_strdup_printf(_("Game statistics for game %d"), iGame + 1);
        HTMLDumpStatcontext(pf, psc, msOrig.nMatchTo, iGame, hecss, header);
        g_free(header);
    }


    if (fLastGame) {
        const gchar *header;

        /* match statistics */
        header = ms.nMatchTo ? _("Match statistics") : _("Session statistics");

        fprintf(pf, "<hr />\n");
        HTMLDumpStatcontext(pf, &scTotal, msOrig.nMatchTo, -1, hecss, header);
        psc_rel = relational_player_stats_get(ap[0].szName, ap[1].szName);
        if (psc_rel) {
            HTMLDumpStatcontext(pf, psc_rel, 0, -1, hecss, _("Statistics from database"));
            g_free(psc_rel);
        }

    }


    HTMLEpilogue(pf, &msExport, aszLinks, hecss);

}

/*
 * Open file gnubg.css with same path as requested html file
 *
 * If the gnubg.css file already exists NULL is returned 
 * (and gnubg.css is NOT overwritten)
 *
 */

static FILE *
OpenCSS(const char *sz)
{

    gchar *pch = g_strdup(sz);
    gchar *pchBase = g_path_get_dirname(pch);
    gchar *pchCSS;
    FILE *pf;

    pchCSS = g_build_filename(pchBase, "gnubg.css", NULL);

    if (g_file_test(pchCSS, G_FILE_TEST_EXISTS)) {
        outputf(_("gnubg.css is not written since it already exist in \"%s\"\n"), pchBase);
        pf = NULL;
    } else if (!(pf = g_fopen(pchCSS, "w"))) {
        outputerr(pchCSS);
    }

    g_free(pch);
    g_free(pchBase);
    g_free(pchCSS);

    return pf;

}


static void
check_for_html_images(gchar * path)
{
    gchar *folder;
    gchar *img;
    char *url = exsExport.szHTMLPictureURL;

    if (url && g_path_is_absolute(url)) {
        if (!g_file_test(url, G_FILE_TEST_EXISTS))
            CommandExportHTMLImages(url);
    } else {
        folder = g_path_get_dirname(path);
        img = g_build_filename(folder, url, NULL);
        if (!g_file_test(img, G_FILE_TEST_EXISTS))
            CommandExportHTMLImages(img);
        g_free(img);
        g_free(folder);
    }
}


extern void
CommandExportGameHtml(char *sz)
{

    FILE *pf;

    sz = NextToken(&sz);

    if (!plGame) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to (see `help export " "game html')."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if (!(pf = g_fopen(sz, "w"))) {
        outputerr(sz);
        return;
    }

    if (exsExport.het == HTML_EXPORT_TYPE_GNU)
        check_for_html_images(sz);

    ExportGameHTML(pf, plGame,
                   exsExport.szHTMLPictureURL, exsExport.szHTMLExtension,
                   exsExport.het, exsExport.hecss, getGameNumber(plGame), FALSE, NULL);


    if (pf != stdout)
        fclose(pf);

    setDefaultFileName(sz);

    /* external stylesheet */

    if (exsExport.hecss == HTML_EXPORT_CSS_EXTERNAL)
        if ((pf = OpenCSS(sz))) {
            WriteStyleSheet(pf, exsExport.hecss);
            fclose(pf);
        }
}


extern void
CommandExportMatchHtml(char *sz)
{

    FILE *pf;
    listOLD *pl;
    int nGames;
    char *aszLinks[4], *filenames[4];
    char *szCurrent;
    int i, j;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to (see `help export " "match html')."));
        return;
    }

    if (exsExport.het == HTML_EXPORT_TYPE_GNU)
        check_for_html_images(sz);

    /* Find number of games in match */

    for (pl = lMatch.plNext, nGames = 0; pl != &lMatch; pl = pl->plNext, nGames++);

    for (pl = lMatch.plNext, i = 0; pl != &lMatch; pl = pl->plNext, i++) {

        szCurrent = filename_from_iGame(sz, i);
        filenames[0] = filename_from_iGame(sz, 0);
        aszLinks[0] = g_path_get_basename(filenames[0]);
        filenames[1] = aszLinks[1] = NULL;
        if (i > 0) {
            filenames[1] = filename_from_iGame(sz, i - 1);
            aszLinks[1] = g_path_get_basename(filenames[1]);
        }

        filenames[2] = aszLinks[2] = NULL;
        if (i < nGames - 1) {
            filenames[2] = filename_from_iGame(sz, i + 1);
            aszLinks[2] = g_path_get_basename(filenames[2]);
        }



        filenames[3] = filename_from_iGame(sz, nGames - 1);
        aszLinks[3] = g_path_get_basename(filenames[3]);
        if (!i) {

            if (!confirmOverwrite(sz, fConfirmSave)) {
                for (j = 0; j < 4; j++)
                    g_free(filenames[j]);

                g_free(szCurrent);
                return;
            }

            setDefaultFileName(sz);

        }


        if (!strcmp(szCurrent, "-"))
            pf = stdout;
        else if (!(pf = g_fopen(szCurrent, "w"))) {
            outputerr(szCurrent);
            for (j = 0; j < 4; j++)
                g_free(filenames[j]);

            g_free(szCurrent);
            return;
        }

        ExportGameHTML(pf, pl->p,
                       exsExport.szHTMLPictureURL, exsExport.szHTMLExtension,
                       exsExport.het, exsExport.hecss, i, i == nGames - 1, aszLinks);

        for (j = 0; j < 4; j++)
            g_free(filenames[j]);

        g_free(szCurrent);

        if (pf != stdout)
            fclose(pf);

    }

    /* external stylesheet */

    if (exsExport.hecss == HTML_EXPORT_CSS_EXTERNAL)
        if ((pf = OpenCSS(sz))) {
            WriteStyleSheet(pf, exsExport.hecss);
            fclose(pf);
        }
}


extern void
CommandExportPositionHtml(char *sz)
{

    FILE *pf;
    int fHistory;
    moverecord *pmr;
    int iMove;

    sz = NextToken(&sz);

    if (ms.gs == GAME_NONE) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to (see `help export " "position html')."));
        return;
    }
    pmr = get_current_moverecord(&fHistory);
    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if (!(pf = g_fopen(sz, "w"))) {
        outputerr(sz);
        return;
    }

    if (exsExport.het == HTML_EXPORT_TYPE_GNU)
        check_for_html_images(sz);

    HTMLPrologue(pf, &ms, getGameNumber(plGame), NULL, exsExport.het, exsExport.hecss);

    if (exsExport.fIncludeMatchInfo)
        HTMLMatchInfo(pf, &mi, exsExport.hecss);

    if (fHistory)
        iMove = getMoveNumber(plGame, pmr) - 1;
    else if (plLastMove)
        iMove = getMoveNumber(plGame, plLastMove->p);
    else
        iMove = -1;

    HTMLBoardHeader(pf, &ms, exsExport.het, exsExport.hecss, getGameNumber(plGame), iMove, TRUE);

    printHTMLBoard(pf, &ms, ms.fTurn,
                   exsExport.szHTMLPictureURL, exsExport.szHTMLExtension, exsExport.het, exsExport.hecss);

    if (pmr) {

        HTMLAnalysis(pf, &ms, pmr,
                     exsExport.szHTMLPictureURL, exsExport.szHTMLExtension, exsExport.het, exsExport.hecss);

        if (exsExport.fIncludeAnnotation)
            HTMLPrintComment(pf, pmr, exsExport.hecss);

    }

    HTMLEpilogue(pf, &ms, NULL, exsExport.hecss);

    if (pf != stdout)
        fclose(pf);

    setDefaultFileName(sz);

    /* external stylesheet */

    if (exsExport.hecss == HTML_EXPORT_CSS_EXTERNAL)
        if ((pf = OpenCSS(sz))) {
            WriteStyleSheet(pf, exsExport.hecss);
            fclose(pf);
        }
}


static void
ExportPositionGammOnLine(FILE * pf)
{
    int fHistory;
    moverecord *pmr = get_current_moverecord(&fHistory);

    if (!pmr) {
        outputerrf(_("Unable to export this position"));
        return;
    }

    fputs("\n<!-- Score -->\n\n", pf);

    fputs("<strong>", pf);

    if (ms.nMatchTo > 1)
        fprintf(pf,
                gettext("Match to %d points - %s %d, %s %d%s"),
                ms.nMatchTo,
                ap[0].szName, ms.anScore[0], ap[1].szName, ms.anScore[1],
                ms.fCrawford ? _(", Crawford game") : (ms.fPostCrawford ? _(", post-Crawford play") : ""));
    else if (ms.nMatchTo == 1)
        fprintf(pf, _("Match to 1 point"));
    else
        fprintf(pf, gettext("Unlimited game, %s"), ms.fJacoby ? _("Jacoby rule") : _("without Jacoby rule"));

    fputs("</strong>\n", pf);

    fputs("\n<!-- End Score -->\n\n", pf);

    printHTMLBoard(pf, &ms, ms.fTurn, "../Images/", "gif", HTML_EXPORT_TYPE_BBS, HTML_EXPORT_CSS_INLINE);

    if (pmr) {
        HTMLAnalysis(pf, &ms, pmr, "../Images/", "gif", HTML_EXPORT_TYPE_BBS, HTML_EXPORT_CSS_INLINE);

        HTMLPrintComment(pf, pmr, HTML_EXPORT_CSS_INLINE);

    }

    HTMLEpilogueComment(pf);

}


extern void
CommandExportPositionGammOnLine(char *sz)
{

    FILE *pf;

    sz = NextToken(&sz);

    if (ms.gs == GAME_NONE) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to (see `help export " "position html')."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if (!(pf = g_fopen(sz, "w"))) {
        outputerr(sz);
        return;
    }

    ExportPositionGammOnLine(pf);

    if (pf != stdout)
        fclose(pf);

}

extern void
CommandExportPositionGOL2Clipboard(char *UNUSED(sz))
{
    char *szClipboard;
    long l;
    FILE *pf;
    char *tmpFile;

    if (ms.gs == GAME_NONE) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    /* get tmp file */

    pf = GetTemporaryFile(NULL, &tmpFile);

    /* generate file */

    ExportPositionGammOnLine(pf);

    /* find size of file */

    if (fseek(pf, 0L, SEEK_END)) {
        outputerr("temporary file");
        return;
    }

    l = ftell(pf);

    if (fseek(pf, 0L, SEEK_SET)) {
        outputerr("temporary file");
        return;
    }

    /* copy file to clipboard */

    szClipboard = (char *) malloc(l + 1);

    if (fread(szClipboard, 1, l, pf) != (unsigned long) l) {
        outputerr("temporary file");
    } else {
        szClipboard[l] = 0;
        TextToClipboard(szClipboard);
    }

    free(szClipboard);
    fclose(pf);
    g_unlink(tmpFile);
    g_free(tmpFile);
}
