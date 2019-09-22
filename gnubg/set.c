/*
 * set.c
 *
 * by Gary Wong <gtw@gnu.org>, 1999, 2000, 2001, 2002.
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
 * $Id: set.c,v 1.394 2015/02/08 15:50:56 plm Exp $
 */

#include "config.h"
#ifndef WEB
#include "gtklocdefs.h"
#endif                          /* WEB */

#ifdef WIN32
/* Needed for thread priority defines */
#include <winsock2.h>
#include <windows.h>
#endif

#include <glib.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif                          /* HAVE_SYS_RESOURCE_H */

#if HAVE_SOCKETS
#ifndef WIN32

#if HAVE_SYS_SOCKET_H
#include <sys/types.h>
#include <sys/socket.h>
#endif                          /* HAVE_SYS_SOCKET_H */

#include <stdio.h>
#include <string.h>

#else                           /* #ifndef WIN32 */
#include <winsock2.h>
#endif                          /* #ifndef WIN32 */
#endif                          /* #if HAVE_SOCKETS */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif                          /* HAVE_UNISTD_H */

#include "backgammon.h"
#include "dice.h"
#include "eval.h"
#include "external.h"
#include "export.h"

#if USE_GTK
#include "gtkgame.h"
#include "gtkprefs.h"
#include "gtkchequer.h"
#include "gtkwindows.h"
#endif                          /* USE_GTK */

#include "matchequity.h"
#include "positionid.h"
#include "matchid.h"
#include "renderprefs.h"
#include "drawboard.h"
#include "format.h"
#include "boarddim.h"
#include "sound.h"
#ifndef WEB
#include "openurl.h"
#endif                          /* WEB */

#if USE_BOARD3D
#include "fun3d.h"
#endif
#include "multithread.h"

static int iPlayerSet, iPlayerLateSet;

static evalcontext *pecSet;
static char *szSet;
static const char *szSetCommand;
static rolloutcontext *prcSet;

static evalsetup *pesSet;

static rng *rngSet;
static rngcontext *rngctxSet;

#if !HAVE_LIBGMP

/* get the next token from the input and convert as an
 * integer. Returns 0 on empty input or non-numerics found, and
 * 1 on success. On failure, one token (if any were available)
 * will have been consumed, it is not pushed back into the input.
 * Unsigned long is returned in pretVal
 */
static gboolean
ParseULong(char **ppch, unsigned long *pretVal)
{

    char *pch, *pchOrig;

    if (!ppch || !(pchOrig = NextToken(ppch)))
        return FALSE;

    for (pch = pchOrig; *pch; pch++)
        if (!isdigit(*pch))
            return FALSE;

    errno = 0;                  /* To distinguish success/failure after call */
    *pretVal = strtol(pchOrig, NULL, 10);

    /* Check for various possible errors */
    if ((errno == ERANGE && (*pretVal == LONG_MAX || *pretVal == (unsigned long)
                             LONG_MIN))
        || (errno != 0 && *pretVal == 0))
        return FALSE;

    return TRUE;
}
#endif

static void
SetSeed(const rng rngx, void *rngctx, char *sz)
{

    if (rngx == RNG_MANUAL || rngx == RNG_RANDOM_DOT_ORG) {
        outputl(_("You can't set a seed " "if you're using manual dice generation or random.org"));
        return;
    }

    if (sz && *sz) {
#if HAVE_LIBGMP
        if (InitRNGSeedLong(sz, rngx, rngctx))
            outputl(_("You must specify a valid seed (see `help set " "seed')."));
        else
            outputf(_("Seed set to %s.\n"), sz);
#else
        gboolean bSuccess;
        unsigned long n = 0;

        bSuccess = ParseULong(&sz, &n);

        if (!bSuccess || n > UINT_MAX) {
            outputl(_("You must specify a valid seed (see `help set seed')."));

            return;
        }

        InitRNGSeed((unsigned int) n, rngx, rngctx);
        outputf(_("Seed set to %ld.\n"), n);
#endif                          /* HAVE_LIBGMP */
    } else
        outputl(RNGSystemSeed(rngx, rngctx, NULL) ?
                _("Seed initialised from system random data.") : _("Seed initialised by system clock."));
}

extern void
SetRNG(rng * prng, rngcontext * rngctx, rng rngNew, char *szSeed)
{

    if (*prng == rngNew && !*szSeed) {
        outputf(_("You are already using the %s generator.\n"), gettext(aszRNG[rngNew]));
        return;
    }

    /* Dispose internal paremeters for RNG */

    CloseRNG(*prng, rngctx);

    switch (rngNew) {
    case RNG_BBS:
#if HAVE_LIBGMP
        {
            char *sz, *sz1;
            int fInit;

            fInit = FALSE;

            if (*szSeed) {
                if (!StrNCaseCmp(szSeed, "modulus", strcspn(szSeed, " \t\n\r\v\f"))) {
                    NextToken(&szSeed); /* skip "modulus" keyword */
                    if (InitRNGBBSModulus(NextToken(&szSeed), rngctx)) {
                        outputf(_("You must specify a valid modulus (see `help " "set rng bbs')."));
                        return;
                    }
                    fInit = TRUE;
                } else if (!StrNCaseCmp(szSeed, "factors", strcspn(szSeed, " \t\n\r\v\f"))) {
                    NextToken(&szSeed); /* skip "modulus" keyword */
                    sz = NextToken(&szSeed);
                    sz1 = NextToken(&szSeed);
                    if (InitRNGBBSFactors(sz, sz1, rngctx)) {
                        outputf(_("You must specify two valid factors (see `help " "set rng bbs')."));
                        return;
                    }
                    fInit = TRUE;
                }
            }

            if (!fInit)
                /* use default modulus, with factors
                 * 148028650191182616877187862194899201391 and
                 * 315270837425234199477225845240496832591. */
                InitRNGBBSModulus("46669116508701198206463178178218347698370"
                                  "262771368237383789001446050921334081", rngctx);
            break;
        }
#else
        abort();
#endif                          /* HAVE_LIBGMP */

    case RNG_FILE:
        {
            char *sz = NextToken(&szSeed);

            if (!sz || !*sz) {
                outputl(_("Please enter filename!"));
                return;
            }

            if (!OpenDiceFile(rngctx, sz)) {
                outputf(_("File %s does not exist or is not readable"), sz);
                return;
            }

        }
        break;


    default:
        ;
    }

    outputf(_("GNU Backgammon will now use the %s generator.\n"), gettext(aszRNG[rngNew]));

    switch ((*prng = rngNew)) {
    case RNG_MANUAL:
    case RNG_RANDOM_DOT_ORG:
    case RNG_FILE:
        /* no-op */
        break;

    default:
        SetSeed(*prng, rngctx, szSeed);
        break;
    }

}


static void
SetMoveFilter(char *sz, movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES])
{

    int ply = ParseNumber(&sz);
    int level;
    int accept;
    movefilter *pmfFilter;
    int extras;
    float tolerance;

    /* Temporary removing the temporary debug ...
     * outputf( _("Temporary debug: '%s' '%s'\n"), szSetCommand, sz ); */

    if (ply < 0) {
        outputl(N_("You must specify for which ply you want to set a filter"));
        return;
    }

    if (!(0 < ply && ply <= MAX_FILTER_PLIES)) {
        outputf(_("You must specify a valid ply for setting move filters "
                  "(see `help set %s movefilter')"), szSetCommand);
        return;
    }

    if (((level = ParseNumber(&sz)) < 0) || (level >= ply)) {
        outputf(_("You must specify a valid level 0..%d for the filter "
                  "(see `help set %s movefilter')"), ply - 1, szSetCommand);
        return;
    }

    pmfFilter = &aamf[ply - 1][level];

    if ((accept = ParseNumber(&sz)) == INT_MIN) {
        outputf(N_("You must specify a number of moves to accept (or a negative number to skip "
                   "this level) (see `help set %s movefilter')"), szSetCommand);
        return;
    }

    if (accept < 0) {
        pmfFilter->Accept = -1;
        pmfFilter->Extra = 0;
        pmfFilter->Threshold = 0.0;
        return;
    }

    if (((extras = ParseNumber(&sz)) < 0) || ((tolerance = ParseReal(&sz)) < 0.0)) {
        outputf(_("You must set a count of extra moves and a search tolerance "
                  "(see `help set %s movefilter')."), szSetCommand);
        return;
    }

    pmfFilter->Accept = accept;
    pmfFilter->Extra = extras;
    pmfFilter->Threshold = tolerance;
}




extern void
CommandSetAnalysisCube(char *sz)
{

    if (SetToggle("analysis cube", &fAnalyseCube, sz,
                  _("Cube action will be analysed."), _("Cube action will not be analysed.")) >= 0)
        UpdateSetting(&fAnalyseCube);
}

extern void
CommandSetAnalysisLuck(char *sz)
{

    if (SetToggle("analysis luck", &fAnalyseDice, sz,
                  _("Dice rolls will be analysed."), _("Dice rolls will not be analysed.")) >= 0)
        UpdateSetting(&fAnalyseDice);
}

extern void
CommandSetAnalysisLuckAnalysis(char *sz)
{


    szSet = _("luck analysis");
    szSetCommand = "set analysis luckanalysis";
    pecSet = &ecLuck;
    HandleCommand(sz, acSetEvaluation);

}


extern void
CommandSetAnalysisMoves(char *sz)
{

    if (SetToggle("analysis moves", &fAnalyseMove, sz,
                  _("Chequer play will be analysed."), _("Chequer play will not be analysed.")) >= 0)
        UpdateSetting(&fAnalyseMove);
}

static void
SetLuckThreshold(lucktype lt, char *sz)
{

    float r = ParseReal(&sz);
    char *szCommand = gettext(aszLuckTypeCommand[lt]);

    if (r <= 0.0f) {
        outputf(_("You must specify a positive number for the threshold (see "
                  "`help set analysis\nthreshold %s').\n"), szCommand);
        return;
    }

    arLuckLevel[lt] = r;

    outputf(_("`%s' threshold set to %.3f.\n"), szCommand, r);
}

static void
SetSkillThreshold(skilltype lt, char *sz)
{

    float r = ParseReal(&sz);
    char *szCommand = gettext(aszSkillTypeCommand[lt]);

    if (r < 0.0f) {
        outputf(_("You must specify a semi-positive number for the threshold (see "
                  "`help set analysis\nthreshold %s').\n"), szCommand);
        return;
    }

    arSkillLevel[lt] = r;

    outputf(_("`%s' threshold set to %.3f.\n"), szCommand, r);
}

extern void
CommandSetAnalysisThresholdBad(char *sz)
{

    SetSkillThreshold(SKILL_BAD, sz);
}

extern void
CommandSetAnalysisThresholdDoubtful(char *sz)
{

    SetSkillThreshold(SKILL_DOUBTFUL, sz);
}

extern void
CommandSetAnalysisThresholdLucky(char *sz)
{

    SetLuckThreshold(LUCK_GOOD, sz);
}

extern void
CommandSetAnalysisThresholdUnlucky(char *sz)
{

    SetLuckThreshold(LUCK_BAD, sz);
}

extern void
CommandSetAnalysisThresholdVeryBad(char *sz)
{

    SetSkillThreshold(SKILL_VERYBAD, sz);
}

extern void
CommandSetAnalysisThresholdVeryLucky(char *sz)
{

    SetLuckThreshold(LUCK_VERYGOOD, sz);
}

extern void
CommandSetAnalysisThresholdVeryUnlucky(char *sz)
{

    SetLuckThreshold(LUCK_VERYBAD, sz);
}

extern void
CommandSetStyledGameList(char *sz)
{

    SetToggle("styledgamelist", &fStyledGamelist, sz,
              _("Show colours in game window"), _("Do not show colours in game window."));

#if USE_GTK
    if (fX)
        ChangeGame(NULL);
#endif
}

extern void
CommandSetFullScreen(char *sz)
{
    int newValue = fFullScreen;
    SetToggle("fullscreen", &newValue, sz, _("Show board in full screen mode"), _("Show board in normal screen mode."));

    if (newValue != fFullScreen) {      /* Value has changed */
        fFullScreen = newValue;
#if USE_GTK
        if (fX)
            FullScreenMode(fFullScreen);
#endif
    }
}

extern void
CommandSetAutoBearoff(char *sz)
{

    SetToggle("automatic bearoff", &fAutoBearoff, sz, _("Will automatically "
                                                        "bear off as many chequers as possible."), _("Will not "
                                                                                                     "automatically bear off chequers."));
}

extern void
CommandSetAutoCrawford(char *sz)
{

    SetToggle("automatic crawford", &fAutoCrawford, sz, _("Will enable the "
                                                          "Crawford game according to match score."), _("Will not "
                                                                                                        "enable the Crawford game according to match score."));
}

extern void
CommandSetAutoDoubles(char *sz)
{

    int n;

    if ((n = ParseNumber(&sz)) < 0) {
        outputl(_("You must specify how many automatic doubles to use " "(see `help set automatic double')."));
        return;
    }

    if (n > 12) {
        outputl(_("Please specify a smaller limit (up to 12 automatic " "doubles)."));
        return;
    }

    if ((cAutoDoubles = (unsigned int) n) > 1)
        outputf(_("Automatic doubles will be used " "(up to a limit of %d).\n"), n);
    else if (cAutoDoubles)
        outputl(_("A single automatic double will be permitted."));
    else
        outputl(_("Automatic doubles will not be used."));

    UpdateSetting(&cAutoDoubles);

    if (cAutoDoubles) {
        if (ms.nMatchTo > 0)
            outputl(_("(Note that automatic doubles will have " "no effect until you " "start session play.)"));
        else if (!ms.fCubeUse)
            outputl(_("Note that automatic doubles will have no effect "
                      "until you " "enable cube use\n(see `help set cube use')."));
    }
}

extern void
CommandSetAutoGame(char *sz)
{

    SetToggle("automatic game", &fAutoGame, sz,
              _("Will automatically start games after wins."), _("Will not automatically start games."));
}

extern void
CommandSetAutoMove(char *sz)
{

    SetToggle("automatic move", &fAutoMove, sz,
              _("Forced moves will be made automatically."), _("Forced moves will not be made automatically."));
}

extern void
CommandSetAutoRoll(char *sz)
{

    SetToggle("automatic roll", &fAutoRoll, sz,
              _("Will automatically roll the "
                "dice when no cube action is possible."), _("Will not automatically roll the dice."));
}

