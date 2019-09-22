/*
 * gtkgame.h
 *
 * by Gary Wong <gtw@gnu.org>, 2000, 2002.
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
 * $Id: gtkgame.h,v 1.154 2013/09/13 19:32:30 plm Exp $
 */

#ifndef GTKGAME_H
#define GTKGAME_H

#include <gtk/gtk.h>
#include "backgammon.h"
#include "gtkpanels.h"
#include "gtkchequer.h"

#define TOOLBAR_ACTION_OFFSET 10000

#if (USE_GTKUIMANAGER)
extern GtkUIManager *puim;
#endif

extern GdkColor wlCol;
extern GtkAccelGroup *pagMain;

extern GtkWidget *pwAnalysis;
extern GtkWidget *pwBoard;
extern GtkWidget *pwCommentary;
extern GtkWidget *pwGrab;
extern GtkWidget *pwMain;
extern GtkWidget *pwMessageText;
extern GtkWidget *pwMoveAnalysis;
extern GtkWidget *pwOldGrab;
extern GtkWidget *pwPanelVbox;
extern GtkWidget *pwToolbar;
extern GtkWidget *pwGnubgID;
extern guint nNextTurn;         /* GTK idle function */
extern int fEndDelay;
extern int fNeedPrompt;
extern int frozen;
extern int fTTY;
extern int fX;
extern int fToolbarShowing;
extern int inCallback;
extern unsigned int nDelay;
extern GtkWidget *hpaned;
extern GtkWidget *pom;
extern moverecord *pmrCurAnn;

