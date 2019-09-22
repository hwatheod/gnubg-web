/*
 * render.c
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
 * $Id: render.c,v 1.102 2015/01/01 16:42:03 plm Exp $
 */

#include "config.h"
#include "common.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>
#if HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#endif
#include <isaac.h>
#include <math.h>
#include <stdlib.h>

#include "render.h"
#include "renderprefs.h"
#include "boarddim.h"
#include "boardpos.h"
#include "backgammon.h"
#include "util.h"

#if USE_GTK
#include <gtk/gtk.h>
#include <cairo.h>
#endif
#if USE_BOARD3D
#include "fun3d.h"
#endif

static randctx rc;
#define RAND irand( &rc )

#if HAVE_FREETYPE
#define FONT_VERA "fonts/Vera.ttf"
#define FONT_VERA_SERIF_BOLD "fonts/VeraSeBd.ttf"
#if 0 /* unused for now */
#define FONT_VERA_BOLD "fonts/VeraBd.ttf"
#endif
#endif

/* aaanPositions[Clockwise][x][point number][x, y. deltay] */
int positions[2][30][3] = { {
                             {BAR_X, BAR_Y_1, -CHEQUER_HEIGHT}, /* bar - player 1 */
                             {POINT_X(1), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(2), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(3), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(4), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(5), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(6), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(7), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(8), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(9), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(10), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(11), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(12), TOP_POINT_Y, CHEQUER_HEIGHT},
                             {POINT_X(13), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(14), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(15), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(16), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(17), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(18), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(19), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(20), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(21), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(22), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(23), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {POINT_X(24), BOT_POINT_Y, -CHEQUER_HEIGHT},
                             {BAR_X, BAR_Y_0, (CHEQUER_HEIGHT)},        /* bar player 0 */
                             {BEAROFF_RIGHT_X, TOP_POINT_Y, CHEQUER_WIDTH},     /* player 0 tray right */
                             {BEAROFF_RIGHT_X, BOT_POINT_Y, -CHEQUER_WIDTH},    /* player 0 tray right */
                             {BEAROFF_LEFT_X, TOP_POINT_Y, CHEQUER_WIDTH},      /* player 0 tray left */
                             {BEAROFF_LEFT_X, BOT_POINT_Y, -CHEQUER_WIDTH}      /* player 1 tray left */
                             }, {
                                 {BAR_X, BAR_Y_1, -CHEQUER_HEIGHT},     /* bar - player 0 */
                                 {POINT_X(12), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(11), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(10), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(9), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(8), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(7), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(6), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(5), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(4), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(3), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(2), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(1), TOP_POINT_Y, CHEQUER_HEIGHT},
                                 {POINT_X(24), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(23), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(22), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(21), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(20), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(19), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(18), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(17), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(16), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(15), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(14), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {POINT_X(13), BOT_POINT_Y, -CHEQUER_HEIGHT},
                                 {BAR_X, BAR_Y_0, (CHEQUER_HEIGHT)},    /* bar player 1 */
                                 {BEAROFF_LEFT_X, TOP_POINT_Y, CHEQUER_WIDTH},  /* player 0 tray left */
                                 {BEAROFF_LEFT_X, BOT_POINT_Y, -CHEQUER_WIDTH}, /* player 1 tray left */
                                 {BEAROFF_RIGHT_X, TOP_POINT_Y, CHEQUER_WIDTH}, /* player 0 tray right */
                                 {BEAROFF_RIGHT_X, BOT_POINT_Y, -CHEQUER_WIDTH},        /* player 0 tray right */
                                 }
};

#if HAVE_FREETYPE
static FT_Library ftl;
#endif

static renderdata rdDefault = {
    WOOD_ALDER,                 /* wt */
    {{0xF1 / 255.0f, 0x25 / 255.0f, 0x25 / 255.0f, 0.9f}, {0, 0, 0xB / 255.0f, 0.5f}},        /* aarColour */
    {{0xF1 / 255.0f, 0x25 / 255.0f, 0x25 / 255.0f, 0.9f}, {0, 0, 0xB / 255.0f, 0.5f}},        /* aarDiceColour */
    {TRUE, TRUE},               /* afDieColour */
    {{0xA4 / 255.0f, 0xA4 / 255.0f, 0xA4 / 255.0f, 1.0f}, {0xA4 / 255.0f, 0xA4 / 255.0f, 0xA4 / 255.0f, 1.0f}}, /* aarDiceDotColour */
    {0xD7 / 255.0f, 0xD7 / 255.0f, 0xD7 / 255.0f, 1.0f},    /* arCubeColour */
    {{0x2F, 0x5F, 0x2F, 0xFF}, {0x00, 0x3F, 0x00, 0xFF},
     {0xFF, 0x5F, 0x5F, 0xFF}, {0xBF, 0xBF, 0xBF, 0xFF}},       /* aanBoardC */
    {25, 25, 25, 25},           /* aSpeckle */
    {1.5, 1.5},                 /* arRefraction */
    {0.2f, 1.0f},               /* arCoefficient */
    {3.0, 30.0},                /* arExponent */
    {0.2f, 1.0f},               /* arDiceCoefficient */
    {3.0, 30.0},                /* arDiceExponent */
    {-0.55667f, 0.32139f, 0.76604f},    /* arLight */
    0.5,                        /* rRound */
    (unsigned int)-1,           /* nSize */
    TRUE,                       /* fHinges */
    TRUE,                       /* fLabels */
    FALSE,                      /* fClockwise */
    FALSE,                      /* Dice area */
    TRUE,                       /* Show game info */
    TRUE,                       /* dynamic labels */
    1                           /* Show move indicator */
#if USE_BOARD3D
        , DT_2D,                /* Display type */
    TRUE,                       /* fHinges3d */
    FALSE,                      /* Show shadows */
    50,                         /* Shadow darkness */
    0,                          /* Darnkess as percentage of ambient light */
    0,                          /* Animate roll */
    0,                          /* Animate flag */
    0,                          /* Close board on exit */
    0,                          /* Quick draw */
    36,                         /* Curve accuracy */
    LT_POSITIONAL,              /* light source type */
    {0, 2, 3.5f},               /* x,y,z pos of light source */
    {50, 70, 100},              /* amibient/diffuse/specular light levels */
    15,                         /* Board angle */
    20,                         /* FOV skew factor */
    0,                          /* Plan view */
    2.5f,                       /* Dice size */
    0,                          /* Rounded edges */
    1,                          /* Background in trays */
    0,                          /* Rounded points */
    PT_ROUNDED,                 /* Piece type */
    PTT_ALL,                    /* Piece texture type */
    {TRUE, TRUE},               /* afDieColour3d */
    0, 0,                       /* Used at runtime */
    /* Default 3d colours - black+white, should never be used -
     * if no 3d settings the first design will be used */
    {{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, 100, 0, NULL, NULL},
     {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, 100, 0, NULL, NULL}},
    {{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, 100, 0, NULL, NULL},
     {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, 100, 0, NULL, NULL}},
    {{{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, 100, 0, NULL, NULL},
     {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, 100, 0, NULL, NULL}},
    {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, 100, 0, NULL, NULL},
    {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, 100, 0, NULL, NULL},
    {{.5, .5, .5, .5}, {.5, .5, .5, .5}, {.5, .5, .5, .5}, 100, 0, NULL, NULL},
    {{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, 100, 0, NULL, NULL},
     {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, 100, 0, NULL, NULL}},
    {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, 100, 0, NULL, NULL},
    {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, 100, 0, NULL, NULL},
    {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, 100, 0, NULL, NULL},
    {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, 100, 0, NULL, NULL}
#endif
};

static inline unsigned char
clamp(float f)
{

    if (f < 0.0f)
        return 0;
    else if (f > 255.0f)
        return 0xFF;
    else
        return (unsigned char) f;
}

static int
intersects(int x0, int y0, int cx0, int cy0, int x1, int y1, int cx1, int cy1)
{

    return (y1 + cy1 > y0) && (y1 < y0 + cy0) && (x1 + cx1 > x0) && (x1 < x0 + cx0);
}

static inline float
ssqrt(float x)
{
    return x < 0.0f ? 0.0f : sqrtf(x);
}

extern void
CopyArea(unsigned char *puchDest, int nDestStride, unsigned char *puchSrc, int nSrcStride, int cx, int cy)
{
    int x;

    nDestStride -= cx * 3;
    nSrcStride -= cx * 3;

    for (; cy; cy--) {
        for (x = cx; x; x--) {
            *puchDest++ = *puchSrc++;
            *puchDest++ = *puchSrc++;
            *puchDest++ = *puchSrc++;
        }
        puchDest += nDestStride;
        puchSrc += nSrcStride;
    }
}

static void
CopyAreaClip(unsigned char *puchDest, int nDestStride,
             int xDest, int yDest, int cxDest, int cyDest,
             unsigned char *puchSrc, int nSrcStride, int xSrc, int ySrc, int cx, int cy)
{
    if (xSrc < 0) {
        cx += xSrc;
        xDest -= xSrc;
        xSrc = 0;
    }

    if (ySrc < 0) {
        cy += ySrc;
        yDest -= ySrc;
        ySrc = 0;
    }

    if (xDest < 0) {
        cx += xDest;
        xSrc -= xDest;
        xDest = 0;
    }

    if (yDest < 0) {
        cy += yDest;
        ySrc -= yDest;
        yDest = 0;
    }

    if (xDest + cx > cxDest)
        cx = cxDest - xDest;

    if (yDest + cy > cyDest)
        cy = cyDest - yDest;

    if (cx <= 0 || cy <= 0)
        return;

    CopyArea(puchDest + yDest * nDestStride + xDest * 3, nDestStride,
             puchSrc + ySrc * nSrcStride + xSrc * 3, nSrcStride, cx, cy);
}

extern void
CopyAreaRotateClip(unsigned char *puchDest, int nDestStride,
                   int xDest, int yDest, int cxDest, int cyDest,
                   unsigned char *puchSrc, int nSrcStride, int xSrc, int ySrc, int cx, int cy, int nTheta)
{

    int x, nSrcPixelStride = 0, nSrcRowStride = 0;

    if (!(nTheta %= 4)) {
        CopyAreaClip(puchDest, nDestStride, xDest, yDest, cxDest, cyDest, puchSrc, nSrcStride, xSrc, ySrc, cx, cy);
        return;
    }

    puchSrc += ySrc * nSrcStride + xSrc * 3;

    switch (nTheta % 4) {
    case 1:
        /* 90 deg anticlockwise */
        puchSrc += (cy - 1) * 3;        /* start at top right */
        nSrcPixelStride = nSrcStride;   /* move down... */
        nSrcRowStride = -3;     /* ...then left */
        break;

    case 2:
        /* 180 deg */
        puchSrc += (cx - 1) * nSrcStride + (cy - 1) * 3;        /* start at
                                                                 * bottom right */
        nSrcPixelStride = -3;   /* move left... */
        nSrcRowStride = -nSrcStride;    /* ...then up */
        break;

    case 3:
        /* 90 deg clockwise */
        puchSrc += (cx - 1) * nSrcStride;       /* start at bottom left */
        nSrcPixelStride = -nSrcStride;  /* move up... */
        nSrcRowStride = 3;      /* ...then right */
        break;
    }

    if (xDest < 0) {
        cx += xDest;
        puchSrc -= xDest * nSrcPixelStride;
        xDest = 0;
    }

    if (yDest < 0) {
        cy += yDest;
        puchSrc -= yDest * nSrcRowStride;
        yDest = 0;
    }

    if (xDest + cx > cxDest)
        cx = cxDest - xDest;

    if (yDest + cy > cyDest)
        cy = cyDest - yDest;

    if (cx <= 0 || cy <= 0)
        return;

    puchDest += yDest * nDestStride + xDest * 3;

    nDestStride -= cx * 3;
    nSrcRowStride -= cx * nSrcPixelStride;

    for (; cy; cy--) {
        for (x = cx; x; x--) {
            *puchDest++ = puchSrc[0];
            *puchDest++ = puchSrc[1];
            *puchDest++ = puchSrc[2];
            puchSrc += nSrcPixelStride;
        }
        puchDest += nDestStride;
        puchSrc += nSrcRowStride;
    }
}

static void
FillArea(unsigned char *puchDest, int nDestStride, int cx, int cy, unsigned char r, unsigned char g, unsigned char b)
{
    int x;

    nDestStride -= cx * 3;

    for (; cy; cy--) {
        for (x = cx; x; x--) {
            *puchDest++ = r;
            *puchDest++ = g;
            *puchDest++ = b;
        }
        puchDest += nDestStride;
    }
}

extern void
AlphaBlendBase(unsigned char *puchDest, int nDestStride,
               unsigned char *puchBack, int nBackStride, unsigned char *puchFore, int nForeStride, int cx, int cy)
{
    int x;

    nDestStride -= cx * 3;
    nBackStride -= cx * 3;
    nForeStride -= cx * 4;

    for (; cy; cy--) {
        for (x = cx; x; x--) {
            unsigned int a = puchFore[3];

            *puchDest++ = clamp((*puchBack++ * a) / 0xFF + *puchFore++);
            *puchDest++ = clamp((*puchBack++ * a) / 0xFF + *puchFore++);
            *puchDest++ = clamp((*puchBack++ * a) / 0xFF + *puchFore++);
            puchFore++;         /* skip the alpha channel */
        }
        puchDest += nDestStride;
        puchBack += nBackStride;
        puchFore += nForeStride;
    }
}

extern void
AlphaBlendClip(unsigned char *puchDest, int nDestStride,
               int xDest, int yDest, int cxDest, int cyDest,
               unsigned char *puchBack, int nBackStride,
               int xBack, int yBack, unsigned char *puchFore, int nForeStride, int xFore, int yFore, int cx, int cy)
{

    if (xFore < 0) {
        cx += xFore;
        xDest -= xFore;
        xBack -= xFore;
        xFore = 0;
    }

    if (yFore < 0) {
        cy += yFore;
        yDest -= yFore;
        yBack -= yFore;
        yFore = 0;
    }

    if (xDest < 0) {
        cx += xDest;
        xBack -= xDest;
        xFore -= xDest;
        xDest = 0;
    }

    if (yDest < 0) {
        cy += yDest;
        yBack -= yDest;
        yFore -= yDest;
        yDest = 0;
    }

    if (xDest + cx > cxDest)
        cx = cxDest - xDest;

    if (yDest + cy > cyDest)
        cy = cyDest - yDest;

    if (cx <= 0 || cy <= 0)
        return;

    AlphaBlendBase(puchDest + yDest * nDestStride + xDest * 3, nDestStride,
                   puchBack + yBack * nBackStride + xBack * 3, nBackStride,
                   puchFore + yFore * nForeStride + xFore * 4, nForeStride, cx, cy);
}

static void
AlphaBlend2(unsigned char *puchDest, int nDestStride,
            unsigned char *puchBack, int nBackStride, unsigned char *puchFore, int nForeStride, int cx, int cy)
{
    /* draw *puchFore on top of *puchBack using the alpha channel as mask into *puchDest */

    int x;

    nDestStride -= cx * 3;
    nBackStride -= cx * 3;
    nForeStride -= cx * 4;

    for (; cy; cy--) {
        for (x = cx; x; x--) {
            unsigned int a = puchFore[3];

            *puchDest++ = clamp((*puchBack++ * (0xFF - a)) / 0xFF + (*puchFore++ * a) / 0xFF);
            *puchDest++ = clamp((*puchBack++ * (0xFF - a)) / 0xFF + (*puchFore++ * a) / 0xFF);
            *puchDest++ = clamp((*puchBack++ * (0xFF - a)) / 0xFF + (*puchFore++ * a) / 0xFF);
            puchFore++;         /* skip the alpha channel */
        }
        puchDest += nDestStride;
        puchBack += nBackStride;
        puchFore += nForeStride;
    }
}

extern void
AlphaBlendClip2(unsigned char *puchDest, int nDestStride,
                int xDest, int yDest, int cxDest, int cyDest,
                unsigned char *puchBack, int nBackStride,
                int xBack, int yBack, unsigned char *puchFore, int nForeStride, int xFore, int yFore, int cx, int cy)
{
    /* draw *puchFore on top of *puchBack using the alpha channel as mask into *puchDest */

    if (xFore < 0) {
        cx += xFore;
        xDest -= xFore;
        xBack -= xFore;
        xFore = 0;
    }

    if (yFore < 0) {
        cy += yFore;
        yDest -= yFore;
        yBack -= yFore;
        yFore = 0;
    }

    if (xDest < 0) {
        cx += xDest;
        xBack -= xDest;
        xFore -= xDest;
        xDest = 0;
    }

    if (yDest < 0) {
        cy += yDest;
        yBack -= yDest;
        yFore -= yDest;
        yDest = 0;
    }

    if (xDest + cx > cxDest)
        cx = cxDest - xDest;

    if (yDest + cy > cyDest)
        cy = cyDest - yDest;

    if (cx <= 0 || cy <= 0)
        return;

    AlphaBlend2(puchDest + yDest * nDestStride + xDest * 3, nDestStride,
                puchBack + yBack * nBackStride + xBack * 3, nBackStride,
                puchFore + yFore * nForeStride + xFore * 4, nForeStride, cx, cy);
}

extern void
RefractBlend(unsigned char *puchDest, int nDestStride,
             unsigned char *puchBack, int nBackStride,
             unsigned char *puchFore, int nForeStride, unsigned short *psRefract, int nRefractStride, int cx, int cy)
{
    int x;

    nDestStride -= cx * 3;
    nForeStride -= cx * 4;
    nRefractStride -= cx;

    for (; cy; cy--) {
        for (x = cx; x; x--) {
            unsigned int a = puchFore[3];
            unsigned char *puch = puchBack + (*psRefract >> 8) * nBackStride + (*psRefract & 0xFF) * 3;

            *puchDest++ = clamp((puch[0] * a) / 0xFF + *puchFore++);
            *puchDest++ = clamp((puch[1] * a) / 0xFF + *puchFore++);
            *puchDest++ = clamp((puch[2] * a) / 0xFF + *puchFore++);
            puchFore++;         /* skip the alpha channel */
            psRefract++;
        }
        puchDest += nDestStride;
        puchFore += nForeStride;
        psRefract += nRefractStride;
    }
}

extern void
RefractBlendClip(unsigned char *puchDest, int nDestStride,
                 int xDest, int yDest, int cxDest, int cyDest,
                 unsigned char *puchBack, int nBackStride,
                 int xBack, int yBack,
                 unsigned char *puchFore, int nForeStride,
                 int xFore, int yFore, unsigned short *psRefract, int nRefractStride, int cx, int cy)
{
    if (xFore < 0) {
        cx += xFore;
        xDest -= xFore;
        xFore = 0;
    }

    if (yFore < 0) {
        cy += yFore;
        yDest -= yFore;
        yFore = 0;
    }

    if (xDest < 0) {
        cx += xDest;
        xFore -= xDest;
        xDest = 0;
    }

    if (yDest < 0) {
        cy += yDest;
        yFore -= yDest;
        yDest = 0;
    }

    if (xDest + cx > cxDest)
        cx = cxDest - xDest;

    if (yDest + cy > cyDest)
        cy = cyDest - yDest;

    if (cx <= 0 || cy <= 0)
        return;

    RefractBlend(puchDest + yDest * nDestStride + xDest * 3, nDestStride,
                 puchBack + yBack * nBackStride + xBack * 3, nBackStride,
                 puchFore + yFore * nForeStride + xFore * 4, nForeStride,
                 psRefract + yFore * nRefractStride + xFore, nRefractStride, cx, cy);
}

static void
RenderBorder(unsigned char *puch, int nStride, int x0, int y0,
             int x1, int y1, int nSize, unsigned char *colours, int fInvert)
{

#define COLOURS( ipix, edge, icol ) \
	( *( colours + ( (ipix) * 4 + (edge) ) * 3 + (icol) ) )

    int i, x, y, iCol;

    x0 *= nSize;
    x1 *= nSize;
    y0 *= nSize;
    y1 *= nSize;
    x1--;
    y1--;

    for (i = 0; i < nSize; i++) {
        for (x = x0; x < x1; x++)
            for (iCol = 0; iCol < 3; iCol++)
                puch[y0 * nStride + x * 3 + iCol] = fInvert ? COLOURS(nSize - i - 1, 1, iCol) : COLOURS(i, 3, iCol);

        for (x = x0; x < x1; x++)
            for (iCol = 0; iCol < 3; iCol++)
                puch[y1 * nStride + x * 3 + iCol] = fInvert ? COLOURS(nSize - i - 1, 3, iCol) : COLOURS(i, 1, iCol);

        for (y = y0; y < y1; y++)
            for (iCol = 0; iCol < 3; iCol++)
                puch[y * nStride + x0 * 3 + iCol] = fInvert ? COLOURS(nSize - i - 1, 0, iCol) : COLOURS(i, 2, iCol);

        for (y = y0; y < y1; y++)
            for (iCol = 0; iCol < 3; iCol++)
                puch[y * nStride + x1 * 3 + iCol] = fInvert ? COLOURS(nSize - i - 1, 2, iCol) : COLOURS(i, 0, iCol);

        x0++;
        x1--;
        y0++;
        y1--;
    }
}

static void
RenderFramePainted(renderdata * prd, unsigned char *puch, int nStride)
{
    int i;
    unsigned int ix;
    float cos_theta, diffuse, specular;
    unsigned char *colours = (unsigned char *) g_alloca(4 * 3 * prd->nSize * sizeof(unsigned char));

    diffuse = 0.8f * prd->arLight[2] + 0.2f;
    specular = powf(prd->arLight[2], 20) * 0.6f;

    /* fill whole board area with flat colour */
    FillArea(puch, nStride, prd->nSize * BOARD_WIDTH, prd->nSize * (BOARD_HEIGHT - 1),
             clamp(specular * 0x100 +
                   diffuse * prd->aanBoardColour[1][0]),
             clamp(specular * 0x100 +
                   diffuse * prd->aanBoardColour[1][1]), clamp(specular * 0x100 + diffuse * prd->aanBoardColour[1][2]));

    for (ix = 0; ix < prd->nSize; ix++) {
        float x = 1.0f - ((float) ix / prd->nSize);
        float z = ssqrt(1.0f - x * x);

        for (i = 0; i < 4; i++) {
            cos_theta = prd->arLight[2] * z + prd->arLight[i & 1] * x;
            if (cos_theta < 0)
                cos_theta = 0;
            diffuse = 0.8f * cos_theta + 0.2f;
            cos_theta = 2 * z * cos_theta - prd->arLight[2];
            specular = powf(cos_theta, 20) * 0.6f;

            COLOURS(ix, i, 0) = clamp(specular * 0x100 + diffuse * prd->aanBoardColour[1][0]);
            COLOURS(ix, i, 1) = clamp(specular * 0x100 + diffuse * prd->aanBoardColour[1][1]);
            COLOURS(ix, i, 2) = clamp(specular * 0x100 + diffuse * prd->aanBoardColour[1][2]);

            if (!(i & 1))
                x = -x;
        }
    }
#undef COLOURS

    RenderBorder(puch, nStride, 0, 0, BOARD_WIDTH / 2, BOARD_HEIGHT, prd->nSize, colours, FALSE);

    RenderBorder(puch, nStride, BOARD_WIDTH / 2, 0, BOARD_WIDTH, BOARD_HEIGHT, prd->nSize, colours, FALSE);

    RenderBorder(puch, nStride, 2, 2, BEAROFF_WIDTH - 2, BOARD_HEIGHT / 2 - 2, prd->nSize, colours, TRUE);

    RenderBorder(puch, nStride, 2, BOARD_HEIGHT / 2 + 2, BEAROFF_WIDTH - 2,
                 BOARD_HEIGHT - 2, prd->nSize, colours, TRUE);

    RenderBorder(puch, nStride, BOARD_WIDTH - BEAROFF_WIDTH + 2, 2,
                 BOARD_WIDTH - 2, BOARD_HEIGHT / 2 - 2, prd->nSize, colours, TRUE);


    RenderBorder(puch, nStride, BOARD_WIDTH - BEAROFF_WIDTH + 2,
                 BOARD_HEIGHT / 2 + 2, BOARD_WIDTH - 2, BOARD_HEIGHT - 2, prd->nSize, colours, TRUE);

    RenderBorder(puch, nStride, BEAROFF_WIDTH - 1, 2,
                 (BOARD_WIDTH - BAR_WIDTH) / 2 + 1, BOARD_HEIGHT - 2, prd->nSize, colours, TRUE);

    RenderBorder(puch, nStride, (BOARD_WIDTH + BAR_WIDTH) / 2 - 1, 2,
                 BOARD_WIDTH - BEAROFF_WIDTH + 1, BOARD_HEIGHT - 2, prd->nSize, colours, TRUE);

}

static float
WoodHash(float r)
{
    /* A quick and dirty hash function for floating point numbers; returns
     * a value in the range [0,1). */

    int n;
    float x;

    if (r == 0.0f)
        return 0;

    x = frexpf(r, &n);

    return fabsf(frexpf(x * 131073.1294427f + n, &n)) * 2 - 1;
}

static void
WoodPixel(float x, float y, float z, unsigned char auch[3], woodtype wt)
{
    float r;
    int grain, figure;

    r = sqrtf(x * x + y * y);
    r -= z / 60;

    switch (wt) {
    case WOOD_ALDER:
        r *= 3;
        grain = ((int) r % 60);

        if (grain < 10) {
            auch[0] = (unsigned char) (230 - grain * 2);
            auch[1] = (unsigned char) (100 - grain);
            auch[2] = (unsigned char) (20 - grain / 2);
        } else if (grain < 20) {
            auch[0] = (unsigned char) (210 + (grain - 10) * 2);
            auch[1] = (unsigned char) (90 + (grain - 10));
            auch[2] = (unsigned char) (15 + (grain - 10) / 2);
        } else {
            auch[0] = (unsigned char) (230 + (grain % 3));
            auch[1] = (unsigned char) (100 + (grain % 3));
            auch[2] = (unsigned char) (20 + (grain % 3));
        }

        figure = (int) (r / 29 + x / 15 + y / 17 + z / 29);

        if (figure % 3 == (grain / 3) % 3) {
            auch[0] -= WoodHash(figure + grain) * 8;
            auch[1] -= WoodHash(figure + grain) * 4;
            auch[2] -= WoodHash(figure + grain) * 2;
        }

        break;

    case WOOD_ASH:
        r *= 3;
        grain = ((int) r % 60);

        if (grain > 40)
            grain = 120 - 2 * grain;

        grain *= WoodHash((int) r / 60) * 0.7f + 0.3f;

        auch[0] = (unsigned char) (230 - grain);
        auch[1] = (unsigned char) (125 - grain / 2);
        auch[2] = (unsigned char) (20 - grain / 8);

        figure = (int) (r / 53 + x / 5 + y / 7 + z / 50);

        if (figure % 3 == (grain / 3) % 3) {
            auch[0] -= WoodHash(figure + grain) * 16;
            auch[1] -= WoodHash(figure + grain) * 8;
            auch[2] -= WoodHash(figure + grain) * 4;
        }

        break;

    case WOOD_BASSWOOD:
        r *= 5;
        grain = ((int) r % 60);

        if (grain > 50)
            grain = 60 - grain;
        else if (grain > 40)
            grain = 10;
        else if (grain > 30)
            grain -= 30;
        else
            grain = 0;

        auch[0] = (unsigned char) (230 - grain);
        auch[1] = (unsigned char) (205 - grain);
        auch[2] = (unsigned char) (150 - grain);

        break;

    case WOOD_BEECH:
        r *= 3;
        grain = ((int) r % 60);

        if (grain > 40)
            grain = 120 - 2 * grain;

        auch[0] = (unsigned char) (230 - grain);
        auch[1] = (unsigned char) (125 - grain / 2);
        auch[2] = (unsigned char) (20 - grain / 8);

        figure = (int) (r / 29 + x / 15 + y / 17 + z / 29);

        if (figure % 3 == (grain / 3) % 3) {
            auch[0] -= WoodHash(figure + grain) * 16;
            auch[1] -= WoodHash(figure + grain) * 8;
            auch[2] -= WoodHash(figure + grain) * 4;
        }

        break;

    case WOOD_CEDAR:
        r *= 3;
        grain = ((int) r % 60);

        if (grain < 10) {
            auch[0] = (unsigned char) (230 + grain);
            auch[1] = (unsigned char) (135 + grain);
            auch[2] = (unsigned char) (85 + grain / 2);
        } else if (grain < 20) {
            auch[0] = (unsigned char) (240 - (grain - 10) * 3);
            auch[1] = (unsigned char) (145 - (grain - 10) * 3);
            auch[2] = (unsigned char) (90 - (grain - 10) * 3 / 2);
        } else if (grain < 30) {
            auch[0] = (unsigned char) (200 + grain);
            auch[1] = (unsigned char) (105 + grain);
            auch[2] = (unsigned char) (70 + grain / 2);
        } else {
            auch[0] = (unsigned char) (230 + (grain % 3));
            auch[1] = (unsigned char) (135 + (grain % 3));
            auch[2] = (unsigned char) (85 + (grain % 3));
        }

        break;

    case WOOD_EBONY:
        r *= 3;
        grain = ((int) r % 60);

        if (grain > 40)
            grain = 120 - 2 * grain;

        auch[0] = (unsigned char) (30 + grain / 4);
        auch[1] = (unsigned char) (10 + grain / 8);
        auch[2] = 0;

        break;

    case WOOD_FIR:
        r *= 5;
        grain = ((int) r % 60);

        if (grain < 10) {
            auch[0] = (unsigned char) (230 - grain * 2 + grain % 3 * 3);
            auch[1] = (unsigned char) (100 - grain * 2 + grain % 3 * 3);
            auch[2] = (unsigned char) (20 - grain + grain % 3 * 3);
        } else if (grain < 30) {
            auch[0] = (unsigned char) (210 + grain % 3 * 3);
            auch[1] = (unsigned char) (80 + grain % 3 * 3);
            auch[2] = (unsigned char) (10 + grain % 3 * 3);
        } else if (grain < 40) {
            auch[0] = (unsigned char) (210 + (grain - 30) * 2 + grain % 3 * 3);
            auch[1] = (unsigned char) (80 + (grain - 30) * 2 + grain % 3 * 3);
            auch[2] = (unsigned char) (10 + (grain - 30) + grain % 3 * 3);
        } else {
            auch[0] = (unsigned char) (230 + grain % 3 * 5);
            auch[1] = (unsigned char) (100 + grain % 3 * 5);
            auch[2] = (unsigned char) (20 + grain % 3 * 5);
        }

        break;

    case WOOD_MAPLE:
        r *= 3;
        grain = ((int) r % 60);

        if (grain < 10) {
            auch[0] = (unsigned char) (230 - grain * 2 + grain % 3);
            auch[1] = (unsigned char) (180 - grain * 2 + grain % 3);
            auch[2] = (unsigned char) (50 - grain + grain % 3);
        } else if (grain < 20) {
            auch[0] = (unsigned char) (210 + grain % 3);
            auch[1] = (unsigned char) (160 + grain % 3);
            auch[2] = (unsigned char) (40 + grain % 3);
        } else if (grain < 30) {
            auch[0] = (unsigned char) (210 + (grain - 20) * 2 + grain % 3);
            auch[1] = (unsigned char) (160 + (grain - 20) * 2 + grain % 3);
            auch[2] = (unsigned char) (40 + (grain - 20) + grain % 3);
        } else {
            auch[0] = (unsigned char) (230 + grain % 3);
            auch[1] = (unsigned char) (180 + grain % 3);
            auch[2] = (unsigned char) (50 + grain % 3);
        }

        break;

    case WOOD_OAK:
        r *= 4;
        grain = ((int) r % 60);

        if (grain > 40)
            grain = 120 - 2 * grain;

        grain *= WoodHash((int) r / 60) * 0.7f + 0.3f;

        auch[0] = (unsigned char) (230 + grain / 2);
        auch[1] = (unsigned char) (125 + grain / 3);
        auch[2] = (unsigned char) (20 + grain / 8);

        figure = (int) (r / 53 + x / 5 + y / 7 + z / 30);

        if (figure % 3 == (grain / 3) % 3) {
            auch[0] -= WoodHash(figure + grain) * 32;
            auch[1] -= WoodHash(figure + grain) * 16;
            auch[2] -= WoodHash(figure + grain) * 8;
        }

        break;

    case WOOD_PINE:
        r *= 2;
        grain = ((int) r % 60);

        if (grain < 10) {
            auch[0] = (unsigned char) (230 + grain * 2 + grain % 3 * 3);
            auch[1] = (unsigned char) (160 + grain * 2 + grain % 3 * 3);
            auch[2] = (unsigned char) (50 + grain + grain % 3 * 3);
        } else if (grain < 20) {
            auch[0] = (unsigned char) (250 + grain % 3);
            auch[1] = (unsigned char) (180 + grain % 3);
            auch[2] = (unsigned char) (60 + grain % 3);
        } else if (grain < 30) {
            auch[0] = (unsigned char) (250 - (grain - 20) * 2 + grain % 3);
            auch[1] = (unsigned char) (180 - (grain - 20) * 2 + grain % 3);
            auch[2] = (unsigned char) (50 - (grain - 20) + grain % 3);
        } else {
            auch[0] = (unsigned char) (230 + grain % 3 * 3);
            auch[1] = (unsigned char) (160 + grain % 3 * 3);
            auch[2] = (unsigned char) (50 + grain % 3 * 3);
        }

        break;

    case WOOD_REDWOOD:
        r *= 5;
        grain = ((int) r % 60);

        if (grain > 40)
            grain = 120 - 2 * grain;

        auch[0] = (unsigned char) (220 - grain);
        auch[1] = (unsigned char) (70 - grain / 2);
        auch[2] = (unsigned char) (40 - grain / 4);

        break;

    case WOOD_WALNUT:
        r *= 3;
        grain = ((int) r % 60);

        if (grain > 40)
            grain = 120 - 2 * grain;

        grain *= WoodHash((int) r / 60) * 0.7f + 0.3f;

        auch[0] = (unsigned char) (80 + (grain * 3 / 2));
        auch[1] = (unsigned char) (40 + grain);
        auch[2] = (unsigned char) (grain / 2);

        break;

    case WOOD_WILLOW:
        r *= 3;
        grain = ((int) r % 60);

        if (grain > 40)
            grain = 120 - 2 * grain;

        auch[0] = (unsigned char) (230 + grain / 3);
        auch[1] = (unsigned char) (100 + grain / 5);
        auch[2] = (unsigned char) (20 + grain / 10);

        figure = (int) (r / 60 + z / 30);

        if (figure % 3 == (grain / 3) % 3) {
            auch[0] -= WoodHash(figure + grain) * 16;
            auch[1] -= WoodHash(figure + grain) * 8;
            auch[2] -= WoodHash(figure + grain) * 4;
        }

        break;

    default:
        g_assert_not_reached();
    }
#if USE_GTK
    if (showingGray)
        GrayScaleColC(auch);
#endif
}

static void
RenderFrameWood(renderdata * prd, unsigned char *puch, int nStride)
{

#define BUF( y, x, i ) ( puch[ (y) * nStride + (x) * 3 + (i) ] )

    int i, x, y, nSpecularTop, nSpecular, s = prd->nSize;
    unsigned char a[3];
    float cos_theta, rDiffuseTop, rHeight, rDiffuse;
    int *anSpecular[4], *anSpecularData = g_alloca(4 * s * sizeof(int));
    float *arDiffuse[4], *arDiffuseData = g_alloca(4 * s * sizeof(float));
    float *arHeight = g_alloca(s * sizeof(float));
    anSpecular[0] = anSpecularData;
    anSpecular[1] = anSpecularData + s;
    anSpecular[2] = anSpecularData + 2 * s;
    anSpecular[3] = anSpecularData + 3 * s;
    arDiffuse[0] = arDiffuseData;
    arDiffuse[1] = arDiffuseData + s;
    arDiffuse[2] = arDiffuseData + 2 * s;
    arDiffuse[3] = arDiffuseData + 3 * s;


    nSpecularTop = (int) (powf(prd->arLight[2], 20) * 0.6f * 0x100);
    rDiffuseTop = 0.8f * prd->arLight[2] + 0.2f;

    for (x = 0; x < s; x++) {
        float rx = 1.0f - ((float) x / s);
        float rz = ssqrt(1.0f - rx * rx);
        arHeight[x] = rz * s;

        for (i = 0; i < 4; i++) {
            cos_theta = prd->arLight[2] * rz + prd->arLight[i & 1] * rx;
            if (cos_theta < 0)
                cos_theta = 0;
            arDiffuse[i][x] = 0.8f * cos_theta + 0.2f;
            cos_theta = 2 * rz * cos_theta - prd->arLight[2];
            anSpecular[i][x] = (int) (powf(cos_theta, 20) * 0.6f * 0x100);

            if (!(i & 1))
                rx = -rx;
        }
    }

    /* Top and bottom edges */
    for (y = 0; y < s * BORDER_HEIGHT; y++)
        for (x = 0; x < s * BOARD_WIDTH; x++) {
            if (y < s) {
                rDiffuse = arDiffuse[3][y];
                nSpecular = anSpecular[3][y];
                rHeight = arHeight[y];
            } else if (y < 2 * s) {
                rDiffuse = rDiffuseTop;
                nSpecular = nSpecularTop;
                rHeight = s;
            } else {
                rDiffuse = arDiffuse[1][3 * s - y - 1];
                nSpecular = anSpecular[1][3 * s - y - 1];
                rHeight = arHeight[3 * s - y - 1];
            }

            WoodPixel(100 - y * 0.85f + x * 0.1f, rHeight - x * 0.11f, 200 + x * 0.93f + y * 0.16f, a, prd->wt);

            for (i = 0; i < 3; i++)
                BUF(y, x, i) = clamp(a[i] * rDiffuse + nSpecular);

            WoodPixel(123 + y * 0.87f - x * 0.08f, rHeight + x * 0.06f, -100 - x * 0.94f - y * 0.11f, a, prd->wt);

            for (i = 0; i < 3; i++)
                BUF(y + (BOARD_HEIGHT - BORDER_HEIGHT) * s, x, i) = clamp(a[i] * rDiffuse + nSpecular);
        }

    /* Left and right edges */
    for (y = 0; y < s * BOARD_HEIGHT; y++)
        for (x = 0; x < s * BORDER_WIDTH; x++) {
            if (x < s) {
                rDiffuse = arDiffuse[2][x];
                nSpecular = anSpecular[2][x];
                rHeight = arHeight[x];
            } else if (x < 2 * s) {
                rDiffuse = rDiffuseTop;
                nSpecular = nSpecularTop;
                rHeight = s;
            } else {
                rDiffuse = arDiffuse[0][3 * s - x - 1];
                nSpecular = anSpecular[0][3 * s - x - 1];
                rHeight = arHeight[3 * s - x - 1];
            }

            WoodPixel(300 + x * 0.9f + y * 0.1f, rHeight + y * 0.06f, 200 - y * 0.9f + x * 0.1f, a, prd->wt);

            if (x < y && x + y < s * BOARD_HEIGHT)
                for (i = 0; i < 3; i++)
                    BUF(y, x, i) = clamp(a[i] * rDiffuse + nSpecular);

            WoodPixel(-100 - x * 0.86f + y * 0.13f, rHeight - y * 0.07f, 300 + y * 0.92f + x * 0.08f, a, prd->wt);

            if (s * 3 - x <= y && s * 3 - x + y < s * BOARD_HEIGHT)
                for (i = 0; i < 3; i++)
                    BUF(y, x + (BOARD_WIDTH - BORDER_WIDTH) * s, i) = clamp(a[i] * rDiffuse + nSpecular);
        }

    /* Bar */
    for (y = 0; y < s * BOARD_HEIGHT; y++)
        for (x = 0; x < s * BAR_WIDTH / 2; x++) {
            if (y < s && y < x && y < s * (BAR_WIDTH / 2) - x - 1) {
                rDiffuse = arDiffuse[3][y];
                nSpecular = anSpecular[3][y];
                rHeight = arHeight[y];
            } else if (y > (BOARD_HEIGHT - 1) * s &&
                       s * BOARD_HEIGHT - y - 1 < x && s * BOARD_HEIGHT - y - 1 < s * (BAR_WIDTH / 2) - x - 1) {
                rDiffuse = arDiffuse[1][BOARD_HEIGHT * s - y - 1];
                nSpecular = anSpecular[1][BOARD_HEIGHT * s - y - 1];
                rHeight = arHeight[BOARD_HEIGHT * s - y - 1];
            } else if (x < s) {
                rDiffuse = arDiffuse[2][x];
                nSpecular = anSpecular[2][x];
                rHeight = arHeight[x];
            } else if (x < 5 * s) {
                rDiffuse = rDiffuseTop;
                nSpecular = nSpecularTop;
                rHeight = s;
            } else {
                rDiffuse = arDiffuse[0][6 * s - x - 1];
                nSpecular = anSpecular[0][6 * s - x - 1];
                rHeight = arHeight[6 * s - x - 1];
            }

            WoodPixel(100 - x * 0.88f + y * 0.08f, 50 + rHeight - y * 0.10f, -200 + y * 0.99f - x * 0.12f, a, prd->wt);

            if (y + x >= s * BORDER_HEIGHT && y - x <= s * (BOARD_HEIGHT - BORDER_HEIGHT))
                for (i = 0; i < 3; i++)
                    BUF(y, x + (BOARD_WIDTH - BAR_WIDTH) / 2 * s, i)
                        = clamp(a[i] * rDiffuse + nSpecular);

            WoodPixel(100 - x * 0.86f + y * 0.02f, 50 + rHeight - y * 0.07f, 200 - y * 0.92f + x * 0.03f, a, prd->wt);

            if (y + s * BAR_WIDTH / 2 - x >= s * BORDER_WIDTH &&
                y - s * BAR_WIDTH / 2 + x <= s * (BOARD_HEIGHT - BORDER_HEIGHT))
                for (i = 0; i < 3; i++)
                    BUF(y, x + BOARD_WIDTH / 2 * s, i) = clamp(a[i] * rDiffuse + nSpecular);
        }

    /* Left and right separators (between board and bearoff tray) */
    for (y = 0; y < s * (BOARD_HEIGHT - BORDER_HEIGHT - 1); y++)
        for (x = 0; x < s * BORDER_WIDTH; x++)
            if (x + y >= s && y - x <= s * (BOARD_HEIGHT - 2 * BORDER_HEIGHT + 1) &&
                y + s * BORDER_HEIGHT - x >= s && x + y <= s * (BOARD_HEIGHT - BORDER_HEIGHT + 1)) {
                if (x < s) {
                    rDiffuse = arDiffuse[2][x];
                    nSpecular = anSpecular[2][x];
                    rHeight = arHeight[x];
                } else if (x < 2 * s) {
                    rDiffuse = rDiffuseTop;
                    nSpecular = nSpecularTop;
                    rHeight = s;
                } else {
                    rDiffuse = arDiffuse[0][3 * s - x - 1];
                    nSpecular = anSpecular[0][3 * s - x - 1];
                    rHeight = arHeight[3 * s - x - 1];
                }

                WoodPixel(-300 - x * 0.91f + y * 0.10f, rHeight + y * 0.02f, -200 + y * 0.94f - x * 0.06f, a, prd->wt);

                for (i = 0; i < 3; i++)
                    BUF(y + 2 * s, x + (BEAROFF_WIDTH - BORDER_WIDTH) * s, i) = clamp(a[i] * rDiffuse + nSpecular);

                WoodPixel(100 - x * 0.89f - y * 0.07f, rHeight + y * 0.05f, 300 - y * 0.94f + x * 0.11f, a, prd->wt);

                for (i = 0; i < 3; i++)
                    BUF(y + 2 * s, x + (BOARD_WIDTH - BEAROFF_WIDTH) * s, i) = clamp(a[i] * rDiffuse + nSpecular);
            }

    /* Left and right dividers (between the bearoff trays) */
    for (y = 0; y < s * BEAROFF_DIVIDER_HEIGHT; y++)
        for (x = 0; x < s * (BEAROFF_WIDTH - BORDER_WIDTH - 1); x++)
            if (x + y >= s && y - x <= s * (BEAROFF_DIVIDER_HEIGHT - 1) &&
                y + s * (BEAROFF_WIDTH - BORDER_WIDTH - 1) - x >= s && x + y <= s * (BEAROFF_WIDTH + 1)) {
                if (y < s) {
                    rDiffuse = arDiffuse[3][y];
                    nSpecular = anSpecular[3][y];
                    rHeight = arHeight[y];
                } else if (y < 5 * s) {
                    rDiffuse = rDiffuseTop;
                    nSpecular = nSpecularTop;
                    rHeight = s;
                } else {
                    rDiffuse = arDiffuse[1][6 * s - y - 1];
                    nSpecular = anSpecular[1][6 * s - y - 1];
                    rHeight = arHeight[6 * s - y - 1];
                }

                WoodPixel(-100 - y * 0.85f + x * 0.11f, rHeight - x * 0.04f, -100 - x * 0.93f + y * 0.08f, a, prd->wt);

                for (i = 0; i < 3; i++)
                    BUF(y + (BOARD_HEIGHT - BEAROFF_DIVIDER_HEIGHT) / 2 * s,
                        x + (BORDER_WIDTH - 1) * s, i) = clamp(a[i] * rDiffuse + nSpecular);

                WoodPixel(-123 - y * 0.93f - x * 0.12f, rHeight + x * 0.11f, -150 + x * 0.88f - y * 0.07f, a, prd->wt);

                for (i = 0; i < 3; i++)
                    BUF(y + (BOARD_HEIGHT - BEAROFF_DIVIDER_HEIGHT) / 2 * s,
                        x + (BOARD_WIDTH - BEAROFF_WIDTH + BORDER_WIDTH - 1) * s, i) =
                        clamp(a[i] * rDiffuse + nSpecular);
            }
#undef BUF
}

static void
HingePixel(renderdata * prd, float xNorm, float yNorm, float xEye, float yEye, unsigned char auch[3])
{

    float arReflection[3], arAuxLight[2][3] = {
        {0.6f, 0.7f, 0.5f},
        {0.5f, -0.6f, 0.7f}
    };
    float zNorm, zEye;
    float diffuse, specular = 0, cos_theta;
    float l;
    int i;
    float *arLight[3];
    arLight[0] = prd->arLight;
    arLight[1] = arAuxLight[0];
    arLight[2] = arAuxLight[1];

    zNorm = ssqrt(1.0f - xNorm * xNorm - yNorm * yNorm);

    if ((cos_theta = xNorm * arLight[0][0] + yNorm * arLight[0][1] + zNorm * arLight[0][2]) < 0)
        diffuse = 0.2f;
    else {
        diffuse = cos_theta * 0.8f + 0.2f;

        for (i = 0; i < 3; i++) {
            if ((cos_theta = xNorm * arLight[i][0] + yNorm * arLight[i][1] + zNorm * arLight[i][2]) < 0)
                cos_theta = 0;

            arReflection[0] = arLight[i][0] - 2 * xNorm * cos_theta;
            arReflection[1] = arLight[i][1] - 2 * yNorm * cos_theta;
            arReflection[2] = arLight[i][2] - 2 * zNorm * cos_theta;

            l = sqrtf(arReflection[0] * arReflection[0] +
                      arReflection[1] * arReflection[1] + arReflection[2] * arReflection[2]);

            arReflection[0] /= l;
            arReflection[1] /= l;
            arReflection[2] /= l;

            zEye = ssqrt(1.0f - xEye * xEye - yEye * yEye);
            cos_theta = arReflection[0] * xEye + arReflection[1] * yEye + arReflection[2] * zEye;

            specular += powf(cos_theta, 30) * 0.7f;
        }
    }

    auch[0] = clamp(200 * diffuse + specular * 0x100);
    auch[1] = clamp(230 * diffuse + specular * 0x100);
    auch[2] = clamp(20 * diffuse + specular * 0x100);
}

static void
RenderHinges(renderdata * prd, unsigned char *puch, int nStride)
{

    int x, y, s = prd->nSize;
    float xNorm, yNorm;

    for (y = 0; y < HINGE_HEIGHT * s; y++)
        for (x = 0; x < 2 * s; x++) {
            if (s < 5 && y && !(y % (2 * s)))
                yNorm = 0.5;
            else if (y % (2 * s) < s / 5)
                yNorm = (s / 5 - y % (2 * s)) * (2.5f / s);
            else if (y % (2 * s) >= (2 * s - s / 5))
                yNorm = (y % (2 * s) - (2 * s - s / 5 - 1)) * (-2.5f / s);
            else
                yNorm = 0;

            xNorm = (x - s) / (float) s *(1.0f - yNorm * yNorm);

            HingePixel(prd, xNorm, yNorm,
                       (s - x) / (40 * s), (y - 20 * s) / (40 * s),
                       puch + (y + HINGE_BOT_Y * s) * nStride + (x + (BOARD_WIDTH / 2 - 1) * s) * 3);

            HingePixel(prd, xNorm, yNorm,
                       (s - x) / (40 * s), (y + 20 * s) / (40 * s),
                       puch + (y + HINGE_TOP_Y * s) * nStride + (x + (BOARD_WIDTH / 2 - 1) * s) * 3);
        }
}

static void
RenderBasicGlyph(unsigned char *puch, int nStride,
                 int nSize, unsigned n, int xOff, int yOff, unsigned char r, unsigned char g, unsigned
                 char b)
{

    int i, x0, x1, y0, y1, y2;

    x0 = xOff + nSize / 12 + 1;
    x1 = xOff + nSize * 5 / 12;
    y0 = yOff;
    y1 = yOff - nSize / 2;
    y2 = yOff - nSize;

#define PUT( x, y ) { puch[ (y) * nStride + (x) * 3 + 0 ] = r; \
		      puch[ (y) * nStride + (x) * 3 + 1 ] = g; \
    		      puch[ (y) * nStride + (x) * 3 + 2 ] = b; }

    if (n == 0 || n == 2 || n == 3 || n == 5 || n == 6 || n == 7 || n == 8 || n == 9)
        /* top */
        for (i = x0; i <= x1; i++)
            PUT(i, y2);

    if (n == 2 || n == 3 || n == 4 || n == 5 || n == 6 || n == 8 || n == 9)
        /* middle */
        for (i = x0; i <= x1; i++)
            PUT(i, y1);

    if (n == 0 || n == 2 || n == 3 || n == 5 || n == 6 || n == 8 || n == 9)
        /* bottom */
        for (i = x0; i <= x1; i++)
            PUT(i, y0);

    if (n == 0 || n == 4 || n == 5 || n == 6 || n == 8 || n == 9)
        /* top left */
        for (i = y2; i <= y1; i++)
            PUT(x0, i);

    if (n == 0 || n == 2 || n == 6 || n == 8)
        /* bottom left */
        for (i = y1; i <= y0; i++)
            PUT(x0, i);

    if (n == 0 || n == 1 || n == 2 || n == 3 || n == 4 || n == 7 || n == 8 || n == 9)
        /* top right */
        for (i = y2; i <= y1; i++)
            PUT(x1, i);

    if (n == 0 || n == 1 || n == 3 || n == 4 || n == 5 || n == 6 || n == 7 || n == 8 || n == 9)
        /* bottom right */
        for (i = y1; i <= y0; i++)
            PUT(x1, i);
#undef PUT
}


static void
RenderBasicNumber(unsigned char *puch, int nStride,
                  int nSize, unsigned n, int xOff, int yOff, unsigned char r, unsigned char g, unsigned
                  char b)
{
    int x, c;

    for (c = 0, x = n; x; x /= 10)
        c += nSize / 2;

    xOff += c / 2;

    for (; n; n /= 10) {
        xOff -= nSize / 2;
        RenderBasicGlyph(puch, nStride, nSize, n % 10, xOff, yOff, r, g, b);
    }
}

#if HAVE_FREETYPE
static void
RenderGlyph(unsigned char *puch, int nStride, FT_Glyph pftg,
            int xOff, int yOff, unsigned char r, unsigned char g, unsigned char b)
{

    FT_BitmapGlyph pftbg;
    FT_Bitmap *pb;
    unsigned int x, y, x0 = 0, y0 = 0;

    g_assert(pftg->format == FT_GLYPH_FORMAT_BITMAP);

    pftbg = (FT_BitmapGlyph) pftg;
    pb = &pftbg->bitmap;

    xOff += pftbg->left;
    yOff -= pftbg->top;

    if (xOff < 0) {
        x0 = -xOff;
        xOff = 0;
    }

    if (yOff < 0) {
        y0 = -yOff;
        yOff = 0;
    }

    g_assert(pb->pixel_mode == FT_PIXEL_MODE_GRAY);

    puch += yOff * nStride + xOff * 3;
    nStride -= 3 * pb->width;

    for (y = y0; y < pb->rows; y++) {
        for (x = x0; x < pb->width; x++) {
            *puch = (unsigned char) ((*puch * (pb->num_grays -
                                               pb->buffer[y * pb->pitch + x]) + r * pb->buffer[y * pb->pitch +
                                                                                               x]) / pb->num_grays);
            puch++;
            *puch = (unsigned char) ((*puch * (pb->num_grays -
                                               pb->buffer[y * pb->pitch + x]) + g * pb->buffer[y * pb->pitch +
                                                                                               x]) / pb->num_grays);
            puch++;
            *puch = (unsigned char) ((*puch * (pb->num_grays -
                                               pb->buffer[y * pb->pitch + x]) + b * pb->buffer[y * pb->pitch +
                                                                                               x]) / pb->num_grays);
            puch++;
        }
        puch += nStride;
    }
}

static void
RenderNumber(unsigned char *puch, int nStride, FT_Glyph * aftg,
             unsigned n, int xOff, int yOff, unsigned char r, unsigned char g, unsigned char b)
{
    int x, c;

    for (c = 0, x = n; x; x /= 10)
        c += (aftg[x % 10]->advance.x + 0x8000) >> 16;

    xOff += c / 2;

    for (; n; n /= 10) {
        xOff -= (aftg[n % 10]->advance.x + 0x8000) >> 16;
        RenderGlyph(puch, nStride, aftg[n % 10], xOff, yOff, r, g, b);
    }
}


#endif

static void
RenderBasicLabels(renderdata * prd, unsigned char *puch, int nStride,
                  const int iStart, const int iEnd, const int iDelta)
{

    int i;

    for (i = 0; i < (1 + abs(iStart - iEnd)); ++i)
        RenderBasicNumber(puch, nStride, prd->nSize, iStart + i * iDelta,
                          (positions[prd->fClockwise][i + 1][0] + 3) * prd->nSize, 2 * prd->nSize, 0xFF, 0xFF, 0xFF);


}


static void
RenderLabels(renderdata * prd, unsigned char *puch, int nStride, const int iStart, const int iEnd, const int iDelta)
{
#if HAVE_FREETYPE
    FT_Face ftf;
    int i;
    FT_Glyph aftg[10];
    char *file;

    file = BuildFilename(FONT_VERA);
    if (FT_New_Face(ftl, file, 0, &ftf)) {
        RenderBasicLabels(prd, puch, nStride, iStart, iEnd, iDelta);
        g_free(file);
        return;
    }
    g_free(file);

    if (FT_Set_Pixel_Sizes(ftf, 0, prd->nSize * 5 / 2)) {
        RenderBasicLabels(prd, puch, nStride, iStart, iEnd, iDelta);
        return;
    }

    if (prd->fLabels) {
        for (i = 0; i < 10; i++) {
            FT_Load_Char(ftf, '0' + i, FT_LOAD_RENDER);
            FT_Get_Glyph(ftf->glyph, aftg + i);
        }

        FT_Done_Face(ftf);

        for (i = 0; i < (1 + abs(iStart - iEnd)); ++i)
            RenderNumber(puch, nStride, aftg, iStart + i * iDelta,
                         (positions[prd->fClockwise][i + 1][0] + 3) * prd->nSize, 7 * prd->nSize / 3, 0xFF, 0xFF, 0xFF);

        for (i = 0; i < 10; i++)
            FT_Done_Glyph(aftg[i]);
    }
#else
    RenderBasicLabels(prd, puch, nStride, iStart, iEnd, iDelta);
#endif

}

static unsigned char
BoardPixel(renderdata * prd, int i, int antialias, int j)
{
    int rand1 = RAND;
    int rand2 = RAND;
    return clamp((((int) prd->aanBoardColour[0][j] -
                   (int) prd->aSpeckle[0] / 2 +
                   (int) rand1 % (prd->aSpeckle[0] + 1)) *
                  (20 - antialias) +
                  ((int) prd->aanBoardColour[i][j] -
                   (int) prd->aSpeckle[i] / 2 +
                   (int) rand2 % (prd->aSpeckle[i] + 1)) * antialias) * (prd->arLight[2] * 0.8f + 0.2f) / 20);
}

extern void
RenderBoard(renderdata * prd, unsigned char *puch, int nStride)
{

    unsigned int ix, iy;
    int antialias;

#define BUF( y, x, i ) ( puch[ (y) * nStride + (x) * 3 + (i) ] )

    if (prd->wt == WOOD_PAINT)
        RenderFramePainted(prd, puch, nStride);
    else
        RenderFrameWood(prd, puch, nStride);

    if (prd->fHinges)
        RenderHinges(prd, puch, nStride);

    /*
     * if( prd->fLabels )
     * RenderLabels( prd, puch, nStride ); */

    /* fill bottom left 2 points and top left 2 points, does not do area between
     * points.   */
    for (iy = 0; iy < DISPLAY_POINT_HEIGHT * prd->nSize; iy++)
        for (ix = 0; ix < CHEQUER_WIDTH * prd->nSize; ix++) {
            /* <= 0 is board; >= 20 is on a point; interpolate in between */
            antialias = 2 * (DISPLAY_POINT_HEIGHT * prd->nSize - iy)
                + 1 - (BEAROFF_WIDTH + 2 * CHEQUER_WIDTH - 1) * abs(BORDER_WIDTH * (int)prd->nSize - (int)ix);

            if (antialias < 0)
                antialias = 0;
            else if (antialias > 20)
                antialias = 20;

            BUF(iy + BORDER_HEIGHT * prd->nSize,
                ix + (CHEQUER_WIDTH + BEAROFF_WIDTH) * prd->nSize, 0) =
                BUF((BOARD_HEIGHT - BORDER_HEIGHT) * prd->nSize - iy - 1,
                    ix + BEAROFF_WIDTH * prd->nSize, 0) = BoardPixel(prd, 2, antialias, 0);

            BUF(iy + BORDER_HEIGHT * prd->nSize,
                ix + (CHEQUER_WIDTH + BEAROFF_WIDTH) * prd->nSize, 1) =
                BUF((BOARD_HEIGHT - BORDER_HEIGHT) * prd->nSize - iy - 1,
                    ix + BEAROFF_WIDTH * prd->nSize, 1) = BoardPixel(prd, 2, antialias, 1);

            BUF(iy + BORDER_HEIGHT * prd->nSize,
                ix + (CHEQUER_WIDTH + BEAROFF_WIDTH) * prd->nSize, 2) =
                BUF((BOARD_HEIGHT - BORDER_HEIGHT) * prd->nSize - iy - 1,
                    ix + BEAROFF_WIDTH * prd->nSize, 2) = BoardPixel(prd, 2, antialias, 2);

            BUF(iy + BORDER_HEIGHT * prd->nSize, ix + BEAROFF_WIDTH * prd->nSize, 0) =
                BUF((BOARD_HEIGHT - BORDER_HEIGHT) * prd->nSize - iy - 1,
                    ix + (CHEQUER_WIDTH + BEAROFF_WIDTH) * prd->nSize, 0)
                = BoardPixel(prd, 3, antialias, 0);

            BUF(iy + BORDER_HEIGHT * prd->nSize, ix + BEAROFF_WIDTH * prd->nSize, 1) =
                BUF((BOARD_HEIGHT - BORDER_HEIGHT) * prd->nSize - iy - 1,
                    ix + (CHEQUER_WIDTH + BEAROFF_WIDTH) * prd->nSize, 1) = BoardPixel(prd, 3, antialias, 1);

            BUF(iy + BORDER_HEIGHT * prd->nSize, ix + BEAROFF_WIDTH * prd->nSize, 2) =
                BUF((BOARD_HEIGHT - BORDER_HEIGHT) * prd->nSize - iy - 1,
                    ix + (CHEQUER_WIDTH + BEAROFF_WIDTH) * prd->nSize, 2) = BoardPixel(prd, 3, antialias, 2);
        }
    /* fill in the space between the points */
    for (iy = 0; iy < (BOARD_HEIGHT / 2 - DISPLAY_POINT_HEIGHT + 1) * prd->nSize; iy++)
        for (ix = 0; ix < 2 * CHEQUER_WIDTH * prd->nSize; ix++) {
            BUF((DISPLAY_POINT_HEIGHT + BORDER_HEIGHT) * prd->nSize + iy,
                BEAROFF_WIDTH * prd->nSize + ix, 0) = BoardPixel(prd, 0, 0, 0);
            BUF((DISPLAY_POINT_HEIGHT + BORDER_HEIGHT) * prd->nSize + iy,
                BEAROFF_WIDTH * prd->nSize + ix, 1) = BoardPixel(prd, 0, 0, 1);
            BUF((DISPLAY_POINT_HEIGHT + BORDER_HEIGHT) * prd->nSize + iy,
                BEAROFF_WIDTH * prd->nSize + ix, 2) = BoardPixel(prd, 0, 0, 2);
        }


    CopyArea(&BUF(BORDER_HEIGHT * prd->nSize,
                  (BEAROFF_WIDTH + 2 * CHEQUER_WIDTH) * prd->nSize, 0), nStride,
             &BUF(BORDER_HEIGHT * prd->nSize, BEAROFF_WIDTH * prd->nSize, 0), nStride,
             (2 * CHEQUER_WIDTH) * prd->nSize, (BOARD_HEIGHT - 2 * BORDER_HEIGHT) * prd->nSize);
    CopyArea(&BUF(BORDER_HEIGHT * prd->nSize,
                  (BEAROFF_WIDTH + 4 * CHEQUER_WIDTH) * prd->nSize, 0), nStride,
             &BUF(BORDER_HEIGHT * prd->nSize, BEAROFF_WIDTH * prd->nSize, 0), nStride,
             (2 * CHEQUER_WIDTH) * prd->nSize, (BOARD_HEIGHT - 2 * BORDER_HEIGHT) * prd->nSize);
    CopyArea(&BUF(BORDER_HEIGHT * prd->nSize,
                  (BEAROFF_WIDTH + 8 * CHEQUER_WIDTH) * prd->nSize, 0), nStride,
             &BUF(BORDER_HEIGHT * prd->nSize, BEAROFF_WIDTH * prd->nSize, 0), nStride,
             (6 * CHEQUER_WIDTH) * prd->nSize, (BOARD_HEIGHT - 2 * BORDER_HEIGHT) * prd->nSize);

    /* This is the tray filling */
    for (iy = 0; iy < DISPLAY_BEAROFF_HEIGHT * prd->nSize; iy++)
        for (ix = 0; ix < BEAROFF_INSIDE * prd->nSize; ix++) {
            BUF(iy + BORDER_HEIGHT * prd->nSize, ix + BORDER_WIDTH * prd->nSize, 0) = BoardPixel(prd, 0, 0, 0);
            BUF(iy + BORDER_HEIGHT * prd->nSize, ix + BORDER_WIDTH * prd->nSize, 1) = BoardPixel(prd, 0, 0, 1);
            BUF(iy + BORDER_HEIGHT * prd->nSize, ix + BORDER_WIDTH * prd->nSize, 2) = BoardPixel(prd, 0, 0, 2);
        }

    /* And then the tray filling is copied to the other 3 trays */
    CopyArea(&BUF((BOARD_HEIGHT / 2 + BORDER_HEIGHT) * prd->nSize,
                  BORDER_WIDTH * prd->nSize, 0), nStride, &BUF(BORDER_HEIGHT * prd->nSize, BORDER_WIDTH * prd->nSize, 0)
             , nStride, BEAROFF_INSIDE * prd->nSize, DISPLAY_BEAROFF_HEIGHT * prd->nSize);
    CopyArea(&BUF(BORDER_HEIGHT * prd->nSize,
                  (BOARD_WIDTH - BEAROFF_WIDTH + BORDER_WIDTH) * prd->nSize, 0),
             nStride,
             &BUF(BORDER_HEIGHT * prd->nSize, BORDER_WIDTH * prd->nSize, 0),
             nStride, BEAROFF_INSIDE * prd->nSize, DISPLAY_BEAROFF_HEIGHT * prd->nSize);
    CopyArea(&BUF((BOARD_HEIGHT / 2 + BORDER_HEIGHT) * prd->nSize,
                  (BOARD_WIDTH - BEAROFF_WIDTH + BORDER_WIDTH) * prd->nSize, 0),
             nStride,
             &BUF(BORDER_HEIGHT * prd->nSize, BORDER_WIDTH * prd->nSize, 0),
             nStride, BEAROFF_INSIDE * prd->nSize, DISPLAY_BEAROFF_HEIGHT * prd->nSize);

#undef BUF
}

extern void
RenderChequers(renderdata * prd, unsigned char *puch0,
               unsigned char *puch1, unsigned short *psRefract0, unsigned short *psRefract1, int UNUSED(nStride))
{

    int size = CHEQUER_WIDTH * prd->nSize;
    int ix, iy, in, fx, fy, i;
    float x, y, z, x_loop, y_loop, diffuse, specular_x, specular_o, cos_theta, r, x1, y1, len;

#define BUFX( y, x, i ) puch0[ ( (y) * size + (x) ) * 4 + (i) ]
#define BUFO( y, x, i ) puch1[ ( (y) * size + (x) ) * 4 + (i) ]

    for (iy = 0, y_loop = -1.0; iy < size; iy++) {
        for (ix = 0, x_loop = -1.0; ix < size; ix++) {
            in = 0;
            diffuse = specular_x = specular_o = 0.0;
            fy = 0;
            y = y_loop;
            do {
                fx = 0;
                x = x_loop;
                do {
                    r = sqrtf(x * x + y * y);
                    if (r < prd->rRound)
                        x1 = y1 = 0.0;
                    else {
                        x1 = x * (r / (1 - prd->rRound) - 1 / (1 - prd->rRound) + 1);
                        y1 = y * (r / (1 - prd->rRound) - 1 / (1 - prd->rRound) + 1);
                    }
                    if ((z = 1.0f - x1 * x1 - y1 * y1) > 0.0f) {
                        in++;
                        diffuse += 0.3f;
                        z = sqrtf(z) * 1.5f;
                        len = sqrtf(x1 * x1 + y1 * y1 + z * z);
                        if ((cos_theta = (prd->arLight[0] * x1 + prd->arLight[1] * -y1 + prd->arLight[2] * z) / len)
                            > 0) {
                            diffuse += cos_theta;
                            if ((cos_theta = 2 * z * cos_theta / len - prd->arLight[2]) > 0) {
                                specular_x += powf(cos_theta, prd->arExponent[0]) * prd->arCoefficient[0];
                                specular_o += powf(cos_theta, prd->arExponent[1]) * prd->arCoefficient[1];
                            }
                        }
                    }
                    x += 1.0f / (size);
                } while (!fx++);
                y += 1.0f / (size);
            } while (!fy++);

            if (!in) {
                /* pixel is outside chequer */
                for (i = 0; i < 3; i++)
                    BUFX(iy, ix, i) = BUFO(iy, ix, i) = 0;

                *psRefract0++ = *psRefract1++ = (unsigned short) ((iy << 8) | ix);

                BUFX(iy, ix, 3) = BUFO(iy, ix, 3) = 0xFF;
            } else {
                /* pixel is inside chequer */
                float r, s, r1, s1, theta;
                int f;

                r = sqrtf(x_loop * x_loop + y_loop * y_loop);
                if (r < prd->rRound)
                    r1 = 0.0;
                else
                    r1 = r / (1 - prd->rRound) - 1 / (1 - prd->rRound) + 1;
                s = ssqrt(1 - r * r);
                s1 = ssqrt(1 - r1 * r1);
                if (s1 != 0)
                    theta = atanf(r1 / s1);
                else
                    theta = 0;

                for (f = 0; f < 2; f++) {
                    float b = asinf(sinf(theta) / prd->arRefraction[f]);
                    float a = theta - b;
                    float p = r - s * tanf(a);
                    float q = p / r;

                    /* write the comparison this strange way to pick up
                     * NaNs as well */
                    if (!(q >= -1.0f && q <= 1.0f))
                        q = 1.0f;

                    *(f ? psRefract1++ : psRefract0++) =
                        (unsigned short) ((lrintf(iy * q + size / 2 * (1.0f - q)) << 8) |
                                          lrintf(ix * q + size / 2 * (1.0f - q)));
                }

                BUFX(iy, ix, 0) =
                    clamp((diffuse * (float) prd->aarColour[0][0] * (float) prd->aarColour[0][3] + specular_x) * 64.0f);
                BUFX(iy, ix, 1) =
                    clamp((diffuse * (float) prd->aarColour[0][1] * (float) prd->aarColour[0][3] + specular_x) * 64.0f);
                BUFX(iy, ix, 2) =
                    clamp((diffuse * (float) prd->aarColour[0][2] * (float) prd->aarColour[0][3] + specular_x) * 64.0f);

                BUFO(iy, ix, 0) =
                    clamp((diffuse * (float) prd->aarColour[1][0] * (float) prd->aarColour[1][3] + specular_o) * 64.0f);
                BUFO(iy, ix, 1) =
                    clamp((diffuse * (float) prd->aarColour[1][1] * (float) prd->aarColour[1][3] + specular_o) * 64.0f);
                BUFO(iy, ix, 2) =
                    clamp((diffuse * (float) prd->aarColour[1][2] * (float) prd->aarColour[1][3] + specular_o) * 64.0f);

                BUFX(iy, ix, 3) = clamp(0xFF * 0.25f * ((4 - in) + ((1.0f - (float) prd->aarColour[0][3]) * diffuse)));
                BUFO(iy, ix, 3) = clamp(0xFF * 0.25f * ((4 - in) + ((1.0f - (float) prd->aarColour[1][3]) * diffuse)));
            }
            x_loop += 2.0f / (size);
        }
        y_loop += 2.0f / (size);
    }
#undef BUFX
#undef BUFO
}

extern void
RenderChequerLabels(renderdata * prd, unsigned char *puch, int nStride)
{
    int i;
    unsigned int ip;
#if HAVE_FREETYPE
    FT_Face ftf;
    FT_Glyph aftg[10];
    int fFreetype = FALSE;
    char *file;

    file = BuildFilename(FONT_VERA);
    if (!FT_New_Face(ftl, file, 0, &ftf) && !FT_Set_Pixel_Sizes(ftf, 0, 2 * prd->nSize)) {
        fFreetype = TRUE;
        for (i = 0; i < 10; i++) {
            FT_Load_Char(ftf, '0' + i, FT_LOAD_RENDER);
            FT_Get_Glyph(ftf->glyph, aftg + i);
        }

        FT_Done_Face(ftf);
    }
    g_free(file);
#endif

    for (i = 0; i < 12; i++) {
        FillArea(puch, nStride, CHEQUER_LABEL_WIDTH * prd->nSize, CHEQUER_LABEL_HEIGHT * prd->nSize, 0xC0, 0xC0, 0xC0);

        for (ip = 0; ip < CHEQUER_LABEL_WIDTH * prd->nSize; ip++) {
            puch[3 * ip + nStride + 0] = 0xE0;
            puch[3 * ip + nStride + 1] = 0xE0;
            puch[3 * ip + nStride + 2] = 0xE0;

            puch[3 * ip + (4 * prd->nSize - 2) * nStride + 0] = 0x80;
            puch[3 * ip + (4 * prd->nSize - 2) * nStride + 1] = 0x80;
            puch[3 * ip + (4 * prd->nSize - 2) * nStride + 2] = 0x80;

            puch[ip * nStride + 3 + 0] = 0xE0;
            puch[ip * nStride + 3 + 1] = 0xE0;
            puch[ip * nStride + 3 + 2] = 0xE0;

            puch[ip * nStride + (4 * prd->nSize - 2) * 3 + 0] = 0x80;
            puch[ip * nStride + (4 * prd->nSize - 2) * 3 + 1] = 0x80;
            puch[ip * nStride + (4 * prd->nSize - 2) * 3 + 2] = 0x80;
        }

        for (ip = 0; ip < CHEQUER_LABEL_WIDTH * prd->nSize; ip++) {
            puch[3 * ip + 0] = 0xFF;
            puch[3 * ip + 1] = 0xFF;
            puch[3 * ip + 2] = 0xFF;

            puch[3 * ip + (4 * prd->nSize - 1) * nStride + 0] = 0;
            puch[3 * ip + (4 * prd->nSize - 1) * nStride + 1] = 0;
            puch[3 * ip + (4 * prd->nSize - 1) * nStride + 2] = 0;

            puch[ip * nStride + 0] = 0xFF;
            puch[ip * nStride + 1] = 0xFF;
            puch[ip * nStride + 2] = 0xFF;

            puch[ip * nStride + (4 * prd->nSize - 1) * 3 + 0] = 0;
            puch[ip * nStride + (4 * prd->nSize - 1) * 3 + 1] = 0;
            puch[ip * nStride + (4 * prd->nSize - 1) * 3 + 2] = 0;
        }

#if HAVE_FREETYPE
        if (fFreetype)
            RenderNumber(puch, nStride, aftg, i + 4, 2 * prd->nSize, 45 * prd->nSize / 16, 0, 0, 0);
        else
#endif
            RenderBasicNumber(puch, nStride, 2 * prd->nSize, i + 4, 2 * prd->nSize, 3 * prd->nSize, 0, 0, 0);

        puch += CHEQUER_LABEL_WIDTH * prd->nSize * nStride;
    }

#if HAVE_FREETYPE
    if (fFreetype)
        for (i = 0; i < 10; i++)
            FT_Done_Glyph(aftg[i]);
#endif
}

static void
RenderBasicCube(const float arLight[3], const int nSize, const float arColour[4], unsigned char *puch, int nStride)
{

    int ix, iy, in, fx, fy, i;
    float x, y, x_loop, y_loop, diffuse, specular, cos_theta, x_norm, y_norm, z_norm;

    nStride -= CUBE_WIDTH * nSize * 4;

    for (iy = 0, y_loop = -1.0; iy < CUBE_HEIGHT * nSize; iy++) {
        for (ix = 0, x_loop = -1.0; ix < CUBE_WIDTH * nSize; ix++) {
            in = 0;
            diffuse = specular = 0.0;
            fy = 0;
            y = y_loop;
            do {
                fx = 0;
                x = x_loop;
                do {
                    if (fabsf(x) < 7.0f / 8.0f && fabsf(y) < 7.0f / 8.0f) {
                        /* flat surface */
                        in++;
                        diffuse += arLight[2] * 0.8f + 0.2f;
                        specular += powf(arLight[2], 10) * 0.4f;
                    } else {
                        if (fabsf(x) < 7.0f / 8.0f) {
                            /* top/bottom edge */
                            x_norm = 0.0;
                            y_norm = -7.0f * y - (y > 0.0f ? -6.0f : 6.0f);
                            z_norm = ssqrt(1.0f - y_norm * y_norm);
                        } else if (fabsf(y) < 7.0f / 8.0f) {
                            /* left/right edge */
                            x_norm = 7.0f * x + (x > 0.0f ? -6.0f : 6.0f);
                            y_norm = 0.0;
                            z_norm = ssqrt(1.0f - x_norm * x_norm);
                        } else {
                            /* corner */
                            x_norm = 7.0f * x + (x > 0.0f ? -6.0f : 6.0f);
                            y_norm = -7.0f * y - (y > 0.0f ? -6.0f : 6.0f);
                            if ((z_norm = 1 - x_norm * x_norm - y_norm * y_norm) < 0.0f)
                                goto missed;
                            z_norm = sqrtf(z_norm);
                        }

                        in++;
                        diffuse += 0.2f;
                        if ((cos_theta = arLight[0] * x_norm + arLight[1] * y_norm + arLight[2] * z_norm) > 0.0f) {
                            diffuse += cos_theta * 0.8f;
                            cos_theta = 2 * z_norm * cos_theta - arLight[2];
                            specular += powf(cos_theta, 10) * 0.4f;
                        }
                    }
                  missed:
                    x += 1.0f / (CUBE_WIDTH * nSize);
                } while (!fx++);
                y += 1.0f / (CUBE_HEIGHT * nSize);
            } while (!fy++);

            for (i = 0; i < 3; i++)
                *puch++ = clamp((diffuse * arColour[i] + specular) * 64.0f);

            *puch++ = (unsigned char) (255 * (4 - in) / 4);     /* alpha channel */

            x_loop += 2.0f / (CUBE_WIDTH * nSize);
        }
        y_loop += 2.0f / (CUBE_HEIGHT * nSize);
        puch += nStride;
    }
}



static void
RenderResign(renderdata * prd, unsigned char *puch, int nStride)
{

    const float arColour[4] = { 1.0f, 1.0f, 1.0f, 0.0f };  /* white */

    RenderBasicCube(prd->arLight, prd->nSize, arColour, puch, nStride);

}

extern void
RenderCube(renderdata * prd, unsigned char *puch, int nStride)
{

    RenderBasicCube(prd->arLight, prd->nSize, prd->arCubeColour, puch, nStride);

}

extern void
RenderCubeFaces(renderdata * prd, unsigned char *puch, int nStride, unsigned char *puchCube, int nStrideCube)
{
    int i;
#if HAVE_FREETYPE
    FT_Face ftf;
    FT_Glyph aftg[10], aftgSmall[10];
    int fFreetype = FALSE;
    char *file;

    file = BuildFilename(FONT_VERA_SERIF_BOLD);
    if (!FT_New_Face(ftl, file, 0, &ftf) && !FT_Set_Pixel_Sizes(ftf, 0, 3 * prd->nSize)) {
        fFreetype = TRUE;

        for (i = 0; i < 10; i++) {
            FT_Load_Char(ftf, '0' + i, FT_LOAD_RENDER);
            FT_Get_Glyph(ftf->glyph, aftg + i);
        }

        FT_Set_Pixel_Sizes(ftf, 0, 2 * prd->nSize);

        for (i = 0; i < 10; i++) {
            FT_Load_Char(ftf, '0' + i, FT_LOAD_RENDER);
            FT_Get_Glyph(ftf->glyph, aftgSmall + i);
        }

        FT_Done_Face(ftf);
    }
    g_free(file);
#endif

    for (i = 0; i < 6; i++) {
        /* Clear destination buffer (no blending at present - so all overwriten anyway) */
        memset(puch, 0, nStride * CUBE_LABEL_HEIGHT * prd->nSize);

        AlphaBlendBase(puch, nStride, puch, nStride, puchCube + prd->nSize * 4 +
                       prd->nSize * nStrideCube, nStrideCube,
                       CUBE_LABEL_WIDTH * prd->nSize, CUBE_LABEL_HEIGHT * prd->nSize);

#if HAVE_FREETYPE
        if (fFreetype)
            RenderNumber(puch, nStride, aftg, 2 << i, 2 * prd->nSize, 3 * prd->nSize, 0, 0, 0x80);
        else
#endif
            RenderBasicNumber(puch, nStride, 4 * prd->nSize, 2 << i, 3 * prd->nSize, 5 * prd->nSize, 0, 0, 0x80);

        puch += CUBE_LABEL_WIDTH * prd->nSize * nStride;
    }

    for (; i < 12; i++) {
        /* Clear destination buffer (no blending at present - so all overwriten anyway) */
        memset(puch, 0, nStride * CUBE_LABEL_HEIGHT * prd->nSize);

        AlphaBlendBase(puch, nStride, puch, nStride, puchCube + prd->nSize * 4 +
                       prd->nSize * nStrideCube, nStrideCube,
                       CUBE_LABEL_WIDTH * prd->nSize, CUBE_LABEL_HEIGHT * prd->nSize);

#if HAVE_FREETYPE
        if (fFreetype)
            RenderNumber(puch, nStride, aftgSmall, 2 << i, 2 * prd->nSize, 3 * prd->nSize, 0, 0, 0x80);
        else
#endif
            RenderBasicNumber(puch, nStride, 3 * prd->nSize, 2 << i, 3 * prd->nSize, 4 * prd->nSize, 0, 0, 0x80);

        puch += CUBE_LABEL_WIDTH * prd->nSize * nStride;
    }

#if HAVE_FREETYPE
    if (fFreetype)
        for (i = 0; i < 10; i++) {
            FT_Done_Glyph(aftg[i]);
            FT_Done_Glyph(aftgSmall[i]);
        }
#endif
}


static void
RenderResignFaces(renderdata * prd, unsigned char *puch, int nStride, unsigned char *puchCube, int nStrideCube)
{
    int i;

#if HAVE_FREETYPE
    FT_Face ftf;
    FT_Glyph aftg[10], aftgSmall[10];
    int fFreetype = FALSE;
    char *file;

    file = BuildFilename(FONT_VERA_SERIF_BOLD);
    if (!FT_New_Face(ftl, file, 0, &ftf) && !FT_Set_Pixel_Sizes(ftf, 0, 4 * prd->nSize)) {
        fFreetype = TRUE;

        for (i = 0; i < 10; i++) {
            FT_Load_Char(ftf, '0' + i, FT_LOAD_RENDER);
            FT_Get_Glyph(ftf->glyph, aftg + i);
        }

        FT_Set_Pixel_Sizes(ftf, 0, 3 * prd->nSize);

        for (i = 0; i < 10; i++) {
            FT_Load_Char(ftf, '0' + i, FT_LOAD_RENDER);
            FT_Get_Glyph(ftf->glyph, aftgSmall + i);
        }

        FT_Done_Face(ftf);
    }
    g_free(file);
#endif

    for (i = 0; i < 3; i++) {
        /* Clear destination buffer (no blending at present - so all overwriten anyway) */
        memset(puch, 0, nStride * RESIGN_LABEL_HEIGHT * prd->nSize);

        AlphaBlendBase(puch, nStride, puch, nStride, puchCube + prd->nSize * 4 +
                       prd->nSize * nStrideCube, nStrideCube,
                       RESIGN_LABEL_WIDTH * prd->nSize, RESIGN_LABEL_HEIGHT * prd->nSize);

#if HAVE_FREETYPE
        if (fFreetype)
            RenderNumber(puch, nStride, aftg, i + 1, 2 * prd->nSize, (7 * prd->nSize) / 2, 0, 0, 0x80);
        else
#endif
            RenderBasicNumber(puch, nStride, 4 * prd->nSize, i + 1, 3 * prd->nSize, 5 * prd->nSize, 0, 0, 0x80);

        puch += RESIGN_LABEL_WIDTH * prd->nSize * nStride;
    }

#if HAVE_FREETYPE
    if (fFreetype)
        for (i = 0; i < 10; i++) {
            FT_Done_Glyph(aftg[i]);
            FT_Done_Glyph(aftgSmall[i]);
        }
#endif
}

#define circ(x,y) ssqrt( (x*x) + (y*y) )

extern void
RenderDice(renderdata * prd, unsigned char *puch0, unsigned char *puch1, int nStride, int alpha)
{

    unsigned int ix, iy;
    int in, fx, fy, i;
    float x, y, x_loop, y_loop, diffuse, specular_x, specular_o, cos_theta, x_norm, y_norm, z_norm;
    float *aarDiceColour[2];
    float arDiceCoefficient[2], arDiceExponent[2];

    nStride -= 4 * DIE_WIDTH * prd->nSize;

    for (i = 0; i < 2; i++)
        if (prd->afDieColour[i]) {
            /* die same color as chequers */
            aarDiceColour[i] = prd->aarColour[i];
            arDiceCoefficient[i] = prd->arCoefficient[i];
            arDiceExponent[i] = prd->arExponent[i];
        } else {
            /* user color */
            aarDiceColour[i] = prd->aarDiceColour[i];
            arDiceCoefficient[i] = prd->arDiceCoefficient[i];
            arDiceExponent[i] = prd->arDiceExponent[i];
        }

    for (iy = 0, y_loop = -1.0; iy < DIE_HEIGHT * prd->nSize; iy++) {
        for (ix = 0, x_loop = -1.0; ix < DIE_WIDTH * prd->nSize; ix++) {
            in = 0;
            diffuse = specular_x = specular_o = 0.0;
            fy = 0;
            y = y_loop;
            do {
                fx = 0;
                x = x_loop;
                do {
                    if (circ(x, y) < 1.0f) {
                        /* flat surface */
                        in++;
                        diffuse += prd->arLight[2] * 0.8f + 0.2f;
                        specular_x += powf(prd->arLight[2], arDiceExponent[0]) * arDiceCoefficient[0];
                        specular_o += powf(prd->arLight[2], arDiceExponent[1]) * arDiceCoefficient[1];
                    } else {
                        /* corner */
                        x_norm = 0.707f * x;    /* - ( x > 0.0 ? 1.0 : 1.0);  */
                        y_norm = -0.707f * y;   /* - ( y > 0.0 ? 1.0 : 1.0 ); */
                        z_norm = 1 - x_norm * x_norm - y_norm * y_norm;
                        z_norm = ssqrt(z_norm);

                        in++;
                        diffuse += 0.2f;
                        if ((cos_theta = prd->arLight[0] * x_norm +
                             prd->arLight[1] * y_norm + prd->arLight[2] * z_norm) > 0.0f) {
                            diffuse += cos_theta * 0.8f;
                            if ((cos_theta = 2 * z_norm * cos_theta - prd->arLight[2]) > 0.0f) {
                                specular_x += powf(cos_theta, arDiceExponent[0]) * arDiceCoefficient[0];
                                specular_o += powf(cos_theta, arDiceExponent[1]) * arDiceCoefficient[1];
                            }
                        }
                    }
                    x += 1.0f / (DIE_WIDTH * prd->nSize);
                } while (!fx++);
                y += 1.0f / (DIE_HEIGHT * prd->nSize);
            } while (!fy++);

            for (i = 0; i < 3; i++)
                *puch0++ = clamp((diffuse * aarDiceColour[0][i] + specular_x) * 64.0f);
            if (alpha)
                *puch0++ = (unsigned char) (255 * (4 - in) / 4);        /* alpha channel */

            for (i = 0; i < 3; i++)
                *puch1++ = clamp((diffuse * aarDiceColour[1][i] + specular_o) * 64.0f);
            if (alpha)
                *puch1++ = (unsigned char) (255 * (4 - in) / 4);        /* alpha channel */

            x_loop += 2.0f / (DIE_WIDTH * prd->nSize);
        }
        y_loop += 2.0f / (DIE_HEIGHT * prd->nSize);
        puch0 += nStride;
        puch1 += nStride;
    }
}

extern void
RenderPips(renderdata * prd, unsigned char *puch0, unsigned char *puch1, int nStride)
{

    unsigned int ix, iy;
    int in, fx, fy, i;
    float x, y, z, x_loop, y_loop, diffuse, specular_x, specular_o, cos_theta, dice_top[2][3];
    float *aarDiceColour[2];
    float arDiceCoefficient[2], arDiceExponent[2];

    nStride -= 3 * prd->nSize;

    for (i = 0; i < 2; i++)
        if (prd->afDieColour[i]) {
            /* die same color as chequers */
            aarDiceColour[i] = prd->aarColour[i];
            arDiceCoefficient[i] = prd->arCoefficient[i];
            arDiceExponent[i] = prd->arExponent[i];
        } else {
            /* user color */
            aarDiceColour[i] = prd->aarDiceColour[i];
            arDiceCoefficient[i] = prd->arDiceCoefficient[i];
            arDiceExponent[i] = prd->arDiceExponent[i];
        }

    diffuse = prd->arLight[2] * 0.8f + 0.2f;
    specular_x = powf(prd->arLight[2], arDiceExponent[0]) * arDiceCoefficient[0];
    specular_o = powf(prd->arLight[2], arDiceExponent[1]) * arDiceCoefficient[1];
    dice_top[0][0] = (diffuse * aarDiceColour[0][0] + specular_x) * 64.0f;
    dice_top[0][1] = (diffuse * aarDiceColour[0][1] + specular_x) * 64.0f;
    dice_top[0][2] = (diffuse * aarDiceColour[0][2] + specular_x) * 64.0f;
    dice_top[1][0] = (diffuse * aarDiceColour[1][0] + specular_o) * 64.0f;
    dice_top[1][1] = (diffuse * aarDiceColour[1][1] + specular_o) * 64.0f;
    dice_top[1][2] = (diffuse * aarDiceColour[1][2] + specular_o) * 64.0f;

    for (iy = 0, y_loop = -1.0; iy < prd->nSize; iy++) {
        for (ix = 0, x_loop = -1.0; ix < prd->nSize; ix++) {
            in = 0;
            diffuse = specular_x = specular_o = 0.0;
            fy = 0;
            y = y_loop;
            do {
                fx = 0;
                x = x_loop;
                do {
                    if ((z = 1.0f - x * x - y * y) > 0.0f) {
                        in++;
                        diffuse += 0.2f;
                        z = sqrtf(z) * 5.0f;
                        if ((cos_theta = (-prd->arLight[0] * x +
                                          prd->arLight[1] * y +
                                          prd->arLight[2] * z) / sqrtf(x * x + y * y + z * z)) > 0) {
                            diffuse += cos_theta * 0.8f;
                            if ((cos_theta = 2 * z / 5 * cos_theta - prd->arLight[2]) > 0.0f) {
                                specular_x += powf(cos_theta, arDiceExponent[0]) * arDiceCoefficient[0];
                                specular_o += powf(cos_theta, arDiceExponent[1]) * arDiceCoefficient[1];
                            }
                        }
                    }
                    x += 1.0f / (prd->nSize);
                } while (!fx++);
                y += 1.0f / (prd->nSize);
            } while (!fy++);

            for (i = 0; i < 3; i++)
                *puch0++ =
                    clamp((diffuse * (float) prd->aarDiceDotColour[0][i] + specular_x) * 64.0f +
                          (4 - in) * dice_top[0][i]);

            for (i = 0; i < 3; i++)
                *puch1++ =
                    clamp((diffuse * (float) prd->aarDiceDotColour[1][i] + specular_o) * 64.0f +
                          (4 - in) * dice_top[1][i]);

            x_loop += 2.0f / (prd->nSize);
        }
        y_loop += 2.0f / (prd->nSize);
        puch0 += nStride;
        puch1 += nStride;
    }
}

static void
Copy_RGB_to_RGBA(unsigned char *puchDest, int nDestStride,
                 unsigned char *puchSrc, int nSrcStride, int cx, int cy, unsigned char uchAlpha)
{
    /* copy an 24-bit RGB buffer into an 24+8-bit RGBA buffer, setting
     * the alpha channel to uchAlpha */

    int x;

    nDestStride -= cx * 4;      /* 8 bit alpha + 24 packed rgb bits */
    nSrcStride -= cx * 3;       /* 24 packed rgb bits */

    for (; cy; cy--) {
        for (x = cx; x; x--) {
            *puchDest++ = *puchSrc++;
            *puchDest++ = *puchSrc++;
            *puchDest++ = *puchSrc++;
            *puchDest++ = uchAlpha;
        }
        puchDest += nDestStride;
        puchSrc += nSrcStride;
    }
}

#if USE_GTK && HAVE_CAIRO
static float
Highlight(float c)
{
    if (c < .75f)
        return c + .25f;
    else
        return c - .25f;
}

static void
RenderArrow(unsigned char *puch, float arColour[4], int nSize, int left)
{
    cairo_surface_t *surface = cairo_image_surface_create_for_data(puch, CAIRO_FORMAT_ARGB32,
                                                                   nSize * ARROW_WIDTH,
                                                                   nSize * ARROW_HEIGHT,
                                                                   nSize * ARROW_WIDTH * 4);
    cairo_t *cr = cairo_create(surface);

#define AR_LINE_WIDTH 0.06
#define AR_WIDTH 0.35
#define AR_HEAD_SIZE 0.35

    cairo_scale(cr, nSize * ARROW_WIDTH, nSize * ARROW_HEIGHT);

    /* Point arrows correct way */
    cairo_translate(cr, .5, .5);
    if (left)
        cairo_rotate(cr, -G_PI / 2);
    else
        cairo_rotate(cr, G_PI / 2);
    cairo_translate(cr, -.5, -.5);

    cairo_set_line_width(cr, AR_LINE_WIDTH);

    /* clean the canvas */
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_rectangle(cr, 0, 0, 1, 1);
    cairo_fill(cr);

    cairo_move_to(cr, (1 - AR_WIDTH) / 2, 1 - AR_LINE_WIDTH);
    cairo_line_to(cr, 1 - (1 - AR_WIDTH) / 2, 1 - AR_LINE_WIDTH);
    cairo_line_to(cr, 1 - (1 - AR_WIDTH) / 2, AR_HEAD_SIZE);
    cairo_line_to(cr, 1 - AR_LINE_WIDTH, AR_HEAD_SIZE);
    cairo_line_to(cr, 0.5, AR_LINE_WIDTH);
    cairo_line_to(cr, AR_LINE_WIDTH, AR_HEAD_SIZE);
    cairo_line_to(cr, (1 - AR_WIDTH) / 2, AR_HEAD_SIZE);
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, (double) arColour[2], (double) arColour[1], (double) arColour[0], 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, (double) Highlight(arColour[2]), (double) Highlight(arColour[1]), (double) Highlight(arColour[0]), 1);
    cairo_stroke(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

extern void
RenderArrows(renderdata * prd, unsigned char *puch0, unsigned char *puch1, int UNUSED(nStride), int fClockwise)
{
    RenderArrow(puch0, prd->aarColour[0], prd->nSize, fClockwise);
    RenderArrow(puch1, prd->aarColour[1], prd->nSize, fClockwise);
}
#endif

static void
DrawChequers(renderdata * prd, unsigned char *puch, int nStride,
             renderimages * pri, int iPoint, int n, int f, int x, int y, int cx, int cy)
{
    int i, c, yChequer;

    c = (iPoint == 0 || iPoint == 25) ? 3 : 5;
    yChequer = positions[prd->fClockwise][iPoint][1] * prd->nSize;

    for (i = 0; i < n; i++) {
        RefractBlendClip(puch, nStride,
                         positions[prd->fClockwise][iPoint][0] *
                         prd->nSize - x, yChequer - y, cx, cy,
                         pri->ach, BOARD_WIDTH * prd->nSize * 3,
                         positions[prd->fClockwise][iPoint][0] *
                         prd->nSize, yChequer, pri->achChequer[f],
                         CHEQUER_WIDTH * prd->nSize * 4, 0, 0,
                         pri->asRefract[f],
                         CHEQUER_WIDTH * prd->nSize, CHEQUER_WIDTH * prd->nSize, CHEQUER_HEIGHT * prd->nSize);

        if (i == c - 1)
            break;

        yChequer -= positions[prd->fClockwise][iPoint][2] * prd->nSize;
    }

    if (n > c)
        CopyAreaClip(puch, nStride,
                     (positions[prd->fClockwise][iPoint][0] + 1) *
                     prd->nSize - x, yChequer + prd->nSize - y, cx, cy,
                     pri->achChequerLabels, 4 * prd->nSize * 3,
                     0, 4 * prd->nSize * (n - 4), 4 * prd->nSize, 4 * prd->nSize);
}

extern void
CalculateArea(renderdata * prd, unsigned char *puch, int nStride,
              renderimages * pri, TanBoard anBoard,
              int *anOff, const unsigned int anDice[2],
              int anDicePosition[2][2],
              int fDiceColour, int anCubePosition[2],
              int nLogCube, int nCubeOrientation,
              int anResignPosition[2],
              int fResign, int nResignOrientation,
              int anArrowPosition[2], int UNUSED(fPlaying), int nPlayer, int x, int y, int cx, int cy)
{

    int i, xPoint, yPoint, cxPoint, cyPoint, n;
    int anOffCalc[2];

    if (x < 0) {
        puch -= x * 3;
        cx += x;
        x = 0;
    }

    if (y < 0) {
        puch -= y * nStride;
        cy += y;
        y = 0;
    }

    if (x + cx > BOARD_WIDTH * (int) prd->nSize)
        cx = BOARD_WIDTH * prd->nSize - x;

    if (y + cy > BOARD_HEIGHT * (int) prd->nSize)
        cy = BOARD_HEIGHT * prd->nSize - y;

    if (cx <= 0 || cy <= 0)
        return;

    /* draw board */
    CopyArea(puch, nStride, pri->ach + x * 3 + y * BOARD_WIDTH * prd->nSize * 3, BOARD_WIDTH * prd->nSize * 3, cx, cy);

    if (!anOff) {
        anOff = anOffCalc;
        anOff[0] = anOff[1] = 15;
        for (i = 0; i < 25; i++) {
            anOff[0] -= anBoard[0][i];
            anOff[1] -= anBoard[1][i];
        }
    }

    /* draw labels */

    if (intersects(x, y, cx, cy, 0, 0, BOARD_WIDTH * prd->nSize, 3 * prd->nSize)) {

        AlphaBlendClip(puch, nStride,
                       -x, -y,
                       cx, cy,
                       puch, nStride,
                       -x, -y,
                       pri->achLabels[prd->fDynamicLabels ? nPlayer : 1],
                       BOARD_WIDTH * prd->nSize * 4, 0, 0, BOARD_WIDTH * prd->nSize, 3 * prd->nSize);

    }

    if (intersects(x, y, cx, cy, 0, (BOARD_HEIGHT - BORDER_HEIGHT) * prd->nSize,
                   BOARD_WIDTH * prd->nSize, 3 * prd->nSize)) {

        AlphaBlendClip(puch, nStride,
                       -x, (BOARD_HEIGHT - BORDER_HEIGHT) * prd->nSize - y,
                       cx, cy,
                       puch, nStride,
                       -x, (BOARD_HEIGHT - BORDER_HEIGHT) * prd->nSize - y,
                       pri->achLabels[prd->fDynamicLabels ? !nPlayer : 0],
                       BOARD_WIDTH * prd->nSize * 4, 0, 0, BOARD_WIDTH * prd->nSize, 3 * prd->nSize);

    }

    /* draw points */

    for (i = 0; i < 28; i++) {
        PointArea(prd->fClockwise, prd->nSize, i, &xPoint, &yPoint, &cxPoint, &cyPoint);
        if (intersects(x, y, cx, cy, xPoint, yPoint, cxPoint, cyPoint)) {
            switch (i) {
            case 0:
                /* top player on bar */
                n = -anBoard[0][24];
                break;
            case 25:
                /* bottom player on bar */
                n = anBoard[1][24];
                break;
            case 26:
                /* bottom player borne off */
                n = anOff[1];
                break;
            case 27:
                /* top player borne off */
                n = -anOff[0];
                break;
            default:
                /* ordinary point */
                n = anBoard[1][i - 1] - anBoard[0][24 - i];
                break;
            }
            if (n)
                DrawChequers(prd, puch, nStride, pri, i, abs(n), n > 0, x, y, cx, cy);
        }
    }

    /* draw dice */
    for (i = 0; i < 2; i++)
        if (anDice[i] && intersects(x, y, cx, cy, anDicePosition[i][0] * prd->nSize,
                                    anDicePosition[i][1] * prd->nSize,
                                    DIE_WIDTH * prd->nSize, DIE_HEIGHT * prd->nSize)) {
            int ix, iy, afPip[9], n = anDice[i];

            AlphaBlendClip(puch, nStride,
                           anDicePosition[i][0] * prd->nSize - x,
                           anDicePosition[i][1] * prd->nSize - y,
                           cx, cy, puch, nStride,
                           anDicePosition[i][0] * prd->nSize - x,
                           anDicePosition[i][1] * prd->nSize - y,
                           pri->achDice[fDiceColour],
                           prd->nSize * DIE_WIDTH * 4, 0, 0, prd->nSize * DIE_WIDTH, prd->nSize * DIE_HEIGHT);

            afPip[0] = afPip[8] = (n == 4) || (n == 5) || (n == 6) ||
                (((n == 2) || (n == 3)) && anDicePosition[i][0] & 1);
            afPip[1] = afPip[7] = n == 6 && !(anDicePosition[i][0] & 1);
            afPip[2] = afPip[6] = (n == 4) || (n == 5) || (n == 6) ||
                (((n == 2) || (n == 3)) && !(anDicePosition[i][0] & 1));
            afPip[3] = afPip[5] = n == 6 && anDicePosition[i][0] & 1;
            afPip[4] = n & 1;

            for (iy = 0; iy < 3; iy++)
                for (ix = 0; ix < 3; ix++)
                    if (afPip[iy * 3 + ix])
                        CopyAreaClip(puch, nStride,
                                     (2 * anDicePosition[i][0] + 3 + 3 * ix) * prd->nSize / 2 - x,
                                     (2 * anDicePosition[i][1] + 3 + 3 * iy) * prd->nSize / 2 - y,
                                     cx, cy, pri->achPip[fDiceColour], prd->nSize * 3, 0, 0, prd->nSize, prd->nSize);
        }

    /* draw cube */
    if (nLogCube >= 0 && intersects(x, y, cx, cy,
                                    anCubePosition[0] * prd->nSize,
                                    anCubePosition[1] * prd->nSize,
                                    CUBE_WIDTH * prd->nSize, CUBE_HEIGHT * prd->nSize)) {
        AlphaBlendClip(puch, nStride,
                       anCubePosition[0] * prd->nSize - x,
                       anCubePosition[1] * prd->nSize - y,
                       cx, cy, puch, nStride,
                       anCubePosition[0] * prd->nSize - x,
                       anCubePosition[1] * prd->nSize - y,
                       pri->achCube, prd->nSize * CUBE_WIDTH * 4,
                       0, 0, prd->nSize * CUBE_WIDTH, prd->nSize * CUBE_HEIGHT);

        if (nLogCube < 1)
            nLogCube = 6;       /* 64 */
        else if (nLogCube > 11)
            nLogCube = 12;      /* 4096 */

        CopyAreaRotateClip(puch, nStride,
                           (anCubePosition[0] + 1) * prd->nSize - x,
                           (anCubePosition[1] + 1) * prd->nSize - y,
                           cx, cy, pri->achCubeFaces,
                           prd->nSize * CUBE_LABEL_WIDTH * 3,
                           0, prd->nSize * CUBE_LABEL_WIDTH * (nLogCube - 1),
                           prd->nSize * CUBE_LABEL_WIDTH, prd->nSize * CUBE_LABEL_HEIGHT, nCubeOrientation + 1);
    }

    /* draw resignation */

    if (fResign && intersects(x, y, cx, cy,
                              anResignPosition[0] * prd->nSize,
                              anResignPosition[1] * prd->nSize,
                              RESIGN_WIDTH * prd->nSize, RESIGN_HEIGHT * prd->nSize)) {

        AlphaBlendClip(puch, nStride,
                       anResignPosition[0] * prd->nSize - x,
                       anResignPosition[1] * prd->nSize - y,
                       cx, cy, puch, nStride,
                       anResignPosition[0] * prd->nSize - x,
                       anResignPosition[1] * prd->nSize - y,
                       pri->achResign, prd->nSize * RESIGN_WIDTH * 4,
                       0, 0, prd->nSize * RESIGN_WIDTH, prd->nSize * RESIGN_HEIGHT);

        CopyAreaRotateClip(puch, nStride,
                           (anResignPosition[0] + 1) * prd->nSize - x,
                           (anResignPosition[1] + 1) * prd->nSize - y,
                           cx, cy, pri->achResignFaces,
                           prd->nSize * RESIGN_LABEL_WIDTH * 3,
                           0, prd->nSize * RESIGN_LABEL_WIDTH * (fResign - 1),
                           prd->nSize * RESIGN_LABEL_WIDTH, prd->nSize * RESIGN_LABEL_HEIGHT, nResignOrientation + 1);
    }

    /* draw arrow for direction of play */
#if USE_GTK
    if (prd->showMoveIndicator &&
        intersects(x, y, cx, cy,
                   anArrowPosition[0], anArrowPosition[1], ARROW_WIDTH * prd->nSize, ARROW_HEIGHT * prd->nSize)) {
        AlphaBlendClip2(puch, nStride,
                        anArrowPosition[0] - x, anArrowPosition[1] - y,
                        cx, cy,
                        puch, nStride,
                        anArrowPosition[0] - x, anArrowPosition[1] - y,
                        (unsigned char *) pri->auchArrow[nPlayer],
                        prd->nSize * ARROW_WIDTH * 4, 0, 0, prd->nSize * ARROW_WIDTH, prd->nSize * ARROW_HEIGHT);
    }
#else
    (void) anArrowPosition;     /* suppress unused parameter compiler warning */
#endif
}

extern void
RenderBoardLabels(renderdata * prd, unsigned char *achLo, unsigned char *achHi, int UNUSED(nStride))
{

    unsigned char *achTemp = malloc(BOARD_WIDTH * prd->nSize * 5 * prd->nSize * 5);

    /* 12 11 10 9 8 7 - 6 5 4 3 2 1 */

    memset(achTemp, 0, BOARD_WIDTH * prd->nSize * 3 * prd->nSize * 3);
    RenderLabels(prd, achTemp, prd->nSize * BOARD_WIDTH * 3, 1, 12, 1);

    Copy_RGB_to_RGBA(achLo, prd->nSize * BOARD_WIDTH * 4,
                     achTemp, prd->nSize * BOARD_WIDTH * 3, prd->nSize * BOARD_WIDTH, prd->nSize * 3, 0xFF);

    /* 13 14 15 16 17 18 - 19 20 21 22 24 24 */

    memset(achTemp, 0, BOARD_WIDTH * prd->nSize * 3 * prd->nSize * 3);
    RenderLabels(prd, achTemp, prd->nSize * BOARD_WIDTH * 3, 24, 13, -1);

    Copy_RGB_to_RGBA(achHi, prd->nSize * BOARD_WIDTH * 4,
                     achTemp, prd->nSize * BOARD_WIDTH * 3, prd->nSize * BOARD_WIDTH, prd->nSize * 3, 0xFF);

    free(achTemp);

}

extern void
RenderImages(renderdata * prd, renderimages * pri)
{

    int i;
    int nSize = prd->nSize;

    pri->ach = malloc(nSize * nSize * BOARD_WIDTH * BOARD_HEIGHT * 3);
    pri->achChequer[0] = malloc(nSize * nSize * CHEQUER_WIDTH * CHEQUER_HEIGHT * 4);
    pri->achChequer[1] = malloc(nSize * nSize * CHEQUER_WIDTH * CHEQUER_HEIGHT * 4);
    pri->achChequerLabels = malloc(nSize * nSize * CHEQUER_WIDTH * CHEQUER_HEIGHT * 3 * 12);
    pri->achDice[0] = malloc(nSize * nSize * DIE_WIDTH * DIE_HEIGHT * 4);
    pri->achDice[1] = malloc(nSize * nSize * DIE_WIDTH * DIE_HEIGHT * 4);
    pri->achPip[0] = malloc(nSize * nSize * 3);
    pri->achPip[1] = malloc(nSize * nSize * 3);
    pri->achCube = malloc(nSize * nSize * CUBE_WIDTH * CUBE_HEIGHT * 4);
    pri->achCubeFaces = malloc(nSize * nSize * CUBE_WIDTH * CUBE_HEIGHT * 3 * 12);
    pri->asRefract[0] = malloc(nSize * nSize * CHEQUER_WIDTH * CHEQUER_HEIGHT * sizeof(unsigned short));
    pri->asRefract[1] = malloc(nSize * nSize * CHEQUER_WIDTH * CHEQUER_HEIGHT * sizeof(unsigned short));
    pri->achResign = malloc(nSize * nSize * RESIGN_WIDTH * RESIGN_HEIGHT * 4);
    pri->achResignFaces = malloc(nSize * nSize * RESIGN_WIDTH * RESIGN_HEIGHT * 3 * 3);
#if USE_GTK
    pri->auchArrow[0] = malloc(prd->nSize * prd->nSize * ARROW_WIDTH * ARROW_HEIGHT * 4);
    pri->auchArrow[1] = malloc(prd->nSize * prd->nSize * ARROW_WIDTH * ARROW_HEIGHT * 4);
#else
    pri->auchArrow[0] = NULL;
    pri->auchArrow[1] = NULL;
#endif
    for (i = 0; i < 2; ++i)
        pri->achLabels[i] = malloc(nSize * nSize * BOARD_WIDTH * BORDER_HEIGHT * 4);

    RenderBoard(prd, pri->ach, BOARD_WIDTH * nSize * 3);
    RenderChequers(prd, pri->achChequer[0], pri->achChequer[1],
                   pri->asRefract[0], pri->asRefract[1], nSize * CHEQUER_WIDTH * 4);
    RenderChequerLabels(prd, pri->achChequerLabels, nSize * CHEQUER_LABEL_WIDTH * 3);
    RenderDice(prd, pri->achDice[0], pri->achDice[1], nSize * DIE_WIDTH * 4, TRUE);
    RenderPips(prd, pri->achPip[0], pri->achPip[1], nSize * 3);
    RenderCube(prd, pri->achCube, nSize * CUBE_WIDTH * 4);
    RenderCubeFaces(prd, pri->achCubeFaces, nSize * CUBE_LABEL_WIDTH * 3, pri->achCube, nSize * CUBE_WIDTH * 4);
    RenderResign(prd, pri->achResign, nSize * RESIGN_WIDTH * 4);
    RenderResignFaces(prd, pri->achResignFaces, nSize * RESIGN_LABEL_WIDTH * 3,
                      pri->achResign, nSize * RESIGN_WIDTH * 4);
#if USE_GTK && HAVE_CAIRO
    if (prd->showMoveIndicator)
        RenderArrows(prd, pri->auchArrow[0], pri->auchArrow[1], nSize * ARROW_WIDTH * 4, prd->fClockwise);
#endif

    RenderBoardLabels(prd, pri->achLabels[0], pri->achLabels[1], BOARD_WIDTH * nSize * 4);

}

extern void
FreeImages(renderimages * pri)
{

    int i;

    free(pri->ach);
    free(pri->achChequer[0]);
    free(pri->achChequer[1]);
    free(pri->achChequerLabels);
    free(pri->achDice[0]);
    free(pri->achDice[1]);
    free(pri->achPip[0]);
    free(pri->achPip[1]);
    free(pri->achCube);
    free(pri->achCubeFaces);
    free(pri->asRefract[0]);
    free(pri->asRefract[1]);
    free(pri->achResign);
    free(pri->achResignFaces);
#if USE_GTK
    free(pri->auchArrow[0]);
    free(pri->auchArrow[1]);
#endif
    for (i = 0; i < 2; ++i)
        free(pri->achLabels[i]);
}

extern void
RenderInitialise(void)
{

    memcpy((void *) GetMainAppearance(), &rdDefault, sizeof(rdDefault));

    irandinit(&rc, FALSE);

#if HAVE_FREETYPE
    FT_Init_FreeType(&ftl);
#endif
}

extern void
RenderFinalise(void)
{
#if HAVE_FREETYPE
    FT_Done_FreeType(ftl);
#endif
}

static int
TolComp(float f1, float f2)
{
    return fabsf(f1 - f2) < .005f;
}

static int
ColourCompare(float c1[4], float c2[4])
{
    return TolComp(c1[0], c2[0]) && TolComp(c1[1], c2[1]) && TolComp(c1[2], c2[2]) && TolComp(c1[3], c2[3]);
}

#if USE_BOARD3D

extern int
MaterialCompare(Material * pMat1, Material * pMat2)
{
    return TolComp(pMat1->ambientColour[0], pMat2->ambientColour[0]) &&
        TolComp(pMat1->ambientColour[1], pMat2->ambientColour[1]) &&
        TolComp(pMat1->ambientColour[2], pMat2->ambientColour[2]) &&
        TolComp(pMat1->ambientColour[3], pMat2->ambientColour[3]) &&
        TolComp(pMat1->diffuseColour[0], pMat2->diffuseColour[0]) &&
        TolComp(pMat1->diffuseColour[1], pMat2->diffuseColour[1]) &&
        TolComp(pMat1->diffuseColour[2], pMat2->diffuseColour[2]) &&
        TolComp(pMat1->diffuseColour[3], pMat2->diffuseColour[3]) &&
        TolComp(pMat1->specularColour[0], pMat2->specularColour[0]) &&
        TolComp(pMat1->specularColour[1], pMat2->specularColour[1]) &&
        TolComp(pMat1->specularColour[2], pMat2->specularColour[2]) &&
        TolComp(pMat1->specularColour[3], pMat2->specularColour[3]) &&
        pMat1->shine == pMat2->shine && pMat1->alphaBlend == pMat2->alphaBlend;
}

static int
MaterialTextCompare(Material * pMat1, Material * pMat2)
{
    return MaterialCompare(pMat1, pMat2) &&
        ((!pMat1->textureInfo && !pMat2->textureInfo) ||
         (pMat1->textureInfo && pMat2->textureInfo &&
          !strcmp(MaterialGetTextureFilename(pMat1), MaterialGetTextureFilename(pMat2))));
}
#endif

extern int
PreferenceCompare(renderdata * prd1, renderdata * prd2)
{
#if USE_BOARD3D
    if (display_is_3d(prd1)) {  /* 3d settings */
        return (prd1->pieceType == prd2->pieceType &&
                prd1->fHinges3d == prd2->fHinges3d &&
                prd1->pieceTextureType == prd2->pieceTextureType &&
                prd1->roundedEdges == prd2->roundedEdges &&
                prd1->bgInTrays == prd2->bgInTrays &&
                prd1->roundedPoints == prd2->roundedPoints &&
                prd1->lightType == prd2->lightType &&
                (prd1->lightPos[0] == prd2->lightPos[0] &&
                 prd1->lightPos[1] == prd2->lightPos[1] &&
                 prd1->lightPos[2] == prd2->lightPos[2]) &&
                (prd1->lightLevels[0] == prd2->lightLevels[0] &&
                 prd1->lightLevels[1] == prd2->lightLevels[1] &&
                 prd1->lightLevels[2] == prd2->lightLevels[2]) &&
                !memcmp(prd1->afDieColour3d, prd2->afDieColour3d, sizeof(prd1->afDieColour3d)) &&
                MaterialTextCompare(&prd1->ChequerMat[0], &prd2->ChequerMat[0]) &&
                MaterialCompare(&prd1->ChequerMat[1], &prd2->ChequerMat[1]) &&
                (prd1->afDieColour3d[0] ||
                 MaterialCompare(&prd1->DiceMat[0], &prd2->DiceMat[0])) &&
                (prd1->afDieColour3d[1] ||
                 MaterialCompare(&prd1->DiceMat[1], &prd2->DiceMat[1])) &&
                MaterialCompare(&prd1->DiceDotMat[0], &prd2->DiceDotMat[0]) &&
                MaterialCompare(&prd1->DiceDotMat[1], &prd2->DiceDotMat[1]) &&
                MaterialCompare(&prd1->CubeMat, &prd2->CubeMat) &&
                MaterialCompare(&prd1->CubeNumberMat, &prd2->CubeNumberMat) &&
                MaterialCompare(&prd1->PointNumberMat, &prd2->PointNumberMat) &&
                MaterialTextCompare(&prd1->BaseMat, &prd2->BaseMat) &&
                MaterialTextCompare(&prd1->PointMat[0], &prd2->PointMat[0]) &&
                MaterialTextCompare(&prd1->PointMat[1], &prd2->PointMat[1]) &&
                MaterialTextCompare(&prd1->BoxMat, &prd2->BoxMat) &&
                MaterialTextCompare(&prd1->HingeMat, &prd2->HingeMat) &&
                MaterialTextCompare(&prd1->BackGroundMat, &prd2->BackGroundMat)
            );
    } else
#endif
    {                           /* 2d settings */
        return (prd1->wt == prd2->wt && prd1->fHinges == prd2->fHinges &&
                prd1->rRound == prd2->rRound &&
                ColourCompare(prd1->aarColour[0], prd2->aarColour[0]) &&
                ColourCompare(prd1->aarColour[1], prd2->aarColour[1]) &&
                !memcmp(prd1->afDieColour, prd2->afDieColour, sizeof(prd1->afDieColour)) &&
                (prd1->afDieColour[0] ||
                 (ColourCompare(prd1->aarDiceColour[0], prd2->aarDiceColour[0]) &&
                  prd1->arDiceCoefficient[0] == prd2->arDiceCoefficient[0] &&
                  prd1->arDiceExponent[0] == prd2->arDiceExponent[0])) &&
                (prd1->afDieColour[1] ||
                 (ColourCompare(prd1->aarDiceColour[1], prd2->aarDiceColour[1]) &&
                  prd1->arDiceCoefficient[1] == prd2->arDiceCoefficient[1] &&
                  prd1->arDiceExponent[1] == prd2->arDiceExponent[1])) &&
                ColourCompare(prd1->aarDiceDotColour[0], prd2->aarDiceDotColour[0]) &&
                ColourCompare(prd1->aarDiceDotColour[1], prd2->aarDiceDotColour[1]) &&
                ColourCompare(prd1->arCubeColour, prd2->arCubeColour) &&
                (TolComp(prd1->arLight[0], prd2->arLight[0]) &&
                 TolComp(prd1->arLight[1], prd2->arLight[1]) &&
                 TolComp(prd1->arLight[2], prd2->arLight[2])) &&
                !memcmp(prd1->aanBoardColour[0], prd2->aanBoardColour[0], sizeof(prd1->aanBoardColour[0])) &&
                !memcmp(prd1->aanBoardColour[1], prd2->aanBoardColour[1], sizeof(prd1->aanBoardColour[1])) &&
                !memcmp(prd1->aanBoardColour[2], prd2->aanBoardColour[2], sizeof(prd1->aanBoardColour[2])) &&
                !memcmp(prd1->aanBoardColour[3], prd2->aanBoardColour[3], sizeof(prd1->aanBoardColour[3])) &&
                prd1->arRefraction[0] == prd2->arRefraction[0] &&
                prd1->arRefraction[1] == prd2->arRefraction[1] &&
                prd1->arExponent[0] == prd2->arExponent[0] &&
                prd1->arExponent[1] == prd2->arExponent[1] &&
                prd1->aSpeckle[0] == prd2->aSpeckle[0] &&
                prd1->aSpeckle[1] == prd2->aSpeckle[1] &&
                prd1->aSpeckle[2] == prd2->aSpeckle[2] && prd1->aSpeckle[3] == prd2->aSpeckle[3]
            );
    }
}
