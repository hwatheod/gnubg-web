/*
 * gtkgame.c
 *
 * by Joern Thyssen <jthyssen@dk.ibm.com>, 2002
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
 * $Id: gtktheory.c,v 1.61 2013/06/16 02:16:17 mdpetch Exp $
 */

#include "config.h"
#include "gtklocdefs.h"

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backgammon.h"
#include "eval.h"
#include "gtktheory.h"
#include "matchequity.h"
#include "gtkwindows.h"

#define MAXPLY 4

typedef struct _theorywidget {

    cubeinfo ci;

    /* radio buttons */

    GtkWidget *apwRadio[2];

    /* frames ("match score" and "moeny game") */

    GtkWidget *apwFrame[2];

    /* gammon/backgammon rates widgets */

    GtkAdjustment *aapwRates[2][2];

    /* score */

    GtkAdjustment *apwScoreAway[2];

    /* cube */

    GtkWidget *apwCube[7];
    GtkWidget *pwCubeFrame;

    /* crawford, jacoby and more */

    GtkWidget *pwCrawford, *pwJacoby, *pwBeavers;

    /* reset */

    GtkWidget *pwReset;

    /* market window */

    GtkWidget *apwMW[2];

    /* window graphs */
    GtkWidget *apwGraph[2];

    /* gammon prices */

    GtkWidget *pwGammonPrice;

    /* dead double, live cash, and dead too good points; for graph drawing */
    float aar[2][3];


    /* radio buttons for plies */

    GtkWidget *apwPly[MAXPLY];

} theorywidget;


static void
ResetTheory(GtkWidget * UNUSED(pw), theorywidget * ptw)
{
    float aarRates[2][2];
    evalcontext ec = { FALSE, 0, FALSE, TRUE, 0.0 };
    float arOutput[NUM_OUTPUTS];
    int i, j;

    /* get current gammon rates */

    GetMatchStateCubeInfo(&ptw->ci, &ms);

    getCurrentGammonRates(aarRates, arOutput, msBoard(), &ptw->ci, &ec);

    /* cube */

    j = 1;
    for (i = 0; i < 7; i++) {

        if (j == ptw->ci.nCube) {

            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptw->apwCube[i]), TRUE);
            break;

        }

        j *= 2;

    }

    /* set match play/money play radio button */

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptw->apwRadio[ptw->ci.nMatchTo == 0]), TRUE);

    /* crawford, jacoby, beavers */

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptw->pwCrawford), ptw->ci.fCrawford);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptw->pwJacoby), ptw->ci.fJacoby);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptw->pwBeavers), ptw->ci.fBeavers);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptw->apwPly[ec.nPlies]), TRUE);


    for (i = 0; i < 2; i++) {

        /* set score for player i */

        gtk_adjustment_set_value(GTK_ADJUSTMENT(ptw->apwScoreAway[i]),
                                 (ptw->ci.nMatchTo) ? (ptw->ci.nMatchTo - ptw->ci.anScore[i]) : 1);

        for (j = 0; j < 2; j++)
            /* gammon/backgammon rates */
            gtk_adjustment_set_value(GTK_ADJUSTMENT(ptw->aapwRates[i][j]), aarRates[i][j] * 100.0f);

    }


}


static void
TheoryGetValues(theorywidget * ptw, cubeinfo * pci, float aarRates[2][2])
{

    int i, j;

    /* get rates */

    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++)
            aarRates[i][j] = 0.01f * (float) gtk_adjustment_get_value(ptw->aapwRates[i][j]);


    /* money game or match play */

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ptw->apwRadio[0]))) {

        /* match play */

        int n0 = (int) gtk_adjustment_get_value(ptw->apwScoreAway[0]);
        int n1 = (int) gtk_adjustment_get_value(ptw->apwScoreAway[1]);

        pci->nMatchTo = (n1 > n0) ? n1 : n0;

        pci->anScore[0] = pci->nMatchTo - n0;
        pci->anScore[1] = pci->nMatchTo - n1;

        pci->fCrawford = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ptw->pwCrawford));

        j = 1;
        for (i = 0; i < 7; i++) {

            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ptw->apwCube[i]))) {

                pci->nCube = j;
                break;

            }

            j *= 2;

        }


    } else {

        /* money play */

        pci->nMatchTo = 0;

        pci->anScore[0] = pci->anScore[1] = 0;

        pci->fJacoby = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ptw->pwJacoby));
        pci->fBeavers = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ptw->pwBeavers));

    }

}


