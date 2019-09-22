/* gtk-multiview.h
 * Copyright (C) 2000  Jonathan Blandford
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 3 or later of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * $Id: gtk-multiview.h,v 1.9 2013/07/22 18:51:01 mdpetch Exp $
 */

/* License changed from the GNU LGPL to the GNU GPL (as permitted
 * under Term 3 of the GNU LGPL) by Gary Wong for distribution
 * with GNU Backgammon. */

#ifndef __GTK_MULTIVIEW_H__
#define __GTK_MULTIVIEW_H__

#define GTK_TYPE_MULTIVIEW			(gtk_multiview_get_type ())
#define GTK_MULTIVIEW(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_MULTIVIEW, GtkMultiview))
#define GTK_MULTIVIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_MULTIVIEW, GtkMultiviewClass))
#define GTK_IS_MULTIVIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_MULTIVIEW))
#define GTK_IS_MULTIVIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((obj), GTK_TYPE_MULTIVIEW))
#define GTK_MULTIVIEW_GET_CLASS(obj)  		(G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_MULTIVIEW, GtkMultiviewClass))


typedef struct _GtkMultiview GtkMultiview;
typedef struct _GtkMultiviewClass GtkMultiviewClass;

struct _GtkMultiview {
    GtkContainer parent;

    /*< private > */
    GtkWidget *current;
    GList *children;
};

struct _GtkMultiviewClass {
    GtkContainerClass parent_class;
};

GType gtk_multiview_get_type(void);
GtkWidget *gtk_multiview_new(void);
void gtk_multiview_prepend_child(GtkMultiview * multiview, GtkWidget * child);
void gtk_multiview_insert_child(GtkMultiview * multiview, GtkWidget * back_child, GtkWidget * child);
void gtk_multiview_append_child(GtkMultiview * multiview, GtkWidget * child);
void gtk_multiview_set_current(GtkMultiview * multiview, GtkWidget * child);



#endif                          /* __GTK_MULTIVIEW_H__ */
