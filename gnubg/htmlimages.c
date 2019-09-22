/*
 * htmlimages.c
 *
 * by Gary Wong <gtw@gnu.org>, 1997-2002.
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
 * $Id: htmlimages.c,v 1.55 2014/07/20 20:56:28 plm Exp $
 */

#include "config.h"
#include <errno.h>

#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdlib.h>
#include <glib.h>
#ifdef WIN32
#include <io.h>
#endif

#include "backgammon.h"
#include "export.h"
#include <glib/gstdio.h>
#include "render.h"
#include "renderprefs.h"
#include "boardpos.h"
#include "boarddim.h"

#if HAVE_LIBPNG

static char *szFile, *pchFile;
static int imagesWritten;

#define NUM_IMAGES 344

/* Overall board size */
static int s;
/* Size of cube and dice - now same as rest of board */
#define ss s

/* Helpers for 2d position in arrays */
#define boardStride (BOARD_WIDTH * s * 3)
#define coord(x, y) ((x) * s * 3 + (y) * s * boardStride)
#define coordStride(x, y, stride) ((x) * s * 3 + (y) * s * stride)

static unsigned char *auchBoard, *auchChequer[2], *auchChequerLabels, *auchLo, *auchHi,
    *auchLoRev, *auchHiRev, *auchCube, *auchCubeFaces, *auchDice[2], *auchPips[2],
    *auchLabel, *auchOff[2], *auchMidBoard, *auchBar[2], *auchPoint[2][2][2];

static unsigned short *asRefract[2];

#if USE_GTK
static unsigned char *auchArrow[2];
#endif
static unsigned char *auchMidlb;

static void
WriteImageStride(unsigned char *img, int stride, int cx, int cy)
{
    if (WritePNG(szFile, img, stride, cx, cy) == -1)
        outputf("Error creating file %s\n", szFile);
    imagesWritten++;
    ProgressValueAdd(1);
}

static void
WriteStride(const char *name, unsigned char *img, int stride, int cx, int cy)
{
    strcpy(pchFile, name);
    WriteImageStride(img, stride, cx, cy);
}

static void
WriteImage(unsigned char *img, int cx, int cy)
{
    WriteImageStride(img, boardStride, cx, cy);
}

static void
Write(const char *name, unsigned char *img, int cx, int cy)
{
    strcpy(pchFile, name);
    WriteImage(img, cx, cy);
}

static void
DrawArrow(int side, int player)
{                               /* side 0 = left, 1 = right */
    int offset_x = 0;

    memcpy(auchMidlb, auchBoard, BOARD_WIDTH * s * BOARD_HEIGHT * s * 3);

#if USE_GTK
    {
        int x, y;
        ArrowPosition(side /* rd.fClockwise */ , player, s, &x, &y);

        AlphaBlendClip2(auchMidlb, boardStride,
                        x, y,
                        BOARD_WIDTH * s, BOARD_HEIGHT * s,
                        auchMidlb, boardStride,
                        x, y, auchArrow[player], s * ARROW_WIDTH * 4, 0, 0, s * ARROW_WIDTH, s * ARROW_HEIGHT);
    }
#endif

    sprintf(pchFile, "b-mid%cb-%c.png", side ? 'r' : 'l', player ? 'o' : 'x');
    if (side == 1)
        offset_x = BOARD_WIDTH - BEAROFF_WIDTH;

    WriteImage(auchMidlb + coord(offset_x, BOARD_HEIGHT / 2 - BOARD_CENTER_HEIGHT / 2),
               BEAROFF_WIDTH * s, BOARD_CENTER_HEIGHT * s);
}

static void
DrawPips(unsigned char *auchDest, int nStride, unsigned char *auchPip, int n)
{
    int ix, iy, afPip[9];

    afPip[0] = afPip[8] = (n == 2) || (n == 3) || (n == 4) || (n == 5) || (n == 6);
    afPip[1] = afPip[7] = 0;
    afPip[2] = afPip[6] = (n == 4) || (n == 5) || (n == 6);
    afPip[3] = afPip[5] = n == 6;
    afPip[4] = n & 1;

    for (iy = 0; iy < 3; iy++) {
        for (ix = 0; ix < 3; ix++) {
            if (afPip[iy * 3 + ix])
                CopyArea(auchDest + (int) (1.5 * (ix + 1) * ss) * 3 +
                         ((int) (1.5 * (iy + 1) * ss) * nStride), nStride, auchPip, ss * 3, ss, ss);
        }
    }
}

static void
WriteBorder(const char *file, unsigned char *auchSrc, unsigned char *auchBoardSrc)
{
    CopyArea(auchLabel, boardStride, auchBoardSrc, boardStride, BOARD_WIDTH * s, BORDER_HEIGHT * s);

    AlphaBlendClip(auchLabel, boardStride, 0, 0,
                   BOARD_WIDTH * s, BORDER_HEIGHT * s,
                   auchLabel, boardStride, 0, 0,
                   auchSrc, BOARD_WIDTH * s * 4, 0, 0, BOARD_WIDTH * s, BORDER_HEIGHT * s);

    Write(file, auchLabel, BOARD_WIDTH * s, BORDER_HEIGHT * s);
}