static void
do_mw_views(theorywidget * ptw)
{
    int i;
    GtkListStore *store;
    GtkCellRenderer *renderer;

    for (i = 0; i < 2; i++) {
        store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        ptw->apwMW[i] = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
        g_object_unref(store);
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ptw->apwMW[i]), -1, "", renderer, "text", 0, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ptw->apwMW[i]), -1, _("Dead cube"), renderer,
                                                    "text", 1, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ptw->apwMW[i]), -1, _("Fully live"), renderer,
                                                    "text", 2, NULL);
    }
}

static void
remove_mw_rows(theorywidget * ptw)
{
    int i;
    GtkTreeModel *model;
    GtkTreeIter iter;
    for (i = 0; i < 2; ++i) {
        model = gtk_tree_view_get_model(GTK_TREE_VIEW(ptw->apwMW[i]));
        if (gtk_tree_model_iter_nth_child(model, &iter, NULL, 0))
            while (gtk_list_store_remove(GTK_LIST_STORE(model), &iter)) {
            };
    }
}

static void
add_mw_money_rows(theorywidget * ptw, const cubeinfo * pci, float aarRates[2][2])
{
    int i, j, k;
    gchar *asz[3];
    float aaarPoints[2][7][2];
    GtkListStore *store;
    GtkTreeIter iter;
    const char *aszMoneyPointLabel[] = {
        N_("Take Point (TP)"),
        N_("Beaver Point (BP)"),
        N_("Raccoon Point (RP)"),
        N_("Initial Double Point (IDP)"),
        N_("Redouble Point (RDP)"),
        N_("Cash Point (CP)"),
        N_("Too Good point (TG)")
    };

    /* money play */
    getMoneyPoints(aaarPoints, pci->fJacoby, pci->fBeavers, aarRates);
    for (i = 0; i < 2; ++i) {
        store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ptw->apwMW[i])));
        for (j = 0; j < 7; j++) {
            asz[0] = g_strdup(gettext(aszMoneyPointLabel[j]));
            for (k = 0; k < 2; k++)
                asz[k + 1] = g_strdup_printf("%7.3f%%", 100.0f * aaarPoints[i][j][k]);
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, asz[0], 1, asz[1], 2, asz[2], -1);
            for (k = 0; k < 3; ++k)
                g_free(asz[k]);
        }
        ptw->aar[i][0] = aaarPoints[i][3][0];
        ptw->aar[i][1] = aaarPoints[i][5][1];
        ptw->aar[i][2] = aaarPoints[i][6][0];
    }
}

static void
add_mw_match_rows(theorywidget * ptw, const cubeinfo * pci, float aarRates[2][2])
{
    int i, j, k;
    gchar *asz[3];
    float aaarPointsMatch[2][4][2];
    int afAutoRedouble[2];
    int afDead[2];
    GtkListStore *store;
    GtkTreeIter iter;
    const char *aszMatchPlayLabel[] = {
        N_("Take Point (TP)"),
        N_("Double point (DP)"),
        N_("Cash Point (CP)"),
        N_("Too Good point (TG)")
    };


    for (i = 0; i < 2; i++) {
        store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ptw->apwMW[i])));
        getMatchPoints(aaarPointsMatch, afAutoRedouble, afDead, pci, aarRates);
        for (j = 0; j < 4; j++) {
            asz[0] = g_strdup(gettext(aszMatchPlayLabel[j]));
            for (k = 0; k < 2; k++) {
                int f = ((!k) || (!afDead[i])) &&
                    !(k && afAutoRedouble[i] && !j) && !(k && afAutoRedouble[i] && j == 3);
                f = f || (k && afAutoRedouble[!i] && !j);
                f = f && (!pci->nMatchTo || (pci->anScore[i] + pci->nCube < pci->nMatchTo));
                if (f)
                    asz[1 + k] = g_strdup_printf("%7.3f%%", 100.0f * aaarPointsMatch[i][j][k]);
                else
                    asz[1 + k] = g_strdup("");
            }
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, asz[0], 1, asz[1], 2, asz[2], -1);
            for (k = 0; k < 3; ++k)
                g_free(asz[k]);
        }
        ptw->aar[i][0] = aaarPointsMatch[i][1][0];
        ptw->aar[i][1] = aaarPointsMatch[i][2][1];
        ptw->aar[i][2] = aaarPointsMatch[i][3][0];
    }
}

