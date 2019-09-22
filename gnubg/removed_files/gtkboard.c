/*
 * gtkboard.c
 *
 * by Gary Wong <gtw@gnu.org>, 1997-2001.
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
 * $Id: gtkboard.c,v 1.324 2014/12/23 09:22:24 plm Exp $
 */

/*! \file gtkboard.c
 * \brief gtk board for playing in the GUI
 */

#include "config.h"
#include "gtklocdefs.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <isaac.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backgammon.h"
#include "drawboard.h"
#include "gtkboard.h"
#include "gtkgame.h"
#include "gtk-multiview.h"
#include "gtkprefs.h"
#include "positionid.h"
#include "render.h"
#include "renderprefs.h"
#include "sound.h"
#include "matchid.h"
#include "boardpos.h"
#include "matchequity.h"
#include "gtktoolbar.h"
#include "boarddim.h"
#if USE_BOARD3D
#include "fun3d.h"
#endif

/* minimum time in milliseconds before a drag to the
 * same point is considered a real drag rather than a click */
#define CLICK_TIME 450
/* After this time show drag target help */
#define HINT_TIME 150


animation animGUI = ANIMATE_SLIDE;
int fGUIBeep = TRUE;
int fGUIHighDieFirst = TRUE;
int fGUIIllegal = FALSE;
int fShowIDs = TRUE;
GuiShowPips gui_show_pips = GUI_SHOW_PIPS_EPC;
int fGUIDragTargetHelp = TRUE;
int fGUIGrayEdit = TRUE;

unsigned int nGUIAnimSpeed = 4;
int animate_player, *animate_move_list, animation_finished = TRUE;

static GtkVBoxClass *parent_class = NULL;
static randctx rc;


typedef struct _SetDiceData {
    unsigned char *TTachDice[2], *TTachPip[2], *TTachGrayDice, *TTachGrayPip;
    BoardData *bd;
    manualDiceType mdt;
} SetDiceData;
/*todo - tidy set cube like above */
static unsigned char *TTachCube, *TTachCubeFaces;

#define RAND irand( &rc )

static gint board_set(Board * board, const gchar * board_text, const gint resigned, const gint cube_use);
static void InitialPos(BoardData * bd);


G_DEFINE_TYPE(Board, board, GTK_TYPE_VBOX)
extern GtkWidget *
board_new(renderdata * prd)
{
    /* Create widget */
    GtkWidget *board = g_object_new(TYPE_BOARD, NULL);
    /* Initialize board data members */
    BoardData *bd = BOARD(board)->board_data;
    bd->rd = prd;
    bd->rd->nSize = (unsigned int) -1;

    bd->jacoby_flag = ms.fJacoby = fJacoby;
    bd->crawford_game = 0;
    bd->cube = 1;
    bd->cube_use = 0;
    bd->doubled = 0;
    bd->cube_owner = 0;
    bd->resigned = 0;
    bd->diceShown = DICE_BELOW_BOARD;
    bd->grayBoard = FALSE;
    bd->turn = 0;

    bd->x_dice[0] = bd->y_dice[0] = 0;
    bd->x_dice[1] = bd->y_dice[1] = 0;

    InitialPos(bd);

    bd->move_list.cMoves = 0;

    return board;
}

static void
InitialPos(BoardData * bd)
{                               /* Set up initial board position */
    int ip[] = { 0, -2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, -5, 5, 0, 0, 0, -3, 0, -5, 0, 0, 0, 0, 2, 0, 0, 0 };
    memcpy(bd->points, ip, sizeof(bd->points));
#if USE_BOARD3D
    if (gtk_gl_init_success && ShadowsInitilised(bd->bd3d))
        updatePieceOccPos(bd, bd->bd3d);
#endif
}

extern void
InitBoardPreview(BoardData * bd)
{
    /* Show set position */
    InitialPos(bd);
    bd->cube_use = 1;
    bd->crawford_game = 0;
    bd->jacoby_flag = ms.fJacoby;
    bd->doubled = bd->cube_owner = bd->cube = 0;
    bd->resigned = 0;
    bd->diceShown = DICE_ON_BOARD;
    bd->diceRoll[0] = 4;
    bd->diceRoll[1] = 3;
    bd->turn = 1;
}

static int
intersects(int x0, int y0, int cx0, int cy0, int x1, int y1, int
           cx1, int cy1)
{

    return (y1 + cy1 > y0) && (y1 < y0 + cy0) && (x1 + cx1 > x0) && (x1 < x0 + cx0);
}

extern void
board_beep(BoardData * UNUSED(bd))
{
    if (fGUIBeep)
        gdk_beep();
}

extern void
read_board(BoardData * bd, TanBoard points)
{

    gint i;

    for (i = 0; i < 24; i++) {
        points[bd->turn <= 0][i] = bd->points[24 - i] < 0 ? abs(bd->points[24 - i]) : 0;
        points[bd->turn > 0][i] = bd->points[i + 1] > 0 ? abs(bd->points[i + 1]) : 0;
    }

    points[bd->turn <= 0][24] = abs(bd->points[0]);
    points[bd->turn > 0][24] = abs(bd->points[25]);
}


static void
write_points(gint points[28], const gint turn, const gint nchequers, TanBoard anBoard)
{

    gint i;
    gint anOff[2];
    TanBoard an;

    memcpy(an, anBoard, sizeof an);

    if (turn < 0)
        SwapSides(an);

    for (i = 0; i < 28; ++i)
        points[i] = 0;

    /* Opponent on bar */
    points[0] = -(int) an[0][24];

    /* Board */
    for (i = 0; i < 24; i++) {
        if (an[1][i])
            points[i + 1] = an[1][i];
        if (an[0][i])
            points[24 - i] = -(int) an[0][i];
    }

    /* Player on bar */
    points[25] = an[1][24];

    anOff[0] = anOff[1] = nchequers;
    for (i = 0; i < 25; i++) {
        anOff[0] -= an[0][i];
        anOff[1] -= an[1][i];
    }

    points[26] = anOff[1];
    points[27] = -anOff[0];

}

extern void
write_board(BoardData * bd, TanBoard anBoard)
{

    write_points(bd->points, bd->turn, bd->nchequers, anBoard);

}

static void
chequer_position(int point, int chequer, int *px, int *py)
{

    ChequerPosition(fClockwise, point, chequer, px, py);

}

static void
point_area(BoardData * bd, int n, int *px, int *py, int *pcx, int
           *pcy)
{

    PointArea(fClockwise, bd->rd->nSize, n, px, py, pcx, pcy);

}

static void
resign_position(BoardData * bd, int *px, int *py, int *porient)
{

    ResignPosition(bd->resigned, px, py, porient);

}

static void
RenderArea(BoardData * bd, unsigned char *puch, int x, int y, int
           cx, int cy)
{

    TanBoard anBoard;
    unsigned int anDice[2];
    int anOff[2], anDicePosition[2][2], anCubePosition[2], anArrowPosition[2], nOrient;
    int anResignPosition[2], nResignOrientation;

    read_board(bd, anBoard);
    if (bd->colour != bd->turn)
        SwapSides(anBoard);
    anOff[0] = -bd->points[27];
    anOff[1] = bd->points[26];
    anDice[0] = bd->diceRoll[0];
    anDice[1] = bd->diceRoll[1];
    anDicePosition[0][0] = bd->x_dice[0];
    anDicePosition[0][1] = bd->y_dice[0];
    anDicePosition[1][0] = bd->x_dice[1];
    anDicePosition[1][1] = bd->y_dice[1];
    CubePosition(bd->crawford_game, bd->cube_use, bd->doubled, bd->cube_owner, fClockwise,
                 &anCubePosition[0], &anCubePosition[1], &nOrient);
    resign_position(bd, anResignPosition, anResignPosition + 1, &nResignOrientation);
    ArrowPosition(fClockwise, bd->turn, bd->rd->nSize, &anArrowPosition[0], &anArrowPosition[1]);
    CalculateArea(bd->rd, puch, cx * 3, &bd->ri, anBoard, anOff,
                  anDice, anDicePosition, bd->colour == bd->turn,
                  anCubePosition, LogCube(bd->cube) + (bd->doubled != 0),
                  nOrient, anResignPosition,
                  abs(bd->resigned), nResignOrientation, anArrowPosition, bd->playing, bd->turn == 1, x, y, cx, cy);
}

static gboolean
board_expose(GtkWidget * drawing_area, GdkEventExpose * event, BoardData * bd)
{
    int x, y, cx, cy;
    unsigned char *puch;

    g_assert(GTK_IS_DRAWING_AREA(drawing_area));

    if (bd->rd->nSize == 0)
        return TRUE;

    x = event->area.x;
    y = event->area.y;
    cx = event->area.width;
    cy = event->area.height;

    if (x < 0) {
        cx += x;
        x = 0;
    }

    if (y < 0) {
        cy += y;
        y = 0;
    }

    if (y + cy > BOARD_HEIGHT * (int) bd->rd->nSize)
        cy = BOARD_HEIGHT * bd->rd->nSize - y;

    if (x + cx > BOARD_WIDTH * (int) bd->rd->nSize)
        cx = BOARD_WIDTH * bd->rd->nSize - x;

    if (cx <= 0 || cy <= 0)
        return TRUE;

    puch = malloc(cx * cy * 3);

    RenderArea(bd, puch, x, y, cx, cy);

    /* FIXME use dithalign */
    gdk_draw_rgb_image(gtk_widget_get_window(drawing_area), bd->gc_copy, x, y, cx, cy,
                       GDK_RGB_DITHER_MAX, puch, cx * 3);

    free(puch);

    return TRUE;
}

extern void
stop_board_expose(BoardData * bd)
{
    g_signal_handlers_disconnect_by_func(G_OBJECT(bd->drawing_area), (gpointer)G_CALLBACK(board_expose), bd);
}

static void
board_invalidate_rect(GtkWidget * drawing_area, int x, int y, int
                      cx, int cy, BoardData * UNUSED(bd))
{

    g_assert(GTK_IS_DRAWING_AREA(drawing_area));

    {
        GdkRectangle r;

        r.x = x;
        r.y = y;
        r.width = cx;
        r.height = cy;

        if (gtk_widget_get_window(drawing_area))
            gdk_window_invalidate_rect(gtk_widget_get_window(drawing_area), &r, FALSE);
    }
}

static void
board_invalidate_point(BoardData * bd, int n)
{

    int x, y, cx, cy;

    if (bd->rd->nSize == 0)
        return;

    point_area(bd, n, &x, &y, &cx, &cy);

    board_invalidate_rect(bd->drawing_area, x, y, cx, cy, bd);
}

static void
board_invalidate_dice(BoardData * bd)
{

    int x, y, cx, cy;

    x = bd->x_dice[0] * bd->rd->nSize;
    y = bd->y_dice[0] * bd->rd->nSize;
    cx = DIE_WIDTH * bd->rd->nSize;
    cy = DIE_HEIGHT * bd->rd->nSize;

    board_invalidate_rect(bd->drawing_area, x, y, cx, cy, bd);

    x = bd->x_dice[1] * bd->rd->nSize;
    y = bd->y_dice[1] * bd->rd->nSize;

    board_invalidate_rect(bd->drawing_area, x, y, cx, cy, bd);
}

static void
board_invalidate_labels(BoardData * bd)
{

    int x, y, cx, cy;

    x = 0;
    y = 0;
    cx = BOARD_WIDTH * bd->rd->nSize;
    cy = BORDER_HEIGHT * bd->rd->nSize;

    board_invalidate_rect(bd->drawing_area, x, y, cx, cy, bd);

    x = 0;
    y = (BOARD_HEIGHT - BORDER_HEIGHT) * bd->rd->nSize;
    cx = BOARD_WIDTH * bd->rd->nSize;
    cy = BORDER_HEIGHT * bd->rd->nSize;

    board_invalidate_rect(bd->drawing_area, x, y, cx, cy, bd);

}

static void
board_invalidate_cube(BoardData * bd)
{

    int x, y, orient;

    CubePosition(bd->crawford_game, bd->cube_use, bd->doubled, bd->cube_owner, fClockwise, &x, &y, &orient);

    board_invalidate_rect(bd->drawing_area, x * bd->rd->nSize,
                          y * bd->rd->nSize, CUBE_WIDTH * bd->rd->nSize, CUBE_HEIGHT * bd->rd->nSize, bd);
}

static void
board_invalidate_resign(BoardData * bd)
{

    int x, y, orient;

    resign_position(bd, &x, &y, &orient);

    board_invalidate_rect(bd->drawing_area, x * bd->rd->nSize,
                          y * bd->rd->nSize, RESIGN_WIDTH * bd->rd->nSize, RESIGN_HEIGHT * bd->rd->nSize, bd);
}

static void
board_invalidate_arrow(BoardData * bd)
{

    int x, y;

    ArrowPosition(fClockwise, bd->turn, bd->rd->nSize, &x, &y);

    board_invalidate_rect(bd->drawing_area, x, y, ARROW_WIDTH * bd->rd->nSize, ARROW_HEIGHT * bd->rd->nSize, bd);
}

static int
board_point(GtkWidget * UNUSED(board), BoardData * bd, int x0, int y0)
{

    int i, x, y, cx, cy, xCube, yCube;

    x0 /= bd->rd->nSize;
    y0 /= bd->rd->nSize;

    if (intersects(x0, y0, 0, 0, bd->x_dice[0], bd->y_dice[0], DIE_WIDTH, DIE_HEIGHT) ||
        intersects(x0, y0, 0, 0, bd->x_dice[1], bd->y_dice[1], DIE_WIDTH, DIE_HEIGHT))
        return POINT_DICE;

    CubePosition(bd->crawford_game, bd->cube_use, bd->doubled, bd->cube_owner, fClockwise, &xCube, &yCube, NULL);
    if (intersects(x0, y0, 0, 0, xCube, yCube, CUBE_WIDTH, CUBE_HEIGHT))
        return POINT_CUBE;

    resign_position(bd, &xCube, &yCube, NULL);
    if (intersects(x0, y0, 0, 0, xCube, yCube, CUBE_WIDTH, CUBE_HEIGHT))
        return POINT_RESIGN;

    /* jsc: support for faster rolling of dice by clicking board
     *
     * These arguments should be dynamically calculated instead 
     * of hardcoded, but it's too painful right now.
     */
    if (intersects(x0, y0, 0, 0,
                   (BOARD_WIDTH + BAR_WIDTH) / 2, BORDER_HEIGHT + 5 * CHEQUER_HEIGHT,
                   6 * CHEQUER_WIDTH, BOARD_HEIGHT - 10 * CHEQUER_HEIGHT - BORDER_HEIGHT * 3))
        return POINT_RIGHT;
    else if (intersects(x0, y0, 0, 0,
                        BEAROFF_WIDTH, BORDER_HEIGHT + 5 * CHEQUER_HEIGHT,
                        6 * CHEQUER_WIDTH, BOARD_HEIGHT - 10 * CHEQUER_HEIGHT - BORDER_HEIGHT * 3))
        return POINT_LEFT;

    for (i = 0; i < 28; i++) {
        point_area(bd, i, &x, &y, &cx, &cy);

        x /= bd->rd->nSize;
        y /= bd->rd->nSize;
        cx /= bd->rd->nSize;
        cy /= bd->rd->nSize;

        if (intersects(x0, y0, 0, 0, x, y, cx, cy))
            return i;
    }

    return -1;
}

extern void
update_gnubg_id(BoardData * bd, const TanBoard points)
{

    int anScore[2];
    int fCubeOwner;
    gchar *str;

    anScore[0] = bd->score_opponent;
    anScore[1] = bd->score;

    if (bd->can_double) {
        if (bd->opponent_can_double)
            fCubeOwner = -1;
        else
            fCubeOwner = 1;
    } else {
        fCubeOwner = 0;
    }
    str = g_strdup_printf("%s:%s",
                          PositionID(points),
                          MatchID(bd->diceRoll, ms.fTurn, ms.fResigned,
                                  ms.fDoubled, ms.fMove, fCubeOwner, bd->crawford_game, bd->match_to,
                                  anScore, bd->cube, ms.fJacoby, ms.gs));

    gtk_label_set_text(GTK_LABEL(pwGnubgID), str);
    g_free(str);
}

extern char *
ReturnHits(TanBoard anBoard)
{

    int aiHit[15];
    int i, j, k, l, m, n, c;
    movelist ml;
    int aiDiceHit[6][6];

    memset(aiHit, 0, sizeof(aiHit));
    memset(aiDiceHit, 0, sizeof(aiDiceHit));

    SwapSides(anBoard);

    /* find blots */

    for (i = 0; i < 6; ++i)
        for (j = 0; j <= i; ++j) {

            if (!(c = GenerateMoves(&ml, (ConstTanBoard) anBoard, i + 1, j + 1, FALSE)))
                /* no legal moves */
                continue;

            k = 0;
            for (l = 0; l < 24; ++l)
                if (anBoard[0][l] == 1) {
                    for (m = 0; m < c; ++m) {
                        move *pm = &ml.amMoves[m];
                        for (n = 0; n < 4 && pm->anMove[2 * n] > -1; ++n)
                            if (pm->anMove[2 * n + 1] == (23 - l)) {
                                /* hit */
                                aiHit[k] += 2 - (i == j);
                                ++aiDiceHit[i][j];
                                /* next point */
                                goto nextpoint;
                            }
                    }
                  nextpoint:
                    ++k;
                }

        }

    for (j = 14; j >= 0; --j)
        if (aiHit[j])
            break;

    if (j >= 0) {
        char *pch = g_malloc(3 * (j + 1) + 200);
        strcpy(pch, "");
        for (i = 0; i <= j; ++i)
            if (aiHit[i])
                sprintf(strchr(pch, 0), "%d ", aiHit[i]);

        for (n = 0, i = 0; i < 6; ++i)
            for (j = 0; j <= i; ++j)
                n += (aiDiceHit[i][j] > 0) * (2 - (i == j));

        sprintf(strchr(pch, 0), ngettext("(no hit: %d roll)", "(no hit: %d rolls)", (36 - n)), 36 - n);
        return pch;
    }

    return NULL;

}

static void
show_pip_none(BoardData * UNUSED(bd), const TanBoard UNUSED(points), GString * gst[4])
{
    g_string_append_printf(gst[0], _("n/a"));
    g_string_append_printf(gst[1], _("n/a"));
    g_string_append_printf(gst[2], _("Pips: "));
    g_string_append_printf(gst[3], _("Pips: "));
}

