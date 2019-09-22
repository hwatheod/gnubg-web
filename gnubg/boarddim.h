/*
 * boarddim.h
 *
 * by Holger Bochnig <hbgg@gmx.net>, 2003
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
 * $Id: boarddim.h,v 1.15 2013/06/16 02:16:10 mdpetch Exp $
 */

#ifndef BOARDDIM_H
#define BOARDDIM_H


/* fundamental constants */
#define CHEQUER_WIDTH     6
#define DIE_WIDTH         7
#define CUBE_WIDTH        6
#define ARROW_WIDTH       3
#define HINGE_HEIGHT     12
#define HINGE_WIDTH       2

/* additional space (beyone one chequer's worth) between two points 
 * with 5 chequers */
#define EXTRA_HEIGHT      10

/* derived constants */
#define CHEQUER_HEIGHT    CHEQUER_WIDTH
#define DIE_HEIGHT        DIE_WIDTH
#define CUBE_HEIGHT       CUBE_WIDTH
#define ARROW_HEIGHT      ARROW_WIDTH

#define BORDER_HEIGHT    (CHEQUER_HEIGHT / 2)
#define BORDER_WIDTH     (CHEQUER_WIDTH / 2)
#define BAR_WIDTH        (2 * CHEQUER_WIDTH)
#define BEAROFF_INSIDE    CHEQUER_WIDTH
#define BEAROFF_WIDTH    (BEAROFF_INSIDE + 2 * BORDER_WIDTH)
#define BOARD_WIDTH      (12 * CHEQUER_WIDTH + 2 * BEAROFF_WIDTH + BAR_WIDTH)
#define BEAROFF_DIVIDER_HEIGHT   CHEQUER_HEIGHT

/* the following is technically wrong - it should be 10 chequers, die/cube
 * and two borders - there may be undisplayable positions otherwise */
#define BOARD_MIN_HEIGHT (11 * CHEQUER_HEIGHT + 2 * BORDER_HEIGHT)

#define BOARD_HEIGHT     (BOARD_MIN_HEIGHT + EXTRA_HEIGHT)


/* more derived constants */
#define CUBE_LABEL_WIDTH     (CUBE_WIDTH - 2)
#define CUBE_LABEL_HEIGHT    (CUBE_HEIGHT - 2)
#define RESIGN_WIDTH          CUBE_WIDTH
#define RESIGN_HEIGHT         CUBE_HEIGHT
#define RESIGN_LABEL_WIDTH    CUBE_LABEL_WIDTH
#define RESIGN_LABEL_HEIGHT   CUBE_LABEL_HEIGHT
#define CHEQUER_LABEL_WIDTH  (CHEQUER_WIDTH - 2)
#define CHEQUER_LABEL_HEIGHT (CHEQUER_HEIGHT - 2)
#define POINT_WIDTH           CHEQUER_WIDTH

/* for a tall enough board, let the points extend past a stack of 5 chequers */

#if (EXTRA_HEIGHT >= (CHEQUER_HEIGHT + 4))
#define DISPLAY_POINT_EXTRA      4
#elif (EXTRA_HEIGHT >= (CHEQUER_HEIGHT + 3))
#define DISPLAY_POINT_EXTRA      3
#elif (EXTRA_HEIGHT >= (CHEQUER_HEIGHT + 2))
#define DISPLAY_POINT_EXTRA      2
#elif (EXTRA_HEIGHT >= (CHEQUER_HEIGHT + 1))
#define DISPLAY_POINT_EXTRA      1
#else
#define DISPLAY_POINT_EXTRA      0
#endif

#define DISPLAY_POINT_HEIGHT     (5 * CHEQUER_HEIGHT + DISPLAY_POINT_EXTRA)

