/* This program is free software; you can redistribute it and/or modify
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
 * $Id: simpleboard.h,v 1.8 2013/06/16 02:16:21 mdpetch Exp $
 */

#if HAVE_PANGOCAIRO

#define SIZE_1PERPAGE 250.0f
#define SIZE_2PERPAGE 150.0f

typedef struct _SimpleBoardColor SimpleBoardColor;

/** \brief struct to hold the colors for points, dice, checkers, etc.*/
struct _SimpleBoardColor {
    float fill[3];
    float stroke[3];
    float text[3];
};

typedef struct _SimpleBoard SimpleBoard;
struct _SimpleBoard {
    SimpleBoardColor color_checker[2];
    SimpleBoardColor color_point[2];
    SimpleBoardColor color_cube;
    matchstate ms;
    gchar *annotation;
    gchar *header;
    gint text_size;
    gdouble size;
    gdouble surface_x;
    gdouble surface_y;
    cairo_t *cr;
};

extern gint simple_board_draw(SimpleBoard * board);

extern SimpleBoard *simple_board_new(matchstate * ms, cairo_t * cr, float simple_board_size);

extern void simple_board_destroy(SimpleBoard * board);
#endif