static int
CorrectNumberOfChequers(TanBoard anBoard, int numCheq)
{                               /* Check players don't have too many chequers (esp. for hypergammon) */
    int ac[2], i;
    ac[0] = ac[1] = 0;
    for (i = 0; i < 25; i++) {
        ac[0] += anBoard[0][i];
        ac[1] += anBoard[1][i];
    }
    /* Nb. representation doesn't specify how many chequers
     * may be off the board - so has to be <= not == */
    if (ac[0] <= numCheq && ac[1] <= numCheq)
        return 1;
    else
        return 0;
}

extern void
CommandSetBoard(char *sz)
{

    TanBoard an;
    moverecord *pmr;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("There must be a game in progress to set the board."));

        return;
    }

    if (!*sz) {
        outputl(_("You must specify a position (see `help set board')."));

        return;
    }

    /* FIXME how should =n notation be handled? */
    if (ParsePosition(an, &sz, NULL) < 0 || !CorrectNumberOfChequers(an, anChequers[ms.bgv]))
        return;

    pmr = NewMoveRecord();

    pmr->mt = MOVE_SETBOARD;
    pmr->fPlayer = ms.fMove;

    if (ms.fMove)
        SwapSides(an);
    PositionKey((ConstTanBoard) an, &pmr->sb.key);

    AddMoveRecord(pmr);

    /* this way the player turn is stored */
    get_current_moverecord(NULL);

    ShowBoard();
}

static int
CheckCubeAllowed(void)
{

    if (ms.gs != GAME_PLAYING) {
        outputl(_("There must be a game in progress to set the cube."));
        return -1;
    }

    if (ms.fCrawford) {
        outputl(_("The cube is disabled during the Crawford game."));
        return -1;
    }

    if (!ms.fCubeUse) {
        outputl(_("The cube is disabled (see `help set cube use')."));
        return -1;
    }

    return 0;
}

extern void
CommandSetCache(char *sz)
{
    int n;
    if ((n = ParseNumber(&sz)) < 0) {
        outputl(_("You must specify the number of cache entries to use."));
        return;
    }

    n = EvalCacheResize(n);
    if (n != -1)
        outputf(ngettext
                ("The position cache has been sized to %d entry.\n",
                 "The position cache has been sized to %d entries.\n", n), n);
    else
        outputerr("EvalCacheResize");
}

#if USE_MULTITHREAD
extern void
CommandSetThreads(char *sz)
{
    int n;

    if ((n = ParseNumber(&sz)) <= 0) {
        outputl(_("You must specify the number of threads to use."));

        return;
    }

    if (n > MAX_NUMTHREADS) {
        outputf(_("%d is the maximum number of threads supported"), MAX_NUMTHREADS);
        output(".\n");
        n = MAX_NUMTHREADS;
    }

    MT_SetNumThreads(n);
    outputf(_("The number of threads has been set to %d.\n"), n);
}
#endif

extern void
CommandSetVsync3d(char *sz)
{
#if defined(WIN32) && USE_BOARD3D
    SetToggle("vsync", &fSync, sz, _("Set vsync on."), _("Set vsync off."));
    if (setVSync(fSync) == FALSE) {
        if (gtk_widget_get_realized(pwMain)) {
            fSync = -1;
            outputl(_("Unable to set vsync."));
            return;
        } else
            fResetSync = TRUE;  /* Try again once main window is created */
    }
    fSync = (fSync != 0) ? 1 : 0;       /* Set to 1 or 0, (-1 == not set) */
#else
    (void) sz;                  /* suppress unused parameter compiler warning */
    outputl(_("This function is for the MS Windows 3d board only"));
#endif
}

#ifndef WEB
extern void
CommandSetBrowser(char *sz)
{

    if (!sz || !*sz)
        set_web_browser("");
    set_web_browser(NextToken(&sz));
}
#endif /* WEB */

extern void
CommandSetCalibration(char *sz)
{

    float r;

    if (!sz || !*sz) {
        rEvalsPerSec = -1.0f;
        outputl(_("The evaluation speed has been cleared."));
        return;
    }

    if ((r = ParseReal(&sz)) <= 2.0f) {
        outputl(_("If you give a parameter to `set calibration', it must "
                  "be a legal number of evaluations per second."));
        return;
    }

    rEvalsPerSec = r;

    outputf(_("The speed estimate has been set to %.0f static " "evaluations per second.\n"), rEvalsPerSec);
}

extern void
CommandSetClockwise(char *sz)
{

    SetToggle("clockwise", &fClockwise, sz,
              _("Player 1 moves clockwise (and "
                "player 0 moves anticlockwise)."),
              _("Player 1 moves anticlockwise (and " "player 0 moves clockwise)."));

#if USE_GTK
    if (fX) {
#if USE_BOARD3D
        BoardData *bd = BOARD(pwBoard)->board_data;
        ShowBoard();
        if (display_is_3d(bd->rd)) {
            RestrictiveRedraw();
            RerenderBase(bd->bd3d);
        }
#else
        ShowBoard();
#endif
    }
#endif                          /* USE_GTK */
}

extern void
CommandSetAppearance(char *sz)
{
#if USE_GTK
    SetBoardPreferences(pwBoard, sz);
#else
    char *apch[2];

    while (ParseKeyValue(&sz, apch))
        RenderPreferencesParam(GetMainAppearance(), apch[0], apch[1]);
#endif
}

extern void
CommandSetConfirmDefault(char *sz)
{

    if (!sz || !*sz) {
        outputf("Needs an argument!\n");
        return;
    }
    if (strcmp(sz, "yes") == 0)
        nConfirmDefault = 1;
    else if (strcmp(sz, "no") == 0)
        nConfirmDefault = 0;
    else if (strcmp(sz, "ask") == 0)
        nConfirmDefault = -1;
    else
        outputf(_("Invalid argument\n"));
}

extern void
CommandSetConfirmNew(char *sz)
{

    SetToggle("confirm new", &fConfirmNew, sz,
              _("Will ask for confirmation before "
                "aborting games in progress."),
              _("Will not ask for confirmation " "before aborting games in progress."));
}

extern void
CommandSetConfirmSave(char *sz)
{

    SetToggle("confirm save", &fConfirmSave, sz,
              _("Will ask for confirmation before "
                "overwriting existing files."), _("Will not ask for confirmation " "overwriting existing files."));
}

extern void
CommandSetCubeCentre(char *UNUSED(sz))
{

    moverecord *pmr;

    if (CheckCubeAllowed())
        return;

    pmr = NewMoveRecord();

    pmr->mt = MOVE_SETCUBEPOS;
    pmr->scp.fCubeOwner = -1;
    pmr->fPlayer = ms.fMove;

    AddMoveRecord(pmr);

    outputl(_("The cube has been centred."));

#if USE_GTK
    if (fX)
        ShowBoard();
#endif                          /* USE_GTK */
}

extern void
CommandSetCubeOwner(char *sz)
{
    moverecord *pmr;

    int i;

    if (CheckCubeAllowed())
        return;

    switch (i = ParsePlayer(NextToken(&sz))) {
    case 0:
    case 1:
        break;

    case 2:
        /* "set cube owner both" is the same as "set cube centre" */
        CommandSetCubeCentre(NULL);
        return;

    default:
        outputl(_("You must specify which player owns the cube " "(see `help set cube owner')."));
        return;
    }

    pmr = NewMoveRecord();

    pmr->mt = MOVE_SETCUBEPOS;
    pmr->scp.fCubeOwner = i;
    pmr->fPlayer = ms.fMove;

    AddMoveRecord(pmr);

    outputf(_("%s now owns the cube.\n"), ap[ms.fCubeOwner].szName);

#if USE_GTK
    if (fX)
        ShowBoard();
#endif                          /* USE_GTK */
}

extern void
CommandSetCubeUse(char *sz)
{

    if (SetToggle("cube use", &fCubeUse, sz,
                  _("Use of the doubling cube is permitted."), _("Use of the doubling cube is disabled.")) < 0)
        return;

    if (!ms.nMatchTo && ms.fJacoby && !fCubeUse)
        outputl(_("Note that you'll have to disable the Jacoby rule "
                  "if you want gammons and\nbackgammons to be scored " "(see `help set jacoby')."));

    if (ms.fCrawford && fCubeUse)
        outputl(_("(But the Crawford rule is in effect, " "so you won't be able to use it during\nthis game.)"));
    else if (ms.gs == GAME_PLAYING && !fCubeUse) {
        /* The cube was being used and now it isn't; reset it to 1,
         * centred. */
        ms.nCube = 1;
        ms.fCubeOwner = -1;
        UpdateSetting(&ms.nCube);
        UpdateSetting(&ms.fCubeOwner);
        CancelCubeAction();
    }

    ms.fCubeUse = fCubeUse;

#if USE_GTK
    if (fX)
        ShowBoard();
#endif                          /* USE_GTK */
}

extern void
CommandSetCubeValue(char *sz)
{

    int i, n;
    moverecord *pmr;

    if (CheckCubeAllowed())
        return;

    n = ParseNumber(&sz);

    for (i = MAX_CUBE; i; i >>= 1)
        if (n == i) {
            pmr = NewMoveRecord();
            pmr->mt = MOVE_SETCUBEVAL;
            pmr->fPlayer = ms.fMove;

            pmr->scv.nCube = n;

            AddMoveRecord(pmr);

            outputf(_("The cube has been set to %d.\n"), n);

#if USE_GTK
            if (fX)
                ShowBoard();
#endif                          /* USE_GTK */
            return;
        }

    outputl(_("You must specify a legal cube value (see `help set cube " "value')."));
}

extern void
CommandSetDelay(char *sz)
{
#if USE_GTK
    if (fX) {
        int n;

        if (*sz && !StrNCaseCmp(sz, "none", strlen(sz)))
            n = 0;
        else if ((n = ParseNumber(&sz)) < 0 || n > 10000) {
            outputl(_("You must specify a legal move delay (see `help set " "delay')."));
            return;
        }

        if (n) {
            outputf(ngettext("All moves will be shown for at least %d millisecond.\n",
                             "All moves will be shown for at least %d milliseconds.\n", n), n);
            if (!fDisplay)
                outputl(_("You will also need to use `set display' to turn "
                          "board updates on (see `help set display')."));
        } else
            outputl(_("Moves will not be delayed."));

        nDelay = n;
        UpdateSetting(&nDelay);
    } else
#endif                          /* USE_GTK */
        (void) sz;              /* suppress unused parameter compiler warning */
    outputl(_("The `set delay' command applies only when using a window " "system."));
}

extern void
CommandSetDice(char *sz)
{

    int n0, n1;
    moverecord *pmr;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("There must be a game in progress to set the dice."));

        return;
    }

    n0 = ParseNumber(&sz);

    if (n0 > 10) {
        /* assume a 2-digit number; n0 first digit, n1 second */
        n1 = n0 % 10;
        n0 /= 10;
    } else
        n1 = ParseNumber(&sz);

    if (n0 < 1 || n0 > 6 || n1 < 1 || n1 > 6) {
        outputl(_("You must specify two numbers from 1 to 6 for the dice."));

        return;
    }

    pmr = NewMoveRecord();

    pmr->mt = MOVE_SETDICE;
    pmr->fPlayer = ms.fMove;
    pmr->anDice[0] = n0;
    pmr->anDice[1] = n1;

    AddMoveRecord(pmr);

    outputf(_("The dice have been set to %d and %d.\n"), n0, n1);

#if USE_BOARD3D
    RestrictiveRedraw();
#endif
#if USE_GTK
    if (fX)
        ShowBoard();
#endif
}

extern void
CommandSetDisplay(char *sz)
{

    SetToggle("display", &fDisplay, sz, _("Will display boards for computer "
                                          "moves."), _("Will not display boards for computer moves."));
}

extern void
CommandSetEvalCubeful(char *sz)
{

    char asz[2][128], szCommand[64];
    int f = pecSet->fCubeful;

    sprintf(asz[0], _("%s will use cubeful evaluation.\n"), szSet);
    sprintf(asz[1], _("%s will use cubeless evaluation.\n"), szSet);
    sprintf(szCommand, "%s cubeful", szSetCommand);
    SetToggle(szCommand, &f, sz, asz[0], asz[1]);
    pecSet->fCubeful = f;
}

extern void
CommandSetEvalPrune(char *sz)
{

    char asz[2][128], szCommand[64];
    int f = pecSet->fUsePrune;

    sprintf(asz[0], _("%s will use pruning.\n"), szSet);
    sprintf(asz[1], _("%s will not use pruning.\n"), szSet);
    sprintf(szCommand, "%s prune", szSetCommand);
    SetToggle(szCommand, &f, sz, asz[0], asz[1]);
    pecSet->fUsePrune = f;
}

extern void
CommandSetEvalDeterministic(char *sz)
{

    char asz[2][128], szCommand[64];
    int f = pecSet->fDeterministic;

    sprintf(asz[0], _("%s will use deterministic noise.\n"), szSet);
    sprintf(asz[1], _("%s will use pseudo-random noise.\n"), szSet);
    sprintf(szCommand, "%s deterministic", szSetCommand);
    SetToggle(szCommand, &f, sz, asz[0], asz[1]);
    pecSet->fDeterministic = f;

    if (pecSet->rNoise == 0.0f)
        outputl(_("(Note that this setting will have no effect unless you " "set noise to some non-zero value.)"));
}

extern void
CommandSetEvalNoise(char *sz)
{

    float r = ParseReal(&sz);

    if (r < 0.0f) {
        outputf(_("You must specify a valid amount of noise to use " "(see `help set\n%s noise').\n"), szSetCommand);

        return;
    }

    pecSet->rNoise = r;

    if (pecSet->rNoise > 0.0f)
        outputf(_("%s will use noise with standard deviation %5.3f.\n"), szSet, pecSet->rNoise);
    else
        outputf(_("%s will use noiseless evaluations.\n"), szSet);
}

extern void
CommandSetEvalPlies(char *sz)
{

    int n = ParseNumber(&sz);

    if (n < 0 || n > 7)
        outputf(_("Valid numbers of plies to look ahead are 0 to 7.\n"));
    else
        pecSet->nPlies = n;

    outputf(_("%s will use %d ply evaluation.\n"), szSet, pecSet->nPlies);
}

#if USE_GTK
extern void
CommandSetGUIAnimationBlink(char *UNUSED(sz))
{

    animGUI = ANIMATE_BLINK;
}

extern void
CommandSetGUIAnimationNone(char *UNUSED(sz))
{

    animGUI = ANIMATE_NONE;
}

extern void
CommandSetGUIAnimationSlide(char *UNUSED(sz))
{

    animGUI = ANIMATE_SLIDE;
}