/* for HTML */
#define BEAROFF_HEIGHT   (5 * CHEQUER_HEIGHT)
#define POINT_HEIGHT     (5 * CHEQUER_HEIGHT)
#define BOARD_CENTER_WIDTH  (6 * CHEQUER_WIDTH)
#define BOARD_CENTER_HEIGHT (BOARD_HEIGHT - 2 * (BORDER_HEIGHT + POINT_HEIGHT))

#define DISPLAY_BEAROFF_HEIGHT ((BOARD_HEIGHT - BEAROFF_DIVIDER_HEIGHT) / 2 - \
                                 BORDER_HEIGHT)

/* where to place a chequer on the bar  - x */
#define BAR_X ((BOARD_WIDTH - CHEQUER_WIDTH) / 2)
/* where to place the first player 0 chequer on the bar - y */
#define BAR_Y_0 (BOARD_HEIGHT / 2 - 16)
/* and where to place the first player 1 chequer on the bar - y */
#define BAR_Y_1 (BOARD_HEIGHT / 2 + 9)

/* where to start point x, x = 1..13 */

#define POINT_X(n) ((n < 7) ? (BOARD_WIDTH - BEAROFF_WIDTH -     \
                               n * CHEQUER_WIDTH) :              \
                    (n < 13) ? ((BOARD_WIDTH - BAR_WIDTH) / 2 -  \
                               (n - 6) * CHEQUER_WIDTH) :        \
                    (n < 19) ? (BEAROFF_WIDTH +                  \
                                (n - 13) * CHEQUER_WIDTH) :      \
                    (BOARD_WIDTH + BAR_WIDTH) / 2 + (n - 19) * CHEQUER_WIDTH)

/* top and bottom y co-ordinates of chequers */
#define TOP_POINT_Y (BOARD_HEIGHT - BORDER_HEIGHT - CHEQUER_HEIGHT)
#define BOT_POINT_Y (BORDER_HEIGHT)
/* left and right bearoff tray coordinates */
#define BEAROFF_RIGHT_X (BOARD_WIDTH - BORDER_WIDTH  - CHEQUER_WIDTH)
#define BEAROFF_LEFT_X  (BORDER_WIDTH)

/* x and y coordinates of cube when not available/doubling/owned/centred */
#define NO_CUBE -32768
/* width of board between tray and bar */
#define PLAY_WIDTH   (((BOARD_WIDTH - (2 * BEAROFF_WIDTH)) - BAR_WIDTH) / 2)

#define CUBE_RIGHT_X  (BEAROFF_WIDTH + (PLAY_WIDTH - CUBE_WIDTH) / 2)
#define CUBE_LEFT_X   (BOARD_WIDTH - (CUBE_RIGHT_X))
#define CUBE_TRAY_X (BEAROFF_LEFT_X + (BEAROFF_INSIDE - CUBE_WIDTH) / 2)
#define PLAY_HEIGHT   (BOARD_HEIGHT - 2 * BORDER_HEIGHT )
#define CUBE_CENTRE_Y (BORDER_HEIGHT + (PLAY_HEIGHT - CUBE_HEIGHT) / 2)
#define CUBE_OWN_1_Y  (BOARD_HEIGHT - (CUBE_HEIGHT + BORDER_HEIGHT))
#define CUBE_OWN_0_Y  (BORDER_HEIGHT)
#define CUBE_RESIGN_LEFT_X   (CUBE_LEFT_X - CHEQUER_WIDTH)
#define CUBE_RESIGN_RIGHT_X  (CUBE_RIGHT_X + CHEQUER_WIDTH)
/* where the hinges begin - y */
#define HINGE_BOT_Y  ((BOARD_HEIGHT - CUBE_HEIGHT) / 2 - \
                      2 * CHEQUER_HEIGHT - HINGE_HEIGHT - 1)
#define HINGE_TOP_Y ((BOARD_HEIGHT + CUBE_HEIGHT) / 2 + \
                     2 * CHEQUER_HEIGHT + 1)

#endif                          /* BOARDDIM_H */
