/*
 * gtkexport.c
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
 * $Id: gtkexport.c,v 1.51 2014/03/09 23:31:28 plm Exp $
 */

#include "config.h"
#include <gtk/gtk.h>

#include <stdio.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

#include "backgammon.h"
#include "eval.h"
#include "gtkgame.h"
#include "export.h"
#include "gtkexport.h"
#include "boarddim.h"
#include "gtkwindows.h"
#include "gtklocdefs.h"

static char *aszInclude[] = {
    N_("Annotations"),
    N_("Analysis"),
    N_("Statistics"),
    N_("Match Information")
};

#define NUM_INCLUDE (sizeof(aszInclude)/sizeof(aszInclude[0]))

static char *aszMovesDisplay[] = {
    N_("Show for moves marked 'very bad'"),
    N_("Show for moves marked 'bad'"),
    N_("Show for moves marked 'doubtful'"),
    N_("Show for unmarked moves"),
};

#define NUM_MOVES (sizeof(aszMovesDisplay)/sizeof(aszMovesDisplay[0]))

static char *aszCubeDisplay[] = {
    N_("Show for cube decisions marked 'very bad'"),
    N_("Show for cube decisions marked 'bad'"),
    N_("Show for cube decisions marked 'doubtful'"),
    N_("Show for unmarked cube decisions"),
    N_("Show for actual cube decisions"),
    N_("Show for missed doubles"),
    N_("Show for close cube decisions")
};

#define NUM_CUBES (sizeof(aszCubeDisplay)/sizeof(aszCubeDisplay[0]))

typedef struct _exportwidget {

    /* export settings */

    exportsetup *pexs;

    /* include */

    GtkWidget *apwInclude[NUM_INCLUDE];

    /* board */

    GtkAdjustment *padjDisplayBoard;
    GtkWidget *apwSide[2];

    /* moves */

    GtkAdjustment *padjMoves;
    GtkWidget *pwMovesDetailProb;
    GtkWidget *apwMovesParameters[2];
    GtkWidget *apwMovesDisplay[NUM_MOVES];

    /* cube */

    GtkWidget *pwCubeDetailProb;
    GtkWidget *apwCubeParameters[2];
    GtkWidget *apwCubeDisplay[NUM_CUBES];

    /* other stuff */

    GtkWidget *pwHTMLPictureURL;
    GtkWidget *pwHTMLType;
    GtkWidget *pwHTMLCSS;

    /* Sizes */

    GtkWidget *pwPNGSize;
    GtkAdjustment *adjPNGSize;

    GtkWidget *pwHtmlSize;
    GtkAdjustment *adjHtmlSize;

} exportwidget;

static void
ExportGetValues(exportwidget * pew, exportsetup * pexs)
{

    unsigned int i;

    /* include */

    pexs->fIncludeAnnotation = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->apwInclude[0]));

    pexs->fIncludeAnalysis = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->apwInclude[1]));

    pexs->fIncludeStatistics = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->apwInclude[2]));

    pexs->fIncludeMatchInfo = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->apwInclude[3]));


    /* board */

    pexs->fDisplayBoard = (int) gtk_adjustment_get_value(pew->padjDisplayBoard);

    pexs->fSide = 0;
    for (i = 0; i < 2; i++)
        pexs->fSide = pexs->fSide | (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->apwSide[i])) << i);

    /* moves */

    pexs->nMoves = (int) gtk_adjustment_get_value(pew->padjMoves);

    pexs->fMovesDetailProb = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->pwMovesDetailProb));

    for (i = 0; i < 2; i++)
        pexs->afMovesParameters[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->apwMovesParameters[i]));

    for (i = 0; i < NUM_MOVES; i++)
        pexs->afMovesDisplay[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->apwMovesDisplay[i]));

    /* cube */

    pexs->fCubeDetailProb = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->pwCubeDetailProb));

    for (i = 0; i < 2; i++)
        pexs->afCubeParameters[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->apwCubeParameters[i]));

    /* skip unused entries */
    for (i = 0; i < NUM_CUBES; i++) {
        if (aszCubeDisplay[i]) {
            pexs->afCubeDisplay[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pew->apwCubeDisplay[i]));
        }
    }

    /* HTML */

    pexs->szHTMLPictureURL = g_strdup(gtk_entry_get_text(GTK_ENTRY(pew->pwHTMLPictureURL)));

    pexs->het = (htmlexporttype) gtk_combo_box_get_active(GTK_COMBO_BOX(pew->pwHTMLType));

    pexs->hecss = (htmlexportcss) gtk_combo_box_get_active(GTK_COMBO_BOX(pew->pwHTMLCSS));

    /* sizes */
    pexs->nPNGSize = (int) gtk_adjustment_get_value(pew->adjPNGSize);
    pexs->nHtmlSize = (int) gtk_adjustment_get_value(pew->adjHtmlSize);
}

