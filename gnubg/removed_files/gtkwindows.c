/*
 * gtkwindows.c
 * by Jon Kinsey, 2006
 *
 * functions to create windows
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
 * $Id: gtkwindows.c,v 1.53 2014/09/07 19:24:06 plm Exp $
 */

#include "config.h"
#include "gtklocdefs.h"

#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "gtkwindows.h"
#include "gtkgame.h"
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
typedef void (*dialog_func_ty) (GtkWidget *, void *);

typedef struct _Warning {
    char *warningString;
    char *warningName;
    int warningEnabled;
    int isWarningQuestion;
} Warning;

static Warning warnings[WARN_NUM_WARNINGS] = {
    {
     N_("Press escape to exit full screen mode"),
     "fullscreenexit", TRUE, FALSE},
    {
     N_("This option will speed up the 3d drawing, but may not work correctly on all machines"),
     "quickdraw", TRUE, FALSE},
    {
     N_("Drawing shadows is only supported on the latest graphics cards\n"
        "Disable this option if performance is poor"),
     "shadows", TRUE, FALSE},
    {
     N_("No hardware accelerated graphics card found, performance may be slow"),
     "unaccelerated", TRUE, FALSE},
    {
     N_("Interrupt the current process?"),
     "stop", TRUE, TRUE},
    {
     N_("Play to the end of the game automatically?"),
     "endgame", TRUE, TRUE},
    {
     N_("Automatically perform correct resignation?"),
     "resign", TRUE, TRUE},
    {
     N_
     ("This action will roll out the current position before the dice are rolled.\nIt will not be possible to extend nor save this rollout."),
     "rollout", TRUE, FALSE}
};

static char *aszStockItem[NUM_DIALOG_TYPES] = {
    GTK_STOCK_DIALOG_INFO,
    GTK_STOCK_DIALOG_QUESTION,
    GTK_STOCK_DIALOG_WARNING,
    GTK_STOCK_DIALOG_WARNING,
    GTK_STOCK_DIALOG_ERROR,
    NULL
};

static void
quitter(GtkWidget * UNUSED(widget), GtkWidget * parent)
{
    gtk_main_quit();
    if (parent)
        gtk_window_present(GTK_WINDOW(parent));
}

typedef struct _CallbackStruct {
    void (*DialogFun) (GtkWidget *, void *);
    gpointer data;
} CallbackStruct;

static void
DialogResponse(GtkWidget * dialog, gint response, CallbackStruct * data)
{
    if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_CLOSE) {
        if (data->DialogFun)
            data->DialogFun(dialog, data->data);
        else
            OK(dialog, data->data);
    } else if (response == GTK_RESPONSE_CANCEL) {
        gtk_widget_destroy(dialog);
    } else {                    /* Ignore response */
    }
}

static void
dialog_mapped(GtkWidget * window, gpointer UNUSED(data))
{
    GdkRectangle monitorrect;
    GtkAllocation allocation;
    GdkScreen *screen = gtk_widget_get_screen(window);

    gtk_widget_get_allocation(window, &allocation);

    if (!screen || gdk_screen_get_n_monitors(screen) == 1) {
        monitorrect.x = 0;
        monitorrect.y = 0;
        monitorrect.width = gdk_screen_width();
        monitorrect.height = gdk_screen_height();
    } else {
        int monitor = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(window));
        gdk_screen_get_monitor_geometry(screen, monitor, &monitorrect);
    }

    if (allocation.width > monitorrect.width || allocation.height > monitorrect.height) {       /* Dialog bigger than window! (just show at top left) */
        gtk_window_move(GTK_WINDOW(window), monitorrect.x, monitorrect.y);
    } else {
        GdkRectangle rect;
        gdk_window_get_frame_extents(gtk_widget_get_window(window), &rect);
        if (rect.x < monitorrect.x)
            rect.x = monitorrect.x;
        if (rect.y < monitorrect.y)
            rect.y = monitorrect.y;
        if (rect.x + rect.width > monitorrect.x + monitorrect.width)
            rect.x = monitorrect.x + monitorrect.width - rect.width;
        if (rect.y + rect.height > monitorrect.y + monitorrect.height)
            rect.y = monitorrect.y + monitorrect.height - rect.height;
        gtk_window_move(GTK_WINDOW(window), rect.x, rect.y);
    }
}