extern void
CommandSetGUIAnimationSpeed(char *sz)
{

    unsigned int n = ParseNumber(&sz);

    if (n > 7) {
        outputl(_("You must specify a speed between 0 and 7 " "(see `help set speed')."));

        return;
    }

    nGUIAnimSpeed = n;

    outputf(_("Animation speed set to %d.\n"), n);
}

extern void
CommandSetGUIBeep(char *sz)
{

    SetToggle("gui beep", &fGUIBeep, sz,
              _("GNU Backgammon will beep on illegal input."), _("GNU Backgammon will not beep on illegal input."));
}

extern void
CommandSetGUIGrayEdit(char *sz)
{

    SetToggle("gui grayedit", &fGUIGrayEdit, sz,
              _("Board will be grayed in edit mode."), _("Board will not change color in edit mode."));
}


extern void
CommandSetGUIDiceArea(char *sz)
{

    if (SetToggle("gui dicearea", &GetMainAppearance()->fDiceArea, sz,
                  _("A dice icon will be shown below the board when a human "
                    "player is on roll."), _("No dice icon will be shown.")) >= 0)
        UpdateSetting(&GetMainAppearance()->fDiceArea);
}

extern void
CommandSetGUIHighDieFirst(char *sz)
{

    SetToggle("gui highdiefirst", &fGUIHighDieFirst, sz,
              _("The higher die will be shown on the left."), _("The dice will be shown in the order rolled."));
}

extern void
CommandSetGUIIllegal(char *sz)
{

    SetToggle("gui illegal", &fGUIIllegal, sz,
              _("Chequers may be dragged to illegal points."), _("Chequers may not be dragged to illegal points."));
}

extern void
CommandSetGUIShowIDs(char *sz)
{
    if (!inCallback) {
        SetToggle("gui showids", &fShowIDs, sz,
                  _("The position and match IDs will be shown above the board."),
                  _("The position and match IDs will not be shown."));
    }
}

extern void
CommandSetGUIDragTargetHelp(char *sz)
{

    if (SetToggle("gui dragtargethelp", &fGUIDragTargetHelp, sz,
                  _("The target help while dragging a chequer will "
                    "be shown."), _("The target help while dragging a chequer will " "not be shown.")))
        UpdateSetting(&fGUIDragTargetHelp);
}

extern void
CommandSetGUIUseStatsPanel(char *sz)
{

    SetToggle("gui usestatspanel", &fGUIUseStatsPanel, sz,
              _("The match statistics will be shown in a panel"), _("The match statistics will be shown in a list"));
}

extern void
CommandSetGUIMoveListDetail(char *sz)
{
    SetToggle("gui movelistdetail", &showMoveListDetail, sz,
              _("The win loss statistics will be shown in the move analysis"),
              _("Basic details will be shown in the move analysis"));
}

extern void
CommandSetGUIShowPipsNone(char *UNUSED(sz))
{
    gui_show_pips = GUI_SHOW_PIPS_NONE;
    outputf(_("The pip counts will not be shown."));
    UpdateSetting(&gui_show_pips);
}

extern void
CommandSetGUIShowPipsPips(char *UNUSED(sz))
{
    gui_show_pips = GUI_SHOW_PIPS_PIPS;
    outputf(_("Pip counts will be shown."));
    UpdateSetting(&gui_show_pips);
}

extern void
CommandSetGUIShowPipsEPC(char *UNUSED(sz))
{
    gui_show_pips = GUI_SHOW_PIPS_EPC;
    outputf(_("Effective pip counts will be shown."));
    UpdateSetting(&gui_show_pips);
}

extern void
CommandSetGUIShowPipsWastage(char *UNUSED(sz))
{
    gui_show_pips = GUI_SHOW_PIPS_WASTAGE;
    outputf(_("Pip wastage will be shown."));
    UpdateSetting(&gui_show_pips);
}

extern void
CommandSetGUIWindowPositions(char *sz)
{

    SetToggle("gui windowpositions", &fGUISetWindowPos, sz,
              _("Saved window positions will be applied to new windows."),
              _("Saved window positions will not be applied to new " "windows."));
}
#else
static void
NoGUI(void)
{

    outputl(_("This installation of GNU Backgammon was compiled without GUI " "support."));
}

extern void
CommandSetGUIAnimationBlink(char *UNUSED(sz))
{

    NoGUI();
}

extern void
CommandSetGUIAnimationNone(char *UNUSED(sz))
{

    NoGUI();
}

extern void
CommandSetGUIAnimationSlide(char *UNUSED(sz))
{

    NoGUI();
}

extern void
CommandSetGUIAnimationSpeed(char *UNUSED(sz))
{

    NoGUI();
}

extern void
CommandSetGUIBeep(char *UNUSED(sz))
{

    NoGUI();
}

extern void
CommandSetGUIDiceArea(char *UNUSED(sz))
{

    NoGUI();
}

extern void
CommandSetGUIHighDieFirst(char *UNUSED(sz))
{

    NoGUI();
}

extern void
CommandSetGUIIllegal(char *UNUSED(sz))
{

    NoGUI();
}

extern void
CommandSetGUIShowIDs(char *UNUSED(sz))
{

    NoGUI();
}

extern void
CommandSetGUIShowPipsNone(char *UNUSED(sz))
{
    NoGUI();
}

extern void
CommandSetGUIShowPipsPips(char *UNUSED(sz))
{
    NoGUI();
}

extern void
CommandSetGUIShowPipsEPC(char *UNUSED(sz))
{
    NoGUI();
}

extern void
CommandSetGUIShowPipsWastage(char *UNUSED(sz))
{
    NoGUI();
}

extern void
CommandSetGUIWindowPositions(char *UNUSED(sz))
{

    NoGUI();
}
#endif

extern void
CommandSetPlayerMoveFilter(char *sz)
{

    SetMoveFilter(sz, ap[iPlayerSet].aamf);

}

extern void
CommandSetPlayerChequerplay(char *sz)
{

    szSet = ap[iPlayerSet].szName;
    szSetCommand = "player chequerplay evaluation";
    pesSet = &ap[iPlayerSet].esChequer;

    outputpostpone();

    HandleCommand(sz, acSetEvalParam);

    if (ap[iPlayerSet].pt != PLAYER_GNU)
        outputf(_("(Note that this setting will have no effect until you "
                  "`set player %s gnu'.)\n"), ap[iPlayerSet].szName);

    outputresume();
}


extern void
CommandSetPlayerCubedecision(char *sz)
{

    szSet = ap[iPlayerSet].szName;
    szSetCommand = "player cubedecision evaluation";
    pesSet = &ap[iPlayerSet].esCube;

    outputpostpone();

    HandleCommand(sz, acSetEvalParam);

    if (ap[iPlayerSet].pt != PLAYER_GNU)
        outputf(_("(Note that this setting will have no effect until you "
                  "`set player %s gnu'.)\n"), ap[iPlayerSet].szName);

    outputresume();
}


extern void
CommandSetPlayerExternal(char *sz)
{

#if !HAVE_SOCKETS
    outputl(_("This installation of GNU Backgammon was compiled without\n"
              "socket support, and does not implement external players."));
#else
    int h, cb;
    struct sockaddr *psa;
    char *pch;

    if (ap[iPlayerSet].pt == PLAYER_EXTERNAL)
        closesocket(ap[iPlayerSet].h);

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify the name of the socket to the external\n"
                  "player (see `help set player external')."));
        return;
    }

    pch = strcpy(malloc(strlen(sz) + 1), sz);

    if ((h = ExternalSocket(&psa, &cb, sz)) < 0) {
        SockErr(pch);
        free(pch);
        return;
    }

    while (connect(h, psa, cb) < 0) {
        if (errno == EINTR) {
            if (fInterrupt) {
                closesocket(h);
                free(psa);
                free(pch);
                return;
            }
            continue;
        }

        SockErr(pch);
        closesocket(h);
        free(psa);
        free(pch);
        return;
    }

    ap[iPlayerSet].pt = PLAYER_EXTERNAL;
    ap[iPlayerSet].h = h;
    if (ap[iPlayerSet].szSocket)
        free(ap[iPlayerSet].szSocket);
    ap[iPlayerSet].szSocket = pch;

    free(psa);
#endif                          /* !HAVE_SOCKETS */
}

extern void
CommandSetPlayerGNU(char *UNUSED(sz))
{

#if HAVE_SOCKETS
    if (ap[iPlayerSet].pt == PLAYER_EXTERNAL)
        closesocket(ap[iPlayerSet].h);
#endif

    ap[iPlayerSet].pt = PLAYER_GNU;

    outputf(_("Moves for %s will now be played by GNU Backgammon.\n"), ap[iPlayerSet].szName);

#if USE_GTK
    if (fX)
        /* The "play" button might now be required; update the board. */
        ShowBoard();
#endif                          /* USE_GTK */
}

extern void
CommandSetPlayerHuman(char *UNUSED(sz))
{

#if HAVE_SOCKETS
    if (ap[iPlayerSet].pt == PLAYER_EXTERNAL)
        closesocket(ap[iPlayerSet].h);
#endif

    ap[iPlayerSet].pt = PLAYER_HUMAN;

    outputf(_("Moves for %s must now be entered manually.\n"), ap[iPlayerSet].szName);
}

extern void
CommandSetPlayerName(char *sz)
{

    if (!sz || !*sz) {
        outputl(_("You must specify a name to use."));

        return;
    }

    if (strlen(sz) > 31)
        sz[31] = 0;

    if ((*sz == '0' || *sz == '1') && !sz[1]) {
        outputf(_("`%c' is not a valid name.\n"), *sz);

        return;
    }

    if (!StrCaseCmp(sz, "both")) {
        outputl(_("`both' is a reserved word; you can't call a player " "that.\n"));

        return;
    }

    if (!CompareNames(sz, ap[!iPlayerSet].szName)) {
        outputl(_("That name is already in use by the other player."));

        return;
    }

    strcpy(ap[iPlayerSet].szName, sz);

    outputf(_("Player %d is now known as `%s'.\n"), iPlayerSet, sz);

#if USE_GTK
    if (fX)
        ShowBoard();
#endif                          /* USE_GTK */
}

extern void
CommandSetPlayer(char *sz)
{

    char *pch = NextToken(&sz), *pchCopy;
    int i;
    char szTemp[32];

    if (!pch) {
        outputl(_("You must specify a player (see `help set player')."));

        return;
    }

    szSetCommand = szTemp;

    if ((i = ParsePlayer(pch)) == 0 || i == 1) {
        iPlayerSet = i;
        sprintf(szTemp, "player %d", i);

        HandleCommand(sz, acSetPlayer);
        UpdateSetting(ap);

        szSetCommand = NULL;
        return;
    }

    if (i == 2) {
        if ((pchCopy = malloc(strlen(sz) + 1)) == 0) {
            outputl(_("Insufficient memory."));

            szSetCommand = NULL;
            return;
        }

        strcpy(pchCopy, sz);

        outputpostpone();

        iPlayerSet = 0;
        szSetCommand = "player 0";
        HandleCommand(sz, acSetPlayer);

        iPlayerSet = 1;
        szSetCommand = "player 1";
        HandleCommand(pchCopy, acSetPlayer);

        outputresume();

        UpdateSetting(ap);

        free(pchCopy);

        szSetCommand = NULL;
        return;
    }

    outputf(_("Unknown player `%s' (see `help set player').\n"), pch);

    szSetCommand = NULL;
    return;
}


extern void
CommandSetDefaultNames(char *sz)
{
    char *names[2] = { NextToken(&sz), NextToken(&sz) };
    int i;

    for (i = 0; i < 2; i++) {
        char *pch = names[i];
        if (!pch || !*pch) {
            outputl(_("You must specify two player names use."));
            return;
        }

        if (strlen(pch) > 31)
            pch[31] = 0;

        if ((*pch == '0' || *pch == '1') && !pch[1]) {
            outputf(_("`%c' is not a valid name.\n"), *pch);
            return;
        }

        if (!StrCaseCmp(pch, "both")) {
            outputl(_("`both' is a reserved word; you can't call a player " "that.\n"));
            return;
        }
    }

    if (!CompareNames(names[0], names[1])) {
        outputl(_("Player names identical"));
        return;
    }
    if (StrCaseCmp(names[0], default_names[0]) == 0 && StrCaseCmp(names[1], default_names[1]) == 0)
        return;

    strcpy(default_names[0], names[0]);
    strcpy(default_names[1], names[1]);

    outputf(_("Players will be known as `%s' and `%s'.\n This setting will take effect when a new match is started.\n"),
            default_names[0], default_names[1]);
}

extern void
CommandSetAliases(char *sz)
{
    if (strlen(sz) >= sizeof(player1aliases))
        outputf("%s %lu %s.\n", _("Aliases list limited to"), (long unsigned int) (sizeof(player1aliases) - 1),
                _("characters, truncating"));

    strncpy(player1aliases, sz, sizeof(player1aliases) - 1);

    outputf(_("Aliases for player 1 when importing MAT files set to \"%s\".\n "), player1aliases);
}


extern void
CommandSetPrompt(char *szParam)
{

    static char sz[128];        /* FIXME check overflow */

    szPrompt = (szParam && *szParam) ? strcpy(sz, szParam) : szDefaultPrompt;

    outputf(_("The prompt has been set to `%s'.\n"), szPrompt);
}

extern void
CommandSetRecord(char *sz)
{

    SetToggle("record", &fRecord, sz,
              _("All games in a session will be recorded."), _("Only the active game in a session will be recorded."));
}

extern void
CommandSetRNG(char *sz)
{

    rngSet = &rngCurrent;
    rngctxSet = rngctxCurrent;
    HandleCommand(sz, acSetRNG);

}

extern void
CommandSetRNGAnsi(char *sz)
{

    SetRNG(rngSet, rngctxSet, RNG_ANSI, sz);
}

extern void
CommandSetRNGFile(char *sz)
{

    SetRNG(rngSet, rngctxSet, RNG_FILE, sz);
}

