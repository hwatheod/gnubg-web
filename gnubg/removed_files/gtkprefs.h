/*
 * gtkprefs.h
 *
 * by Gary Wong <gtw@gnu.org>, 2000.
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
 * $Id: gtkprefs.h,v 1.17 2015/01/01 16:42:05 plm Exp $
 */

#ifndef GTKPREFS_H
#define GTKPREFS_H

#include "gtkboard.h"

extern void BoardPreferences(GtkWidget * pwBoard);
extern void SetBoardPreferences(GtkWidget * pwBoard, char *sz);
extern void Default3dSettings(BoardData * bd);
extern void UpdatePreview(void);
extern void gtk_color_button_get_array(GtkColorButton * button, float array[4]);
extern void gtk_color_button_set_from_array(GtkColorButton * button, float array[4]);

extern GtkWidget *pwPrevBoard;

#endif
