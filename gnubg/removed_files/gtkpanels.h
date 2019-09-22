/*
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
 * $Id: gtkpanels.h,v 1.7 2013/06/16 02:16:16 mdpetch Exp $
 */

/* position of windows: main window, game list, and annotation */

#ifndef GTKPANELS_H
#define GTKPANELS_H

typedef enum _gnubgwindow {
    WINDOW_MAIN = 0,
    WINDOW_GAME,
    WINDOW_ANALYSIS,
    WINDOW_ANNOTATION,
    WINDOW_HINT,
    WINDOW_MESSAGE,
    WINDOW_COMMAND,
    WINDOW_THEORY,
    NUM_WINDOWS
} gnubgwindow;

typedef struct _windowgeometry {
    int nWidth, nHeight;
    int nPosX, nPosY, max;
} windowgeometry;

extern void SaveWindowSettings(FILE * pf);
extern void HidePanel(gnubgwindow window);
extern void getWindowGeometry(gnubgwindow window);
extern int PanelShowing(gnubgwindow window);
extern void ClosePanels(void);

extern int GetPanelSize(void);
extern void SetPanelWidth(int size);
extern void GTKGameSelectDestroy(void);

#endif
