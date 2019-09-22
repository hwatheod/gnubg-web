/*
 * gtkuidefs.h
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
 * $Id: gtkuidefs.h,v 1.3 2013/11/28 21:49:50 plm Exp $
 */

#define GNUBG_MAIN_UI \
	"<ui>" \
	  \
	  "<menubar name='MainMenu'>" \
	    "<menu name='FileMenu' action='FileMenuAction'>" \
	      "<menuitem name='New' action='FileNewAction' />" \
	      "<menuitem name='Open' action='FileOpenAction' />" \
	      "<menuitem name='Save' action='FileSaveAction' />" \
	      "<separator/>" \
	      "<menuitem name='CommandsOpen' action='FileCommandsOpenAction' />" \
	      "<separator/>" \
	      "<menuitem name='MatchInfo' action='FileMatchInfoAction' />" \
	      "<separator/>" \
	      "<menuitem name='Exit' action='FileExitAction' />" \
	    "</menu>" \
	    \
	    "<menu name='EditMenu' action='EditMenuAction'>" \
	      "<menuitem name='Undo' action='UndoAction' />" \
	      "<separator/>" \
	      \
	      "<menu name='CopyIDMenu' action='CopyIDMenuAction'>" \
	        "<menuitem name='GNUBGID' action='CopyGNUBGIDAction' />" \
	        "<menuitem name='MatchID' action='CopyMatchIDAction' />" \
	        "<menuitem name='PositionID' action='CopyPositionIDAction' />" \
	      "</menu>" \
	      \
	      "<menu name='CopyAsMenu' action='CopyAsMenuAction'>" \
	        "<menuitem name='PosAsAscii' action='CopyPosAsAsciiAction' />" \
	        "<menuitem name='GammOnLine' action='CopyAsGammOnLineAction' />" \
	      "</menu>" \
	      \
	      "<menuitem name='PasteID' action='PasteIDAction' />" \
	      "<separator/>" \
	      "<menuitem name='EditPosition' action='EditPositionAction' />" \
	    "</menu>" \
	    \
	    "<menu name='ViewMenu' action='ViewMenuAction'>" \
	    \
	      "<menu name='PanelsMenu' action='PanelsMenuAction'>" \
	        "<menuitem name='GameRecord' action='PanelGameRecordAction' />" \
	        "<menuitem name='Analysis' action='PanelAnalysisAction' />" \
	        "<menuitem name='Commentary' action='PanelCommentaryAction' />" \
	        "<menuitem name='Message' action='PanelMessageAction' />" \
	        "<menuitem name='Theory' action='PanelTheoryAction' />" \
	        "<menuitem name='Command' action='PanelCommandAction' />" \
	      "</menu>" \
	      \
	      "<menuitem name='DockPanels' action='DockPanelsAction' />" \
	      "<menuitem name='RestorePanels' action='RestorePanelsAction' />" \
 	     "<menuitem name='HidePanels' action='HidePanelsAction' />" \
	      "<separator/>" \
	      "<menuitem name='ShowIDStatusBar' action='ShowIDStatusBarAction' />" \
	      \
	      "<menu name='ToolBarMenu' action='ToolBarMenuAction'>" \
	        "<menuitem name='HideToolBar' action='HideToolBarAction' />" \
	        "<menuitem name='ShowToolBar' action='ShowToolBarAction' />" \
	        "<separator/>" \
	        "<menuitem name='TextOnly' action='TextOnlyToolBarAction' />" \
	        "<menuitem name='IconsOnly' action='IconsOnlyToolBarAction' />" \
	        "<menuitem name='Both' action='BothToolBarAction' />" \
	      "</menu>" \
	      \
	      "<menuitem name='FullScreen' action='FullScreenAction' />" \
	      "<separator/>" \
	      "<menuitem name='PlayClockwise' action='PlayClockwiseAction' />" \
	      "<separator/>" \
	      "<placeholder name='3DUIAddition' />" \
	    "</menu>" \
	    \
	    "<menu name='GameMenu' action='GameMenuAction'>" \
	      "<menuitem name='Roll' action='RollAction' />" \
	      "<menuitem name='FinishMove' action='FinishMoveAction' />" \
	      "<separator/>" \
	      "<menuitem name='Double' action='DoubleAction' />" \
	      "<menuitem name='Resign' action='ResignAction' />" \
	      "<separator/>" \
	      "<menuitem name='Accept' action='AcceptAction' />" \
	      "<menuitem name='Reject' action='RejectAction' />" \
	      "<separator/>" \
	      "<menuitem name='PlayComputerTurn' action='PlayComputerTurnAction' />" \
	      "<menuitem name='EndGame' action='EndGameAction' />" \
	      "<separator/>" \
	      "<menuitem name='SwapPlayers' action='SwapPlayersAction' />" \
	      "<separator/>" \
	      "<menuitem name='SetCube' action='SetCubeAction' />" \
	      "<menuitem name='SetDice' action='SetDiceAction' />" \
	      \
	      "<menu name='SetTurnMenu' action='SetTurnMenuAction'>" \
	        "<menuitem name='SetTurnPlayer0' action='SetTurnPlayer0Action' />" \
	        "<menuitem name='SetTurnPlayer1' action='SetTurnPlayer1Action' />" \
	      "</menu>" \
	      \
	      "<menuitem name='ClearTurn' action='ClearTurnAction' />" \
	    "</menu>" \
	    \
	    "<menu name='AnalyseMenu' action='AnalyseMenuAction'>" \
	      "<menuitem name='Evaluate' action='EvaluateAction' />" \
	      "<menuitem name='Hint' action='HintAction' />" \
	      "<menuitem name='Rollout' action='RolloutAction' />" \
	      "<separator/>" \
	      "<menuitem name='AnalyseMove' action='AnalyseMoveAction' />" \
	      "<menuitem name='AnalyseGame' action='AnalyseGameAction' />" \
	      "<menuitem name='AnalyseMatch' action='AnalyseMatchAction' />" \
	      "<separator/>" \
	      \
	      "<menu name='ClearAnalysisMenu' action='ClearAnalysisMenuAction'>" \
	        "<menuitem name='Move' action='ClearAnalysisMoveAction' />" \
	        "<menuitem name='Game' action='ClearAnalysisGameAction' />" \
	        "<menuitem name='MatchOrSession' action='ClearAnalysisMatchOrSessionAction' />" \
	      "</menu>" \
	      \
	      "<separator/>" \
	      \
	      "<menu name='CMarkMenu' action='CMarkMenuAction'>" \
	      \
	        "<menu name='CMarkCubeMenu' action='CMarkCubeMenuAction'>" \
	          "<menuitem name='Clear' action='CMarkCubeClearAction' />" \
	          "<menuitem name='Show' action='CMarkCubeShowAction' />" \
	        "</menu>" \
	        \
	        "<menu name='CMarkMoveMenu' action='CMarkMoveMenuAction'>" \
	          "<menuitem name='Clear' action='CMarkMoveClearAction' />" \
	          "<menuitem name='Show' action='CMarkMoveShowAction' />" \
	        "</menu>" \
	        \
	        "<menu name='CMarkGameMenu' action='CMarkGameMenuAction'>" \
	          "<menuitem name='Clear' action='CMarkGameClearAction' />" \
	          "<menuitem name='Show' action='CMarkGameShowAction' />" \
	        "</menu>" \
	        \
	        "<menu name='CMarkMatchMenu' action='CMarkMatchMenuAction'>" \
	          "<menuitem name='Clear' action='CMarkMatchClearAction' />" \
	          "<menuitem name='Show' action='CMarkMatchShowAction' />" \
	        "</menu>" \
	        \
	      "</menu>" \
	      \
	      "<separator/>" \
	      \
	      "<menu name='RolloutMenu' action='RolloutMenuAction'>" \
	          "<menuitem name='Cube' action='RolloutCubeAction' />" \
	          "<menuitem name='Move' action='RolloutMoveAction' />" \
	          "<menuitem name='Game' action='RolloutGameAction' />" \
	          "<menuitem name='Match' action='RolloutMatchAction' />" \
	      "</menu>" \
	      \
	      "<separator/>" \
	      "<menuitem name='BatchAnalyse' action='BatchAnalyseAction' />" \
	      "<separator/>" \
	      "<menuitem name='MatchOrSessionStats' action='MatchOrSessionStatsAction' />" \
	      "<separator/>" \
	      "<menuitem name='AddMatchOrSessionStatsToDB' action='AddMatchOrSessionStatsToDBAction' />" \
	      "<menuitem name='ShowRecords' action='ShowRecordsAction' />" \
	      "<separator/>" \
	      "<menuitem name='DistributionOfRolls' action='DistributionOfRollsAction' />" \
	      "<menuitem name='TemperatureMap' action='TemperatureMapAction' />" \
	      "<menuitem name='TemperatureMapCube' action='TemperatureMapCubeAction' />" \
	      "<separator/>" \
	      "<menuitem name='RaceTheory' action='RaceTheoryAction' />" \
	      "<separator/>" \
	      "<menuitem name='MarketWindow' action='MarketWindowAction' />" \
	      "<menuitem name='MatchEquityTable' action='MatchEquityTableAction' />" \
	      "<separator/>" \
	      "<menuitem name='EvaluationSpeed' action='EvaluationSpeedAction' />" \
	    "</menu>" \
	    \
	    "<menu name='SettingsMenu' action='SettingsMenuAction'>" \
	      "<menuitem name='Analysis' action='SettingsAnalysisAction' />" \
	      "<menuitem name='BoardAppearance' action='SettingsBoardAppearanceAction' />" \
	      "<menuitem name='Export' action='SettingsExportAction' />" \
	      "<menuitem name='Players' action='SettingsPlayersAction' />" \
	      "<menuitem name='Rollouts' action='SettingsRolloutsAction' />" \
	      "<separator/>" \
	      "<menuitem name='Options' action='SettingsOptionsAction' />" \
	      "<menuitem name='Language' action='SettingsLanguageAction' />" \
	    "</menu>" \
	    \
	    "<menu name='GoMenu' action='GoMenuAction'>" \
	      "<menuitem name='PreviousMarkedMove' action='GoPreviousMarkedMoveAction' />" \
	      "<menuitem name='PreviousCMarkedMove' action='GoPreviousCMarkedMoveAction' />" \
	      "<menuitem name='PreviousRoll' action='GoPreviousRollAction' />" \
	      "<menuitem name='PreviousGame' action='GoPreviousGameAction' />" \
	      "<separator/>" \
	      "<menuitem name='NextGame' action='GoNextGameAction' />" \
	      "<menuitem name='NextRoll' action='GoNextRollAction' />" \
	      "<menuitem name='NextCMarkedMove' action='GoNextCMarkedMoveAction' />" \
	      "<menuitem name='NextMarkedMove' action='GoNextMarkedMoveAction' />" \
	    "</menu>" \
	    \
	    "<menu name='HelpMenu' action='HelpMenuAction'>" \
	      "<menuitem name='Commands' action='HelpCommandsAction' />" \
	      "<separator/>" \
	      "<menuitem name='ManualAllAbout' action='HelpManualAllAboutAction' />" \
	      "<menuitem name='ManualWeb' action='HelpManualWebAction' />" \
	      "<separator/>" \
	      "<menuitem name='AboutGNUBG' action='HelpAboutGNUBGAction' />" \
	    "</menu>" \
	    \
	  "</menubar>" \
	  \
	  "<toolbar name='MainToolBar' action='MainToolBarAction'>" \
	    "<toolitem name='New' action='FileNewAction'/>" \
	    "<toolitem name='Open' action='FileOpenAction'/>" \
	    "<toolitem name='Save' action='FileSaveAction'/>" \
	    "<separator/>" \
	    "<toolitem name='Accept' action='AcceptAction' />" \
	    "<toolitem name='Reject' action='RejectAction' />" \
	    "<toolitem name='Double' action='DoubleAction' />" \
	    "<toolitem name='Resign' action='ResignAction' />" \
	    "<toolitem name='EndGame' action='EndGameAction' />" \
	    "<separator/>" \
	    "<toolitem name='Undo' action='UndoAction' />" \
	    "<toolitem name='Hint' action='HintAction' />" \
	    "<toolitem name='EditPosition' action='EditPositionAction' />" \
	    "<toolitem name='PlayClockwise' action='PlayClockwiseAction' />" \
	    "<separator expand='true'/>" \
	    "<toolitem name='PreviousMarkedMove' action='GoPreviousMarkedMoveAction' />" \
	    "<toolitem name='PreviousCMarkedMove' action='GoPreviousCMarkedMoveAction' />" \
	    "<toolitem name='PreviousRoll' action='GoPreviousRollAction' />" \
	    "<toolitem name='PreviousGame' action='GoPreviousGameAction' />" \
	    "<toolitem name='NextGame' action='GoNextGameAction' />" \
	    "<toolitem name='NextRoll' action='GoNextRollAction' />" \
	    "<toolitem name='NextCMarkedMove' action='GoNextCMarkedMoveAction' />" \
	    "<toolitem name='NextMarkedMove' action='GoNextMarkedMoveAction' />" \
	  "</toolbar>" \
	  \
	"</ui>"

#define  UIADDITIONS3D \
	"<ui>" \
        "<menubar name='MainMenu'>" \
        "       <menu name='ViewMenu' action='ViewMenuAction'>" \
        "               <menuitem name='SwitchMode' action='SwitchModeAction'/>" \
        "       </menu>" \
        "</menubar>" \
        "</ui>"