static void
TheoryUpdated(GtkWidget * UNUSED(pw), theorywidget * ptw)
{
    cubeinfo ci;
    float aarRates[2][2];

    int i, j;
    gchar *pch;
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    /* get values */

    TheoryGetValues(ptw, &ci, aarRates);

    /* set max on the gammon spinners */

    for (i = 0; i < 2; ++i)
        gtk_adjustment_set_upper(ptw->aapwRates[i][1], 100.0f - gtk_adjustment_get_value(ptw->aapwRates[i][0]));

    SetCubeInfo(&ci, ci.nCube, 0, 0, ci.nMatchTo, ci.anScore, ci.fCrawford, ci.fJacoby, ci.fBeavers, ci.bgv);

    /* hide show widgets */

    gtk_widget_show(ptw->apwFrame[0]);
    gtk_widget_show(ptw->apwFrame[1]);
    gtk_widget_hide(ptw->apwFrame[ci.nMatchTo != 0]);

    /* update match play widget */

    gtk_widget_set_sensitive(ptw->pwCrawford,
                             ((ci.anScore[0] == ci.nMatchTo - 1) ^ (ci.anScore[1] == ci.nMatchTo - 1)));


    if (ci.nMatchTo) {

        gtk_widget_set_sensitive(ptw->pwCubeFrame, ci.nMatchTo != 1);

        j = 1;
        for (i = 0; i < 7; i++) {

            gtk_widget_set_sensitive(ptw->apwCube[i], j < 2 * ci.nMatchTo);

            j *= 2;

        }

    }


    /* update money play widget */


    /* 
     * update market window widgets 
     */

    remove_mw_rows(ptw);
    if (ci.nMatchTo)
        add_mw_match_rows(ptw, &ci, aarRates);
    else
        add_mw_money_rows(ptw, &ci, aarRates);

    for (i = 0; i < 2; i++)
        gtk_widget_queue_draw(ptw->apwGraph[i]);
    /*
     * Update gammon price widgets
     */

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ptw->pwGammonPrice));
    gtk_text_buffer_set_text(buffer, "", -1);
    gtk_text_buffer_get_end_iter(buffer, &iter);

    if (ci.nMatchTo) {

        pch = g_strdup_printf(_("Gammon values at %d-away, %d-away (%d cube)\n\n"),
                              ci.nMatchTo - ci.anScore[0], ci.nMatchTo - ci.anScore[1], ci.nCube);

        gtk_text_buffer_insert(buffer, &iter, pch, -1);

        g_free(pch);

        gtk_text_buffer_insert(buffer, &iter, _("Player                          Gammon   BG\n"), -1);


        for (j = 0; j < 2; ++j) {

            pch = g_strdup_printf("%-31.31s %6.4f   %6.4f\n",
                                  ap[j].szName,
                                  0.5f * ci.arGammonPrice[j], 0.5f * (ci.arGammonPrice[2 + j] + ci.arGammonPrice[j]));

            gtk_text_buffer_insert(buffer, &iter, pch, -1);

            g_free(pch);

        }

    } else {

        gtk_text_buffer_insert(buffer, &iter, _("Gammon values for money game:\n\n"), -1);

        for (i = 0; i < 2; ++i) {

            pch = g_strdup_printf(_("%s:\n\n"
                                    "Player                          "
                                    "Gammon   BG\n"), i ? _("Doubled cube") : _("Centered cube"));

            gtk_text_buffer_insert(buffer, &iter, pch, -1);

            g_free(pch);


            SetCubeInfo(&ci, 1, i ? 1 : -1, 0, ci.nMatchTo, ci.anScore, ci.fCrawford, ci.fJacoby, ci.fBeavers, ci.bgv);

            for (j = 0; j < 2; ++j) {

                pch = g_strdup_printf("%-31.31s %6.4f   %6.4f\n",
                                      ap[j].szName,
                                      0.5f * ci.arGammonPrice[j],
                                      0.5f * (ci.arGammonPrice[2 + j] + ci.arGammonPrice[j]));

                gtk_text_buffer_insert(buffer, &iter, pch, -1);

                g_free(pch);

            }

            gtk_text_buffer_insert(buffer, &iter, "\n", -1);

        }

    }

}

