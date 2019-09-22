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
 * $Id: simpleboard.c,v 1.25 2015/01/11 22:48:14 plm Exp $
 */

/*! \file simpleboard.c
 * \brief exporting a matchstate and board to a cairo drawing context
 */

#include "config.h"
#if HAVE_PANGOCAIRO
#include <cairo.h>
#include <pango/pangocairo.h>
#include <glib.h>
#include <string.h>
#include "backgammon.h"
#include "positionid.h"
#include "matchid.h"
#include "simpleboard.h"

/*! \brief get number of checkers from a backgammon variation
 *
 */
static gint
checkers_from_bgv(bgvariation bgv)
{
    switch (bgv) {
    case VARIATION_STANDARD:
    case VARIATION_NACKGAMMON:
        return 15;
    case VARIATION_HYPERGAMMON_1:
        return 1;
    case VARIATION_HYPERGAMMON_2:
        return 2;
    case VARIATION_HYPERGAMMON_3:
        return 3;
    default:
        g_assert_not_reached();
        return 0;
    }
}

/*! \brief fills and stroke the active context
 *
 */
static void
fill_and_stroke(cairo_t * cr, SimpleBoardColor c)
{
    cairo_set_source_rgb(cr, c.fill[0], c.fill[1], c.fill[2]);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, c.stroke[0], c.stroke[1], c.stroke[2]);
    cairo_stroke(cr);
}

/*! \brief calculates the point position and direction up or down
 * 
 * points are numbered 0 to 23 counter clockwize for the top player
 */
static void
get_point_base(gint i, gint * x, gint * y, gint * direction)
{
    g_assert(i >= 0 && i < 24);

    if (i > 17) {
        *x = 170 + (i - 18) * 20;
        *y = 270;
        *direction = -1;
    } else if (i > 11) {
        *x = 30 + (i - 12) * 20;
        *y = 270;
        *direction = -1;
    } else if (i > 5) {
        *x = 30 + (11 - i) * 20;
        *y = 30;
        *direction = 1;
    } else if (i > -1) {
        *x = 170 + (5 - i) * 20;
        *y = 30;
        *direction = 1;
    }
}

/*! \brief draws a string in monospace, left oriented, multiple lines allowed
 * 
 */
static void
draw_fixed_text(cairo_t * cr, const char *text, gint text_size)
{
    PangoLayout *layout;
    PangoFontDescription *desc;
    gchar *font;

    layout = pango_cairo_create_layout(cr);
    font = g_strdup_printf("monospace %d", text_size);
    desc = pango_font_description_from_string(font);
    g_free(font);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);
    pango_layout_set_text(layout, text, -1);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
}

/*! \brief draws the header text, max 3 lines
 * 
 */
static void
draw_header_text(SimpleBoard * board)
{
    if (!board->header)
        return;
    cairo_move_to(board->cr, board->text_size * 3, board->text_size * 3);
    draw_fixed_text(board->cr, board->header, board->text_size);
    cairo_translate(board->cr, 0.0, board->text_size * 6);
}

/*! \brief draws the annotation text
 * 
 */
static void
draw_annotation_text(SimpleBoard * board)
{
    if (!board->annotation)
        return;
    cairo_move_to(board->cr, board->text_size * 3, 0.0);
    draw_fixed_text(board->cr, board->annotation, board->text_size);
}


/*! \brief draws a string in sans, centered horizonally and vertically
 * 
 */
static void
draw_centered_text(cairo_t * cr, float color[3], gfloat text_size, const char *text)
{
    PangoLayout *layout;
    gint width, height;
    gchar *f_text;
    gchar buf[G_ASCII_DTOSTR_BUF_SIZE];


    cairo_save(cr);

    layout = pango_cairo_create_layout(cr);
    g_ascii_formatd(buf, G_ASCII_DTOSTR_BUF_SIZE, "%.1f", text_size);
    f_text = g_strdup_printf("<span "
                             "font_desc=\"Sans %s\" "
                             "foreground=\"#%.2x%.2x%.2x\" >"
                             "%s"
                             "</span>",
                             buf, (int) (color[0] * 255.0f), (int) (color[1] * 255.0f), (int) (color[2] * 255.0f), text);
    pango_layout_set_markup(layout, f_text, -1);
    pango_layout_get_size(layout, &width, &height);
    cairo_rel_move_to(cr, -((double) width / PANGO_SCALE) / 2, -((double) height / PANGO_SCALE) / 2);
    pango_cairo_update_layout(cr, layout);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
    g_free(f_text);
    cairo_restore(cr);
}

