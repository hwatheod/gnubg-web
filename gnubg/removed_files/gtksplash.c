/*
 * gtksplash.c
 *
 * by Joern Thyssen <jth@gnubg.org>, 2002
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
 * $Id: gtksplash.c,v 1.34 2013/06/16 02:16:16 mdpetch Exp $
 */

#include "config.h"
#include "gtklocdefs.h"

#include "gtkgame.h"
#include "util.h"
#include "gtksplash.h"

typedef struct _gtksplash {
    GtkWidget *pwWindow;
    GtkWidget *apwStatus[2];
} gtksplash;


extern GtkWidget *
CreateSplash(void)
{

    gtksplash *pgs;
    GtkWidget *pwvbox, *pwFrame, *pwb;
    GtkWidget *pwImage;
    gchar *fn;
    int i;

    pgs = (gtksplash *) g_malloc(sizeof(gtksplash));

    pgs->pwWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_role(GTK_WINDOW(pgs->pwWindow), "splash screen");
    gtk_window_set_type_hint(GTK_WINDOW(pgs->pwWindow), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
    gtk_window_set_title(GTK_WINDOW(pgs->pwWindow), _("Starting " VERSION_STRING));
    gtk_window_set_position(GTK_WINDOW(pgs->pwWindow), GTK_WIN_POS_CENTER);

    gtk_widget_realize(GTK_WIDGET(pgs->pwWindow));

    /* gdk_window_set_decorations ( GTK_WIDGET ( pgs->pwWindow )->window, 0 ); */

    /* content of page */

    pwvbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pgs->pwWindow), pwvbox);

    /* image */

    fn = g_build_filename(getPkgDataDir(), "pixmaps", "gnubg-big.png", NULL);
    pwImage = gtk_image_new_from_file(fn);
    g_free(fn);
    gtk_box_pack_start(GTK_BOX(pwvbox), pwImage, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pwvbox), pwFrame = gtk_frame_new(NULL), FALSE, FALSE, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(pwFrame), GTK_SHADOW_ETCHED_OUT);

    gtk_container_add(GTK_CONTAINER(pwFrame), pwb = gtk_vbox_new(FALSE, 0));

    /* status bar */

    for (i = 0; i < 2; ++i) {
        pgs->apwStatus[i] = gtk_label_new(NULL);
        gtk_box_pack_start(GTK_BOX(pwb), pgs->apwStatus[i], FALSE, FALSE, 4);
    }


    /* signals */

    g_object_set_data_full(G_OBJECT(pgs->pwWindow), "gtksplash", pgs, g_free);

    gtk_widget_show_all(GTK_WIDGET(pgs->pwWindow));

    ProcessEvents();

    return pgs->pwWindow;

}


extern void
DestroySplash(GtkWidget * pwSplash)
{

    if (!pwSplash)
        return;

#ifndef WIN32
    /* Don't bother with these pauses on windows? */
    g_usleep(250 * 1000);
#endif

    gtk_widget_destroy(pwSplash);

}


extern void
PushSplash(GtkWidget * pwSplash, const gchar * szText0, const gchar * szText1)
{
    gtksplash *pgs;

    if (!pwSplash)
        return;

    pgs = (gtksplash *) g_object_get_data(G_OBJECT(pwSplash), "gtksplash");

    gtk_label_set_text(GTK_LABEL(pgs->apwStatus[0]), szText0);
    gtk_label_set_text(GTK_LABEL(pgs->apwStatus[1]), szText1);

    ProcessEvents();
}
