/*
 * progress.c
 *
 * by Joern Thyssen <jth@gnubg.org>, 2003
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
 * $Id: progress.c,v 1.74 2015/01/25 20:14:45 plm Exp $
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <math.h>

#if USE_GTK
#include <gtk/gtk.h>
#endif                          /* USE_GTK */


#include "eval.h"
#include "rollout.h"
#include "progress.h"
#include "backgammon.h"
#include "format.h"
#include "time.h"

#if USE_GTK
#include "gtkgame.h"
#include "gtkwindows.h"
typedef enum _rollout_colls {
    TITLE_C, RANK_C, TRIALS_C, WIN_C, WIN_G_C, WIN_BG_C, LOSE_G_C, LOSE_BG_C, CLESS_C, CFUL_C, CFUL_S_C, JSD_C,
    N_ROLLOUT_COLS
} rollout_cols;

#endif                          /* USE_GTK */


typedef struct _rolloutprogress {

    void *p;
    int n;
    char **ppch;
    int iNextAlternative;
    int iNextGame;
    time_t tStart;

#if USE_GTK
    rolloutstat *prs;
    GtkWidget *pwRolloutDialog;
    GtkWidget *pwRolloutResult;
    GtkListStore *pwRolloutResultList;
    GtkWidget *pwRolloutProgress;
    GtkWidget *pwRolloutOK;
    GtkWidget *pwRolloutStop;
    GtkWidget *pwRolloutStopAll;
    GtkWidget *pwRolloutViewStat;
    gulong nRolloutSignal;
    GtkWidget *pwElapsed;
    GtkWidget *pwLeft;
    GtkWidget *pwSE;
    int nGamesDone;
    char ***pListText;
    int stopped;
#endif

} rolloutprogress;

#if USE_GTK
static void
AllocTextList(rolloutprogress * prp)
{                               /* 2d array to cache displayed widget text */
    int i;
    int lines = prp->n;
    prp->pListText = malloc(sizeof(char **) * lines * 2);

    for (i = 0; i < lines; i++) {
        prp->pListText[i * 2] = malloc(sizeof(char *) * (N_ROLLOUT_COLS));
        memset(prp->pListText[i * 2], 0, sizeof(char *) * (N_ROLLOUT_COLS));
        prp->pListText[i * 2 + 1] = malloc(sizeof(char *) * (N_ROLLOUT_COLS));
        memset(prp->pListText[i * 2 + 1], 0, sizeof(char *) * (N_ROLLOUT_COLS));
    }
}

static void
FreeTextList(rolloutprogress * prp)
{                               /* destroy list */
    int i;
    int lines = prp->n;

    for (i = 0; i < lines; i++) {
        free(prp->pListText[i * 2]);
        free(prp->pListText[i * 2 + 1]);
    }
    free(prp->pListText);
}

#endif


/*
 *
 */

static time_t
time_left(unsigned int n_games_todo, unsigned int n_games_done, unsigned int initial_game_count, time_t t_start)
{
    float pt;
    time_t t_now;
    time(&t_now);
    pt = ((float) n_games_todo) / (n_games_done - initial_game_count);
    return (time_t) (pt * (t_now - t_start));
}

static char *
formatDelta(const time_t t)
{

    static char sz[128];

    if (t < 60)
        sprintf(sz, "%ds", (int) t);
    else if (t < 60 * 60)
        sprintf(sz, "%dm%02ds", (int) t / 60, (int) t % 60);
    else if (t < 24 * 60 * 60)
        sprintf(sz, "%dh%02dm%02ds", (int) t / 3600, ((int) t % 3600) / 60, (int) t % 60);
    else
        sprintf(sz, "%dd%02dh%02dm%02ds",
                (int) t / 86400, ((int) t % 86400) / 3600, ((int) t % 3600) / 60, (int) t % 60);

    return sz;

}


static float
estimatedSE(const float rSE, const int iGame, const int nTrials)
{

    return rSE * sqrtf((float) iGame / (nTrials - 1));

}

#if USE_GTK

/*
 * Make pages with statistics.
 */

static void
add_stat_columns(GtkTreeView * treeview, const char *title, const char **headers, int n)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    int i, j;
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes(title, renderer, "text", 0, NULL);
    gtk_tree_view_column_set_alignment(column, 1.0);
    gtk_tree_view_append_column(treeview, column);

    for (i = 0; i < 2; i++) {
        for (j = 0; j < n; j++) {
            char *header = g_strdup_printf("%s%s", headers[j], ap[i].szName);
            renderer = gtk_cell_renderer_text_new();
            g_object_set(renderer, "xalign", 1.0, NULL);
            column = gtk_tree_view_column_new_with_attributes(header, renderer, "text", 1 + j + i * n, NULL);
            gtk_tree_view_column_set_expand(column, TRUE);
            gtk_tree_view_column_set_alignment(column, 1.0);
            gtk_tree_view_append_column(treeview, column);
            g_free(header);
        }
    }
}