extern GtkWidget *
GTKCreateDialog(const char *szTitle, const dialogtype dt,
                GtkWidget * parent, int flags, GCallback okFun, void *okFunData)
{
    CallbackStruct *cbData;
    GtkWidget *pwDialog, *pwHbox, *pwPixmap;
    GtkAccelGroup *pag;
    int fQuestion = (dt == DT_QUESTION || dt == DT_AREYOUSURE);

    pwDialog = gtk_dialog_new();
    if (flags & DIALOG_FLAG_MINMAXBUTTONS)
        gtk_window_set_type_hint(GTK_WINDOW(pwDialog), GDK_WINDOW_TYPE_HINT_NORMAL);
    gtk_window_set_title(GTK_WINDOW(pwDialog), szTitle);

    if (parent == NULL)
        parent = GTKGetCurrentParent();
    if (!GTK_IS_WINDOW(parent))
        parent = gtk_widget_get_toplevel(parent);
    if (parent && !gtk_widget_get_realized(parent))
        /* if parent isn't realized we should not present it. */
        parent = NULL;
    if (parent && GTK_IS_WINDOW(parent))
        gtk_window_present(GTK_WINDOW(parent));
    if (parent != NULL)
        gtk_window_set_transient_for(GTK_WINDOW(pwDialog), GTK_WINDOW(parent));

    if ((flags & DIALOG_FLAG_MODAL) && !(flags & DIALOG_FLAG_NOTIDY))
        g_signal_connect(G_OBJECT(pwDialog), "destroy", G_CALLBACK(quitter), parent);

    pag = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(pwDialog), pag);

    pwHbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(pwDialog))), pwHbox);

    if (dt != DT_CUSTOM) {
        pwPixmap = gtk_image_new_from_stock(aszStockItem[dt], GTK_ICON_SIZE_DIALOG);
        gtk_misc_set_padding(GTK_MISC(pwPixmap), 8, 8);
        gtk_box_pack_start(GTK_BOX(pwHbox), pwPixmap, FALSE, FALSE, 0);
    }

    cbData = g_new0(CallbackStruct, 1);

    cbData->DialogFun = (dialog_func_ty) okFun;

    cbData->data = okFunData;
    g_object_set_data_full(G_OBJECT(pwDialog), "cbData", cbData, g_free);
    if (!(flags & DIALOG_FLAG_NORESPONSE))
        g_signal_connect(pwDialog, "response", G_CALLBACK(DialogResponse), cbData);

    if ((flags & DIALOG_FLAG_NOOK) == 0) {
        int OkButton = (flags & DIALOG_FLAG_MODAL && (flags & DIALOG_FLAG_CLOSEBUTTON) == 0);

        gtk_dialog_add_button(GTK_DIALOG(pwDialog), OkButton ? _("OK") : _("Close"),
                              OkButton ? GTK_RESPONSE_OK : GTK_RESPONSE_CLOSE);
        gtk_dialog_set_default_response(GTK_DIALOG(pwDialog), OkButton ? GTK_RESPONSE_OK : GTK_RESPONSE_CLOSE);

        if (!fQuestion)
            gtk_widget_add_accelerator(DialogArea(pwDialog, DA_OK), "clicked", pag, GDK_Escape, 0, 0);
    }

    if (fQuestion)
        gtk_dialog_add_button(GTK_DIALOG(pwDialog), _("Cancel"), GTK_RESPONSE_CANCEL);

    if (flags & DIALOG_FLAG_MODAL)
        gtk_window_set_modal(GTK_WINDOW(pwDialog), TRUE);

    g_signal_connect(pwDialog, "map-event", G_CALLBACK(dialog_mapped), 0);

    return pwDialog;
}