static void
show_pip_pip(BoardData * bd, const TanBoard points, GString * gst[4])
{
    unsigned int anPip[2];
    int f;
    PipCount(points, anPip);
    f = (bd->turn > 0);
    g_string_append_printf(gst[0], "%d (%+d)", anPip[!f], anPip[!f] - anPip[f]);
    g_string_append_printf(gst[1], "%d (%+d)", anPip[f], anPip[f] - anPip[!f]);
    g_string_append_printf(gst[2], _("Pips: "));
    g_string_append_printf(gst[3], _("Pips: "));
}

static void
show_pip_epc(BoardData * bd, const TanBoard points, GString * gst[4])
{
    int f;
    float arEPC[2];
    if (EPC(points, arEPC, NULL, NULL, NULL, TRUE) != 0) {
        show_pip_pip(bd, points, gst);
        return;
    }
    f = (bd->turn > 0);
    g_string_append_printf(gst[0], "%.2f (%+.2f)", arEPC[!f], arEPC[!f] - arEPC[f]);
    g_string_append_printf(gst[1], "%.2f (%+.2f)", arEPC[f], arEPC[f] - arEPC[!f]);
    g_string_append_printf(gst[2], _("EPC: "));
    g_string_append_printf(gst[3], _("EPC: "));
}

static void
show_pip_pwe(BoardData * bd, const TanBoard points, GString * gst[4])
{
    unsigned int anPip[2];
    int f;
    float arEPC[2];
    if (EPC(points, arEPC, NULL, NULL, NULL, TRUE) != 0) {
        show_pip_pip(bd, points, gst);
        return;
    }
    PipCount(points, anPip);
    f = (bd->turn > 0);
    g_string_append_printf(gst[0], " %d + %.2f = %.2f(%+.2f)", anPip[!f],
                           arEPC[!f] - anPip[!f], arEPC[!f], arEPC[!f] - arEPC[f]);
    g_string_append_printf(gst[1], " %d + %.2f = %.2f(%+.2f)", anPip[f],
                           arEPC[f] - anPip[f], arEPC[f], arEPC[f] - arEPC[!f]);
    g_string_append_printf(gst[2], _("EPC: "));
    g_string_append_printf(gst[3], _("EPC: "));
}

extern void
update_pipcount(BoardData * bd, const TanBoard points)
{
    int i;
    GString *gst[4];

    for (i = 0; i < 4; i++)
        gst[i] = g_string_new(NULL);

    switch (gui_show_pips) {
    case GUI_SHOW_PIPS_WASTAGE:
        show_pip_pwe(bd, points, gst);
        break;
    case GUI_SHOW_PIPS_EPC:
        show_pip_epc(bd, points, gst);
        break;
    case GUI_SHOW_PIPS_PIPS:
        show_pip_pip(bd, points, gst);
        break;
    default:
        show_pip_none(bd, points, gst);
    }

    gtk_label_set_text(GTK_LABEL(bd->pipcount0), gst[0]->str);
    gtk_label_set_text(GTK_LABEL(bd->pipcount1), gst[1]->str);
    gtk_label_set_text(GTK_LABEL(bd->pipcountlabel0), gst[2]->str);
    gtk_label_set_text(GTK_LABEL(bd->pipcountlabel1), gst[3]->str);

    for (i = 0; i < 4; i++)
        g_string_free(gst[i], TRUE);

    UpdateTheoryData(bd, TT_PIPCOUNT | TT_EPC | TT_KLEINCOUNT, points);
}

/* A chequer has been moved or the board has been updated -- update the
 * move and position ID labels. */
int
update_move(BoardData * bd)
{
    char *move = _("Illegal move"), move_buf[40];
    unsigned int i;
    TanBoard points;
    positionkey key;
    int fIncomplete = TRUE, fIllegal = TRUE;

    read_board(bd, points);
    update_gnubg_id(bd, (ConstTanBoard) points);
    update_pipcount(bd, (ConstTanBoard) points);

    bd->valid_move = NULL;

    if (ToolbarIsEditing(pwToolbar) && bd->playing) {
        move = _("(Editing)");
        fIncomplete = fIllegal = FALSE;
    } else if (EqualBoards((ConstTanBoard) points, (ConstTanBoard) bd->old_board)) {
        /* no move has been made */
        move = NULL;
        fIncomplete = fIllegal = FALSE;
    } else {
        PositionKey((ConstTanBoard) points, &key);

        for (i = 0; i < bd->move_list.cMoves; i++)
            if (EqualKeys(bd->move_list.amMoves[i].key, key)) {
                bd->valid_move = bd->move_list.amMoves + i;
                fIncomplete = bd->valid_move->cMoves < bd->move_list.cMaxMoves
                    || bd->valid_move->cPips < bd->move_list.cMaxPips;
                fIllegal = FALSE;
                FormatMove(move = move_buf, (ConstTanBoard) bd->old_board, bd->valid_move->anMove);
                break;
            }

        /* show number of return hits */
        UpdateTheoryData(bd, TT_RETURNHITS, msBoard());

        if (bd->valid_move) {
            TanBoard anBoard;
            char *pch;
            PositionFromKey(anBoard, &bd->valid_move->key);
            if ((pch = ReturnHits(anBoard))) {
                outputf(_("Return hits: %s\n"), pch);
                outputx();
                g_free(pch);
            } else {
                outputl("");
                outputx();
            }

        }

    }

    gtk_widget_set_state(bd->wmove, fIncomplete ? GTK_STATE_ACTIVE : GTK_STATE_NORMAL);
    gtk_label_set_text(GTK_LABEL(bd->wmove), move);

    return (fIllegal && !fGUIIllegal) ? -1 : 0;
}

extern void
Confirm(BoardData * bd)
{

    char move[40];
    TanBoard points;

    read_board(bd, points);

    if (!bd->move_list.cMoves && EqualBoards((ConstTanBoard) points, (ConstTanBoard) bd->old_board))
        UserCommand("move");
    else if (bd->valid_move &&
             bd->valid_move->cMoves == bd->move_list.cMaxMoves && bd->valid_move->cPips == bd->move_list.cMaxPips) {
        FormatMovePlain(move, bd->old_board, bd->valid_move->anMove);

        UserCommand(move);
    } else
        /* Illegal move */
        board_beep(bd);
}

static void
board_start_drag(GtkWidget * UNUSED(widget), BoardData * bd, int
                 drag_point, int x, int y)
{

    bd->drag_point = drag_point;
    bd->drag_colour = bd->points[drag_point] < 0 ? -1 : 1;

    bd->points[drag_point] -= bd->drag_colour;

#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {
        SetMovingPieceRotation(bd, bd->bd3d, bd->drag_point);
        updatePieceOccPos(bd, bd->bd3d);
        if (bd->rd->quickDraw)
            RestrictiveStartMouseMove(drag_point, abs(bd->points[drag_point] + bd->drag_colour));
    } else
#endif
    {
        board_invalidate_point(bd, drag_point);

        gdk_window_process_updates(gtk_widget_get_window(bd->drawing_area), FALSE);

        bd->x_drag = x;
        bd->y_drag = y;
    }
}

/*! \brief generates a list of legal (sub)moves
 * \param dice the dice
 * \param real_board the board being dragged from
 * \param old_board the board before the move
 * \param the output list of moves
 */
static void
generate_drag_moves(guint * dice, TanBoard real_board, TanBoard old_board, movelist * pml)
{
    if (dice[0] != dice[1]
        && memcmp(real_board, old_board, sizeof(TanBoard)) != 0) {
        /* only complete moves to disallow moving, e.g., a 2 twice in a 
         * 42 roll */
        GenerateMoves(pml, (ConstTanBoard) old_board, dice[0], dice[1], FALSE);
    } else {
        /* complete and partial moves */
        GenerateMoves(pml, (ConstTanBoard) old_board, dice[0], dice[1], TRUE);
    }
}

/*! \brief the possible legal permutation of dice
 * \param dice the dice
 * \param perm the output permutations
 */
static void
setup_dice_perm(guint * dice, gint perm[][5])
{
    /* long but simple */
    if (dice[0] == dice[1]) {
        perm[0][0] = dice[0];
        perm[0][1] = -1;

        perm[1][0] = dice[0];
        perm[1][1] = dice[0];
        perm[1][2] = -1;

        perm[2][0] = dice[0];
        perm[2][1] = dice[0];
        perm[2][2] = dice[0];
        perm[2][3] = -1;

        perm[3][0] = dice[0];
        perm[3][1] = dice[0];
        perm[3][2] = dice[0];
        perm[3][3] = dice[0];
        perm[3][4] = -1;

    } else {
        perm[0][0] = dice[0];
        perm[0][1] = -1;

        perm[1][0] = dice[1];
        perm[1][1] = -1;

        perm[2][0] = dice[0];
        perm[2][1] = dice[1];
        perm[2][2] = -1;

        perm[3][0] = dice[1];
        perm[3][1] = dice[0];
        perm[3][2] = -1;

    }
}

/*! \brief apply a dice permutation to the board
 * \param board the real board
 * \param from_point the drag point in, the dest point out
 * \param permi the dice permutation
 * \return 1 if the move is legal, 0 otherwise
 */
static int
apply_perm(TanBoard board, int *from_point, int permi[])
{
    int j;
    for (j = 0; permi[j] > 0; j++) {
        if (ApplySubMove(board, *from_point, permi[j], TRUE) != 0)
            return 0;
        *from_point = MAX(-1, *from_point - permi[j]);
    }
    return 1;
}

/*! \brief finds legal destinations for a dragged chequer
 * \param bd The board with a chequer lifted
 * \param dest_points the legal destinations if any
 * \return TRUE when legal destinations are found
 */
static gboolean
legal_dest_points(BoardData * bd, int dest_points[4])
{

    unsigned int i;
    movelist ml;
    int count = 0;
    TanBoard real_board;
    int player = bd->drag_colour == -1 ? 0 : 1;
    int drag_point;
    gint perm[4][5];

    /* initialise */
    for (i = 0; i < 4; i++)
        dest_points[i] = -1;

    g_return_val_if_fail(ap[player].pt == PLAYER_HUMAN, FALSE);

    drag_point = player ? bd->drag_point - 1 : 24 - bd->drag_point;

    /* the current board including the dragged chequer */
    read_board(bd, real_board);
    real_board[1][drag_point]++;

    generate_drag_moves(bd->diceRoll, real_board, bd->old_board, &ml);
    setup_dice_perm(bd->diceRoll, perm);

    for (i = 0; i < 4; i++) {
        TanBoard board;
        int from_point = drag_point;
        memcpy(board, real_board, sizeof(TanBoard));
        if (!apply_perm(board, &from_point, perm[i]))
            continue;
        if (!board_in_list(&ml, (ConstTanBoard) bd->old_board, (ConstTanBoard) board, NULL))
            continue;

        /* from_point is now the final dest */
        if (from_point == -1) {
            /* bearoff */
            dest_points[count++] = player ? 26 : 27;
        } else {
            dest_points[count++] = player ? from_point + 1 : 24 - from_point;
        }

    }
    return count ? TRUE : FALSE;
}

#if USE_BOARD3D
static void
board_drag(GtkWidget * widget, BoardData * bd, int x, int y)
#else
static void
board_drag(GtkWidget * UNUSED(widget), BoardData * bd, int x, int y)
#endif
{

    unsigned char *puch, *puchNew, *puchChequer;
    int s = bd->rd->nSize;

#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {
        if (MouseMove3d(bd, bd->bd3d, bd->rd, x, y))
            gtk_widget_queue_draw(widget);
        return;
    }
#endif

    gdk_window_process_updates(gtk_widget_get_window(bd->drawing_area), FALSE);

    if (s == 0)
        return;

    puch = g_alloca(6 * s * 6 * s * 3);
    puchNew = g_alloca(6 * s * 6 * s * 3);
    puchChequer = g_alloca(6 * s * 6 * s * 3);

    RenderArea(bd, puch, bd->x_drag - 3 * s, bd->y_drag - 3 * s, 6 * s, 6 * s);
    RenderArea(bd, puchNew, x - 3 * s, y - 3 * s, 6 * s, 6 * s);
    RefractBlendClip(puchChequer, 6 * s * 3, 0, 0, 6 * s, 6 * s, puchNew,
                     6 * s * 3, 0, 0,
                     bd->ri.achChequer[bd->drag_colour > 0], 6 * s * 4, 0,
                     0, bd->ri.asRefract[bd->drag_colour > 0], 6 * s, 6 * s, 6 * s);

    /* FIXME use dithalign */
    {
        GdkRegion *pr;
        GdkRectangle r;

        r.x = bd->x_drag - 3 * s;
        r.y = bd->y_drag - 3 * s;
        r.width = 6 * s;
        r.height = 6 * s;
        pr = gdk_region_rectangle(&r);

        r.x = x - 3 * s;
        r.y = y - 3 * s;
        gdk_region_union_with_rect(pr, &r);

        gdk_window_begin_paint_region(gtk_widget_get_window(bd->drawing_area), pr);

        gdk_region_destroy(pr);
    }

    gdk_draw_rgb_image(gtk_widget_get_window(bd->drawing_area), bd->gc_copy,
                       bd->x_drag - 3 * s, bd->y_drag - 3 * s, 6 * s, 6 * s, GDK_RGB_DITHER_MAX, puch, 6 * s * 3);
    gdk_draw_rgb_image(gtk_widget_get_window(bd->drawing_area), bd->gc_copy,
                       x - 3 * s, y - 3 * s, 6 * s, 6 * s, GDK_RGB_DITHER_MAX, puchChequer, 6 * s * 3);
    gdk_window_end_paint(gtk_widget_get_window(bd->drawing_area));
    bd->x_drag = x;
    bd->y_drag = y;
}

static void
board_end_drag(GtkWidget * UNUSED(widget), BoardData * bd)
{

    unsigned char *puch;
    int s = bd->rd->nSize;

    gdk_window_process_updates(gtk_widget_get_window(bd->drawing_area), FALSE);

    if (s == 0)
        return;

    puch = g_alloca(6 * s * 6 * s * 3);

    RenderArea(bd, puch, bd->x_drag - 3 * s, bd->y_drag - 3 * s, 6 * s, 6 * s);

    /* FIXME use dithalign */
    gdk_draw_rgb_image(gtk_widget_get_window(bd->drawing_area), bd->gc_copy,
                       bd->x_drag - 3 * s, bd->y_drag - 3 * s, 6 * s, 6 * s, GDK_RGB_DITHER_MAX, puch, 6 * s * 3);
}

/* This code is called on a button release event
 *
 * Since this code has side-effects, it should only be called from
 * button_release_event().  Mainly, it assumes that a checker
 * has been picked up.  If the move fails, that pickup will be reverted.
 */

gboolean
place_chequer_or_revert(BoardData * bd, int dest)
{
    /* This procedure has grown more complicated than I like
     * We might want to discard most of it and use a calculated 
     * list of valid destination points (since we have the code for that available 
     * 
     * Known problems:
     *    
     * Not tested for "allow dragging to illegal points". Unlikely to work, might crash 
     * It is not possible to drag checkers from the bearoff tray. This must be corrected in 
     * the pick-up code - this proc should be ready for it.
     *
     * Nis Jorgensen, 2004-06-17
     */

    int hitCheckers[4] = { 0, 0, 0, 0 };
    int unhitCheckers[4] = { 0, 0, 0, 0 };
    int bar, hit = 0;
    gboolean placed = TRUE;
    int unhit = 0;
    int oldpoints[28];
    int passpoint;
    int source, dest2, i;
    int diceset = (bd->diceRoll[0] >= 1 && bd->diceRoll[0] <= 6 && bd->diceRoll[1] >= 1 && bd->diceRoll[1] <= 6);

    /* dest2 is the destination point used  for numerical calculations */
    dest2 = dest;

    source = bd->drag_point;


    /* This is the opponents bar point */
    bar = bd->drag_colour == bd->colour ? 25 - bd->bar : bd->bar;


    if (dest == -1 || (bd->drag_colour > 0 ? bd->points[dest] < -1 : bd->points[dest] > 1) || dest == bar || dest > 27) {
        placed = FALSE;
        dest = dest2 = source;
    } else if (dest >= 26) {
        /* bearing off */
        dest = bd->drag_colour > 0 ? 26 : 27;
        dest2 = bd->drag_colour > 0 ? 0 : 25;
    }


    /* Check for hits, undoing hits, including pick-and pass */


    if (((source - dest2) * bd->drag_colour > 0)        /*We are moving forward */
        ||ToolbarIsEditing(pwToolbar)) {        /* Or in edit mode */
        if (bd->points[dest] == -bd->drag_colour) {
            /* outputf ("Hitting on %d \n", dest); */
            hit++;
            hitCheckers[0] = dest;
            bd->points[dest] = 0;
            bd->points[bar] -= bd->drag_colour;
            board_invalidate_point(bd, bar);
        }

        if (diceset && (bd->diceRoll[0] == bd->diceRoll[1])) {
            for (i = 1; i <= 3; i++) {
                passpoint = source - i * bd->diceRoll[0] * bd->drag_colour;
                if ((dest2 - passpoint) * bd->drag_colour >= 0)
                    break;
                if (bd->points[passpoint] == -bd->drag_colour) {
                    hit++;
                    hitCheckers[i] = passpoint;
                    bd->points[passpoint] += bd->drag_colour;
                    bd->points[bar] -= bd->drag_colour;
                    board_invalidate_point(bd, bar);
                    board_invalidate_point(bd, passpoint);
                }
            }
        } else {
            if (diceset && (Abs(source - dest2) == bd->diceRoll[0] + bd->diceRoll[1] ||
                            (dest > 25 && Abs(source - dest2) > MAX(bd->diceRoll[0], bd->diceRoll[1]))
                ))
                for (i = 0; i < 2; i++) {
                    passpoint = source - bd->diceRoll[i] * bd->drag_colour;
                    if ((dest2 - passpoint) * bd->drag_colour >= 0)
                        continue;
                    if (bd->points[passpoint] == -bd->drag_colour) {
                        hit++;
                        hitCheckers[i + 1] = passpoint;
                        bd->points[passpoint] += bd->drag_colour;
                        bd->points[bar] -= bd->drag_colour;
                        board_invalidate_point(bd, bar);
                        board_invalidate_point(bd, passpoint);

                        break;
                    }
                }
        }
    } else if ((source - dest2) * bd->drag_colour < 0) {

        /* 
         * Check for taking chequer off point where we hit 
         */

        /* check if the opponent had a chequer on the drag point (source),
         * and that it's not pick'n'pass */

        write_points(oldpoints, bd->turn, bd->nchequers, bd->old_board);

        if (oldpoints[source] == -bd->drag_colour) {
            unhit++;
            unhitCheckers[0] = source;
            bd->points[bar] += bd->drag_colour;
            board_invalidate_point(bd, bar);
            bd->points[source] -= bd->drag_colour;
            board_invalidate_point(bd, source);
        }

        if (diceset && (bd->diceRoll[0] == bd->diceRoll[1])) {
            /* Doubles are tricky - we can have pick-and-passed with 2 chequers */
            for (i = 1; i <= 3; i++) {
                passpoint = source + i * bd->diceRoll[0] * bd->drag_colour;
                if ((dest2 - passpoint) * bd->drag_colour <= 0)
                    break;
                if ((oldpoints[passpoint] == -bd->drag_colour) &&       /*there was a blot */
                    (bd->points[passpoint] == 0) &&     /* We actually did p&p */
                    bd->points[source] == oldpoints[source]) {  /* We didn't p&p with a second checker */
                    unhit++;
                    unhitCheckers[i] = passpoint;
                    bd->points[bar] += bd->drag_colour;
                    bd->points[passpoint] -= bd->drag_colour;
                    board_invalidate_point(bd, bar);
                    board_invalidate_point(bd, passpoint);
                }
            }
        } else if (diceset) {
            for (i = 0; i < 2; i++) {
                passpoint = source + bd->diceRoll[i] * bd->drag_colour;
                if ((dest2 - passpoint) * bd->drag_colour <= 0)
                    continue;
                if ((oldpoints[passpoint] == -bd->drag_colour) && (bd->points[passpoint] == 0)) {
                    unhit++;
                    unhitCheckers[i + 1] = passpoint;
                    bd->points[bar] += bd->drag_colour;
                    board_invalidate_point(bd, bar);
                    bd->points[passpoint] -= bd->drag_colour;
                    board_invalidate_point(bd, passpoint);
                }
            }

        }

    }

    bd->points[dest] += bd->drag_colour;
    board_invalidate_point(bd, dest);


    if (source != dest) {
        if (update_move(bd)) {
            /* the move was illegal; undo it */
            bd->points[source] += bd->drag_colour;
            board_invalidate_point(bd, source);
            bd->points[dest] -= bd->drag_colour;
            board_invalidate_point(bd, dest);
            if (hit > 0) {
                bd->points[bar] += hit * bd->drag_colour;
                board_invalidate_point(bd, bar);
                for (i = 0; i < 4; i++)
                    if (hitCheckers[i] > 0) {
                        bd->points[hitCheckers[i]] = -bd->drag_colour;
                        board_invalidate_point(bd, hitCheckers[i]);
                    }
            }

            if (unhit > 0) {
                bd->points[bar] -= unhit * bd->drag_colour;
                board_invalidate_point(bd, bar);
                for (i = 0; i < 4; i++)
                    if (unhitCheckers[i] > 0) {
                        bd->points[unhitCheckers[i]] += bd->drag_colour;
                        board_invalidate_point(bd, unhitCheckers[i]);
                    }
            }

            update_move(bd);
            placed = FALSE;
        }
    }

    board_invalidate_point(bd, placed ? dest : source);

#if USE_BOARD3D
    if (display_is_3d(bd->rd) && bd->rd->quickDraw) {
        if (placed)
            RestrictiveEndMouseMove(dest, abs(bd->points[dest]));
        if (placed && hit)
            RestrictiveDrawPiece(bar, abs(bd->points[bar]));
    }
    if (placed && display_is_3d(bd->rd))
        PlaceMovingPieceRotation(bd, bd->bd3d, dest, source);
#endif
    return placed;
}