static GtkTreeModel *
create_win_model(const rolloutstat * prs, int cGames, int *cGamesCount)
{
    GtkListStore *store;
    GtkTreeIter iter;
    gchar *s[6];
    int i, j;
    int anTotal[6];
    *cGamesCount = 0;
    memset(anTotal, 0, sizeof(anTotal));

    store = gtk_list_store_new(7, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                               G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    for (i = 0; i < STAT_MAXCUBE; i++) {
        gchar *title;

        s[0] = g_strdup_printf("%d", prs->acWin[i]);
        s[1] = g_strdup_printf("%d", prs->acWinGammon[i]);
        s[2] = g_strdup_printf("%d", prs->acWinBackgammon[i]);
        s[3] = g_strdup_printf("%d", (prs + 1)->acWin[i]);
        s[4] = g_strdup_printf("%d", (prs + 1)->acWinGammon[i]);
        s[5] = g_strdup_printf("%d", (prs + 1)->acWinBackgammon[i]);

        gtk_list_store_append(store, &iter);
        title = g_strdup_printf((i < STAT_MAXCUBE - 1) ? _("%d-cube") : _(">= %d-cube"), 1 << i);
        gtk_list_store_set(store, &iter, 0, title, 1, s[0], 2, s[1], 3, s[2], 4, s[3], 5, s[4], 6, s[5], -1);

        anTotal[0] += prs->acWin[i];
        anTotal[1] += prs->acWinGammon[i];
        anTotal[2] += prs->acWinBackgammon[i];
        anTotal[3] += (prs + 1)->acWin[i];
        anTotal[4] += (prs + 1)->acWinGammon[i];
        anTotal[5] += (prs + 1)->acWinBackgammon[i];

        for (j = 0; j < 6; j++)
            g_free(s[j]);
    }

    for (j = 0; j < 6; j++)
        s[j] = g_strdup_printf("%d", anTotal[j]);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Total"), 1, s[0], 2, s[1], 3, s[2], 4, s[3], 5, s[4], 6, s[5], -1);
    for (j = 0; j < 6; j++)
        g_free(s[j]);

    for (j = 0; j < 6; j++) {
        s[j] = g_strdup_printf("%6.2f%%", 100.0f * (float) anTotal[j] / (float) cGames);
        *cGamesCount += anTotal[j];
    }
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "", 1, s[0], 2, s[1], 3, s[2], 4, s[3], 5, s[4], 6, s[5], -1);
    for (j = 0; j < 6; j++)
        g_free(s[j]);

    return GTK_TREE_MODEL(store);
}

static GtkWidget *
GTKStatPageWin(const rolloutstat * prs, const int cGames)
{
    GtkWidget *pw;
    GtkWidget *pwLabel;
    GtkWidget *treeview;
    GtkTreeModel *model;
    const char *headers[] = { N_("Win Single\n"), N_("Win Gammon\n"), N_("Win BG\n") };
    int cGamesCount = 0;
    char *sz;

    pw = gtk_vbox_new(FALSE, 0);

    pwLabel = gtk_label_new(_("Win statistics"));

    gtk_box_pack_start(GTK_BOX(pw), pwLabel, FALSE, FALSE, 4);

    /* create treeview */
    model = create_win_model(prs, cGames, &cGamesCount);
    treeview = gtk_tree_view_new_with_model(model);
    g_object_unref(model);
    add_stat_columns(GTK_TREE_VIEW(treeview), _("Cube"), headers, 3);

    gtk_box_pack_start(GTK_BOX(pw), treeview, TRUE, TRUE, 0);

    /* Truncated should be the number of games truncated at the
     * 2-sided bearoff database, but there is another possible
     * discrepancy between cGames and cGamesCount for cube rollouts :
     * one of the double/no-double branch could have had up to
     * nthreads less trials than the other, leading to a negative
     * number here if there is no truncation, like in match play.
     * Don't display this but a 0 instead.
     */
    sz = g_strdup_printf(_("%d/%d games truncated"), ((cGames - cGamesCount) >= 0 ? (cGames - cGamesCount) : 0),
                         cGames);

    pwLabel = gtk_label_new(sz);
    gtk_box_pack_start(GTK_BOX(pw), pwLabel, FALSE, FALSE, 4);
    g_free(sz);

    return pw;
}

