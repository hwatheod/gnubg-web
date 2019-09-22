/* $Id: commands.inc,v 1.44 2015/01/25 17:32:08 plm Exp $ */

/* All the commands are in this file */

static command cER = {
    /* dummy command used for evaluation/rollout parameters */
    NULL, NULL, NULL, NULL, &cER
}, cFilename = {
    /* dummy command used for filename parameters */
    NULL, NULL, NULL, NULL, &cFilename
}, cOnOff = {
    /* dummy command used for on/off parameters */
    NULL, NULL, NULL, NULL, &cOnOff
}, cPlayer = {
    /* dummy command used for player parameters */
    NULL, NULL, NULL, NULL, &cPlayer
}, cPlayerBoth = {
    /* dummy command used for player parameters; "both" also permitted */
    NULL, NULL, NULL, NULL, &cPlayerBoth
}, cExportParameters = {
    /* dummy command used for export parameters */
    NULL, NULL, NULL, NULL, &cExportParameters
}, cExportMovesDisplay = {
    /* dummy command used for export moves to display */
    NULL, NULL, NULL, NULL, &cExportMovesDisplay
}, cExportCubeDisplay = {
    /* dummy command used for player cube to display */
    NULL, NULL, NULL, NULL, &cExportCubeDisplay
}, acAnalyseClear[] = {
    { "game", CommandAnalyseClearGame, 
      N_("Clear analysis for this game"), NULL, NULL },
    { "match", CommandAnalyseClearMatch, 
      N_("Clear analysis for entire match"), NULL, NULL },
    { "session", CommandAnalyseClearMatch, 
      N_("Clear analysis for entire session"), NULL, NULL },
    { "move", CommandAnalyseClearMove, 
      N_("Clear analysis for this move"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acAnalyseRollout[] = {
    { "cube", CommandAnalyseRolloutCube, 
      N_("Rollout an analysed cube"),
	      NULL, NULL },
    { "move", CommandAnalyseRolloutMove, 
      N_("Rollout choices in the analysis movelist.\n"
	 "\"analyse rollout move 1 2 3\" will rollout the first three choices.\n"
	 "\"analyse rollout move\" will rollout previously marked candidates."),
	      NULL, NULL },
    { "game", CommandAnalyseRolloutGame, 
      N_("Rollout marked candidates in the game"),
	      NULL, NULL },
    { "match", CommandAnalyseRolloutMatch, 
      N_("Rollout marked candidates in the match"),
	      NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acCmarkCubeSet[] = {
    { "rollout", CommandCMarkCubeSetRollout, N_("CMark cube for rollout"), NULL, NULL },
    { "none", CommandCMarkCubeSetNone, N_("CMark cube clear"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acCmarkMoveSet[] = {
    { "rollout", CommandCMarkMoveSetRollout, N_("CMark moves for rollout"), NULL, NULL },
    { "none", CommandCMarkMoveSetNone, N_("CMark move clear"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acCmarkCube[] = {
    { "clear", CommandCMarkCubeSetNone, N_("CMark cube clear"), NULL, NULL },
    { "set", NULL, N_("CMark cube"), NULL, acCmarkCubeSet },
    { "show", CommandCMarkCubeShow, N_("Show cmarks for cube"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acCmarkMatch[] = {
    { "clear", CommandCMarkMatchClear, N_("CMark move clear all"), NULL, NULL },
    { "show", CommandCMarkMatchShow, N_("Show cmarks for move"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acCmarkGame[] = {
    { "clear", CommandCMarkGameClear, N_("CMark move clear all"), NULL, NULL },
    { "show", CommandCMarkGameShow, N_("Show cmarks for move"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acCmarkMove[] = {
    { "clear", CommandCMarkMoveClear, N_("CMark move clear all"), NULL, NULL },
    { "set", NULL, N_("CMark moves"), NULL, acCmarkMoveSet },
    { "show", CommandCMarkMoveShow, N_("Show cmarks for move"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acCmark[] = {
    { "match", NULL, N_("CMark moves and cubes in match"), NULL, acCmarkMatch },
    { "game", NULL, N_("CMark moves and cubes in game"), NULL, acCmarkGame },
    { "move", NULL, N_("CMark moves in movelist"), NULL, acCmarkMove },
    { "cube", NULL, N_("CMark cube"), NULL, acCmarkCube },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetAutoSave[] = {
    { "rollout", CommandSetAutoSaveRollout, N_("Autosave during rollout"), szONOFF, &cOnOff },
    { "analysis", CommandSetAutoSaveAnalysis, N_("Autosave after each analysed game"), szONOFF, &cOnOff },
    { "confirm", CommandSetAutoSaveConfirmDelete, N_("Confirm deletion of autosaves"), szONOFF, &cOnOff },
    { "time", CommandSetAutoSaveTime, N_("Set how often to autosave in minutes"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acAnalyse[] = {
    { "clear", NULL, 
      N_("Clear previous analysis"), NULL, acAnalyseClear },
    { "game", CommandAnalyseGame, 
      N_("Compute analysis and annotate current game"),
      NULL, NULL },
    { "match", CommandAnalyseMatch, 
      N_("Compute analysis and annotate every game "
      "in the match"), NULL, NULL },
    { "move", CommandAnalyseMove, 
      N_("Compute analysis and annotate the current "
      "move"), NULL, NULL },
    { "rollout", NULL, 
      N_("Rollout analysis"), NULL, acAnalyseRollout },
    { "session", CommandAnalyseSession, 
      N_("Compute analysis and annotate every "
      "game in the session"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acEnd[] = {
{ "game", CommandEndGame, N_("Let the computer play to the end of the game"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acAnnotateClear[] = {
    { "comment", CommandAnnotateClearComment, 
      N_("Erase commentary about a move"),
      NULL, NULL },
    { "luck", CommandAnnotateClearLuck, 
      N_("Erase annotations for a dice roll"),
      NULL, NULL },
    { "skill", CommandAnnotateClearSkill, 
      N_("Erase skill annotations for a move"),
      NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

command acAnnotateMove[] = {
    { "comment", CommandAnnotateAddComment, N_("Add commentary about a move"), szCOMMENT, NULL },
    { "bad", CommandAnnotateBad, N_("Mark as bad"), NULL, NULL },
    { "clear", CommandAnnotateClearSkill, 
      N_("Remove annotations"), NULL, NULL },
    { "doubtful", CommandAnnotateDoubtful, N_("Mark as doubtful"), NULL, NULL },
    { "verybad", CommandAnnotateVeryBad, N_("Mark as very bad"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

static command acAnnotateRoll[] = {
    { "clear", CommandAnnotateClearLuck, 
      N_("Remove annotations"), NULL, NULL },
    { "lucky", CommandAnnotateLucky, 
      N_("Mark a lucky dice roll"), NULL, NULL },
    { "unlucky", CommandAnnotateUnlucky, N_("Mark an unlucky dice roll"),
      NULL, NULL },
    { "verylucky", CommandAnnotateVeryLucky, N_("Mark a very lucky dice roll"),
      NULL, NULL },
    { "veryunlucky", CommandAnnotateVeryUnlucky, 
      N_("Mark a very unlucky dice roll"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acAnnotate[] = {
    { "clear", NULL, N_("Clear annotation"), NULL, acAnnotateClear },
    { "move", CommandAnnotateMove, N_("Mark a move"), NULL, acAnnotateMove },
    { "roll", NULL, N_("Mark a roll"), NULL, acAnnotateRoll },
    { "cube", CommandAnnotateCube, N_("Mark a cube decision"), 
      NULL, acAnnotateMove },
    { "double", CommandAnnotateDouble, 
      N_("Mark a double"), NULL, acAnnotateMove },
    { "accept", CommandAnnotateAccept, N_("Mark an accept decision"), 
      NULL, acAnnotateMove },
    { "drop", CommandAnnotateDrop, N_("Mark a drop decision"), 
      NULL, acAnnotateMove },
    { "pass", CommandAnnotateDrop, N_("Mark a pass decision"), 
      NULL, acAnnotateMove },
    { "reject", CommandAnnotateReject, N_("Mark a reject decision"), 
      NULL, acAnnotateMove },
    { "resign", CommandAnnotateResign, N_("Mark a resign decision"), 
      NULL, acAnnotateMove },
    { "take", CommandAnnotateAccept, N_("Mark a take decision"), 
      NULL, acAnnotateMove },
    { NULL, NULL, NULL, NULL, NULL }
}, acClear[] = {
  { "cache", CommandClearCache, 
    N_("Clear evaluation cache"), NULL, NULL },
  { "hint", CommandClearHint, 
    N_("Clear analysis used for `hint'"), NULL, NULL },
  { "turn", CommandClearTurn, 
    N_("Clear initialized cube action and dice roll"), NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL }
}, acExportGame[] = {
    { "gam", CommandExportGameGam, N_("Records a log of the game in .gam "
      "format"), szFILENAME, &cFilename },
    { "snowietxt", CommandExportGameSnowieTxt, N_("Records a log of the game in Snowie .txt format"), szFILENAME, &cFilename },
    { "html", CommandExportGameHtml,
      N_("Records a log of the game in .html format"), szFILENAME,
      &cFilename },
    { "latex", CommandExportGameLaTeX, N_("Records a log of the game in LaTeX "
      "format"), szFILENAME, &cFilename },
    { "pdf", CommandExportGamePDF, N_("Records a log of the game in PDF "
      "format"), szFILENAME, &cFilename },
    { "ps", CommandExportGamePS, N_("Records a log of the game in PS "
      "format"), szFILENAME, &cFilename },
    { "text", CommandExportGameText, N_("Export a log of the game in text format"), 
      szFILENAME, &cFilename },
    { NULL, NULL, NULL, NULL, NULL }
}, acExportMatch[] = {
    { "mat", CommandExportMatchMat, N_("Records a log of the match in .mat "
      "format"), szFILENAME, &cFilename },
    { "snowietxt", CommandExportMatchSnowieTxt, N_("Records a log of the match in Snowie .txt format"), szFILENAME, &cFilename },
    { "html", CommandExportMatchHtml,
      N_("Records a log of the match in .html format"), szFILENAME,
      &cFilename },
    { "latex", CommandExportMatchLaTeX, 
      N_("Records a log of the match in LaTeX "
      "format"), szFILENAME, &cFilename },
    { "text", CommandExportMatchText, 
      N_("Records a log of the match in text format"), 
      szFILENAME, &cFilename },
    { "pdf", CommandExportMatchPDF, N_("Save the current match in "
      "PDF format"), szFILENAME, &cFilename },
    { "ps", CommandExportMatchPS, N_("Save the current match in "
      "PS format"), szFILENAME, &cFilename },
    { NULL, NULL, NULL, NULL, NULL }
}, acExportPosition[] = {
    { "gammonline", CommandExportPositionGammOnLine,
      N_("Save the current position in .html format "
         "(special for GammOnLine)"), 
      szFILENAME, &cFilename },
    { "gol2clipboard", CommandExportPositionGOL2Clipboard,
      N_("Copy the current position in .html format to clipboard"
         "(special for GammOnLine)"), 
      szFILENAME, &cFilename },
#if HAVE_PANGOCAIRO
    { "ps", CommandExportPositionPS, N_("Save the current position in "
      "PostScript format"), szFILENAME, &cFilename },
    { "pdf", CommandExportPositionPDF, N_("Save the current position in "
      "PDF format"), szFILENAME, &cFilename },
    { "svg", CommandExportPositionSVG, N_("Save the current position in "
      "SVG format"), szFILENAME, &cFilename },
#endif /* HAVE_PANGOCAIRO */
    { "html", CommandExportPositionHtml,
      N_("Save the current position in .html format"), 
      szFILENAME, &cFilename },
#if HAVE_LIBPNG
    { "png", CommandExportPositionPNG, N_("Save the current position in "
      "Portable Network Graphics (PNG) format"), szFILENAME, &cFilename },
#endif /* HAVE_LIBPNG */
    { "pos", CommandExportPositionJF, N_("Save the current position in .pos "
      "format"), szFILENAME, &cFilename },
    { "snowietxt", CommandExportPositionSnowieTxt,
      N_("Save the current position in Snowie .txt format"), 
      szFILENAME, &cFilename },
    { "text", CommandExportPositionText,
      N_("Save the current position in text format"), 
      szFILENAME, &cFilename },
    { NULL, NULL, NULL, NULL, NULL }
}, acExportSession[] = {
    { "html", CommandExportMatchHtml,
      N_("Records a log of the session in .html format"), szFILENAME,
      &cFilename },
    { "latex", CommandExportMatchLaTeX, N_("Records a log of the session in "
      "LaTeX format"), szFILENAME, &cFilename },
    { "text", CommandExportMatchText,
      N_("Records a log of the session in text format"),
      szFILENAME, &cFilename },
    { NULL, NULL, NULL, NULL, NULL }
}, acExport[] = {
    { "game", NULL, N_("Record a log of the game so far to a file"), NULL,
      acExportGame },
    { "htmlimages", CommandExportHTMLImages, N_("Generate images to be used "
      "by the HTML export commands"), szFILENAME, &cFilename },
    { "match", NULL, N_("Record a log of the match so far to a file"), NULL,
      acExportMatch },
    { "position", NULL, N_("Write the current position to a file"), NULL,
      acExportPosition },
    { "session", NULL, N_("Record a log of the session so far to a file"), 
      NULL, acExportSession },
    { NULL, NULL, NULL, NULL, NULL }
}, acFirst[] = {
  { "game", CommandFirstGame,
    N_("Goto first game of the match or session"), NULL, NULL },
  { "move", CommandFirstMove,
    N_("Goto first move of the current game"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acImport[] = {
    { "auto", CommandImportAuto, N_("Import from any known format"),
      szFILENAME, &cFilename },
    { "mat", CommandImportMat, N_("Import a Jellyfish match"), szFILENAME,
      &cFilename },
    { "gam", CommandImportMat, N_("Import a Jellyfish game"), szFILENAME,
      &cFilename },
    { "oldmoves", CommandImportOldmoves, N_("Import a FIBS oldmoves file"),
      szFILENAME, &cFilename },
    { "empire", CommandImportEmpire, N_("Import a GammonEmpire game file"),
      szFILENAME, &cFilename },
    { "party", CommandImportParty, N_("Import a PartyGammon game file"),
      szFILENAME, &cFilename },
    { "bgroom", CommandImportBGRoom, N_("Import a BGRoom game file"),
      szFILENAME, &cFilename },
    { "pos", CommandImportJF, 
      N_("Import a Jellyfish position file"), szFILENAME,
      &cFilename },
    { "sgg", CommandImportSGG, 
      N_("Import an SGG match"), szFILENAME, &cFilename },
    { "snowietxt", CommandImportSnowieTxt, 
      N_("Import a Snowie .txt match"), szFILENAME, &cFilename },
    { "tmg", CommandImportTMG, 
      N_("Import an TMG match"), szFILENAME, &cFilename },
    { NULL, NULL, NULL, NULL, NULL }
}, acList[] = {
    { "game", CommandListGame, N_("Show the moves made in this game"), NULL,
      NULL },
    { "match", CommandListMatch, 
      N_("Show the games played in this match"), NULL,
      NULL },
    { "session", CommandListMatch, N_("Show the games played in this session"),
      NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acLoad[] = {
    { "commands", CommandLoadCommands, N_("Read commands from a script file"),
      szFILENAME, &cFilename },
    { "game", CommandLoadGame, N_("Read a saved game from a file"), szFILENAME,
      &cFilename },
    { "match", CommandLoadMatch, 
      N_("Read a saved match from a file"), szFILENAME,
      &cFilename },
    { "position", CommandLoadPosition, 
      N_("Read a saved position from a file"), szFILENAME, &cFilename },
    { "python", CommandLoadPython,
      N_("Load a python script from a file"), szFILENAME, &cFilename },
    { "weights", CommandNotImplemented, 
      N_("Read neural net weights from a file"),
      szOPTFILENAME, &cFilename },
    { NULL, NULL, NULL, NULL, NULL }
}, acNew[] = {
    { "game", CommandNewGame, 
      N_("Start a new game within the current match or session"), NULL, NULL },
    { "match", CommandNewMatch, 
      N_("Play a new match to some number of points"), szOPTLENGTH, NULL },
    { "session", CommandNewSession, N_("Start a new (money) session"), NULL,
      NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acRelationalAdd[] = {
    { "match", CommandRelationalAddMatch,
      N_("Log the match to the external relational database"), 
      szQUIET, NULL },
    { NULL, NULL, NULL, NULL, NULL }    
}, acRelationalErase[] = {
    { "player", CommandRelationalErase, N_("Remove all statistics from one player "
			"in the relational database"), szNAME, NULL },
    { "allplayers", CommandRelationalEraseAll,
      N_("Remove all player statistics in the relational database"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acRelationalShow[] = {
    { "details", CommandRelationalShowDetails, 
      N_("Show details of the matches for a given player in the database"), 
      szNAME, NULL },
    { "players", CommandRelationalShowPlayers, 
      N_("Show a list of all the players in the database"), 
      NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }    
}, acRelational[] = {
    { "add", NULL, N_("Add to the external relational database"), NULL,
      acRelationalAdd },
    { "erase", NULL, N_("Remove from the external relational database"), NULL,
      acRelationalErase },
    { "select", CommandRelationalSelect, N_("Query the relational database"),
      szCOMMAND, NULL },
    { "setup", CommandRelationalSetup, N_("Setup database parameters"),
      szKEYVALUE, NULL },
    { "show", NULL, N_("Show information from the relational database"),
      NULL, acRelationalShow },
    { "test", CommandRelationalTest, 
      N_("Test the external relational database"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }    
}, acSave[] = {
    { "game", CommandSaveGame, N_("Record a log of the game so far to a "
      "file"), szFILENAME, &cFilename },
    { "match", CommandSaveMatch, 
      N_("Record a log of the match so far to a file"),
      szFILENAME, &cFilename },
    { "position", CommandSavePosition, N_("Record the current board position "
      "to a file"), szFILENAME, &cFilename },
    { "settings", CommandSaveSettings, N_("Use the current settings in future "
      "sessions"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

command acSetAnalysisPlayer[] = {
    { "analyse", CommandSetAnalysisPlayerAnalyse, 
      N_("Specify if this player is to be analysed"), szONOFF, &cOnOff },
    { NULL, NULL, NULL, NULL, NULL }
};

static command acSetAnalysisThreshold[] = {
    { "bad", CommandSetAnalysisThresholdBad, 
      N_("Specify the equity loss for a bad move"), szVALUE, NULL },
    { "doubtful", CommandSetAnalysisThresholdDoubtful, 
      N_("Specify the equity loss for a doubtful move"), szVALUE, NULL },
    { "lucky", CommandSetAnalysisThresholdLucky, 
      N_("Specify the equity gain for "
      "a lucky roll"), szVALUE, NULL },
    { "unlucky", CommandSetAnalysisThresholdUnlucky, 
      N_("Specify the equity loss "
      "for an unlucky roll"), szVALUE, NULL },
    { "verybad", CommandSetAnalysisThresholdVeryBad, 
      N_("Specify the equity loss "
      "for a very bad move"), szVALUE, NULL },
    { "verylucky", CommandSetAnalysisThresholdVeryLucky, 
      N_("Specify the equity "
      "gain for a very lucky roll"), szVALUE, NULL },
    { "veryunlucky", CommandSetAnalysisThresholdVeryUnlucky, N_("Specify the "
      "equity loss for a very unlucky roll"), szVALUE, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

command acSetEvaluation[] = {
    { "cubeful", CommandSetEvalCubeful, N_("Cubeful evaluations"), szONOFF,
      &cOnOff },
    { "deterministic", CommandSetEvalDeterministic, N_("Specify whether added "
      "noise is determined by position"), szONOFF, &cOnOff },
    { "noise", CommandSetEvalNoise, N_("Distort evaluations with noise"),
      szSTDDEV, NULL },
    { "plies", CommandSetEvalPlies, N_("Choose how many plies to look ahead"),
      szPLIES, NULL },
    { "prune", CommandSetEvalPrune,
      N_("use fast pruning networks"), szONOFF, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetPlayer[] = {
    { "chequerplay", CommandSetPlayerChequerplay, N_("Control chequerplay "
      "parameters when gnubg plays"), NULL, acSetEvalParam },
    { "cubedecision", CommandSetPlayerCubedecision, N_("Control cube decision "
      "parameters when gnubg plays"), NULL, acSetEvalParam },
    { "external", CommandSetPlayerExternal, N_("Have another process make all "
      "moves for a player"), szFILENAME, &cFilename },
    { "gnubg", CommandSetPlayerGNU, 
      N_("Have gnubg make all moves for a player"),
      NULL, NULL },
    { "human", CommandSetPlayerHuman, N_("Have a human make all moves for a "
      "player"), NULL, NULL },
    { "movefilter", CommandSetPlayerMoveFilter, 
      N_("Set parameters for choosing moves to evaluate"), 
      szFILTER, NULL},
    { "name", CommandSetPlayerName, 
      N_("Change a player's name"), szNAME, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetEvalParam[] = {
  { "type", CommandSetEvalParamType,
    N_("Specify type (evaluation or rollout)"), szER, &cER },
  { "evaluation", CommandSetEvalParamEvaluation,
    N_("Specify parameters for neural net evaluation"), NULL,
    acSetEvaluation },
  { "rollout", CommandSetEvalParamRollout,
    N_("Specify parameters for rollout"), NULL,
    acSetRollout },
  { NULL, NULL, NULL, NULL, NULL }
};

static command acSetAnalysis[] = {
    { "chequerplay", CommandSetAnalysisChequerplay, N_("Specify parameters "
      "for the analysis of chequerplay"), NULL, acSetEvalParam },
    { "cube", CommandSetAnalysisCube, N_("Select whether cube action will be "
      "analysed"), szONOFF, &cOnOff },
    { "cubedecision", CommandSetAnalysisCubedecision, N_("Specify parameters "
      "for the analysis of cube decisions"), NULL,
      acSetEvalParam },
    { "luck", CommandSetAnalysisLuck, N_("Select whether dice rolls will be "
      "analysed"), szONOFF, &cOnOff },
    { "luckanalysis", CommandSetAnalysisLuckAnalysis,
      N_("Specify parameters for the luck analysis"), NULL, acSetEvaluation },
    { "movefilter", CommandSetAnalysisMoveFilter, 
      N_("Set parameters for choosing moves to evaluate"), 
      szFILTER, NULL},
    { "moves", CommandSetAnalysisMoves, 
      N_("Select whether chequer play will be "
      "analysed"), szONOFF, &cOnOff },
    { "player", CommandSetAnalysisPlayer,
      N_("Player specific options"), szPLAYER, acSetAnalysisPlayer },
    { "threshold", NULL, N_("Specify levels for marking moves"), NULL,
      acSetAnalysisThreshold },
#if USE_GTK
    { "window", CommandSetAnalysisWindows, N_("Display window with analysis"),
      szONOFF, &cOnOff },
#endif
    { NULL, NULL, NULL, NULL, NULL }    
};

static command acSetAutomatic[] = {
    { "bearoff", CommandSetAutoBearoff, N_("Automatically bear off as many "
      "chequers as possible"), szONOFF, &cOnOff },
    { "crawford", CommandSetAutoCrawford, N_("Enable the Crawford game "
      "based on match score"), szONOFF, &cOnOff },
    { "doubles", CommandSetAutoDoubles, N_("Control automatic doubles "
      "during (money) session play"), szLIMIT, NULL },
    { "game", CommandSetAutoGame, N_("Select whether to start new games "
      "after wins"), szONOFF, &cOnOff },
    { "move", CommandSetAutoMove, N_("Select whether forced moves will be "
      "made automatically"), szONOFF, &cOnOff },
    { "roll", CommandSetAutoRoll, N_("Control whether dice will be rolled "
      "automatically"), szONOFF, &cOnOff },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetConfirm[] = {
    { "default", CommandSetConfirmDefault, N_("Set default answer to yes/no questions"), NULL, NULL },
    { "new", CommandSetConfirmNew, N_("Ask for confirmation before aborting "
      "a game in progress"), szONOFF, &cOnOff },
    { "save", CommandSetConfirmSave, N_("Ask for confirmation before "
      "overwriting existing files"), szONOFF, &cOnOff },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetCube[] = {
    { "center", CommandSetCubeCentre, N_("The U.S.A. spelling of `centre'"),
      NULL, NULL },
    { "centre", CommandSetCubeCentre, N_("Allow both players access to the "
      "cube"), NULL, NULL },
    { "owner", CommandSetCubeOwner, N_("Allow only one player to double"),
      szPLAYER, &cPlayerBoth },
    { "use", CommandSetCubeUse, 
      N_("Control use of the doubling cube"), szONOFF, &cOnOff },
    { "value", CommandSetCubeValue, 
      N_("Fix what the cube stake has been set to"),
      szVALUE, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetCubeEfficiencyRace[] = {
    { "factor", CommandSetCubeEfficiencyRaceFactor, 
      N_("Set cube efficiency race factor"),
      szVALUE, NULL },
    { "coefficient", CommandSetCubeEfficiencyRaceCoefficient, 
      N_("Set cube efficiency race coefficient"),
      szVALUE, NULL },
    { "min", CommandSetCubeEfficiencyRaceMin, 
      N_("Set cube efficiency race minimum value"),
      szVALUE, NULL },
    { "max", CommandSetCubeEfficiencyRaceMax, 
      N_("Set cube efficiency race maximum value"),
      szVALUE, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetCubeEfficiency[] = {
    { "os", CommandSetCubeEfficiencyOS, 
      N_("Set cube efficiency for one sided bearoff positions"),
      szVALUE, NULL },
    { "crashed", CommandSetCubeEfficiencyCrashed, 
      N_("Set cube efficiency for crashed positions"),
      szVALUE, NULL },
    { "contact", CommandSetCubeEfficiencyContact, 
      N_("Set cube efficiency for contact positions"),
      szVALUE, NULL },
    { "race", NULL, 
      N_("Set cube efficiency parameters for race positions"),
      szVALUE, acSetCubeEfficiencyRace },
    { NULL, NULL, NULL, NULL, NULL }
#if USE_GTK
};

command acSetGeometryValues[] = {
    { "width", CommandSetGeometryWidth, N_("set width of window"), 
      szVALUE, NULL },
    { "height", CommandSetGeometryHeight, N_("set height of window"),
      szVALUE, NULL },
    { "xpos", CommandSetGeometryPosX, N_("set x-position of window"),
      szVALUE, NULL },
    { "ypos", CommandSetGeometryPosY, N_("set y-position of window"),
      szVALUE, NULL },
    { "max", CommandSetGeometryMax, N_("set maximised state of window"),
      szVALUE, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

static command acSetGeometry[] = {
    { "analysis", CommandSetGeometryAnalysis,
      N_("set geometry of analysis window"), NULL, acSetGeometryValues },
    { "command", CommandSetGeometryCommand,
      N_("set geometry of command window"), NULL, acSetGeometryValues },
    { "game", CommandSetGeometryGame,
      N_("set geometry of game-list window"), NULL, acSetGeometryValues },
    { "hint", CommandSetGeometryHint,
      N_("set geometry of game-list window"), NULL, acSetGeometryValues },
    { "main", CommandSetGeometryMain,
      N_("set geometry of main window"), NULL, acSetGeometryValues },
    { "message", CommandSetGeometryMessage,
      N_("set geometry of message window"), NULL, acSetGeometryValues },
    { "theory", CommandSetGeometryTheory,
      N_("set geometry of theory window"), NULL, acSetGeometryValues },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetGUIAnimation[] = {
    { "blink", CommandSetGUIAnimationBlink,
      N_("Blink chequers being moves"), NULL, NULL },
    { "none", CommandSetGUIAnimationNone,
      N_("Do not animate moving chequers"), NULL, NULL },
    { "slide", CommandSetGUIAnimationSlide,
      N_("Slide chequers across board when moved"), NULL, NULL },
    { "speed", CommandSetGUIAnimationSpeed,
      N_("Specify animation rate for moving chequers"), szVALUE, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetGUIShowPips[] = {
    { "none", CommandSetGUIShowPipsNone,
      N_("Don't show pip counts under the board"), NULL, NULL },
    { "pips", CommandSetGUIShowPipsPips,
      N_("Show pip counts under the board"), NULL, NULL },
    { "epc", CommandSetGUIShowPipsEPC,
      N_("Show EPC under the board, or pip count if unavailable"), NULL, NULL },
    { "wastage", CommandSetGUIShowPipsWastage,
      N_("Show EPC and wastage under the board, or pip count if unavailable"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetGUI[] = {
    { "animation", NULL, N_("Control how moving chequers are displayed"), NULL,
      acSetGUIAnimation },
    { "beep", CommandSetGUIBeep, N_("Enable beeping on illegal input"),
      szONOFF, NULL },
    { "dicearea", CommandSetGUIDiceArea,
      N_("Show dice icon when human player on roll"), szONOFF, NULL },
    { "dragtargethelp", CommandSetGUIDragTargetHelp,
      N_("Show target help while dragging a chequer"), szONOFF, NULL },
    { "usestatspanel", CommandSetGUIUseStatsPanel,
      N_("Show statistics in a panel"), szONOFF, NULL },
    { "movelistdetail", CommandSetGUIMoveListDetail,
      N_("Show win/loss stats in move list"), szONOFF, NULL },
    { "grayedit", CommandSetGUIGrayEdit,
      N_("Gray board in edit mode"), szONOFF, NULL },
    { "highdiefirst", CommandSetGUIHighDieFirst,
      N_("Show the higher die on the left"), szONOFF, NULL },
    { "illegal", CommandSetGUIIllegal,
      N_("Permit dragging chequers to illegal points"), szONOFF, NULL },
    { "showids", CommandSetGUIShowIDs,
      N_("Show the position and match IDs above the board"), szONOFF, NULL },
    { "showpips", NULL,
      N_("Show the pip counts below the board"), NULL, acSetGUIShowPips },
    { "windowpositions", CommandSetGUIWindowPositions,
      N_("Save and restore window positions and sizes"), szONOFF, NULL },
    { NULL, NULL, NULL, NULL, NULL }
#endif
}, acSetMatchInfo[] = {
    { "annotator", CommandSetMatchAnnotator,
      N_("Record the name of the match commentator"), szOPTNAME, NULL },
    { "comment", CommandSetMatchComment,
      N_("Record miscellaneous notes about the match"), szOPTVALUE, NULL },
    { "date", CommandSetMatchDate,
      N_("Record the date when the match was played"), szOPTDATE, NULL },
    { "event", CommandSetMatchEvent,
      N_("Record the name of the event the match is from"), szOPTVALUE, NULL },
    { "place", CommandSetMatchPlace,
      N_("Record the location where the match was played"), szOPTVALUE, NULL },
    { "rating", CommandSetMatchRating,
      N_("Record the ratings of the players"), szPLAYEROPTRATING, &cPlayer },
    { "round", CommandSetMatchRound,
      N_("Record the round of the match within the event"), szOPTVALUE, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetOutput[] = {
    { "output", CommandSetOutputOutput,
      N_("Print to messages to stdout"),
      szONOFF, &cOnOff },
    { "matchpc", CommandSetOutputMatchPC,
      N_("Show match equities as percentages (on) or probabilities (off)"),
      szONOFF, &cOnOff },
    { "mwc", CommandSetOutputMWC, N_("Show output in MWC (on) or "
      "equity (off) (match play only)"), szONOFF, &cOnOff },
    { "rawboard", CommandSetOutputRawboard, N_("Give FIBS \"boardstyle 3\" "
      "output (on), or an ASCII board (off)"), szONOFF, &cOnOff },
    { "winpc", CommandSetOutputWinPC,
      N_("Show winning chances as percentages (on) or probabilities (off)"),
      szONOFF, &cOnOff },
    { "digits", CommandSetOutputDigits,
      N_("Set number of digits after the decimal point in outputs"),
      szVALUE, NULL},
    { "errorratefactor", CommandSetOutputErrorRateFactor,
      N_("The factor used for multiplying error rates"), szVALUE, NULL},
    { NULL, NULL, NULL, NULL, NULL }
};

command acSetRNG[] = {
    { "ansi", CommandSetRNGAnsi, N_("Use the ANSI C rand() (usually linear "
      "congruential) generator"), szOPTSEED, NULL },
    { "bbs", CommandSetRNGBBS, N_("Use the Blum, Blum and Shub generator"),
      szOPTMODULUSOPTSEED, NULL },
    { "bsd", CommandSetRNGBsd, N_("Use the BSD random() non-linear additive "
      "feedback generator"), szOPTSEED, NULL },
    { "file", CommandSetRNGFile, 
      N_("Read dice from file"), szFILENAME, NULL },
    { "isaac", CommandSetRNGIsaac, N_("Use the I.S.A.A.C. generator"), 
      szOPTSEED, NULL },
    { "manual", CommandSetRNGManual, 
      N_("Enter all dice rolls manually"), NULL, NULL },
    { "md5", CommandSetRNGMD5, N_("Use the MD5 generator"), szOPTSEED, NULL },
    { "mersenne", CommandSetRNGMersenne, 
      N_("Use the Mersenne Twister generator"),
      szOPTSEED, NULL },
    { "random.org", CommandSetRNGRandomDotOrg, 
      N_("Use random numbers fetched from <www.random.org>"),
      NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
}, acSetRolloutLatePlayer[] = {
    { "chequerplay", CommandSetRolloutPlayerLateChequerplay, 
      N_("Specify parameters "
         "for chequerplay during later plies of rollouts"), 
      NULL, acSetEvaluation },
    { "cubedecision", CommandSetRolloutPlayerLateCubedecision,
      N_("Specify parameters "
         "for cube decisions during later plies of rollouts"),
      NULL, acSetEvaluation },
    { "movefilter", CommandSetRolloutPlayerLateMoveFilter, 
      N_("Set parameters for choosing moves to evaluate"), 
      szFILTER, NULL},
    { NULL, NULL, NULL, NULL, NULL }
}, acSetRolloutLimit[] = {
    { "enable", CommandSetRolloutLimitEnable,
      N_("Stop rollouts when STD's are small enough"),
      szONOFF, &cOnOff },
    { "minimumgames", CommandSetRolloutLimitMinGames, 
      N_("Always rollout at least this many games"),
      szMINGAMES, NULL},
    { "maxerror", CommandSetRolloutMaxError,
      N_("Stop when all ratios |std/value| are less than this "),
      szMAXERR, NULL},
    { NULL, NULL, NULL, NULL, NULL }
},acSetRolloutJsd[] = {
  { "limit", CommandSetRolloutJsdLimit, 
    N_("Stop when equities differ by this many j.s.d.s"),
    szJSDS, NULL},
  { "minimumgames", CommandSetRolloutJsdMinGames,
      N_("Always rollout at least this many games"),
      szMINGAMES, NULL},
  { "stop", CommandSetRolloutJsdEnable,
    N_("Stop entire rollout when J.S.D.s are large enough"),
    szONOFF, &cOnOff },
  { NULL, NULL, NULL, NULL, NULL }
}, acSetRolloutPlayer[] = {
    { "chequerplay", CommandSetRolloutPlayerChequerplay, 
      N_("Specify parameters "
      "for chequerplay during rollouts"), NULL, acSetEvaluation },
    { "cubedecision", CommandSetRolloutPlayerCubedecision,
      N_("Specify parameters for cube decisions during rollouts"),
      NULL, acSetEvaluation },
    { "movefilter", CommandSetRolloutPlayerMoveFilter, 
      N_("Set parameters for choosing moves to evaluate"), 
      szFILTER, NULL},
    { NULL, NULL, NULL, NULL, NULL }
};

static command acSetRolloutBearoffTruncation[] = {
    { "exact", CommandSetRolloutBearoffTruncationExact, 
      N_("Truncate *cubeless* rollouts at exact bearoff database"),
      NULL, &cOnOff },
    { "onesided", CommandSetRolloutBearoffTruncationOS, 
      N_("Truncate *cubeless* rollouts when reaching "
         "one-sided bearoff database"),
      NULL, &cOnOff },
    { NULL, NULL, NULL, NULL, NULL }
};

command acSetRolloutLate[] = {
  { "chequerplay", CommandSetRolloutLateChequerplay, 
    N_("Specify parameters for chequerplay during later plies of rollouts"), 
    NULL, acSetEvaluation },
  { "cubedecision", CommandSetRolloutLateCubedecision, 
    N_("Specify parameters "
       "for cube decisions during later plies of rollouts"), 
    NULL, acSetEvaluation },
  { "enable", CommandSetRolloutLateEnable, 
    N_("Enable/Disable different evaluations for later plies of rollouts"), 
    szONOFF, &cOnOff },
  { "plies", CommandSetRolloutLatePlies, 
    N_("Change evaluations for later plies in rollouts"),
    szPLIES, NULL },
  { "movefilter", CommandSetRolloutLateMoveFilter, 
    N_("Set parameters for choosing moves to evaluate"), 
    szFILTER, NULL},
  { "player", CommandSetRolloutLatePlayer, 
    N_("Control eval parameters on later plies for each side individually"), 
    szPLAYER, acSetRolloutLatePlayer }, 
  { NULL, NULL, NULL, NULL, NULL }
}, acSetRollout[] = {
    { "bearofftruncation", NULL, 
      N_("Control truncation of rollout when reaching bearoff databases"),
      NULL, acSetRolloutBearoffTruncation },
    { "chequerplay", CommandSetRolloutChequerplay, N_("Specify parameters "
      "for chequerplay during rollouts"), NULL, acSetEvaluation },
    { "cubedecision", CommandSetRolloutCubedecision, N_("Specify parameters "
      "for cube decisions during rollouts"), NULL, acSetEvaluation },
	{ "cube-equal-chequer", CommandSetRolloutCubeEqualChequer,
	  N_("Use same rollout evaluations for cube and chequer play"),
      szONOFF, &cOnOff },
    { "cubeful", CommandSetRolloutCubeful, N_("Specify whether the "
      "rollout is cubeful or cubeless"), szONOFF, &cOnOff },
    { "initial", CommandSetRolloutInitial, 
      N_("Roll out as the initial position of a game"), szONOFF, &cOnOff },
    { "jsd", CommandSetRolloutJsd, 
      N_("Stop truncations based on j.s.d. of equities"),
      NULL, acSetRolloutJsd},
    {"later", CommandSetRolloutLate,
     N_("Control evaluation parameters for later plies of rollout"),
     NULL, acSetRolloutLate },
    {"limit", CommandSetRolloutLimit,
     N_("Stop rollouts based on Standard Deviations"),
     NULL, acSetRolloutLimit },
    {"log", CommandSetRolloutLogEnable,
     N_("Enable recording of rolled out games"),
     szONOFF, &cOnOff },
    {"logfile", CommandSetRolloutLogFile,
     N_("Set template file name for rollout .sgf files"),
     szFILENAME, NULL },
    { "movefilter", CommandSetRolloutMoveFilter, 
      N_("Set parameters for choosing moves to evaluate"), 
      szFILTER, NULL},
    { "player", CommandSetRolloutPlayer, 
      N_("Control evaluation parameters for each side individually"), 
      szPLAYER, acSetRolloutPlayer }, 
    { "players-are-same", CommandSetRolloutPlayersAreSame,
      N_("Use same settings for both players in rollouts"), szONOFF, &cOnOff },
    { "quasirandom", CommandSetRolloutRotate, 
      N_("Permute the dice rolls according to a uniform distribution"),
      szONOFF, &cOnOff },
    { "rng", CommandSetRolloutRNG, N_("Specify the random number "
      "generator algorithm for rollouts"), NULL, acSetRNG },
    { "rotate", CommandSetRolloutRotate, 
      N_("Synonym for `quasirandom'"), szONOFF, &cOnOff },
    { "seed", CommandSetRolloutSeed, N_("Specify the base pseudo-random seed "
      "to use for rollouts"), szOPTSEED, NULL },
    { "trials", CommandSetRolloutTrials, N_("Control how many rollouts to "
      "perform"), szTRIALS, NULL },
	{ "truncation", CommandSetRolloutTruncation, N_("Set parameters for "
      "truncating rollouts"), NULL, acSetTruncation },
    { "truncate-equal-player0", CommandSetRolloutTruncationEqualPlayer0,
      N_("Use player 0 settings for rollout evaluation at truncation point"),
      szONOFF, &cOnOff },
    { "varredn", CommandSetRolloutVarRedn, N_("Use lookahead during rollouts "
      "to reduce variance"), szONOFF, &cOnOff },
    /* FIXME add commands for cube variance reduction, settlements... */
    { NULL, NULL, NULL, NULL, NULL }
}, acSetTruncation[] = {
    { "chequerplay", CommandSetRolloutTruncationChequer,
	  N_("Set chequerplay evaluation when truncating a rollout"), NULL,
	  acSetEvaluation},
    { "cubedecision", CommandSetRolloutTruncationCube,
	  N_("Set cubedecision evaluation when truncating a rollout"), NULL,
	  acSetEvaluation},
    { "enable", CommandSetRolloutTruncationEnable, 
	N_("Enable/Disable truncated rollouts"), szONOFF, &cOnOff },
    { "plies", CommandSetRolloutTruncationPlies,
	N_("End rollouts at a particular depth"), szPLIES, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

static command acSetEval[] = {
  { "chequerplay", CommandSetEvalChequerplay,
    N_("Set evaluation parameters for chequer play"), NULL,
    acSetEvalParam },
  { "cubedecision", CommandSetEvalCubedecision,
    N_("Set evaluation parameters for cube decisions"), NULL,
    acSetEvalParam },
  { "movefilter", CommandSetEvalMoveFilter, 
    N_("Set parameters for choosing moves to evaluate"), 
    szFILTER, NULL},
  { "sameasanalysis", CommandSetEvalSameAsAnalysis, N_("Select if evaluation settings should be the "
	"same as the analysis setting"), szONOFF, &cOnOff },
  { NULL, NULL, NULL, NULL, NULL }    
};

command acSetExportParameters[] = {
  { "evaluation", CommandSetExportParametersEvaluation,
    N_("show detailed parameters for evaluations"), szONOFF, &cOnOff },
  { "rollout", CommandSetExportParametersRollout,
    N_("show detailed parameters for rollouts"), szONOFF, &cOnOff },
  { NULL, NULL, NULL, NULL, NULL }    
};

static command acSetExportHTMLType[] = {
  { "bbs", CommandSetExportHTMLTypeBBS,
    N_("Use BBS-type images for posting to, e.g., GammOnLine"), NULL, NULL },
  { "fibs2html", CommandSetExportHTMLTypeFibs2html,
    N_("Use fibs2html-type pictures"), NULL, NULL },
  { "gnu", CommandSetExportHTMLTypeGNU,
    N_("Default images"), NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetExportCSS[] = {
  { "head", CommandSetExportHTMLCSSHead,
    N_("Write CSS stylesheet in <head>"), NULL, NULL },
  { "external", CommandSetExportHTMLCSSExternal,
    N_("Write stylesheet to external file (\"gnubg.css\")"), NULL, NULL },
  { "inline", CommandSetExportHTMLCSSInline,
    N_("Write stylesheet inside tags"), NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetExportHTML[] = {
  { "css", NULL,
    N_("Control how the CSS stylesheet is written"), szVALUE, acSetExportCSS },
  { "pictureurl", CommandSetExportHTMLPictureURL,
    N_("set URL to pictures used in HTML export"), szURL, NULL },
  { "type", NULL,
    N_("set type of HTML boards used in HTML export"), NULL, 
    acSetExportHTMLType },
  { "size", CommandSetExportHtmlSize, 
      N_("Set size of board for HTML image export"), 
      szVALUE, NULL },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetExportMovesDisplay[] = {
  { "verybad", CommandSetExportMovesDisplayVeryBad,
    N_("show very bad moves"), szONOFF, &cOnOff },
  { "bad", CommandSetExportMovesDisplayBad,
    N_("show bad moves"), szONOFF, &cOnOff },
  { "doubtful", CommandSetExportMovesDisplayDoubtful,
    N_("show doubtful moves"), szONOFF, &cOnOff },
  { "unmarked", CommandSetExportMovesDisplayUnmarked,
    N_("show unmarked moves"), szONOFF, &cOnOff },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetExportCubeDisplay[] = {
  { "actual", CommandSetExportCubeDisplayActual,
    N_("show actual cube decisions"), szONOFF, &cOnOff },
  { "bad", CommandSetExportCubeDisplayBad,
    N_("show bad cube decisions"), szONOFF, &cOnOff },
  { "close", CommandSetExportCubeDisplayClose,
    N_("show close cube decisions"), szONOFF, &cOnOff },
  { "doubtful", CommandSetExportCubeDisplayDoubtful,
    N_("show doubtful cube decisions"), szONOFF, &cOnOff },
  { "missed", CommandSetExportCubeDisplayMissed,
    N_("show missed doubles"), szONOFF, &cOnOff },
  { "unmarked", CommandSetExportCubeDisplayUnmarked,
    N_("show unmarked cube decisions"), szONOFF, &cOnOff },
  { "verybad", CommandSetExportCubeDisplayVeryBad,
    N_("show very bad cube decisions"), szONOFF, &cOnOff },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetExportInclude[] = {
  { "annotations", CommandSetExportIncludeAnnotations,
    N_("include annotations"), szONOFF, &cOnOff },
  { "analysis", CommandSetExportIncludeAnalysis,
    N_("include analysis (evaluations/rollouts)"), szONOFF, &cOnOff },
  { "statistics", CommandSetExportIncludeStatistics,
    N_("include statistics (# of bad moves, # of jokers, etc)"), 
    szONOFF, &cOnOff },
  { "matchinfo", CommandSetExportIncludeMatchInfo,
    N_("include information about the match" ),
    szONOFF, &cOnOff },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetExportShow[] = {
  { "board", CommandSetExportShowBoard,
    N_("show board every [value] move (0 for never)"), szVALUE, NULL },
  { "player", CommandSetExportShowPlayer,
    N_("which player(s) to show"), szPLAYER, &cPlayerBoth },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetExportMoves[] = {
  { "number", CommandSetExportMovesNumber,
    N_("show at most [value] moves"), szVALUE, NULL },
  { "probabilities", CommandSetExportMovesProb,
    N_("show detailed probabilities"), szONOFF, &cOnOff },
  { "parameters", CommandSetExportMovesParameters,
    N_("show detailed rollout/evaluation parameters"), NULL, 
    acSetExportParameters },
  { "display", NULL, N_("when to show moves"), NULL, acSetExportMovesDisplay },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetExportCube[] = {
  { "probabilities", CommandSetExportCubeProb,
    N_("show detailed probabilities"), szONOFF, &cOnOff },
  { "parameters", CommandSetExportCubeParameters,
    N_("show detailed rollout/evaluation parameters"), NULL, 
    acSetExportParameters },
  { "display", NULL, N_("when to show moves"), NULL, acSetExportCubeDisplay },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetExportPNG[] = {
    { "size", CommandSetExportPNGSize,
      N_("Set size of board for PNG export"),
      szVALUE, NULL },
  { NULL, NULL, NULL, NULL, NULL }
}, acSetExport[] = {
  { "folder", CommandSetExportFolder, N_("Set default folder "
      "for export"), szFOLDER, &cFilename },
  { "html", NULL,
    N_("Set options for HTML export"), NULL, acSetExportHTML },
  { "include", NULL,
    N_("Control which blocks to include in exports"), 
    NULL, acSetExportInclude },
  { "png", NULL,
    N_("Set options for PNG export"), NULL, acSetExportPNG },
  { "show", NULL,
    N_("Control display of boards/players in exports"), NULL, acSetExportShow },
  { "moves", NULL,
    N_("Control display of moves in exports"), NULL, acSetExportMoves },
  { "cube", NULL,
    N_("Control display of cube in exports"), NULL, acSetExportCube },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetImport[] = {
  { "folder", CommandSetImportFolder, N_("Set default folder "
      "for import"), szFOLDER, &cFilename },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetInvert[] = {
  { "matchequitytable", CommandSetInvertMatchEquityTable,
    N_("invert match equity table"), szONOFF, &cOnOff },
  { "met", CommandSetInvertMatchEquityTable,
    N_("alias for 'set invert matchequitytable'"), szONOFF, &cOnOff },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetPriority[] = {
  { "abovenormal", CommandSetPriorityAboveNormal,
    N_("Set priority to above normal"), NULL, NULL },
  { "belownormal", CommandSetPriorityBelowNormal,
    N_("Set priority to below normal"), NULL, NULL },
  { "highest", CommandSetPriorityHighest, 
    N_("Set priority to highest"), NULL, NULL },
  { "idle", CommandSetPriorityIdle,
    N_("Set priority to idle"), NULL, NULL },
  { "nice", CommandSetPriorityNice, N_("Specify priority numerically"),
    szPRIORITY, NULL },
  { "normal", CommandSetPriorityNormal,
    N_("Set priority to normal"), NULL, NULL },
  { "timecritical", CommandSetPriorityTimeCritical,
    N_("Set priority to time critical"), NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL }
}, acSetSGF[] = {
  { "folder", CommandSetSGFFolder, N_("Set default folder "
      "for import"), szFOLDER, &cFilename },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetSoundSystem[] = {
  { "command", CommandSetSoundSystemCommand, 
    N_("Specify external command for playing sounds"), szCOMMAND, NULL },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetSoundSound[] = {
  { "agree", CommandSetSoundSoundAgree, 
    N_("Agree"), szOPTFILENAME, &cFilename },
  { "analysisfinished", CommandSetSoundSoundAnalysisFinished, 
    N_("Analysis is finished"), szOPTFILENAME, &cFilename },
  { "botfans", CommandSetSoundSoundBotDance, 
    N_("Bot fans"), szOPTFILENAME, &cFilename },
  { "botwinsgame", CommandSetSoundSoundBotWinGame, 
    N_("Bot wins game"), szOPTFILENAME, &cFilename },
  { "botwinsmatch", CommandSetSoundSoundBotWinMatch, 
    N_("Bot wins match"), szOPTFILENAME, &cFilename },
  { "chequer", CommandSetSoundSoundChequer, 
    N_("Chequer movement"), szOPTFILENAME, &cFilename },
  { "double", CommandSetSoundSoundDouble, 
    N_("Double"), szOPTFILENAME, &cFilename },
  { "drop", CommandSetSoundSoundDrop, 
    N_("Drop"), szOPTFILENAME, &cFilename },
  { "exit", CommandSetSoundSoundExit, 	 
    N_("Exiting of GNU Backgammon"), szOPTFILENAME, &cFilename },
  { "humanfans", CommandSetSoundSoundHumanDance, 
    N_("Human fans"), szOPTFILENAME, &cFilename },
  { "humanwinsgame", CommandSetSoundSoundHumanWinGame, 
    N_("Human wins game"), szOPTFILENAME, &cFilename },
  { "humanwinsmatch", CommandSetSoundSoundHumanWinMatch, 
    N_("Human wins match"), szOPTFILENAME, &cFilename },
  { "move", CommandSetSoundSoundMove, 
    N_("Move"), szOPTFILENAME, &cFilename },
  { "redouble", CommandSetSoundSoundRedouble, 
    N_("Redouble"), szOPTFILENAME, &cFilename },
  { "resign", CommandSetSoundSoundResign, 
    N_("Resign"), szOPTFILENAME, &cFilename },
  { "roll", CommandSetSoundSoundRoll, 
    N_("Roll"), szOPTFILENAME, &cFilename },
  { "start", CommandSetSoundSoundStart, 
    N_("Starting of GNU Backgammon"), szOPTFILENAME, &cFilename },
  { "take", CommandSetSoundSoundTake, 
    N_("Take"), szOPTFILENAME, &cFilename },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetSound[] = {
  { "sound", NULL, 
    N_("Set sound files"), NULL, acSetSoundSound }, 
  { "enable", CommandSetSoundEnable, 
    N_("Select whether sounds should be played or not"), szONOFF, &cOnOff },
  { "system", NULL, 
    N_("Select sound system"), NULL, acSetSoundSystem },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetTutorSkill[] = {
  { "doubtful", CommandSetTutorSkillDoubtful, N_("Warn about `doubtful' play"),
    NULL, NULL },
  { "bad", CommandSetTutorSkillBad, N_("Warn about `bad' play"),
    NULL, NULL },
  { "verybad", CommandSetTutorSkillVeryBad, N_("Warn about `very bad' play"),
    NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetTutor[] = {
  { "mode", CommandSetTutorMode, N_("Give advice on possible errors"),
    szONOFF, &cOnOff },
  { "cube", CommandSetTutorCube, N_("Include cube play in advice"),
    szONOFF, &cOnOff },
  { "chequer", CommandSetTutorChequer, 
    N_("Include chequer play in advice"),
    szONOFF, &cOnOff },
  { "skill", NULL, N_("Set level for tutor warnings"), 
    NULL, acSetTutorSkill },
  { NULL, NULL, NULL, NULL, NULL }    
}, acSetCheat[] = {
  { "enable", CommandSetCheatEnable,
   N_("Control whether GNU Backgammon is allowed to manipulate the dice"), 
    szONOFF, &cOnOff },
  { "player", CommandSetCheatPlayer,
   N_("Parameters for the dice manipulation"), szPLAYER, acSetCheatPlayer },
  { NULL, NULL, NULL, NULL, NULL }    
};

command acSetCheatPlayer[] = {
  { "roll", CommandSetCheatPlayerRoll,
   N_("Which roll should GNU Backgammon choose (1=Best and 21=Worst)"), 
    szVALUE, NULL },
  { NULL, NULL, NULL, NULL, NULL }    
};

static command acSetVariation[] = {
  { "1-chequer-hypergammon", CommandSetVariation1ChequerHypergammon,
    N_("Play 1-chequer hypergammon"), NULL, NULL },
  { "2-chequer-hypergammon", CommandSetVariation2ChequerHypergammon,
    N_("Play 2-chequer hypergammon"), NULL, NULL },
  { "3-chequer-hypergammon", CommandSetVariation3ChequerHypergammon,
    N_("Play 3-chequer hypergammon"), NULL, NULL },
  { "nackgammon", CommandSetVariationNackgammon,
    N_("Play standard backgammon with Nackgammon starting position"), 
    NULL, NULL },
  { "standard", CommandSetVariationStandard,
    N_("Play standard backgammon"), 
    NULL, NULL },
  { NULL, NULL, NULL, NULL, NULL }  
}, acSet[] = {
    { "analysis", NULL, N_("Control parameters used when analysing moves"),
      NULL, acSetAnalysis },
#if USE_GTK
    { "annotation", CommandSetAnnotation, N_("Select whether move analysis and "
      "commentary are shown"), szONOFF, &cOnOff },
#endif
    { "appearance", CommandSetAppearance, N_("Modify the look and feel of the "
      "graphical interface"), szKEYVALUE, NULL },
    { "automatic", NULL, N_("Perform certain functions without user input"),
      NULL, acSetAutomatic },
    { "beavers", CommandSetBeavers, 
      N_("Set whether beavers are allowed in money game or not"), 
      szVALUE, NULL },
    { "board", CommandSetBoard, N_("Set up the board in a particular "
      "position. Accepted formats are:\n"
      " set board =2 (sets the board to match the second position in the hint list.)\n"
      " set board simple 3 2 4 0 -3 -5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2\n"
      " set board 4PPgASjgc/ABMA (sets the board to match the position ID.)\n"
      " set board PPGPAABAAAPHNNAAAAAA (sets the board to match the gnubg-nn position string.)\n"
	      ), szPOSITION, NULL },
#ifndef WEB
    { "browser", CommandSetBrowser, 
      N_("Set web browser"), szOPTCOMMAND, NULL },
#endif /* WEB */
    { "cache", CommandSetCache, N_("Set the size of the evaluation cache"),
      szSIZE, NULL },
    { "calibration", CommandSetCalibration,
      N_("Specify the evaluation speed to be assumed for time estimates"),
      szOPTVALUE, NULL },
    { "cheat", NULL, 
      N_("Control GNU Backgammon's manipulation of the dice"),
      NULL, acSetCheat },
    { "clockwise", CommandSetClockwise, N_("Control the board orientation"),
      szONOFF, &cOnOff },
    { "colours", CommandSetAppearance, 
      N_("Synonym for `set appearance'"), NULL, NULL },
#if USE_GTK
    { "commandwindow", CommandSetCommandWindow, N_("Display command window"),
      szONOFF, &cOnOff },
#endif
    { "confirm", NULL, N_("Confirmation settings"), NULL, acSetConfirm },
    { "crawford", CommandSetCrawford, 
      N_("Set whether this is the Crawford game"), szONOFF, &cOnOff },
    { "cube", NULL, N_("Set the cube owner and/or value"), NULL, acSetCube },
    { "cubeefficiency", NULL, 
      N_("Set parameters for cube evaluations"), NULL, acSetCubeEfficiency },
    { "delay", CommandSetDelay, N_("Limit the speed at which moves are made"),
      szMILLISECONDS, NULL },
    { "dice", CommandSetDice, N_("Select the roll for the current move"),
      szDICE, NULL },
    { "display", CommandSetDisplay, 
      N_("Select whether the board is updated on the computer's turn"), 
      szONOFF, &cOnOff },
#if USE_GTK
    { "dockpanels", CommandSetDockPanels, N_("Dock or float windows"),
      szONOFF, &cOnOff },
#endif
    { "evaluation", NULL, N_("Control position evaluation "
      "parameters"), NULL, acSetEval },
    { "export", NULL, N_("Set settings for export"), NULL, acSetExport },
    { "fullscreen", CommandSetFullScreen, N_("Change to full screen mode"),
      szONOFF, &cOnOff },
#if USE_GTK
    { "gamelist", CommandSetGameList, N_("Display game window with moves"),
      szONOFF, &cOnOff },
    { "geometry", NULL, N_("Set geometry of windows"), NULL, acSetGeometry },
#endif
    { "gotofirstgame", CommandSetGotoFirstGame, 
      N_("Control whether you want to go to the first or last game "
         "when loading matches or sessions"), NULL, NULL },
#if USE_GTK
    { "gui", NULL, N_("Control parameters for the graphical interface"), NULL,
      acSetGUI },
#endif
    { "import", NULL, N_("Set settings for import"), NULL, acSetImport },
    { "sgf", NULL, N_("Set settings for sgf"), NULL, acSetSGF },
    { "invert", NULL, N_("Invert match equity table"), NULL, acSetInvert },
    { "jacoby", CommandSetJacoby, N_("Set whether to use the Jacoby rule in "
      "money games"), szONOFF, &cOnOff },
    { "defaultnames", CommandSetDefaultNames, N_("Set default names for players"),
	  szNAME, NULL },
    { "aliases", CommandSetAliases, N_("Set aliases for player 1 when importing MAT files"), szNAME, NULL },
    { "lang", CommandSetLang, N_("Set your language preference"),
      szLANG, NULL },
    { "matchequitytable", CommandSetMET,
      N_("Read match equity table from XML file"), szFILENAME, &cFilename },
    { "matchid", CommandSetMatchID, N_("set Match ID"), szMATCHID, NULL },
    { "gnubgid", CommandSetGNUBgID, N_("set GNUbg ID"), szGNUBGID, NULL },
    { "xgid", CommandSetXGID, N_("set GNUbg ID from XGID"), szXGID, NULL },
    { "matchinfo", NULL, N_("Record auxiliary match information"), NULL,
      acSetMatchInfo },
    { "matchlength", CommandSetMatchLength,
      N_("Specify the default length for new matches"), szLENGTH, NULL },
#if USE_GTK
	{ "message", CommandSetMessage, N_("Display window with messages"),
      szONOFF, &cOnOff },
#endif
    { "met", CommandSetMET,
      N_("Synonym for `set matchequitytable'"), szFILENAME, &cFilename },
    { "output", NULL, N_("Modify options for formatting results"), NULL,
      acSetOutput },
#if USE_GTK
    { "panels", CommandSetPanels, 
      N_("Display game list, annotation and message panels/windows"), 
	 szONOFF, &cOnOff },
#endif
    { "panelwidth", CommandSetPanelWidth, N_("Set the width of the docked panels"),
      szVALUE, NULL },
    { "player", CommandSetPlayer, N_("Change options for one or both "
      "players"), szPLAYER, acSetPlayer },
    { "postcrawford", CommandSetPostCrawford, 
      N_("Set whether this is a post-Crawford game"), szONOFF, &cOnOff },
    { "priority", NULL, N_("Set the priority of gnubg"), NULL, acSetPriority },
    { "prompt", CommandSetPrompt, N_("Customise the prompt gnubg prints when "
      "ready for commands"), szPROMPT, NULL },
    { "ratingoffset", CommandSetRatingOffset,
      N_("Set rating offset used for estimating abs. rating"),
      szVALUE, NULL },
    { "record", CommandSetRecord, N_("Set whether all games in a session are "
      "recorded"), szONOFF, &cOnOff },
    { "rng", CommandSetRNG, N_("Select the random number generator algorithm"), NULL,
      acSetRNG },
    { "rollout", CommandSetRollout, N_("Control rollout parameters"),
      NULL, acSetRollout }, 
    { "score", CommandSetScore, N_("Set the match or session score "),
      szSCORE, NULL },
    { "seed", CommandSetSeed, 
      N_("Set the dice generator seed"), szOPTSEED, NULL },
    { "sound", NULL, 
      N_("Control audio parameters"), NULL, acSetSound },
    { "styledgamelist", CommandSetStyledGameList, N_("Display colours for marked moves in game window"),
      szONOFF, &cOnOff },
#if USE_GTK
    { "theorywindow", CommandSetTheoryWindow, N_("Display game theory in window"),
      szONOFF, &cOnOff },
#endif
#if USE_MULTITHREAD
    { "threads", CommandSetThreads, N_("Set the number of calculation threads"),
      szSIZE, NULL },
#endif
    { "toolbar", CommandSetToolbar, N_("Change if icons and/or text are shown on toolbar"),
      szVALUE, NULL },
    { "turn", CommandSetTurn, N_("Set which player is on roll"), szPLAYER,
      &cPlayer },
    { "tutor", NULL, N_("Control tutor setup"), NULL, acSetTutor }, 
    { "variation", NULL, N_("Set which variation of backgammon is used"), 
      NULL, acSetVariation }, 
    { "vsync3d", CommandSetVsync3d, N_("Turn vertical sync on/off"), szONOFF, &cOnOff },
#if USE_GTK
    { "warning", CommandSetWarning, N_("Turn warnings on or off"), szWARNYN, NULL},
#endif
    { "autosave", NULL, N_("Control autosave"), NULL, acSetAutoSave },
    { NULL, NULL, NULL, NULL, NULL }
}, acShowStatistics[] = {
    { "game", CommandShowStatisticsGame, 
      N_("Compute statistics for current game"), NULL, NULL },
    { "match", CommandShowStatisticsMatch, 
      N_("Compute statistics for every game in the match"), NULL, NULL },
    { "session", CommandShowStatisticsSession, 
      N_("Compute statistics for every game in the session"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
},
#ifndef WEB
   acShowManual[] = {
    { "about", CommandShowManualAbout, N_("Show all about gnu backgammon in a web browser"), 
      NULL, NULL },
    { "web", CommandShowManualWeb, N_("Show manual in a web browser"), 
      NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
},
#endif /* WEB */
   acShow[] = {
    { "aliases", CommandShowAliases, N_("Show aliases for player 1 when importing MAT files"), NULL, NULL }, 
    { "analysis", CommandShowAnalysis, N_("Show parameters used for analysing "
      "moves"), NULL, NULL },
    { "autosave", CommandShowAutoSave, N_("Show parameters used for autosaving"), NULL, NULL },
    { "automatic", CommandShowAutomatic, N_("List which functions will be "
      "performed without user input"), NULL, NULL },
    { "beavers", CommandShowBeavers, 
      N_("Show whether beavers are allowed in money game or not"), 
      NULL, NULL },
    { "bearoff", CommandShowBearoff, 
      N_("Lookup data in various bearoff databases "), NULL, NULL },
    { "board", CommandShowBoard, 
      N_("Redisplay the board position"), szOPTPOSITION, NULL },
    { "buildinfo", CommandShowBuildInfo, 
      N_("Display details of this build of gnubg"), NULL, NULL },
#ifndef WEB
    { "browser", CommandShowBrowser, 
      N_("Display the currently used web browser"), NULL, NULL },
#endif /* WEB */
    { "cache", CommandShowCache, N_("Display statistics on the evaluation "
      "cache"), NULL, NULL },
    { "calibration", CommandShowCalibration,
      N_("Show the previously recorded evaluation speed"), NULL, NULL },
    { "cheat", CommandShowCheat,
      N_("Show parameters for dice manipulation"), NULL, NULL },
    { "clockwise", CommandShowClockwise, N_("Display the board orientation"),
      NULL, NULL },
    { "commands", CommandShowCommands, N_("List all available commands"),
      NULL, NULL },
    { "confirm", CommandShowConfirm, 
      N_("Show whether confirmation is required before aborting a game"), 
      NULL, NULL },
    { "copying", CommandShowCopying, N_("Conditions for redistributing copies "
      "of GNU Backgammon"), NULL, NULL },
    { "crawford", CommandShowCrawford, 
      N_("See if this is the Crawford game"), NULL, NULL },
    { "credits", CommandShowCredits, 
      N_("Display contributors to gnubg"), NULL, NULL },
    { "cube", CommandShowCube, N_("Display the current cube value and owner"),
      NULL, NULL },
    { "cubeefficiency", CommandShowCubeEfficiency, 
      N_("Show parameters for cube evaluations"), NULL, NULL },
    { "delay", CommandShowDelay, N_("See what the current delay setting is"), 
      NULL, NULL },
    { "dice", CommandShowDice, N_("See what the current dice roll is"), NULL,
      NULL },
    { "display", CommandShowDisplay, 
      N_("Show whether the board will be updated on the computer's turn"), 
      NULL, NULL },
    { "engine", CommandShowEngine, N_("Display the status of the evaluation "
      "engine"), NULL, NULL },
    { "evaluation", CommandShowEvaluation, N_("Display evaluation settings "
      "and statistics"), NULL, NULL },
    { "fullboard", CommandShowFullBoard, 
      N_("Redisplay the board position"), szOPTPOSITION, NULL },
    { "gammonvalues", CommandShowGammonValues, N_("Show gammon values"),
      NULL, NULL },
    { "export", CommandShowExport, N_("Show current export settings"), 
      NULL, NULL },
#if USE_GTK
    { "geometry", CommandShowGeometry, N_("Show geometry settings"), 
      NULL, NULL },
#endif
    { "jacoby", CommandShowJacoby, 
      N_("See if the Jacoby rule is used in money sessions"), NULL, NULL },
    { "8912", CommandShow8912, N_("Use 8912 rule to predict cube action"),
        szOPTPOSITION, NULL },
    { "keith", CommandShowKeith, N_("Calculate Keith Count for "
      "position"), szOPTPOSITION, NULL },
    { "kleinman", CommandShowKleinman, N_("Calculate Kleinman count for "
      "position"), szOPTPOSITION, NULL },
    { "lang", CommandShowLang, N_("Display your language preference"),
      NULL, NULL },
#ifndef WEB
    { "manual", NULL, N_("Show manual"), NULL, acShowManual },
#endif /* WEB */
    { "marketwindow", CommandShowMarketWindow, 
      N_("show market window for doubles"), NULL, NULL },
    { "matchequitytable", CommandShowMatchEquityTable, 
      N_("Show match equity table"), szOPTVALUE, NULL },
    { "matchinfo", CommandShowMatchInfo,
      N_("Display auxiliary match information"), NULL, NULL },
    { "matchlength", CommandShowMatchLength,
      N_("Show default match length"), NULL, NULL },
    { "matchresult", CommandShowMatchResult,
      N_("Show the actual and luck adjusted result for each game "
         "and the entire match"), NULL, NULL },
    { "met", CommandShowMatchEquityTable, 
      N_("Synonym for `show matchequitytable'"), szOPTVALUE, NULL },
    { "onesidedrollout", CommandShowOneSidedRollout, 
      N_("Show misc race theory"), NULL, NULL },
    { "output", CommandShowOutput, N_("Show how results will be formatted"),
      NULL, NULL },
#if USE_GTK
    { "panels", CommandShowPanels, N_("Show if the panels are displayed"),
      NULL, NULL},
#endif
    { "pipcount", CommandShowPipCount, 
      N_("Count the number of pips each player must move to bear off"), 
      szOPTPOSITION, NULL },
    { "player", CommandShowPlayer, N_("View per-player options"), NULL, NULL },
    { "postcrawford", CommandShowPostCrawford, 
      N_("See if this is post-Crawford play"), NULL, NULL },
    { "prompt", CommandShowPrompt, N_("Show the prompt that will be printed "
      "when ready for commands"), NULL, NULL },
    { "ratingoffset", CommandShowRatingOffset, N_("Show the rating offset "
      "used for estimating abs. rating"), NULL, NULL },
    { "rng", CommandShowRNG, N_("Display which random number generator "
      "is being used"), NULL, NULL },
    { "rollout", CommandShowRollout, N_("Display the evaluation settings used "
      "during rollouts"), NULL, NULL },
    { "rolls", CommandShowRolls, N_("Display evaluations for all rolls "),
      szOPTDEPTH, NULL },
    { "score", CommandShowScore, N_("View the match or session score "),
      NULL, NULL },
    { "scoresheet", CommandShowScoreSheet,
      N_("View the score sheet for the match or session"), NULL, NULL },
    { "seed", CommandShowSeed, N_("Show the dice generator seed"), 
      NULL, NULL },
    { "sound", CommandShowSound, N_("Show information abount sounds"), 
      NULL, NULL },
    { "statistics", NULL, N_("Show statistics"), NULL, acShowStatistics },
    { "temperaturemap", CommandShowTemperatureMap, 
      N_("Show temperature map (graphic overview of dice distribution)"), 
      NULL, NULL },
#if USE_MULTITHREAD
    { "threads", CommandShowThreads, N_("Show number of calculation threads"),
	NULL, NULL },
#endif
    { "thorp", CommandShowThorp, N_("Calculate Thorp Count for "
      "position"), szOPTPOSITION, NULL },
    { "turn", CommandShowTurn, 
      N_("Show which player is on roll"), NULL, NULL },
    { "version", CommandShowVersion, 
      N_("Describe this version of GNU Backgammon"), NULL, NULL },
    { "tutor", CommandShowTutor, 
      N_("Give warnings for possible errors in play"), NULL, NULL },
    { "variation", CommandShowVariation, 
      N_("Give information about which variation of backgammon is being played"),
      NULL, NULL },
#if USE_GTK
    { "warning", CommandShowWarning, N_("Show warning settings"), szWARN, NULL},
#endif
    { "warranty", CommandShowWarranty, 
      N_("Various kinds of warranty you do not have"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }    
}, acSwap[] = {
    { "players", CommandSwapPlayers, N_("Swap players"), NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

command acTop[] = {
    { "accept", CommandAccept, N_("Accept a cube or resignation"),
      NULL, NULL },
    { "agree", CommandAgree, N_("Agree to a resignation"), NULL, NULL },
    { "analyse", NULL, N_("Run analysis"), NULL, acAnalyse },
    { "analysis", NULL, NULL, NULL, acAnalyse },
    { "analyze", NULL, NULL, NULL, acAnalyse },
    { "annotate", NULL, N_("Record notes about a game"), NULL, acAnnotate },
    { "end", NULL, N_("Automatically make plays"), NULL, acEnd },
    { "beaver", CommandRedouble, N_("Synonym for `redouble'"), NULL, NULL },
#ifndef WEB
    { "calibrate", CommandCalibrate,
      N_("Measure evaluation speed, for later time estimates"), szOPTVALUE,
      NULL },
#endif /* WEB */
    { "clear", NULL, N_("Clear information"), NULL, acClear },
    { "cmark", NULL, N_("Mark candidates"), NULL, acCmark }, 
    { "copy", CommandCopy, N_("Copy current position to clipboard"), 
      NULL, NULL },
    { "decline", CommandDecline, N_("Decline a resignation"), NULL, NULL },
    { "dicerolls", CommandDiceRolls, N_("Generate a list of rolls"), 
      szTRIALS, NULL },
    { "double", CommandDouble, N_("Offer a double"), NULL, NULL },
    { "drop", CommandDrop, N_("Decline an offered double"), NULL, NULL },
    { "eq2mwc", CommandEq2MWC,
      N_("Convert normalised money equity to match winning chance"),
      szVALUE, NULL },
    { "eval", CommandEval, N_("Display evaluation of a position"), 
      szOPTPOSITION, NULL },
#ifndef WEB
    { "exit", CommandQuit, N_("Leave GNU Backgammon"), NULL, NULL },
#endif /* WEB */
    { "export", NULL, N_("Write data for use by other programs"), 
      NULL, acExport },
#ifndef WEB
    { "external", CommandExternal, N_("Make moves for an external controller"),
      szFILENAME, &cFilename },
#endif /* WEB */
    { "first", NULL, N_("Goto first move or game"),
      NULL, acFirst },
    { "help", CommandHelp, N_("Describe commands"), szOPTCOMMAND, NULL },
    { "hint", CommandHint,  
      N_("Give hints on cube action or best legal moves"), 
      szOPTVALUE, NULL },
#if HAVE_LIBREADLINE
    { "history", CommandHistory, N_("Display current history"), NULL, NULL },
#endif /* HAVE_LIBREADLINE */
    { "invert", NULL, N_("invert match equity tables, etc."), 
      NULL, acSetInvert },
    { "import", NULL, 
      N_("Import matches, games or positions from other programs"),
      NULL, acImport },
    { "list", NULL, N_("Show a list of games or moves"), NULL, acList },
    { "load", NULL, N_("Read data from a file"), NULL, acLoad },
    { "move", CommandMove, N_("Make a backgammon move"), szMOVE, NULL },
    { "mwc2eq", CommandMWC2Eq,
      N_("Convert match winning chance to normalised money equity"),
      szVALUE, NULL },
    { "n", CommandNext, NULL, szSTEP, NULL },
    { "new", NULL, N_("Start a new game, match or session"), NULL, acNew },
    { "next", CommandNext, N_("Step ahead within the game"), szSTEP, NULL },
    { "p", CommandPrevious, NULL, szSTEP, NULL },
    { "pass", CommandDrop, N_("Synonym for `drop'"), NULL, NULL },
    { "play", CommandPlay, N_("Force the computer to move"), NULL, NULL },
    { "previous", CommandPrevious, N_("Step backward within the game"), szSTEP,
      NULL },
#ifndef WEB
    { "quit", CommandQuit, N_("Leave GNU Backgammon"), NULL, NULL },
#endif /* WEB */
    { "r", CommandRoll, NULL, NULL, NULL },
    { "redouble", CommandRedouble, N_("Accept the cube one level higher "
      "than it was offered"), NULL, NULL },
    { "reject", CommandReject, N_("Reject a cube or resignation"), 
      NULL, NULL },
    { "relational", NULL, 
      N_("Log matches to an external relational database"), NULL,
      acRelational },
    { "resign", CommandResign, N_("Offer to end the current game"), szVALUE,
      NULL },
    { "roll", CommandRoll, N_("Roll the dice"), NULL, NULL },
    { "rollout", CommandRollout, 
      N_("Have gnubg perform rollouts of the current position."),
      szOPTPOSITION, NULL },
    { "save", NULL, N_("Write data to a file"), NULL, acSave },
    { "set", NULL, N_("Modify program parameters"), NULL, acSet },
    { "show", NULL, N_("View program parameters"), NULL, acShow },
    { "swap", NULL, N_("Swap players"), NULL, acSwap },
    { "take", CommandTake, N_("Agree to an offered double"), NULL, NULL },
    { "?", CommandHelp, N_("Describe commands"), szOPTCOMMAND, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

static command cTop = { NULL, NULL, NULL, NULL, acTop };
