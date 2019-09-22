/*
 * movefilter.c
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
 * $Id: gtkmovefilter.c,v 1.33 2013/06/16 02:16:15 mdpetch Exp $
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gtkgame.h"
#include "gtkmovefilter.h"
#include "gtkwindows.h"
#include "gtklocdefs.h"

typedef struct _movefilterwidget {


    movefilter *pmf;

    /* the menu with default settings */

    GtkWidget *pwOptionMenu;

    /* callback for the parent */

    GCallback pfChanged;
    gpointer userdata;

} movefilterwidget;


typedef struct _movefiltersetupwidget {

    int *pfOK;
    movefilter *pmf;

    GtkAdjustment *aapadjAccept[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    GtkAdjustment *aapadjExtra[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    GtkAdjustment *aapadjThreshold[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    GtkWidget *aapwA[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    GtkWidget *aapwET[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    GtkWidget *aapwT[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    GtkWidget *aapwEnable[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    GtkWidget *pwOptionMenu;

} movefiltersetupwidget;


static void
MoveFilterSetupSetValues(movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES], const movefiltersetupwidget * pmfsw)
{

    int i, j;
    int f;

    for (i = 0; i < MAX_FILTER_PLIES; ++i)
        for (j = 0; j <= i; ++j) {

            gtk_adjustment_set_value(pmfsw->aapadjAccept[i][j], aamf[i][j].Accept >= 0 ? aamf[i][j].Accept : 0);
            gtk_adjustment_set_value(pmfsw->aapadjExtra[i][j], aamf[i][j].Accept >= 0 ? aamf[i][j].Extra : 0);
            gtk_adjustment_set_value(pmfsw->aapadjThreshold[i][j],
                                     (aamf[i][j].Accept >= 0 && aamf[i][j].Extra) ? aamf[i][j].Threshold : 0);

            f = aamf[i][j].Accept >= 0;
            gtk_widget_set_sensitive(GTK_WIDGET(pmfsw->aapwA[i][j]), f);
            gtk_widget_set_sensitive(GTK_WIDGET(pmfsw->aapwET[i][j]), f);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pmfsw->aapwEnable[i][j]), f);
            f = f && aamf[i][j].Extra;
            gtk_widget_set_sensitive(GTK_WIDGET(pmfsw->aapwT[i][j]), f);


        }

}



static void
MoveFilterSetupGetValues(movefilter * pmf, const movefiltersetupwidget * pmfsw)
{

    int i, j;
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    int f;

    memset(aamf, 0, sizeof(aamf));

    for (i = 0; i < MAX_FILTER_PLIES; ++i)
        for (j = 0; j <= i; ++j) {

            f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pmfsw->aapwEnable[i][j]));

            aamf[i][j].Accept = f ? (int) gtk_adjustment_get_value(pmfsw->aapadjAccept[i][j]) : -1;
            aamf[i][j].Extra = (aamf[i][j].Accept >= 0) ? (int) gtk_adjustment_get_value(pmfsw->aapadjExtra[i][j]) : 0;
            aamf[i][j].Threshold = (aamf[i][j].Extra) ?
                (float) gtk_adjustment_get_value(pmfsw->aapadjThreshold[i][j]) : 0;

        }

    memcpy(pmf, aamf, sizeof(aamf));

}


static void
AcceptChanged(GtkWidget * UNUSED(pw), movefiltersetupwidget * pmfsw)
{

    int fFound;
    unsigned int i;
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];

    /* see if current settings match a predefined one */

    fFound = FALSE;

    MoveFilterSetupGetValues(aamf[0], pmfsw);

    for (i = 0; i < NUM_MOVEFILTER_SETTINGS; ++i)
        if (equal_movefilters(aamf, aaamfMoveFilterSettings[i])) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(pmfsw->pwOptionMenu), i);
            fFound = TRUE;
            break;
        }

    if (!fFound)
        gtk_combo_box_set_active(GTK_COMBO_BOX(pmfsw->pwOptionMenu), NUM_MOVEFILTER_SETTINGS);
}