static GtkTreeModel *
create_cube_model(const rolloutstat * prs, int cGames, int *anTotal)
{
    GtkListStore *store;
    GtkTreeIter iter;
    gchar *s[4];
    int i, j;
    store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    memset(anTotal, 0, 4 * sizeof(int));

    for (i = 0; i < STAT_MAXCUBE; i++) {
        gchar *title;

        s[0] = g_strdup_printf("%d", prs->acDoubleTake[i]);
        s[1] = g_strdup_printf("%d", prs->acDoubleDrop[i]);
        s[2] = g_strdup_printf("%d", (prs + 1)->acDoubleTake[i]);
        s[3] = g_strdup_printf("%d", (prs + 1)->acDoubleDrop[i]);
        gtk_list_store_append(store, &iter);
        title = g_strdup_printf((i < STAT_MAXCUBE - 1) ? _("%d-cube") : _(">= %d-cube"), 2 << i);
        gtk_list_store_set(store, &iter, 0, title, 1, s[0], 2, s[1], 3, s[2], 4, s[3], -1);
        for (j = 0; j < 4; j++)
            g_free(s[j]);

        anTotal[0] += prs->acDoubleTake[i];
        anTotal[1] += prs->acDoubleDrop[i];
        anTotal[2] += (prs + 1)->acDoubleTake[i];
        anTotal[3] += (prs + 1)->acDoubleDrop[i];

    }

    for (j = 0; j < 4; j++)
        s[j] = g_strdup_printf("%d", anTotal[j]);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Total"), 1, s[0], 2, s[1], 3, s[2], 4, s[3], -1);
    for (j = 0; j < 4; j++)
        g_free(s[j]);

    for (j = 0; j < 4; j++)
        s[j] = g_strdup_printf("%6.2f%%", 100.0f * (float) anTotal[j] / (float) cGames);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "", 1, s[0], 2, s[1], 3, s[2], 4, s[3], -1);
    for (j = 0; j < 4; j++)
        g_free(s[j]);

    return GTK_TREE_MODEL(store);
}

static GtkWidget *
GTKStatPageCube(const rolloutstat * prs, const int cGames)
{
    GtkWidget *pw;
    GtkWidget *pwLabel;
    GtkWidget *treeview;
    GtkTreeModel *model;
    const char *headers[] = { N_("Double, take\n"), N_("Double, pass\n") };
    int anTotal[4];
    char sz[100];

    pw = gtk_vbox_new(FALSE, 0);

    pwLabel = gtk_label_new(_("Cube statistics"));

    gtk_box_pack_start(GTK_BOX(pw), pwLabel, FALSE, FALSE, 4);

    /* create treeview */
    model = create_cube_model(prs, cGames, anTotal);
    treeview = gtk_tree_view_new_with_model(model);
    g_object_unref(model);
    add_stat_columns(GTK_TREE_VIEW(treeview), _("Cube"), headers, 2);

    gtk_box_pack_start(GTK_BOX(pw), treeview, TRUE, TRUE, 0);

    if (anTotal[1] + anTotal[0]) {
        sprintf(sz, _("Cube efficiency for %s: %7.4f"), ap[0].szName, (float) anTotal[0] / (anTotal[1] + anTotal[0]));
        pwLabel = gtk_label_new(sz);
        gtk_box_pack_start(GTK_BOX(pw), pwLabel, FALSE, FALSE, 4);
    }

    if (anTotal[2] + anTotal[3]) {
        sprintf(sz, _("Cube efficiency for %s: %7.4f"), ap[1].szName, (float) anTotal[2] / (anTotal[3] + anTotal[2]));
        pwLabel = gtk_label_new(sz);
        gtk_box_pack_start(GTK_BOX(pw), pwLabel, FALSE, FALSE, 4);
    }
    return pw;

}

static GtkTreeModel *
create_bearoff_model(const rolloutstat * prs)
{
    GtkListStore *store;
    GtkTreeIter iter;
    gchar *s1, *s2;
    store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    s1 = g_strdup_printf("%d", prs->nBearoffMoves);
    s2 = g_strdup_printf("%d", (prs + 1)->nBearoffMoves);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Moves with bearoff"), 1, s1, 2, s2, -1);
    g_free(s1);
    g_free(s2);

    s1 = g_strdup_printf("%d", prs->nBearoffPipsLost);
    s2 = g_strdup_printf("%d", (prs + 1)->nBearoffPipsLost);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Pips lost"), 1, s1, 2, s2, -1);
    g_free(s1);
    g_free(s2);

    if (prs->nBearoffMoves)
        s1 = g_strdup_printf("%7.2f", (float) prs->nBearoffPipsLost / prs->nBearoffMoves);
    else
        s1 = g_strdup_printf("n/a");

    if ((prs + 1)->nBearoffMoves)
        s2 = g_strdup_printf("%7.2f", (float) (prs + 1)->nBearoffPipsLost / (prs + 1)->nBearoffMoves);
    else
        s2 = g_strdup_printf("n/a");
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Average Pips lost"), 1, s1, 2, s2, -1);
    g_free(s1);
    g_free(s2);

    return GTK_TREE_MODEL(store);
}

static GtkWidget *
GTKStatPageBearoff(const rolloutstat * prs, const int UNUSED(cGames))
{
    GtkWidget *pw;
    GtkWidget *pwLabel;
    GtkWidget *treeview;
    GtkTreeModel *model;
    const char *headers[] = { "" };

    pw = gtk_vbox_new(FALSE, 0);

    pwLabel = gtk_label_new(_("Bearoff statistics"));

    gtk_box_pack_start(GTK_BOX(pw), pwLabel, FALSE, FALSE, 4);

    /* create treeview */
    model = create_bearoff_model(prs);
    treeview = gtk_tree_view_new_with_model(model);
    g_object_unref(model);
    add_stat_columns(GTK_TREE_VIEW(treeview), "", headers, 1);

    gtk_box_pack_start(GTK_BOX(pw), treeview, TRUE, TRUE, 0);

    return pw;
}