/* jsc: Special version :( of board_point which also allows clicking on a
 * small border and all bearoff trays */
static int
board_point_with_border(GtkWidget * UNUSED(board), BoardData * bd, int x0, int y0)
{
    int i, x, y, cx, cy, xCube, yCube;

    x0 /= bd->rd->nSize;
    y0 /= bd->rd->nSize;

    /* Similar to board_point, but adds the nasty y-=3 cy+=3 border
     * allowances */

    if (intersects(x0, y0, 0, 0, bd->x_dice[0], bd->y_dice[0], DIE_WIDTH, DIE_HEIGHT) ||
        intersects(x0, y0, 0, 0, bd->x_dice[1], bd->y_dice[1], DIE_WIDTH, DIE_HEIGHT))
        return POINT_DICE;

    CubePosition(bd->crawford_game, bd->cube_use, bd->doubled, bd->cube_owner, fClockwise, &xCube, &yCube, NULL);
    if (intersects(x0, y0, 0, 0, xCube, yCube, CUBE_WIDTH, CUBE_HEIGHT))
        return POINT_CUBE;

    if (intersects(x0, y0, 0, 0,
                   (BOARD_WIDTH + BAR_WIDTH) / 2, BORDER_HEIGHT + 5 * CHEQUER_HEIGHT,
                   6 * CHEQUER_WIDTH, BOARD_HEIGHT - 10 * CHEQUER_HEIGHT - BORDER_HEIGHT * 3))
        return POINT_RIGHT;
    else if (intersects(x0, y0, 0, 0,
                        BEAROFF_WIDTH, BORDER_HEIGHT + 5 * CHEQUER_HEIGHT,
                        6 * CHEQUER_WIDTH, BOARD_HEIGHT - 10 * CHEQUER_HEIGHT - BORDER_HEIGHT * 3))
        return POINT_LEFT;

    for (i = 0; i < 30; i++) {
        point_area(bd, i, &x, &y, &cx, &cy);

        x /= bd->rd->nSize;
        y /= bd->rd->nSize;
        cx /= bd->rd->nSize;
        cy /= bd->rd->nSize;

        /* Adjusted bear-off y slightly (i == 0 and 25) */
        if (y < 6 * CHEQUER_HEIGHT) {
            if (i > 0)
                y -= (BORDER_HEIGHT);
        }
        if (i == 25)
            y -= (BORDER_HEIGHT);

        cy += BORDER_HEIGHT;

        if (intersects(x0, y0, 0, 0, x, y, cx, cy))
            return i;
    }

    resign_position(bd, &xCube, &yCube, NULL);
    if (intersects(x0, y0, 0, 0, xCube, yCube, CUBE_WIDTH, CUBE_HEIGHT))
        return POINT_RESIGN;

    /* Could not find an overlapping point */
    return -1;
}

/* jsc: Given (x0,y0) which is ASSUMED to intersect point, return a
 * non-negative integer i representing the ith chequer position which
 * intersects (x0, y0).  On failure, return -1 */
static int
board_chequer_number(GtkWidget * UNUSED(board), BoardData * bd, int point, int x0, int y0)
{
    int i, y, cx, cy, dy, c_chequer;

    if (point < 0 || point > 27)
        return -1;

    if (bd->rd->nSize == 0)
        return -1;

    x0 /= bd->rd->nSize;
    y0 /= bd->rd->nSize;

    c_chequer = (!point || point == 25) ? 3 : 5;

    cx = CHEQUER_WIDTH;

    y = positions[fClockwise][point][1];
    dy = -positions[fClockwise][point][2];

    if (dy < 0)
        cy = -dy;
    else
        cy = dy;

    /* FIXME: test for border overlap before for() loop to see if we
     * should return 0, and return -1 if the for() loop completes */

    for (i = 1; i <= c_chequer; ++i) {
        /* We use cx+1 and cy+1 because intersects may return 0 at boundary */
        if (intersects(x0, y0, 0, 0, positions[fClockwise][point][0], y, cx + 1, cy + 1))
            return i;

        y += dy;
    }

    /* Didn't intersect any position on the point */
    return 0;
}

#if USE_BOARD3D
static void
updateBoard(GtkWidget * board, BoardData * bd)
#else
static void
updateBoard(GtkWidget * UNUSED(board), BoardData * bd)
#endif
{
    TanBoard points;
    read_board(bd, points);
    update_gnubg_id(bd, (ConstTanBoard) points);
    update_pipcount(bd, (ConstTanBoard) points);

#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {
        gtk_widget_queue_draw(board);
        updatePieceOccPos(bd, bd->bd3d);
    }
#endif
}

/* Snowie style editing: we will try to place i chequers of the
 * indicated colour on point n.  The x,y coordinates will be used to
 * map to a point n and a checker position i.
 * 
 * Clicking on a point occupied by the opposite color clears the point
 * first.  If not enough chequers are available in the bearoff tray,
 * we try to add what we can.  So if there are no chequers in the
 * bearoff tray, no chequers will be added.  This may be a point of
 * confusion during usage.  Clicking on the outside border of a point
 * corresponds to i=0, i.e. remove all chequers from that point. */
static void
board_quick_edit(GtkWidget * board, BoardData * bd, int x, int y, int dragging)
{
    int current, delta, c_chequer;
    int off, opponent_off;
    int opponent_bar;
    int n, i;

    int colour = bd->drag_button == 1 ? 1 : -1;

#if USE_BOARD3D
    if (display_is_3d(bd->rd))
        n = BoardPoint3d(bd, x, y);
    else
#endif
        /* Map (x,y) to a point from 0..27 using a version of
         * board_point() that ignores the dice and cube and which allows
         * clicking on a narrow border */
        n = board_point_with_border(board, bd, x, y);

    if (!dragging && (n == POINT_UNUSED0 || n == POINT_UNUSED1 || n == 26 || n == 27)) {
        if (n == 26 || n == 27) {       /* click on bearoff tray in edit mode -- bear off all chequers */
            for (i = 0; i < 26; i++) {
                bd->points[i] = 0;
            }
            bd->points[26] = bd->nchequers;
            bd->points[27] = -bd->nchequers;
        } else {                /* if (n == POINT_UNUSED0 || n == POINT_UNUSED1) */
            /* click on unused bearoff tray in edit mode -- reset to starting position */
            TanBoard anBoard;
            InitBoard(anBoard, ms.bgv);
            write_board(bd, anBoard);
        }
#if USE_BOARD3D
        if (display_is_3d(bd->rd))
            RestrictiveRedraw();
        else
#endif
            for (i = 0; i < 28; i++)
                board_invalidate_point(bd, i);

        updateBoard(board, bd);
    }

    /* Only points or bar allowed */
    if (n < 0 || n > 25)
        return;

    /* Make sure that if we drag across a bar, we started on that bar.
     * This is to make sure that if you drag a prime across say point
     * 4 to point 9, you don't inadvertently add chequers to the bar */
    if (dragging && (n == 0 || n == 25) && n != bd->qedit_point)
        return;

    bd->qedit_point = n;

    off = (colour == 1) ? 26 : 27;
    opponent_off = (colour == 1) ? 27 : 26;

    opponent_bar = (colour == 1) ? 0 : 25;

    /* Can't add checkers to the wrong bar */
    if (n == opponent_bar)
        return;

    c_chequer = (n == 0 || n == 25) ? 3 : 5;

    current = abs(bd->points[n]);

    /* Given n, map (x, y) to the ith checker position on point n */
#if USE_BOARD3D
    if (display_is_3d(bd->rd))
        i = BoardSubPoint3d(bd, x, y, n);
    else
#endif
        i = board_chequer_number(board, bd, n, x, y);
    if (i < 0)
        return;

    /* We are at the maximum position in the point, so we should not
     * respond to dragging.  Each click will try to add one more
     * chequer */
    if (!dragging && i >= c_chequer && current >= c_chequer)
        i = current + 1;

    /* Clear chequers of the other colour from this point */
    if (current && ((SGN(bd->points[n])) != colour)) {
        bd->points[opponent_off] += current * -colour;
        bd->points[n] = 0;
        current = 0;

#if USE_BOARD3D
        if (display_is_3d(bd->rd)) {
            gtk_widget_queue_draw(board);
            updatePieceOccPos(bd, bd->bd3d);
        } else
#endif
        {
            board_invalidate_point(bd, n);
            board_invalidate_point(bd, opponent_off);
        }
    }

    delta = i - current;

    /* No chequers of our colour added or removed */
    if (delta == 0)
        return;

    if (delta < 0) {            /* Need to remove some chequers of the same colour */
        bd->points[off] -= delta * colour;
        bd->points[n] += delta * colour;
    } else {
        int mv, avail = abs(bd->points[off]);
        /* any free chequers? */
        if (!avail)
            return;
        /* move up to delta chequers, from avail chequers to point n */
        mv = (avail > delta) ? delta : avail;

        bd->points[off] -= mv * colour;
        bd->points[n] += mv * colour;
    }

#if USE_BOARD3D
    if (display_is_3d(bd->rd))
        RestrictiveRedraw();
    else
#endif
    {
        board_invalidate_point(bd, n);
        board_invalidate_point(bd, off);
    }

    updateBoard(board, bd);
}

static int
ForcedMove(TanBoard anBoard, unsigned int anDice[2])
{

    movelist ml;

    GenerateMoves(&ml, (ConstTanBoard) anBoard, anDice[0], anDice[1], FALSE);

    if (ml.cMoves == 1) {

        ApplyMove(anBoard, ml.amMoves[0].anMove, TRUE);
        return TRUE;

    } else
        return FALSE;

}

static int
GreadyBearoff(TanBoard anBoard, unsigned int anDice[2])
{

    movelist ml;
    unsigned int i, iMove, cMoves;

    /* check for all chequers inside home quadrant */

    for (i = 6; i < 25; ++i)
        if (anBoard[1][i])
            return FALSE;

    cMoves = (anDice[0] == anDice[1]) ? 4 : 2;

    GenerateMoves(&ml, (ConstTanBoard) anBoard, anDice[0], anDice[1], FALSE);

    for (i = 0; i < ml.cMoves; i++)
        for (iMove = 0; iMove < cMoves; iMove++)
            if ((ml.amMoves[i].anMove[iMove << 1] < 0) || (ml.amMoves[i].anMove[(iMove << 1) + 1] != -1))
                /* not a bearoff move */
                break;
            else if (iMove == cMoves - 1) {
                /* All dice bear off */
                ApplyMove(anBoard, ml.amMoves[i].anMove, TRUE);
                return TRUE;
            }

    return FALSE;

}

#if USE_BOARD3D
static void
RestrictiveDrawTargetHelp(BoardData * bd)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (bd->iTargetHelpPoints[i] != -1)
            RestrictiveDrawPiece(bd->iTargetHelpPoints[i], abs(bd->points[bd->iTargetHelpPoints[i]]) + 1);
    }
}
#endif

extern int
UpdateMove(BoardData * bd, TanBoard anBoard)
{

    int old_points[28];
    int i, j;
    int an[28];
    int rc;

    memcpy(old_points, bd->points, sizeof old_points);

    write_board(bd, anBoard);

    for (i = 0, j = 0; i < 28; ++i)
        if (old_points[i] != bd->points[i])
            an[j++] = i;

    if ((rc = update_move(bd)))
        /* illegal move */
        memcpy(bd->points, old_points, sizeof old_points);

    /* Show move */
#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {
        if (bd->rd->quickDraw) {
            int k;
            for (i = 0; i < j; ++i) {
                int min = MIN(abs(old_points[an[i]]), abs(bd->points[an[i]]));
                int max = MAX(abs(old_points[an[i]]), abs(bd->points[an[i]]));
                min = MIN(min, max - 1);        /* huffed - no change in number of chequers */
                for (k = min + 1; k <= max; k++)
                    RestrictiveDrawPiece(an[i], k);
            }
        }
        updatePieceOccPos(bd, bd->bd3d);
        DrawScene3d(bd->bd3d);
    } else
#endif
    {
        for (i = 0; i < j; ++i)
            board_invalidate_point(bd, an[i]);
    }

    return rc;

}

static void
ShowBoardPopup(GdkEventButton * event)
{
    static GtkWidget *boardMenu = NULL;
    if (!boardMenu) {
        GtkWidget *menu_item;
        boardMenu = gtk_menu_new();

        menu_item = gtk_menu_item_new_with_label("Undo Move");
        gtk_menu_shell_append(GTK_MENU_SHELL(boardMenu), menu_item);
        gtk_widget_show(menu_item);
        g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(GTKUndo), NULL);

        menu_item = gtk_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(boardMenu), menu_item);
        gtk_widget_show(menu_item);

        menu_item = gtk_menu_item_new_with_label("Score Sheet");
        gtk_menu_shell_append(GTK_MENU_SHELL(boardMenu), menu_item);
        gtk_widget_show(menu_item);
        g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(GTKShowScoreSheet), NULL);
    }
    gtk_menu_popup(GTK_MENU(boardMenu), NULL, NULL, NULL, NULL, event->button, event->time);
}

extern gboolean
board_button_press(GtkWidget * board, GdkEventButton * event, BoardData * bd)
{
    int x = (int) event->x;
    int y = (int) event->y;
    int editing = ToolbarIsEditing(pwToolbar);
    int numOnPoint;

    /* Ignore double-clicks and multiple presses */
    if (event->type != GDK_BUTTON_PRESS || bd->drag_point >= 0)
        return TRUE;

    if (!bd->playing) {
        if (quick_roll() == 0)
            board_beep(bd);
        return TRUE;
    }
#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {        /* Ignore clicks when animating */
        GtkAllocation allocation;

        if (Animating3d(bd->bd3d))
            return TRUE;

        gtk_widget_get_allocation(board, &allocation);
        /* Reverse screen y coords for openGL */
        y = allocation.height - y;
        bd->drag_point = BoardPoint3d(bd, x, y);
    } else