static void
EnableToggled(GtkWidget * pw, movefiltersetupwidget * pmfsw)
{
    int f;
    int i, j;

    for (i = 0; i < MAX_FILTER_PLIES; ++i)
        for (j = 0; j <= i; ++j) {

            f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pmfsw->aapwEnable[i][j]));

            gtk_widget_set_sensitive(GTK_WIDGET(pmfsw->aapwA[i][j]), f);
            gtk_widget_set_sensitive(GTK_WIDGET(pmfsw->aapwET[i][j]), f);
            gtk_widget_set_sensitive(GTK_WIDGET(pmfsw->aapwT[i][j]), f);
        }

    AcceptChanged(pw, pmfsw);
}




static void
SetupSettingsMenuActivate(GtkWidget * combo, const movefiltersetupwidget * pfmsw)
{

    int iSelected = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

    if (iSelected == NUM_MOVEFILTER_SETTINGS)
        return;                 /* user defined */

    MoveFilterSetupSetValues(aaamfMoveFilterSettings[iSelected], pfmsw);

}




static GtkWidget *
MoveFilterPage(const int i, const int j,
               movefilter UNUSED(aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]), movefiltersetupwidget * pmfsw)
{

    GtkWidget *pwPage;
    GtkWidget *pwhbox;
    GtkWidget *pw;

    pwPage = gtk_vbox_new(FALSE, 0);

    /* enable */

    pmfsw->aapwEnable[i][j] = gtk_check_button_new_with_label(_("Enable this level"));
    gtk_box_pack_start(GTK_BOX(pwPage), pmfsw->aapwEnable[i][j], FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(pmfsw->aapwEnable[i][j]), "toggled", G_CALLBACK(EnableToggled), pmfsw);

    /* accept */

    pwhbox = gtk_hbox_new(FALSE, 0);
    pmfsw->aapwA[i][j] = pwhbox;
    gtk_box_pack_start(GTK_BOX(pwPage), pwhbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pwhbox), gtk_label_new(_("Always accept: ")), FALSE, FALSE, 0);
    pmfsw->aapadjAccept[i][j] = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1000, 1, 5, 0));

    pw = gtk_spin_button_new(GTK_ADJUSTMENT(pmfsw->aapadjAccept[i][j]), 1, 0);

    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(pw), TRUE);

    gtk_box_pack_start(GTK_BOX(pwhbox), pw, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pwhbox), gtk_label_new(_("moves.")), FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(pmfsw->aapadjAccept[i][j]), "value-changed", G_CALLBACK(AcceptChanged), pmfsw);

    /* extra */

    pwhbox = gtk_hbox_new(FALSE, 0);
    pmfsw->aapwET[i][j] = pwhbox;
    gtk_box_pack_start(GTK_BOX(pwPage), pwhbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pwhbox), gtk_label_new(_("Add extra: ")), FALSE, FALSE, 0);

    pmfsw->aapadjExtra[i][j] = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1000, 1, 5, 0));

    pw = gtk_spin_button_new(GTK_ADJUSTMENT(pmfsw->aapadjExtra[i][j]), 1, 0);

    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(pw), TRUE);

    gtk_box_pack_start(GTK_BOX(pwhbox), pw, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(pmfsw->aapadjExtra[i][j]), "value-changed", G_CALLBACK(AcceptChanged), pmfsw);

    /* threshold */

    gtk_box_pack_start(GTK_BOX(pwhbox), gtk_label_new(_("moves within")), FALSE, FALSE, 0);

    pmfsw->aapadjThreshold[i][j] = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 10, 0.001, 0.1, 0));

    pw = gtk_spin_button_new(GTK_ADJUSTMENT(pmfsw->aapadjThreshold[i][j]), 1, 3);
    pmfsw->aapwT[i][j] = pw;

    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(pw), TRUE);

    gtk_box_pack_start(GTK_BOX(pwhbox), pw, TRUE, TRUE, 0);

    g_signal_connect(G_OBJECT(pmfsw->aapadjThreshold[i][j]), "value-changed", G_CALLBACK(AcceptChanged), pmfsw);



    return pwPage;

}