static GtkTreeModel *
create_closed_out_model(const rolloutstat * prs)
{
    GtkListStore *store;
    GtkTreeIter iter;
    gchar *s1, *s2;
    store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    /* add data to the list store */
    s1 = g_strdup_printf("%d", prs->nOpponentClosedOut);
    s2 = g_strdup_printf("%d", (prs + 1)->nOpponentClosedOut);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Number of close-outs"), 1, s1, 2, s2, -1);
    g_free(s1);
    g_free(s2);

    if (prs->nOpponentClosedOut)
        s1 = g_strdup_printf("%7.2f", 1.0f + prs->rOpponentClosedOutMove / prs->nOpponentClosedOut);
    else
        s1 = g_strdup("n/a");
    if ((prs + 1)->nOpponentClosedOut)
        s2 = g_strdup_printf("%7.2f", 1.0f + (prs + 1)->rOpponentClosedOutMove / (prs + 1)->nOpponentClosedOut);
    else
        s2 = g_strdup("n/a");
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Average move number for close out"), 1, s1, 2, s2, -1);
    g_free(s1);
    g_free(s2);

    return GTK_TREE_MODEL(store);
}

static GtkWidget *
GTKStatPageClosedOut(const rolloutstat * prs, const int UNUSED(cGames))
{
    GtkWidget *pw;
    GtkWidget *pwLabel;
    GtkWidget *treeview;
    GtkTreeModel *model;
    const char *headers[] = { "" };

    pw = gtk_vbox_new(FALSE, 0);

    pwLabel = gtk_label_new(_("Closed out statistics"));

    gtk_box_pack_start(GTK_BOX(pw), pwLabel, FALSE, FALSE, 4);

    /* create treeview */
    model = create_closed_out_model(prs);
    treeview = gtk_tree_view_new_with_model(model);
    g_object_unref(model);
    add_stat_columns(GTK_TREE_VIEW(treeview), "", headers, 1);

    gtk_box_pack_start(GTK_BOX(pw), treeview, TRUE, TRUE, 0);

    return pw;

}

static GtkTreeModel *
create_hit_model(const rolloutstat * prs, int cGames)
{
    GtkListStore *store;
    GtkTreeIter iter;
    gchar *s1, *s2;
    store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    /* add data to the list store */
    s1 = g_strdup_printf("%d", prs->nOpponentHit);
    s2 = g_strdup_printf("%d", (prs + 1)->nOpponentHit);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Number of games with hit(s)"), 1, s1, 2, s2, -1);
    g_free(s1);
    g_free(s2);

    s1 = g_strdup_printf("%7.2f%%", 100.0 * (prs)->nOpponentHit / (1.0 * cGames));
    s2 = g_strdup_printf("%7.2f%%", 100.0 * (prs + 1)->nOpponentHit / (1.0 * cGames));
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Percent games with hits"), 1, s1, 2, s2, -1);
    g_free(s1);
    g_free(s2);

    if (prs->nOpponentHit)
        s1 = g_strdup_printf("%7.2f", 1.0f + prs->rOpponentHitMove / (1.0f * prs->nOpponentHit));
    else
        s1 = g_strdup_printf("n/a");

    if ((prs + 1)->nOpponentHit)
        s2 = g_strdup_printf("%7.2f", 1.0f + (prs + 1)->rOpponentHitMove / (1.0f * (prs + 1)->nOpponentHit));
    else
        s2 = g_strdup_printf("n/a");
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Average move number for first hit"), 1, s1, 2, s2, -1);
    g_free(s1);
    g_free(s2);

    return GTK_TREE_MODEL(store);
}

static GtkWidget *
GTKStatPageHit(const rolloutstat * prs, const int cGames)
{
    GtkWidget *pw;
    GtkWidget *pwLabel;
    GtkWidget *treeview;
    GtkTreeModel *model;
    const char *headers[] = { "" };

    pw = gtk_vbox_new(FALSE, 0);

    pwLabel = gtk_label_new(_("Hit statistics"));

    gtk_box_pack_start(GTK_BOX(pw), pwLabel, FALSE, FALSE, 4);

    /* create treeview */
    model = create_hit_model(prs, cGames);
    treeview = gtk_tree_view_new_with_model(model);
    g_object_unref(model);
    add_stat_columns(GTK_TREE_VIEW(treeview), "", headers, 1);

    gtk_box_pack_start(GTK_BOX(pw), treeview, TRUE, TRUE, 0);

    return pw;

}

static GtkWidget *
GTKRolloutStatPage(const rolloutstat * prs, const int cGames)
{


    /* GTK Widgets */

    GtkWidget *pw;
    GtkWidget *pwWin, *pwCube, *pwHit, *pwBearoff, *pwClosedOut;

    GtkWidget *psw;

    /* Create notebook pages */

    pw = gtk_vbox_new(FALSE, 0);

    pwWin = GTKStatPageWin(prs, cGames);
    pwCube = GTKStatPageCube(prs, cGames);
    pwBearoff = GTKStatPageBearoff(prs, cGames);
    pwHit = GTKStatPageHit(prs, cGames);
    pwClosedOut = GTKStatPageClosedOut(prs, cGames);

    psw = gtk_scrolled_window_new(NULL, NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(psw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_box_pack_start(GTK_BOX(pw), pwWin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pw), pwCube, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pw), pwBearoff, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pw), pwClosedOut, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pw), pwHit, FALSE, FALSE, 0);

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(psw), pw);


    return psw;

}