static void
draw_points(SimpleBoard * board)
{
    gint x, y, direction, i;
    gchar *number;
    cairo_t *cr = board->cr;
    for (i = 0; i < 24; i++) {
        SimpleBoardColor color = board->color_point[i % 2];
        gint point_nr;

        get_point_base(i, &x, &y, &direction);

        cairo_save(cr);
        cairo_translate(cr, x, y);
        cairo_move_to(cr, -10, 0);
        cairo_line_to(cr, 0, direction * 100);
        cairo_line_to(cr, 10, 0);
        cairo_close_path(cr);

        fill_and_stroke(cr, color);

        cairo_move_to(cr, 0, 0);
        point_nr = board->ms.fMove ? 24 - i : i + 1;
        number = g_strdup_printf("%d", point_nr);
        cairo_move_to(cr, 0, -10 * direction);
        draw_centered_text(cr, color.text, 8.0, number);
        g_free(number);
        cairo_restore(cr);
    }
}

static void
draw_cube(SimpleBoard * board)
{
    gint x = 150, y = 150;
    gchar *text;
    SimpleBoardColor color = board->color_cube;
    cairo_t *cr = board->cr;
    gint cube;

    cube = board->ms.nCube;
    if (board->ms.fDoubled) {
        if (board->ms.fMove)
            x = 200;
        else
            x = 100;
        cube *= 2;
    } else {
        switch (board->ms.fCubeOwner) {
        case -1:
            break;
        case 0:
            y = 40;
            break;
        case 1:
            y = 260;
            break;
        default:
            g_assert_not_reached();
        }
    }

    cairo_rectangle(cr, x - 8, y - 8, 16, 16);
    fill_and_stroke(cr, color);
    text = g_strdup_printf("%d", cube);
    cairo_move_to(cr, x, y);
    draw_centered_text(cr, color.text, 10.0f - 2 * floorf(log10f(cube)), text);
    g_free(text);
}