#define CHECKVALUE(orig,new,flag,text,format) \
{ \
   if ( orig->flag != new->flag ) { \
      char *sz = g_strdup_printf ( "set export " text " " format, \
                                   new->flag ); \
      UserCommand ( sz ); \
      g_free ( sz ); \
   } \
}

#define CHECKFLAG(orig,new,flag,text) \
{ \
   if ( orig->flag != new->flag ) { \
      char *sz = g_strdup_printf ( "set export " text " %s", \
                                   new->flag ? "on" : "off" ); \
      UserCommand ( sz ); \
      g_free ( sz ); \
   } \
}

#define CHECKFLAG2(orig,new,flag,text,text2) \
{ \
   if ( orig->flag != new->flag ) { \
      char *sz = g_strdup_printf ( "set export " text " %s %s", \
                                   text2, new->flag ? "on" : "off" ); \
      UserCommand ( sz ); \
      g_free ( sz ); \
   } \
}

static void
SetExportCommands(const exportsetup * pexsOrig, const exportsetup * pexsNew)
{

    int i;

    /* display */

    CHECKFLAG(pexsOrig, pexsNew, fIncludeAnnotation, "include annotation");
    CHECKFLAG(pexsOrig, pexsNew, fIncludeAnalysis, "include analysis");
    CHECKFLAG(pexsOrig, pexsNew, fIncludeStatistics, "include statistics");
    CHECKFLAG(pexsOrig, pexsNew, fIncludeMatchInfo, "include matchinfo");

    /* board */

    CHECKVALUE(pexsOrig, pexsNew, fDisplayBoard, "show board", "%d");

    if (pexsOrig->fSide != pexsNew->fSide) {
        if (pexsNew->fSide == 3)
            UserCommand("set export show player both");
        else {
            CHECKVALUE(pexsOrig, pexsNew, fSide - 1, "show player", "%d");
        }
    }

    /* moves */

    CHECKVALUE(pexsOrig, pexsNew, nMoves, "moves number", "%d");
    CHECKFLAG(pexsOrig, pexsNew, fMovesDetailProb, "moves probabilities");
    CHECKFLAG(pexsOrig, pexsNew, afMovesParameters[0], "moves parameters evaluation");
    CHECKFLAG(pexsOrig, pexsNew, afMovesParameters[1], "moves parameters rollout");

    for (i = 0; i < N_SKILLS; ++i) {
        if (i == SKILL_NONE) {
            CHECKFLAG(pexsOrig, pexsNew, afMovesDisplay[i], "moves display unmarked");
        } else {
            CHECKFLAG2(pexsOrig, pexsNew, afMovesDisplay[i], "moves display", aszSkillTypeCommand[i]);
        }
    }

    /* cube */

    CHECKFLAG(pexsOrig, pexsNew, fCubeDetailProb, "cube probabilities");
    CHECKFLAG(pexsOrig, pexsNew, afCubeParameters[0], "cube parameters evaluation");
    CHECKFLAG(pexsOrig, pexsNew, afCubeParameters[1], "cube parameters rollout");

    for (i = 0; i < N_SKILLS; ++i) {
        if (i == SKILL_NONE) {
            CHECKFLAG(pexsOrig, pexsNew, afCubeDisplay[i], "cube display unmarked");
        } else {
            CHECKFLAG2(pexsOrig, pexsNew, afCubeDisplay[i], "cube display", aszSkillTypeCommand[i]);
        }
    }

    CHECKFLAG(pexsOrig, pexsNew, afCubeDisplay[EXPORT_CUBE_ACTUAL], "cube display actual");
    CHECKFLAG(pexsOrig, pexsNew, afCubeDisplay[EXPORT_CUBE_MISSED], "cube display missed");
    CHECKFLAG(pexsOrig, pexsNew, afCubeDisplay[EXPORT_CUBE_CLOSE], "cube display close");

    /* HTML */

    if (strcmp(pexsOrig->szHTMLPictureURL, pexsNew->szHTMLPictureURL)) {
        char *sz = g_strdup_printf("set export html pictureurl \"%s\"",
                                   pexsNew->szHTMLPictureURL);
        UserCommand(sz);
        g_free(sz);
    }

    if (pexsOrig->het != pexsNew->het) {
        char *sz = g_strdup_printf("set export html type \"%s\"",
                                   aszHTMLExportType[pexsNew->het]);
        UserCommand(sz);
        g_free(sz);
    }

    if (pexsOrig->hecss != pexsNew->hecss) {
        char *sz = g_strdup_printf("set export html css \"%s\"",
                                   aszHTMLExportCSSCommand[pexsNew->hecss]);
        UserCommand(sz);
        g_free(sz);
    }

    /* Sizes */
    if (pexsOrig->nPNGSize != pexsNew->nPNGSize) {
        char *sz = g_strdup_printf("set export png size %d", pexsNew->nPNGSize);
        UserCommand(sz);
        g_free(sz);
    }
    if (pexsOrig->nHtmlSize != pexsNew->nHtmlSize) {
        char *sz = g_strdup_printf("set export html size %d", pexsNew->nHtmlSize);
        UserCommand(sz);
        g_free(sz);
    }
    UserCommand("save settings");
}