static void
WriteImages(void)
{
    int i, j, k;
    imagesWritten = 0;

    /* top border-high numbers */
    WriteBorder("b-hitop.png", auchHi, auchBoard);
    WriteBorder("b-hitopr.png", auchHiRev, auchBoard);

    /* top border-low numbers */
    WriteBorder("b-lotop.png", auchLo, auchBoard);
    WriteBorder("b-lotopr.png", auchLoRev, auchBoard);

    /* empty points */
    Write("b-gd.png", auchBoard + coord(BEAROFF_WIDTH, BORDER_HEIGHT), POINT_WIDTH * s, POINT_HEIGHT * s);

    Write("b-rd.png", auchBoard + coord(BEAROFF_WIDTH + POINT_WIDTH, BORDER_HEIGHT), POINT_WIDTH * s, POINT_HEIGHT * s);

    Write("b-ru.png", auchBoard + coord(BEAROFF_WIDTH, BOARD_HEIGHT / 2 + BOARD_CENTER_HEIGHT / 2),
          POINT_WIDTH * s, POINT_HEIGHT * s);

    Write("b-gu.png", auchBoard + coord(BEAROFF_WIDTH + POINT_WIDTH, BOARD_HEIGHT / 2 + BOARD_CENTER_HEIGHT / 2),
          POINT_WIDTH * s, POINT_HEIGHT * s);

    /* upper left bearoff tray */
    Write("b-loff-x0.png", auchBoard + coord(0, BORDER_HEIGHT), BEAROFF_WIDTH * s, BEAROFF_HEIGHT * s);

    /* upper right bearoff tray */
    Write("b-roff-x0.png", auchBoard + coord(BOARD_WIDTH - BEAROFF_WIDTH, BORDER_HEIGHT),
          BEAROFF_WIDTH * s, BEAROFF_HEIGHT * s);

    /* lower right bearoff tray */
    Write("b-roff-o0.png",
          auchBoard + coord(BOARD_WIDTH - BEAROFF_WIDTH, BORDER_HEIGHT + POINT_HEIGHT + BOARD_CENTER_HEIGHT),
          BEAROFF_WIDTH * s, BEAROFF_HEIGHT * s);

    /* lower left bearoff tray */
    Write("b-loff-o0.png", auchBoard + coord(0, BORDER_HEIGHT + POINT_HEIGHT + BOARD_CENTER_HEIGHT),
          BEAROFF_WIDTH * s, BEAROFF_HEIGHT * s);

    /* bar top */
    Write("b-ct.png", auchBoard + coord(BOARD_WIDTH / 2 - BAR_WIDTH / 2, BORDER_HEIGHT),
          BAR_WIDTH * s, CUBE_HEIGHT * s);

    /* bearoff tray dividers */
    /* 4 arrows, left+right side for each player */
    DrawArrow(0, 0);
    DrawArrow(0, 1);
    DrawArrow(1, 0);
    DrawArrow(1, 1);

    /* left bearoff centre */
    strcpy(pchFile, "b-midlb.png");
    WriteImage(auchBoard + coord(0, BOARD_HEIGHT / 2 - BOARD_CENTER_HEIGHT / 2),
               BEAROFF_WIDTH * s, BOARD_CENTER_HEIGHT * s);

    /* right bearoff centre */
    strcpy(pchFile, "b-midrb.png");
    WriteImage(auchBoard + coord(BOARD_WIDTH - BEAROFF_WIDTH, BOARD_HEIGHT / 2 - BOARD_CENTER_HEIGHT / 2),
               BEAROFF_WIDTH * s, BOARD_CENTER_HEIGHT * s);

    /* left board centre */
    strcpy(pchFile, "b-midl.png");
    WriteImage(auchBoard + coord(BEAROFF_WIDTH, BOARD_HEIGHT / 2 - BOARD_CENTER_HEIGHT / 2),
               BOARD_CENTER_WIDTH * s, BOARD_CENTER_HEIGHT * s);

    /* bar centre */
    strcpy(pchFile, "b-midc.png");
    WriteImage(auchBoard + coord(BOARD_WIDTH / 2 - BAR_WIDTH / 2, BOARD_HEIGHT / 2 - BOARD_CENTER_HEIGHT / 2),
               BAR_WIDTH * s, BOARD_CENTER_HEIGHT * s);

    /* right board centre */
    strcpy(pchFile, "b-midr.png");
    WriteImage(auchBoard + coord(BOARD_WIDTH / 2 + BAR_WIDTH / 2, BOARD_HEIGHT / 2 - BOARD_CENTER_HEIGHT / 2),
               BOARD_CENTER_WIDTH * s, BOARD_CENTER_HEIGHT * s);

    /* bar bottom */
    Write("b-cb.png", auchBoard + coord(BOARD_WIDTH / 2 - BAR_WIDTH / 2, BOARD_HEIGHT - CUBE_HEIGHT - BORDER_HEIGHT),
          BAR_WIDTH * s, CUBE_HEIGHT * s);

    /* bottom border-high numbers */
    WriteBorder("b-hibot.png", auchHi, auchBoard + coord(0, BOARD_HEIGHT - BORDER_HEIGHT));
    WriteBorder("b-hibotr.png", auchHiRev, auchBoard + coord(0, BOARD_HEIGHT - BORDER_HEIGHT));

    /* bottom border-low numbers */
    WriteBorder("b-lobot.png", auchLo, auchBoard + coord(0, BOARD_HEIGHT - BORDER_HEIGHT));
    WriteBorder("b-lobotr.png", auchLoRev, auchBoard + coord(0, BOARD_HEIGHT - BORDER_HEIGHT));

    /* Copy 4 points (top/bottom and both colours) */
    CopyArea(auchPoint[0][0][0], POINT_WIDTH * s * 3,
             auchBoard + coord(BEAROFF_WIDTH, BORDER_HEIGHT), boardStride, POINT_WIDTH * s, POINT_HEIGHT * s);
    CopyArea(auchPoint[0][0][1], POINT_WIDTH * s * 3,
             auchBoard + coord(BEAROFF_WIDTH, BORDER_HEIGHT), boardStride, POINT_WIDTH * s, POINT_HEIGHT * s);
    CopyArea(auchPoint[0][1][0], POINT_WIDTH * s * 3,
             auchBoard + coord(BEAROFF_WIDTH + POINT_WIDTH, BOARD_HEIGHT / 2 + BOARD_CENTER_HEIGHT / 2), boardStride,
             POINT_WIDTH * s, POINT_HEIGHT * s);
    CopyArea(auchPoint[0][1][1], POINT_WIDTH * s * 3,
             auchBoard + coord(BEAROFF_WIDTH + POINT_WIDTH, BOARD_HEIGHT / 2 + BOARD_CENTER_HEIGHT / 2), boardStride,
             POINT_WIDTH * s, POINT_HEIGHT * s);
    CopyArea(auchPoint[1][0][0], POINT_WIDTH * s * 3,
             auchBoard + coord(BEAROFF_WIDTH + POINT_WIDTH, BORDER_HEIGHT), boardStride,
             POINT_WIDTH * s, POINT_HEIGHT * s);
    CopyArea(auchPoint[1][0][1], POINT_WIDTH * s * 3,
             auchBoard + coord(BEAROFF_WIDTH + POINT_WIDTH, BORDER_HEIGHT), boardStride,
             POINT_WIDTH * s, POINT_HEIGHT * s);
    CopyArea(auchPoint[1][1][0], POINT_WIDTH * s * 3,
             auchBoard + coord(BEAROFF_WIDTH, BOARD_HEIGHT / 2 + BOARD_CENTER_HEIGHT / 2), boardStride,
             POINT_WIDTH * s, POINT_HEIGHT * s);
    CopyArea(auchPoint[1][1][1], POINT_WIDTH * s * 3,
             auchBoard + coord(BEAROFF_WIDTH, BOARD_HEIGHT / 2 + BOARD_CENTER_HEIGHT / 2), boardStride,
             POINT_WIDTH * s, POINT_HEIGHT * s);

    /* Bear off trays */
    CopyArea(auchOff[0], BEAROFF_WIDTH * s * 3,
             auchBoard + coord(BOARD_WIDTH - BEAROFF_WIDTH, BORDER_HEIGHT), boardStride,
             BEAROFF_WIDTH * s, BEAROFF_HEIGHT * s);
    CopyArea(auchOff[1], BEAROFF_WIDTH * s * 3,
             auchBoard + coord(BOARD_WIDTH - BEAROFF_WIDTH, BOARD_HEIGHT / 2 + BOARD_CENTER_HEIGHT / 2), boardStride,
             BEAROFF_WIDTH * s, BEAROFF_HEIGHT * s);

    /* Bar areas */
    CopyArea(auchBar[0], BAR_WIDTH * s * 3,
             auchBoard + coord(BOARD_WIDTH / 2 - BAR_WIDTH / 2, BORDER_HEIGHT + CUBE_HEIGHT), boardStride,
             BAR_WIDTH * s, (POINT_HEIGHT - CUBE_HEIGHT) * s);
    CopyArea(auchBar[1], BAR_WIDTH * s * 3,
             auchBoard + coord(BOARD_WIDTH / 2 - BAR_WIDTH / 2, BOARD_HEIGHT / 2 + BOARD_CENTER_HEIGHT / 2),
             boardStride, BAR_WIDTH * s, (POINT_HEIGHT - CUBE_HEIGHT) * s);

    for (i = 1; i <= 5; i++) {
        for (j = 0; j < 2; j++) {       /* 1-5 chequers, both colours, down point */
            RefractBlend(auchPoint[0][0][j] + coordStride(0, CHEQUER_HEIGHT * (i - 1), POINT_WIDTH * s * 3),
                         POINT_WIDTH * s * 3, auchBoard + coord(BEAROFF_WIDTH,
                                                                BORDER_HEIGHT + CHEQUER_HEIGHT * (i - 1)), boardStride,
                         auchChequer[j], CHEQUER_WIDTH * s * 4, asRefract[j], CHEQUER_WIDTH * s, CHEQUER_WIDTH * s,
                         CHEQUER_HEIGHT * s);

            sprintf(pchFile, "b-gd-%c%d.png", j ? 'o' : 'x', i);
            WriteImageStride(auchPoint[0][0][j], POINT_WIDTH * s * 3, POINT_WIDTH * s, POINT_HEIGHT * s);
        }
        for (j = 0; j < 2; j++) {       /* 1-5 chequers, both colours, other down point */
            RefractBlend(auchPoint[1][0][j] + coordStride(0, CHEQUER_HEIGHT * (i - 1), POINT_WIDTH * s * 3),
                         POINT_WIDTH * s * 3, auchBoard + coord(BEAROFF_WIDTH + POINT_WIDTH,
                                                                BORDER_HEIGHT + CHEQUER_HEIGHT * (i - 1)), boardStride,
                         auchChequer[j], CHEQUER_WIDTH * s * 4, asRefract[j], CHEQUER_WIDTH * s, CHEQUER_WIDTH * s,
                         CHEQUER_HEIGHT * s);

            sprintf(pchFile, "b-rd-%c%d.png", j ? 'o' : 'x', i);
            WriteImageStride(auchPoint[1][0][j], POINT_WIDTH * s * 3, POINT_WIDTH * s, POINT_HEIGHT * s);
        }
        for (j = 0; j < 2; j++) {       /* 1-5 chequers, both colours, up point */
            RefractBlend(auchPoint[0][1][j] + coordStride(0, CHEQUER_HEIGHT * (5 - i), POINT_WIDTH * s * 3),
                         POINT_WIDTH * s * 3, auchBoard + coord(BEAROFF_WIDTH + POINT_WIDTH,
                                                                BOARD_HEIGHT - BORDER_HEIGHT - CHEQUER_HEIGHT * i),
                         boardStride, auchChequer[j], CHEQUER_WIDTH * s * 4, asRefract[j], CHEQUER_WIDTH * s,
                         CHEQUER_WIDTH * s, CHEQUER_HEIGHT * s);

            sprintf(pchFile, "b-gu-%c%d.png", j ? 'o' : 'x', i);
            WriteImageStride(auchPoint[0][1][j], POINT_WIDTH * s * 3, POINT_WIDTH * s, POINT_HEIGHT * s);
        }
        for (j = 0; j < 2; j++) {       /* 1-5 chequers, both colours, other up point */
            RefractBlend(auchPoint[1][1][j] + coordStride(0, CHEQUER_HEIGHT * (5 - i), POINT_WIDTH * s * 3),
                         POINT_WIDTH * s * 3, auchBoard + coord(BEAROFF_WIDTH,
                                                                BOARD_HEIGHT - BORDER_HEIGHT - CHEQUER_HEIGHT * i),
                         boardStride, auchChequer[j], CHEQUER_WIDTH * s * 4, asRefract[j], CHEQUER_WIDTH * s,
                         CHEQUER_WIDTH * s, CHEQUER_HEIGHT * s);

            sprintf(pchFile, "b-ru-%c%d.png", j ? 'o' : 'x', i);
            WriteImageStride(auchPoint[1][1][j], POINT_WIDTH * s * 3, POINT_WIDTH * s, POINT_HEIGHT * s);
        }

        /* 1-5 chequers in bear-off trays */
        RefractBlend(auchOff[0] + coordStride(BORDER_WIDTH, CHEQUER_HEIGHT * (i - 1), BEAROFF_WIDTH * s * 3),
                     BEAROFF_WIDTH * s * 3, auchBoard + coord(BOARD_WIDTH - BORDER_WIDTH - CHEQUER_WIDTH,
                                                              BORDER_HEIGHT + CHEQUER_HEIGHT * (i - 1)), boardStride,
                     auchChequer[0], CHEQUER_WIDTH * s * 4, asRefract[0], CHEQUER_WIDTH * s, CHEQUER_WIDTH * s,
                     CHEQUER_HEIGHT * s);

        sprintf(pchFile, "b-roff-x%d.png", i);
        WriteImageStride(auchOff[0], BEAROFF_WIDTH * s * 3, BEAROFF_WIDTH * s, BEAROFF_HEIGHT * s);

        RefractBlend(auchOff[1] + coordStride(BORDER_WIDTH, CHEQUER_HEIGHT * (5 - i), BEAROFF_WIDTH * s * 3),
                     BEAROFF_WIDTH * s * 3, auchBoard + coord(BOARD_WIDTH - BORDER_WIDTH - CHEQUER_WIDTH,
                                                              BOARD_HEIGHT - BORDER_HEIGHT - CHEQUER_HEIGHT * i),
                     boardStride, auchChequer[1], CHEQUER_WIDTH * s * 4, asRefract[1], CHEQUER_WIDTH * s,
                     CHEQUER_WIDTH * s, CHEQUER_HEIGHT * s);

        sprintf(pchFile, "b-roff-o%d.png", i);
        WriteImageStride(auchOff[1], BEAROFF_WIDTH * s * 3, BEAROFF_WIDTH * s, BEAROFF_HEIGHT * s);
    }

    for (i = 6; i <= 15; i++) {
        for (j = 0; j < 2; j++) {       /* 6-15 chequers, both colours, down point */
            CopyArea(auchPoint[0][0][j] + coordStride(1, CHEQUER_HEIGHT * 4 + 1, POINT_WIDTH * s * 3),
                     POINT_WIDTH * s * 3, auchChequerLabels + coordStride(0, (i - 4) * CHEQUER_LABEL_HEIGHT,
                                                                          CHEQUER_LABEL_WIDTH * s * 3),
                     CHEQUER_LABEL_WIDTH * s * 3, CHEQUER_LABEL_WIDTH * s, CHEQUER_LABEL_HEIGHT * s);
            sprintf(pchFile, "b-gd-%c%d.png", j ? 'o' : 'x', i);
            WriteImageStride(auchPoint[0][0][j], POINT_WIDTH * s * 3, POINT_WIDTH * s, POINT_HEIGHT * s);
        }
        for (j = 0; j < 2; j++) {       /* 6-15 chequers, both colours, other down point */
            CopyArea(auchPoint[1][0][j] + coordStride(1, CHEQUER_HEIGHT * 4 + 1, POINT_WIDTH * s * 3),
                     POINT_WIDTH * s * 3, auchChequerLabels + coordStride(0, (i - 4) * CHEQUER_LABEL_HEIGHT,
                                                                          CHEQUER_LABEL_WIDTH * s * 3),
                     CHEQUER_LABEL_WIDTH * s * 3, CHEQUER_LABEL_WIDTH * s, CHEQUER_LABEL_HEIGHT * s);
            sprintf(pchFile, "b-rd-%c%d.png", j ? 'o' : 'x', i);
            WriteImageStride(auchPoint[1][0][j], POINT_WIDTH * s * 3, POINT_WIDTH * s, POINT_HEIGHT * s);
        }
        for (j = 0; j < 2; j++) {       /* 6-15 chequers, both colours, up point */
            CopyArea(auchPoint[0][1][j] + coordStride(1, 1, POINT_WIDTH * s * 3), POINT_WIDTH * s * 3,
                     auchChequerLabels + coordStride(0, (i - 4) * CHEQUER_LABEL_HEIGHT, CHEQUER_LABEL_WIDTH * s * 3),
                     CHEQUER_LABEL_WIDTH * s * 3, CHEQUER_LABEL_WIDTH * s, CHEQUER_LABEL_HEIGHT * s);
            sprintf(pchFile, "b-gu-%c%d.png", j ? 'o' : 'x', i);
            WriteImageStride(auchPoint[0][1][j], POINT_WIDTH * s * 3, POINT_WIDTH * s, POINT_HEIGHT * s);
        }
        for (j = 0; j < 2; j++) {       /* 6-15 chequers, both colours, other up point */
            CopyArea(auchPoint[1][1][j] + coordStride(1, 1, POINT_WIDTH * s * 3), POINT_WIDTH * s * 3,
                     auchChequerLabels + coordStride(0, (i - 4) * CHEQUER_LABEL_HEIGHT, CHEQUER_LABEL_WIDTH * s * 3),
                     CHEQUER_LABEL_WIDTH * s * 3, CHEQUER_LABEL_WIDTH * s, CHEQUER_LABEL_HEIGHT * s);
            sprintf(pchFile, "b-ru-%c%d.png", j ? 'o' : 'x', i);
            WriteImageStride(auchPoint[1][1][j], POINT_WIDTH * s * 3, POINT_WIDTH * s, POINT_HEIGHT * s);
        }

        /* 6-15 chequers in bear-off trays */
        CopyArea(auchOff[0] + coordStride(CHEQUER_LABEL_WIDTH, CHEQUER_HEIGHT * 4 + 1, BEAROFF_WIDTH * s * 3),
                 BEAROFF_WIDTH * s * 3, auchChequerLabels + coordStride(0, (i - 4) * CHEQUER_LABEL_HEIGHT,
                                                                        CHEQUER_LABEL_WIDTH * s * 3),
                 CHEQUER_LABEL_WIDTH * s * 3, CHEQUER_LABEL_WIDTH * s, CHEQUER_LABEL_HEIGHT * s);
        sprintf(pchFile, "b-roff-x%d.png", i);
        WriteImageStride(auchOff[0], BEAROFF_WIDTH * s * 3, BEAROFF_WIDTH * s, BEAROFF_HEIGHT * s);

        CopyArea(auchOff[1] + coordStride(CHEQUER_LABEL_WIDTH, 1, BEAROFF_WIDTH * s * 3), BEAROFF_WIDTH * s * 3,
                 auchChequerLabels + coordStride(0, (i - 4) * CHEQUER_LABEL_HEIGHT, CHEQUER_LABEL_WIDTH * s * 3),
                 CHEQUER_LABEL_WIDTH * s * 3, CHEQUER_LABEL_WIDTH * s, CHEQUER_LABEL_HEIGHT * s);
        sprintf(pchFile, "b-roff-o%d.png", i);
        WriteImageStride(auchOff[1], BEAROFF_WIDTH * s * 3, BEAROFF_WIDTH * s, BEAROFF_HEIGHT * s);
    }

    /* 0-15 chequers on bar */
    WriteStride("b-bar-o0.png", auchBar[0], BAR_WIDTH * s * 3, BAR_WIDTH * s, (POINT_HEIGHT - CUBE_HEIGHT) * s);
    WriteStride("b-bar-x0.png", auchBar[1], BAR_WIDTH * s * 3, BAR_WIDTH * s, (POINT_HEIGHT - CUBE_HEIGHT) * s);

    for (i = 1; i <= 3; i++) {
        RefractBlend(auchBar[0] +
                     coordStride(BAR_WIDTH / 2 - CHEQUER_WIDTH / 2, (CHEQUER_HEIGHT + 1) * (3 - i) + 1,
                                 BAR_WIDTH * s * 3), BAR_WIDTH * s * 3,
                     auchBoard + coord(BOARD_WIDTH / 2 - CHEQUER_WIDTH / 2,
                                       (BORDER_HEIGHT + CUBE_HEIGHT) + (CHEQUER_HEIGHT + 1) * (3 - i)), boardStride,
                     auchChequer[0], CHEQUER_WIDTH * s * 4, asRefract[0], CHEQUER_WIDTH * s, CHEQUER_WIDTH * s,
                     CHEQUER_HEIGHT * s);
        sprintf(pchFile, "b-bar-o%d.png", i);
        WriteImageStride(auchBar[0], BAR_WIDTH * s * 3, BAR_WIDTH * s, (POINT_HEIGHT - CUBE_HEIGHT) * s);

        RefractBlend(auchBar[1] +
                     coordStride(BAR_WIDTH / 2 - CHEQUER_WIDTH / 2, (CHEQUER_HEIGHT + 1) * (i - 1) + 1,
                                 BAR_WIDTH * s * 3), BAR_WIDTH * s * 3,
                     auchBoard + coord(BOARD_WIDTH / 2 - CHEQUER_WIDTH / 2,
                                       (BOARD_HEIGHT / 2 + BOARD_CENTER_HEIGHT / 2) + (CHEQUER_HEIGHT + 1) * (i - 1)),
                     boardStride, auchChequer[1], CHEQUER_WIDTH * s * 4, asRefract[1], CHEQUER_WIDTH * s,
                     CHEQUER_WIDTH * s, CHEQUER_HEIGHT * s);
        sprintf(pchFile, "b-bar-x%d.png", i);
        WriteImageStride(auchBar[1], BAR_WIDTH * s * 3, BAR_WIDTH * s, (POINT_HEIGHT - CUBE_HEIGHT) * s);
    }

    for (i = 4; i <= 15; i++) {
        CopyArea(auchBar[0] + coordStride(CHEQUER_LABEL_WIDTH, 2, BAR_WIDTH * s * 3), BAR_WIDTH * s * 3,
                 auchChequerLabels + coordStride(0, (i - 4) * CHEQUER_LABEL_HEIGHT, CHEQUER_LABEL_WIDTH * s * 3),
                 CHEQUER_LABEL_WIDTH * s * 3, CHEQUER_LABEL_WIDTH * s, CHEQUER_LABEL_HEIGHT * s);
        sprintf(pchFile, "b-bar-o%d.png", i);
        WriteImageStride(auchBar[0], BAR_WIDTH * s * 3, BAR_WIDTH * s, (POINT_HEIGHT - CUBE_HEIGHT) * s);

        CopyArea(auchBar[1] +
                 coordStride(CHEQUER_LABEL_WIDTH, POINT_HEIGHT - CUBE_HEIGHT - CHEQUER_HEIGHT, BAR_WIDTH * s * 3),
                 BAR_WIDTH * s * 3, auchChequerLabels + coordStride(0, (i - 4) * CHEQUER_LABEL_HEIGHT,
                                                                    CHEQUER_LABEL_WIDTH * s * 3),
                 CHEQUER_LABEL_WIDTH * s * 3, CHEQUER_LABEL_WIDTH * s, CHEQUER_LABEL_HEIGHT * s);
        sprintf(pchFile, "b-bar-x%d.png", i);
        WriteImageStride(auchBar[1], BAR_WIDTH * s * 3, BAR_WIDTH * s, (POINT_HEIGHT - CUBE_HEIGHT) * s);
    }

    /* cube - top and bottom of bar */
    for (i = 0; i < 2; i++) {
        int offset;
        int cube_y = i ? ((BOARD_HEIGHT - BORDER_HEIGHT) * s - CUBE_HEIGHT * ss) : (BORDER_HEIGHT * s);

        AlphaBlendBase(auchBoard + (BOARD_WIDTH / 2) * s * 3 - (CUBE_WIDTH / 2) * ss * 3 +
                       cube_y * boardStride, boardStride,
                       auchBoard + (BOARD_WIDTH / 2) * s * 3 - (CUBE_WIDTH / 2) * ss * 3 +
                       cube_y * boardStride, boardStride,
                       auchCube, CUBE_WIDTH * ss * 4, CUBE_WIDTH * ss, CUBE_HEIGHT * ss);

        offset = i ? coord(BOARD_WIDTH / 2 - BAR_WIDTH / 2, BOARD_HEIGHT - BORDER_HEIGHT - CUBE_HEIGHT)
            : coord(BOARD_WIDTH / 2 - BAR_WIDTH / 2, BORDER_HEIGHT);

        for (j = 0; j < 12; j++) {
            CopyAreaRotateClip(auchBoard, boardStride,
                               (BOARD_WIDTH / 2) * s - (CUBE_WIDTH / 2) * ss + ss, cube_y + ss,
                               BOARD_WIDTH * s, BOARD_HEIGHT * s,
                               auchCubeFaces, CUBE_LABEL_WIDTH * ss * 3,
                               0, CUBE_LABEL_HEIGHT * ss * j, ss * CUBE_LABEL_WIDTH, ss * CUBE_LABEL_HEIGHT, 2 - 2 * i);

            sprintf(pchFile, "b-%s-%d.png", i ? "cb" : "ct", 2 << j);
            WriteImage(auchBoard + offset, BAR_WIDTH * s, CUBE_HEIGHT * s);

            if (j == 5) {       /* 64 cube is also the cube for 1 */
                sprintf(pchFile, "b-%sc-1.png", i ? "cb" : "ct");
                WriteImage(auchBoard + offset, BAR_WIDTH * s, CUBE_HEIGHT * s);
            }
        }
    }
    /* cube - doubles */
    for (i = 0; i < 2; i++) {
        int offset;

        CopyArea(auchMidBoard, BOARD_CENTER_WIDTH * s * 3,
                 auchBoard + coord(i ? BOARD_WIDTH / 2 + BAR_WIDTH / 2 : BEAROFF_WIDTH,
                                   BOARD_HEIGHT / 2 - BOARD_CENTER_HEIGHT / 2), boardStride, BOARD_CENTER_WIDTH * s,
                 BOARD_CENTER_HEIGHT * s);

        offset = ((BOARD_CENTER_HEIGHT / 2) * s - (CUBE_HEIGHT / 2) * ss) * BOARD_CENTER_WIDTH * s * 3 +
            3 * POINT_WIDTH * s * 3 - (CUBE_WIDTH / 2) * ss * 3;

        AlphaBlendBase(auchMidBoard + offset, BOARD_CENTER_WIDTH * s * 3,
                       auchMidBoard + offset, BOARD_CENTER_WIDTH * s * 3,
                       auchCube, CUBE_WIDTH * ss * 4, CUBE_WIDTH * ss, CUBE_HEIGHT * ss);

        for (j = 0; j < 12; j++) {
            CopyAreaRotateClip(auchMidBoard, BOARD_CENTER_WIDTH * s * 3,
                               POINT_WIDTH * s * 3 - ss * CUBE_LABEL_WIDTH / 2,
                               (BOARD_CENTER_HEIGHT / 2) * s - (CUBE_LABEL_HEIGHT / 2) * ss,
                               BOARD_CENTER_WIDTH * s, BOARD_CENTER_HEIGHT * s,
                               auchCubeFaces, CUBE_LABEL_WIDTH * ss * 3,
                               0, CUBE_LABEL_HEIGHT * ss * j, ss * CUBE_LABEL_WIDTH, ss * CUBE_LABEL_HEIGHT, i << 1);

            sprintf(pchFile, "b-mid%c-c%d.png", i ? 'r' : 'l', 2 << j);
            WriteImageStride(auchMidBoard, BOARD_CENTER_WIDTH * s * 3, BOARD_CENTER_WIDTH * s, BOARD_CENTER_HEIGHT * s);
        }
    }
    /* cube - centered */
    AlphaBlendBase(auchBoard +
                   ((BOARD_HEIGHT / 2) * s - (CUBE_HEIGHT / 2) * ss) * boardStride +
                   (BOARD_WIDTH / 2) * s * 3 - (CUBE_WIDTH / 2) * ss * 3,
                   boardStride,
                   auchBoard +
                   ((BOARD_HEIGHT / 2) * s - (CUBE_HEIGHT / 2) * ss) * boardStride +
                   (BOARD_WIDTH / 2) * s * 3 - (CUBE_WIDTH / 2) * ss * 3,
                   boardStride, auchCube, CUBE_WIDTH * ss * 4, CUBE_WIDTH * ss, CUBE_HEIGHT * ss);

    for (j = 0; j < 12; j++) {
        CopyAreaRotateClip(auchBoard, boardStride,
                           (BOARD_WIDTH / 2) * s - (CUBE_WIDTH / 2) * ss + ss,
                           (BOARD_HEIGHT / 2) * s - (CUBE_HEIGHT / 2) * ss + ss,
                           BOARD_WIDTH * s, BOARD_HEIGHT * s,
                           auchCubeFaces, CUBE_LABEL_WIDTH * ss * 3,
                           0, CUBE_LABEL_HEIGHT * ss * j, CUBE_LABEL_WIDTH * ss, CUBE_LABEL_HEIGHT * ss, 1);
        sprintf(pchFile, "b-midc-%d.png", 2 << j);
        WriteImage(auchBoard +
                   coord((BOARD_WIDTH / 2) - (BAR_WIDTH / 2), ((BOARD_HEIGHT / 2) - (BOARD_CENTER_HEIGHT / 2))),
                   BAR_WIDTH * s, BOARD_CENTER_HEIGHT * s);

        if (j == 5) {           /* 64 cube is also the cube for 1 */
            Write("b-midc-1.png",
                  auchBoard + coord((BOARD_WIDTH / 2) - (BAR_WIDTH / 2),
                                    ((BOARD_HEIGHT / 2) - (BOARD_CENTER_HEIGHT / 2))), BAR_WIDTH * s,
                  BOARD_CENTER_HEIGHT * s);
        }
    }

    /* dice rolls */
    for (i = 0; i < 2; i++) {
        int dice_x = (BEAROFF_WIDTH + 3 * POINT_WIDTH +
                      (6 * POINT_WIDTH + BAR_WIDTH) * i) * s * 3 - (DIE_WIDTH / 2) * ss * 3;

        int dice_y = ((BOARD_HEIGHT / 2) * s - (DIE_HEIGHT / 2) * ss) * boardStride;

        AlphaBlendBase(auchBoard + dice_x - DIE_WIDTH * ss * 3 + dice_y, boardStride,
                       auchBoard + dice_x - DIE_WIDTH * ss * 3 + dice_y, boardStride,
                       auchDice[i], DIE_WIDTH * ss * 4, DIE_WIDTH * ss, DIE_HEIGHT * ss);

        AlphaBlendBase(auchBoard + dice_x + DIE_WIDTH * ss * 3 + dice_y, boardStride,
                       auchBoard + dice_x + DIE_WIDTH * ss * 3 + dice_y, boardStride,
                       auchDice[i], DIE_WIDTH * ss * 4, DIE_WIDTH * ss, DIE_HEIGHT * ss);

        for (j = 0; j < 6; j++) {
            for (k = 0; k < 6; k++) {
                CopyArea(auchMidBoard, BOARD_CENTER_WIDTH * s * 3,
                         auchBoard + coord(i ? BOARD_WIDTH / 2 + BAR_WIDTH / 2 : BEAROFF_WIDTH,
                                           BORDER_HEIGHT + POINT_HEIGHT), boardStride, BOARD_CENTER_WIDTH * s,
                         BOARD_CENTER_HEIGHT * s);

                DrawPips(auchMidBoard + 3 * POINT_WIDTH * s * 3
                         - (DIE_WIDTH / 2) * ss * 3 - DIE_WIDTH * ss * 3
                         + ((BOARD_CENTER_HEIGHT / 2) * s - (DIE_HEIGHT / 2) * ss)
                         * BOARD_CENTER_WIDTH * s * 3, BOARD_CENTER_WIDTH * s * 3, auchPips[i], j + 1);
                DrawPips(auchMidBoard + 3 * POINT_WIDTH * s * 3
                         - (DIE_WIDTH / 2) * ss * 3 + DIE_WIDTH * ss * 3
                         + ((BOARD_CENTER_HEIGHT / 2) * s - (DIE_HEIGHT / 2) * ss)
                         * BOARD_CENTER_WIDTH * s * 3, BOARD_CENTER_WIDTH * s * 3, auchPips[i], k + 1);

                sprintf(pchFile, "b-mid%c-%c%d%d.png", i ? 'r' : 'l', i ? 'o' : 'x', j + 1, k + 1);
                WriteImageStride(auchMidBoard, BOARD_CENTER_WIDTH * s * 3,
                                 BOARD_CENTER_WIDTH * s, BOARD_CENTER_HEIGHT * s);
            }
        }
    }

    if (imagesWritten != NUM_IMAGES)
        g_print("Wrong number of images generated - %d written, expected %d\n", imagesWritten, NUM_IMAGES);
}