#if HAVE_LIBGMP
extern void
CommandSetRNGBBS(char *sz)
{
    SetRNG(rngSet, rngctxSet, RNG_BBS, sz);
#else
extern void
CommandSetRNGBBS(char *UNUSED(sz))
{
    outputl(_("This installation of GNU Backgammon was compiled without the " "Blum, Blum and Shub generator."));
#endif                          /* HAVE_LIBGMP */
}

extern void
CommandSetRNGBsd(char *sz)
{
#if HAVE_RANDOM
    SetRNG(rngSet, rngctxSet, RNG_BSD, sz);
#else
    outputl(_("This installation of GNU Backgammon was compiled without the " "BSD generator."));
#endif                          /* HAVE_RANDOM */
}

extern void
CommandSetRNGIsaac(char *sz)
{

    SetRNG(rngSet, rngctxSet, RNG_ISAAC, sz);
}

extern void
CommandSetRNGManual(char *sz)
{

    SetRNG(rngSet, rngctxSet, RNG_MANUAL, sz);
}

extern void
CommandSetRNGMD5(char *sz)
{

    SetRNG(rngSet, rngctxSet, RNG_MD5, sz);
}

extern void
CommandSetRNGMersenne(char *sz)
{

    SetRNG(rngSet, rngctxSet, RNG_MERSENNE, sz);
}

extern void
CommandSetRNGRandomDotOrg(char *sz)
{

#if defined(LIBCURL_PROTOCOL_HTTPS)
    SetRNG(rngSet, rngctxSet, RNG_RANDOM_DOT_ORG, sz);
#else
    (void) sz;                  /* suppress unused parameter compiler warning */
    outputl(_("This installation of GNU Backgammon was compiled without\n"
              "support for HTTPS(libcurl) which is needed for fetching\n" "random numbers from <www.random.org>"));
#endif

}

extern void
CommandSetRolloutLate(char *sz)
{

    HandleCommand(sz, acSetRolloutLate);

}

extern void
CommandSetRolloutLogEnable(char *sz)
{
    int f = log_rollouts;

    SetToggle("rollout .sgf files", &f, sz,
              _("Create an .sgf file for each game rolled out"),
              _("Do not create an .sgf file for each game rolled out"));

    log_rollouts = f;
}

extern void
CommandSetRolloutLogFile(char *sz)
{

    if (log_file_name) {
        free(log_file_name);
    }

    log_file_name = g_strdup(sz);
}

extern void
CommandSetRolloutLateEnable(char *sz)
{
    int l = prcSet->fLateEvals;
    if (SetToggle("separate evaluation for later plies", &l, sz,
                  _("Use different evaluation for later moves of rollout."),
                  _("Do not change evaluations during rollout.")) != -1) {
        prcSet->fLateEvals = l;
    }
}

extern void
CommandSetRolloutLatePlies(char *sz)
{

    int n = ParseNumber(&sz);

    if (n < 1) {
        outputl(_("You must specify a valid ply at which to change evaluations "
                  "(see `help set rollout late plies')."));

        return;
    }

    prcSet->nLate = (unsigned short) n;

    if (!n)
        outputl(_("No evaluations changes will be made during rollouts."));
    else
        outputf(_("Evaluations will change after %d plies in rollouts.\n"), n);

}


extern void
CommandSetRolloutTruncation(char *sz)
{

    HandleCommand(sz, acSetTruncation);
}

extern void
CommandSetRolloutLimit(char *sz)
{

    HandleCommand(sz, acSetRolloutLimit);

}

extern void
CommandSetRolloutLimitEnable(char *sz)
{
    int s = prcSet->fStopOnSTD;

    if (SetToggle("stop when the STD's are small enough", &s, sz,
                  _("Stop rollout when STD's are small enough"), _("Do not stop rollout based on STDs")) != -1) {
        prcSet->fStopOnSTD = s;
    }
}

extern void
CommandSetRolloutLimitMinGames(char *sz)
{

    int n = ParseNumber(&sz);

    if (n < 1) {
        outputl(_("You must specify a valid minimum number of games to rollout "
                  "(see `help set rollout limit minimumgames')."));
        return;
    }

    prcSet->nMinimumGames = n;

    outputf(_("After %d games, rollouts will stop if the STDs are small enough" ".\n"), n);
}

extern void
CommandSetRolloutMaxError(char *sz)
{

    float r = ParseReal(&sz);

    if (r < 0.0001f) {
        outputl(_("You must set a valid fraction for the ratio "
                  "STD/value where rollouts can stop " "(see `help set rollout limit maxerror')."));
        return;
    }

    prcSet->rStdLimit = r;

    outputf(_("Rollouts can stop when the ratio |STD/value| is less than "
              "%5.4f for every value (win/gammon/backgammon/...equity)\n"), r);
}

extern void
CommandSetRolloutJsd(char *sz)
{

    HandleCommand(sz, acSetRolloutJsd);

}

extern void
CommandSetRolloutJsdEnable(char *sz)
{
    int s = prcSet->fStopOnJsd;
    if (SetToggle("stop rollout of choices which appear to  "
                  "to be worse with statistical certainty", &s, sz,
                  _("Stop rollout of choices based on JSDs"),
                  _("Do not stop rollout of moves choices on JSDs")) != -1) {
        prcSet->fStopOnJsd = s;
    }
}

extern void
CommandSetRolloutJsdMinGames(char *sz)
{

    int n = ParseNumber(&sz);

    if (n < 1) {
        outputl(_("You must specify a valid minimum number of games to rollout "
                  "(see `help set rollout jsd minimumgames')."));
        return;
    }
    prcSet->nMinimumJsdGames = n;

    outputf(_("After %d games, rollouts will stop if the JSDs are large enough" ".\n"), n);
}


extern void
CommandSetRolloutJsdLimit(char *sz)
{

    float r = ParseReal(&sz);

    if (r < 0.0001f) {
        outputl(_("You must set a number of joint standard deviations for the equity"
                  " difference with the best move being rolled out " "(see `help set rollout jsd limit')."));
        return;
    }

    prcSet->rJsdLimit = r;

    outputf(_("Rollouts (or rollouts of moves) may stop when the equity is more "
              "than %5.3f joint standard deviations from the best move being rolled out\n"), r);
}

extern void
CommandSetRollout(char *sz)
{

    prcSet = &rcRollout;
    HandleCommand(sz, acSetRollout);

}


extern void
CommandSetRolloutRNG(char *sz)
{

    rngSet = &prcSet->rngRollout;
    rngctxSet = rngctxRollout;
    HandleCommand(sz, acSetRNG);

}

/* set an eval context, then copy to other player's settings */
static void
SetRolloutEvaluationContextBoth(char *sz, evalcontext * pec[])
{

    g_assert(pec[0] != 0);
    g_assert(pec[1] != 0);

    pecSet = pec[0];

    HandleCommand(sz, acSetEvaluation);

    /* copy to both players */
    /* FIXME don't copy if there was an error setting player 0 */
    memcpy(pec[1], pec[0], sizeof(evalcontext));

}

extern void
CommandSetRolloutChequerplay(char *sz)
{

    evalcontext *pec[2];

    szSet = _("Chequer play in rollouts");
    szSetCommand = "rollout chequerplay";

    pec[0] = prcSet->aecChequer;
    pec[1] = prcSet->aecChequer + 1;

    SetRolloutEvaluationContextBoth(sz, pec);

}

extern void
CommandSetRolloutMoveFilter(char *sz)
{

    szSetCommand = "rollout";
    SetMoveFilter(sz, prcSet->aaamfChequer[0]);
    SetMoveFilter(sz, prcSet->aaamfChequer[1]);

}

extern void
CommandSetRolloutLateMoveFilter(char *sz)
{

    szSetCommand = "rollout late";
    SetMoveFilter(sz, prcSet->aaamfLate[0]);
    SetMoveFilter(sz, prcSet->aaamfLate[1]);

}

extern void
CommandSetRolloutPlayerMoveFilter(char *sz)
{

    szSetCommand = "rollout player";
    SetMoveFilter(sz, prcSet->aaamfChequer[iPlayerSet]);

}

extern void
CommandSetRolloutPlayerLateMoveFilter(char *sz)
{

    szSetCommand = "rollout player late";
    SetMoveFilter(sz, prcSet->aaamfLate[iPlayerLateSet]);

}

extern void
CommandSetRolloutLateChequerplay(char *sz)
{

    evalcontext *pec[2];

    szSet = _("Chequer play for later moves in rollouts");
    szSetCommand = "rollout late chequerplay";

    pec[0] = prcSet->aecChequerLate;
    pec[1] = prcSet->aecChequerLate + 1;

    SetRolloutEvaluationContextBoth(sz, pec);

}


static void
SetRolloutEvaluationContext(char *sz, evalcontext * pec[], int iPlayer)
{

    g_assert((iPlayer == 0) || (iPlayer == 1));
    g_assert(pec[iPlayer] != 0);

    pecSet = pec[iPlayer];

    HandleCommand(sz, acSetEvaluation);
}

extern void
CommandSetRolloutPlayerChequerplay(char *sz)
{

    evalcontext *pec[2];

    szSet = iPlayerSet ? _("Chequer play in rollouts (for player 1)") : _("Chequer play in rollouts (for player 0)");
    szSetCommand = iPlayerSet ? "rollout player 1 chequerplay" : "rollout player 0 chequerplay";

    pec[0] = prcSet->aecChequer;
    pec[1] = prcSet->aecChequer + 1;
    SetRolloutEvaluationContext(sz, pec, iPlayerSet);
}

extern void
CommandSetRolloutPlayerLateChequerplay(char *sz)
{

    evalcontext *pec[2];

    szSet = iPlayerLateSet ?
        _("Chequer play for later moves in rollouts (for player 1)") :
        _("Chequer play for later moves in rollouts (for player 0)");
    szSetCommand = iPlayerLateSet ? "rollout late player 1 chequerplay" : "rollout late player 0 chequerplay";

    pec[0] = prcSet->aecChequerLate;
    pec[1] = prcSet->aecChequerLate + 1;
    SetRolloutEvaluationContext(sz, pec, iPlayerLateSet);
}


extern void
CommandSetRolloutCubedecision(char *sz)
{

    evalcontext *pec[2];

    szSet = _("Cube decisions in rollouts");
    szSetCommand = "rollout cubedecision";

    pec[0] = prcSet->aecCube;
    pec[1] = prcSet->aecCube + 1;

    SetRolloutEvaluationContextBoth(sz, pec);

}

extern void
CommandSetRolloutLateCubedecision(char *sz)
{

    evalcontext *pec[2];

    szSet = _("Cube decisions for later plies in rollouts");
    szSetCommand = "rollout late cubedecision";

    pec[0] = prcSet->aecCubeLate;
    pec[1] = prcSet->aecCubeLate + 1;

    SetRolloutEvaluationContextBoth(sz, pec);

}

extern void
CommandSetRolloutPlayerCubedecision(char *sz)
{

    evalcontext *pec[2];

    szSet = iPlayerSet ? _("Cube decisions in rollouts (for player 1)") :
        _("Cube decisions in rollouts (for player 0)");
    szSetCommand = iPlayerSet ? "rollout player 1 cubedecision" : "rollout player 0 cubedecision";

    pec[0] = prcSet->aecCube;
    pec[1] = prcSet->aecCube + 1;
    SetRolloutEvaluationContext(sz, pec, iPlayerSet);

}

extern void
CommandSetRolloutPlayerLateCubedecision(char *sz)
{

    evalcontext *pec[2];

    szSet = iPlayerLateSet ? _("Cube decisions for later plies of rollouts (for player 1)") :
        _("Cube decisions in later plies of rollouts (for player 0)");
    szSetCommand = iPlayerLateSet ? "rollout late player 1 cubedecision" : "rollout late player 0 cubedecision";

    pec[0] = prcSet->aecCubeLate;
    pec[1] = prcSet->aecCubeLate + 1;
    SetRolloutEvaluationContext(sz, pec, iPlayerLateSet);

}

extern void
CommandSetRolloutBearoffTruncationExact(char *sz)
{

    int f = prcSet->fTruncBearoff2;

    SetToggle("rollout bearofftruncation exact", &f, sz,
              _("Will truncate *cubeless* rollouts when reaching"
                " exact bearoff database"),
              _("Will not truncate *cubeless* rollouts when reaching" " exact bearoff database"));

    prcSet->fTruncBearoff2 = f;

}


extern void
CommandSetRolloutBearoffTruncationOS(char *sz)
{

    int f = prcSet->fTruncBearoffOS;

    SetToggle("rollout bearofftruncation onesided", &f, sz,
              _("Will truncate *cubeless* rollouts when reaching"
                " one-sided bearoff database"),
              _("Will not truncate *cubeless* rollouts when reaching" " one-sided bearoff database"));

    prcSet->fTruncBearoffOS = f;


}


extern void
CommandSetRolloutInitial(char *sz)
{

    int f = prcSet->fCubeful;

    SetToggle("rollout initial", &f, sz,
              _("Rollouts will be made as the initial position of a game."),
              _("Rollouts will be made for normal (non-opening) positions."));

    prcSet->fInitial = f;
}

extern void
CommandSetRolloutSeed(char *sz)
{

    if (prcSet->rngRollout == RNG_MANUAL) {
        outputl(_("You can't set a seed if you're using manual dice " "generation."));
        return;
    }

    if (*sz) {
        const int n = ParseNumber(&sz);

        if (n < 0) {
            outputl(_("You must specify a valid seed (see `help set seed')."));

            return;
        }

        prcSet->nSeed = n;
        outputf(_("Rollout seed set to %d.\n"), n);
    } else
        outputl(RNGSystemSeed(prcSet->rngRollout, rngctxRollout, NULL) ?
                _("Seed initialised from system random data.") : _("Seed initialised by system clock."));

}

extern void
CommandSetRolloutTrials(char *sz)
{

    int n = ParseNumber(&sz);

    if (n < 1) {
        outputl(_("You must specify a valid number of trials to make (see `help set rollout trials')."));

        return;
    }

    prcSet->nTrials = n;

    outputf(ngettext("%d game will be played per rollout.\n", "%d games will be played per rollout.\n", n), n);

}

extern void
CommandSetRolloutTruncationEnable(char *sz)
{
    int t = prcSet->fDoTruncate;

    if (SetToggle("rollout truncation enable", &t, sz,
                  _("Games in rollouts will be stopped after"
                    " a fixed number of moves."), _("Games in rollouts will be played out" " until the end.")) != -1) {
        prcSet->fDoTruncate = t;
    }
}

extern void
CommandSetRolloutCubeEqualChequer(char *sz)
{

    SetToggle("rollout cube-equal-chequer", &fCubeEqualChequer, sz,
              _("Rollouts use same settings for cube and chequer play."),
              _("Rollouts use separate settings for cube and chequer play."));
}

extern void
CommandSetRolloutPlayersAreSame(char *sz)
{

    SetToggle("rollout players-are-same", &fPlayersAreSame, sz,
              _("Rollouts use same settings for both players."), _("Rollouts use separate settings for both players."));
}

extern void
CommandSetRolloutTruncationEqualPlayer0(char *sz)
{

    SetToggle("rollout truncate-equal-player0", &fTruncEqualPlayer0, sz,
              _("Evaluation of rollouts at truncation point will be same as player 0."),
              _("Evaluation of rollouts at truncation point are separately specified."));
}

extern void
CommandSetRolloutTruncationPlies(char *sz)
{

    int n = ParseNumber(&sz);

    if (n < 0) {
        outputl(_("You must specify a valid ply at which to truncate rollouts " "(see `help set rollout')."));

        return;
    }

    prcSet->nTruncate = (unsigned short) n;

    if ((n == 0) || !prcSet->fDoTruncate)
        outputl(_("Rollouts will not be truncated."));
    else
        outputf(ngettext
                ("Rollouts will be truncated after %d ply.\n", "Rollouts will be truncated after %d plies.\n", n), n);

}

extern void
CommandSetRolloutTruncationChequer(char *sz)
{

    szSet = _("Chequer play evaluations at rollout truncation point");
    szSetCommand = "rollout truncation chequerplay";

    pecSet = &prcSet->aecChequerTrunc;

    HandleCommand(sz, acSetEvaluation);
}

extern void
CommandSetRolloutTruncationCube(char *sz)
{

    szSet = _("Cube decisions at rollout truncation point");
    szSetCommand = "rollout truncation cubedecision";

    pecSet = &prcSet->aecCubeTrunc;

    HandleCommand(sz, acSetEvaluation);
}


extern void
CommandSetRolloutVarRedn(char *sz)
{

    int f = prcSet->fVarRedn;

    SetToggle("rollout varredn", &f, sz,
              _("Will use lookahead during rollouts to reduce variance."),
              _("Will not use lookahead variance " "reduction during rollouts."));

    prcSet->fVarRedn = f;
}


extern void
CommandSetRolloutRotate(char *sz)
{

    int f = prcSet->fRotate;

    SetToggle("rollout quasirandom", &f, sz,
              _("Use quasi-random dice in rollouts"), _("Do not use quasi-random dice in rollouts"));

    prcSet->fRotate = f;

}



extern void
CommandSetRolloutCubeful(char *sz)
{

    int f = prcSet->fCubeful;

    SetToggle("rollout cubeful", &f, sz,
              _("Cubeful rollouts will be performed."), _("Cubeless rollouts will be performed."));

    prcSet->fCubeful = f;
}


extern void
CommandSetRolloutPlayer(char *sz)
{

    char *pch = NextToken(&sz), *pchCopy;
    int i;

    if (!pch) {
        outputf(_("You must specify a player (see `help set %s player').\n"), szSetCommand);

        return;
    }

    if ((i = ParsePlayer(pch)) == 0 || i == 1) {
        iPlayerSet = i;

        HandleCommand(sz, acSetRolloutPlayer);

        return;
    }

    if (i == 2) {
        if ((pchCopy = malloc(strlen(sz) + 1)) == 0) {
            outputl(_("Insufficient memory."));

            return;
        }

        strcpy(pchCopy, sz);

        outputpostpone();

        iPlayerSet = 0;
        HandleCommand(sz, acSetRolloutPlayer);

        iPlayerSet = 1;
        HandleCommand(pchCopy, acSetRolloutPlayer);

        outputresume();

        free(pchCopy);

        return;
    }

    outputf(_("Unknown player `%s'\n" "(see `help set %s player').\n"), pch, szSetCommand);
}

extern void
CommandSetRolloutLatePlayer(char *sz)
{

    char *pch = NextToken(&sz), *pchCopy;
    int i;

    if (!pch) {
        outputf(_("You must specify a player (see `help set %s player').\n"), szSetCommand);

        return;
    }

    if ((i = ParsePlayer(pch)) == 0 || i == 1) {
        iPlayerLateSet = i;

        HandleCommand(sz, acSetRolloutLatePlayer);

        return;
    }

    if (i == 2) {
        if ((pchCopy = malloc(strlen(sz) + 1)) == 0) {
            outputl(_("Insufficient memory."));

            return;
        }

        strcpy(pchCopy, sz);

        outputpostpone();

        iPlayerLateSet = 0;
        HandleCommand(sz, acSetRolloutLatePlayer);

        iPlayerLateSet = 1;
        HandleCommand(pchCopy, acSetRolloutLatePlayer);

        outputresume();

        free(pchCopy);

        return;
    }

    outputf(_("Unknown player `%s'\n" "(see `help set %s player').\n"), pch, szSetCommand);
}

extern void
CommandSetScore(char *sz)
{

    moverecord *pmr;
    xmovegameinfo *pmgi;
    const char *pch0, *pch1;
    char *pchEnd0, *pchEnd1;
    int n0, n1, fCrawford0, fCrawford1, fPostCrawford0, fPostCrawford1;

    if ((pch0 = NextToken(&sz)) == 0)
        pch0 = "";

    if ((pch1 = NextToken(&sz)) == 0)
        pch1 = "";

    n0 = (int) strtol(pch0, &pchEnd0, 10);
    if (pch0 == pchEnd0)
        n0 = INT_MIN;

    n1 = (int) strtol(pch1, &pchEnd1, 10);
    if (pch1 == pchEnd1)
        n1 = INT_MIN;

    if (((fCrawford0 = *pchEnd0 == '*' ||
          (*pchEnd0 && !StrNCaseCmp(pchEnd0, "crawford",
                                    strlen(pchEnd0)))) &&
         n0 != INT_MIN && n0 != -1 && n0 != ms.nMatchTo - 1) ||
        ((fCrawford1 = *pchEnd1 == '*' ||
          (*pchEnd1 && !StrNCaseCmp(pchEnd1, "crawford",
                                    strlen(pchEnd1)))) && n1 != INT_MIN && n1 != -1 && n1 != ms.nMatchTo - 1)) {
        outputl(_("The Crawford rule applies only in match play when a " "player's score is 1-away."));
        return;
    }

    if (((fPostCrawford0 = (*pchEnd0 && !StrNCaseCmp(pchEnd0, "postcrawford", strlen(pchEnd0)))) &&
         n0 != INT_MIN && n0 != -1 && n0 != ms.nMatchTo - 1) ||
        ((fPostCrawford1 = (*pchEnd1 && !StrNCaseCmp(pchEnd1, "postcrawford", strlen(pchEnd1)))) &&
         n1 != INT_MIN && n1 != -1 && n1 != ms.nMatchTo - 1)) {
        outputl(_("The Crawford rule applies only in match play when a " "player's score is 1-away."));
        return;
    }

    if (!ms.nMatchTo && (fCrawford0 || fCrawford1 || fPostCrawford0 || fPostCrawford1)) {
        outputl(_("The Crawford rule applies only in match play when a " "player's score is 1-away."));
        return;
    }

    if (fCrawford0 && fCrawford1) {
        outputl(_("You cannot set the Crawford rule when both players' scores " "are 1-away."));
        return;
    }

    if ((fCrawford0 && fPostCrawford1) || (fCrawford1 && fPostCrawford0)) {
        outputl(_("You cannot set both Crawford and post-Crawford " "simultaneously."));
        return;
    }

    /* silently ignore the case where both players are set post-Crawford;
     * assume that is unambiguous and means double match point */

    if (fCrawford0 || fPostCrawford0)
        n0 = -1;

    if (fCrawford1 || fPostCrawford1)
        n1 = -1;

    if (n0 < 0)                 /* -n means n-away */
        n0 += ms.nMatchTo;

    if (n1 < 0)
        n1 += ms.nMatchTo;

    if ((fPostCrawford0 && !n1) || (fPostCrawford1 && !n0)) {
        outputl(_("You cannot set post-Crawford play if the trailer has yet " "to score."));
        return;
    }

    if (n0 < 0 || n1 < 0) {
        outputl(_("You must specify two valid scores."));
        return;
    }

    if (ms.nMatchTo && n0 >= ms.nMatchTo && n1 >= ms.nMatchTo) {
        outputl(_("Only one player may win the match."));
        return;
    }

    if ((fCrawford0 || fCrawford1) && (n0 >= ms.nMatchTo || n1 >= ms.nMatchTo)) {
        outputl(_("You cannot play the Crawford game once the match is " "already over."));
        return;
    }

    /* allow scores above the match length, since that doesn't really
     * hurt anything */

    CancelCubeAction();

    ms.anScore[0] = n0;
    ms.anScore[1] = n1;

    if (ms.nMatchTo) {
        if (n0 != ms.nMatchTo - 1 && n1 != ms.nMatchTo - 1)
            /* must be pre-Crawford */
            ms.fCrawford = ms.fPostCrawford = FALSE;
        else if ((n0 == ms.nMatchTo - 1 && n1 == ms.nMatchTo - 1) || fPostCrawford0 || fPostCrawford1) {
            /* must be post-Crawford */
            ms.fCrawford = FALSE;
            ms.fPostCrawford = ms.nMatchTo > 1;
        } else {
            /* possibly the Crawford game */
            if (n0 >= ms.nMatchTo || n1 >= ms.nMatchTo)
                ms.fCrawford = FALSE;
            else if (fCrawford0 || fCrawford1 || !n0 || !n1)
                ms.fCrawford = TRUE;

            ms.fPostCrawford = !ms.fCrawford;
        }
    }

    if (ms.gs < GAME_OVER && plGame && (pmr = (moverecord *) plGame->plNext->p) && (pmgi = &pmr->g)) {
        g_assert(pmr->mt == MOVE_GAMEINFO);
        pmgi->anScore[0] = ms.anScore[0];
        pmgi->anScore[1] = ms.anScore[1];
        pmgi->fCrawfordGame = ms.fCrawford;
#if USE_GTK
        /* The score this game was started at is displayed in the option
         * menu, and is now out of date. */
        if (fX)
            GTKRegenerateGames();
#endif                          /* USE_GTK */
    }

    CommandShowScore(NULL);

#if USE_GTK
    if (fX)
        ShowBoard();
#endif                          /* USE_GTK */
}

extern void
CommandSetSeed(char *sz)
{
    SetSeed(rngCurrent, rngctxCurrent, sz);
}

extern void
CommandSetToolbar(char *sz)
{
    if (!StrCaseCmp("on", sz) || !StrCaseCmp("off", sz)) {
#if USE_GTK
        if (!StrCaseCmp("on", sz)) {
            if (!fToolbarShowing)
                ShowToolbar();
        } else {
            if (fToolbarShowing)
                HideToolbar();
        }
#endif
    } else {
        int n = ParseNumber(&sz);

        if (n != 0 && n != 1 && n != 2) {
            outputl(_("You must specify either 0, 1 or 2"));
            return;
        }
#if USE_GTK
        if (fX)
            SetToolbarStyle(n);
#endif
    }
}

extern void
SetTurn(int i)
{
    if (ms.fTurn != i)
        SwapSides(ms.anBoard);

    ms.fTurn = ms.fMove = i;
    CancelCubeAction();
    pmr_hint_destroy();
    fNextTurn = FALSE;
#if USE_GTK
    if (fX) {

        BoardData *bd = BOARD(pwBoard)->board_data;
        bd->diceRoll[0] = bd->diceRoll[1] = -1;
        fJustSwappedPlayers = TRUE;
    }
#endif
    ms.anDice[0] = ms.anDice[1] = 0;


    UpdateSetting(&ms.fTurn);

#if USE_GTK
    if (fX)
        ShowBoard();
#endif                          /* USE_GTK */

    return;
}

extern void
CommandSetTurn(char *sz)
{

    char *pch = NextToken(&sz);
    int i;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("There must be a game in progress to set a player on roll."));

        return;
    }

    if (ms.fResigned) {
        outputl(_("Please resolve the resignation first."));

        return;
    }

    if (!pch) {
        outputl(_("Which player do you want to set on roll?"));

        return;
    }

    if ((i = ParsePlayer(pch)) < 0) {
        outputf(_("Unknown player `%s' (see `help set turn').\n"), pch);

        return;
    }

    if (i == 2) {
        outputl(_("You can't set both players on roll."));

        return;
    }

    SetTurn(i);

    outputf(_("`%s' is now on roll.\n"), ap[i].szName);
}