static void
ExportOK(GtkWidget * pw, exportwidget * pew)
{

    exportsetup *pexs = pew->pexs;
    exportsetup exsNew;

    /* get new settings */

    ExportGetValues(pew, &exsNew);

    /* set new values */

    SetExportCommands(pexs, &exsNew);
    g_free(exsNew.szHTMLPictureURL);

    gtk_widget_destroy(gtk_widget_get_toplevel(pw));

}

static void
ExportSet(exportwidget * pew)
{

    exportsetup *pexs = pew->pexs;
    unsigned int i;

    /* include */

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->apwInclude[0]), pexs->fIncludeAnnotation);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->apwInclude[1]), pexs->fIncludeAnalysis);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->apwInclude[2]), pexs->fIncludeStatistics);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->apwInclude[3]), pexs->fIncludeMatchInfo);

    /* board */

    gtk_adjustment_set_value(pew->padjDisplayBoard, pexs->fDisplayBoard);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->apwSide[0]), pexs->fSide & 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->apwSide[1]), pexs->fSide & 2);

    /* moves */

    gtk_adjustment_set_value(pew->padjMoves, pexs->nMoves);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->pwMovesDetailProb), pexs->fMovesDetailProb);
    for (i = 0; i < 2; i++)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->apwMovesParameters[i]), pexs->afMovesParameters[i]);

    for (i = 0; i < NUM_MOVES; i++)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->apwMovesDisplay[i]), pexs->afMovesDisplay[i]);

    /* cube */

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->pwCubeDetailProb), pexs->fCubeDetailProb);

    for (i = 0; i < 2; i++)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->apwCubeParameters[i]), pexs->afCubeParameters[i]);

    for (i = 0; i < NUM_CUBES; i++) {
        if (aszCubeDisplay[i]) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pew->apwCubeDisplay[i]), pexs->afCubeDisplay[i]);
        }
    }

    /* HTML */

    if (pexs->szHTMLPictureURL)
        gtk_entry_set_text(GTK_ENTRY(pew->pwHTMLPictureURL), pexs->szHTMLPictureURL);

    gtk_combo_box_set_active(GTK_COMBO_BOX(pew->pwHTMLType), pexs->het);

    gtk_combo_box_set_active(GTK_COMBO_BOX(pew->pwHTMLCSS), pexs->hecss);

    /* Sizes */
    gtk_adjustment_set_value(GTK_ADJUSTMENT(pew->adjPNGSize), pexs->nPNGSize);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(pew->adjHtmlSize), pexs->nHtmlSize);
}