static GtkWidget *
MoveFilterSetup(movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES], int *pfOK)
{

    GtkWidget *pwSetup;
    GtkWidget *pwFrame;
    int i, j;
    movefiltersetupwidget *pmfsw;
    GtkWidget *pwNotebook;
    GtkWidget *pwvbox;

    pwSetup = gtk_vbox_new(FALSE, 4);

    pmfsw = (movefiltersetupwidget *) g_malloc(sizeof(movefiltersetupwidget));

    /* predefined settings */

    /* output widget (with "User defined", or "Large" etc */

    pwFrame = gtk_frame_new(_("Predefined move filters:"));
    gtk_box_pack_start(GTK_BOX(pwSetup), pwFrame, TRUE, TRUE, 0);
    pmfsw->pwOptionMenu = gtk_combo_box_text_new();
    for (i = 0; i <= NUM_MOVEFILTER_SETTINGS; i++) {

        if (i < NUM_MOVEFILTER_SETTINGS)
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pmfsw->pwOptionMenu), Q_(aszMoveFilterSettings[i]));
        else
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pmfsw->pwOptionMenu), _("user defined"));
    }

    g_signal_connect(G_OBJECT(pmfsw->pwOptionMenu), "changed", G_CALLBACK(SetupSettingsMenuActivate), pmfsw);

    gtk_container_add(GTK_CONTAINER(pwFrame), pmfsw->pwOptionMenu);

    /* notebook with pages for each ply */

    pwNotebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(pwSetup), pwNotebook, FALSE, FALSE, 0);

    for (i = 0; i < MAX_FILTER_PLIES; ++i) {

        char *sz = g_strdup_printf(_("%d-ply"), i + 1);

        pwvbox = gtk_vbox_new(FALSE, 4);
        gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), pwvbox, gtk_label_new(sz));
        g_free(sz);

        for (j = 0; j <= i; ++j) {

            sz = g_strdup_printf(_("%d-ply"), j);
            pwFrame = gtk_frame_new(sz);
            g_free(sz);

            gtk_box_pack_start(GTK_BOX(pwvbox), pwFrame, FALSE, FALSE, 4);

            gtk_container_add(GTK_CONTAINER(pwFrame), MoveFilterPage(i, j, aamf, pmfsw));

        }

    }

    g_object_set_data_full(G_OBJECT(pwSetup), "user_data", pmfsw, g_free);

    pmfsw->pfOK = pfOK;
    pmfsw->pmf = aamf[0];

    MoveFilterSetupSetValues(aamf, pmfsw);

    return pwSetup;

}

static void
MoveFilterSetupOK(GtkWidget * pw, GtkWidget * pwMoveFilterSetup)
{

    movefiltersetupwidget *pmfsw = (movefiltersetupwidget *)
        g_object_get_data(G_OBJECT(pwMoveFilterSetup), "user_data");


    if (pmfsw->pfOK)
        *pmfsw->pfOK = TRUE;

    MoveFilterSetupGetValues(pmfsw->pmf, pmfsw);

    if (pmfsw->pfOK)
        gtk_widget_destroy(gtk_widget_get_toplevel(pw));

}


typedef void (*changed) (GtkWidget * pw, gpointer p);

static void
MoveFilterChanged(const movefilterwidget * pmfw)
{

    unsigned int i;
    int fFound = FALSE;
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];

    memcpy(aamf, pmfw->pmf, sizeof(aamf));

    for (i = 0; i < NUM_MOVEFILTER_SETTINGS; ++i)
        if (equal_movefilters(aamf, aaamfMoveFilterSettings[i])) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(pmfw->pwOptionMenu), i);
            fFound = TRUE;
            break;
        }

    if (!fFound)
        gtk_combo_box_set_active(GTK_COMBO_BOX(pmfw->pwOptionMenu), NUM_MOVEFILTER_SETTINGS);

    /* callback for parent */

    if (pmfw->pfChanged)
        ((changed) (*pmfw->pfChanged)) (NULL, pmfw->userdata);

}


static void
SettingsMenuActivate(GtkWidget * combo, movefilterwidget * pmfw)
{

    int iSelected = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

    if (iSelected == NUM_MOVEFILTER_SETTINGS)
        return;                 /* user defined */

    memcpy(pmfw->pmf, aaamfMoveFilterSettings[iSelected], MAX_FILTER_PLIES * MAX_FILTER_PLIES * sizeof(movefilter));

    MoveFilterChanged(pmfw);

}