extern char *ReturnHits(TanBoard anBoard);
extern gboolean ShowGameWindow(void);
extern gint MoveListClearSelection(GtkWidget * pw, GdkEventSelection * pes, hintdata * phd);
extern gint NextTurnNotify(gpointer p);
extern GList *MoveListGetSelectionList(const hintdata * phd);
extern GtkWidget *GetPanelWidget(gnubgwindow window);
extern GtkWidget *GL_Create(void);
extern int ArePanelsDocked(void);
extern int ArePanelsShowing(void);
extern int DockedPanelsShowing(void);
extern int edit_new(unsigned int length);
extern int GetPanelWidth(gnubgwindow panel);
extern int GTKGetManualDice(unsigned int an[2]);
extern int GTKGetMove(int anMove[8]);
extern int GtkTutor(char *sz);
extern int IsPanelDocked(gnubgwindow window);
extern int IsPanelShowVar(gnubgwindow panel, void *p);
extern int SetMainWindowSize(void);
extern move *MoveListGetMove(const hintdata * phd, GList * pl);
extern void CommentaryChanged(GtkWidget * pw, GtkTextBuffer * buffer);
extern void DisplayWindows(void);
extern void DockPanels(void);
extern void FullScreenMode(int state);
extern void GetFullscreenWindowSettings(int *panels, int *ids, int *maxed);
extern void GetStyleFromRCFile(GtkStyle ** ppStyle, const char *name, GtkStyle * psBase);
extern void GL_Freeze(void);
extern void GL_SetNames(void);
extern void GL_Thaw(void);
extern void GTKAddGame(moverecord * pmr);
extern void GTKAddMoveRecord(moverecord * pmr);
extern void GTKAllowStdin(void);
extern void GTKBearoffProgress(int i);
extern void GTKCalibrationEnd(void *context);
extern void *GTKCalibrationStart(void);
extern void GTKCalibrationUpdate(void *context, float rEvalsPerSec);
extern void GtkChangeLanguage(void);
extern void GTKClearMoveRecord(void);
extern void GTKCommandShowCredits(GtkWidget * pw, GtkWidget * parent);
extern void GTKCubeHint(moverecord * pmr, const matchstate * pms, int did_double, int did_take, int hist);
extern void GTKDelay(void);
extern void GTKDisallowStdin(void);
extern void GTKDumpStatcontext(int game);
extern void GTKEval(char *szOutput);
extern void GTKFreeze(void);
extern void GTKHelp(char *sz);
extern void GTKHint(moverecord * pmr, int hist);
extern void GTKMatchInfo(void);
extern void GTKNew(void);
extern void GTKOutput(const char *sz);
extern void GTKOutputErr(const char *sz);
extern void GTKOutputNew(void);
extern void GTKOutputX(void);
extern void GTKPopGame(int c);
extern void GTKPopMoveRecord(moverecord * pmr);
extern void GTKProgressEnd(void);
extern void GTKProgressStart(const char *sz);
extern void GTKProgressStartValue(char *sz, int iMax);
extern void GTKProgressValue(int fValue, int iMax);
extern void GTKProgress(void);
extern void GTKRecordShow(FILE * pfIn, char *sz, char *szPlayer);
extern void GTKRegenerateGames(void);
extern void GTKResign(gpointer p, guint n, GtkWidget * pw);
extern void GTKResignHint(float arOutput[], float rEqBefore, float rEqAfter, cubeinfo * pci, int fMWC);
extern void GTKResumeInput(void);
extern void GTKSaveSettings(void);
extern void GTKSetCube(gpointer p, guint n, GtkWidget * pw);
extern void GTKSetDice(gpointer p, guint n, GtkWidget * pw);
extern void GTKSetGame(int i);
extern void GTKSetMoveRecord(moverecord * pmr);
extern void GTKSet(void *p);
extern void GTKShowBuildInfo(GtkWidget * pw, GtkWidget * pwParent);
extern void GTKShowCalibration(void);
extern void GTKShowScoreSheet(void);
extern void GTKShowVersion(void);
extern void GTKSuspendInput(void);
extern void GTKTextToClipboard(const char *sz);
extern void GTKTextWindow(const char *szOutput, const char *title, const int type, GtkWidget * parent);
extern void GTKThaw(void);
extern void HideAllPanels(gpointer p, guint n, GtkWidget * pw);
extern void HintDoubleClick(GtkTreeView * treeview, GtkTreePath * path, GtkTreeViewColumn * col, hintdata * phd);
extern void HintSelect(GtkTreeSelection * selection, hintdata * phd);
extern void InitGTK(int *argc, char ***argv);
extern void MoveListCreate(hintdata * phd);
extern void MoveListFreeSelectionList(GList * pl);
extern void MoveListShowToggledClicked(GtkWidget * pw, hintdata * phd);
extern void MoveListUpdate(const hintdata * phd);
extern void OK(GtkWidget * pw, int *pf);
extern void PanelHide(gnubgwindow panel);
extern void PanelShow(gnubgwindow panel);
extern void RefreshGeometries(void);
extern void RunGTK(GtkWidget * pwSplash, char *commands, char *python_script, char *match);
extern void SetAnnotation(moverecord * pmr);
extern void SetAnalysis(gpointer p, guint n, GtkWidget * pw);
extern void SetMET(GtkWidget * pw, gpointer p);
extern void SetPanelWidget(gnubgwindow window, GtkWidget * pWin);
extern void SetRollouts(gpointer p, guint n, GtkWidget * pw);
extern void SetToolbarStyle(int value);
extern void setWindowGeometry(gnubgwindow window);
extern void ShowAllPanels(gpointer p, guint n, GtkWidget * pw);
extern void ShowHidePanel(gnubgwindow panel);
extern void ShowList(char *asz[], const char *szTitle, GtkWidget * parent);
extern void ShowMove(hintdata * phd, const int f);
extern void SwapBoardToPanel(int ToPanel, int updateEvents);
extern void DoHideAllPanels(int updateEvents);
#if (USE_GTKUIMANAGER)
extern void ToggleEdit(GtkToggleAction * action, gpointer user_data);
extern void ToggleClockwise(GtkToggleAction * action, gpointer user_data);
extern void ToggleDockPanels(GtkToggleAction * action, gpointer user_data);
#else
extern void ToggleDockPanels(gpointer p, guint n, GtkWidget * pw);
#endif
extern void GTKUndo(void);
extern void UserCommand(const char *sz);
extern void ShowToolbar(void);
extern void HideToolbar(void);
extern void MoveListDestroy(void);

#if HAVE_LIBREADLINE
extern int fReadingCommand;
extern void ProcessInput(char *sz);
#endif

#if USE_BOARD3D
extern void SetSwitchModeMenuText(void);
extern gboolean gtk_gl_init_success;
#endif

#endif