static void
GTKViewRolloutStatistics(GtkWidget * UNUSED(widget), gpointer data)
{

    /* Rollout statistics information */

    rolloutprogress *prp = (rolloutprogress *) data;
    rolloutstat *prs = prp->prs;
    int cGames = prp->nGamesDone;
    int nRollouts = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(prp->pwRolloutResultList), NULL);
    int i;

    /* GTK Widgets */

    GtkWidget *pwDialog;
    GtkWidget *pwNotebook;

    /* Temporary variables */

    char *sz;

    /* Create dialog */

    pwDialog = GTKCreateDialog(_("Rollout statistics"), DT_INFO, prp->pwRolloutDialog, DIALOG_FLAG_MODAL, NULL, NULL);
    gtk_window_set_default_size(GTK_WINDOW(pwDialog), 0, 400);

    /* Create notebook pages */

    pwNotebook = gtk_notebook_new();

    gtk_container_set_border_width(GTK_CONTAINER(pwNotebook), 4);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwNotebook);

    for (i = 0; i < nRollouts; i++) {
        GtkTreeIter iter;
        gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(prp->pwRolloutResultList), &iter, NULL, i);
        gtk_tree_model_get(GTK_TREE_MODEL(prp->pwRolloutResultList), &iter, 0, &sz, -1);
        gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), GTKRolloutStatPage(&prs[i * 2], cGames), gtk_label_new(sz));
        g_free(sz);
    }

    GTKRunDialog(pwDialog);
}

static void
RolloutCancel(GtkObject * UNUSED(po), rolloutprogress * prp)
{
    pwGrab = pwOldGrab;
    prp->pwRolloutDialog = NULL;
    prp->pwRolloutResult = NULL;
    prp->pwRolloutResultList = NULL;
    prp->pwRolloutProgress = NULL;
    fInterrupt = TRUE;
}

static void
RolloutStop(GtkObject * UNUSED(po), rolloutprogress * prp)
{
    fInterrupt = TRUE;
    prp->stopped = -1;
}


static void
RolloutStopAll(GtkObject * UNUSED(po), rolloutprogress * prp)
{
    fInterrupt = TRUE;
    prp->stopped = -2;
}

static void
create_rollout_list(int n, char asz[][40], GtkWidget ** View, GtkListStore ** List, gboolean cubeful)
{
    int i;
    GtkTreeModel *sort_model;
    static const char *aszTitle[N_ROLLOUT_COLS] = {
        NULL,
        N_("Rank"),
        N_("Trials"),
        N_("Win"),
        N_("Win (g)"),
        N_("Win (bg)"),
        N_("Lose (g)"),
        N_("Lose (bg)"),
        N_("Cubeless"),
        N_("Cubeful"),
        N_("Std dev"),
        N_("JSDs")
    };
    char *aszTemp[N_ROLLOUT_COLS];

    for (i = 0; i < N_ROLLOUT_COLS; i++)
        aszTemp[i] = aszTitle[i] ? gettext(aszTitle[i]) : "";

    *List = gtk_list_store_new(N_ROLLOUT_COLS,
                               G_TYPE_STRING,
                               G_TYPE_INT,
                               G_TYPE_INT,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    for (i = 0; i < n; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(*List, &iter);
        gtk_list_store_set(*List, &iter, TITLE_C, asz[i], RANK_C, i, TRIALS_C, 0, -1);
    }
    sort_model = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(*List));
    *View = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sort_model));

    for (i = 0; i < N_ROLLOUT_COLS; i++) {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        if (i == CFUL_C && !cubeful)
            continue;
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes(aszTemp[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(*View), column);
        gtk_tree_view_column_set_sort_column_id(column, i);
    }
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sort_model), RANK_C, GTK_SORT_ASCENDING);
}