static void
RenderObjects(void)
{
    int clockwise;
    renderdata rd;
    CopyAppearance(&rd);

    rd.fLabels = TRUE;
    rd.nSize = s;
#if USE_BOARD3D
    /* Use 2d colours for dice */
    rd.fDisplayType = DT_2D;
#endif

    RenderBoard(&rd, auchBoard, boardStride);
    RenderChequers(&rd, auchChequer[0], auchChequer[1], asRefract[0], asRefract[1], CHEQUER_WIDTH * s * 4);
    RenderChequerLabels(&rd, auchChequerLabels, CHEQUER_LABEL_WIDTH * s * 3);

#if USE_GTK && HAVE_CAIRO
    RenderArrows(&rd, auchArrow[0], auchArrow[1], s * ARROW_WIDTH * 4, rd.fClockwise);
#endif

    /* Render numbers in both directions */
    clockwise = rd.fClockwise;
    rd.fClockwise = TRUE;
    RenderBoardLabels(&rd, auchLo, auchHi, BOARD_WIDTH * s * 4);
    rd.fClockwise = FALSE;
    RenderBoardLabels(&rd, auchLoRev, auchHiRev, BOARD_WIDTH * s * 4);
    rd.fClockwise = clockwise;

    /* cubes and dice are rendered a bit smaller */
    rd.nSize = ss;

    RenderCube(&rd, auchCube, CUBE_WIDTH * ss * 4);
    RenderCubeFaces(&rd, auchCubeFaces, CUBE_LABEL_WIDTH * ss * 3, auchCube, CUBE_WIDTH * ss * 4);

    RenderDice(&rd, auchDice[0], auchDice[1], DIE_WIDTH * ss * 4, TRUE);
    RenderPips(&rd, auchPips[0], auchPips[1], ss * 3);
}