static void
GraphExpose(GtkWidget * pwGraph, GdkEventExpose * UNUSED(pev), theorywidget * ptw)
{

    GtkAllocation allocation;
    int i, x, y, cx, cy, iPlayer, ax[3];
    char sz[4];
    PangoLayout *layout;

    gtk_widget_get_allocation(pwGraph, &allocation);
    x = 8;
    y = 12;
    cx = allocation.width - 16 - 1;
    cy = allocation.height - 12;
    layout = gtk_widget_create_pango_layout(pwGraph, NULL);

    pango_layout_set_font_description(layout, pango_font_description_from_string("sans 7"));

    /* FIXME: The co-ordinates used in this function should be determined
     * from the text size, not hard-coded.  But GDK's text handling will
     * undergo an overhaul with Pango once GTK+ 2.0 comes out, so let's
     * cheat for now and then get it right once 2.0 is here. */

    iPlayer = pwGraph == ptw->apwGraph[1];

    for (i = 0; i <= 20; i++) {
        gtk_paint_vline(gtk_widget_get_style(pwGraph), gtk_widget_get_window(pwGraph), GTK_STATE_NORMAL,
                        NULL, pwGraph, "tick", y - 1, i & 3 ? y - 3 : y - 5, x + cx * i / 20);

        if (!(i & 3)) {
            int width;
            int height;
            sprintf(sz, "%d", i * 5);
            pango_layout_set_text(layout, sz, -1);
            pango_layout_get_pixel_size(layout, &width, &height);
            gtk_paint_layout(gtk_widget_get_style(pwGraph), gtk_widget_get_window(pwGraph),
                             GTK_STATE_NORMAL, TRUE, NULL, pwGraph, "label",
                             x + cx * i / 20 - width / 2 /* FIXME */ , y - height - 1, layout);
        }
    }
    g_object_unref(layout);

    for (i = 0; i < 3; i++)
        ax[i] = (int) (x + cx * ptw->aar[iPlayer][i]);

    gtk_paint_box(gtk_widget_get_style(pwGraph), gtk_widget_get_window(pwGraph), GTK_STATE_NORMAL,
                  GTK_SHADOW_IN, NULL, pwGraph, "doubling-window", x, 12, cx, cy);

    /* FIXME it's horrible to abuse the "state" parameters like this */
    if (ptw->aar[iPlayer][1] > ptw->aar[iPlayer][0])
        gtk_paint_box(gtk_widget_get_style(pwGraph), gtk_widget_get_window(pwGraph),
                      GTK_STATE_ACTIVE, GTK_SHADOW_OUT, NULL, pwGraph, "take", ax[0], 13, ax[1] - ax[0], cy - 2);

    if (ptw->aar[iPlayer][2] > ptw->aar[iPlayer][1])
        gtk_paint_box(gtk_widget_get_style(pwGraph), gtk_widget_get_window(pwGraph),
                      GTK_STATE_PRELIGHT, GTK_SHADOW_OUT, NULL, pwGraph, "drop", ax[1], 13, ax[2] - ax[1], cy - 2);

    if (ptw->aar[iPlayer][2] < 1.0)
        gtk_paint_box(gtk_widget_get_style(pwGraph), gtk_widget_get_window(pwGraph),
                      GTK_STATE_SELECTED, GTK_SHADOW_OUT, NULL, pwGraph, "too-good", ax[2], 13, x + cx - ax[2], cy - 2);
}


