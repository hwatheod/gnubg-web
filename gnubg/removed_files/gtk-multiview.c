/* gtk-multiview.c
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
 * $Id: gtk-multiview.c,v 1.16 2013/07/22 18:51:01 mdpetch Exp $
 */

/* License changed from the GNU LGPL to the GNU GPL (as permitted
 * under Term 3 of the GNU LGPL) by Gary Wong for distribution
 * with GNU Backgammon. */

#include "config.h"
#include "common.h"
#include "gtklocdefs.h"
#include <gtk/gtk.h>
#include "gtk-multiview.h"

G_DEFINE_TYPE(GtkMultiview, gtk_multiview, GTK_TYPE_CONTAINER)


static void
gtk_multiview_size_request(GtkWidget * widget, GtkRequisition * requisition);
static void
gtk_multiview_size_allocate(GtkWidget * widget, GtkAllocation * allocation);
static void
gtk_multiview_map(GtkWidget * widget);
static void
gtk_multiview_unmap(GtkWidget * widget);
static GType
gtk_multiview_child_type(GtkContainer * container);
static void
gtk_multiview_forall(GtkContainer * container,
                     gboolean include_internals, GtkCallback callback, gpointer callback_data);
static void
gtk_multiview_add(GtkContainer * widget, GtkWidget * child);
static void
gtk_multiview_remove(GtkContainer * widget, GtkWidget * child);

static void
gtk_multiview_init(GtkMultiview * multiview)
{
    gtk_widget_set_has_window(GTK_WIDGET(multiview), FALSE);

    multiview->current = NULL;
    multiview->children = NULL;
}

static void
gtk_multiview_class_init(GtkMultiviewClass * klass)
{
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;

    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    widget_class->size_request = gtk_multiview_size_request;
    widget_class->size_allocate = gtk_multiview_size_allocate;
    widget_class->map = gtk_multiview_map;
    widget_class->unmap = gtk_multiview_unmap;

    container_class->forall = gtk_multiview_forall;
    container_class->add = gtk_multiview_add;
    container_class->remove = gtk_multiview_remove;
    container_class->child_type = gtk_multiview_child_type;
}

static void
gtk_multiview_size_request(GtkWidget * widget, GtkRequisition * requisition)
{
    GList *tmp_list;
    GtkMultiview *multiview;
    GtkRequisition child_requisition;
    GtkWidget *child;

    multiview = GTK_MULTIVIEW(widget);

    requisition->width = 0;
    requisition->height = 0;
    /* We find the maximum size of all children widgets */
    tmp_list = multiview->children;
    while (tmp_list) {
        child = GTK_WIDGET(tmp_list->data);
        tmp_list = tmp_list->next;

        if (gtk_widget_get_visible(child)) {
            gtk_widget_size_request(child, &child_requisition);
            requisition->width = MAX(requisition->width, child_requisition.width);
            requisition->height = MAX(requisition->height, child_requisition.height);
            if (gtk_widget_get_mapped(child) && child != multiview->current) {
                gtk_widget_unmap(GTK_WIDGET(child));
            }
        }
    }
}

static void
gtk_multiview_size_allocate(GtkWidget * widget, GtkAllocation * allocation)
{
    GtkMultiview *multiview;
    GList *tmp_list;
    GtkWidget *child;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(GTK_IS_MULTIVIEW(widget));

    multiview = GTK_MULTIVIEW(widget);
    gtk_widget_set_allocation(widget, allocation);

    tmp_list = multiview->children;
    while (tmp_list) {
        child = GTK_WIDGET(tmp_list->data);
        tmp_list = tmp_list->next;

        if (gtk_widget_get_visible(child)) {
            gtk_widget_size_allocate(child, allocation);
        }
    }
}

static void
gtk_multiview_map(GtkWidget * widget)
{
    GtkMultiview *multiview;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(GTK_IS_MULTIVIEW(widget));

    multiview = GTK_MULTIVIEW(widget);
    gtk_widget_set_mapped(widget, TRUE);

    if (multiview->current && gtk_widget_get_visible(multiview->current) && !gtk_widget_get_mapped(multiview->current)) {
        gtk_widget_map(GTK_WIDGET(multiview->current));
    }
}

static void
gtk_multiview_unmap(GtkWidget * widget)
{
    GtkMultiview *multiview;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(GTK_IS_MULTIVIEW(widget));

    multiview = GTK_MULTIVIEW(widget);
    gtk_widget_set_mapped(widget, FALSE);

    if (multiview->current && gtk_widget_get_visible(multiview->current) && gtk_widget_get_mapped(multiview->current)) {
        gtk_widget_unmap(GTK_WIDGET(multiview->current));
    }
}

static GType
gtk_multiview_child_type(GtkContainer * UNUSED(container))
{
    return gtk_widget_get_type();
}

static void
gtk_multiview_forall(GtkContainer * container,
                     gboolean UNUSED(include_internals), GtkCallback callback, gpointer callback_data)
{
    GtkWidget *child;
    GtkMultiview *multiview;
    GList *tmp_list;

    g_return_if_fail(container != NULL);
    g_return_if_fail(GTK_IS_MULTIVIEW(container));
    g_return_if_fail(callback != NULL);

    multiview = GTK_MULTIVIEW(container);

    tmp_list = multiview->children;
    while (tmp_list) {
        child = GTK_WIDGET(tmp_list->data);
        tmp_list = tmp_list->next;
        (*callback) (GTK_WIDGET(child), callback_data);
    }
}