static void
ModifyClickButton(GtkWidget * pw, movefilterwidget * pmfw)
{

    int fOK;
    GtkWidget *pwDialog;
    GtkWidget *pwMoveFilterSetup;
    movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES];

    memcpy(aamf, pmfw->pmf, sizeof(aamf));
    pwMoveFilterSetup = MoveFilterSetup(aamf, &fOK);

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Move filter setup"),
                               DT_QUESTION, pw, DIALOG_FLAG_MODAL, G_CALLBACK(MoveFilterSetupOK), pwMoveFilterSetup);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwMoveFilterSetup);

    GTKRunDialog(pwDialog);

    if (fOK) {
        memcpy(pmfw->pmf, aamf, sizeof(aamf));
        MoveFilterChanged(pmfw);
    }

}


extern GtkWidget *
MoveFilterWidget(movefilter * pmf, int *UNUSED(pfOK), GCallback pfChanged, gpointer userdata)
{

    GtkWidget *pwFrame;
    movefilterwidget *pmfw;
    GtkWidget *pw;
    GtkWidget *pwButton;
    int i;

    pwFrame = gtk_frame_new(_("Move filter"));
    pmfw = (movefilterwidget *) g_malloc(sizeof(movefilterwidget));
    pmfw->pmf = pmf;
    pmfw->userdata = userdata;
    pmfw->pfChanged = NULL;     /* UGLY; see comment later */

    /* output widget (with "User defined", or "Large" etc */

    pw = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(pwFrame), pw);
    pmfw->pwOptionMenu = gtk_combo_box_text_new();
    for (i = 0; i <= NUM_MOVEFILTER_SETTINGS; i++) {

        if (i < NUM_MOVEFILTER_SETTINGS)
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pmfw->pwOptionMenu), Q_(aszMoveFilterSettings[i]));
        else
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pmfw->pwOptionMenu), _("user defined"));

    }
    g_signal_connect(G_OBJECT(pmfw->pwOptionMenu), "changed", G_CALLBACK(SettingsMenuActivate), pmfw);

    gtk_box_pack_start(GTK_BOX(pw), pmfw->pwOptionMenu, TRUE, TRUE, 0);

    /* Button */

    pwButton = gtk_button_new_with_label(_("Modify..."));
    gtk_box_pack_end(GTK_BOX(pw), pwButton, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(pwButton), "clicked", G_CALLBACK(ModifyClickButton), pmfw);

    /* save movefilterwidget */

    g_object_set_data_full(G_OBJECT(pwFrame), "user_data", pmfw, g_free);

    MoveFilterChanged(pmfw);

    /* don't set pfChanged until here, as we don't want to call EvalChanged
     * just yet. This is a big ugly... */
    pmfw->pfChanged = pfChanged;

    return pwFrame;

}

extern void
SetMovefilterCommands(const char *sz,
                      movefilter aamfNew[MAX_FILTER_PLIES][MAX_FILTER_PLIES],
                      movefilter aamfOld[MAX_FILTER_PLIES][MAX_FILTER_PLIES])
{

    int i, j;
    char *szCmd;
    gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

    for (i = 0; i < MAX_FILTER_PLIES; ++i)
        for (j = 0; j <= i; ++j) {
            if (memcmp(&aamfNew[i][j], &aamfOld[i][j], sizeof(movefilter)) != 0) {
                szCmd = g_strdup_printf("%s %d %d %d %d %s",
                                        sz, i + 1, j,
                                        aamfNew[i][j].Accept,
                                        aamfNew[i][j].Extra,
                                        g_ascii_formatd(buf, G_ASCII_DTOSTR_BUF_SIZE, "%0.3f",
                                                        (double) aamfNew[i][j].Threshold));
                UserCommand(szCmd);
                g_free(szCmd);
            }

        }
    UserCommand("save settings");

}

extern void
MoveFilterSetPredefined(GtkWidget * pwMoveFilter, const int i)
{


    movefilterwidget *pmfw = (movefilterwidget *)
        g_object_get_data(G_OBJECT(pwMoveFilter), "user_data");

    if (i < 0)
        return;

    memcpy(pmfw->pmf, aaamfMoveFilterSettings[i], MAX_FILTER_PLIES * MAX_FILTER_PLIES * sizeof(movefilter));

    gtk_combo_box_set_active(GTK_COMBO_BOX(pmfw->pwOptionMenu), i);

    MoveFilterChanged(pmfw);

}