static void
GTKRolloutProgressStart(const cubeinfo * UNUSED(pci), const int n,
                        rolloutstat aars[][2], rolloutcontext * prc, char asz[][40], gboolean multiple, void **pp)
{

    gchar *sz;
    GtkWidget *pwVbox;
    GtkWidget *pwButtons;
    GtkWidget *pwhbox;
    rolloutprogress *prp = (rolloutprogress *) g_malloc(sizeof(rolloutprogress));
    *pp = prp;
    prp->prs = (rolloutstat *) aars;
    if (aars)
        memset(aars, 0, 2 * n * sizeof(rolloutstat));
    prp->n = n;
    prp->nGamesDone = 0;
    prp->stopped = 0;
    fInterrupt = FALSE;

    AllocTextList(prp);

    prp->pwRolloutDialog = GTKCreateDialog(_("GNU Backgammon - Rollout"), DT_INFO, NULL,
                                           DIALOG_FLAG_MODAL | DIALOG_FLAG_MINMAXBUTTONS | DIALOG_FLAG_NOTIDY, NULL,
                                           NULL);
    prp->pwRolloutViewStat = gtk_button_new_with_label(_("View statistics"));
    prp->pwRolloutStop = gtk_button_new_with_label(_("Stop"));
    if (multiple)
        prp->pwRolloutStopAll = gtk_button_new_with_label(_("Stop All"));

    pwOldGrab = pwGrab;
    pwGrab = prp->pwRolloutDialog;

    prp->nRolloutSignal = g_signal_connect(G_OBJECT(prp->pwRolloutDialog), "destroy", G_CALLBACK(RolloutCancel), prp);

    /* Buttons */

    pwButtons = DialogArea(prp->pwRolloutDialog, DA_BUTTONS);
    prp->pwRolloutOK = DialogArea(prp->pwRolloutDialog, DA_OK);

    gtk_container_add(GTK_CONTAINER(pwButtons), prp->pwRolloutStop);
    if (multiple)
        gtk_container_add(GTK_CONTAINER(pwButtons), prp->pwRolloutStopAll);

    if (aars && (prc->nGamesDone == 0))
        gtk_container_add(GTK_CONTAINER(pwButtons), prp->pwRolloutViewStat);

    gtk_widget_set_sensitive(prp->pwRolloutOK, FALSE);
    gtk_widget_set_sensitive(prp->pwRolloutViewStat, FALSE);

    /* Setup signal */

    g_signal_connect(G_OBJECT(prp->pwRolloutStop), "clicked", G_CALLBACK(RolloutStop), prp);

    if (multiple)
        g_signal_connect(G_OBJECT(prp->pwRolloutStopAll), "clicked", G_CALLBACK(RolloutStopAll), prp);


    g_signal_connect(G_OBJECT(prp->pwRolloutViewStat), "clicked", G_CALLBACK(GTKViewRolloutStatistics), prp);

    pwVbox = gtk_vbox_new(FALSE, 4);
    create_rollout_list(n, asz, &prp->pwRolloutResult, &prp->pwRolloutResultList, prc->fCubeful);
    prp->pwRolloutProgress = gtk_progress_bar_new();

    gtk_box_pack_start(GTK_BOX(pwVbox), prp->pwRolloutResult, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pwVbox), prp->pwRolloutProgress, FALSE, FALSE, 0);

    /* time elapsed and left */

    pwhbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(pwVbox), pwhbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pwhbox), gtk_label_new(_("Time elapsed")), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(pwhbox), prp->pwElapsed = gtk_label_new(_("n/a")), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(pwhbox), gtk_label_new(_("Estimated time left")), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(pwhbox), prp->pwLeft = gtk_label_new(_("n/a")), FALSE, FALSE, 4);
    if (asz && asz[0] && *asz[0])
        sz = g_strdup_printf(_("Estimated SE for \"%s\" after %d trials "), asz[0], prc->nTrials);
    else
        sz = g_strdup_printf(_("Estimated SE after %d trials "), prc->nTrials);

    gtk_box_pack_start(GTK_BOX(pwhbox), gtk_label_new(sz), FALSE, FALSE, 4);
    g_free(sz);
    gtk_box_pack_start(GTK_BOX(pwhbox), prp->pwSE = gtk_label_new(_("n/a")), FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(DialogArea(prp->pwRolloutDialog, DA_MAIN)), pwVbox);
    gtk_widget_show_all(prp->pwRolloutDialog);

    /* record start time */
    time(&prp->tStart);

}

