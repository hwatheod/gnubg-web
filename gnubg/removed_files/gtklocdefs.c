/*
 * gtklocdefs.c
 *
 * by Michael Petch <mpetch@capp-sysware.com>, 2011.
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
 * $Id: gtklocdefs.c,v 1.11 2014/06/22 22:34:27 mdpetch Exp $
 */


#include "config.h"
#include "gtkgame.h"
#include "gtklocdefs.h"

#if (USE_GTK)
#include <gtk/gtk.h>

#if ! GTK_CHECK_VERSION(2,22,0)
gint
gdk_visual_get_depth(GdkVisual * visual)
{
    return visual->depth;
}
#endif

#if ! GTK_CHECK_VERSION(2,18,0)
void
gtk_widget_get_allocation(GtkWidget * widget, GtkAllocation * allocation)
{
    *allocation = widget->allocation;
}

void
gtk_widget_set_allocation(GtkWidget * widget, const GtkAllocation * allocation)
{
    widget->allocation = *allocation;
}

void
gtk_cell_renderer_get_alignment(GtkCellRenderer * cell, gfloat * xalign, gfloat * yalign)
{
    *xalign = cell->xalign;
    *yalign = cell->yalign;
}

void
gtk_cell_renderer_set_padding(GtkCellRenderer * cell, gint xpad, gint ypad)
{
    cell->xpad = xpad;
    cell->ypad = ypad;
}
#endif

#if ! GTK_CHECK_VERSION(2,14,0)

GtkWidget *
gtk_dialog_get_action_area(GtkDialog * dialog)
{
    return (dialog->action_area);
}

GtkWidget *
gtk_dialog_get_content_area(GtkDialog * dialog)
{
    return (dialog->vbox);
}

GdkWindow *
gtk_widget_get_window(GtkWidget * widget)
{
    return (widget->window);
}

gdouble
gtk_adjustment_get_upper(GtkAdjustment * adjustment)
{
    return adjustment->upper;
}

void
gtk_adjustment_set_upper(GtkAdjustment * adjustment, gdouble upper)
{
    adjustment->upper = upper;
}

guchar *
gtk_selection_data_get_data(GtkSelectionData * data)
{
    return data->data;
}

#endif

#if ! GTK_CHECK_VERSION(2,12,0)
GtkTooltips *ptt;
#endif

extern GtkWidget *
get_statusbar_label(GtkStatusbar * statusbar)
{
#if GTK_CHECK_VERSION(2,20,0)
    return
        GTK_WIDGET(gtk_container_get_children
                   (GTK_CONTAINER(gtk_statusbar_get_message_area(GTK_STATUSBAR(statusbar))))->data);
#else
    return GTK_WIDGET(statusbar->label);
#endif
}


extern void
toolbar_set_orientation(GtkToolbar * toolbar, GtkOrientation orientation)
{
#if GTK_CHECK_VERSION(2,16,0)
    gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar), orientation);
#else
    gtk_toolbar_set_orientation(toolbar, orientation);
#endif
}

#endif