extern void
CommandSetJacoby(char *sz)
{

    if (SetToggle("jacoby", &fJacoby, sz,
                  _("Will use the Jacoby rule for money sessions."),
                  _("Will not use the Jacoby rule for money sessions.")))
        return;

    if (fJacoby && !ms.fCubeUse)
        outputl(_("Note that you'll have to enable the cube if you want "
                  "gammons and backgammons\nto be scored (see `help set " "cube use')."));

    ms.fJacoby = fJacoby;

}

extern void
CommandSetCrawford(char *sz)
{

    moverecord *pmr;
    xmovegameinfo *pmgi;

    if (ms.nMatchTo > 0) {
        if ((ms.nMatchTo - ms.anScore[0] == 1) || (ms.nMatchTo - ms.anScore[1] == 1)) {

            if (SetToggle("crawford", &ms.fCrawford, sz,
                          _("This game is the Crawford game (no doubling allowed)."),
                          _("This game is not the Crawford game.")) < 0)
                return;

            /* sanity check */
            ms.fPostCrawford = !ms.fCrawford;

            if (ms.fCrawford)
                CancelCubeAction();

            if (plGame && (pmr = plGame->plNext->p) && (pmgi = &pmr->g)) {
                g_assert(pmr->mt == MOVE_GAMEINFO);
                pmgi->fCrawfordGame = ms.fCrawford;
            }
        } else {
            if (ms.fCrawford) { /* Allow crawford to be turned off if set at incorrect score */
                SetToggle("crawford", &ms.fCrawford, sz,
                          _("This game is the Crawford game (no doubling allowed)."),
                          _("This game is not the Crawford game."));
                return;
            }
            outputl(_("Cannot set whether this is the Crawford game\n"
                      "as none of the players are 1-away from winning."));
        }
        /* Clear previous data in the hint cache after toggling Crawford */
        pmr_hint_destroy();
    } else if (!ms.nMatchTo)
        outputl(_("Cannot set Crawford play for money sessions."));
    else
        outputl(_("No match in progress (type `new match n' to start one)."));
}

extern void
CommandSetPostCrawford(char *sz)
{

    moverecord *pmr;
    xmovegameinfo *pmgi;

    if (ms.nMatchTo > 0) {
        if ((ms.nMatchTo - ms.anScore[0] == 1) || (ms.nMatchTo - ms.anScore[1] == 1)) {

            SetToggle("postcrawford", &ms.fPostCrawford, sz,
                      _("This is post-Crawford play (doubling allowed)."), _("This is not post-Crawford play."));

            /* sanity check */
            ms.fCrawford = !ms.fPostCrawford;

            if (ms.fCrawford)
                CancelCubeAction();

            if (plGame && (pmr = plGame->plNext->p) && (pmgi = &pmr->g)) {
                g_assert(pmr->mt == MOVE_GAMEINFO);
                pmgi->fCrawfordGame = ms.fCrawford;
            }
        } else {
            outputl(_("Cannot set whether this is post-Crawford play\n"
                      "as none of the players are 1-away from winning."));
        }
    } else if (!ms.nMatchTo)
        outputl(_("Cannot set post-Crawford play for money sessions."));
    else
        outputl(_("No match in progress (type `new match n' to start one)."));

}

