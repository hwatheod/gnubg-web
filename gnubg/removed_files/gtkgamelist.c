/*
 * gtkgamelist.c
 * by Jon Kinsey, 2004
 *
 * Game list window
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
 * $Id: gtkgamelist.c,v 1.42 2013/06/16 02:16:14 mdpetch Exp $
 */

#undef GDK_DISABLE_DEPRECATED
#undef GTK_DISABLE_DEPRECATED

#include "config.h"
#include "gtklocdefs.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "backgammon.h"
#include "gtkboard.h"
#include "drawboard.h"
#include "positionid.h"
#include "gtkgame.h"
#if USE_BOARD3D
#include "fun3d.h"
#endif

static GtkWidget *pwGameList;
static GtkStyle *psGameList, *psCurrent, *psCubeErrors[3], *psChequerErrors[3], *psLucky[N_LUCKS];

static int
gtk_compare_fonts(GtkStyle * psOne, GtkStyle * psTwo)
{
    return pango_font_description_equal(psOne->font_desc, psTwo->font_desc);
}

static void
gtk_set_font(GtkStyle * psStyle, GtkStyle * psValue)
{
    pango_font_description_free(psStyle->font_desc);
    psStyle->font_desc = pango_font_description_copy(psValue->font_desc);
}

typedef struct _gamelistrow {
    moverecord *apmr[2];        /* moverecord entries for each column */
    int fCombined;              /* this message's row is combined across both columns */
} gamelistrow;

extern void
GTKClearMoveRecord(void)
{

    gtk_clist_clear(GTK_CLIST(pwGameList));
}

static void
GameListSelectRow(GtkCList * pcl, gint y, gint x, GdkEventButton * UNUSED(pev), gpointer UNUSED(p))
{
#if USE_BOARD3D
    BoardData *bd = BOARD(pwBoard)->board_data;
#endif
    gamelistrow *pglr;
    moverecord *pmr, *pmrPrev = NULL;
    listOLD *pl;

    if (x < 1 || x > 2)
        return;

    pglr = gtk_clist_get_row_data(pcl, y);
    if (!pglr)
        pmr = NULL;
    else
        pmr = pglr->apmr[(pglr->fCombined) ? 0 : x - 1];

    /* Get previous move record */
    if (pglr && !pglr->fCombined && x == 2)
        x = 1;
    else {
        y--;
        x = 2;
    }
    pglr = gtk_clist_get_row_data(pcl, y);
    if (!pglr)
        pmrPrev = NULL;
    else
        pmrPrev = pglr->apmr[(pglr->fCombined) ? 0 : x - 1];

    if (!pmr && !pmrPrev)
        return;

    for (pl = plGame->plPrev; pl != plGame; pl = pl->plPrev) {
        g_assert(pl->p);
        if (pl == plGame->plPrev && pl->p == pmr && pmr && pmr->mt == MOVE_SETDICE)
            break;

        if (pl->p == pmrPrev && pmr != pmrPrev) {
            /* pmr = pmrPrev; */
            break;
        } else if (pl->plNext->p == pmr) {
            /* pmr = pl->p; */
            break;
        }
    }

    if (pl == plGame)
        /* couldn't find the moverecord they selected */
        return;

    plLastMove = pl;

    CalculateBoard();

    if (pmr && (pmr->mt == MOVE_NORMAL || pmr->mt == MOVE_SETDICE)) {
        /* roll dice */
        ms.gs = GAME_PLAYING;
        ms.anDice[0] = pmr->anDice[0];
        ms.anDice[1] = pmr->anDice[1];
    }
#if USE_BOARD3D
    if (display_is_3d(bd->rd)) {        /* Make sure dice are shown (and not rolled) */
        bd->diceShown = DICE_ON_BOARD;
        bd->diceRoll[0] = !ms.anDice[0];
    }
#endif

    UpdateSetting(&ms.nCube);
    UpdateSetting(&ms.fCubeOwner);
    UpdateSetting(&ms.fTurn);
    UpdateSetting(&ms.gs);

    SetMoveRecord(pl->p);

    ShowBoard();
}