static void
GTKRolloutProgress(float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                   float aarStdDev[][NUM_ROLLOUT_OUTPUTS],
                   const rolloutcontext * prc,
                   const cubeinfo aci[],
                   unsigned int initial_game_count,
                   const int iGame,
                   const int iAlternative,
                   const int nRank,
                   const float rJsd, const int fStopped, const int fShowRanks, int fCubeRollout, rolloutprogress * prp)
{

    static unsigned int n_games_todo = 0;
    static unsigned int n_games_done = 0;
    static int min_games_done = 0;
    char sz[32];
    int i;
    gchar *gsz;
    float frac;
    GtkTreeIter iter;

    if (!prp || !prp->pwRolloutResult)
        return;

    gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(prp->pwRolloutResultList), &iter, NULL, iAlternative);
    gtk_list_store_set(prp->pwRolloutResultList, &iter, TRIALS_C, iGame + 1, -1);
    for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++) {

        /* result */

        if (i < OUTPUT_EQUITY)
            strcpy(sz, OutputPercent(aarOutput[iAlternative][i]));
        else if (i == OUTPUT_EQUITY)
            strcpy(sz, OutputEquityScale(aarOutput[iAlternative][i], &aci[iAlternative], &aci[0], TRUE));
        else
            strcpy(sz, prc->fCubeful ? OutputMWC(aarOutput[iAlternative][i], &aci[0], TRUE) : "n/a");

        gtk_list_store_set(prp->pwRolloutResultList, &iter, i + 3, sz, -1);

    }
    if (prc->fCubeful)
        strcpy(sz, OutputMWC(aarStdDev[iAlternative][OUTPUT_CUBEFUL_EQUITY], &aci[0], FALSE));
    else
        strcpy(sz, OutputEquityScale(aarStdDev[iAlternative][OUTPUT_EQUITY], &aci[iAlternative], &aci[0], FALSE));
    gtk_list_store_set(prp->pwRolloutResultList, &iter, i + 3, sz, -1);
    if (fShowRanks && iGame > 1) {
        gtk_list_store_set(prp->pwRolloutResultList, &iter, RANK_C, nRank, -1);
        if (nRank != 1 || fCubeRollout)
            sprintf(sz, "%5.3f", rJsd);
        else
            strcpy(sz, " ");
        gtk_list_store_set(prp->pwRolloutResultList, &iter, i + 4, sz, -1);
    }

    /* Update progress bar with highest number trials for all the alternatives */
    if (iAlternative == 0) {
        n_games_todo = 0;
        n_games_done = 0;
        min_games_done = prc->nTrials;
    }
    n_games_done += iGame + 1;
    if (!fStopped) {
        n_games_todo += prc->nTrials - (iGame + 1);
        if (iGame < min_games_done)
            min_games_done = iGame + 1;
    }
    if (iAlternative == (prp->n - 1)) {
        frac = ((float) min_games_done) / prc->nTrials;
        gsz = g_strdup_printf("%d/%d (%d%%)", min_games_done, prc->nTrials, (int) (100.0f * frac));
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(prp->pwRolloutProgress), frac);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(prp->pwRolloutProgress), gsz);
        g_free(gsz);
        prp->nGamesDone = min_games_done;
    }

    /* calculate estimate time left */

    if ((iAlternative == (prp->n - 1)) && n_games_done > initial_game_count) {
        time_t t = time_left(n_games_todo, n_games_done, initial_game_count, prp->tStart);
        gtk_label_set_text(GTK_LABEL(prp->pwElapsed), formatDelta(time(NULL) - prp->tStart));
        gtk_label_set_text(GTK_LABEL(prp->pwLeft), formatDelta(t));

    }

    /* calculate estimated SE */

    if (!iAlternative && iGame > 10) {

        float r;

        if (prc->fCubeful) {
            r = estimatedSE(aarStdDev[0][OUTPUT_CUBEFUL_EQUITY], iGame, prc->nTrials);
            gtk_label_set_text(GTK_LABEL(prp->pwSE), OutputMWC(r, &aci[0], FALSE));
        } else {
            r = estimatedSE(aarStdDev[0][OUTPUT_EQUITY], iGame, prc->nTrials);
            gtk_label_set_text(GTK_LABEL(prp->pwSE), OutputEquityScale(r, &aci[0], &aci[0], FALSE));
        }

    }

    return;
}

static int
GTKRolloutProgressEnd(void **pp, gboolean destroy)
{

    gchar *gsz;
    rolloutprogress *prp = *pp;
    int stopped = prp->stopped;

    fInterrupt = FALSE;

    pwGrab = pwOldGrab;

    FreeTextList(prp);

    /* if they cancelled the rollout early, prp->pwRolloutDialog has
     * already been destroyed */
    if (!prp->pwRolloutDialog) {
        g_free(*pp);
        return stopped;
    }

    gtk_widget_set_sensitive(prp->pwRolloutOK, TRUE);
    gtk_widget_set_sensitive(prp->pwRolloutStop, FALSE);
    gtk_widget_set_sensitive(prp->pwRolloutViewStat, TRUE);

    gsz = g_strdup_printf(_("Finished (%d trials)"), prp->nGamesDone);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(prp->pwRolloutProgress), gsz);
    g_free(gsz);

    g_signal_handler_disconnect(G_OBJECT(prp->pwRolloutDialog), prp->nRolloutSignal);
    if (destroy)
        gtk_widget_destroy(prp->pwRolloutDialog);
    else {
        g_signal_connect(G_OBJECT(prp->pwRolloutDialog), "destroy", G_CALLBACK(gtk_main_quit), NULL);
        gtk_main();
    }

    prp->pwRolloutProgress = NULL;

    g_free(*pp);
    return stopped;
}


#endif                          /* USE_GTK */

static void
TextRolloutProgressStart(const cubeinfo * UNUSED(pci), const int n,
                         rolloutstat UNUSED(aars[2][2]),
                         rolloutcontext * prc, char asz[][40], gboolean UNUSED(multiple), void **pp)
{

    int i;

    rolloutprogress *prp = (rolloutprogress *) g_malloc(sizeof(rolloutprogress));

    *pp = prp;

    prp->ppch = (char **) g_malloc(n * sizeof(char *));
    for (i = 0; i < n; ++i)
        prp->ppch[i] = (char *) asz[i];
    prp->n = n;
    prp->iNextAlternative = 0;
    prp->iNextGame = prc->nTrials / 10;

    /* record start time */
    time(&prp->tStart);

}

static int
TextRolloutProgressEnd(void **pp)
{

    rolloutprogress *prp = *pp;

    g_free(prp->ppch);
    g_free(prp);

    output("\r\n");
    fflush(stdout);
    return fInterrupt ? -1 : 0;

}