#if USE_GTK
extern void
CommandSetWarning(char *sz)
{
    char buf[100];
    warningType warning;
    char *pValue = strchr(sz, ' ');

    if (!pValue) {
        outputl(_("Incorrect syntax for set warning command."));
        return;
    }
    *pValue++ = '\0';

    warning = ParseWarning(sz);
    if ((int) warning < 0) {
        sprintf(buf, _("Unknown warning %s."), sz);
        outputl(buf);
        return;
    }

    while (*pValue == ' ')
        pValue++;

    if (!StrCaseCmp(pValue, "on")) {
        SetWarningEnabled(warning, TRUE);
    } else if (!StrCaseCmp(pValue, "off")) {
        SetWarningEnabled(warning, FALSE);
    } else {
        sprintf(buf, _("Unknown value %s."), pValue);
        outputl(buf);
        return;
    }
    sprintf(buf, _("Warning %s set to %s."), sz, pValue);
    outputl(buf);
}

extern void
CommandShowWarning(char *sz)
{
    warningType warning;

    while (*sz == ' ')
        sz++;

    if (!*sz) {                 /* Show all warnings */
        for (warning = 0; warning < WARN_NUM_WARNINGS; warning++)
            PrintWarning(warning);
    } else {                    /* Show specific warning */
        warning = ParseWarning(sz);
        if ((int) warning < 0) {
            char buf[100];
            sprintf(buf, _("Unknown warning %s."), sz);
            outputl(buf);
            return;
        }
        PrintWarning(warning);
    }
}
#endif

extern void
CommandSetBeavers(char *sz)
{

    int n;

    if ((n = ParseNumber(&sz)) < 0) {
        outputl(_("You must specify the number of beavers to allow."));

        return;
    }

    nBeavers = (unsigned int) n;

    if (nBeavers > 1)
        outputf(_("%d beavers/raccoons allowed in money sessions.\n"), nBeavers);
    else if (nBeavers == 1)
        outputl(_("1 beaver allowed in money sessions."));
    else
        outputl(_("No beavers allowed in money sessions."));
}

extern void
CommandSetOutputDigits(char *sz)
{

    int n = ParseNumber(&sz);

    if (n < 0 || n > 6) {
        outputl(_("You must specify a number between 1 and 6.\n"));
        return;
    }

    fOutputDigits = (unsigned int) n;

    outputf(_("Probabilities and equities will be shown with %d digits "
              "after the decimal separator\n"), fOutputDigits);

#if USE_GTK
    MoveListRefreshSize();
#endif
}


extern void
CommandSetOutputMatchPC(char *sz)
{

    SetToggle("output matchpc", &fOutputMatchPC, sz,
              _("Match winning chances will be shown as percentages."),
              _("Match winning chances will be shown as probabilities."));
}

extern void
CommandSetOutputMWC(char *sz)
{

    SetToggle("output mwc", &fOutputMWC, sz,
              _("Match evaluations will be shown as match winning chances."),
              _("Match evaluations will be shown as equivalent money equity."));
}

extern void
CommandSetOutputRawboard(char *sz)
{

    SetToggle("output rawboard", &fOutputRawboard, sz,
              _("TTY boards will be given in raw format."), _("TTY boards will be given in ASCII."));
}

extern void
CommandSetOutputWinPC(char *sz)
{

    SetToggle("output winpc", &fOutputWinPC, sz,
              _("Game winning chances will be shown as percentages."),
              _("Game winning chances will be shown as probabilities."));
}

static void
SetInvertMET(void)
{
    invertMET();
    /* Clear any stored results to stop previous table causing problems */
    EvalCacheFlush();
    pmr_hint_destroy();
}

extern void
CommandSetMET(char *sz)
{

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a filename. " "See \"help set met\". "));
        return;
    }

    InitMatchEquity(sz);
    /* Cubeful evaluation get confused with entries from another table */
    EvalCacheFlush();

    /* clear hint */
    CommandClearHint(NULL);

    outputf(_("GNU Backgammon will now use the %s match equity table.\n"), miCurrent.szName);

    if (miCurrent.nLength < MAXSCORE && miCurrent.nLength != -1) {

        outputf(_("\n"
                  "Note that this match equity table only supports "
                  "matches of length %i and below.\n"
                  "For scores above %i-away an extrapolation "
                  "scheme is used.\n"), miCurrent.nLength, miCurrent.nLength);

    }
    if (fInvertMET)
        SetInvertMET();
}

extern void
CommandSetEvalParamType(char *sz)
{
    switch (sz[0]) {

    case 'r':
        pesSet->et = EVAL_ROLLOUT;
        break;

    case 'e':
        pesSet->et = EVAL_EVAL;
        break;

    default:
        outputf(_("Unknown evaluation type: %s (see\n" "`help set %s type').\n"), sz, szSetCommand);
        return;

    }

    outputf(_("%s will now use %s.\n"), szSet, gettext(aszEvalType[pesSet->et]));
}


extern void
CommandSetEvalParamEvaluation(char *sz)
{

    pecSet = &pesSet->ec;

    HandleCommand(sz, acSetEvaluation);

    if (pesSet->et != EVAL_EVAL)
        outputf(_("(Note that this setting will have no effect until you\n"
                  "`set %s type evaluation'.)\n"), szSetCommand);
}


extern void
CommandSetEvalParamRollout(char *sz)
{

    prcSet = &pesSet->rc;

    HandleCommand(sz, acSetRollout);

    if (pesSet->et != EVAL_ROLLOUT)
        outputf(_("(Note that this setting will have no effect until you\n" "`set %s type rollout.)'\n"), szSetCommand);

}

extern void
CommandSetEvalSameAsAnalysis(char *sz)
{
    SetToggle("eval sameasanalysis", &fEvalSameAsAnalysis, sz,
              _("Evaluation settings will be same as analysis settings."),
              _("Evaluation settings separate from analysis settings."));
}

extern void
CommandSetAnalysisPlayer(char *sz)
{

    char *pch = NextToken(&sz), *pchCopy;
    int i;

    if (!pch) {
        outputl(_("You must specify a player " "(see `help set analysis player')."));
        return;
    }

    if ((i = ParsePlayer(pch)) == 0 || i == 1) {
        iPlayerSet = i;

        HandleCommand(sz, acSetAnalysisPlayer);

        return;
    }

    if (i == 2) {
        if ((pchCopy = malloc(strlen(sz) + 1)) == 0) {
            outputl(_("Insufficient memory."));

            return;
        }

        strcpy(pchCopy, sz);

        outputpostpone();

        iPlayerSet = 0;
        HandleCommand(sz, acSetAnalysisPlayer);

        iPlayerSet = 1;
        HandleCommand(pchCopy, acSetAnalysisPlayer);

        outputresume();

        free(pchCopy);

        return;
    }

    outputf(_("Unknown player `%s'\n" "(see `help set analysis player').\n"), pch);

}


extern void
CommandSetAnalysisPlayerAnalyse(char *sz)
{

    char sz1[100];
    char sz2[100];

    sprintf(sz1, _("Analyse %s's chequerplay and cube decisions."), ap[iPlayerSet].szName);

    sprintf(sz2, _("Do not analyse %s's chequerplay and cube decisions."), ap[iPlayerSet].szName);

    SetToggle("analysis player", &afAnalysePlayers[iPlayerSet], sz, sz1, sz2);

}


extern void
CommandSetAnalysisChequerplay(char *sz)
{

    pesSet = &esAnalysisChequer;

    szSet = _("Analysis chequerplay");
    szSetCommand = "analysis chequerplay";

    HandleCommand(sz, acSetEvalParam);

}

extern void
CommandSetAnalysisCubedecision(char *sz)
{


    pesSet = &esAnalysisCube;

    szSet = _("Analysis cubedecision");
    szSetCommand = "analysis cubedecision";

    HandleCommand(sz, acSetEvalParam);

}


extern void
CommandSetEvalChequerplay(char *sz)
{

    pesSet = &esEvalChequer;

    szSet = _("`eval' and `hint' chequerplay");
    szSetCommand = "evaluation chequerplay ";

    HandleCommand(sz, acSetEvalParam);

}

extern void
CommandSetEvalCubedecision(char *sz)
{


    pesSet = &esEvalCube;

    szSet = _("`eval' and `hint' cube decisions");
    szSetCommand = "evaluation cubedecision ";

    HandleCommand(sz, acSetEvalParam);

}


extern void
CommandSetEvalMoveFilter(char *sz)
{

    szSetCommand = "evaluation";
    SetMoveFilter(sz, aamfEval);

}

extern void
CommandSetAnalysisMoveFilter(char *sz)
{

    szSetCommand = "analysis";
    SetMoveFilter(sz, aamfAnalysis);

}




extern void
SetMatchInfo(char **ppch, const char *sz, char *szMessage)
{
    if (*ppch)
        g_free(*ppch);

    if (sz && *sz) {
        *ppch = g_strdup(sz);
        if (szMessage)
            outputf(_("%s set to: %s\n"), szMessage, sz);
    } else {
        *ppch = NULL;
        if (szMessage)
            outputf(_("%s cleared.\n"), szMessage);
    }
}

extern void
CommandSetMatchAnnotator(char *sz)
{

    SetMatchInfo(&mi.pchAnnotator, sz, _("Match annotator"));
}

extern void
CommandSetMatchComment(char *sz)
{

    SetMatchInfo(&mi.pchComment, sz, _("Match comment"));
}