static char *
GetFilenameBase(char *sz)
{
    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputf(_("You must specify a directory to export to (see `%s')\n"), "help export htmlimages");
        return 0;
    }

    if (!g_file_test(sz, G_FILE_TEST_IS_DIR) && g_mkdir(sz, 0777) < 0) {
        outputerr(sz);
        return 0;
    }

    szFile = malloc(strlen(sz) + 32);
    strcpy(szFile, sz);

    if (szFile[strlen(szFile) - 1] != '/')
        strcat(szFile, "/");

    pchFile = strchr(szFile, 0);

    return szFile;
}

static void
AllocObjects(void)
{
    int i, j, k;

    s = exsExport.nHtmlSize;

    auchMidlb = malloc((BOARD_WIDTH * s * 3 * BOARD_HEIGHT * s) * sizeof(unsigned char));
#if USE_GTK
    auchArrow[0] = malloc(s * ARROW_WIDTH * 4 * s * ARROW_HEIGHT);
    auchArrow[1] = malloc(s * ARROW_WIDTH * 4 * s * ARROW_HEIGHT);
#endif

    auchBoard = malloc((BOARD_WIDTH * s * 3 * BOARD_HEIGHT * s) * sizeof(unsigned char));
    for (i = 0; i < 2; i++)
        auchChequer[i] = malloc((CHEQUER_WIDTH * s * 4 * CHEQUER_HEIGHT * s) * sizeof(unsigned char));
    auchChequerLabels = malloc((12 * CHEQUER_LABEL_WIDTH * s * 3 * CHEQUER_LABEL_HEIGHT * s) * sizeof(unsigned char));
    auchLo = malloc((BOARD_WIDTH * s * 4 * BORDER_HEIGHT * s) * sizeof(unsigned char));
    auchHi = malloc((BOARD_WIDTH * s * 4 * BORDER_HEIGHT * s) * sizeof(unsigned char));
    auchLoRev = malloc((BOARD_WIDTH * s * 4 * BORDER_HEIGHT * s) * sizeof(unsigned char));
    auchHiRev = malloc((BOARD_WIDTH * s * 4 * BORDER_HEIGHT * s) * sizeof(unsigned char));
    auchCube = malloc((CUBE_WIDTH * ss * 4 * CUBE_HEIGHT * ss) * sizeof(unsigned char));
    auchCubeFaces = malloc((12 * CUBE_LABEL_WIDTH * ss * 3 * CUBE_LABEL_HEIGHT * ss) * sizeof(unsigned char));
    for (i = 0; i < 2; i++) {
        auchDice[i] = malloc((DIE_WIDTH * ss * 4 * DIE_HEIGHT * ss) * sizeof(unsigned char));
        auchPips[i] = malloc((ss * ss * 3) * sizeof(unsigned char));
    }
    auchLabel = malloc((BOARD_WIDTH * s * 3 * BOARD_HEIGHT * s) * sizeof(unsigned char));
    for (i = 0; i < 2; i++)
        auchOff[i] = malloc((BEAROFF_WIDTH * s * 3 * DISPLAY_BEAROFF_HEIGHT * s) * sizeof(unsigned char));
    auchMidBoard = malloc((BOARD_CENTER_WIDTH * s * 3 * BOARD_CENTER_HEIGHT * s) * sizeof(unsigned char));
    for (i = 0; i < 2; i++)
        auchBar[i] = malloc((BAR_WIDTH * s * 3 * (POINT_HEIGHT - CUBE_HEIGHT) * s) * sizeof(unsigned char));
    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++)
            for (k = 0; k < 2; k++)
                auchPoint[i][j][k] = malloc((POINT_WIDTH * s * 3 * DISPLAY_POINT_HEIGHT * s) * sizeof(unsigned char));
    for (i = 0; i < 2; i++)
        asRefract[i] = malloc((CHEQUER_WIDTH * s * CHEQUER_HEIGHT * s) * sizeof(unsigned short));
}