extern GtkWidget *
DialogArea(GtkWidget * pw, dialogarea da)
{
    GList *pl, *pl_org;
    GtkWidget *pwChild = NULL;

    switch (da) {
    case DA_MAIN:
        pl = gtk_container_get_children(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(pw))));
        pwChild = pl->data;
        g_list_free(pl);
        return pwChild;

    case DA_BUTTONS:
        return gtk_dialog_get_action_area(GTK_DIALOG(pw));

    case DA_OK:
        pl = pl_org = gtk_container_get_children(GTK_CONTAINER(gtk_dialog_get_action_area(GTK_DIALOG(pw))));
        while (pl) {
            pwChild = pl->data;
            if (!strcmp(gtk_button_get_label(GTK_BUTTON(pwChild)), _("OK")))
                break;
            pl = pl->next;
        }
        g_list_free(pl_org);
        return pwChild;

    default:
        return NULL;
    }
}

/* Use to temporarily set the parent dialog for nested dialogs
 * Note that passing a control of a window is ok (and common) */
static GtkWidget *pwCurrentParent = NULL;

extern void
GTKSetCurrentParent(GtkWidget * parent)
{
    pwCurrentParent = parent;
}

extern GtkWidget *
GTKGetCurrentParent(void)
{
    if (pwCurrentParent) {
        GtkWidget *current = pwCurrentParent;
        pwCurrentParent = NULL; /* Single set/get usage */
        return current;
    } else
        return pwMain;
}

extern int
GTKMessage(const char *sz, dialogtype dt)
{
#define MAXWINSIZE 400
#define MAXSTRLEN 200
    int answer = FALSE;
    static char *aszTitle[NUM_DIALOG_TYPES] = {
        N_("GNU Backgammon - Message"),
        N_("GNU Backgammon - Question"),
        N_("GNU Backgammon - Warning"), /* are you sure */
        N_("GNU Backgammon - Warning"),
        N_("GNU Backgammon - Error"),
    };
    GtkWidget *pwDialog, *pwText, *sw, *frame;
    GtkRequisition req;
    GtkTextBuffer *buffer;

    g_return_val_if_fail(sz, FALSE);

    pwDialog = GTKCreateDialog(gettext(aszTitle[dt]), dt, GTKGetCurrentParent(), DIALOG_FLAG_MODAL, NULL, &answer);

    pwText = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(pwText), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(pwText), FALSE);
    buffer = gtk_text_buffer_new(NULL);
    gtk_text_buffer_set_text(buffer, sz, -1);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(pwText), buffer);

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sw), pwText);
    gtk_widget_size_request(GTK_WIDGET(pwText), &req);
    if (strlen(sz) > MAXSTRLEN || req.height > MAXWINSIZE) {
        int wz = MIN(MAXWINSIZE, req.height + 100);
        gtk_window_set_default_size(GTK_WINDOW(pwDialog), -1, wz);
    }
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), sw);
    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), frame);

    /* This dialog should be REALLY modal -- disable "next turn" idle
     * processing and stdin handler, to avoid reentrancy problems. */
    if (nNextTurn)
        g_source_remove(nNextTurn);

    GTKRunDialog(pwDialog);

    if (nNextTurn)
        nNextTurn = g_idle_add(NextTurnNotify, NULL);

    return answer;
}

extern int
GTKGetInputYN(char *szPrompt)
{
    return GTKMessage(szPrompt, DT_AREYOUSURE);
}

static char *inputString;

static void
GetInputOk(GtkWidget * pw, GtkWidget * pwEntry)
{
    inputString = g_strdup(gtk_entry_get_text(GTK_ENTRY(pwEntry)));
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
}

