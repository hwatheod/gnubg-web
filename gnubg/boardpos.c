/*
 * boardpos.c
 *
 * by Jørn Thyssen <jth@gnubg.org>, 2003.
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
 * $Id: boardpos.c,v 1.18 2013/06/16 02:16:10 mdpetch Exp $
 */

#include "config.h"

#include <glib.h>
#include <stdlib.h>

#include "boarddim.h"
#include "boardpos.h"
#include "common.h"

extern void
ChequerPosition(const int clockwise, const int point, const int chequer, int *px, int *py)
{

    int c_chequer;

    c_chequer = (!point || point == 25) ? 3 : 5;

    if (c_chequer > chequer)
        c_chequer = chequer;

    *px = positions[clockwise][point][0];
    *py = positions[clockwise][point][1] - (c_chequer - 1) * positions[clockwise][point][2];

}


extern void
PointArea(const int fClockwise, const int nSize, const int n, int *px, int *py, int *pcx, int *pcy)
{

    /* max chequer in column */
    int c_chequer = (n == 0 || n == 25) ? 3 : 5;

    *px = positions[fClockwise][n][0] * nSize;
    *py = positions[fClockwise][n][1] * nSize;
    *pcx = CHEQUER_WIDTH * nSize;
    *pcy = positions[fClockwise][n][2] * nSize;

    if (*pcy > 0) {
        *pcy = *pcy * (c_chequer - 1) + (CHEQUER_HEIGHT + DISPLAY_POINT_EXTRA) * nSize;
        *py += CHEQUER_HEIGHT * nSize - *pcy;
    } else
        *pcy = -*pcy * (c_chequer - 1) + (CHEQUER_HEIGHT + DISPLAY_POINT_EXTRA) * nSize;
}


/* Determine the position and rotation of the cube; *px and *py return the
 * position (in board units -- multiply by nSize to get
 * pixels) and *porient returns the rotation (1 = facing the top, 0 = facing
 * the side, -1 = facing the bottom). */
extern void
CubePosition(const int crawford_game, const int cube_use,
             const int doubled, const int cube_owner, int fClockwise, int *px, int *py, int *porient)
{
    if (crawford_game || !cube_use) {
        /* no cube */
        if (px)
            *px = NO_CUBE;
        if (py)
            *py = NO_CUBE;
        if (porient)
            *porient = -1;
    } else if (doubled) {
        if (px)
            *px = (doubled > 0) ? CUBE_RIGHT_X : CUBE_LEFT_X;
        if (py)
            *py = CUBE_CENTRE_Y;
        if (porient)
            *porient = doubled;
    } else {
        if (px) {
            if (fClockwise)
                *px = BOARD_WIDTH - (BEAROFF_INSIDE + CUBE_TRAY_X);
            else
                *px = CUBE_TRAY_X;
        }
        if (py)
            *py = (cube_owner < 0) ? CUBE_OWN_1_Y : (cube_owner == 0) ? CUBE_CENTRE_Y : CUBE_OWN_0_Y;
        if (porient)
            *porient = cube_owner;
    }

}


extern void
ResignPosition(const int resigned, int *px, int *py, int *porient)
{

    if (resigned) {
        if (px)
            *px = (SGN(resigned) < 0) ? CUBE_RESIGN_LEFT_X : CUBE_RESIGN_RIGHT_X;
        if (py)
            *py = CUBE_CENTRE_Y;
        if (porient)
            *porient = -SGN(resigned);
    } else {
        /* no resignation */
        if (px)
            *px = NO_CUBE;
        if (py)
            *py = NO_CUBE;
        if (porient)
            *porient = -1;
    }

}

extern void
ArrowPosition(const int clockwise, int turn, const int nSize, int *px, int *py)
{
    /* calculate the position of the arrow to indicate
     * player on turn and direction of play;  *px and *py
     * return the position of the upper left corner in pixels,
     * NOT board units */

    int Point_x, Point_y, Point_dx, Point_dy;

    PointArea(clockwise, nSize, (turn == 1) ? 26 : 27, &Point_x, &Point_y, &Point_dx, &Point_dy);

    if (px) {
        if (clockwise)
            *px = Point_x + (nSize * BEAROFF_INSIDE);
        else
            *px = Point_x - (nSize * BORDER_WIDTH);
        /* Center arrow in border */
        *px += (nSize * (BORDER_WIDTH - ARROW_WIDTH)) / 2;
    }
    if (py) {
        if (turn == 1)
            *py = Point_y + Point_dy;
        else
            *py = Point_y - (nSize * BORDER_WIDTH);            /*- 2 * nSize * ARROW_HEIGHT; */
        /* Center arrow in border */
        *py += (nSize * (BORDER_WIDTH - ARROW_HEIGHT)) / 2;
    }
}