#endif
    if (editing && !(event->state & GDK_CONTROL_MASK))
        bd->drag_point = board_point_with_border(board, bd, x, y);
    else
        bd->drag_point = board_point(board, bd, x, y);

    bd->click_time = gdk_event_get_time((GdkEvent *) event);
    bd->drag_button = event->button;
    bd->DragTargetHelp = 0;

    /* Set dice in editing mode */
    if (editing && (bd->drag_point == POINT_DICE || bd->drag_point == POINT_LEFT || bd->drag_point == POINT_RIGHT)) {
        if (bd->drag_point == POINT_LEFT && bd->turn != -1)
            UserCommand("set turn 0");
        else if (bd->drag_point == POINT_RIGHT && bd->turn != 1)
            UserCommand("set turn 1");

        /* Avoid dragging after selection causing pieces to appear */
        bd->drag_point = -2;
        GTKSetDice(NULL, 0, NULL);
        return TRUE;
    }

    switch (bd->drag_point) {
    case -1:
        if (event->button == 3) {       /* Show context menu when right click on nothing */
            ShowBoardPopup(event);
            return TRUE;
        }
        /* Click on illegal area. */
        board_beep(bd);
        return TRUE;

    case POINT_CUBE:
        /* Clicked on cube; double. */
        bd->drag_point = -1;

        if (editing) {
            GTKSetCube(NULL, 0, NULL);
            /* Avoid dragging after selection causing pieces to appear */
            bd->drag_point = -2;
        } else if (bd->doubled) {
            switch (event->button) {
            case 1:
                /* left */
                UserCommand("take");
                break;
            case 2:
                /* center */
                if (!bd->match_to)
                    UserCommand("redouble");
                else
                    UserCommand("take");
                break;
            case 3:
            default:
                /* right */
                UserCommand("drop");
                break;

            }
        } else
            UserCommand("double");

        return TRUE;

    case POINT_RESIGN:
#if USE_BOARD3D
        if (display_is_3d(bd->rd)) {
            StopIdle3d(bd, bd->bd3d);
            /* clicked on resignation symbol */
            updateFlagOccPos(bd, bd->bd3d);
        }
#endif

        bd->drag_point = -1;
        if (bd->resigned && !editing)
            UserCommand(event->button == 1 ? "accept" : "reject");

        return TRUE;

    case POINT_DICE:
#if USE_BOARD3D
        if (display_is_3d(bd->rd) && bd->diceShown == DICE_BELOW_BOARD) {       /* Click on dice (below board) - shake */
            bd->drag_point = -1;
            if (quick_roll() != 0)
                board_beep(bd);
            return TRUE;
        }
#endif
        bd->drag_point = -1;
        if (event->button == 1) {
            if (ms.gs == GAME_RESIGNED)
                UserCommand("new game");
            else
                /* Clicked on dice; end move. */
                Confirm(bd);
        } else {                /* Other buttons on dice swaps positions. */
            swap_us(bd->diceRoll, bd->diceRoll + 1);

            /* Display swapped dice */
#if USE_BOARD3D
            if (display_is_3d(bd->rd))
                gtk_widget_queue_draw(board);
            else
#endif
                board_invalidate_dice(bd);
        }
        return TRUE;

    case POINT_LEFT:
    case POINT_RIGHT:
        /* If playing and dice not rolled yet, this code handles
         * rolling the dice if bottom player clicks the right side of
         * the board, or the top player clicks the left side of the
         * board (his/her right side). */
        if (bd->diceShown == DICE_BELOW_BOARD) {
            /* NB: the UserCommand() call may cause reentrancies,
             * so it is vital to reset bd->drag_point first! */
            bd->drag_point = -1;
            quick_roll();
            return TRUE;
        }
        bd->drag_point = -1;
        return TRUE;

    default:                   /* Points */
        /* Don't let them move chequers unless the dice have been
         * rolled, or they're editing the board. */
        if (bd->diceShown != DICE_ON_BOARD && !editing) {
            outputl("You must roll the dice before moving pieces");
            outputx();
            board_beep(bd);
            bd->drag_point = -1;

            return TRUE;
        }

        if (ap[ms.fTurn].pt != PLAYER_HUMAN && !editing) {
            /* calling CommandPlay here may lead to crashes if
             * the click is in the small time frame between end
             * of player turn and next computer turn */
            outputl(_("It is the computer's turn -- type `play' to force it to " "move immediately."));
            outputx();

            board_beep(bd);
            bd->drag_point = -1;

            return TRUE;
        }

        if (editing && !(event->state & GDK_CONTROL_MASK)) {
            board_quick_edit(board, bd, x, y, 0);
            bd->drag_point = -1;
            return TRUE;
        }

        if ((bd->drag_point == POINT_UNUSED0) || (bd->drag_point == POINT_UNUSED1)) {   /* Clicked in un-used bear-off tray */
            bd->drag_point = -1;
            return TRUE;
        }

        /* if nDragPoint is 26 or 27 (i.e. off), bear off as many chequers as possible. */
        if (!editing && bd->drag_point == (53 - bd->turn) / 2) {
            /* user clicked on bear-off tray: try to bear-off chequers or
             * show forced move */
            TanBoard anBoard;

            memcpy(anBoard, msBoard(), sizeof(TanBoard));

            bd->drag_colour = bd->turn;
            bd->drag_point = -1;

            if (ForcedMove(anBoard, bd->diceRoll) || GreadyBearoff(anBoard, bd->diceRoll)) {
                int old_points[28];
                memcpy(old_points, bd->points, sizeof old_points);

                /* we've found a move: update board  */
                if (UpdateMove(bd, anBoard)) {
                    /* should not happen as ForcedMove and GreadyBearoff
                     * always return legal moves */
                    g_assert_not_reached();
                }
                /* Play a sound if any chequers have moved */
                if (memcmp(old_points, bd->points, sizeof old_points))
                    playSound(SOUND_CHEQUER);
            }
            return TRUE;
        }

        /* How many chequers on clicked point */
        numOnPoint = bd->points[bd->drag_point];

        /* Click on an empty point or opponent blot; try to make the point. */
        if (!editing && bd->drag_point <= 24 && (numOnPoint == 0 || numOnPoint == -bd->turn)) {
            int n[2], bar, i;
            int old_points[28];
            TanBoard points;
            positionkey key;

            memcpy(old_points, bd->points, sizeof old_points);

            bd->drag_colour = bd->turn;
            bar = bd->drag_colour == bd->colour ? 25 - bd->bar : bd->bar;

            if (bd->diceRoll[0] == bd->diceRoll[1]) {
                /* Rolled a double; find the two closest chequers to make
                 * the point. */
                int c = 0;

                n[0] = n[1] = -1;

                for (i = 0; i < 4 && c < 2; i++) {
                    int j = bd->drag_point + bd->diceRoll[0] * bd->drag_colour * (i + 1);

                    if (j < 0 || j > 25)
                        break;

                    while (c < 2 && bd->points[j] * bd->drag_colour > 0) {
                        /* temporarily take chequer, so it's not used again */
                        bd->points[j] -= bd->drag_colour;
                        n[c++] = j;
                    }
                }

                /* replace chequers removed above */
                for (i = 0; i < c; i++)
                    bd->points[n[i]] += bd->drag_colour;
            } else {
                /* Rolled a non-double; take one chequer from each point
                 * indicated by the dice. */
                n[0] = bd->drag_point + bd->diceRoll[0] * bd->drag_colour;
                n[1] = bd->drag_point + bd->diceRoll[1] * bd->drag_colour;
            }

            if (n[0] >= 0 && n[0] <= 25 && n[1] >= 0 && n[1] <= 25 &&
                bd->points[n[0]] * bd->drag_colour > 0 && bd->points[n[1]] * bd->drag_colour > 0) {
                /* the point can be made */
                if (bd->points[bd->drag_point])
                    /* hitting the opponent in the process */
                    bd->points[bar] -= bd->drag_colour;

                bd->points[n[0]] -= bd->drag_colour;
                bd->points[n[1]] -= bd->drag_colour;
                bd->points[bd->drag_point] = bd->drag_colour << 1;

                read_board(bd, points);
                PositionKey((ConstTanBoard) points, &key);

                if (!update_move(bd)) { /* Show Move */
#if USE_BOARD3D
                    if (display_is_3d(bd->rd)) {
                        if (bd->rd->quickDraw) {        /* Redraw 2 start positions, end position and perhaps bar */
                            RestrictiveDrawPiece(n[0], abs(bd->points[n[0]]) + 1);
                            if (n[0] == n[1])
                                RestrictiveDrawPiece(n[0], abs(bd->points[n[0]]) + 2);
                            else
                                RestrictiveDrawPiece(n[1], abs(bd->points[n[1]]) + 1);

                            RestrictiveDrawPiece(bd->drag_point, abs(bd->points[bd->drag_point]));
                            RestrictiveDrawPiece(bd->drag_point, abs(bd->points[bd->drag_point]) - 1);

                            if (old_points[bd->drag_point])
                                RestrictiveDrawPiece(bar, abs(bd->points[bar]));
                        }
                        updatePieceOccPos(bd, bd->bd3d);
                        gtk_widget_queue_draw(board);
                    } else
#endif
                    {
                        board_invalidate_point(bd, n[0]);
                        board_invalidate_point(bd, n[1]);
                        board_invalidate_point(bd, bd->drag_point);
                        board_invalidate_point(bd, bar);
                    }

                    playSound(SOUND_CHEQUER);
                } else {
                    /* the move to make the point wasn't legal; undo it. */
                    memcpy(bd->points, old_points, sizeof bd->points);
                    update_move(bd);
                    board_beep(bd);
                }
            } else
                board_beep(bd);

            bd->drag_point = -1;
            return TRUE;
        }

        if (numOnPoint == 0) {  /* clicked on empty point */
            board_beep(bd);
            bd->drag_point = -1;
            return TRUE;
        }

        bd->drag_colour = numOnPoint < 0 ? -1 : 1;

        /* trying to move opponent's chequer  */
        if (!editing && bd->drag_colour != bd->turn) {
            board_beep(bd);
            bd->drag_point = -1;
            return TRUE;
        }

        /* Start Dragging piece */
        board_start_drag(board, bd, bd->drag_point, x, y);
        board_drag(board, bd, x, y);

        return TRUE;
    }
    return FALSE;
}

/*! \brief callback for release of mouse button
 * \param board the GtkBoard
 * \param event the event containing the xy pos
 * \param bd board data
 */
extern gboolean
board_button_release(GtkWidget * board, GdkEventButton * event, BoardData * bd)
{
    int x = (int) event->x;
    int y = (int) event->y;
    int editing = ToolbarIsEditing(pwToolbar);
    int release_point;
    int drag_time;
    int legal_point = -1;
    int i, j;

    if (bd->drag_point < 0)
        return TRUE;

#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {        /* Reverse screen y coords for OpenGL */
        GtkAllocation allocation;
        gtk_widget_get_allocation(board, &allocation);

        y = allocation.height - y;
        release_point = BoardPoint3d(bd, x, y);
    } else
#endif
    {
        board_end_drag(board, bd);
        release_point = board_point(board, bd, x, y);
    }

    drag_time = gdk_event_get_time((GdkEvent *) event) - bd->click_time;
    if (!editing && release_point == bd->drag_point && drag_time < CLICK_TIME) {
        int dests[2] = { -1, -1 };
        if (bd->drag_colour == bd->turn) {
            /* Button 1 tries the left roll first.
             * other buttons try the right dice roll first */
            if (bd->drag_button == 1) {
                dests[0] = bd->drag_point - bd->diceRoll[0] * bd->drag_colour;
                dests[1] = bd->drag_point - bd->diceRoll[1] * bd->drag_colour;
            } else {
                dests[1] = bd->drag_point - bd->diceRoll[0] * bd->drag_colour;
                dests[0] = bd->drag_point - bd->diceRoll[1] * bd->drag_colour;
            }
        }
        legal_dest_points(bd, bd->iTargetHelpPoints);

        for (i = 0; i < 2; ++i) {
            int dest = dests[i];
            if ((dest <= 0) || (dest >= 25))
                /* bearing off */
                dest = bd->drag_colour > 0 ? 26 : 27;

            for (j = 0; j < 4; j++) {
                if (bd->iTargetHelpPoints[j] < 0)
                    continue;
                if (dest == bd->iTargetHelpPoints[j]) {
                    legal_point = dest;
                    goto move_chequer;
                }
            }
        }
    }
    /* if we haven't found a legal move yet we parse the release point
     * to place_chequer_or revert. That is either an illegal move or a
     * drag n' drop */
    if (legal_point < 0)
        legal_point = release_point;

  move_chequer:
    if (place_chequer_or_revert(bd, legal_point))
        playSound(SOUND_CHEQUER);
    else {
        board_invalidate_point(bd, release_point);
        board_beep(bd);
#if USE_BOARD3D
        if (display_is_3d(bd->rd) && bd->rd->quickDraw)
            RestrictiveEndMouseMove(bd->drag_point, abs(bd->points[bd->drag_point]));
#endif
    }

#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {
        /* Update 3d Display */
        updatePieceOccPos(bd, bd->bd3d);
        gtk_widget_queue_draw(board);

        /* undo drag target help if quick drawing */
        if (bd->rd->quickDraw && fGUIDragTargetHelp && bd->DragTargetHelp)
            RestrictiveDrawTargetHelp(bd);
    } else
#endif
    {
        /* undo drag target help */
        if (fGUIDragTargetHelp && bd->DragTargetHelp) {
            int i;
            for (i = 0; i <= 3; ++i) {
                if (bd->iTargetHelpPoints[i] != -1)
                    board_invalidate_point(bd, bd->iTargetHelpPoints[i]);
            }
        }
    }
    bd->DragTargetHelp = 0;
    bd->drag_point = -1;

    return TRUE;
}

extern gboolean
board_motion_notify(GtkWidget * board, GdkEventMotion * event, BoardData * bd)
{
    int x = (int) event->x;
    int y = (int) event->y;
    int editing = ToolbarIsEditing(pwToolbar);

#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {        /* Reverse screen y coords for openGL */
        GtkAllocation allocation;
        gtk_widget_get_allocation(board, &allocation);

        y = allocation.height - y;
    }
#endif

    /* In quick editing mode, dragging across points */
    if (editing && !(event->state & GDK_CONTROL_MASK) && bd->drag_point == -1) {
        board_quick_edit(board, bd, x, y, 1);
        return TRUE;
    }

    if (bd->drag_point < 0)
        return TRUE;

    board_drag(board, bd, x, y);

    if (fGUIDragTargetHelp && !bd->DragTargetHelp && !editing) {        /* Decide if drag targets should be shown (shown after small pause) */
        if ((ap[bd->drag_colour == -1 ? 0 : 1].pt == PLAYER_HUMAN)      /* not for computer turn */
            &&gdk_event_get_time((GdkEvent *) event) - bd->click_time > HINT_TIME) {
            bd->DragTargetHelp = legal_dest_points(bd, bd->iTargetHelpPoints);

#if USE_BOARD3D
            if (bd->rd->quickDraw && fGUIDragTargetHelp && bd->DragTargetHelp)
                RestrictiveDrawTargetHelp(bd);
#endif
        }
    }
#if USE_BOARD3D
    if (display_is_2d(bd->rd))
#endif
    {
        if (bd->DragTargetHelp) {       /* Display 2d drag target help */
            gint i, ptx, pty, ptcx, ptcy;
            GdkColor *TargetHelpColor;

            TargetHelpColor = (GdkColor *) malloc(sizeof(GdkColor));
            /* values of RGB components within GdkColor are
             * taken from 0 to 65535, not 0 to 255. */
            TargetHelpColor->red = 0 * (65535 / 255);
            TargetHelpColor->green = 255 * (65535 / 255);
            TargetHelpColor->blue = 0 * (65535 / 255);
            TargetHelpColor->pixel = (guint32) (TargetHelpColor->red * 65536 +
                                                TargetHelpColor->green * 256 + TargetHelpColor->blue);
            /* get the closest color available in the colormap if no 24-bit */
            gdk_colormap_alloc_color(gtk_widget_get_colormap(board), TargetHelpColor, TRUE, TRUE);
            gdk_gc_set_foreground(bd->gc_copy, TargetHelpColor);

            /* draw help rectangles around target points */
            for (i = 0; i <= 3; ++i) {
                if (bd->iTargetHelpPoints[i] != -1) {
                    /* calculate region coordinates for point */
                    point_area(bd, bd->iTargetHelpPoints[i], &ptx, &pty, &ptcx, &ptcy);
                    gdk_draw_rectangle(gtk_widget_get_window(board), bd->gc_copy, FALSE, ptx + 1, pty + 1, ptcx - 2,
                                       ptcy - 2);
                }
            }

            free(TargetHelpColor);
        }
    }

    return TRUE;
}

static void UpdateCrawfordToggle(GtkWidget * pw, BoardData * bd);
static void board_set_crawford(GtkWidget * pw, BoardData * bd); /* recursion
                                                                 */
static void board_set_jacoby(GtkWidget * pw, BoardData * bd);
static void match_change_val(GtkWidget * pw, BoardData * bd);

static void
score_changed(GtkAdjustment * adj, BoardData * bd)
{

    gchar buf[32];
    int nMatchLen = (int) gtk_adjustment_get_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(bd->match)));

    if ((bd->match_to != nMatchLen) && (adj == bd->amatch)) {
        /* reset limits for scores if match length is changed */
        gfloat upper = (gfloat) (nMatchLen == 0) ? 32767 : nMatchLen - 1;
        gtk_adjustment_set_upper(bd->ascore0, upper);
        gtk_adjustment_set_upper(bd->ascore1, upper);
    }

    gtk_adjustment_changed(bd->ascore0);
    gtk_adjustment_changed(bd->ascore1);

    if (nMatchLen) {

        if (bd->score_opponent >= nMatchLen)
            sprintf(buf, _("%d (won match)"), bd->score_opponent);
        else
            sprintf(buf, _("%d (%d-away)"), bd->score_opponent, nMatchLen - bd->score_opponent);
        gtk_label_set_text(GTK_LABEL(bd->lscore0), buf);

        if (bd->score >= nMatchLen)
            sprintf(buf, _("%d (won match)"), bd->score);
        else
            sprintf(buf, _("%d (%d-away)"), bd->score, nMatchLen - bd->score);
        gtk_label_set_text(GTK_LABEL(bd->lscore1), buf);

    } else {

        /* money game */

        sprintf(buf, "%d", bd->score_opponent);
        gtk_label_set_text(GTK_LABEL(bd->lscore0), buf);
        sprintf(buf, "%d", bd->score);
        gtk_label_set_text(GTK_LABEL(bd->lscore1), buf);

    }
    UpdateCrawfordToggle(NULL, bd);

}

extern void
RollDice2d(BoardData * bd)
{
    int iAttempt = 0, iPoint, x, y, cx, cy;

    bd->x_dice[0] = RAND % (3 * DIE_WIDTH) + BEAROFF_WIDTH + 1;
    bd->x_dice[1] = RAND % (6 * CHEQUER_WIDTH - 2 - bd->x_dice[0]) + bd->x_dice[0] + DIE_WIDTH + 1;

    if (bd->colour == bd->turn) {
        bd->x_dice[0] += 6 * CHEQUER_WIDTH + BAR_WIDTH;
        bd->x_dice[1] += 6 * CHEQUER_WIDTH + BAR_WIDTH;
    }
#define POINT_GAP (BOARD_HEIGHT  - 10 * CHEQUER_HEIGHT - 2 * BORDER_HEIGHT)
#if (POINT_GAP > DIE_HEIGHT)
    bd->y_dice[0] = RAND % (POINT_GAP - DIE_HEIGHT) + 5 * CHEQUER_HEIGHT + BORDER_HEIGHT;
    bd->y_dice[1] = RAND % (POINT_GAP - DIE_HEIGHT) + 5 * CHEQUER_HEIGHT + BORDER_HEIGHT;
#else
    bd->y_dice[0] = 5 * CHEQUER_HEIGHT + BORDER_HEIGHT;
    bd->y_dice[1] = 5 * CHEQUER_HEIGHT + BORDER_HEIGHT;

#endif

    if (bd->rd->nSize > 0) {
        for (iPoint = 1; iPoint <= 24; iPoint++) {
            if (abs(bd->points[iPoint]) >= 5) {
                point_area(bd, iPoint, &x, &y, &cx, &cy);
                x /= bd->rd->nSize;
                y /= bd->rd->nSize;
                cx /= bd->rd->nSize;
                cy /= bd->rd->nSize;

                if ((intersects(bd->x_dice[0], bd->y_dice[0], DIE_WIDTH, DIE_HEIGHT, x, y, cx, cy) || intersects(bd->x_dice[1], bd->y_dice[1], DIE_WIDTH, DIE_HEIGHT, x, y, cx, cy)) && iAttempt++ < 0x80) {    /* Try placing again */
                    RollDice2d(bd);
                }
            }
        }
    }
}