extern void
GL_SetNames(void)
{
    gtk_clist_set_column_title(GTK_CLIST(pwGameList), 1, (ap[0].szName));
    gtk_clist_set_column_title(GTK_CLIST(pwGameList), 2, (ap[1].szName));
}

static void
SetColourIfDifferent(GdkColor cOrg[5], GdkColor cNew[5], GdkColor cDefault[5])
{
    int i;
    for (i = 0; i < 5; i++) {
        if (memcmp(&cNew[i], &cDefault[i], sizeof(GdkColor)))
            memcpy(&cOrg[i], &cNew[i], sizeof(GdkColor));
    }
}

static void
UpdateStyle(GtkStyle * psStyle, GtkStyle * psNew, GtkStyle * psDefault)
{
    /* Only set changed attributes */
    SetColourIfDifferent(psStyle->fg, psNew->fg, psDefault->fg);
    SetColourIfDifferent(psStyle->bg, psNew->bg, psDefault->bg);
    SetColourIfDifferent(psStyle->base, psNew->base, psDefault->base);

    if (!gtk_compare_fonts(psNew, psDefault))
        gtk_set_font(psStyle, psNew);
}

void
GetStyleFromRCFile(GtkStyle ** ppStyle, const char *name, GtkStyle * psBase)
{                               /* Note gtk 1.3 doesn't seem to have a nice way to do this... */
    BoardData *bd = BOARD(pwBoard)->board_data;
    GtkStyle *psDefault, *psNew;
    GtkWidget *dummy, *temp;
    char styleName[100];
    /* Get default style so only changes to this are applied */
    temp = gtk_button_new();
    gtk_widget_ensure_style(temp);
    psDefault = gtk_widget_get_style(temp);

    /* Get Style from rc file */
    strcpy(styleName, "gnubg-");
    strcat(styleName, name);
    dummy = gtk_label_new("");
    gtk_widget_ensure_style(dummy);
    gtk_widget_set_name(dummy, styleName);
    /* Pack in box to make sure style is loaded */
    gtk_box_pack_start(GTK_BOX(bd->table), dummy, FALSE, FALSE, 0);
    psNew = gtk_widget_get_style(dummy);

    /* Base new style on base style passed in */
    *ppStyle = gtk_style_copy(psBase);
    /* Make changes to fg+bg and copy to selected states */
    if (memcmp(&psNew->fg[GTK_STATE_ACTIVE], &psDefault->fg[GTK_STATE_ACTIVE], sizeof(GdkColor)))
        memcpy(&(*ppStyle)->fg[GTK_STATE_NORMAL], &psNew->fg[GTK_STATE_ACTIVE], sizeof(GdkColor));
    if (memcmp(&psNew->fg[GTK_STATE_NORMAL], &psDefault->fg[GTK_STATE_NORMAL], sizeof(GdkColor)))
        memcpy(&(*ppStyle)->fg[GTK_STATE_NORMAL], &psNew->fg[GTK_STATE_NORMAL], sizeof(GdkColor));
    memcpy(&(*ppStyle)->fg[GTK_STATE_SELECTED], &(*ppStyle)->fg[GTK_STATE_NORMAL], sizeof(GdkColor));

    if (memcmp(&psNew->base[GTK_STATE_NORMAL], &psDefault->base[GTK_STATE_NORMAL], sizeof(GdkColor)))
        memcpy(&(*ppStyle)->base[GTK_STATE_NORMAL], &psNew->base[GTK_STATE_NORMAL], sizeof(GdkColor));
    memcpy(&(*ppStyle)->bg[GTK_STATE_SELECTED], &(*ppStyle)->base[GTK_STATE_NORMAL], sizeof(GdkColor));
    /* Update the font if different */
    if (!gtk_compare_fonts(psNew, psDefault))
        gtk_set_font(*ppStyle, psNew);

    /* Remove useless widgets */
    gtk_widget_destroy(dummy);
    g_object_ref_sink(G_OBJECT(temp));
    g_object_unref(G_OBJECT(temp));
}