static int
DaysInMonth(int nYear, int nMonth)
{

    static int an[12] = { 31, -1, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (nMonth < 1 || nMonth > 12)
        return -1;
    else if (nMonth != 2)
        return an[nMonth - 1];
    else if (nYear % 4 || (!(nYear % 100) && nYear % 400))
        return 28;
    else
        return 29;
}

extern void
CommandSetMatchDate(char *sz)
{

    int nYear, nMonth, nDay;

    if (!sz || !*sz) {
        mi.nYear = 0;
        outputl(_("Match date cleared."));
        return;
    }

    if (sscanf(sz, "%4d-%2d-%2d", &nYear, &nMonth, &nDay) < 3 ||
        nYear < 1753 || nMonth < 1 || nMonth > 12 || nDay < 1 || nDay > DaysInMonth(nYear, nMonth)) {
        outputf(_("%s is not a valid date (see `help set matchinfo " "date').\n"), sz);
        return;
    }

    mi.nYear = nYear;
    mi.nMonth = nMonth;
    mi.nDay = nDay;

    outputf(_("Match date set to %04d-%02d-%02d.\n"), nYear, nMonth, nDay);
}

extern void
CommandSetMatchEvent(char *sz)
{

    SetMatchInfo(&mi.pchEvent, sz, _("Match event"));
}

extern void
CommandSetMatchLength(char *sz)
{

    unsigned int n = ParseNumber(&sz);

    nDefaultLength = n;

    outputf(ngettext("New matches default to %d point.\n", "New matches default to %d points.\n", nDefaultLength),
            nDefaultLength);

}

extern void
CommandSetMatchPlace(char *sz)
{

    SetMatchInfo(&mi.pchPlace, sz, _("Match place"));
}

extern void
CommandSetMatchRating(char *sz)
{

    int n;
    char szMessage[64];

    if ((n = ParsePlayer(NextToken(&sz))) < 0) {
        outputl(_("You must specify which player's rating to set (see `help " "set matchinfo rating')."));
        return;
    }

    sprintf(szMessage, _("Rating for %s"), ap[n].szName);

    SetMatchInfo(&mi.pchRating[n], sz, szMessage);
}

extern void
CommandSetMatchRound(char *sz)
{

    SetMatchInfo(&mi.pchRound, sz, _("Match round"));
}

extern void
CommandSetMatchID(char *sz)
{

    SetMatchID(sz);

}

extern void
CommandSetExportIncludeAnnotations(char *sz)
{

    SetToggle("annotations", &exsExport.fIncludeAnnotation, sz,
              _("Include annotations in exports"), _("Do not include annotations in exports"));

}

extern void
CommandSetExportIncludeAnalysis(char *sz)
{

    SetToggle("analysis", &exsExport.fIncludeAnalysis, sz,
              _("Include analysis in exports"), _("Do not include analysis in exports"));

}

extern void
CommandSetExportIncludeStatistics(char *sz)
{

    SetToggle("statistics", &exsExport.fIncludeStatistics, sz,
              _("Include statistics in exports"), _("Do not include statistics in exports"));

}

extern void
CommandSetExportIncludeMatchInfo(char *sz)
{

    SetToggle("matchinfo", &exsExport.fIncludeMatchInfo, sz,
              _("Include match information in exports"), _("Do not include match information in exports"));

}

extern void
CommandSetExportShowBoard(char *sz)
{

    int n;

    if ((n = ParseNumber(&sz)) < 0) {
        outputl(_("You must specify a semi-positive number."));

        return;
    }

    exsExport.fDisplayBoard = n;

    if (!n)
        output(_("The board will never been shown in exports."));
    else
        outputf(_("The board will be shown every %d. move in exports."), n);

}


extern void
CommandSetExportShowPlayer(char *sz)
{

    int i;

    if ((i = ParsePlayer(sz)) < 0) {
        outputf(_("Unknown player `%s' " "(see `help set export show player').\n"), sz);
        return;
    }

    exsExport.fSide = i + 1;

    if (i == 2)
        outputl(_("Analysis, boards etc will be " "shown for both players in exports."));
    else
        outputf(_("Analysis, boards etc will only be shown for " "player %s in exports.\n"), ap[i].szName);

}


extern void
CommandSetExportMovesNumber(char *sz)
{

    int n;

    if ((n = ParseNumber(&sz)) < 0) {
        outputl(_("You must specify a semi-positive number."));

        return;
    }

    exsExport.nMoves = n;

    outputf(_("Show at most %d moves in exports.\n"), n);

}

extern void
CommandSetExportMovesProb(char *sz)
{

    SetToggle("probabilities", &exsExport.fMovesDetailProb, sz,
              _("Show detailed probabilities for moves"), _("Do not show detailed probabilities for moves"));

}

static int *pParameter;

extern void
CommandSetExportMovesParameters(char *sz)
{

    pParameter = exsExport.afMovesParameters;
    HandleCommand(sz, acSetExportParameters);

}

extern void
CommandSetExportCubeProb(char *sz)
{

    SetToggle("probabilities", &exsExport.fCubeDetailProb, sz,
              _("Show detailed probabilities for cube decisions"),
              _("Do not show detailed probabilities for cube decisions"));

}

extern void
CommandSetExportCubeParameters(char *sz)
{


    pParameter = exsExport.afCubeParameters;
    HandleCommand(sz, acSetExportParameters);

}


extern void
CommandSetExportParametersEvaluation(char *sz)
{

    SetToggle("evaluation", &pParameter[0], sz,
              _("Show detailed parameters for evaluations"), _("Do not show detailed parameters for evaluations"));

}

extern void
CommandSetExportParametersRollout(char *sz)
{

    SetToggle("rollout", &pParameter[1], sz,
              _("Show detailed parameters for rollouts"), _("Do not show detailed parameters for rollouts"));

}


extern void
CommandSetExportMovesDisplayVeryBad(char *sz)
{

    SetToggle("export moves display very bad",
              &exsExport.afMovesDisplay[SKILL_VERYBAD], sz,
              _("Export moves marked 'very bad'."), _("Do not export moves marked 'very bad'."));

}

extern void
CommandSetExportMovesDisplayBad(char *sz)
{

    SetToggle("export moves display bad",
              &exsExport.afMovesDisplay[SKILL_BAD], sz,
              _("Export moves marked 'bad'."), _("Do not export moves marked 'bad'."));

}

extern void
CommandSetExportMovesDisplayDoubtful(char *sz)
{

    SetToggle("export moves display doubtful",
              &exsExport.afMovesDisplay[SKILL_DOUBTFUL], sz,
              _("Export moves marked 'doubtful'."), _("Do not export moves marked 'doubtful'."));

}

extern void
CommandSetExportMovesDisplayUnmarked(char *sz)
{

    SetToggle("export moves display unmarked",
              &exsExport.afMovesDisplay[SKILL_NONE], sz,
              _("Export unmarked moves."), _("Do not export unmarked moves."));

}

extern void
CommandSetExportCubeDisplayVeryBad(char *sz)
{

    SetToggle("export cube display very bad",
              &exsExport.afCubeDisplay[SKILL_VERYBAD], sz,
              _("Export cube decisions marked 'very bad'."), _("Do not export cube decisions marked 'very bad'."));

}

extern void
CommandSetExportCubeDisplayBad(char *sz)
{

    SetToggle("export cube display bad",
              &exsExport.afCubeDisplay[SKILL_BAD], sz,
              _("Export cube decisions marked 'bad'."), _("Do not export cube decisions marked 'bad'."));

}

extern void
CommandSetExportCubeDisplayDoubtful(char *sz)
{

    SetToggle("export cube display doubtful",
              &exsExport.afCubeDisplay[SKILL_DOUBTFUL], sz,
              _("Export cube decisions marked 'doubtful'."), _("Do not export cube decisions marked 'doubtful'."));

}

extern void
CommandSetExportCubeDisplayUnmarked(char *sz)
{

    SetToggle("export cube display unmarked",
              &exsExport.afCubeDisplay[SKILL_NONE], sz,
              _("Export unmarked cube decisions."), _("Do not export unmarked cube decisions."));

}

extern void
CommandSetExportCubeDisplayActual(char *sz)
{

    SetToggle("export cube display actual",
              &exsExport.afCubeDisplay[EXPORT_CUBE_ACTUAL], sz,
              _("Export actual cube decisions."), _("Do not export actual cube decisions."));

}

extern void
CommandSetExportCubeDisplayClose(char *sz)
{

    SetToggle("export cube display close",
              &exsExport.afCubeDisplay[EXPORT_CUBE_CLOSE], sz,
              _("Export close cube decisions."), _("Do not export close cube decisions."));

}

extern void
CommandSetExportCubeDisplayMissed(char *sz)
{

    SetToggle("export cube display missed",
              &exsExport.afCubeDisplay[EXPORT_CUBE_MISSED], sz,
              _("Export missed cube decisions."), _("Do not export missed cube decisions."));

}

static void
SetExportHTMLType(const htmlexporttype het, const char *szExtension)
{

    if (exsExport.szHTMLExtension)
        g_free(exsExport.szHTMLExtension);

    exsExport.het = het;
    exsExport.szHTMLExtension = g_strdup(szExtension);

    outputf(_("HTML export type is now: \n" "%s\n"), aszHTMLExportType[exsExport.het]);

}

extern void
CommandSetExportHTMLTypeBBS(char *UNUSED(sz))
{

    SetExportHTMLType(HTML_EXPORT_TYPE_BBS, "gif");

}

extern void
CommandSetExportHTMLTypeFibs2html(char *UNUSED(sz))
{

    SetExportHTMLType(HTML_EXPORT_TYPE_FIBS2HTML, "gif");

}

extern void
CommandSetExportHTMLTypeGNU(char *UNUSED(sz))
{

    SetExportHTMLType(HTML_EXPORT_TYPE_GNU, "png");

}


static void
SetExportHTMLCSS(const htmlexportcss hecss)
{

    if (exsExport.hecss == hecss)
        return;

    if (exsExport.hecss == HTML_EXPORT_CSS_EXTERNAL)
        CommandNotImplemented(NULL);

    exsExport.hecss = hecss;

    outputf(_("CSS stylesheet for HTML export: %s\n"), gettext(aszHTMLExportCSS[hecss]));

}


extern void
CommandSetExportHTMLCSSHead(char *UNUSED(sz))
{

    SetExportHTMLCSS(HTML_EXPORT_CSS_HEAD);

}

extern void
CommandSetExportHTMLCSSInline(char *UNUSED(sz))
{

    SetExportHTMLCSS(HTML_EXPORT_CSS_INLINE);

}

extern void
CommandSetExportHTMLCSSExternal(char *UNUSED(sz))
{

    SetExportHTMLCSS(HTML_EXPORT_CSS_EXTERNAL);

}


extern void
CommandSetExportHTMLPictureURL(char *sz)
{

    if (!sz || !*sz) {
        outputl(_("You must specify a URL. " "See `help set export html pictureurl'."));
        return;
    }

    if (exsExport.szHTMLPictureURL)
        g_free(exsExport.szHTMLPictureURL);

    sz = NextToken(&sz);
    exsExport.szHTMLPictureURL = g_strdup(sz);

    outputf(_("URL for picture in HTML export is now: \n" "%s\n"), exsExport.szHTMLPictureURL);

}



extern void
CommandSetInvertMatchEquityTable(char *sz)
{

    int fOldInvertMET = fInvertMET;

    if (SetToggle("invert matchequitytable", &fInvertMET, sz,
                  _("Match equity table will be used inverted."),
                  _("Match equity table will not be use inverted.")) >= 0)
        UpdateSetting(&fInvertMET);

    if (fOldInvertMET != fInvertMET)
        SetInvertMET();
}


extern void
CommandSetTutorMode(char *sz)
{

    SetToggle("tutor-mode", &fTutor, sz, _("Warn about possibly bad play."), _("No warnings for possibly bad play."));
}

extern void
CommandSetTutorCube(char *sz)
{

    SetToggle("tutor-cube", &fTutorCube, sz,
              _("Include advice on cube decisions in tutor mode."),
              _("Exclude advice on cube decisions from tutor mode."));
}

extern void
CommandSetTutorChequer(char *sz)
{

    SetToggle("tutor-chequer", &fTutorChequer, sz,
              _("Include advice on chequer play in tutor mode."), _("Exclude advice on chequer play from tutor mode."));
}

static void
_set_tutor_skill(skilltype Skill, int skillno, char *skill)
{

    nTutorSkillCurrent = skillno;
    TutorSkill = Skill;
    outputf(_("Tutor warnings will be given for play marked `%s'.\n"), skill);
}

extern void
CommandSetTutorSkillDoubtful(char *UNUSED(sz))
{

    _set_tutor_skill(SKILL_DOUBTFUL, 0, _("doubtful"));
}

extern void
CommandSetTutorSkillBad(char *UNUSED(sz))
{

    _set_tutor_skill(SKILL_BAD, 1, _("bad"));
}

extern void
CommandSetTutorSkillVeryBad(char *UNUSED(sz))
{

    _set_tutor_skill(SKILL_VERYBAD, 2, _("very bad"));
}

/*
 * Sounds
 */

/* enable/disable sounds */

extern void
CommandSetSoundEnable(char *sz)
{

    SetToggle("sound enable", &fSound, sz, _("Enable sounds."), _("Disable sounds."));

}

/* sound system */
extern void
CommandSetSoundSystemCommand(char *sz)
{
    sound_set_command(sz);
}

extern void
CommandSetSoundSoundAgree(char *sz)
{

    SetSoundFile(SOUND_AGREE, NextToken(&sz));

}

extern void
CommandSetSoundSoundAnalysisFinished(char *sz)
{

    SetSoundFile(SOUND_ANALYSIS_FINISHED, NextToken(&sz));

}

extern void
CommandSetSoundSoundBotDance(char *sz)
{

    SetSoundFile(SOUND_BOT_DANCE, NextToken(&sz));

}

extern void
CommandSetSoundSoundBotWinGame(char *sz)
{

    SetSoundFile(SOUND_BOT_WIN_GAME, NextToken(&sz));

}

extern void
CommandSetSoundSoundBotWinMatch(char *sz)
{

    SetSoundFile(SOUND_BOT_WIN_MATCH, NextToken(&sz));

}

extern void
CommandSetSoundSoundChequer(char *sz)
{

    SetSoundFile(SOUND_CHEQUER, NextToken(&sz));

}

extern void
CommandSetSoundSoundDouble(char *sz)
{

    SetSoundFile(SOUND_DOUBLE, NextToken(&sz));

}

extern void
CommandSetSoundSoundDrop(char *sz)
{

    SetSoundFile(SOUND_DROP, NextToken(&sz));

}

extern void
CommandSetSoundSoundExit(char *sz)
{

    SetSoundFile(SOUND_EXIT, NextToken(&sz));

}

extern void
CommandSetSoundSoundHumanDance(char *sz)
{

    SetSoundFile(SOUND_HUMAN_DANCE, NextToken(&sz));

}

extern void
CommandSetSoundSoundHumanWinGame(char *sz)
{

    SetSoundFile(SOUND_HUMAN_WIN_GAME, NextToken(&sz));

}

extern void
CommandSetSoundSoundHumanWinMatch(char *sz)
{

    SetSoundFile(SOUND_HUMAN_WIN_MATCH, NextToken(&sz));

}

extern void
CommandSetSoundSoundMove(char *sz)
{

    SetSoundFile(SOUND_MOVE, NextToken(&sz));

}

extern void
CommandSetSoundSoundRedouble(char *sz)
{

    SetSoundFile(SOUND_REDOUBLE, NextToken(&sz));

}

extern void
CommandSetSoundSoundResign(char *sz)
{

    SetSoundFile(SOUND_RESIGN, NextToken(&sz));

}

extern void
CommandSetSoundSoundRoll(char *sz)
{

    SetSoundFile(SOUND_ROLL, NextToken(&sz));

}

extern void
CommandSetSoundSoundStart(char *sz)
{

    SetSoundFile(SOUND_START, NextToken(&sz));

}

extern void
CommandSetSoundSoundTake(char *sz)
{

    SetSoundFile(SOUND_TAKE, NextToken(&sz));

}


static void
SetPriority(int n)
{

#if HAVE_SETPRIORITY
    if (setpriority(PRIO_PROCESS, getpid(), n))
        outputerr("setpriority");
    else {
        outputf(_("Scheduling priority set to %d.\n"), n);
        nThreadPriority = n;
    }
#elif WIN32
    /* tp - thread priority, pp - process priority */
    int tp = THREAD_PRIORITY_NORMAL;
    int pp = NORMAL_PRIORITY_CLASS;
    char *pch;

    if (n < -19) {
        tp = THREAD_PRIORITY_TIME_CRITICAL;
        pch = N_("time critical");
    } else if (n < -10) {
        tp = THREAD_PRIORITY_HIGHEST;
        pch = N_("highest");
    } else if (n < 0) {
        tp = THREAD_PRIORITY_ABOVE_NORMAL;
        pch = N_("above normal");
    } else if (!n) {
        pch = N_("normal");
    } else if (n < 19) {
        tp = THREAD_PRIORITY_BELOW_NORMAL;
        pch = N_("below normal");
    } else {
        /* Lowest - set to idle prioirty but raise the thread priority
         * to make sure it runs instead of screen savers */
        tp = THREAD_PRIORITY_HIGHEST;
        pp = IDLE_PRIORITY_CLASS;
        pch = N_("idle");
    }

    if (SetThreadPriority(GetCurrentThread(), tp)
        && SetPriorityClass(GetCurrentProcess(), pp)) {
        outputf(_("Priority of program set to: %s\n"), pch);
        nThreadPriority = n;
    } else
        outputerrf(_("Changing priority failed (trying to set priority " "%s)\n"), pch);
#else
    outputerrf(_("Priority changes are not supported on this platform.\n"));
#endif                          /* HAVE_SETPRIORITY */
}

extern void
CommandSetPriorityAboveNormal(char *UNUSED(sz))
{

    SetPriority(-10);
}

extern void
CommandSetPriorityBelowNormal(char *UNUSED(sz))
{

    SetPriority(10);
}

extern void
CommandSetPriorityHighest(char *UNUSED(sz))
{

    SetPriority(-19);
}

extern void
CommandSetPriorityIdle(char *UNUSED(sz))
{

    SetPriority(19);
}

extern void
CommandSetPriorityNice(char *sz)
{

    int n;

    if ((n = ParseNumber(&sz)) < -20 || n > 20) {
        outputl(_("You must specify a priority between -20 and 20."));
        return;
    }

    SetPriority(n);
}

extern void
CommandSetPriorityNormal(char *UNUSED(sz))
{

    SetPriority(0);
}

extern void
CommandSetPriorityTimeCritical(char *UNUSED(sz))
{

    SetPriority(-20);
}


extern void
CommandSetCheatEnable(char *sz)
{

    SetToggle("cheat enable", &fCheat, sz,
              _("Allow GNU Backgammon to manipulate the dice."), _("Disallow GNU Backgammon to manipulate the dice."));

}


extern void
CommandSetCheatPlayer(char *sz)
{

    char *pch = NextToken(&sz), *pchCopy;
    int i;

    if (!pch) {
        outputl(_("You must specify a player " "(see `help set cheat player')."));
        return;
    }

    if ((i = ParsePlayer(pch)) == 0 || i == 1) {
        iPlayerSet = i;

        HandleCommand(sz, acSetCheatPlayer);

        return;
    }

    if (i == 2) {
        if ((pchCopy = malloc(strlen(sz) + 1)) == 0) {
            outputl(_("Insufficient memory."));

            return;
        }

        strcpy(pchCopy, sz);

        outputpostpone();

        iPlayerSet = 0;
        HandleCommand(sz, acSetCheatPlayer);

        iPlayerSet = 1;
        HandleCommand(pchCopy, acSetCheatPlayer);

        outputresume();

        free(pchCopy);

        return;
    }

    outputf(_("Unknown player `%s'\n" "(see `help set %s player').\n"), pch, szSetCommand);

}

extern void
PrintCheatRoll(const int fPlayer, const int n)
{

    static const char *aszNumber[21] = {
        N_("best"), N_("second best"), N_("third best"),
        N_("4th best"), N_("5th best"), N_("6th best"),
        N_("7th best"), N_("8th best"), N_("9th best"),
        N_("10th best"),
        N_("median"),
        N_("10th worst"),
        N_("9th worst"), N_("8th worst"), N_("7th worst"),
        N_("6th worst"), N_("5th worst"), N_("4th worst"),
        N_("third worst"), N_("second worst"), N_("worst")
    };

    outputf(_("%s will get the %s roll on each turn.\n"), ap[fPlayer].szName, gettext(aszNumber[n]));

}

extern void
CommandSetCheatPlayerRoll(char *sz)
{

    int n;
    if ((n = ParseNumber(&sz)) < 1 || n > 21) {
        outputl(_("You must specify a size between 1 and 21."));
        return;
    }

    afCheatRoll[iPlayerSet] = n - 1;

    PrintCheatRoll(iPlayerSet, afCheatRoll[iPlayerSet]);

}

extern void
CommandSetExportHtmlSize(char *sz)
{

    int n;

    if ((n = ParseNumber(&sz)) < 1 || n > 20) {
        outputl(_("You must specify a size between 1 and 20."));
        return;
    }

    exsExport.nHtmlSize = n;

    outputf(_("Size of generated HTML images is %dx%d pixels\n"), n * BOARD_WIDTH, n * BOARD_HEIGHT);


}

extern void
CommandSetExportPNGSize(char *sz)
{

    int n;

    if ((n = ParseNumber(&sz)) < 1 || n > 20) {
        outputl(_("You must specify a size between 1 and 20."));
        return;
    }

    exsExport.nPNGSize = n;

    outputf(_("Size of generated PNG images are %dx%d pixels\n"), n * BOARD_WIDTH, n * BOARD_HEIGHT);


}

static void
SetVariation(const bgvariation bgvx)
{

    bgvDefault = bgvx;
    CommandShowVariation(NULL);

    if (ms.gs != GAME_NONE)
        outputf(_("The current match or session is being played as `%s'.\n"), gettext(aszVariations[ms.bgv]));

    outputf(_("Please start a new match or session to play `%s'\n"), gettext(aszVariations[bgvDefault]));

#if USE_GTK
    if (fX && ms.gs == GAME_NONE)
        ShowBoard();
#endif                          /* USE_GTK */

}

extern void
CommandSetVariation1ChequerHypergammon(char *UNUSED(sz))
{

    SetVariation(VARIATION_HYPERGAMMON_1);

}

extern void
CommandSetVariation2ChequerHypergammon(char *UNUSED(sz))
{

    SetVariation(VARIATION_HYPERGAMMON_2);

}

extern void
CommandSetVariation3ChequerHypergammon(char *UNUSED(sz))
{

    SetVariation(VARIATION_HYPERGAMMON_3);

}

extern void
CommandSetVariationNackgammon(char *UNUSED(sz))
{

    SetVariation(VARIATION_NACKGAMMON);

}

extern void
CommandSetVariationStandard(char *UNUSED(sz))
{

    SetVariation(VARIATION_STANDARD);

}


extern void
CommandSetGotoFirstGame(char *sz)
{

    SetToggle("gotofirstgame", &fGotoFirstGame, sz,
              _("Goto first game when loading matches or sessions."),
              _("Goto last game when loading matches or sessions."));

}


static void
SetEfficiency(const char *szText, char *sz, float *prX)
{

    float r = ParseReal(&sz);

    if (r >= 0.0f && r <= 1.0f) {
        *prX = r;
        outputf("%s: %7.5f\n", szText, *prX);
    } else
        outputl(_("Cube efficiency must be between 0 and 1"));

}

extern void
CommandSetCubeEfficiencyOS(char *sz)
{

    SetEfficiency(_("Cube efficiency for one sided bearoff positions"), sz, &rOSCubeX);

}

extern void
CommandSetCubeEfficiencyCrashed(char *sz)
{

    SetEfficiency(_("Cube efficiency for crashed positions"), sz, &rCrashedX);

}

extern void
CommandSetCubeEfficiencyContact(char *sz)
{

    SetEfficiency(_("Cube efficiency for contact positions"), sz, &rContactX);

}

extern void
CommandSetCubeEfficiencyRaceFactor(char *sz)
{

    float r = ParseReal(&sz);

    if (r >= 0) {
        rRaceFactorX = r;
        outputf(_("Cube efficiency race factor set to %7.5f\n"), rRaceFactorX);
    } else
        outputl(_("Cube efficiency race factor must be larger than 0."));

}

extern void
CommandSetCubeEfficiencyRaceMax(char *sz)
{

    SetEfficiency(_("Cube efficiency race max"), sz, &rRaceMax);

}

extern void
CommandSetCubeEfficiencyRaceMin(char *sz)
{

    SetEfficiency(_("Cube efficiency race min"), sz, &rRaceMin);

}

extern void
CommandSetCubeEfficiencyRaceCoefficient(char *sz)
{

    float r = ParseReal(&sz);

    if (r >= 0) {
        rRaceCoefficientX = r;
        outputf(_("Cube efficiency race coefficient set to %7.5f\n"), rRaceCoefficientX);
    } else
        outputl(_("Cube efficiency race coefficient must be larger than 0."));

}


extern void
CommandSetRatingOffset(char *sz)
{

    float r = ParseReal(&sz);

    if (r < 0) {
        outputl(_("Please provide a positive rating offset\n"));
        return;
    }

    rRatingOffset = r;

    outputf(_("The rating offset for estimating absolute ratings is: %.1f\n"), rRatingOffset);

}

extern void
CommandSetLang(char *sz)
{
    char *result;
    g_free(szLang);
    szLang = (sz && *sz) ? g_strdup(sz) : g_strdup("system");
    result = SetupLanguage(szLang);

    if (result) {
#if USE_GTK
        if (fX)
            GtkChangeLanguage();
        else
#endif
            outputf(_("Locale is now '%s'"), result);
    } else
        outputerrf(_("Locale '%s' not supported by C library.\n"), sz);
}

extern void
CommandSetPanelWidth(char *sz)
{
    int n = ParseNumber(&sz);

    if (n < 50) {
        outputl(_("You must specify a number greater than 50"));
        return;
    }
#if USE_GTK
    if (fX)
        SetPanelWidth(n);
#endif
}

extern void
CommandSetOutputErrorRateFactor(char *sz)
{

    float r = ParseReal(&sz);

    if (r < 0) {
        outputl(_("Please provide a positive number\n"));
        return;
    }

    rErrorRateFactor = r;

    outputf(_("The factor used for multiplying error rates is: %.1f\n"), rErrorRateFactor);



}

static void
SetFolder(char **folder, char *sz)
{
    g_free(*folder);
    if (sz && *sz)
        *folder = g_strdup(sz);
    else
        *folder = NULL;
}

extern void
CommandSetExportFolder(char *sz)
{
    SetFolder(&default_export_folder, NextToken(&sz));
}

extern void
CommandSetImportFolder(char *sz)
{
    SetFolder(&default_import_folder, NextToken(&sz));
}

extern void
CommandSetSGFFolder(char *sz)
{
    SetFolder(&default_sgf_folder, NextToken(&sz));
}

static int
SetXGID(char *sz)
{
    int nMatchTo = -1;
    int nRules = -1;
    int fCrawford = 0;
    int anScore[2];
    int fMove = -1;
    int fTurn;
    unsigned int anDice[2] = { 0, 0 };
    int fCubeOwner = -1;
    int nCube = -1;
    int fDoubled = 0;
    int fJacoby = 0;
    matchstate msxg;
    TanBoard anBoard;
    char *posid, *matchid;
    char *pos;

    char *s = g_strdup(sz);

    char *c;
    int i;
    char v[9][5];
    int fSidesSwapped = FALSE;

    for (i = 0; i < 9 && (c = strrchr(s, ':')); i++) {
        strncpy(v[i], c + 1, 4);
        v[i][4] = '\0';
        *c = '\0';
    }

    c = strrchr(s, '=');
    pos = c ? c + 1 : s;

    if (strlen(pos) != 26) {
        g_free(s);
        return 1;
    }

    if (PositionFromXG(anBoard, pos)) {
        g_free(s);
        return 1;
    } else
        g_free(s);

    /* atoi(v[0]) is a maximum (money) cube value, unused in gnubg */

    nMatchTo = atoi(v[1]);

    nRules = atoi(v[2]);

    if (nMatchTo > 0) {
        switch (nRules) {
        case 0:
            fCrawford = 0;
            break;
        case 1:
            fCrawford = 1;
            break;
        default:
            return (1);
        }
    } else {
        switch (nRules) {
        case 0:
            fJacoby = 0;
            nBeavers = 0;
            break;
        case 1:
            fJacoby = 1;
            nBeavers = 0;
            break;
        case 2:
            fJacoby = 0;
            nBeavers = 3;
            break;
        case 3:
            fJacoby = 1;
            nBeavers = 3;
            break;
        default:
            return 1;
        }
    }

    anScore[0] = atoi(v[3]);
    anScore[1] = atoi(v[4]);

    fMove = atoi(v[6]) == 1 ? 1 : 0;

    switch (v[5][0]) {
    case 'D':
        fTurn = !fMove;
        fDoubled = 1;
        anDice[0] = anDice[1] = 0;
        break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
        if (strlen(v[5]) != 2) {
            return 1;
        }
        fTurn = fMove;
        anDice[1] = atoi(v[5] + 1);
        v[5][1] = '\0';
        anDice[0] = atoi(v[5]);
        break;
    default:
        return 1;
    }

    nCube = 1 << atoi(v[8]);

    switch (atoi(v[7])) {
    case 1:
        fCubeOwner = 1;
        break;
    case 0:
        fCubeOwner = -1;
        break;
    case -1:
        fCubeOwner = 0;
    }

    msxg.anDice[0] = anDice[0];
    msxg.anDice[1] = anDice[1];
    msxg.fResigned = 0;
    msxg.fResignationDeclined = 0;
    msxg.fDoubled = fDoubled;
    msxg.cGames = 0;
    msxg.fMove = fMove;
    msxg.fTurn = fTurn;
    msxg.fCubeOwner = fCubeOwner;
    msxg.fCrawford = fCrawford;
    msxg.fPostCrawford = !fCrawford && (anScore[0] == nMatchTo - 1 || anScore[1] == nMatchTo - 1);
    msxg.nMatchTo = nMatchTo;
    msxg.anScore[0] = anScore[0];
    msxg.anScore[1] = anScore[1];
    msxg.nCube = nCube;
    msxg.bgv = bgvDefault;
    msxg.fCubeUse = fCubeUse;
    msxg.fJacoby = fJacoby;
    msxg.gs = GAME_PLAYING;

    matchid = g_strdup(MatchIDFromMatchState(&msxg));
    CommandSetMatchID(matchid);
    g_free(matchid);

    if (!fMove) {
        SwapSides(anBoard);
        fSidesSwapped = TRUE;
    }
    posid = g_strdup(PositionID((ConstTanBoard) anBoard));
    CommandSetBoard(posid);
    g_free(posid);

    if ((anDice[0] == 0 && fSidesSwapped) || (anDice[0] && !fMove)) {

        if (GetInputYN
            (_
             ("This position has player on roll appearing on top. \nSwap players so the player on roll appears on the bottom? ")))
            CommandSwapPlayers(NULL);
    }
    return 0;
}

static char *
get_base64(char *inp, char **next)
{
    char *first, *last;
    int l = 0;

    *next = NULL;
    g_return_val_if_fail(inp, NULL);
    g_return_val_if_fail(*inp, NULL);

    for (first = inp; *first; first++) {
        if (Base64(*first) != 255)
            break;
    }

    if (!*first) {
        *next = first;
        return NULL;
    }

    for (last = first; *last; last++, l++) {
        if (Base64(*last) == 255)
            break;
    }
    *next = last;
    return g_strndup(first, l);
}

extern void
CommandSetXGID(char *sz)
{
    if (SetXGID(sz))
        outputerrf(_("Not a valid XGID '%s'"), sz);
}

extern void
CommandSetGNUBgID(char *sz)
{
    char *out;
    char *posid = NULL;
    char *matchid = NULL;

    if (SetXGID(sz) == 0)
        return;

    while (sz && *sz) {
        out = get_base64(sz, &sz);
        if (out) {
            if (strlen(out) == L_MATCHID) {
                if (matchid)
                    continue;
                matchid = g_strdup(out);
            } else if (strlen(out) == L_POSITIONID) {
                if (posid)
                    continue;
                posid = g_strdup(out);
            }
            g_free(out);
        }
        if (posid && matchid)
            break;
    }
    if (!posid && !matchid) {
        outputerrf(_("No valid IDs found"));
        return;
    }
    if (matchid)
        CommandSetMatchID(matchid);
    if (posid)
        CommandSetBoard(posid);
    outputf(_("Setting GNUbg ID %s:%s\n"), posid ? posid : "", matchid ? matchid : "");
    g_free(posid);
    g_free(matchid);
}

extern void
CommandSetAutoSaveRollout(char *sz)
{
    SetToggle("autosave rollout", &fAutoSaveRollout, sz,
              _("Auto save during rollouts"), _("Don't auto save during rollouts"));
}

extern void
CommandSetAutoSaveAnalysis(char *sz)
{
    SetToggle("autosave analysis", &fAutoSaveAnalysis, sz,
              _("Auto save after each analysed game"), _("Don't auto save after each analysed game"));
}

extern void
CommandSetAutoSaveConfirmDelete(char *sz)
{
    SetToggle("autosave confirm", &fAutoSaveConfirmDelete, sz,
              _("Prompt before deleting autosaves"), _("Delete autosaves automatically"));
}

extern void
CommandSetAutoSaveTime(char *sz)
{
    int n = ParseNumber(&sz);

    if (n < 1) {
        outputl(_("You must specify a positive autosave time in minutes"));
        return;
    }
    nAutoSaveTime = n;
}