static void
SetCrawfordToggle(BoardData * bd)
{
    if (bd->crawford && (bd->crawford_game != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bd->crawford)))) {
        /* Block handler to stop warning message in click handler */
        g_signal_handlers_block_by_func(G_OBJECT(bd->crawford), (gpointer)G_CALLBACK(board_set_crawford), bd);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd->crawford), bd->crawford_game);
        g_signal_handlers_unblock_by_func(G_OBJECT(bd->crawford), (gpointer)G_CALLBACK(board_set_crawford), bd);
    }
}

static void
SetJacobyToggle(BoardData * bd)
{
    if (bd->jacoby && (bd->jacoby_flag != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bd->jacoby)))) {
        /* Block handler to stop warning message in click handler */
        g_signal_handlers_block_by_func(G_OBJECT(bd->jacoby), (gpointer)G_CALLBACK(board_set_jacoby), bd);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd->jacoby), bd->jacoby_flag);
        g_signal_handlers_unblock_by_func(G_OBJECT(bd->jacoby), (gpointer)G_CALLBACK(board_set_jacoby), bd);
    }
}


static int
board_text_to_setting(const gchar ** board_text, gint * failed)
{
    if (*failed)
        return 0;

    if (*(*board_text)++ != ':') {
        *failed = 1;
        return 0;
    }
    return (int) strtol(*board_text, (char **) board_text, 10);
}

static gint
board_set(Board * board, const gchar * board_text, const gint resigned, const gint cube_use)
{

    BoardData *bd = board->board_data;
    gchar *dest, buf[32];
    gint i, *pn, **ppn;
    gint old_board[28];
    int old_cube, old_doubled, old_crawford, old_xCube, old_yCube, editing;
    int old_resigned;
    int old_xResign, old_yResign;
    int old_turn;
    int old_diceShown;
    int old_jacoby;
    int redrawNeeded = 0;
    gint failed = 0;

    int *match_settings[3];
    unsigned int old_dice[2];

    match_settings[0] = &bd->match_to;
    match_settings[1] = &bd->score;
    match_settings[2] = &bd->score_opponent;

    old_dice[0] = bd->diceRoll[0];
    old_dice[1] = bd->diceRoll[1];
    old_diceShown = bd->diceShown;
    old_turn = bd->turn;

    editing = bd->playing && ToolbarIsEditing(pwToolbar);

    if (strncmp(board_text, "board:", 6))
        return -1;

    board_text += 6;

    for (dest = bd->name, i = 31; i && *board_text && *board_text != ':'; i--)
        *dest++ = *board_text++;

    *dest = 0;

    if (!board_text)
        return -1;

    board_text++;

    for (dest = bd->name_opponent, i = 31; i && *board_text && *board_text != ':'; i--)
        *dest++ = *board_text++;

    *dest = 0;

    if (!board_text)
        return -1;

    for (i = 3, ppn = match_settings; i--;) {
        **ppn++ = board_text_to_setting(&board_text, &failed);
    }
    if (failed)
        return -1;

    for (i = 0, pn = bd->points; i < 26; i++) {
        old_board[i] = *pn;
        *pn++ = board_text_to_setting(&board_text, &failed);
    }
    if (failed)
        return -1;

    old_board[26] = bd->points[26];
    old_board[27] = bd->points[27];

    old_cube = bd->cube;
    old_doubled = bd->doubled;
    old_crawford = bd->crawford_game;
    old_jacoby = bd->jacoby_flag;
    old_resigned = bd->resigned;

    CubePosition(bd->crawford_game, bd->cube_use, bd->doubled, bd->cube_owner, fClockwise, &old_xCube, &old_yCube,
                 NULL);

    resign_position(bd, &old_xResign, &old_yResign, NULL);
    bd->resigned = resigned;

    bd->turn = board_text_to_setting(&board_text, &failed);
    bd->diceRoll[0] = board_text_to_setting(&board_text, &failed);
    bd->diceRoll[1] = board_text_to_setting(&board_text, &failed);
    board_text_to_setting(&board_text, &failed);
    board_text_to_setting(&board_text, &failed);
    bd->cube = board_text_to_setting(&board_text, &failed);
    bd->can_double = board_text_to_setting(&board_text, &failed);
    bd->opponent_can_double = board_text_to_setting(&board_text, &failed);
    bd->doubled = board_text_to_setting(&board_text, &failed);
    bd->colour = board_text_to_setting(&board_text, &failed);
    bd->direction = board_text_to_setting(&board_text, &failed);
    bd->home = board_text_to_setting(&board_text, &failed);
    bd->bar = board_text_to_setting(&board_text, &failed);
    bd->off = board_text_to_setting(&board_text, &failed);
    bd->off_opponent = board_text_to_setting(&board_text, &failed);
    bd->on_bar = board_text_to_setting(&board_text, &failed);
    bd->on_bar_opponent = board_text_to_setting(&board_text, &failed);
    bd->to_move = board_text_to_setting(&board_text, &failed);
    bd->forced = board_text_to_setting(&board_text, &failed);
    bd->crawford_game = board_text_to_setting(&board_text, &failed);
    bd->redoubles = board_text_to_setting(&board_text, &failed);
    bd->jacoby_flag = ms.fJacoby;
    if (failed)
        return -1;

    if (bd->colour < 0)
        bd->off = -bd->off;
    else
        bd->off_opponent = -bd->off_opponent;

    if (bd->direction < 0) {
        bd->points[26] = bd->off;
        bd->points[27] = bd->off_opponent;
    } else {
        bd->points[26] = bd->off_opponent;
        bd->points[27] = bd->off;
    }

    /* calculate number of chequers */

    bd->nchequers = 0;
    for (i = 0; i < 28; ++i)
        if (bd->points[i] > 0)
            bd->nchequers += bd->points[i];

    if (!editing) {
        gtk_entry_set_text(GTK_ENTRY(bd->name0), bd->name_opponent);
        gtk_entry_set_text(GTK_ENTRY(bd->name1), bd->name);
        gtk_label_set_text(GTK_LABEL(bd->lname0), bd->name_opponent);
        gtk_label_set_text(GTK_LABEL(bd->lname1), bd->name);

        if (bd->match_to) {
            sprintf(buf, "%d", bd->match_to);
            gtk_label_set_text(GTK_LABEL(bd->lmatch), buf);
        } else {
            gtk_label_set_text(GTK_LABEL(bd->lmatch), _("unlimited"));
        }
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(bd->score0), bd->score_opponent);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(bd->score1), bd->score);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(bd->match), bd->match_to);

        score_changed(NULL, bd);
        /*        gtk_adjustment_value_changed ( GTK_ADJUSTMENT ( bd->amatch ) ); */

        if (bd->crawford) {
            gtk_widget_set_sensitive(bd->crawford, FALSE);
        }
        if (bd->jacoby) {
            gtk_widget_set_sensitive(bd->jacoby, FALSE);
        }

        read_board(bd, bd->old_board);
        update_pipcount(bd, (ConstTanBoard) bd->old_board);
    }

    update_gnubg_id(bd, (ConstTanBoard) bd->old_board);
    update_move(bd);

    if (fGUIHighDieFirst && bd->diceRoll[0] < bd->diceRoll[1])
        swap_us(bd->diceRoll, bd->diceRoll + 1);

    if (bd->diceRoll[0] != old_dice[0] ||
        bd->diceRoll[1] != old_dice[1] || (bd->diceRoll[0] > 0 && old_diceShown == DICE_BELOW_BOARD) || editing) {
        redrawNeeded = 1;

        if (bd->x_dice[0] > 0
#if USE_BOARD3D
            && display_is_2d(bd->rd)
#endif
            ) {
            /* dice were visible before; now they're not */
            int ax[2];
            ax[0] = bd->x_dice[0];
            ax[1] = bd->x_dice[1];
            bd->x_dice[0] = bd->x_dice[1] = -DIE_WIDTH - 3;
            if (bd->rd->nSize > 0) {
                board_invalidate_rect(bd->drawing_area,
                                      ax[0] * bd->rd->nSize,
                                      bd->y_dice[0] * bd->rd->nSize,
                                      DIE_WIDTH * bd->rd->nSize, DIE_HEIGHT * bd->rd->nSize, bd);
                board_invalidate_rect(bd->drawing_area, ax[1] * bd->rd->nSize,
                                      bd->y_dice[1] * bd->rd->nSize, DIE_WIDTH * bd->rd->nSize,
                                      DIE_HEIGHT * bd->rd->nSize, bd);
            }
        }

        if (bd->diceRoll[0] <= 0) {     /* Dice not on board */
            bd->x_dice[0] = bd->x_dice[1] = -DIE_WIDTH - 3;

            if ((bd->diceRoll[0] == 0 && old_dice[0] > 0) && (bd->diceRoll[1] == 0 && old_dice[1] > 0)) {
                bd->diceShown = DICE_BELOW_BOARD;
                /* Keep showing shaken values */
                bd->diceRoll[0] = old_dice[0];
                bd->diceRoll[1] = old_dice[1];
            } else
                bd->diceShown = DICE_NOT_SHOWN;
        } else {
#if USE_BOARD3D
            if (display_is_3d(bd->rd)) {
                /* Has been shaken, so roll dice */
                if (bd->diceShown == DICE_ROLLING) {
                    bd->diceShown = DICE_ON_BOARD;
                    RollDice3d(bd, bd->bd3d, bd->rd);
                    redrawNeeded = 0;
                } else {        /* Move dice to new position */
                    setDicePos(bd, bd->bd3d);
                }
            } else
#endif
            {
                RollDice2d(bd);
            }
            bd->diceShown = DICE_ON_BOARD;
        }
#if USE_BOARD3D
        if (display_is_3d(bd->rd) && bd->rd->quickDraw)
            RestrictiveDrawDice(bd);
#endif
    }

    if (bd->diceShown == DICE_ON_BOARD) {
        GenerateMoves(&bd->move_list, (ConstTanBoard) bd->old_board, bd->diceRoll[0], bd->diceRoll[1], TRUE);

        /* bd->move_list contains pointers to static data, so we need to
         * copy the actual moves into private storage. */
        if (bd->all_moves)
            free(bd->all_moves);
        bd->all_moves = malloc(bd->move_list.cMoves * sizeof(move));
        bd->move_list.amMoves = memcpy(bd->all_moves, bd->move_list.amMoves, bd->move_list.cMoves * sizeof(move));
        bd->valid_move = NULL;
    }

    if (bd->rd->nSize == 0)
        return 0;

    if (bd->turn != old_turn) {
#if USE_BOARD3D
        if (display_is_3d(bd->rd)) {
            /* Make sure flag shadow is correct if players are swapped when resigned */
            if (bd->resigned)
                updateFlagOccPos(bd, bd->bd3d);

            if (bd->rd->quickDraw) {
                if (bd->rd->fDynamicLabels)
                    RestrictiveDrawBoardNumbers(bd->bd3d);
                if (bd->rd->showMoveIndicator)
                    RestrictiveDrawMoveIndicator(bd);
            }
        }
#endif
        redrawNeeded = 1;
    }

    if (bd->doubled != old_doubled ||
        bd->cube != old_cube ||
        bd->cube_owner != bd->opponent_can_double - bd->can_double ||
        cube_use != bd->cube_use || bd->crawford_game != old_crawford || bd->jacoby_flag != old_jacoby) {
        int xCube, yCube;
#if USE_BOARD3D
        int old_cube_owner = bd->cube_owner;
#endif
        redrawNeeded = 1;

        bd->cube_owner = bd->opponent_can_double - bd->can_double;
        bd->cube_use = cube_use;

#if USE_BOARD3D
        if (display_is_3d(bd->rd)) {
            SetupViewingVolume3d(bd, bd->bd3d, bd->rd); /* Cube may be out of top of screen */
            if (bd->rd->quickDraw)
                RestrictiveDrawCube(bd, old_doubled, old_cube_owner);
        } else
#endif
        {
            /* erase old cube */
            board_invalidate_rect(bd->drawing_area, old_xCube * bd->rd->nSize,
                                  old_yCube * bd->rd->nSize, 8 * bd->rd->nSize, 8 * bd->rd->nSize, bd);

            CubePosition(bd->crawford_game, bd->cube_use, bd->doubled, bd->cube_owner, fClockwise, &xCube, &yCube,
                         NULL);
            /* draw new cube */
            board_invalidate_rect(bd->drawing_area, xCube * bd->rd->nSize,
                                  yCube * bd->rd->nSize, 8 * bd->rd->nSize, 8 * bd->rd->nSize, bd);
        }
    }

    if (bd->resigned != old_resigned) {
        int xResign, yResign;

        redrawNeeded = 1;
        /* erase old resign */
        board_invalidate_rect(bd->drawing_area,
                              old_xResign * bd->rd->nSize,
                              old_yResign * bd->rd->nSize, 8 * bd->rd->nSize, 8 * bd->rd->nSize, bd);

        resign_position(bd, &xResign, &yResign, NULL);
        /* draw new resign */
        board_invalidate_rect(bd->drawing_area,
                              xResign * bd->rd->nSize,
                              yResign * bd->rd->nSize, 8 * bd->rd->nSize, 8 * bd->rd->nSize, bd);
#if USE_BOARD3D
        if (display_is_3d(bd->rd))
            ShowFlag3d(bd, bd->bd3d, bd->rd);
#endif
    }

    if (fClockwise != bd->rd->fClockwise) {
        /* This is complete overkill, but we need to recalculate the
         * board pixmap if the points are numbered and the direction
         * changes. */
        board_free_pixmaps(bd);
        bd->rd->fClockwise = fClockwise;
        board_create_pixmaps(pwBoard, bd);
        gtk_widget_queue_draw(bd->drawing_area);

        ToolbarSetClockwise(pwToolbar, fClockwise);

        return 0;
    }
#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {
        if (redrawNeeded)
            DrawScene3d(bd->bd3d);
    } else
#endif
    {
        for (i = 0; i < 28; i++)
            if (bd->points[i] != old_board[i]) {
                board_invalidate_point(bd, i);
            }

        if (redrawNeeded) {
            board_invalidate_dice(bd);
            board_invalidate_labels(bd);
            board_invalidate_cube(bd);
            board_invalidate_resign(bd);
            board_invalidate_arrow(bd);
        }
    }

    return 0;
}

unsigned int
convert_point(int i, int player)
{

    if (player)
        return (i < 0) ? 26 : i + 1;
    else
        return (i < 0) ? 27 : 24 - i;
}


static gint
board_blink_timeout(gpointer p)
{
    Board *board = p;
    BoardData *pbd = board->board_data;
    int src, dest, src_cheq = 0, dest_cheq = 0, colour;
    static int blink_move, blink_count;

    if (blink_move >= 8 || animate_move_list[blink_move] < 0 || fInterrupt) {
        blink_move = 0;
        animation_finished = TRUE;
        return FALSE;
    }

    src = convert_point(animate_move_list[blink_move], animate_player);
    dest = convert_point(animate_move_list[blink_move + 1], animate_player);
    colour = animate_player ? 1 : -1;

    if (!(blink_count & 1)) {
        src_cheq = pbd->points[src];
        dest_cheq = pbd->points[dest];

        if (pbd->points[dest] == -colour) {
            pbd->points[dest] = 0;

            if (blink_count == 4) {
                pbd->points[animate_player ? 0 : 25] -= colour;
                board_invalidate_point(pbd, animate_player ? 0 : 25);
            }
        }

        pbd->points[src] -= colour;
        pbd->points[dest] += colour;
    }

    board_invalidate_point(pbd, src);
    board_invalidate_point(pbd, dest);

    if (!(blink_count & 1) && blink_count < 4) {
        pbd->points[src] = src_cheq;
        pbd->points[dest] = dest_cheq;
    }

    if (blink_count++ >= 4) {
        blink_count = 0;
        blink_move += 2;
    }

    gdk_window_process_updates(gtk_widget_get_window(pbd->drawing_area), FALSE);

    return TRUE;
}