static void
gtk_multiview_add(GtkContainer * container, GtkWidget * child)
{
    g_return_if_fail(container != NULL);
    g_return_if_fail(GTK_IS_MULTIVIEW(container));
    g_return_if_fail(child != NULL);
    g_return_if_fail(GTK_IS_WIDGET(child));

    gtk_multiview_append_child(GTK_MULTIVIEW(container), child);
}

static void
gtk_multiview_remove(GtkContainer * container, GtkWidget * child)
{
    GtkMultiview *multiview;
    GList *list;

    g_return_if_fail(container != NULL);
    g_return_if_fail(GTK_IS_MULTIVIEW(container));
    g_return_if_fail(child != NULL);

    multiview = GTK_MULTIVIEW(container);

    list = g_list_find(multiview->children, child);
    g_return_if_fail(list != NULL);

    /* If we are mapped and visible, we want to deal with changing the page. */
    if ((gtk_widget_get_mapped(GTK_WIDGET(container))) &&
        (list->data == (gpointer) multiview->current) && (list->next != NULL)) {
        gtk_multiview_set_current(multiview, GTK_WIDGET(list->next->data));
    }

    multiview->children = g_list_remove(multiview->children, child);
    gtk_widget_unparent(child);
}

/* Public Functions */
GtkWidget *
gtk_multiview_new(void)
{
    return g_object_new(GTK_TYPE_MULTIVIEW, NULL);
}

void
gtk_multiview_prepend_child(GtkMultiview * multiview, GtkWidget * child)
{
    g_return_if_fail(multiview != NULL);
    g_return_if_fail(GTK_IS_MULTIVIEW(multiview));
    g_return_if_fail(child != NULL);
    g_return_if_fail(GTK_IS_WIDGET(child));

    gtk_multiview_insert_child(multiview, NULL, child);
}

void
gtk_multiview_insert_child(GtkMultiview * multiview, GtkWidget * back_child, GtkWidget * child)
{
    GList *list;

    g_return_if_fail(multiview != NULL);
    g_return_if_fail(GTK_IS_MULTIVIEW(multiview));
    g_return_if_fail(child != NULL);
    g_return_if_fail(GTK_IS_WIDGET(child));

    list = g_list_find(multiview->children, back_child);
    if (list == NULL) {
        multiview->children = g_list_prepend(multiview->children, child);
    } else {
        GList *new_el = g_list_alloc();

        new_el->next = list->next;
        new_el->prev = list;
        if (new_el->next)
            new_el->next->prev = new_el;
        new_el->prev->next = new_el;
        new_el->data = (gpointer) child;
    }
    gtk_widget_set_parent(GTK_WIDGET(child), GTK_WIDGET(multiview));

    if (gtk_widget_get_realized(GTK_WIDGET(multiview)))
        gtk_widget_realize(GTK_WIDGET(child));

    if (gtk_widget_get_visible(GTK_WIDGET(multiview)) && gtk_widget_get_visible(GTK_WIDGET(child))) {
        if (gtk_widget_get_mapped(GTK_WIDGET(child)))
            gtk_widget_unmap(GTK_WIDGET(child));
        gtk_widget_queue_resize(GTK_WIDGET(multiview));
    }

    /* if it's the first and only entry, we want to bring it to the foreground. */
    if (multiview->children->next == NULL)
        gtk_multiview_set_current(multiview, child);
}

void
gtk_multiview_append_child(GtkMultiview * multiview, GtkWidget * child)
{
    GList *list;

    g_return_if_fail(multiview != NULL);
    g_return_if_fail(GTK_IS_MULTIVIEW(multiview));
    g_return_if_fail(child != NULL);
    g_return_if_fail(GTK_IS_WIDGET(child));

    list = g_list_last(multiview->children);
    if (list) {
        gtk_multiview_insert_child(multiview, GTK_WIDGET(list->data), child);
    } else {
        gtk_multiview_insert_child(multiview, NULL, child);
    }
}

void
gtk_multiview_set_current(GtkMultiview * multiview, GtkWidget * child)
{
    GList *list;
    GtkWidget *old = NULL;

    g_return_if_fail(multiview != NULL);
    g_return_if_fail(GTK_IS_MULTIVIEW(multiview));
    g_return_if_fail(child != NULL);
    g_return_if_fail(GTK_IS_WIDGET(child));

    if (multiview->current == child)
        return;

    list = g_list_find(multiview->children, child);
    g_return_if_fail(list != NULL);

    if ((multiview->current) &&
        (gtk_widget_get_visible(multiview->current)) && (gtk_widget_get_mapped(GTK_WIDGET(multiview)))) {
        old = GTK_WIDGET(multiview->current);
    }

    multiview->current = GTK_WIDGET(list->data);
    if (gtk_widget_get_visible(multiview->current) && (gtk_widget_get_mapped(GTK_WIDGET(multiview)))) {
        gtk_widget_map(multiview->current);
    }
    if (old && gtk_widget_get_mapped(old))
        gtk_widget_unmap(old);
}