extern char *
GTKGetInput(char *title, char *prompt, GtkWidget * parent)
{
    GtkWidget *pwDialog, *pwHbox, *pwEntry;
    pwEntry = gtk_entry_new();
    inputString = NULL;
    pwDialog = GTKCreateDialog(title, DT_QUESTION, parent, DIALOG_FLAG_MODAL, G_CALLBACK(GetInputOk), pwEntry);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwHbox = gtk_hbox_new(FALSE, 0));

    gtk_box_pack_start(GTK_BOX(pwHbox), gtk_label_new(prompt), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwHbox), pwEntry, FALSE, FALSE, 0);
    gtk_entry_set_activates_default(GTK_ENTRY(pwEntry), TRUE);
    gtk_widget_grab_focus(pwEntry);

    GTKRunDialog(pwDialog);

    return inputString;
}

static GtkWidget *pwTick;

static int warningResult;

static void
WarningOK(GtkWidget * pw, warningType warning)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pwTick))) {      /* if tick set, disable warning */
        char cmd[200];
        sprintf(cmd, "set warning %s off", warnings[warning].warningName);
        UserCommand(cmd);
        UserCommand("save settings");
    }
    warningResult = TRUE;
    gtk_widget_destroy(gtk_widget_get_toplevel(pw));
}

extern int
GTKShowWarning(warningType warning, GtkWidget * pwParent)
{
    if (fX && warnings[warning].warningEnabled) {
        char *buf;
        GtkWidget *pwDialog, *pwMsg, *pwv, *label;

        pwDialog =
            GTKCreateDialog(_("GNU Backgammon - Warning"),
                            warnings[warning].isWarningQuestion ? DT_AREYOUSURE : DT_WARNING, pwParent,
                            DIALOG_FLAG_MODAL, G_CALLBACK(WarningOK), (void *) warning);

        pwv = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwv);

        pwMsg = gtk_label_new(gettext(warnings[warning].warningString));
        gtk_misc_set_padding(GTK_MISC(pwMsg), 8, 8);
        gtk_box_pack_start(GTK_BOX(pwv), pwMsg, TRUE, TRUE, 0);

        buf = g_strdup_printf("<small>%s</small>", _("Don't show this again"));
        label = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(label), buf);
        g_free(buf);
        pwTick = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(pwTick), label);
        gtk_widget_set_tooltip_text(pwTick, _("If set, this message won't appear again"));
        gtk_box_pack_start(GTK_BOX(pwv), pwTick, TRUE, TRUE, 0);
        gtk_widget_grab_focus(DialogArea(pwDialog, DA_OK));

        warningResult = FALSE;
        GTKRunDialog(pwDialog);
        return warningResult;
    } else
        return TRUE;
}

extern warningType
ParseWarning(char *str)
{
    int i;

    while (*str == ' ')
        str++;

    for (i = 0; i < WARN_NUM_WARNINGS; i++) {
        if (!StrCaseCmp(str, warnings[i].warningName))
            return i;
    }

    return -1;
}

extern void
SetWarningEnabled(warningType warning, int value)
{
    warnings[warning].warningEnabled = value;
}

extern void
PrintWarning(warningType warning)
{
    char buf[1024];
    sprintf(buf, _("Warning %s (%s) is %s"), warnings[warning].warningName, warnings[warning].warningString,
            warnings[warning].warningEnabled ? "on" : "off");
    outputl(buf);
}

extern void
WriteWarnings(FILE * pf)
{
    int i;
    for (i = 0; i < WARN_NUM_WARNINGS; i++) {
        if (!warnings[i].warningEnabled)
            fprintf(pf, "set warning %s off\n", warnings[i].warningName);
    }
}

extern void
GTKRunDialog(GtkWidget * dialog)
{
    GTKDisallowStdin();
    gtk_widget_show_all(dialog);
    gtk_main();
    GTKAllowStdin();
}