static void
TidyObjects(void)
{
    int i, j, k;
    free(auchMidlb);
#if USE_GTK
    free(auchArrow[0]);
    free(auchArrow[1]);
#endif
    free(szFile);
    free(auchBoard);
    for (i = 0; i < 2; i++)
        free(auchChequer[i]);
    free(auchChequerLabels);
    free(auchLo);
    free(auchHi);
    free(auchLoRev);
    free(auchHiRev);
    free(auchCube);
    free(auchCubeFaces);
    for (i = 0; i < 2; i++) {
        free(auchDice[i]);
        free(auchPips[i]);
    }
    free(auchLabel);
    for (i = 0; i < 2; i++)
        free(auchOff[i]);
    free(auchMidBoard);
    for (i = 0; i < 2; i++)
        free(auchBar[i]);
    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++)
            for (k = 0; k < 2; k++)
                free(auchPoint[i][j][k]);
    for (i = 0; i < 2; i++)
        free(asRefract[i]);
}

extern void
CommandExportHTMLImages(char *sz)
{
    szFile = GetFilenameBase(sz);
    if (!szFile)
        return;

    ProgressStartValue(_("Generating image:"), NUM_IMAGES);
    AllocObjects();
    RenderObjects();
    WriteImages();
    ProgressEnd();
    TidyObjects();
}

#else                           /* not HAVE_LIBPNG */
extern void
CommandExportHTMLImages(char *UNUSED(sz))
{
    outputl(_("This installation of GNU Backgammon was compiled without\n" "support for writing HTML images."));
}
#endif                          /* not HAVE_LIBPNG */
