/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpstock.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gnubgstock.h"
#include "gnubg-stock-pixbufs.h"

static GtkIconFactory *gnubg_stock_factory = NULL;

static void
icon_set_from_inline(GtkIconSet * set,
                     const guchar * inline_data, GtkIconSize size, GtkTextDirection direction, gboolean fallback)
{
    GtkIconSource *source;
    GdkPixbuf *pixbuf;

    source = gtk_icon_source_new();

    if (direction != GTK_TEXT_DIR_NONE) {
        gtk_icon_source_set_direction(source, direction);
        gtk_icon_source_set_direction_wildcarded(source, FALSE);
    }

    gtk_icon_source_set_size(source, size);
    gtk_icon_source_set_size_wildcarded(source, FALSE);

    pixbuf = gdk_pixbuf_new_from_inline(-1, inline_data, FALSE, NULL);

    g_assert(pixbuf);

    gtk_icon_source_set_pixbuf(source, pixbuf);

    g_object_unref(pixbuf);

    gtk_icon_set_add_source(set, source);

    if (fallback) {
        gtk_icon_source_set_size_wildcarded(source, TRUE);
        gtk_icon_set_add_source(set, source);
    }

    gtk_icon_source_free(source);
}

static void
add_sized_with_same_fallback(GtkIconFactory * factory,
                             const guchar * inline_data,
                             const guchar * inline_data_rtl, GtkIconSize size, const gchar * stock_id)
{
    GtkIconSet *set;
    gboolean fallback = FALSE;

    set = gtk_icon_factory_lookup(factory, stock_id);

    if (!set) {
        set = gtk_icon_set_new();
        gtk_icon_factory_add(factory, stock_id, set);
        gtk_icon_set_unref(set);

        fallback = TRUE;
    }

    icon_set_from_inline(set, inline_data, size, GTK_TEXT_DIR_NONE, fallback);

    if (inline_data_rtl)
        icon_set_from_inline(set, inline_data_rtl, size, GTK_TEXT_DIR_RTL, fallback);
}

static const GtkStockItem gnubg_stock_items[] = {
    {GNUBG_STOCK_ACCEPT, N_("_Accept"), 0, 0, NULL},
    {GNUBG_STOCK_ANTI_CLOCKWISE, N_("Play in Anti-clockwise Direction"), 0, 0, NULL},
    {GNUBG_STOCK_CLOCKWISE, N_("Play in _Clockwise Direction"), 0, 0, NULL},
    {GNUBG_STOCK_DOUBLE, N_("_Double"), 0, 0, NULL},
    {GNUBG_STOCK_END_GAME, N_("_End Game"), 0, 0, NULL},
    {GNUBG_STOCK_GO_NEXT_CMARKED, N_("Next CMarked"), 0, 0, NULL},
    {GNUBG_STOCK_GO_NEXT_GAME, N_("Next _Game"), 0, 0, NULL},
    {GNUBG_STOCK_GO_NEXT_MARKED, N_("Next _Marked"), 0, 0, NULL},
    {GNUBG_STOCK_GO_NEXT, N_("_Next"), 0, 0, NULL},
    {GNUBG_STOCK_GO_PREV_CMARKED, N_("Previous CMarked"), 0, 0, NULL},
    {GNUBG_STOCK_GO_PREV_GAME, N_("Previous _Game"), 0, 0, NULL},
    {GNUBG_STOCK_GO_PREV_MARKED, N_("Previous _Marked"), 0, 0, NULL},
    {GNUBG_STOCK_GO_PREV, N_("_Previous"), 0, 0, NULL},
    {GNUBG_STOCK_HINT, N_("_Hint"), 0, 0, NULL},
    {GNUBG_STOCK_REJECT, N_("_Reject"), 0, 0, NULL},
    {GNUBG_STOCK_RESIGN, N_("_Resign"), 0, 0, NULL},
};

static const struct {
    const gchar *stock_id;
    gconstpointer ltr;
    gconstpointer rtl;
    GtkIconSize size;
} gnubg_stock_pixbufs[] = {
    {
    GNUBG_STOCK_ACCEPT, ok_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_ACCEPT, ok_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_ANTI_CLOCKWISE, anti_clockwise_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_CLOCKWISE, clockwise_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_DOUBLE, double_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_DOUBLE, double_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_END_GAME, runit_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_END_GAME, runit_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_GO_NEXT_CMARKED, go_next_cmarked_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_GO_NEXT_CMARKED, go_next_cmarked_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_GO_NEXT_GAME, go_next_game_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_GO_NEXT_GAME, go_next_game_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_GO_NEXT, go_next_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_GO_NEXT, go_next_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_GO_NEXT_MARKED, go_next_marked_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_GO_NEXT_MARKED, go_next_marked_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_GO_PREV_CMARKED, go_prev_cmarked_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_GO_PREV_CMARKED, go_prev_cmarked_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_GO_PREV_GAME, go_prev_game_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_GO_PREV_GAME, go_prev_game_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_GO_PREV, go_prev_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_GO_PREV, go_prev_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_GO_PREV_MARKED, go_prev_marked_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_GO_PREV_MARKED, go_prev_marked_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_HINT, hint_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_HINT, hint_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_NEW0, new0_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_NEW11, new11_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_NEW13, new13_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_NEW15, new15_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_NEW17, new17_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_NEW1, new1_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_NEW3, new3_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_NEW5, new5_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_NEW7, new7_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_NEW9, new9_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_REJECT, cancel_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_REJECT, cancel_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_RESIGN, resign_16, NULL, GTK_ICON_SIZE_MENU}, {
    GNUBG_STOCK_RESIGN, resign_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_RESIGNSB, resignsb_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_RESIGNSG, resignsg_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}, {
    GNUBG_STOCK_RESIGNSN, resignsn_24, NULL, GTK_ICON_SIZE_LARGE_TOOLBAR}
};

/**
 * gnubg_stock_init:
 *
 * Initializes the GNUBG stock icon factory.
 */
void
gnubg_stock_init(void)
{
    guint i;
    gnubg_stock_factory = gtk_icon_factory_new();

    for (i = 0; i < G_N_ELEMENTS(gnubg_stock_pixbufs); i++) {
        add_sized_with_same_fallback(gnubg_stock_factory,
                                     gnubg_stock_pixbufs[i].ltr,
                                     gnubg_stock_pixbufs[i].rtl,
                                     gnubg_stock_pixbufs[i].size, gnubg_stock_pixbufs[i].stock_id);
    }

    gtk_icon_factory_add_default(gnubg_stock_factory);
    gtk_stock_add_static(gnubg_stock_items, G_N_ELEMENTS(gnubg_stock_items));
}