static void
draw_dice(SimpleBoard * board)
{
    gint i, x, y, move;
    guint *dice;
    cairo_t *cr;

    move = board->ms.fMove;
    dice = board->ms.anDice;
    cr = board->cr;
    if (!dice[0] || !dice[1])
        return;

    for (i = 0; i < 2; i++) {
        x = (move ? 200 : 62) + i * 22;
        y = 140 + i * 4;
        cairo_rectangle(cr, x, y, 16, 16);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_stroke(cr);
        switch (dice[i]) {
        case 1:
            cairo_move_to(cr, x + 9, y + 8);
            cairo_arc(cr, x + 8, y + 8, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            break;
        case 2:
            cairo_move_to(cr, x + 5, y + 4);
            cairo_arc(cr, x + 4, y + 4, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 13, y + 12);
            cairo_arc(cr, x + 12, y + 12, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            break;
        case 3:
            cairo_move_to(cr, x + 5, y + 4);
            cairo_arc(cr, x + 4, y + 4, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 9, y + 8);
            cairo_arc(cr, x + 8, y + 8, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 13, y + 12);
            cairo_close_path(cr);
            cairo_arc(cr, x + 12, y + 12, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            break;
        case 4:
            cairo_move_to(cr, x + 5, y + 4);
            cairo_arc(cr, x + 4, y + 4, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 13, y + 12);
            cairo_arc(cr, x + 12, y + 12, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 13, y + 4);
            cairo_arc(cr, x + 12, y + 4, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 5, y + 12);
            cairo_arc(cr, x + 4, y + 12, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            break;
        case 5:
            cairo_move_to(cr, x + 5, y + 4);
            cairo_arc(cr, x + 4, y + 4, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 13, y + 12);
            cairo_arc(cr, x + 12, y + 12, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 9, y + 8);
            cairo_arc(cr, x + 8, y + 8, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 13, y + 4);
            cairo_arc(cr, x + 12, y + 4, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 5, y + 12);
            cairo_arc(cr, x + 4, y + 12, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            break;
        case 6:
            cairo_move_to(cr, x + 5, y + 4);
            cairo_arc(cr, x + 4, y + 4, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 5, y + 8);
            cairo_arc(cr, x + 4, y + 8, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 5, y + 12);
            cairo_arc(cr, x + 4, y + 12, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 13, y + 4);
            cairo_arc(cr, x + 12, y + 4, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 13, y + 8);
            cairo_arc(cr, x + 12, y + 8, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            cairo_move_to(cr, x + 13, y + 12);
            cairo_arc(cr, x + 12, y + 12, 1, 0, 2 * G_PI);
            cairo_close_path(cr);
            break;
        default:
            g_assert_not_reached();
        }
        cairo_stroke(cr);
    }
}

static void
draw_turn(SimpleBoard * board)
{
    gint i;
    SimpleBoardColor color = board->color_checker[board->ms.fMove];
    cairo_t *cr = board->cr;

    i = board->ms.fMove * 2 - 1;
    cairo_rectangle(cr, 8, 150 - i * 15, 4, i * 16);
    cairo_move_to(cr, 3, 150 + i);
    cairo_line_to(cr, 10, 150 + i * 15);
    cairo_line_to(cr, 17, 150 + i);
    cairo_close_path(cr);
    fill_and_stroke(cr, color);
}

static void
draw_checkers_on_xy(cairo_t * cr, gint x, gint y, gint direction, gint max, gint number, SimpleBoardColor color)
{
    gint p;
    gchar *checker_text;
    for (p = 0; number > 0 && p < max; number--, p++) {
        y += 20 * direction;
        cairo_move_to(cr, x + 9, y);
        cairo_arc(cr, x, y, 9, 0, 2 * G_PI);
        cairo_close_path(cr);
        fill_and_stroke(cr, color);
    }
    if (number) {
        checker_text = g_strdup_printf("%d", number + max);
        cairo_move_to(cr, x, y);
        draw_centered_text(cr, color.text, 7.0, checker_text);
        g_free(checker_text);
    }
}

static void
draw_checkers_on_point(cairo_t * cr, gint point, gint number, SimpleBoardColor color)
{
    gint x, y, direction;

    g_assert(number >= 0 && number <= 15);

    if (!number)
        return;

    get_point_base(point, &x, &y, &direction);
    y -= 10 * direction;
    draw_checkers_on_xy(cr, x, y, direction, 5, number, color);
}

static void
draw_checkers_on_bar(cairo_t * cr, gint i, gint n, SimpleBoardColor color)
{
    gint x = 150;
    gint y = i ? 50 : 250;
    gint direction = i ? 1 : -1;

    draw_checkers_on_xy(cr, x, y, direction, 3, n, color);
}

static void
draw_checkers_off(cairo_t * cr, gint i, gint n, SimpleBoardColor color)
{
    gint x = 290;
    gint y = i ? 280 : 20;
    gint direction = i ? -1 : 1;

    draw_checkers_on_xy(cr, x, y, direction, 3, n, color);
}

static void
draw_checkers(SimpleBoard * board)
{
    gint player, point, p, n, i, unplaced;
    SimpleBoardColor c;
    for (i = 0; i < 2; i++) {
        player = (board->ms.fMove) ? i : !i;
        c = board->color_checker[player];
        unplaced = checkers_from_bgv(board->ms.bgv);
        for (point = 0; point < 24; point++) {
            p = player ? 23 - point : point;
            n = board->ms.anBoard[i][p];
            unplaced -= n;
            draw_checkers_on_point(board->cr, point, n, c);
        }
        n = board->ms.anBoard[i][24];
        unplaced -= n;
        draw_checkers_on_bar(board->cr, player, n, c);
        draw_checkers_off(board->cr, player, unplaced, c);
    }
}

static void
draw_borders(cairo_t * cr)
{
    cairo_save(cr);
    cairo_set_line_width(cr, 2);

    cairo_rectangle(cr, 0, 10, 300, 280);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);

    cairo_rectangle(cr, 20, 30, 260, 240);
    cairo_rectangle(cr, 140, 30, 20, 240);

    cairo_rectangle(cr, 0, 132, 20, 36);
    cairo_rectangle(cr, 280, 132, 20, 36);

    cairo_stroke(cr);
    cairo_restore(cr);
}

/*! \brief sbd
 *
 */
extern int
simple_board_draw(SimpleBoard * board)
{
    cairo_t *cr = board->cr;
    g_assert(cr != NULL);

    cairo_save(cr);
    draw_header_text(board);
    if (board->surface_x != 0.0 && board->size > 0.0)
        cairo_translate(cr, board->surface_x / 2.0 - board->size / 2.0, 0);
    cairo_scale(cr, board->size / 300.0, board->size / 300.0);
    draw_borders(cr);
    draw_points(board);
    draw_cube(board);
    draw_dice(board);
    draw_checkers(board);
    draw_turn(board);
    cairo_restore(cr);
    cairo_save(cr);
    cairo_translate(cr, board->text_size * 3, board->text_size * 6 + board->size);
    draw_annotation_text(board);
    cairo_restore(cr);

    return (0);
}

extern SimpleBoard *
simple_board_new(matchstate * ms, cairo_t * cr, float simple_board_size)
{
    SimpleBoard *board;
    SimpleBoardColor black_black_white = { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f} };
    SimpleBoardColor white_black_black = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    SimpleBoardColor grey_black_black = { {0.7f, 0.7f, 0.7f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

    board = g_new0(SimpleBoard, 1);
    board->color_checker[0] = white_black_black;
    board->color_checker[1] = black_black_white;
    board->color_point[0] = grey_black_black;
    board->color_point[1] = white_black_black;
    board->color_cube = white_black_black;
    board->size = simple_board_size;
    board->text_size = 6;
    if (ms)
        board->ms = *ms;
    board->cr = cr;
    return board;
}

#ifdef SB_STAND_ALONE
static SimpleBoard *
simple_board_new_from_ids(gchar * position_id, gchar * match_id, cairo_t * cr)
{
    SimpleBoard *board;
    g_assert(position_id != NULL && match_id != NULL);

    board = simple_board_new(NULL, cr, SIZE_1PERPAGE);
    if (!PositionFromID(board->ms.anBoard, position_id)) {
        g_free(board);
        return (NULL);
    }

    if (MatchFromID
        (board->ms.anDice, &board->ms.fTurn,
         &board->ms.fResigned, &board->ms.fDoubled, &board->ms.fMove,
         &board->ms.fCubeOwner, &board->ms.fCrawford, &board->ms.nMatchTo, board->ms.anScore, &board->ms.nCube,
         &board->ms.fJacoby,
         &board->ms.gs, match_id)) {
        g_free(board);
        return (NULL);
    }

    return (board);
}

#include <cairo-svg.h>
main(int argc, char *argv[])
{
    SimpleBoard *board;
    cairo_surface_t *surface;
    cairo_t *cr;
    if (argc != 4)
        g_error("wrong number of arguments %d, expected 3\n", argc - 1);

    surface = cairo_svg_surface_create(argv[3], SIMPLE_BOARD_SIZE, SIMPLE_BOARD_SIZE);
    cr = cairo_create(surface);
    board = simple_board_new_from_ids(argv[1], argv[2], cr);
    if (!board)
        g_error("Failed to create simple board from ids %s and %s", argv[1], argv[2]);
    simple_board_draw(board);
    cairo_surface_destroy(surface);
    cairo_destroy(board->cr);
    g_free(board);
}
#endif
#endif