static void
PlyClicked(GtkWidget * pw, theorywidget * ptw)
{

    int *pi = (int *) g_object_get_data(G_OBJECT(pw), "ply");
    int f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));
    cubeinfo ci;
    decisionData dd;
    evalcontext ec = { FALSE, 0, FALSE, TRUE, 0.0 };
    int i, j;

    if (!f)
        return;

    GetMatchStateCubeInfo(&ci, &ms);
    TheoryGetValues(ptw, &ci, dd.aarRates);

    ec.nPlies = *pi;

    dd.pboard = msBoard();
    dd.pci = &ci;
    dd.pec = &ec;

    if (RunAsyncProcess((AsyncFun) asyncGammonRates, &dd, _("Evaluating gammon percentages")) != 0) {
        fInterrupt = FALSE;
        return;
    }

    for (i = 0; i < 2; ++i)
        for (j = 0; j < 2; ++j)
            gtk_adjustment_set_value(GTK_ADJUSTMENT(ptw->aapwRates[i][j]), dd.aarRates[i][j] * 100.0f);

    TheoryUpdated(NULL, ptw);

}


/*
 * Display widget with misc. theory:
 * - market windows
 * - gammon price
 *
 * Input:
 *   fActivePage: with notebook page should recieve focus.
 *
 */