extern GtkWidget *
GL_Create(void)
{
    GtkStyle *ps;
    gint nMaxWidth;
    char *asz[] = { NULL, NULL, NULL };
    PangoRectangle logical_rect;
    PangoLayout *layout;

    asz[0] = _("#");
    pwGameList = gtk_clist_new_with_titles(3, asz);
    gtk_widget_set_can_focus(pwGameList, FALSE);

    gtk_clist_set_selection_mode(GTK_CLIST(pwGameList), GTK_SELECTION_BROWSE);
    gtk_clist_column_titles_passive(GTK_CLIST(pwGameList));

    GL_SetNames();

    gtk_clist_set_column_justification(GTK_CLIST(pwGameList), 0, GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_resizeable(GTK_CLIST(pwGameList), 0, FALSE);
    gtk_clist_set_column_resizeable(GTK_CLIST(pwGameList), 1, FALSE);
    gtk_clist_set_column_resizeable(GTK_CLIST(pwGameList), 2, FALSE);
    gtk_widget_ensure_style(pwGameList);
    GetStyleFromRCFile(&ps, "gnubg", gtk_widget_get_style(pwGameList));
    ps->base[GTK_STATE_SELECTED] =
        ps->base[GTK_STATE_ACTIVE] =
        ps->base[GTK_STATE_NORMAL] = gtk_widget_get_style(pwGameList)->base[GTK_STATE_NORMAL];
    ps->fg[GTK_STATE_SELECTED] =
        ps->fg[GTK_STATE_ACTIVE] = ps->fg[GTK_STATE_NORMAL] = gtk_widget_get_style(pwGameList)->fg[GTK_STATE_NORMAL];
    gtk_widget_set_style(pwGameList, ps);

    psGameList = gtk_style_copy(ps);
    psGameList->bg[GTK_STATE_SELECTED] = psGameList->bg[GTK_STATE_NORMAL] = ps->base[GTK_STATE_NORMAL];

    psCurrent = gtk_style_copy(psGameList);
    psCurrent->bg[GTK_STATE_SELECTED] = psCurrent->bg[GTK_STATE_NORMAL] =
        psCurrent->base[GTK_STATE_SELECTED] = psCurrent->base[GTK_STATE_NORMAL] = psGameList->fg[GTK_STATE_NORMAL];
    psCurrent->fg[GTK_STATE_SELECTED] = psCurrent->fg[GTK_STATE_NORMAL] = psGameList->bg[GTK_STATE_NORMAL];

    GetStyleFromRCFile(&psCubeErrors[SKILL_VERYBAD], "gamelist-cube-blunder", psGameList);
    GetStyleFromRCFile(&psCubeErrors[SKILL_BAD], "gamelist-cube-error", psGameList);
    GetStyleFromRCFile(&psCubeErrors[SKILL_DOUBTFUL], "gamelist-cube-doubtful", psGameList);

    GetStyleFromRCFile(&psChequerErrors[SKILL_VERYBAD], "gamelist-chequer-blunder", psGameList);
    GetStyleFromRCFile(&psChequerErrors[SKILL_BAD], "gamelist-chequer-error", psGameList);
    GetStyleFromRCFile(&psChequerErrors[SKILL_DOUBTFUL], "gamelist-chequer-doubtful", psGameList);

    GetStyleFromRCFile(&psLucky[LUCK_VERYBAD], "gamelist-luck-bad", psGameList);
    GetStyleFromRCFile(&psLucky[LUCK_VERYGOOD], "gamelist-luck-good", psGameList);

    layout = gtk_widget_create_pango_layout(pwGameList, "99");
    pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
    g_object_unref(layout);
    nMaxWidth = logical_rect.width;
    gtk_clist_set_column_width(GTK_CLIST(pwGameList), 0, nMaxWidth);

    layout = gtk_widget_create_pango_layout(pwGameList, " (set board AAAAAAAAAAAAAA)");
    pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
    g_object_unref(layout);
    nMaxWidth = logical_rect.width;
    gtk_clist_set_column_width(GTK_CLIST(pwGameList), 1, nMaxWidth - 30);
    gtk_clist_set_column_width(GTK_CLIST(pwGameList), 2, nMaxWidth - 30);

    g_signal_connect(G_OBJECT(pwGameList), "select-row", G_CALLBACK(GameListSelectRow), NULL);

    return pwGameList;
}

static int
AddMoveRecordRow(void)
{
    gamelistrow *pglr;
    static char *aszData[] = { NULL, NULL, NULL };
    char szIndex[5];
    int row;

    sprintf(szIndex, "%d", GTK_CLIST(pwGameList)->rows);
    aszData[0] = szIndex;
    row = gtk_clist_append(GTK_CLIST(pwGameList), aszData);
    gtk_clist_set_row_style(GTK_CLIST(pwGameList), row, psGameList);

    pglr = malloc(sizeof(*pglr));
    pglr->fCombined = FALSE;
    pglr->apmr[0] = pglr->apmr[1] = NULL;
    gtk_clist_set_row_data_full(GTK_CLIST(pwGameList), row, pglr, free);

    return row;
}

static void
AddStyle(GtkStyle ** ppsComb, GtkStyle * psNew)
{
    if (!*ppsComb)
        *ppsComb = psNew;
    else {
        *ppsComb = gtk_style_copy(*ppsComb);
        UpdateStyle(*ppsComb, psNew, psGameList);
    }
}

static void
SetCellColour(int row, int col, moverecord * pmr)
{
    GtkStyle *pStyle = NULL;

    if (fStyledGamelist) {
        if (pmr->lt == LUCK_VERYGOOD)
            pStyle = psLucky[LUCK_VERYGOOD];
        if (pmr->lt == LUCK_VERYBAD)
            pStyle = psLucky[LUCK_VERYBAD];

        if (pmr->n.stMove == SKILL_DOUBTFUL)
            AddStyle(&pStyle, psChequerErrors[SKILL_DOUBTFUL]);
        if (pmr->stCube == SKILL_DOUBTFUL)
            AddStyle(&pStyle, psCubeErrors[SKILL_DOUBTFUL]);

        if (pmr->n.stMove == SKILL_BAD)
            AddStyle(&pStyle, psChequerErrors[SKILL_BAD]);
        if (pmr->stCube == SKILL_BAD)
            AddStyle(&pStyle, psCubeErrors[SKILL_BAD]);

        if (pmr->n.stMove == SKILL_VERYBAD)
            AddStyle(&pStyle, psChequerErrors[SKILL_VERYBAD]);
        if (pmr->stCube == SKILL_VERYBAD)
            AddStyle(&pStyle, psCubeErrors[SKILL_VERYBAD]);
    }

    if (!pStyle)
        pStyle = psGameList;

    gtk_clist_set_cell_style(GTK_CLIST(pwGameList), row, col, pStyle);
}

/* Add a moverecord to the game list window.  NOTE: This function must be
 * called _before_ applying the moverecord, so it can be displayed
 * correctly. */
extern void
GTKAddMoveRecord(moverecord * pmr)
{
    gamelistrow *pglr;
    int i, numRows, fPlayer;
    const char *pch = GetMoveString(pmr, &fPlayer, TRUE);
    if (!pch)
        return;

    i = -1;
    numRows = GTK_CLIST(pwGameList)->rows;
    if (numRows > 0) {
        pglr = gtk_clist_get_row_data(GTK_CLIST(pwGameList), numRows - 1);
        if (pglr && !(pglr->fCombined || pglr->apmr[1] || (fPlayer != 1 && pglr->apmr[0])))
            /* Add to second half of current row */
            i = numRows - 1;
    }
    if (i == -1)
        /* Add new row */
        i = AddMoveRecordRow();

    pglr = gtk_clist_get_row_data(GTK_CLIST(pwGameList), i);

    if (fPlayer == -1) {
        pglr->fCombined = TRUE;
        fPlayer = 0;
    } else
        pglr->fCombined = FALSE;

    pglr->apmr[fPlayer] = pmr;

    gtk_clist_set_text(GTK_CLIST(pwGameList), i, fPlayer + 1, pch);

    SetCellColour(i, fPlayer + 1, pmr);
}


/* Select a moverecord as the "current" one.  NOTE: This function must be
 * called _after_ applying the moverecord. */
extern void
GTKSetMoveRecord(moverecord * pmr)
{

    /* highlighted row/col in game record */
    static int yCurrent = -1, xCurrent = -1;

    GtkCList *pcl = GTK_CLIST(pwGameList);
    gamelistrow *pglr;
    int i;
    /* Avoid lots of screen updates */
    if (!frozen)
        SetAnnotation(pmr);

#ifdef UNDEF
    {
        GtkWidget *pwWin = GetPanelWidget(WINDOW_HINT);
        if (pwWin) {
            hintdata *phd = g_object_get_data(G_OBJECT(pwWin), "user_data");
            phd->fButtonsValid = FALSE;
            CheckHintButtons(phd);
        }
    }
#endif

    if (yCurrent != -1 && xCurrent != -1) {
        moverecord *pmrLast = NULL;
        pglr = gtk_clist_get_row_data(pcl, yCurrent);
        if (pglr) {
            pmrLast = pglr->apmr[xCurrent - 1];
            if (pmrLast)
                SetCellColour(yCurrent, xCurrent, pmrLast);
        }
        if (!pmrLast)
            gtk_clist_set_cell_style(pcl, yCurrent, xCurrent, psGameList);
    }

    yCurrent = xCurrent = -1;

    if (!pmr)
        return;

    if (pmr == plGame->plNext->p) {
        g_assert(pmr->mt == MOVE_GAMEINFO);
        yCurrent = 0;

        if (plGame->plNext->plNext->p) {
            moverecord *pmrNext = plGame->plNext->plNext->p;

            if (pmrNext->mt == MOVE_NORMAL && pmrNext->fPlayer == 1)
                xCurrent = 2;
            else
                xCurrent = 1;
        } else
            xCurrent = 1;
    } else {
        for (i = pcl->rows - 1; i >= 0; i--) {
            pglr = gtk_clist_get_row_data(pcl, i);
            if (pglr->apmr[1] == pmr) {
                xCurrent = 2;
                break;
            } else if (pglr->apmr[0] == pmr) {
                xCurrent = 1;
                break;
            }
        }

        yCurrent = i;

        if (yCurrent >= 0 && !(pmr->mt == MOVE_SETDICE && yCurrent == pcl->rows - 1)) {
            do {
                if (++xCurrent > 2) {
                    xCurrent = 1;
                    yCurrent++;
                }

                pglr = gtk_clist_get_row_data(pcl, yCurrent);
            } while (yCurrent < pcl->rows - 1 && !pglr->apmr[xCurrent - 1]);

            if (yCurrent >= pcl->rows)
                AddMoveRecordRow();
        }
    }

    /* Highlight current move */
    gtk_clist_set_cell_style(pcl, yCurrent, xCurrent, psCurrent);

    if (gtk_clist_row_is_visible(pcl, yCurrent) != GTK_VISIBILITY_FULL)
        gtk_clist_moveto(pcl, yCurrent, xCurrent, 0.8f, 0.5f);
}

extern void
GTKPopMoveRecord(moverecord * pmr)
{

    GtkCList *pcl = GTK_CLIST(pwGameList);
    gamelistrow *pglr = NULL;

    gtk_clist_freeze(pcl);

    while (pcl->rows) {
        pglr = gtk_clist_get_row_data(pcl, pcl->rows - 1);

        if (pglr->apmr[0] != pmr && pglr->apmr[1] != pmr)
            gtk_clist_remove(pcl, pcl->rows - 1);
        else
            break;
    }

    if (pcl->rows) {
        if (pglr->apmr[0] == pmr)
            /* the left column matches; delete the row */
            gtk_clist_remove(pcl, pcl->rows - 1);
        else {
            /* the right column matches; delete that column only */
            gtk_clist_set_text(pcl, pcl->rows - 1, 2, NULL);
            pglr->apmr[1] = NULL;
        }
    }

    gtk_clist_thaw(pcl);
}

extern void
GL_Freeze(void)
{
    gtk_clist_freeze(GTK_CLIST(pwGameList));
}

extern void
GL_Thaw(void)
{
    gtk_clist_thaw(GTK_CLIST(pwGameList));
}