static gint
board_slide_timeout(gpointer p)
{
    Board *board = p;
    BoardData *bd = board->board_data;
    int src, dest, colour;
    static int slide_move, slide_phase, x, y, x_mid, x_dest, y_dest, y_lift;

    if (fInterrupt && bd->drag_point >= 0) {
        board_end_drag(bd->drawing_area, bd);
        bd->drag_point = -1;
    }

    if (slide_move >= 8 || animate_move_list[slide_move] < 0 || fInterrupt) {
        slide_move = slide_phase = 0;
        animation_finished = TRUE;
        return FALSE;
    }

    src = convert_point(animate_move_list[slide_move], animate_player);
    dest = convert_point(animate_move_list[slide_move + 1], animate_player);
    colour = animate_player ? 1 : -1;

    switch (slide_phase) {
    case 0:
#define SLIDE_BOTTOM (BOARD_HEIGHT / 2 - 3 * CHEQUER_HEIGHT)
#define SLIDE_MIDDLE (BOARD_HEIGHT / 2)
#define SLIDE_TOP    (SLIDE_MIDDLE + 3 * CHEQUER_HEIGHT)

        /* start slide */
        chequer_position(src, abs(bd->points[src]), &x, &y);
        x += (CHEQUER_WIDTH / 2);
        y += (CHEQUER_HEIGHT / 2);
        if (y < SLIDE_BOTTOM)
            y_lift = SLIDE_BOTTOM;
        else if (y < SLIDE_MIDDLE)
            y_lift = y + CHEQUER_HEIGHT / 2;
        else if (y > SLIDE_TOP)
            y_lift = SLIDE_TOP;
        else
            y_lift = y - CHEQUER_HEIGHT / 2;
        chequer_position(dest, abs(bd->points[dest]) + 1, &x_dest, &y_dest);
        x_dest += CHEQUER_WIDTH / 2;
        y_dest += CHEQUER_HEIGHT / 2;
        x_mid = (x + x_dest) >> 1;
        board_start_drag(bd->drawing_area, bd, src, x * bd->rd->nSize, y * bd->rd->nSize);
        slide_phase++;
        /* fall through */
    case 1:
        /* lift */
        if (y < SLIDE_MIDDLE && y < y_lift) {
            y += 2;
            break;
        } else if (y > SLIDE_MIDDLE && y > y_lift) {
            y -= 2;
            break;
        } else
            slide_phase++;
        /* fall through */
    case 2:
        /* to mid point */
        if (y > SLIDE_MIDDLE)
            y--;
        else if (y < SLIDE_MIDDLE)
            y++;

        if (x > x_mid + 2) {
            x -= CHEQUER_WIDTH / 2;
            break;
        } else if (x < x_mid - 2) {
            x += CHEQUER_WIDTH / 2;
            break;
        } else
            slide_phase++;
        /* fall through */
    case 3:
        /* from mid point */
        if (y > y_dest + 1)
            y -= 2;
        else if (y < y_dest - 1)
            y += 2;

        if (x < x_dest - 2) {
            x += CHEQUER_WIDTH / 2;
            break;
        } else if (x > x_dest + 2) {
            x -= CHEQUER_WIDTH / 2;
            break;
        } else
            slide_phase++;
        /* fall through */
    case 4:
        /* drop */
        if (y > y_dest + 2)
            y -= CHEQUER_HEIGHT / 2;
        else if (y < y_dest - 2)
            y += CHEQUER_HEIGHT / 2;
        else {
            board_end_drag(bd->drawing_area, bd);

            if (bd->points[dest] == -colour) {
                bd->points[dest] = 0;
                bd->points[animate_player ? 0 : 25] -= colour;
                board_invalidate_point(bd, animate_player ? 0 : 25);
            }

            bd->points[dest] += colour;
            board_invalidate_point(bd, dest);
            bd->drag_point = -1;
            slide_phase = 0;
            slide_move += 2;
            gdk_window_process_updates(gtk_widget_get_window(bd->drawing_area), FALSE);
            playSound(SOUND_CHEQUER);

            return TRUE;
        }
        break;

    default:
        g_assert_not_reached();
    }

    board_drag(bd->drawing_area, bd, x * bd->rd->nSize, y * bd->rd->nSize);

    return TRUE;
}

extern void
board_animate(Board * board, int move[8], int player)
{
    if (animGUI == ANIMATE_NONE || ms.fResigned) {
#if USE_BOARD3D
        RestrictiveRedraw();
#endif
        return;
    }

    animate_move_list = move;
    animate_player = player;

    animation_finished = FALSE;

#if USE_BOARD3D
    {
        BoardData *bd = board->board_data;
        if (display_is_3d(bd->rd)) {
            BoardData *bd = board->board_data;
            AnimateMove3d(bd, bd->bd3d);
            return;
        }
    }
#endif
    if (animGUI == ANIMATE_BLINK)
        g_timeout_add(0x300 >> nGUIAnimSpeed, board_blink_timeout, board);
    else                        /* ANIMATE_SLIDE */
        g_timeout_add(0x100 >> nGUIAnimSpeed, board_slide_timeout, board);

    while (!animation_finished) {
        GTKSuspendInput();
        gtk_main_iteration();
        GTKResumeInput();
    }
}

extern void
board_set_playing(Board * board, gboolean f)
{

    BoardData *bd = board->board_data;

    bd->playing = f;
}

static void
update_buttons(BoardData * bd)
{

    toolbarcontrol c;

    c = ToolbarUpdate(pwToolbar, &ms, bd->diceShown, bd->computer_turn, bd->playing);

    if (bd->rd->fDiceArea
#if USE_BOARD3D
        && (display_is_2d(bd->rd))
#endif
        ) {
        if (c == C_ROLLDOUBLE)
            gtk_widget_show_all(bd->dice_area);
        else
            gtk_widget_hide(bd->dice_area);

    }
}

extern gint
game_set(Board * board, TanBoard points, int roll,
         const gchar * name, const gchar * opp_name, gint match,
         gint score, gint opp_score, gint die0, gint die1, gint computer_turn, gint nchequers)
{
    gchar board_str[256];
    BoardData *bd = board->board_data;
    TanBoard old_points;

    /* Treat a reset of the position to old_board as a no-op while
     * in edit mode. */
    if (ToolbarIsEditing(pwToolbar) &&
        bd->playing && EqualBoards((ConstTanBoard) bd->old_board, (ConstTanBoard) points)) {
        read_board(bd, old_points);
        if (bd->turn < 0)
            SwapSides(old_points);
        points = old_points;
    } else
        memcpy(bd->old_board, points, sizeof(bd->old_board));

    FIBSBoard(board_str, points, roll, name, opp_name, match, score,
              opp_score, die0, die1, ms.nCube, ms.fCubeOwner, ms.fDoubled, ms.fTurn, ms.fCrawford, nchequers);

    if (gtk_widget_get_realized(pwMain))
        board_set(board, board_str, ms.fResigned == -1 ? 0 : -bd->turn * ms.fResigned, ms.fCubeUse);

    /* FIXME update names, score, match length */
    if (bd->rd->nSize == 0)
        return 0;

    bd->computer_turn = computer_turn;

    if (die0) {
        if (fGUIHighDieFirst && die0 < die1)
            swap(&die0, &die1);
    }

    update_buttons(bd);

#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {
        UpdateShadows(bd->bd3d);
        DrawScene3d(bd->bd3d);
    } else
#endif
    {
        if (fJustSwappedPlayers) {
            if (ms.anDice[0] > 0)
                RollDice2d(bd);
            gtk_widget_queue_draw(bd->drawing_area);
            fJustSwappedPlayers = FALSE;
        }
    }

    return 0;
}

int showingGray;
void
GrayScaleColC(unsigned char *pCols)
{
    float tmp[3];
    float gs;
    tmp[0] = pCols[0] + 128.0f;
    tmp[1] = pCols[1] + 128.0f;
    tmp[2] = pCols[2] + 128.0f;
    gs = ((tmp[0] + tmp[1] + tmp[2]) / 3.0f) * 2.0f;
    pCols[0] = (unsigned char) ((tmp[0] + gs) / 3.0f - 128.0f);
    pCols[1] = (unsigned char) ((tmp[1] + gs) / 3.0f - 128.0f);
    pCols[2] = (unsigned char) ((tmp[2] + gs) / 3.0f - 128.0f);
}

/* Create all of the size/colour-dependent pixmaps. */
extern void
board_create_pixmaps(GtkWidget * UNUSED(board), BoardData * bd)
{
    unsigned char auch[20 * 20 * 3],
        auchBoard[BOARD_WIDTH * 3 * BOARD_HEIGHT * 3 * 3], auchChequers[2][CHEQUER_WIDTH * 3 * CHEQUER_HEIGHT * 3 * 4];
    unsigned short asRefract[2][CHEQUER_WIDTH * 3 * CHEQUER_HEIGHT * 3];
    int i, nSizeReal;

    unsigned char aanBoardTemp[4][4];
#if USE_BOARD3D
    int j;
    double aarColourTemp[2][4];
    unsigned char aanBoardColourTemp[4];
    double arCubeColourTemp[4];
    double aarDiceColourTemp[2][4];
    double aarDiceDotColourTemp[2][4];

    if (display_is_3d(bd->rd)) {        /* As settings currently separate, copy 3d colours so 2d dialog colours (small chequers, dice etc.) are correct */
        memcpy(aarColourTemp, bd->rd->aarColour, sizeof(bd->rd->aarColour));
        memcpy(aanBoardColourTemp, bd->rd->aanBoardColour[0], sizeof(bd->rd->aanBoardColour[0]));
        memcpy(arCubeColourTemp, bd->rd->arCubeColour, sizeof(bd->rd->arCubeColour));
        memcpy(aarDiceColourTemp, bd->rd->aarDiceColour, sizeof(bd->rd->aarDiceColour));
        memcpy(aarDiceDotColourTemp, bd->rd->aarDiceDotColour, sizeof(bd->rd->aarDiceDotColour));

        for (j = 0; j < 4; j++) {
            bd->rd->aanBoardColour[0][j] = (unsigned char) (bd->rd->BaseMat.ambientColour[j] * 0xff);
            bd->rd->arCubeColour[j] = bd->rd->CubeMat.ambientColour[j];

            for (i = 0; i < 2; i++) {
                bd->rd->aarColour[i][j] = bd->rd->ChequerMat[i].ambientColour[j];
                bd->rd->aarDiceColour[i][j] = bd->rd->DiceMat[i].ambientColour[j];
                bd->rd->aarDiceDotColour[i][j] = bd->rd->DiceDotMat[i].ambientColour[j];
            }
        }
    } else
#endif
    {
        if (bd->grayBoard) {
            showingGray = bd->grayBoard;
            memcpy(aanBoardTemp, bd->rd->aanBoardColour, sizeof(aanBoardTemp));
            for (i = 0; i < 4; i++)
                GrayScaleColC(bd->rd->aanBoardColour[i]);
        }
    }
    RenderImages(bd->rd, &bd->ri);
    nSizeReal = bd->rd->nSize;
    bd->rd->nSize = 3;
    RenderBoard(bd->rd, auchBoard, BOARD_WIDTH * 3);
    RenderChequers(bd->rd, auchChequers[0], auchChequers[1], asRefract[0], asRefract[1], CHEQUER_WIDTH * 3 * 4);
    bd->rd->nSize = nSizeReal;

    for (i = 0; i < 2; i++) {
        CopyArea(auch, 20 * 3, auchBoard + 3 * 3 * BOARD_WIDTH * 3 + 3 * 3 * 3, BOARD_WIDTH * 3 * 3, 10, 10);
        CopyArea(auch + 10 * 3, 20 * 3, auchBoard + 3 * 3 * BOARD_WIDTH * 3 + 3 * 3 * 3, BOARD_WIDTH * 3 * 3, 10, 10);
        CopyArea(auch + 10 * 3 * 20, 20 * 3,
                 auchBoard + 3 * 3 * BOARD_WIDTH * 3 + 3 * 3 * 3, BOARD_WIDTH * 3 * 3, 10, 10);
        CopyArea(auch + 10 * 3 + 10 * 3 * 20, 20 * 3,
                 auchBoard + 3 * 3 * BOARD_WIDTH * 3 + 3 * 3 * 3, BOARD_WIDTH * 3 * 3, 10, 10);

        RefractBlend(auch + 20 * 3 + 3, 20 * 3,
                     auchBoard + 3 * 3 * BOARD_WIDTH * 3 + 3 * 3 * 3,
                     BOARD_WIDTH * 3 * 3,
                     auchChequers[i], CHEQUER_WIDTH * 3 * 4,
                     asRefract[i], CHEQUER_WIDTH * 3, CHEQUER_WIDTH * 3, CHEQUER_HEIGHT * 3);

        gdk_draw_rgb_image(bd->appmKey[i], bd->gc_copy, 0, 0, 20, 20, GDK_RGB_DITHER_MAX, auch, 20 * 3);

    }
#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {        /* Restore 2d colours */
        memcpy(bd->rd->aarColour, aarColourTemp, sizeof(bd->rd->aarColour));
        memcpy(bd->rd->aanBoardColour[0], aanBoardColourTemp, sizeof(aanBoardColourTemp));
        memcpy(bd->rd->arCubeColour, arCubeColourTemp, sizeof(bd->rd->arCubeColour));
        memcpy(bd->rd->aarDiceColour, aarDiceColourTemp, sizeof(bd->rd->aarDiceColour));
        memcpy(bd->rd->aarDiceDotColour, aarDiceDotColourTemp, sizeof(bd->rd->aarDiceDotColour));
    } else
#endif
    {
        if (bd->grayBoard) {
            showingGray = FALSE;
            memcpy(bd->rd->aanBoardColour, aanBoardTemp, sizeof(aanBoardTemp));
        }
    }
}

extern void
board_free_pixmaps(BoardData * bd)
{

    FreeImages(&bd->ri);
}

#if USE_BOARD3D

extern void
DisplayCorrectBoardType(BoardData * bd, BoardData3d * UNUSED(bd3d), renderdata * UNUSED(prd))
{
    if (display_is_3d(bd->rd)) {
        gtk_widget_hide(bd->drawing_area);
        gtk_widget_show(GetDrawingArea3d(bd->bd3d));
        DrawScene3d(bd->bd3d);
        updateHingeOccPos(bd->bd3d, bd->rd->fHinges3d);
    } else {
        if (gtk_gl_init_success)
            gtk_widget_hide(GetDrawingArea3d(bd->bd3d));
        gtk_widget_show(bd->drawing_area);
        gtk_widget_queue_draw(bd->drawing_area);
    }
}
#endif

static void
board_size_allocate(GtkWidget * board, GtkAllocation * allocation)
{

    BoardData *bd = BOARD(board)->board_data;
    guint old_size = bd->rd->nSize, new_size;
    GtkAllocation child_allocation;
    GtkRequisition requisition;

    gtk_widget_set_allocation(board, allocation);

    /* position ID, match ID: just below toolbar */

    if (bd->rd->fShowGameInfo) {
        gtk_widget_get_child_requisition(bd->table, &requisition);
        allocation->height -= requisition.height;
        child_allocation.x = allocation->x;
        child_allocation.y = allocation->y + allocation->height;
        child_allocation.width = allocation->width;
        child_allocation.height = requisition.height;
        gtk_widget_size_allocate(bd->table, &child_allocation);
    }

    /* ensure there is room for the dice area or the move, whichever is
     * bigger */
    if (bd->rd->fDiceArea
#if USE_BOARD3D
        && (display_is_2d(bd->rd))
#endif
        ) {
        new_size = MIN(allocation->width / BOARD_WIDTH, (allocation->height - 2) / (BOARD_HEIGHT + DIE_HEIGHT));        /* FIXME: is 89 correct? */

        /* subtract pixels used */
        allocation->height -= new_size * DIE_HEIGHT + 2;

    } else {
        new_size = MIN(allocation->width / BOARD_WIDTH, (allocation->height - 2) / BOARD_HEIGHT);
    }
    /* If the window manager honours our minimum size this won't happen, but... */
    if (new_size < 1)
        new_size = 1;

    if ((bd->rd->nSize = new_size) != old_size && gtk_widget_get_realized(board)) {
        board_free_pixmaps(bd);
        board_create_pixmaps(board, bd);
    }
#if USE_BOARD3D
    child_allocation.width = allocation->width;
    child_allocation.height = allocation->height;
    child_allocation.x = allocation->x;
    child_allocation.y = allocation->y;
    if (gtk_gl_init_success)
        gtk_widget_size_allocate(GetDrawingArea3d(bd->bd3d), &child_allocation);
#endif

    child_allocation.width = BOARD_WIDTH * bd->rd->nSize;
    child_allocation.x = allocation->x + ((allocation->width - child_allocation.width) >> 1);
    child_allocation.height = BOARD_HEIGHT * bd->rd->nSize;
    child_allocation.y = allocation->y + ((allocation->height - BOARD_HEIGHT * bd->rd->nSize) >> 1);
    gtk_widget_size_allocate(bd->drawing_area, &child_allocation);

    /* allocation for dice area */

    if (bd->rd->fDiceArea
#if USE_BOARD3D
        && (display_is_2d(bd->rd))
#endif
        ) {
        child_allocation.width = (2 * DIE_WIDTH + 1) * bd->rd->nSize;
        child_allocation.x += (BOARD_WIDTH - (2 * DIE_WIDTH + 1)) * bd->rd->nSize;
        child_allocation.height = DIE_HEIGHT * bd->rd->nSize;
        child_allocation.y += BOARD_HEIGHT * bd->rd->nSize + 1;
        gtk_widget_size_allocate(bd->dice_area, &child_allocation);
    }
}

static void
AddChild(GtkWidget * pw, GtkRequisition * pr)
{

    GtkRequisition r;

    gtk_widget_size_request(pw, &r);

    if (r.width > pr->width)
        pr->width = r.width;

    pr->height += r.height;
}

static void
board_size_request(GtkWidget * pw, GtkRequisition * pr)
{

    BoardData *bd;

    if (!pw || !pr)
        return;

    pr->width = pr->height = 0;

    bd = BOARD(pw)->board_data;

    AddChild(bd->table, pr);

    if (bd->rd->fDiceArea)
        pr->height += DIE_HEIGHT;

    if (pr->width < BOARD_WIDTH)
        pr->width = BOARD_WIDTH;

    pr->height += BOARD_HEIGHT + 2;
}

static void
board_realize(GtkWidget * board)
{

    BoardData *bd = BOARD(board)->board_data;

    if (GTK_WIDGET_CLASS(parent_class)->realize)
        GTK_WIDGET_CLASS(parent_class)->realize(board);

    board_create_pixmaps(board, bd);
}

static void
board_show_child(GtkWidget * pwChild, BoardData * pbd)
{
    if (pwChild != pbd->dice_area && !GTK_IS_HBUTTON_BOX(pwChild))
        gtk_widget_show_all(pwChild);

}

/* Show all children except the dice area; that one hides and shows
 * itself. */
static void
board_show_all(GtkWidget * pw)
{

    BoardData *bd = BOARD(pw)->board_data;

    gtk_container_foreach(GTK_CONTAINER(pw), (GtkCallback) board_show_child, bd);
    gtk_widget_show(pw);
}

static void
UpdateCrawfordToggle(GtkWidget * UNUSED(pw), BoardData * bd)
{
    int anScoreNew[2];
    int allowCrawford;

    /* Adjust the crawford toggle box by disabling it when the score & matchlen
     * make crawford impossible */
    int nMatchLen = (int) gtk_adjustment_get_value(GTK_ADJUSTMENT(bd->amatch));
    anScoreNew[0] = (int) gtk_adjustment_get_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(bd->score0)));
    anScoreNew[1] = (int) gtk_adjustment_get_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(bd->score1)));
    allowCrawford = ((nMatchLen > 0) && (anScoreNew[0] != anScoreNew[1]) &&
                     (((nMatchLen - anScoreNew[0]) == 1) ||
                      ((nMatchLen - anScoreNew[1]) == 1)) && (anScoreNew[0] < nMatchLen && anScoreNew[1] < nMatchLen));
    gtk_widget_set_sensitive(bd->crawford, allowCrawford);
    if (!allowCrawford)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd->crawford), FALSE);
}