extern void
GTKShowTheory(const int fActivePage)
{

    GtkWidget *pwDialog, *pwNotebook;

    GtkWidget *pwOuterHBox, *pwVBox, *pwHBox;
    GtkWidget *pwFrame, *pwTable, *pwAlign;

    GtkWidget *pw, *pwx, *pwz;

    int i, j;
    char sz[256];
    int *pi;
    PangoFontDescription *font_desc;

    theorywidget *ptw;

    ptw = malloc(sizeof(theorywidget));

    /* create dialog */

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Theory"), DT_INFO,
                               NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_NOTIDY, NULL, NULL);

    gtk_window_set_default_size(GTK_WINDOW(pwDialog), 660, 300);

    pwOuterHBox = gtk_hbox_new(FALSE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(pwOuterHBox), 8);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwOuterHBox);

    pwVBox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwOuterHBox), pwVBox, FALSE, FALSE, 0);

    /* match/money play */

    pwHBox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVBox), pwHBox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(pwHBox), ptw->apwRadio[0] = gtk_radio_button_new_with_label(NULL, _("Match play")));
    gtk_container_add(GTK_CONTAINER(pwHBox),
                      ptw->apwRadio[1] =
                      gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ptw->apwRadio[0]), _("Money game")));

    g_signal_connect(G_OBJECT(ptw->apwRadio[0]), "toggled", G_CALLBACK(TheoryUpdated), ptw);
    g_signal_connect(G_OBJECT(ptw->apwRadio[1]), "toggled", G_CALLBACK(TheoryUpdated), ptw);


    gtk_container_add(GTK_CONTAINER(pwHBox), ptw->pwReset = gtk_button_new_with_label(_("Reset")));
    g_signal_connect(G_OBJECT(ptw->pwReset), "clicked", G_CALLBACK(ResetTheory), ptw);

    /* match score widget */

    ptw->apwFrame[0] = gtk_frame_new(_("Match score"));
    gtk_box_pack_start(GTK_BOX(pwVBox), ptw->apwFrame[0], FALSE, FALSE, 0);

    pw = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ptw->apwFrame[0]), pw);

    pwTable = gtk_table_new(2, 3, FALSE);
    gtk_container_add(GTK_CONTAINER(pw), pwTable);

    for (i = 0; i < 2; i++) {

        gtk_table_attach(GTK_TABLE(pwTable),
                         pwx = gtk_label_new(ap[i].szName),
                         0, 1, 0 + i, 1 + i, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0);
        gtk_misc_set_alignment(GTK_MISC(pwx), 0, 0.5);


        ptw->apwScoreAway[i] = GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 64, 1, 5, 0));

        gtk_table_attach(GTK_TABLE(pwTable),
                         gtk_spin_button_new(ptw->apwScoreAway[i],
                                             1, 0),
                         1, 2, 0 + i, 1 + i, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0);

        gtk_table_attach(GTK_TABLE(pwTable),
                         pwx = gtk_label_new(_("-away")),
                         2, 3, 0 + i, 1 + i, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0);
        gtk_misc_set_alignment(GTK_MISC(pwx), 0, 0.5);

        g_signal_connect(G_OBJECT(ptw->apwScoreAway[i]), "value-changed", G_CALLBACK(TheoryUpdated), ptw);

    }

    pwHBox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pw), pwHBox);

    gtk_container_add(GTK_CONTAINER(pwHBox), ptw->pwCrawford = gtk_check_button_new_with_label(_("Crawford game")));

    g_signal_connect(G_OBJECT(ptw->pwCrawford), "toggled", G_CALLBACK(TheoryUpdated), ptw);

    ptw->pwCubeFrame = gtk_frame_new(_("Cube"));
    gtk_container_add(GTK_CONTAINER(pw), ptw->pwCubeFrame);

    pwHBox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ptw->pwCubeFrame), pwHBox);

    j = 1;
    for (i = 0; i < 7; i++) {

        sprintf(sz, "%d", j);

        if (!i)
            gtk_container_add(GTK_CONTAINER(pwHBox), ptw->apwCube[i] = gtk_radio_button_new_with_label(NULL, sz));
        else
            gtk_container_add(GTK_CONTAINER(pwHBox),
                              ptw->apwCube[i] =
                              gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ptw->apwCube[0]), sz));

        g_signal_connect(G_OBJECT(ptw->apwCube[i]), "toggled", G_CALLBACK(TheoryUpdated), ptw);

        j *= 2;

    }

    /* match equity table */

    pwFrame = gtk_frame_new(_("Match equity table"));
    gtk_container_add(GTK_CONTAINER(pw), pwFrame);

    pwx = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwx);

    gtk_box_pack_start(GTK_BOX(pwx), pwz = gtk_label_new((char *) miCurrent.szName), FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(pwz), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(pwx), pwz = gtk_label_new((char *) miCurrent.szFileName), FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(pwz), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(pwx), pwz = gtk_label_new((char *) miCurrent.szDescription), FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(pwz), 0, 0.5);


    /* money play widget */

    ptw->apwFrame[1] = gtk_frame_new(_("Money play"));
    gtk_box_pack_start(GTK_BOX(pwVBox), ptw->apwFrame[1], FALSE, FALSE, 0);

    pwHBox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ptw->apwFrame[1]), pwHBox);

    gtk_container_add(GTK_CONTAINER(pwHBox), ptw->pwJacoby = gtk_check_button_new_with_label(_("Jacoby rule")));

    g_signal_connect(G_OBJECT(ptw->pwJacoby), "toggled", G_CALLBACK(TheoryUpdated), ptw);


    gtk_container_add(GTK_CONTAINER(pwHBox), ptw->pwBeavers = gtk_check_button_new_with_label(_("Beavers allowed")));

    g_signal_connect(G_OBJECT(ptw->pwBeavers), "toggled", G_CALLBACK(TheoryUpdated), ptw);

    /* gammon and backgammon percentages */

    pwFrame = gtk_frame_new(_("Gammon and backgammon percentages"));
    gtk_box_pack_start(GTK_BOX(pwVBox), pwFrame, FALSE, FALSE, 0);

    pwx = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwx);


    pwTable = gtk_table_new(3, 3, TRUE);
    gtk_box_pack_start(GTK_BOX(pwx), pwTable, FALSE, FALSE, 4);

    for (i = 0; i < 2; i++) {

        /* player name */

        gtk_table_attach(GTK_TABLE(pwTable),
                         pw = gtk_label_new(ap[i].szName),
                         0, 1, 1 + i, 2 + i, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0);
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

        /* column title */

        gtk_table_attach(GTK_TABLE(pwTable),
                         pw = gtk_label_new(i ? _("bg rate(%)") :
                                            _("gammon rate(%)")),
                         1 + i, 2 + i, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0);
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

        for (j = 0; j < 2; j++) {

            ptw->aapwRates[i][j] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 100.0, 0.01, 1.0, 0));

            gtk_table_attach(GTK_TABLE(pwTable),
                             gtk_spin_button_new(ptw->aapwRates[i][j],
                                                 0.01, 2),
                             j + 1, j + 2, i + 1, i + 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0);

            g_signal_connect(G_OBJECT(ptw->aapwRates[i][j]), "value-changed", G_CALLBACK(TheoryUpdated), ptw);

        }

    }

    /* radio buttons with plies */

    pwz = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(pwx), pwz, FALSE, FALSE, 4);

    for (i = 0; i < MAXPLY; ++i) {

        gchar *sz = g_strdup_printf(_("%d ply"), i);
        if (!i)
            ptw->apwPly[i] = gtk_radio_button_new_with_label(NULL, sz);
        else
            ptw->apwPly[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(ptw->apwPly[0]), sz);
        g_free(sz);

        pi = (int *) g_malloc(sizeof(int));
        *pi = i;

        g_object_set_data_full(G_OBJECT(ptw->apwPly[i]), "ply", pi, g_free);

        gtk_box_pack_start(GTK_BOX(pwz), ptw->apwPly[i], FALSE, FALSE, 4);

        g_signal_connect(G_OBJECT(ptw->apwPly[i]), "toggled", G_CALLBACK(PlyClicked), ptw);

    }

    /* add notebook pages */

    pwNotebook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(pwNotebook), 0);

    gtk_box_pack_start(GTK_BOX(pwOuterHBox), pwNotebook, TRUE, TRUE, 0);

    /* market window */

    pwVBox = gtk_vbox_new(FALSE, 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), pwVBox, gtk_label_new(_("Market window")));

    do_mw_views(ptw);

    for (i = 0; i < 2; ++i) {

        sprintf(sz, _("Market window for player %s"), ap[i].szName);
        pwFrame = gtk_frame_new(sz);
        gtk_box_pack_start(GTK_BOX(pwVBox), pwFrame, FALSE, FALSE, 0);


        gtk_container_add(GTK_CONTAINER(pwFrame), ptw->apwMW[i]);

    }

    /* window graph */
    pwVBox = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), pwVBox, gtk_label_new(_("Window graph")));

    for (i = 0; i < 2; i++) {
        sprintf(sz, _("Window graph for player %s"), ap[i].szName);
        pwFrame = gtk_frame_new(sz);
        gtk_box_pack_start(GTK_BOX(pwVBox), pwFrame, FALSE, FALSE, 4);

        gtk_container_add(GTK_CONTAINER(pwFrame), pwAlign = gtk_alignment_new(0.5, 0.5, 1, 0));
        gtk_container_add(GTK_CONTAINER(pwAlign), ptw->apwGraph[i] = gtk_drawing_area_new());
        gtk_widget_set_name(ptw->apwGraph[i], "gnubg-doubling-window-graph");
        gtk_container_set_border_width(GTK_CONTAINER(pwAlign), 4);
        gtk_widget_set_size_request(ptw->apwGraph[i], -1, 48);
        g_signal_connect(G_OBJECT(ptw->apwGraph[i]), "expose_event", G_CALLBACK(GraphExpose), ptw);
    }

    /* gammon prices */

    pwVBox = gtk_vbox_new(FALSE, 0);

    ptw->pwGammonPrice = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(ptw->pwGammonPrice), FALSE);

    font_desc = pango_font_description_from_string("Monospace");
    gtk_widget_modify_font(ptw->pwGammonPrice, font_desc);
    pango_font_description_free(font_desc);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(ptw->pwGammonPrice), GTK_WRAP_NONE);

    gtk_container_add(GTK_CONTAINER(pwVBox), ptw->pwGammonPrice);

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), pwVBox, gtk_label_new(_("Gammon values")));

    /* show dialog */

    ResetTheory(NULL, ptw);
    gtk_widget_show_all(pwDialog);
    TheoryUpdated(NULL, ptw);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(pwNotebook), fActivePage ? 2 /* prices */ : 0 /* market */ );

}