static void
SizeChanged(GtkAdjustment * adj, GtkWidget * pwSize)
{

    int n = (int) gtk_adjustment_get_value(adj);

    char *sz = g_strdup_printf(_("%dx%d pixels"),
                               n * BOARD_WIDTH, n * BOARD_HEIGHT);

    gtk_label_set_text(GTK_LABEL(pwSize), sz);

    g_free(sz);

}

static void
ExportHTMLImages(void)
{
    GtkWidget *fc;
    gchar *message, *expfolder, *folder, *command;
    gint ok = FALSE;
    fc = gtk_file_chooser_dialog_new(_
                                     ("Select top folder for HTML export"),
                                     NULL,
                                     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_window_set_modal(GTK_WINDOW(fc), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(fc), GTK_WINDOW(pwMain));

    while (!ok) {
        if (gtk_dialog_run(GTK_DIALOG(fc)) == GTK_RESPONSE_CANCEL) {
            gtk_widget_destroy(fc);
            return;
        }
        folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
        if (folder) {
            char *name = g_path_get_basename(folder);
            if (!StrCaseCmp(name, "html-images"))
                expfolder = g_strdup(folder);
            else
                expfolder = g_build_filename(folder, "html-images", NULL);
            g_free(name);
            if (g_file_test(expfolder, G_FILE_TEST_IS_DIR)) {
                message = g_strdup_printf(_("Folder html-images exists\nin %s\nOK to overwrite images?"), folder);
                ok = GTKGetInputYN(message);
                g_free(message);
            } else if (g_mkdir(expfolder, 0777) == 0) {
                ok = TRUE;
            } else {
                message = g_strdup_printf(_("Folder html-images can't be created\nin %s"), folder);
                GTKMessage(message, DT_ERROR);
                g_free(message);
            }
            if (ok) {
                command = g_strconcat("export htmlimages \"", expfolder, "\"", NULL);
                UserCommand(command);
                g_free(command);
                UserCommand("save settings");
            }
            g_free(expfolder);
            g_free(folder);
        }
    }
    gtk_widget_destroy(fc);
}

static void
GenHtmlImages(GtkWidget * UNUSED(widget), gpointer data)
{                               /* Temporarily set HTML size and create images */
    int temp = exsExport.nHtmlSize;
    exsExport.nHtmlSize = (int) gtk_adjustment_get_value(GTK_ADJUSTMENT(data));
    ExportHTMLImages();
    exsExport.nHtmlSize = temp;
}

extern void
GTKShowExport(exportsetup * pexs)
{
    GtkWidget *pwDialog;

    GtkWidget *pwNotebook;

    GtkWidget *pwVBox;
    GtkWidget *pwFrame;
    GtkWidget *pwTable;
    GtkWidget *pwTableX;
    GtkWidget *pwHBox;
    GtkWidget *pwHScale;
    GtkWidget *genHtml;

    GtkWidget *pw;

    unsigned int i;

    exportwidget *pew;

    pew = (exportwidget *) malloc(sizeof(exportwidget));

    pew->pexs = pexs;

    /* create dialog */

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Export Settings"), DT_QUESTION,
                               NULL, DIALOG_FLAG_MODAL, G_CALLBACK(ExportOK), pew);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwNotebook = gtk_notebook_new());
    gtk_container_set_border_width(GTK_CONTAINER(pwNotebook), 4);

    /* first tab */

    pwTable = gtk_table_new(2, 2, FALSE);

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), pwTable, gtk_label_new(_("Content")));

    /* include stuff */

    pwFrame = gtk_frame_new(_("Include"));
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 8);
    gtk_table_attach(GTK_TABLE(pwTable), pwFrame, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 8, 0);


    pwVBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwVBox);

    for (i = 0; i < NUM_INCLUDE; i++) {

        gtk_box_pack_start(GTK_BOX(pwVBox),
                           pew->apwInclude[i] = gtk_check_button_new_with_label(gettext(aszInclude[i])), TRUE, TRUE, 0);
    }

    /* show stuff */

    pwFrame = gtk_frame_new(_("Board"));

    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 8);
    gtk_table_attach(GTK_TABLE(pwTable), pwFrame, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 2, 2);



    pwTableX = gtk_table_new(2, 3, FALSE);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwTableX);

    gtk_table_attach(GTK_TABLE(pwTableX), pw = gtk_label_new(_("Board")), 0, 1, 0, 1, GTK_FILL, GTK_FILL, 4, 0);
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

    pw = gtk_hbox_new(FALSE, 0);

    pew->padjDisplayBoard = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1000, 1, 1, 0));

    gtk_box_pack_start(GTK_BOX(pw), gtk_spin_button_new(pew->padjDisplayBoard, 1, 0), TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(pw), gtk_label_new(_("move(s) between board shown")), TRUE, TRUE, 0);

    gtk_table_attach(GTK_TABLE(pwTableX), pw, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 4, 0);


    gtk_table_attach(GTK_TABLE(pwTableX), pw = gtk_label_new(_("Players")), 0, 1, 1, 2, GTK_FILL, GTK_FILL, 4, 0);
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

    gtk_table_attach(GTK_TABLE(pwTableX),
                     pew->apwSide[0] =
                     gtk_check_button_new_with_label(ap[0].szName), 1, 2, 1, 2, GTK_FILL, GTK_FILL, 4, 0);

    gtk_table_attach(GTK_TABLE(pwTableX),
                     pew->apwSide[1] =
                     gtk_check_button_new_with_label(ap[1].szName), 1, 2, 2, 3, GTK_FILL, GTK_FILL, 4, 0);

    /* moves */

    pwFrame = gtk_frame_new(_("Output move analysis"));

    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 8);
    gtk_table_attach(GTK_TABLE(pwTable), pwFrame, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 2, 2);

    pwVBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwVBox);


    pw = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVBox), pw, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(pw), gtk_label_new(_("Show at most")), TRUE, TRUE, 4);

    pew->padjMoves = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1000, 1, 1, 0));

    gtk_box_pack_start(GTK_BOX(pw), gtk_spin_button_new(pew->padjMoves, 1, 0), TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(pw), gtk_label_new(_("move(s)")), TRUE, TRUE, 4);


    gtk_box_pack_start(GTK_BOX(pwVBox),
                       pew->pwMovesDetailProb =
                       gtk_check_button_new_with_label(_("Show detailed " "probabilities")), TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(pwVBox),
                       pew->apwMovesParameters[0] =
                       gtk_check_button_new_with_label(_("Show evaluation parameters")), TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(pwVBox),
                       pew->apwMovesParameters[1] =
                       gtk_check_button_new_with_label(_("Show rollout parameters")), TRUE, TRUE, 0);


    for (i = 0; i < NUM_MOVES; i++)

        gtk_box_pack_start(GTK_BOX(pwVBox),
                           pew->apwMovesDisplay[i] =
                           gtk_check_button_new_with_label(gettext(aszMovesDisplay[i])), TRUE, TRUE, 0);

    /* cube */

    pwFrame = gtk_frame_new(_("Output cube decision analysis"));

    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 8);
    gtk_table_attach(GTK_TABLE(pwTable), pwFrame, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 2, 2);

    pwVBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwVBox);


    gtk_box_pack_start(GTK_BOX(pwVBox),
                       pew->pwCubeDetailProb =
                       gtk_check_button_new_with_label(_("Show detailed " "probabilities")), TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(pwVBox),
                       pew->apwCubeParameters[0] =
                       gtk_check_button_new_with_label(_("Show evaluation " "parameters")), TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(pwVBox),
                       pew->apwCubeParameters[1] =
                       gtk_check_button_new_with_label(_("Show rollout " "parameters")), TRUE, TRUE, 0);


    for (i = 0; i < NUM_CUBES; i++) {
        if (aszCubeDisplay[i]) {
            gtk_box_pack_start(GTK_BOX(pwVBox),
                               pew->apwCubeDisplay[i] =
                               gtk_check_button_new_with_label(gettext(aszCubeDisplay[i])), TRUE, TRUE, 0);
        }
    }


    /* second tab */

    pwTable = gtk_table_new(1, 2, FALSE);

    gtk_notebook_append_page(GTK_NOTEBOOK(pwNotebook), pwTable, gtk_label_new(_("Style")));

    /* HTML */

    pwFrame = gtk_frame_new(_("HTML export options"));

    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 8);
    gtk_table_attach(GTK_TABLE(pwTable), pwFrame, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 2, 2);

    pwVBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwVBox);
    gtk_container_set_border_width(GTK_CONTAINER(pwVBox), 4);


    gtk_box_pack_start(GTK_BOX(pwVBox), pw = gtk_label_new(_("URL to pictures")), TRUE, TRUE, 0);
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

    gtk_box_pack_start(GTK_BOX(pwVBox), pew->pwHTMLPictureURL = gtk_entry_new(), TRUE, TRUE, 0);

    pwHBox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwHBox), gtk_label_new(_("HTML board type:")), TRUE, TRUE, 0);

    pew->pwHTMLType = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(pwHBox), pew->pwHTMLType, FALSE, FALSE, 0);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pew->pwHTMLType), _("GNU Backgammon"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pew->pwHTMLType), _("BBS"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pew->pwHTMLType), _("fibs2html"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(pew->pwHTMLType), 0);

    gtk_container_set_border_width(GTK_CONTAINER(pwHBox), 4);
    gtk_box_pack_start(GTK_BOX(pwVBox), pwHBox, FALSE, FALSE, 0);

    /* HTML CSS */

    pwHBox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwHBox), gtk_label_new(_("CSS Style sheet:")), TRUE, TRUE, 0);

    pew->pwHTMLCSS = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(pwHBox), pew->pwHTMLCSS, FALSE, FALSE, 0);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pew->pwHTMLCSS), _("In <head>"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pew->pwHTMLCSS), _("Inline (in tags)"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pew->pwHTMLCSS), _("External file"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(pew->pwHTMLCSS), 0);

    gtk_container_set_border_width(GTK_CONTAINER(pwHBox), 4);
    gtk_box_pack_start(GTK_BOX(pwVBox), pwHBox, FALSE, FALSE, 0);

    /* Sizes */

    pwFrame = gtk_frame_new(_("Export sizes"));

    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 8);
    gtk_table_attach(GTK_TABLE(pwTable), pwFrame, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 2, 2);

    pwVBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwVBox);
    gtk_container_set_border_width(GTK_CONTAINER(pwVBox), 4);

    /* Png size */
    pwHBox = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwHBox), 4);
    gtk_box_pack_start(GTK_BOX(pwVBox), pwHBox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pwHBox), gtk_label_new(_("Size of PNG images:")), TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(pwHBox), pew->pwPNGSize = gtk_label_new(""), TRUE, TRUE, 0);

    pew->adjPNGSize = GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 20, 1, 5, 0));
    pwHScale = gtk_hscale_new(pew->adjPNGSize);
    gtk_scale_set_digits(GTK_SCALE(pwHScale), 0);

    gtk_box_pack_start(GTK_BOX(pwVBox), pwHScale, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(pew->adjPNGSize), "value-changed", G_CALLBACK(SizeChanged), pew->pwPNGSize);

    /* HTML size */
    pwHBox = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwHBox), 4);
    gtk_box_pack_start(GTK_BOX(pwVBox), pwHBox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pwHBox), gtk_label_new(_("Size of HTML images:")), TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(pwHBox), pew->pwHtmlSize = gtk_label_new(""), TRUE, TRUE, 0);

    pew->adjHtmlSize = GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 20, 1, 5, 0));
    pwHScale = gtk_hscale_new(pew->adjHtmlSize);
    gtk_scale_set_digits(GTK_SCALE(pwHScale), 0);
    gtk_box_pack_start(GTK_BOX(pwVBox), pwHScale, FALSE, FALSE, 0);

    genHtml = gtk_button_new_with_label(_("Generate HTML images..."));
    pwHBox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwHBox), genHtml, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVBox), pwHBox, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(genHtml), "clicked", G_CALLBACK(GenHtmlImages), pew->adjHtmlSize);
    g_signal_connect(G_OBJECT(pew->adjHtmlSize), "value-changed", G_CALLBACK(SizeChanged), pew->pwHtmlSize);

    /* show dialog */

    ExportSet(pew);

    GTKRunDialog(pwDialog);
}