static void
match_change_val(GtkWidget * UNUSED(pw), BoardData * bd)
{
    int nMatchLen = (int) gtk_adjustment_get_value(GTK_ADJUSTMENT(bd->amatch));
    if (nMatchLen && gtk_widget_get_parent_window(GTK_WIDGET(bd->jacoby))) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd->jacoby), fJacoby);
        gtk_container_remove(GTK_CONTAINER(bd->pwvboxcnt), bd->jacoby);
        gtk_container_add(GTK_CONTAINER(bd->pwvboxcnt), bd->crawford);
        gtk_widget_show(bd->crawford);
    } else if (nMatchLen == 0 && gtk_widget_get_parent_window(GTK_WIDGET(bd->crawford))) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd->crawford), FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd->jacoby), fJacoby);
        gtk_container_remove(GTK_CONTAINER(bd->pwvboxcnt), bd->crawford);
        gtk_container_add(GTK_CONTAINER(bd->pwvboxcnt), bd->jacoby);
        gtk_widget_show(bd->jacoby);
    }
}

static void
board_set_crawford(GtkWidget * UNUSED(pw), BoardData * bd)
{
    /* Don't allow changes unless editing */
    if (!ToolbarIsEditing(pwToolbar)) {
        SetCrawfordToggle(bd);
        outputl(_("You can only change whether this is the Crawford game when editing"));
        outputx();
    }
}

static void
board_set_jacoby(GtkWidget * UNUSED(pw), BoardData * bd)
{
    /* Don't allow changes unless editing */
    if (!ToolbarIsEditing(pwToolbar)) {
        SetJacobyToggle(bd);
        outputl(_("You can only enable Jacoby while editing a position"));
        outputx();
    }
}

extern void
board_edit(BoardData * bd)
{
    int f = ToolbarIsEditing(pwToolbar);
    int changed = FALSE;

    update_move(bd);
    update_buttons(bd);

    if (bd->crawford)
        gtk_widget_set_sensitive(bd->crawford, f);

    if (bd->jacoby)
        gtk_widget_set_sensitive(bd->jacoby, f);

    if (fGUIGrayEdit)
        bd->grayBoard = f;
    else
        bd->grayBoard = FALSE;

#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {
        RerenderBase(bd->bd3d);
        DrawScene3d(bd->bd3d);
    } else
#endif
    {
        board_free_pixmaps(bd);
        board_create_pixmaps(pwBoard, bd);
        gtk_widget_queue_draw(bd->drawing_area);
    }

    if (f) {
        /* Close hint window */
        DestroyPanel(WINDOW_HINT);

        /* Entering edit mode: enable entry fields for names and scores */
        gtk_multiview_set_current(GTK_MULTIVIEW(bd->mname0), bd->name0);
        gtk_multiview_set_current(GTK_MULTIVIEW(bd->mname1), bd->name1);
        gtk_multiview_set_current(GTK_MULTIVIEW(bd->mscore0), bd->score0);
        gtk_multiview_set_current(GTK_MULTIVIEW(bd->mscore1), bd->score1);
        gtk_multiview_set_current(GTK_MULTIVIEW(bd->mmatch), bd->match);
    } else {
        /* Editing complete; set board. */
        TanBoard points;
        int anScoreNew[2], nMatchToNew, crawford, jacoby;
        const char *pch0, *pch1;
        char sz[64];            /* "set board XXXXXXXXXXXXXX" */

        /* We need to query all the widgets before issuing any commands,
         * since those commands have side effects which disturb other
         * widgets. */
        crawford = jacoby = FALSE;
        if (bd->crawford)
            crawford = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bd->crawford));
        if (bd->jacoby)
            jacoby = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bd->jacoby));

        pch0 = gtk_entry_get_text(GTK_ENTRY(bd->name0));
        pch1 = gtk_entry_get_text(GTK_ENTRY(bd->name1));
        anScoreNew[0] = (int) gtk_adjustment_get_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(bd->score0)));
        anScoreNew[1] = (int) gtk_adjustment_get_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(bd->score1)));
        nMatchToNew = (int) gtk_adjustment_get_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(bd->match)));
        read_board(bd, points);

        outputpostpone();

        /* NB: these comparisons are case-sensitive, and do not use
         * CompareNames(), so that the user can modify the case of names. We
         * need to change the names together, because set player 0 name changes
         * the contents of the player 1 gtk_entry and thereby pch1. */
        if (strcmp(pch0, ap[0].szName) || strcmp(pch1, ap[1].szName)) {
            char sz0[64], sz1[64];
            sprintf(sz0, "set player 0 name %s", pch0);
            sprintf(sz1, "set player 1 name %s", pch1);
            UserCommand(sz0);
            UserCommand(sz1);
        }

        if (bd->playing && !EqualBoards(msBoard(), (ConstTanBoard) points)) {
            sprintf(sz, "set board %s", PositionID((ConstTanBoard) points));
            UserCommand(sz);
        }

        if (anScoreNew[0] != ms.anScore[0] || anScoreNew[1] != ms.anScore[1])
            changed = TRUE;

        if (jacoby != bd->jacoby_flag || nMatchToNew != ms.nMatchTo) {
            if (!nMatchToNew)
                bd->crawford_game = crawford = FALSE;
            else
                ms.fJacoby = bd->jacoby_flag = jacoby;
            changed = TRUE;
        }

        if (nMatchToNew != ms.nMatchTo || changed) {
            /* new match length; issue "set matchid ..." command */
            gchar *sz;
            int i;

            if (nMatchToNew)
                for (i = 0; i < 2; ++i)
                    if (anScoreNew[i] >= nMatchToNew)
                        anScoreNew[i] = 0;
            if ((bd->diceRoll[0] > 6) || (bd->diceRoll[1] > 6)) {
                bd->diceRoll[0] = bd->diceRoll[1] = 0;
            }
            sz = g_strdup_printf("set matchid %s",
                                 MatchID(bd->diceRoll,
                                         ms.fTurn,
                                         ms.fResigned,
                                         ms.fDoubled,
                                         ms.fMove, ms.fCubeOwner, crawford, nMatchToNew, anScoreNew, bd->cube,
                                         jacoby, ms.gs));
            UserCommand(sz);
            g_free(sz);
        }

        /* Update if crawford was changed. */
        if (crawford != bd->crawford_game) {
            sprintf(sz, "set crawford %s", crawford ? "on" : "off");
            UserCommand(sz);
            bd->crawford_game = ms.fCrawford = crawford;
        }

        outputresume();

        gtk_multiview_set_current(GTK_MULTIVIEW(bd->mname0), bd->lname0);
        gtk_multiview_set_current(GTK_MULTIVIEW(bd->mname1), bd->lname1);
        gtk_multiview_set_current(GTK_MULTIVIEW(bd->mscore0), bd->lscore0);
        gtk_multiview_set_current(GTK_MULTIVIEW(bd->mscore1), bd->lscore1);
        gtk_multiview_set_current(GTK_MULTIVIEW(bd->mmatch), bd->lmatch);
    }
}

static void
DrawAlphaImage(GdkDrawable * pd, int x, int y, unsigned char *puchSrc, int nStride, int cx, int cy)
{

    unsigned char *puch, *puchDest, *auch = g_alloca(cx * cy * 4);
    int ix, iy;
    GdkPixbuf *ppb;

    puchDest = auch;
    puch = puchSrc;
    nStride -= cx * 4;

    for (iy = 0; iy < cy; iy++) {
        for (ix = 0; ix < cx; ix++) {
            puchDest[0] = (unsigned char) (puch[0] * 0x100 / (0x100 - puch[3]));
            puchDest[1] = (unsigned char) (puch[1] * 0x100 / (0x100 - puch[3]));
            puchDest[2] = (unsigned char) (puch[2] * 0x100 / (0x100 - puch[3]));
            puchDest[3] = 0xFF - puch[3];

            puch += 4;
            puchDest += 4;
        }
        puch += nStride;
    }

    ppb = gdk_pixbuf_new_from_data(auch, GDK_COLORSPACE_RGB, TRUE, 8, cx, cy, cx * 4, NULL, NULL);
    gdk_draw_pixbuf(pd, NULL, ppb, 0, 0, x, y, cx, cy, GDK_RGB_DITHER_MAX, 0, 0);
    g_object_unref(G_OBJECT(ppb));
}

extern void
DrawDie(GdkDrawable * pd, unsigned char *achDice[2], unsigned
        char *achPip[2], const int s, GdkGC * gc, int x, int y, int
        fColour, int n, int alpha)
{

    int ix, iy, afPip[9];

    if (alpha)
        DrawAlphaImage(pd, x, y, achDice[fColour], DIE_WIDTH * s * 4, DIE_WIDTH * s, DIE_HEIGHT * s);
    else
        gdk_draw_rgb_image(pd, gc, x, y, DIE_WIDTH * s, DIE_HEIGHT * s, GDK_RGB_DITHER_MAX, achDice[fColour],
                           DIE_WIDTH * s * 3);

    afPip[0] = afPip[8] = (n == 2) || (n == 3) || (n == 4) || (n == 5) || (n == 6);
    afPip[1] = afPip[7] = 0;
    afPip[2] = afPip[6] = (n == 4) || (n == 5) || (n == 6);
    afPip[3] = afPip[5] = n == 6;
    afPip[4] = n & 1;

    for (iy = 0; iy < 3; iy++)
        for (ix = 0; ix < 3; ix++)
            if (afPip[iy * 3 + ix])
                gdk_draw_rgb_image(pd, gc, (int) (s * 1.5 + x + 1.5 * s * ix),
                                   (int) (s * 1.5 + y + 1.5 * s * iy), s, s,
                                   GDK_RGB_DITHER_MAX, achPip[fColour], s * 3);
}

static gboolean
dice_expose(GtkWidget * dice, GdkEventExpose * UNUSED(event), BoardData * bd)
{

    if (bd->rd->nSize == 0 || bd->diceShown == DICE_NOT_SHOWN)
        return TRUE;

    DrawDie(gtk_widget_get_window(dice), bd->ri.achDice, bd->ri.achPip,
            bd->rd->nSize, bd->gc_copy, 0, 0, bd->turn > 0, bd->diceRoll[0], TRUE);
    DrawDie(gtk_widget_get_window(dice), bd->ri.achDice, bd->ri.achPip,
            bd->rd->nSize, bd->gc_copy, (DIE_WIDTH + 1) * bd->rd->nSize, 0, bd->turn > 0, bd->diceRoll[1], TRUE);

    return TRUE;
}

static gboolean
dice_press(GtkWidget * UNUSED(dice), GdkEvent * UNUSED(event), BoardData * UNUSED(bd))
{
    quick_roll();

    return TRUE;
}

static gboolean
key_press(GtkWidget * UNUSED(pw), GdkEvent * UNUSED(event), void *p)
{
    UserCommand(p ? "set turn 1" : "set turn 0");

    return TRUE;
}

/* Create a widget to display a single chequer (used as a key in the
 * player table). */
static GtkWidget *
chequer_key_new(int iPlayer, Board * board)
{

    GtkWidget *pw = gtk_event_box_new(), *pwImage;
    BoardData *bd = board->board_data;
    GdkPixmap *ppm;
    char sz[128];

    gtk_event_box_set_visible_window(GTK_EVENT_BOX(pw), FALSE);
    ppm = bd->appmKey[iPlayer] =
        gdk_pixmap_new(NULL, 20, 20, gdk_visual_get_depth(gtk_widget_get_visual(GTK_WIDGET(board))));

    pwImage = gtk_image_new_from_pixmap(ppm, NULL);

    gtk_container_add(GTK_CONTAINER(pw), pwImage);

    gtk_widget_add_events(pw, GDK_BUTTON_PRESS_MASK);

    g_signal_connect(G_OBJECT(pw), "button_press_event", G_CALLBACK(key_press), iPlayer ? pw : NULL);

    sprintf(sz, _("Set player %d on roll."), iPlayer);
    gtk_widget_set_tooltip_text(pw, sz);

    return pw;
}

static void
board_init(Board * board)
{

    BoardData *bd = g_malloc(sizeof(*bd));
    GdkColormap *cmap = gtk_widget_get_colormap(GTK_WIDGET(board));
    GdkVisual *vis = gtk_widget_get_visual(GTK_WIDGET(board));
    GdkGCValues gcval;
    GtkWidget *pw;
    GtkWidget *pwFrame;
    GtkWidget *pwvbox;
    GdkColor color;

    board->board_data = bd;
    bd->widget = GTK_WIDGET(board);

    bd->drag_point = -1;

    bd->crawford_game = FALSE;
    bd->jacoby_flag = FALSE;
    bd->playing = FALSE;
    bd->cube_use = TRUE;
    bd->all_moves = NULL;

    gcval.function = GDK_AND;
    gcval.foreground.pixel = (guint32) ~ 0L;    /* AllPlanes */
    gcval.background.pixel = 0;
    bd->gc_and = gtk_gc_get(gdk_visual_get_depth(vis), cmap, &gcval,
                            GDK_GC_FOREGROUND | GDK_GC_BACKGROUND | GDK_GC_FUNCTION);

    gcval.function = GDK_OR;
    bd->gc_or = gtk_gc_get(gdk_visual_get_depth(vis), cmap, &gcval, GDK_GC_FUNCTION);

    bd->gc_copy = gtk_gc_get(gdk_visual_get_depth(vis), cmap, &gcval, 0);

    gdk_color_parse("#000080", &color);
    gdk_colormap_alloc_color(cmap, &color, TRUE, TRUE);
    gcval.foreground.pixel = color.pixel;
    bd->gc_cube = gtk_gc_get(gdk_visual_get_depth(vis), cmap, &gcval, GDK_GC_FOREGROUND);

    bd->x_dice[0] = bd->x_dice[1] = -10;
    bd->diceRoll[0] = bd->diceRoll[1] = 0;

    /* board drawing area */

    bd->drawing_area = gtk_drawing_area_new();
    /* gtk_widget_set_name(GTK_WIDGET(bd->drawing_area), "background"); */
    gtk_widget_set_size_request(bd->drawing_area, BOARD_WIDTH, BOARD_HEIGHT);
    gtk_widget_add_events(GTK_WIDGET(bd->drawing_area), GDK_EXPOSURE_MASK |
                          GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK | GDK_STRUCTURE_MASK);
    gtk_container_add(GTK_CONTAINER(board), bd->drawing_area);

#if USE_BOARD3D
    /* 3d board drawing area */
    gtk_gl_init_success = gtk_gl_init_success ? CreateGLWidget(bd) : FALSE;
    if (gtk_gl_init_success) {
        gtk_container_add(GTK_CONTAINER(board), GetDrawingArea3d(bd->bd3d));
    } else
        bd->bd3d = NULL;
#endif

    /* the board */

    bd->table = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(board), bd->table, FALSE, TRUE, 0);

    /* 
     * player 0 
     */

    pwFrame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(bd->table), pwFrame, TRUE, TRUE, 0);

    pwvbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwvbox);

    /* first row: picture of chequer + name of player */

    pw = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwvbox), pw, FALSE, FALSE, 0);

    /* picture of chequer */

    bd->key0 = chequer_key_new(0, board);
    gtk_box_pack_start(GTK_BOX(pw), bd->key0, FALSE, FALSE, 0);

    /* name of player */

    bd->mname0 = gtk_multiview_new();
    gtk_box_pack_start(GTK_BOX(pw), bd->mname0, FALSE, FALSE, 8);

    bd->name0 = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(bd->name0), MAX_NAME_LEN);
    bd->lname0 = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(bd->lname0), 0, 0.5);
    gtk_container_add(GTK_CONTAINER(bd->mname0), bd->lname0);
    gtk_container_add(GTK_CONTAINER(bd->mname0), bd->name0);

    /* second row: "Score" + score */

    pw = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwvbox), pw, FALSE, FALSE, 0);

    /* score label */

    gtk_box_pack_start(GTK_BOX(pw), gtk_label_new(_("Score:")), FALSE, FALSE, 0);

    /* score */

    bd->mscore0 = gtk_multiview_new();
    gtk_box_pack_start(GTK_BOX(pw), bd->mscore0, FALSE, FALSE, 8);

    bd->ascore0 = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 32767, 1, 1, 0));
    bd->score0 = gtk_spin_button_new(GTK_ADJUSTMENT(bd->ascore0), 1, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(bd->score0), TRUE);
    bd->lscore0 = gtk_label_new(NULL);
    gtk_container_add(GTK_CONTAINER(bd->mscore0), bd->lscore0);
    gtk_container_add(GTK_CONTAINER(bd->mscore0), bd->score0);

    /* third row: pip count and epc */

    pw = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwvbox), pw, FALSE, FALSE, 0);

    /* pip count label */

    gtk_box_pack_start(GTK_BOX(pw), bd->pipcountlabel0 = gtk_label_new(NULL), FALSE, FALSE, 0);

    /* pip count */

    gtk_box_pack_start(GTK_BOX(pw), bd->pipcount0 = gtk_label_new(NULL), FALSE, FALSE, 0);


    /* 
     * player 1
     */

    pwFrame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(bd->table), pwFrame, TRUE, TRUE, 0);

    pwvbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwvbox);

    /* first row: picture of chequer + name of player */

    pw = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwvbox), pw, FALSE, FALSE, 0);

    /* picture of chequer */

    bd->key1 = chequer_key_new(1, board);
    gtk_box_pack_start(GTK_BOX(pw), bd->key1, FALSE, FALSE, 0);

    /* name of player */

    bd->mname1 = gtk_multiview_new();
    gtk_box_pack_start(GTK_BOX(pw), bd->mname1, FALSE, FALSE, 8);

    bd->name1 = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(bd->name1), MAX_NAME_LEN);
    bd->lname1 = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(bd->lname1), 0, 0.5);
    gtk_container_add(GTK_CONTAINER(bd->mname1), bd->lname1);
    gtk_container_add(GTK_CONTAINER(bd->mname1), bd->name1);

    /* second row: "Score" + score */

    pw = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwvbox), pw, FALSE, FALSE, 0);

    /* score label */

    gtk_box_pack_start(GTK_BOX(pw), gtk_label_new(_("Score:")), FALSE, FALSE, 0);

    /* score */

    bd->mscore1 = gtk_multiview_new();
    gtk_box_pack_start(GTK_BOX(pw), bd->mscore1, FALSE, FALSE, 8);

    bd->ascore1 = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 32767, 1, 1, 0));
    bd->score1 = gtk_spin_button_new(GTK_ADJUSTMENT(bd->ascore1), 1, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(bd->score1), TRUE);
    bd->lscore1 = gtk_label_new(NULL);
    gtk_container_add(GTK_CONTAINER(bd->mscore1), bd->lscore1);
    gtk_container_add(GTK_CONTAINER(bd->mscore1), bd->score1);

    /* third row: pip count and epc */

    pw = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwvbox), pw, FALSE, FALSE, 0);

    /* pip count label */

    gtk_box_pack_start(GTK_BOX(pw), bd->pipcountlabel1 = gtk_label_new(NULL), FALSE, FALSE, 0);

    /* pip count */

    gtk_box_pack_start(GTK_BOX(pw), bd->pipcount1 = gtk_label_new(NULL), FALSE, FALSE, 0);



    /* 
     * move string, match length, crawford flag, dice
     */

    pwFrame = gtk_frame_new(NULL);
    gtk_box_pack_end(GTK_BOX(bd->table), pwFrame, FALSE, FALSE, 0);

    pwvbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwvbox);

    /* move string */

    gtk_box_pack_start(GTK_BOX(pwvbox), bd->wmove = gtk_label_new(NULL), FALSE, FALSE, 0);
    gtk_widget_set_name(bd->wmove, "gnubg-move-current");

    /* match length */

    gtk_box_pack_start(GTK_BOX(pwvbox), pw = gtk_hbox_new(FALSE, 0), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pw), gtk_label_new(_("Match:")), FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(pw), bd->mmatch = gtk_multiview_new(), FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(bd->mmatch), bd->lmatch = gtk_label_new(NULL));

    bd->amatch = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, MAXSCORE, 1, 1, 0));
    bd->match = gtk_spin_button_new(GTK_ADJUSTMENT(bd->amatch), 1, 0);

    g_signal_connect(G_OBJECT(bd->match), "value-changed", G_CALLBACK(match_change_val), bd);

    gtk_container_add(GTK_CONTAINER(bd->mmatch), bd->match);

    /* crawford and jacoby flag */

    bd->crawford = gtk_check_button_new_with_label(_("Crawford game"));
    bd->jacoby = gtk_check_button_new_with_label(_("Jacoby"));
    /*    g_signal_connect( G_OBJECT( bd->crawford ), "toggled",
     * G_CALLBACK( board_set_crawford ), bd );
     * g_signal_connect( G_OBJECT( bd->jacoby ), "toggled",
     * G_CALLBACK( board_set_jacoby ), bd ); 
     */
    bd->pwvboxcnt = gtk_event_box_new();
    if (ms.nMatchTo)
        gtk_container_add(GTK_CONTAINER(bd->pwvboxcnt), bd->crawford);
    else
        gtk_container_add(GTK_CONTAINER(bd->pwvboxcnt), bd->jacoby);

    gtk_box_pack_start(GTK_BOX(pwvbox), bd->pwvboxcnt, FALSE, FALSE, 0);

    g_object_ref(bd->jacoby);
    g_object_ref(bd->crawford);
    gtk_adjustment_value_changed(GTK_ADJUSTMENT(bd->amatch));


    /* dice drawing area */

    gtk_container_add(GTK_CONTAINER(board), bd->dice_area = gtk_drawing_area_new());
    /* gtk_widget_set_name( GTK_WIDGET(bd->dice_area), "dice_area"); */
    gtk_widget_set_size_request(bd->dice_area, 2 * DIE_WIDTH + 1, DIE_HEIGHT);
    gtk_widget_add_events(GTK_WIDGET(bd->dice_area), GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_STRUCTURE_MASK);

    g_signal_connect(G_OBJECT(bd->drawing_area), "expose_event", G_CALLBACK(board_expose), bd);
    g_signal_connect(G_OBJECT(bd->drawing_area), "button_press_event", G_CALLBACK(board_button_press), bd);
    g_signal_connect(G_OBJECT(bd->drawing_area), "button_release_event", G_CALLBACK(board_button_release), bd);
    g_signal_connect(G_OBJECT(bd->drawing_area), "motion_notify_event", G_CALLBACK(board_motion_notify), bd);

    g_signal_connect(G_OBJECT(bd->dice_area), "expose_event", G_CALLBACK(dice_expose), bd);
    g_signal_connect(G_OBJECT(bd->dice_area), "button_press_event", G_CALLBACK(dice_press), bd);

    g_signal_connect(G_OBJECT(bd->amatch), "value-changed", G_CALLBACK(score_changed), bd);
    g_signal_connect(G_OBJECT(bd->ascore0), "value-changed", G_CALLBACK(score_changed), bd);
    g_signal_connect(G_OBJECT(bd->ascore1), "value-changed", G_CALLBACK(score_changed), bd);

    UpdateCrawfordToggle(NULL, bd);
}