static void
TextRolloutProgress(float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                    float aarStdDev[][NUM_ROLLOUT_OUTPUTS], const rolloutcontext * prc,
                    const cubeinfo aci[], unsigned int initial_game_count,
                    const int iGame, const int iAlternative, const int nRank,
                    const float rJsd, const int fStopped, const int fShowRanks, int fCubeRollout, rolloutprogress * prp)
{

    char *pch, *pc;
    time_t t;
    static unsigned int n_games_todo = 0;
    static unsigned int n_games_done = 0;
    static int min_games_done = 0;

    /* write progress 1/10th trial or just when called if mt */

    if (!iAlternative)
        outputl("");

    pch = OutputRolloutResult(NULL,
                              (char (*)[1024]) prp->ppch[iAlternative],
                              (float (*)[NUM_ROLLOUT_OUTPUTS]) aarOutput[iAlternative],
                              (float (*)[NUM_ROLLOUT_OUTPUTS]) aarStdDev[iAlternative],
                              &aci[0], iAlternative, 1, prc->fCubeful);

    if (fShowRanks && iGame > 1) {

        pc = strrchr(pch, '\n');
        *pc = 0;

        if (fCubeRollout)
            sprintf(pc, " %c", fStopped ? 's' : 'r');
        else
            sprintf(pc, " %d%c", nRank, fStopped ? 's' : 'r');

        if (nRank != 1 || fCubeRollout)
            sprintf(strchr(pc, 0), " %5.3f\n", rJsd);
        else
            strcat(pc, "\n");

    }

    prp->iNextAlternative++;
    prp->iNextAlternative = (prp->iNextAlternative) % prp->n;
    if (iAlternative == (prp->n - 1))
        prp->iNextGame += prc->nTrials / 10;

    output(pch);

    output(OutputRolloutContext(NULL, prc));
    if (iAlternative == 0) {
        n_games_todo = 0;
        n_games_done = 0;
        min_games_done = prc->nTrials;
    }
    n_games_done += iGame + 1;
    if (!fStopped) {
        n_games_todo += prc->nTrials - (iGame + 1);
        if (iGame < min_games_done)
            min_games_done = iGame + 1;
    }
    if (iAlternative != (prp->n - 1))
        return;

    /* time elapsed and time left */

    t = time_left(n_games_todo, n_games_done, initial_game_count, prp->tStart);

    outputf(_("Time elapsed %s"), formatDelta(time(NULL) - prp->tStart));
    outputf(_(" Estimated time left %s\n"), formatDelta(t));

    /* estimated SE */

    /* calculate estimated SE */

    if (iGame <= 10)
        return;

    if (prc->fCubeful)
        pc = OutputMWC(estimatedSE(aarStdDev[0][OUTPUT_CUBEFUL_EQUITY], iGame, prc->nTrials), &aci[0], FALSE);
    else
        pc = OutputEquityScale(estimatedSE(aarStdDev[0][OUTPUT_EQUITY], iGame, prc->nTrials), &aci[0], &aci[0], FALSE);

    if (prp->ppch && prp->ppch[0] && *prp->ppch[0])
        outputf(_("Estimated SE for \"%s\" after %d trials %s\n"), prp->ppch[0], prc->nTrials, pc);
    else
        outputf(_("Estimated SE after %d trials %s\n"), prc->nTrials, pc);

}


extern void
RolloutProgressStart(const cubeinfo * pci, const int n,
                     rolloutstat aars[2][2], rolloutcontext * prc, char asz[][40], gboolean multiple, void **pp)
{

    if (!fShowProgress)
        return;

#if USE_GTK
    if (fX) {
        GTKRolloutProgressStart(pci, n, aars, prc, asz, multiple, pp);
        return;
    }
#endif                          /* USE_GTK */

    TextRolloutProgressStart(pci, n, aars, prc, asz, multiple, pp);

}


extern void
RolloutProgress(float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                float aarStdDev[][NUM_ROLLOUT_OUTPUTS],
                const rolloutcontext * prc,
                const cubeinfo aci[],
                unsigned int initial_game_count,
                const int iGame,
                const int iAlternative,
                const int nRank,
                const float rJsd, const int fStopped, const int fShowRanks, int fCubeRollout, void *pUserData)
{

    if (!fShowProgress)
        return;

#if USE_GTK
    if (fX) {
        GTKRolloutProgress(aarOutput, aarStdDev, prc, aci, initial_game_count, iGame, iAlternative,
                           nRank, rJsd, fStopped, fShowRanks, fCubeRollout, pUserData);
        return;
    }
#endif                          /* USE_GTK */

    TextRolloutProgress(aarOutput, aarStdDev, prc, aci, initial_game_count, iGame, iAlternative,
                        nRank, rJsd, fStopped, fShowRanks, fCubeRollout, pUserData);

}

extern int
RolloutProgressEnd(void **pp, gboolean destroy)
{

    if (!fShowProgress)
        return 0;

#if USE_GTK
    if (fX) {
        return GTKRolloutProgressEnd(pp, destroy);
    }
#else
    (void) destroy;             /* suppress unused parameter compiler warning */
#endif                          /* USE_GTK */

    return TextRolloutProgressEnd(pp);

}