static void
board_class_init(BoardClass * c)
{

    parent_class = g_type_class_peek_parent(c);
    g_assert(parent_class);

    ((GtkWidgetClass *) c)->size_allocate = board_size_allocate;
    ((GtkWidgetClass *) c)->size_request = board_size_request;
    ((GtkWidgetClass *) c)->realize = board_realize;
    ((GtkWidgetClass *) c)->show_all = board_show_all;
}


#define N_CUBES_IN_WIDGET 8
static gboolean
cube_widget_expose(GtkWidget * cube, GdkEventExpose * UNUSED(event), BoardData * bd)
{

    int n, nValue;
    unsigned char *puch;
    int setSize = bd->rd->nSize;
    int cubeStride = setSize * CUBE_WIDTH * 4;
    int cubeFaceStride = setSize * CUBE_LABEL_WIDTH * 3;

    n = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cube), "user_data"));
    if ((nValue = n % N_CUBES_IN_WIDGET - 1) == -1)
        nValue = 5;             /* use 64 cube for 1 */

    puch = g_alloca(cubeFaceStride * CUBE_LABEL_HEIGHT * setSize);

    CopyAreaRotateClip(puch, cubeFaceStride, 0, 0,
                       CUBE_LABEL_WIDTH * setSize,
                       CUBE_LABEL_HEIGHT * setSize,
                       TTachCubeFaces,
                       cubeFaceStride,
                       0, CUBE_LABEL_HEIGHT * setSize * nValue,
                       CUBE_LABEL_WIDTH * setSize, CUBE_LABEL_HEIGHT * setSize, 2 - n / N_CUBES_IN_WIDGET);
    DrawAlphaImage(gtk_widget_get_window(cube), 0, 0,
                   TTachCube, cubeStride, CUBE_WIDTH * setSize, CUBE_HEIGHT * setSize);
    gdk_draw_rgb_image(gtk_widget_get_window(cube), bd->gc_copy,
                       setSize, setSize,
                       CUBE_LABEL_WIDTH * setSize,
                       CUBE_LABEL_HEIGHT * setSize, GDK_RGB_DITHER_MAX, puch, cubeFaceStride);

    return TRUE;
}

static gboolean
cube_widget_press(GtkWidget * cube, GdkEvent * UNUSED(event), BoardData * UNUSED(bd))
{

    GtkWidget *pwTable = gtk_widget_get_parent(cube);
    int n = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cube), "user_data"));
    int *an = g_object_get_data(G_OBJECT(pwTable), "user_data");

    an[0] = n % N_CUBES_IN_WIDGET;      /* value */
    if (n < N_CUBES_IN_WIDGET)
        an[1] = 0;              /* top player */
    else if (n < N_CUBES_IN_WIDGET * 2)
        an[1] = -1;             /* centred */
    else
        an[1] = 1;              /* bottom player */

    gtk_widget_destroy(pwTable);

    return TRUE;
}

extern void
DestroySetCube(GtkObject * UNUSED(po), GtkWidget * pw)
{
    free(TTachCubeFaces);
    free(TTachCube);
    gtk_widget_destroy(pw);
}

extern GtkWidget *
board_cube_widget(Board * board)
{
    GtkWidget *pw = gtk_table_new(3, N_CUBES_IN_WIDGET, TRUE), *pwCube;
    BoardData *bd = board->board_data;
    int x, y;
    int setSize = bd->rd->nSize;

    int cubeStride = setSize * CUBE_WIDTH * 4;
    int cubeFaceStride = setSize * CUBE_LABEL_WIDTH * 3;
    renderdata rd;
    CopyAppearance(&rd);
    rd.nSize = setSize;
#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {
        for (x = 0; x < 4; x++)
            rd.arCubeColour[x] = bd->rd->CubeMat.ambientColour[x];
    }
#endif
    TTachCube = malloc(cubeStride * setSize * CUBE_HEIGHT);
    TTachCubeFaces = malloc(cubeFaceStride * setSize * CUBE_LABEL_HEIGHT * 13);

    RenderCube(&rd, TTachCube, cubeStride);
    RenderCubeFaces(&rd, TTachCubeFaces, cubeFaceStride, TTachCube, cubeStride);

    for (y = 0; y <= 2; y++) {
        for (x = 0; x <= N_CUBES_IN_WIDGET - 1; x++) {
            pwCube = gtk_drawing_area_new();
            g_object_set_data(G_OBJECT(pwCube), "user_data", GINT_TO_POINTER((y * N_CUBES_IN_WIDGET + x)));
            gtk_widget_set_size_request(pwCube, CUBE_WIDTH * setSize, CUBE_HEIGHT * setSize);
            gtk_widget_add_events(pwCube, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_STRUCTURE_MASK);
            g_signal_connect(G_OBJECT(pwCube), "expose_event", G_CALLBACK(cube_widget_expose), bd);
            g_signal_connect(G_OBJECT(pwCube), "button_press_event", G_CALLBACK(cube_widget_press), bd);
            gtk_table_attach_defaults(GTK_TABLE(pw), pwCube, x, x + 1, y, y + 1);
        }
    }

    gtk_table_set_row_spacings(GTK_TABLE(pw), 4 * setSize);
    gtk_table_set_col_spacings(GTK_TABLE(pw), 2 * setSize);
    gtk_container_set_border_width(GTK_CONTAINER(pw), setSize);

    return pw;
}


static gboolean
setdice_widget_expose(GtkWidget * dice, GdkEventExpose * UNUSED(event), SetDiceData * sdd)
{
    int setSize = sdd->bd->rd->nSize;
    int n = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dice), "user_data"));

    if (sdd->mdt == MT_FIRSTMOVE && (n % 6 == n / 6)) {
        DrawDie(gtk_widget_get_window(dice), &sdd->TTachGrayDice, &sdd->TTachGrayPip, setSize, sdd->bd->gc_copy,
                0, 0, 0, n % 6 + 1, FALSE);
        DrawDie(gtk_widget_get_window(dice), &sdd->TTachGrayDice, &sdd->TTachGrayPip, setSize, sdd->bd->gc_copy,
                DIE_WIDTH * setSize, 0, 0, n / 6 + 1, FALSE);
    } else {
        int col1, col2;
        if (sdd->mdt == MT_STANDARD)
            col1 = col2 = (sdd->bd->turn == 1) ? 1 : 0;
        else if (sdd->mdt == MT_EDIT && (n % 6 == n / 6))
            col1 = 1, col2 = 0;
        else
            col1 = col2 = ((n % 6) <= n / 6);

        DrawDie(gtk_widget_get_window(dice), sdd->TTachDice, sdd->TTachPip, setSize, sdd->bd->gc_copy,
                0, 0, col1, n % 6 + 1, FALSE);
        DrawDie(gtk_widget_get_window(dice), sdd->TTachDice, sdd->TTachPip, setSize, sdd->bd->gc_copy,
                DIE_WIDTH * setSize, 0, col2, n / 6 + 1, FALSE);
    }
    return TRUE;
}

static gboolean
dice_widget_press(GtkWidget * dice, GdkEvent * UNUSED(event), BoardData * UNUSED(bd))
{
    GtkWidget *pwTable = gtk_widget_get_parent(gtk_widget_get_parent(dice));
    int n = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dice), "user_data"));
    int *an = g_object_get_data(G_OBJECT(pwTable), "user_data");

    an[0] = n % 6 + 1;
    an[1] = n / 6 + 1;

    gtk_widget_destroy(gtk_widget_get_toplevel(dice));

    return TRUE;
}

static void
DestroySetDice(GtkWidget * po, void *data)
{
    SetDiceData *sdd = (SetDiceData *) data;
    free(sdd->TTachDice[0]);
    free(sdd->TTachDice[1]);
    free(sdd->TTachPip[0]);
    free(sdd->TTachPip[1]);
    free(sdd->TTachGrayDice);
    free(sdd->TTachGrayPip);
    gtk_widget_destroy(gtk_widget_get_toplevel(po));
}

#if USE_BOARD3D
extern void
Copy3dDiceColour(renderdata * prd)
{
    if (display_is_3d(prd)) {
        int i, j;
        for (j = 0; j < 4; j++) {
            for (i = 0; i < 2; i++) {
                prd->aarColour[i][j] = prd->ChequerMat[i].ambientColour[j];
                prd->aarDiceColour[i][j] = prd->DiceMat[i].ambientColour[j];
                prd->aarDiceDotColour[i][j] = prd->DiceDotMat[i].ambientColour[j];
            }
        }
    }
}
#endif

extern GtkWidget *
board_dice_widget(Board * board, manualDiceType mdt)
{
    GtkWidget *main_table = gtk_table_new(2, 2, FALSE);
    GtkWidget *pw = gtk_table_new(6, 6, TRUE);
    GtkWidget *pwDice;
    GtkWidget *label;
    char *str;
    BoardData *bd = board->board_data;
    int x, y;
    int setSize = bd->rd->nSize;
    int diceStride = setSize * DIE_WIDTH * 4;
    int pipStride = setSize * 3;
    renderdata rd;
    SetDiceData *sdd = (SetDiceData *) malloc(sizeof(SetDiceData));
    sdd->bd = bd;
    sdd->mdt = mdt;

    if (mdt == MT_FIRSTMOVE) {
        str = g_strdup_printf(_("%s wins opening roll"), ap[0].szName);
        label = gtk_label_new(str);
        g_free(str);
    } else {
        int player = 0;
        if (mdt == MT_STANDARD)
            player = (bd->turn == 1) ? 1 : 0;
        str = g_strdup_printf(_("%s to roll"), ap[player].szName);
        label = gtk_label_new(str);
        g_free(str);
    }
    gtk_table_attach_defaults(GTK_TABLE(main_table), label, 1, 2, 0, 1);

    if (mdt != MT_STANDARD) {
        if (mdt == MT_FIRSTMOVE) {
            str = g_strdup_printf(_("%s wins opening roll"), ap[1].szName);
            label = gtk_label_new(str);
            g_free(str);
        } else {
            str = g_strdup_printf(_("%s to roll"), ap[1].szName);
            label = gtk_label_new(str);
            g_free(str);
        }
        gtk_label_set_angle(GTK_LABEL(label), 90);
        gtk_table_attach_defaults(GTK_TABLE(main_table), label, 0, 1, 1, 2);
    }

    gtk_table_attach_defaults(GTK_TABLE(main_table), pw, 1, 2, 1, 2);

    CopyAppearance(&rd);
    rd.nSize = setSize;
#if USE_BOARD3D
    Copy3dDiceColour(&rd);
#endif
    sdd->TTachDice[0] = malloc(diceStride * setSize * DIE_HEIGHT);
    sdd->TTachDice[1] = malloc(diceStride * setSize * DIE_HEIGHT);
    sdd->TTachPip[0] = malloc(pipStride * setSize);
    sdd->TTachPip[1] = malloc(pipStride * setSize);

    RenderDice(&rd, sdd->TTachDice[0], sdd->TTachDice[1], diceStride, FALSE);
    RenderPips(&rd, sdd->TTachPip[0], sdd->TTachPip[1], pipStride);

    if (mdt == MT_FIRSTMOVE) {  /* Gray dice for doubles on first move */
        sdd->TTachGrayDice = malloc(diceStride * setSize * DIE_HEIGHT);
        sdd->TTachGrayPip = malloc(pipStride * setSize);

        rd.aarDiceColour[0][0] = rd.aarDiceColour[0][1] = rd.aarDiceColour[0][2] = .75;
        rd.aarDiceColour[1][0] = rd.aarDiceColour[1][1] = rd.aarDiceColour[1][2] = .75;
        rd.aarDiceDotColour[0][0] = rd.aarDiceDotColour[0][1] = rd.aarDiceDotColour[0][2] = .25;
        rd.aarDiceDotColour[1][0] = rd.aarDiceDotColour[1][1] = rd.aarDiceDotColour[1][2] = .25;
        rd.afDieColour[0] = rd.afDieColour[1] = 0;

        RenderDice(&rd, sdd->TTachGrayDice, sdd->TTachGrayDice, diceStride, FALSE);
        RenderPips(&rd, sdd->TTachGrayPip, sdd->TTachGrayPip, pipStride);
    } else
        sdd->TTachGrayDice = sdd->TTachGrayPip = NULL;

    for (y = 0; y < 6; y++) {
        for (x = 0; x < 6; x++) {
            pwDice = gtk_drawing_area_new();
            g_object_set_data(G_OBJECT(pwDice), "user_data", GINT_TO_POINTER((y * 6 + x)));
            gtk_widget_set_size_request(pwDice, 2 * DIE_WIDTH * setSize, DIE_HEIGHT * setSize);
            gtk_widget_add_events(pwDice, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_STRUCTURE_MASK);
            g_signal_connect(G_OBJECT(pwDice), "expose_event", G_CALLBACK(setdice_widget_expose), sdd);
            g_signal_connect(G_OBJECT(pwDice), "button_press_event", G_CALLBACK(dice_widget_press), bd);
            gtk_table_attach_defaults(GTK_TABLE(pw), pwDice, x, x + 1, y, y + 1);
        }
    }

    gtk_table_set_row_spacings(GTK_TABLE(pw), 2 * setSize);
    gtk_table_set_col_spacings(GTK_TABLE(pw), 1 * setSize);
    gtk_container_set_border_width(GTK_CONTAINER(pw), setSize);

    g_signal_connect(G_OBJECT(main_table), "destroy", G_CALLBACK(DestroySetDice), sdd);

    return main_table;
}

#if USE_BOARD3D
extern void
InitBoardData(BoardData * bd)
{                               /* Initialize some BoardData settings on new game start */
    /* Only needed for 3d board */
    if (display_is_3d(bd->rd)) {
        /* Move cube back to center */
        bd->cube = 0;
        bd->cube_owner = 0;
        bd->doubled = 0;

        bd->resigned = 0;

        /* Set dice so 3d roll happens */
        bd->diceShown = DICE_BELOW_BOARD;
        bd->diceRoll[0] = bd->diceRoll[1] = 0;

        UpdateShadows(bd->bd3d);
        updateFlagOccPos(bd, bd->bd3d);
        SetupViewingVolume3d(bd, bd->bd3d, bd->rd);
    }
}
#endif
