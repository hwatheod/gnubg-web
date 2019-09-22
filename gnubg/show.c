/*
 * show.c
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
 * $Id: show.c,v 1.284 2015/02/09 21:55:33 plm Exp $
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <glib.h>
#include <ctype.h>
#include <math.h>

#include "backgammon.h"
#include "drawboard.h"
#include "eval.h"
#include "export.h"
#include "format.h"
#include "dice.h"
#include "matchequity.h"
#include "matchid.h"
#include "sound.h"
#include "osr.h"
#include "positionid.h"
#include "boarddim.h"
#include "credits.h"
#include "util.h"
#ifndef WEB
#include "openurl.h"
#endif /* WEB */
#include "multithread.h"

#if USE_GTK
#include "gtkboard.h"
#include "gtkgame.h"
#include "gtktheory.h"
#include "gtkrace.h"
#include "gtkexport.h"
#include "gtkmet.h"
#include "gtkrolls.h"
#include "gtktempmap.h"
#include "gtkoptions.h"
#endif

#ifdef WIN32
#include <io.h>
#endif

static void
ShowMoveFilter(const movefilter * pmf, const int ply)
{

    if (pmf->Accept < 0) {
        outputf(_("Skip pruning for %d-ply moves.\n"), ply);
        return;
    }

    if (pmf->Accept == 1)
        outputf(_("keep the best %d-ply move"), ply);
    else
        outputf(_("keep the first %d %d-ply moves"), pmf->Accept, ply);

    if (pmf->Extra == 0) {
        outputf("\n");
        return;
    }

    outputf(_(" and up to %d more moves within equity %0.3g\n"), pmf->Extra, pmf->Threshold);
}


static void
ShowMoveFilters(movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES])
{

    int i, j;

    for (i = 0; i < MAX_FILTER_PLIES; ++i) {

        outputf("      Move filter for %d ply:\n", i + 1);

        for (j = 0; j <= i; ++j) {
            outputf("        ");
            ShowMoveFilter(&aamf[i][j], j);
        }
    }

    outputl("");

}

static void
ShowEvaluation(const evalcontext * pec)
{

    outputf(_("        %d-ply evaluation.\n"
              "        %s"
              "        %s evaluations.\n"),
            pec->nPlies,
            (pec->fUsePrune) ? _("Using pruning neural nets.") :
            _("Not using pruning neural nets."), pec->fCubeful ? _("Cubeful") : _("Cubeless"));

    if (pec->rNoise > 0.0f) {
        outputf("%s%s %5.3f", ("        "), _("Noise standard deviation"), pec->rNoise);
        outputl(pec->fDeterministic ? _(" (deterministic noise).\n") : _(" (pseudo-random noise).\n"));
    } else
        outputf("%s%s", "        ", _("Noiseless evaluations.\n"));
}

extern int
EvalCmp(const evalcontext * E1, const evalcontext * E2, const int nElements)
{

    int i, cmp = 0;

    if (nElements < 1)
        return 0;

    for (i = 0; i < nElements; ++i, ++E1, ++E2) {
        cmp = memcmp((const void *) E1, (const void *) E2, sizeof(evalcontext));
        if (cmp != 0)
            break;
    }

    return cmp;
}

/* all the sundry displays of evaluations. Deals with identical/different
 * player settings, displays late evaluations where needed */

static void
show_evals(const char *text,
           const evalcontext * early, const evalcontext * late,
           const int fPlayersAreSame, const int fLateEvals, const int nLate)
{

    int i;

    if (fLateEvals)
        outputf(_("%s for first %d plies:\n"), text, nLate);
    else
        outputf("%s\n", text);

    if (fPlayersAreSame)
        ShowEvaluation(early);
    else {
        for (i = 0; i < 2; i++) {
            outputf(_("Player %d:\n"), i);
            ShowEvaluation(early + i);
        }
    }

    if (!fLateEvals)
        return;

    outputf(_("%s after %d plies:\n"), text, nLate);
    if (fPlayersAreSame)
        ShowEvaluation(late);
    else {
        for (i = 0; i < 2; i++) {
            outputf(_("Player %d:\n"), i);
            ShowEvaluation(late + i);
        }
    }
}


static void
show_movefilters(movefilter aaamf[2][MAX_FILTER_PLIES][MAX_FILTER_PLIES])
{

    if (equal_movefilters(aaamf[0], aaamf[1]))
        ShowMoveFilters(aaamf[0]);
    else {
        int i;
        for (i = 0; i < 2; i++) {
            outputf(_("Player %d:\n"), i);
            ShowMoveFilters(aaamf[i]);
        }
    }
}

static void
ShowRollout(rolloutcontext * prc)
{

    int fDoTruncate = 0;
    int fLateEvals = 0;
    int nTruncate = prc->nTruncate;
    int nLate = prc->nLate;
    int fPlayersAreSame = 1;
    int fCubeEqualChequer = 1;

    if (prc->fDoTruncate && (nTruncate > 0))
        fDoTruncate = 1;

    if (prc->fLateEvals && (!fDoTruncate || ((nTruncate > nLate) && (nLate > 2))))
        fLateEvals = 1;

    outputf(ngettext("%d game will be played per rollout.\n",
                     "%d games will be played per rollout.\n", prc->nTrials), prc->nTrials);

    if (!fDoTruncate || nTruncate < 1)
        outputl(_("No truncation."));
    else
        outputf(ngettext("Truncation after %d ply.\n", "Truncation after %d plies.\n", nTruncate), nTruncate);

    outputl(prc->fTruncBearoff2 ?
            _("Will truncate cubeful money game rollouts when reaching "
              "exact bearoff database.") :
            _("Will not truncate cubeful money game rollouts when reaching " "exact bearoff database."));

    outputl(prc->fTruncBearoff2 ?
            _("Will truncate money game rollouts when reaching "
              "exact bearoff database.") :
            _("Will not truncate money game rollouts when reaching " "exact bearoff database."));

    outputl(prc->fTruncBearoffOS ?
            _("Will truncate *cubeless* rollouts when reaching "
              "one-sided bearoff database.") :
            _("Will not truncate *cubeless* rollouts when reaching " "one-sided bearoff database."));

    outputl(prc->fVarRedn ?
            _("Lookahead variance reduction is enabled.") : _("Lookahead variance reduction is disabled."));
    outputl(prc->fRotate ? _("Quasi-random dice are enabled.") : _("Quasi-random dice are disabled."));
    outputl(prc->fCubeful ? _("Cubeful rollout.") : _("Cubeless rollout."));
    outputl(prc->fInitial ? _("Rollout as opening move enabled.") : _("Rollout as opening move disabled."));
    outputf(_("%s dice generator with seed %lu.\n"), gettext(aszRNG[prc->rngRollout]), prc->nSeed);

    /* see if the players settings are the same */
    if (EvalCmp(prc->aecChequer, prc->aecChequer + 1, 1) ||
        EvalCmp(prc->aecCube, prc->aecCube + 1, 1) ||
        (fLateEvals &&
         (EvalCmp(prc->aecChequerLate, prc->aecChequerLate + 1, 1) ||
          EvalCmp(prc->aecCubeLate, prc->aecCubeLate + 1, 1))))
        fPlayersAreSame = 0;

    /* see if the cube and chequer evals are the same */
    if (EvalCmp(prc->aecChequer, prc->aecCube, 2) ||
        (fLateEvals && (EvalCmp(prc->aecChequerLate, prc->aecCubeLate, 2))))
        fCubeEqualChequer = 0;

    if (fCubeEqualChequer) {
        /* simple summary - show_evals will deal with player differences */
        show_evals(_("Evaluation parameters:"), prc->aecChequer,
                   prc->aecChequerLate, fPlayersAreSame, fLateEvals, nLate);
    } else {
        /* Cube different from Chequer */
        show_evals(_("Chequer play parameters:"), prc->aecChequer,
                   prc->aecChequerLate, fPlayersAreSame, fLateEvals, nLate);
        show_evals(_("Cube decision parameters:"), prc->aecCube, prc->aecCubeLate, fPlayersAreSame, fLateEvals, nLate);
    }

    if (fLateEvals) {
        outputf(_("Move filter for first %d plies:\n"), nLate);
        show_movefilters(prc->aaamfChequer);
        outputf(_("Move filter after %d plies:\n"), nLate);
        show_movefilters(prc->aaamfLate);
    } else {
        outputf(_("Move filter:\n"));
        show_movefilters(prc->aaamfChequer);
    }

    if (fDoTruncate) {
        show_evals(_("Truncation point Chequer play evaluation:"), &prc->aecChequerTrunc, 0, 1, 0, 0);
        show_evals(_("Truncation point Cube evaluation:"), &prc->aecCubeTrunc, 0, 1, 0, 0);
    }

    if (prc->fStopOnSTD) {
        if (prc->fCubeful)
            outputf(_("Rollouts may stop after %d games if both ratios |equity/STD|\n"
                      "\t(cubeful and cubeless) are less than %5.4f\n"), prc->nMinimumGames, prc->rStdLimit);
        else
            outputf(_("Rollouts may stop after %d games if the ratio |equity/STD|"
                      " is less than %5.4f\n"), prc->nMinimumGames, prc->rStdLimit);
    }
}

static void
ShowEvalSetup(evalsetup * pes)
{

    switch (pes->et) {

    case EVAL_NONE:
        outputl(_("      No evaluation."));
        break;
    case EVAL_EVAL:
        outputl(_("      Neural net evaluation:"));
        ShowEvaluation(&pes->ec);
        break;
    case EVAL_ROLLOUT:
        outputl(_("      Rollout:"));
        ShowRollout(&pes->rc);
        break;
    default:
        g_assert_not_reached();

    }

}


static void
ShowPaged(char **ppch)
{

    int i, nRows = 0;
    char *pchLines;
#ifdef TIOCGWINSZ
    struct winsize ws;
#endif

    if (isatty(STDIN_FILENO)) {
#ifdef TIOCGWINSZ
        if (!(ioctl(STDIN_FILENO, TIOCGWINSZ, &ws)))
            nRows = ws.ws_row;
#endif
#ifdef WEB
	nRows = 999999;  // the web interface is writing to an unlimited scrolling textarea
#else /* WEB */
        if (!nRows && ((pchLines = getenv("LINES")) != NULL))
            nRows = atoi(pchLines);
        /* FIXME we could try termcap-style tgetnum( "li" ) here, but it
         * hardly seems worth it */

        if (!nRows)
            nRows = 24;
#endif /* WEB */

        i = 0;

        while (*ppch) {
            outputl(*ppch++);
            if (++i >= nRows - 1) {
                GetInput(_("-- Press <return> to continue --"));

                if (fInterrupt)
                    return;

                i = 0;
            }
        }
    } else
        while (*ppch)
            outputl(*ppch++);
}

extern void
CommandShowAnalysis(char *UNUSED(sz))
{

    int i;

    outputl(fAnalyseCube ? _("Cube action will be analysed.") : _("Cube action will not be analysed."));

    outputl(fAnalyseDice ? _("Dice rolls will be analysed.") : _("Dice rolls will not be analysed."));

    if (fAnalyseMove) {
        outputl(_("Chequer play will be analysed."));
    } else
        outputl(_("Chequer play will not be analysed."));

    outputl("");
    for (i = 0; i < 2; ++i)
        outputf(_("Analyse %s's chequerplay and cube decisions: %s\n"),
                ap[i].szName, afAnalysePlayers[i] ? _("yes") : _("no"));

    outputl(_("\nAnalysis thresholds:"));
    outputf(                    /*"  +%.3f %s\n"
                                 * "  +%.3f %s\n" */
               "  -%.3f %s\n"
               "  -%.3f %s\n"
               "  -%.3f %s\n"
               "\n"
               "  +%.3f %s\n"
               "  +%.3f %s\n"
               "  -%.3f %s\n"
               "  -%.3f %s\n",
               arSkillLevel[SKILL_DOUBTFUL],
               gettext(aszSkillType[SKILL_DOUBTFUL]),
               arSkillLevel[SKILL_BAD],
               gettext(aszSkillType[SKILL_BAD]),
               arSkillLevel[SKILL_VERYBAD],
               gettext(aszSkillType[SKILL_VERYBAD]),
               arLuckLevel[LUCK_VERYGOOD],
               gettext(aszLuckType[LUCK_VERYGOOD]),
               arLuckLevel[LUCK_GOOD],
               gettext(aszLuckType[LUCK_GOOD]),
               arLuckLevel[LUCK_BAD],
               gettext(aszLuckType[LUCK_BAD]), arLuckLevel[LUCK_VERYBAD], gettext(aszLuckType[LUCK_VERYBAD]));

    outputl(_("\n" "Analysis will be performed with the " "following evaluation parameters:"));
    outputl(_("    Chequer play:"));
    ShowEvalSetup(&esAnalysisChequer);
    ShowMoveFilters(aamfAnalysis);
    outputl(_("    Cube decisions:"));
    ShowEvalSetup(&esAnalysisCube);

    outputl(_("    Luck analysis:"));
    ShowEvaluation(&ecLuck);

}

extern void
CommandShowAutomatic(char *UNUSED(sz))
{

    static const char *szOn = N_("On"), *szOff = N_("Off");

    outputf(_("bearoff \t(Play certain non-contact bearoff moves):      \t%s\n"
              "crawford\t(Enable the Crawford rule as appropriate):     \t%s\n"
              "doubles \t(Turn the cube when opening roll is a double): \t%d\n"
              "game    \t(Start a new game after each one is completed):\t%s\n"
              "move    \t(Play the forced move when there is no choice):\t%s\n"
              "roll    \t(Roll the dice if no double is possible):      \t%s\n"),
            fAutoBearoff ? gettext(szOn) : gettext(szOff),
            fAutoCrawford ? gettext(szOn) : gettext(szOff),
            cAutoDoubles,
            fAutoGame ? gettext(szOn) : gettext(szOff),
            fAutoMove ? gettext(szOn) : gettext(szOff), fAutoRoll ? gettext(szOn) : gettext(szOff));
}

extern void
CommandShowBoard(char *sz)
{

    TanBoard an;
    char szOut[2048];
    char *ap[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

    if (!*sz) {
        if (ms.gs == GAME_NONE)
            outputl(_("No position specified and no game in progress."));
        else
            ShowBoard();

        return;
    }

    /* FIXME handle =n notation */
    if (ParsePosition(an, &sz, NULL) < 0)
        return;

#if USE_GTK
    if (fX)
        game_set(BOARD(pwBoard), an, TRUE, "", "", 0, 0, 0, 0, 0, FALSE, anChequers[ms.bgv]);
    else
#endif
        outputl(DrawBoard(szOut, (ConstTanBoard) an, TRUE, ap, MatchIDFromMatchState(&ms), anChequers[ms.bgv]));
}

extern
    void
CommandShowFullBoard(char *sz)
{

    TanBoard an;
    char szOut[2048];
    char *apch[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

    if (!*sz) {
        if (ms.gs == GAME_NONE)
            outputl(_("No position specified and no game in progress."));
        else
            ShowBoard();

        return;
    }

    /* FIXME handle =n notation */
    if (ParsePosition(an, &sz, NULL) < 0)
        return;

#if USE_GTK
    if (fX)
        game_set(BOARD(pwBoard), an, ms.fTurn,
                 ap[1].szName, ap[0].szName, ms.nMatchTo,
                 ms.anScore[1], ms.anScore[0], ms.anDice[0], ms.anDice[1], FALSE, anChequers[ms.bgv]);
    else
#endif
        outputl(DrawBoard(szOut, (ConstTanBoard) an, TRUE, apch, MatchIDFromMatchState(&ms), anChequers[ms.bgv]));
}


extern void
CommandShowDelay(char *UNUSED(sz))
{
#if USE_GTK
    if (nDelay)
        outputf(_("The delay is set to %d ms.\n"), nDelay);
    else
        outputl(_("No delay is being used."));
#else
    outputl(_("The `show delay' command applies only when using the GUI"));
#endif
}

extern void
CommandShowAliases(char *UNUSED(sz))
{
    outputf(_("Aliases for player 1 when importing MAT files is set to \"%s\".\n "), player1aliases);
}

extern void
CommandShowCache(char *UNUSED(sz))
{
    unsigned int c[2], cHit[2], cLookup[2];

    EvalCacheStats(c, cLookup, cHit);

    outputf("%10u regular eval entries used %10u lookups %10u hits", c[0], cLookup[0], cHit[0]);

    if (cLookup[0])
        outputf(" (%4.1f%%).", (float) cHit[0] * 100.0f / (float) cLookup[0]);
    else
        outputc('.');

    outputc('\n');

    outputf("%10u pruning eval entries used %10u lookups %10u hits", c[1], cLookup[1], cHit[1]);

    if (cLookup[1])
        outputf(" (%4.1f%%).", (float) cHit[1] * 100.0f / (float) cLookup[1]);
    else
        outputc('.');

    outputc('\n');
}

extern void
CommandShowCalibration(char *UNUSED(sz))
{

#if USE_GTK
    if (fX) {
        GTKShowCalibration();
        return;
    }
#endif

    if (rEvalsPerSec > 0)
        outputf(_("Evaluation speed has been set to %.0f evaluations per " "second.\n"), rEvalsPerSec);
    else
        outputl(_("No evaluation speed has been recorded."));
}

extern void
CommandShowClockwise(char *UNUSED(sz))
{

    if (fClockwise)
        outputl(_("Player 1 moves clockwise (and player 0 moves " "anticlockwise)."));
    else
        outputl(_("Player 1 moves anticlockwise (and player 0 moves " "clockwise)."));
}

static void
ShowCommands(command * pc, const char *szPrefix)
{

    char sz[128], *pch;

    strcpy(sz, szPrefix);
    pch = strchr(sz, 0);

    for (; pc->sz; pc++) {
        if (!pc->szHelp)
            continue;

        strcpy(pch, pc->sz);

        if (pc->pc && pc->pc->pc != pc->pc) {
            strcat(pch, " ");
            ShowCommands(pc->pc, sz);
        } else
            outputl(sz);
    }
}

extern void
CommandShowCommands(char *UNUSED(sz))
{

    ShowCommands(acTop, "");
}

extern void
CommandShowConfirm(char *UNUSED(sz))
{

    if (nConfirmDefault == -1)
        outputl(_("GNU Backgammon will ask for confirmation."));
    else if (nConfirmDefault == 1)
        outputl(_("GNU Backgammon will answer yes to questions."));
    else
        outputl(_("GNU Backgammon will answer no to questions."));

    if (fConfirmNew)
        outputl(_("GNU Backgammon will ask for confirmation before " "aborting games in progress."));
    else
        outputl(_("GNU Backgammon will not ask for confirmation " "before aborting games in progress."));

    if (fConfirmSave)
        outputl(_("GNU Backgammon will ask for confirmation before " "overwriting existing files."));
    else
        outputl(_("GNU Backgammon will not ask for confirmation " "overwriting existing files."));

}

extern void
CommandShowCopying(char *UNUSED(sz))
{

#if USE_GTK
    if (fX)
        ShowList(aszCopying, _("Copying"), NULL);
    else
#endif
        ShowPaged(aszCopying);
}

extern void
CommandShowCrawford(char *UNUSED(sz))
{

    if (ms.nMatchTo > 0)
        outputl(ms.fCrawford ? _("This game is the Crawford game.") : _("This game is not the Crawford game"));
    else if (!ms.nMatchTo)
        outputl(_("Crawford rule is not used in money sessions."));
    else
        outputl(_("No match is being played."));

}

extern void
CommandShowCube(char *UNUSED(sz))
{

    if (ms.gs != GAME_PLAYING) {
        outputl(_("There is no game in progress."));
        return;
    }

    if (ms.fCrawford) {
        outputl(_("The cube is disabled during the Crawford game."));
        return;
    }

    if (!ms.fCubeUse) {
        outputl(_("The doubling cube is disabled."));
        return;
    }

    if (ms.fCubeOwner == -1)
        outputf(_("The cube is at %d, and is centred."), ms.nCube);
    else
        outputf(_("The cube is at %d, and is owned by %s."), ms.nCube, ap[ms.fCubeOwner].szName);
}

extern void
CommandShowDice(char *UNUSED(sz))
{

    if (ms.gs != GAME_PLAYING) {
        outputl(_("The dice will not be rolled until a game is started."));

        return;
    }

    if (ms.anDice[0] < 1)
        outputf(_("%s has not yet rolled the dice.\n"), ap[ms.fMove].szName);
    else
        outputf(_("%s has rolled %d and %d.\n"), ap[ms.fMove].szName, ms.anDice[0], ms.anDice[1]);
}

extern void
CommandShowDisplay(char *UNUSED(sz))
{

    if (fDisplay)
        outputl(_("GNU Backgammon will display boards for computer moves."));
    else
        outputl(_("GNU Backgammon will not display boards for computer moves."));
}

extern void
CommandShowEngine(char *UNUSED(sz))
{

    char szBuffer[4096];

    EvalStatus(szBuffer);

    output(szBuffer);
}

extern void
CommandShowEvaluation(char *UNUSED(sz))
{

    outputl(_("`eval' and `hint' will use:"));
    outputl(_("    Chequer play:"));
    ShowEvalSetup(GetEvalChequer());
    outputl(_("    Move filters:"));
    ShowMoveFilters(*GetEvalMoveFilter());
    outputl(_("    Cube decisions:"));
    ShowEvalSetup(GetEvalCube());

}

extern void
CommandShowJacoby(char *UNUSED(sz))
{

    if (!ms.nMatchTo)
        outputl(ms.fJacoby ? _("This money session is played with the Jacoby rule.")
                : _("This money session is played without the Jacoby rule.")
            );

    if (fJacoby)
        outputl(_("New money sessions are played with the Jacoby rule."));
    else
        outputl(_("New money sessions are played without the Jacoby rule."));

}

extern void
CommandShowLang(char *UNUSED(sz))
{

    if (szLang)
        outputf(_("Your language preference is set to %s.\n"), szLang);
    else
        outputerrf(_("Language not set"));
}

extern void
CommandShowMatchInfo(char *UNUSED(sz))
{

#if USE_GTK
    if (fX) {
        GTKMatchInfo();
        return;
    }
#endif

    outputf(_("%s (%s) vs. %s (%s)"), ap[0].szName,
            mi.pchRating[0] ? mi.pchRating[0] : _("unknown rating"),
            ap[1].szName, mi.pchRating[1] ? mi.pchRating[1] : _("unknown rating"));

    if (mi.nYear)
        outputf(", %04d-%02d-%02d\n", mi.nYear, mi.nMonth, mi.nDay);
    else
        outputc('\n');

    if (mi.pchEvent)
        outputf(_("Event: %s\n"), mi.pchEvent);

    if (mi.pchRound)
        outputf(_("Round: %s\n"), mi.pchRound);

    if (mi.pchPlace)
        outputf(_("Place: %s\n"), mi.pchPlace);

    if (mi.pchAnnotator)
        outputf(_("Annotator: %s\n"), mi.pchAnnotator);

    if (mi.pchComment)
        outputf("\n%s\n", mi.pchComment);
}

extern void
CommandShowMatchLength(char *UNUSED(sz))
{

    outputf(ngettext("New matches default to %d point.\n", "New matches default to %d points.\n", nDefaultLength),
            nDefaultLength);
}

extern void
CommandShowPipCount(char *sz)
{
    TanBoard an;
    unsigned int anPips[2];

    if (!*sz && ms.gs == GAME_NONE) {
        outputl(_("No position specified and no game in progress."));
        return;
    }

    if (ParsePosition(an, &sz, NULL) < 0)
        return;

    PipCount((ConstTanBoard) an, anPips);

    outputf(_("The pip counts are: %s %d, %s %d.\n"), ap[ms.fMove].szName, anPips[1], ap[!ms.fMove].szName, anPips[0]);

#if USE_GTK
    if (fX && fFullScreen) {    /* Display in dialog box in full screen mode (urgh) */
        output(" ");
        outputx();
    }
#endif
}

extern void
CommandShowPlayer(char *UNUSED(sz))
{

    int i;

    for (i = 0; i < 2; i++) {
        outputf(_("Player %d:\n" "  Name: %s\n" "  Type: "), i, ap[i].szName);

        switch (ap[i].pt) {
        case PLAYER_EXTERNAL:
            outputf(_("external: %s\n\n"), ap[i].szSocket);
            break;
        case PLAYER_GNU:
            outputf(_("gnubg:\n"));
            outputl(_("    Checker play:"));
            ShowEvalSetup(&ap[i].esChequer);
            outputl(_("    Move filters:"));
            ShowMoveFilters(ap[i].aamf);
            outputl(_("    Cube decisions:"));
            ShowEvalSetup(&ap[i].esCube);
            break;
        case PLAYER_HUMAN:
            outputl(_("human\n"));
            break;
        }
    }
}

extern void
CommandShowPostCrawford(char *UNUSED(sz))
{

    if (ms.nMatchTo > 0)
        outputl(ms.fPostCrawford ? _("This is post-Crawford play.") : _("This is not post-Crawford play."));
    else if (!ms.nMatchTo)
        outputl(_("Crawford rule is not used in money sessions."));
    else
        outputl(_("No match is being played."));

}

extern void
CommandShowPrompt(char *UNUSED(sz))
{

    outputf(_("The prompt is set to `%s'.\n"), szPrompt);
}

extern void
CommandShowRNG(char *UNUSED(sz))
{

    outputf(_("You are using the %s generator.\n"), gettext(aszRNG[rngCurrent]));

}

extern void
CommandShowRollout(char *UNUSED(sz))
{

    outputl(_("`rollout' will use:"));
    ShowRollout(&rcRollout);

}

extern void
CommandShowScore(char *UNUSED(sz))
{

    outputf((ms.cGames == 1 ? _("The score (after %d game) is: %s %d, %s %d")
             : _("The score (after %d games) is: %s %d, %s %d")),
            ms.cGames, ap[0].szName, ms.anScore[0], ap[1].szName, ms.anScore[1]);

    if (ms.nMatchTo > 0) {
        outputf(ms.nMatchTo == 1 ?
                _(" (match to %d point%s).\n") :
                _(" (match to %d points%s).\n"),
                ms.nMatchTo, ms.fCrawford ? _(", Crawford game") : (ms.fPostCrawford ? _(", post-Crawford play") : ""));
    } else {
        if (ms.fJacoby)
            outputl(_(" (money session,\nwith Jacoby rule)."));
        else
            outputl(_(" (money session,\nwithout Jacoby rule)."));
    }

}

extern void
CommandShowSeed(char *UNUSED(sz))
{

    PrintRNGSeed(rngCurrent, rngctxCurrent);
    PrintRNGCounter(rngCurrent, rngctxCurrent);
}

extern void
CommandShowTurn(char *UNUSED(sz))
{

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game is being played."));

        return;
    }

    if (ms.anDice[0])
        outputf(_("%s in on move.\n"), ap[ms.fMove].szName);
    else
        outputf(_("%s in on roll.\n"), ap[ms.fMove].szName);

    if (ms.fResigned)
        outputf(_("%s has offered to resign a %s.\n"), ap[ms.fMove].szName, gettext(aszGameResult[ms.fResigned - 1]));
}

static void
ShowAuthors(const credEntry ace[], const char *title)
{

    int i;

    outputf("%s", title);
    outputc('\n');

    for (i = 0; ace[i].Name; ++i) {
        if (!(i % 3))
            outputc('\n');
        outputf("   %-20.20s", ace[i].Name);
    }

    outputc('\n');
    outputc('\n');

}

#ifndef WEB
extern void
CommandShowBrowser(char *UNUSED(sz))
{
    outputf(_("The current browser is %s\n"), get_web_browser());
}
#endif /* WEB */

extern void
CommandShowBuildInfo(char *UNUSED(sz))
{
    const char *pch;

#if USE_GTK
    if (fX)
        GTKShowBuildInfo(pwMain, NULL);
#endif

    while ((pch = GetBuildInfoString()) != 0)
        outputl(gettext(pch));

    outputc('\n');
}

extern void
CommandShowScoreSheet(char *UNUSED(sz))
{
    size_t i, width1, width2;
    char *data[2];
    listOLD *pl;

    if (ms.gs == GAME_NONE) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }
#if USE_GTK
    if (fX) {
        GTKShowScoreSheet();
        return;
    }
#endif

    output(_("Score Sheet - "));
    if (ms.nMatchTo > 0)
        outputf(ms.nMatchTo == 1 ? _("Match to %d point") : _("Match to %d points"), ms.nMatchTo);
    else
        output(_("Money Session"));

    output("\n\n");

    width1 = strlen(ap[0].szName);
    width2 = strlen(ap[1].szName);

    outputf("%s | %s\n", ap[0].szName, ap[1].szName);
    for (i = 0; i < width1 + width2 + 3; i++)
        outputc('-');
    output("\n");

    data[0] = malloc(50);
    data[1] = malloc(50);

    for (pl = lMatch.plNext; pl->p; pl = pl->plNext) {
        int score[2];
        listOLD *plGame = pl->plNext->p;

        if (plGame) {
            moverecord *pmr = plGame->plNext->p;
            score[0] = pmr->g.anScore[0];
            score[1] = pmr->g.anScore[1];
        } else {
            moverecord *pmr;
            listOLD *plGame = pl->p;
            if (!plGame) {
                continue;
            } else {
                pmr = plGame->plNext->p;
                score[0] = pmr->g.anScore[0];
                score[1] = pmr->g.anScore[1];
                if (pmr->g.fWinner == -1) {
                    if (pl == lMatch.plNext) {  /* First game */
                        score[0] = score[1] = 0;
                    } else
                        continue;
                } else
                    score[pmr->g.fWinner] += pmr->g.nPoints;
            }
        }
        sprintf(data[0], "%d", score[0]);
        sprintf(data[1], "%d", score[1]);
        outputf("%*s | %s\n", (int) width1, data[0], data[1]);
    }

    free(data[0]);
    free(data[1]);

    output("\n");
    outputx();
}

extern void
CommandShowCredits(char *UNUSED(sz))
{
#if USE_GTK
    if (fX) {
        GTKCommandShowCredits(NULL, NULL);
        return;
    }
#endif

    outputl(aszAUTHORS);

}

extern void
CommandShowWarranty(char *UNUSED(sz))
{

#if USE_GTK
    if (fX)
        ShowList(aszWarranty, _("Warranty"), NULL);
    else
#endif
        ShowPaged(aszWarranty);
}


extern void
show_kleinman(TanBoard an, char *sz)
{
    unsigned int anPips[2];
    float fKC;
    double rK;
    int diff, sum;

    PipCount((ConstTanBoard) an, anPips);
    sprintf(sz, _("Leader Pip Count : %d\n"), anPips[1]);
    sprintf(strchr(sz, 0), _("Trailer Pip Count: %d\n\n"), anPips[0]);

    sum = anPips[0] + anPips[1];
    diff = anPips[0] - anPips[1];

    sprintf(strchr(sz, 0), _("sum              : %d\n"), sum);
    sprintf(strchr(sz, 0), _("diff             : %d\n\n"), diff);
    rK = (double) (diff + 4) / (2 * sqrt(sum - 4));
    sprintf(strchr(sz, 0), _("K = (diff+4)/(2 sqrt(sum-4)) = %8.4g\n"), rK);

    fKC = KleinmanCount(anPips[1], anPips[0]);

    sprintf(strchr(sz, 0), _("Cubeless Winning Chance: %.4f\n\n"), fKC);
}


extern void
CommandShowKleinman(char *sz)
{
    char out[500];
    TanBoard an;

    if (!*sz && ms.gs == GAME_NONE) {
        outputl(_("No position specified and no game in progress."));
        return;
    }

    if (ParsePosition(an, &sz, NULL) < 0)
        return;

#if USE_GTK
    if (fX) {
        GTKShowRace(an);
        return;
    }
#endif
    show_kleinman(an, out);
    outputf("%s", out);
}

#if USE_MULTITHREAD
extern void
CommandShowThreads(char *UNUSED(sz))
{
    int c = MT_GetNumThreads();
    outputf(ngettext("%d calculation thread.\n", "%d calculation threads.\n", c), c);
}
#endif

extern void
show_thorp(TanBoard an, char *sz)
{
    int nLeader, nTrailer;
    float adjusted;

    ThorpCount((ConstTanBoard) an, &nLeader, &adjusted, &nTrailer);
    sprintf(sz, _("Thorp Count Leader            : %d\n"), nLeader);
    sprintf(strchr(sz, 0), _("Thorp Count Leader(+1/10)    L: %.1f\n"), adjusted);
    sprintf(strchr(sz, 0), _("Thorp Count Trailer          T: %d\n\n"), nTrailer);

    if (nTrailer >= (adjusted + 2))
        sprintf(strchr(sz, 0), _("Double, Drop (since L <= T - 2)\n"));
    else if (nTrailer >= (adjusted - 1))
        sprintf(strchr(sz, 0), _("Redouble, Take (since L <= T + 1 )\n"));
    else if (nTrailer >= (adjusted - 2))
        sprintf(strchr(sz, 0), _("Double, Take (since L <= T + 2)\n"));
    else
        sprintf(strchr(sz, 0), _("No Double, Take (since L > T + 2)\n"));
}

extern void
CommandShowThorp(char *sz)
{
    char out[500];
    TanBoard an;

    if (!*sz && ms.gs == GAME_NONE) {
        outputl(_("No position specified and no game in progress."));
        return;
    }

    if (ParsePosition(an, &sz, NULL) < 0)
        return;

#if USE_GTK
    if (fX) {
        GTKShowRace(an);
        return;
    }
#endif
    show_thorp(an, out);
    g_print("%s", out);

}

extern void
show_8912(TanBoard anBoard, char *sz)
{
    unsigned int anPips[2];
    float ahead;
    PipCount((ConstTanBoard) anBoard, anPips);
    sprintf(sz, _("Leader Pip Count : %d\n"), anPips[1]);
    sprintf(strchr(sz, 0), _("Trailer Pip Count: %d\n\n"), anPips[0]);
    ahead = ((float) anPips[0] - (float) anPips[1]) / (float) anPips[1] * 100.0f;
    sprintf(strchr(sz, 0), _("Leader is %.1f percent ahead\n"), ahead);

    if (ahead > 12.0f)
        sprintf(strchr(sz, 0), _("Double, Drop (ahead > 12 percent)\n"));
    else if (ahead > 9.0f)
        sprintf(strchr(sz, 0), _("Double, Take (12 percent > ahead > 9 percent)\n"));
    else if (ahead > 8.0f)
        sprintf(strchr(sz, 0), _("Redouble, Take (12 percent > ahead > 8 percent)\n"));
    else
        sprintf(strchr(sz, 0), _("NoDouble (8 percent > ahead)\n"));
}

extern void
CommandShow8912(char *sz)
{

    TanBoard anBoard;
    char out[500];

    if (!*sz && ms.gs == GAME_NONE) {
        outputl(_("No position specified and no game in progress."));
        return;
    }

    if (ParsePosition(anBoard, &sz, NULL) < 0)
        return;

#if USE_GTK
    if (fX) {
        GTKShowRace(anBoard);
        return;
    }
#endif
    show_8912(anBoard, out);
    outputl(out);
}

extern void
show_keith(TanBoard an, char *sz)
{
    int pn[2];
    float fL;

    KeithCount((ConstTanBoard) an, pn);

    fL = (float) pn[1] * 8.0f / 7.0f;
    sprintf(sz, _("Keith Count Leader            : %d\n"), pn[1]);
    sprintf(strchr(sz, 0), _("Keith Count Leader(+1/7)     L: %.1f\n"), fL);
    sprintf(strchr(sz, 0), _("Keith Count Trailer          T: %d\n\n"), pn[0]);

    if ((float) pn[0] >= (fL - 2.0f))
        sprintf(strchr(sz, 0), _("Double, Drop (since L <= T+2)"));
    else if ((float) pn[0] >= (fL - 3.0f))
        sprintf(strchr(sz, 0), _("Redouble, Take (since L <= T+3)"));
    else if ((float) pn[0] >= (fL - 4.0f))
        sprintf(strchr(sz, 0), _("Double, Take (since L <= T + 4)"));
    else
        sprintf(strchr(sz, 0), _("No Double, Take (since L > T + 4)"));
}


extern void
CommandShowKeith(char *sz)
{
    char out[500];
    TanBoard an;
    if (!*sz && ms.gs == GAME_NONE) {
        outputl(_("No position specified and no game in progress."));
        return;
    }

    if (ParsePosition(an, &sz, NULL) < 0)
        return;

#if USE_GTK
    if (fX) {
        GTKShowRace(an);
        return;
    }
#endif
    show_keith(an, out);
    output(out);
}

extern void
CommandShowBeavers(char *UNUSED(sz))
{

    if (nBeavers > 1)
        outputf(_("%d beavers/raccoons allowed in money sessions.\n"), nBeavers);
    else if (nBeavers == 1)
        outputl(_("1 beaver allowed in money sessions."));
    else
        outputl(_("No beavers allowed in money sessions."));
}

extern void
CommandShowGammonValues(char *UNUSED(sz))
{

    cubeinfo ci;
    int i;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));

        return;
    }
#if USE_GTK
    if (fX) {
        GTKShowTheory(1);
        return;
    }
#endif

    GetMatchStateCubeInfo(&ci, &ms);

    outputf("%-12s     %7s    %s\n", _("Player"), _("Gammon value"), _("Backgammon value"));

    for (i = 0; i < 2; i++) {

        outputf("%-12s     %7.5f         %7.5f\n",
                ap[i].szName, 0.5f * ci.arGammonPrice[i], 0.5f * (ci.arGammonPrice[2 + i] + ci.arGammonPrice[i]));
    }

}

static void
writeMET(float aafMET[][MAXSCORE], const int nRows, const int nCols, const int fInvert)
{

    int i, j;

    output("          ");
    for (j = 0; j < nCols; j++)
        outputf(_(" %3i-away "), j + 1);
    output("\n");

    for (i = 0; i < nRows; i++) {

        outputf(_(" %3i-away "), i + 1);

        for (j = 0; j < nCols; j++)
            outputf(" %8.4f ", fInvert ? 100.0f * (1.0 - GET_MET(i, j, aafMET)) : GET_MET(i, j, aafMET) * 100.0);
        output("\n");
    }
    output("\n");

}


static void
EffectivePipCount(const float arMu[2], const unsigned int anPips[2])
{

    int i;
    const float x = (2 * 3 + 3 * 4 + 4 * 5 + 4 * 6 + 6 * 7 +
                     5 * 8 + 4 * 9 + 2 * 10 + 2 * 11 + 1 * 12 + 1 * 16 + 1 * 20 + 1 * 24) / 36.0f;

    outputl("");
    outputl(_("Effective pip count:"));

    outputf("%-20.20s   %7s  %7s\n", "", _("EPC"), _("Wastage"));
    for (i = 0; i < 2; ++i)
        outputf("%-20.20s   %7.3f  %7.3f\n",
                ap[i].szName, arMu[ms.fMove ? i : !i] * x, arMu[ms.fMove ? i : !i] * x - anPips[ms.fMove ? i : !i]);

    outputf(_("\n" "EPC = Avg. rolls * %5.3f\n" "Wastage = EPC - Pips\n\n"), x);

}

extern void
CommandShowOneSidedRollout(char *sz)
{

    TanBoard anBoard;
    int nTrials = 5760;
    float arMu[2];
    float ar[5];
    unsigned int anPips[2];

    if (!*sz && ms.gs == GAME_NONE) {
        outputl(_("No position specified and no game in progress."));
        return;
    }

    if (ParsePosition(anBoard, &sz, NULL) < 0)
        return;


#if USE_GTK
    if (fX) {
        GTKShowRace(anBoard);
        return;
    }
#endif

    outputf(_("One sided rollout with %d trials (%s on roll):\n"), nTrials, ap[ms.fMove].szName);

    raceProbs((ConstTanBoard) anBoard, nTrials, ar, arMu);
    outputl(OutputPercents(ar, TRUE));

    PipCount((ConstTanBoard) anBoard, anPips);
    EffectivePipCount(arMu, anPips);

}


extern void
CommandShowMatchEquityTable(char *sz)
{

    /* Read a number n. */

    int n = ParseNumber(&sz);
    int i;
    int anScore[2];

    /* If n > 0 write n x n match equity table,
     * else if match write nMatchTo x nMatchTo table,
     * else write full table (may be HUGE!) */

    if ((n <= 0) || (n > MAXSCORE)) {
        if (ms.nMatchTo)
            n = ms.nMatchTo;
        else
            n = MAXSCORE;
    }

    if (ms.nMatchTo && ms.anScore[0] <= n && ms.anScore[1] <= n) {
        anScore[0] = ms.anScore[0];
        anScore[1] = ms.anScore[1];
    } else
        anScore[0] = anScore[1] = -1;


#if USE_GTK
    if (fX) {
        GTKShowMatchEquityTable(n, anScore);
        return;
    }
#endif

    output(_("Match equity table: "));
    outputl((char *) miCurrent.szName);
    outputf("(%s)\n", miCurrent.szFileName);
    outputl((char *) miCurrent.szDescription);
    outputl("");

    /* write tables */

    output(_("Pre-Crawford table:\n\n"));
    writeMET(aafMET, n, n, FALSE);

    for (i = 0; i < 2; i++) {
        outputf(_("Post-Crawford table for player %d (%s):\n\n"), i, ap[i].szName);
        writeMET((float (*)[MAXSCORE]) aafMETPostCrawford[i], 1, n, FALSE);
    }

}

extern void
CommandShowOutput(char *UNUSED(sz))
{

    outputf(fOutputMatchPC ?
            _("Match winning chances will be shown as percentages.\n") :
            _("Match winning chances will be shown as probabilities.\n"));

    if (fOutputMWC)
        outputl(_("Match equities shown in MWC (match winning chance) " "(match play only)."));
    else
        outputl(_("Match equities shown in EMG (normalized money game equity) " "(match play only)."));

    outputf(fOutputWinPC ?
            _("Game winning chances will be shown as percentages.\n") :
            _("Game winning chances will be shown as probabilities.\n"));

#if USE_GTK
    if (!fX)
#endif
        outputf(fOutputRawboard ? _("Boards will be shown in raw format.\n") : _("Boards will be shown in ASCII.\n"));
}

extern void
CommandShowVersion(char *UNUSED(sz))
{
#if USE_GTK
    if (fX) {
        GTKShowVersion();
        return;
    }
#endif

    outputl(gettext(VERSION_STRING));
    outputc('\n');
    ShowAuthors(ceAuthors, _("AUTHORS"));
}

extern void
CommandShowMarketWindow(char *sz)
{

    cubeinfo ci;

    float arOutput[NUM_OUTPUTS];
    float arDP1[2], arDP2[2], arCP1[2], arCP2[2];
    float rDTW, rDTL, rNDW, rNDL, rDP, rRisk, rGain, r;

    float aarRates[2][2];

    int i, fAutoRedouble[2], afDead[2], anNormScore[2];

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));

        return;
    }
#if USE_GTK
    if (fX) {
        GTKShowTheory(0);
        return;
    }
#endif


    /* Show market window */

    /* First, get gammon and backgammon percentages */
    GetMatchStateCubeInfo(&ci, &ms);

    /* see if ratios are given on command line */

    aarRates[0][0] = ParseReal(&sz);

    if (aarRates[0][0] >= 0) {

        /* read the others */

        aarRates[1][0] = ((r = ParseReal(&sz)) > 0.0f) ? r : 0.0f;
        aarRates[0][1] = ((r = ParseReal(&sz)) > 0.0f) ? r : 0.0f;
        aarRates[1][1] = ((r = ParseReal(&sz)) > 0.0f) ? r : 0.0f;

        /* If one of the ratios are larger than 1 we assume the user
         * has entered 25.1 instead of 0.251 */

        if (aarRates[0][0] > 1.0f || aarRates[1][0] > 1.0f || aarRates[0][1] > 1.0f || aarRates[1][1] > 1.0f) {
            aarRates[0][0] /= 100.0f;
            aarRates[1][0] /= 100.0f;
            aarRates[0][1] /= 100.0f;
            aarRates[1][1] /= 100.0f;
        }

        /* Check that ratios are 0 <= ratio <= 1 */

        for (i = 0; i < 2; i++) {
            if (aarRates[i][0] > 1.0f) {
                outputf(_("illegal gammon ratio for player %i: %f\n"), i, aarRates[i][0]);
                return;
            }
            if (aarRates[i][1] > 1.0f) {
                outputf(_("illegal backgammon ratio for player %i: %f\n"), i, aarRates[i][1]);
                return;
            }
        }

        /* Transfer ratios to arOutput
         * (used in call to GetPoints below) */

        arOutput[OUTPUT_WIN] = 0.5f;
        arOutput[OUTPUT_WINGAMMON] = (aarRates[ms.fMove][0] + aarRates[ms.fMove][1]) * 0.5f;
        arOutput[OUTPUT_LOSEGAMMON] = (aarRates[!ms.fMove][0] + aarRates[!ms.fMove][1]) * 0.5f;
        arOutput[OUTPUT_WINBACKGAMMON] = aarRates[ms.fMove][1] * 0.5f;
        arOutput[OUTPUT_LOSEBACKGAMMON] = aarRates[!ms.fMove][1] * 0.5f;

    } else {

        /* calculate them based on current position */

        if (getCurrentGammonRates(aarRates, arOutput, msBoard(), &ci, &GetEvalCube()->ec) < 0)
            return;

    }



    for (i = 0; i < 2; i++)
        outputf(_("Player %-25s: gammon rate %6.2f%%, bg rate %6.2f%%\n"),
                ap[i].szName, aarRates[i][0] * 100.0, aarRates[i][1] * 100.0);


    if (ms.nMatchTo) {

        for (i = 0; i < 2; i++)
            anNormScore[i] = ms.nMatchTo - ms.anScore[i];

        GetPoints(arOutput, &ci, arCP2);

        for (i = 0; i < 2; i++) {

            fAutoRedouble[i] = (anNormScore[i] - 2 * ms.nCube <= 0) && (anNormScore[!i] - 2 * ms.nCube > 0);

            afDead[i] = (anNormScore[!i] - 2 * ms.nCube <= 0);

            /* MWC for "double, take; win" */

            rDTW =
                (1.0f - aarRates[i][0] - aarRates[i][1]) *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      2 * ms.nCube, i, ms.fCrawford, aafMET, aafMETPostCrawford)
                + aarRates[i][0] *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      4 * ms.nCube, i, ms.fCrawford, aafMET, aafMETPostCrawford)
                + aarRates[i][1] *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      6 * ms.nCube, i, ms.fCrawford, aafMET, aafMETPostCrawford);

            /* MWC for "no double, take; win" */

            rNDW =
                (1.0f - aarRates[i][0] - aarRates[i][1]) *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      ms.nCube, i, ms.fCrawford, aafMET, aafMETPostCrawford)
                + aarRates[i][0] *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      2 * ms.nCube, i, ms.fCrawford, aafMET, aafMETPostCrawford)
                + aarRates[i][1] *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      3 * ms.nCube, i, ms.fCrawford, aafMET, aafMETPostCrawford);

            /* MWC for "Double, take; lose" */

            rDTL =
                (1.0f - aarRates[!i][0] - aarRates[!i][1]) *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      2 * ms.nCube, !i, ms.fCrawford, aafMET, aafMETPostCrawford)
                + aarRates[!i][0] *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      4 * ms.nCube, !i, ms.fCrawford, aafMET, aafMETPostCrawford)
                + aarRates[!i][1] *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      6 * ms.nCube, !i, ms.fCrawford, aafMET, aafMETPostCrawford);

            /* MWC for "No double; lose" */

            rNDL =
                (1.0f - aarRates[!i][0] - aarRates[!i][1]) *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      1 * ms.nCube, !i, ms.fCrawford, aafMET, aafMETPostCrawford)
                + aarRates[!i][0] *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      2 * ms.nCube, !i, ms.fCrawford, aafMET, aafMETPostCrawford)
                + aarRates[!i][1] *
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      3 * ms.nCube, !i, ms.fCrawford, aafMET, aafMETPostCrawford);

            /* MWC for "Double, pass" */

            rDP =
                getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                      ms.nCube, i, ms.fCrawford, aafMET, aafMETPostCrawford);

            /* Double point */

            rRisk = rNDL - rDTL;
            rGain = rDTW - rNDW;

            arDP1[i] = rRisk / (rRisk + rGain);
            arDP2[i] = arDP1[i];

            /* Dead cube take point without redouble */

            rRisk = rDTW - rDP;
            rGain = rDP - rDTL;

            arCP1[i] = 1.0f - rRisk / (rRisk + rGain);

            if (fAutoRedouble[i]) {

                /* With redouble */

                rDTW =
                    (1.0f - aarRates[i][0] - aarRates[i][1]) *
                    getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                          4 * ms.nCube, i, ms.fCrawford, aafMET, aafMETPostCrawford)
                    + aarRates[i][0] *
                    getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                          8 * ms.nCube, i, ms.fCrawford, aafMET, aafMETPostCrawford)
                    + aarRates[i][1] *
                    getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                          12 * ms.nCube, i, ms.fCrawford, aafMET, aafMETPostCrawford);

                rDTL =
                    (1.0f - aarRates[!i][0] - aarRates[!i][1]) *
                    getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                          4 * ms.nCube, !i, ms.fCrawford, aafMET, aafMETPostCrawford)
                    + aarRates[!i][0] *
                    getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                          8 * ms.nCube, !i, ms.fCrawford, aafMET, aafMETPostCrawford)
                    + aarRates[!i][1] *
                    getME(ms.anScore[0], ms.anScore[1], ms.nMatchTo, i,
                          12 * ms.nCube, !i, ms.fCrawford, aafMET, aafMETPostCrawford);

                rRisk = rDTW - rDP;
                rGain = rDP - rDTL;

                arCP2[i] = 1.0f - rRisk / (rRisk + rGain);

                /* Double point */

                rRisk = rNDL - rDTL;
                rGain = rDTW - rNDW;

                arDP2[i] = rRisk / (rRisk + rGain);

            }
        }

        /* output */

        output("\n\n");

        for (i = 0; i < 2; i++) {

            outputf(_("Player %s market window:\n\n"), ap[i].szName);

            if (fAutoRedouble[i])
                output(_("Dead cube (opponent doesn't redouble): "));
            else
                output(_("Dead cube: "));

            outputf("%6.2f%% - %6.2f%%\n", 100. * arDP1[i], 100. * arCP1[i]);

            if (fAutoRedouble[i])
                outputf(_("Dead cube (opponent redoubles):" "%6.2f%% - %6.2f%%\n\n"), 100. * arDP2[i], 100. * arCP2[i]);
            else if (!afDead[i])
                outputf(_("Live cube:" "%6.2f%% - %6.2f%%\n\n"), 100. * arDP2[i], 100. * arCP2[i]);

        }

    } /* ms.nMatchTo */
    else {

        /* money play: use Janowski's formulae */

        /* 
         * FIXME's: (1) make GTK version
         *          (2) make output for "current" value of X
         *          (3) improve layout?
         */

        const char *aszMoneyPointLabel[] = {
            N_("Take Point (TP)"),
            N_("Beaver Point (BP)"),
            N_("Raccoon Point (RP)"),
            N_("Initial Double Point (IDP)"),
            N_("Redouble Point (RDP)"),
            N_("Cash Point (CP)"),
            N_("Too good Point (TGP)")
        };

        float aaarPoints[2][7][2];

        int i, j;

        getMoneyPoints(aaarPoints, ci.fJacoby, ci.fBeavers, aarRates);

        for (i = 0; i < 2; i++) {

            outputf(_("\nPlayer %s cube parameters:\n\n"), ap[i].szName);
            outputf("%-27s  %7s      %s\n", _("Cube parameter"), _("Dead Cube"), _("Live Cube"));

            for (j = 0; j < 7; j++)
                outputf("%-27s  %7.3f%%     %7.3f%%\n",
                        gettext(aszMoneyPointLabel[j]), aaarPoints[i][j][0] * 100.0f, aaarPoints[i][j][1] * 100.0f);

        }

    }

}


extern void
CommandShowExport(char *UNUSED(sz))
{

    int i;

#if USE_GTK
    if (fX) {
        GTKShowExport(&exsExport);
        return;
    }
#endif

    output(_("\n"
             "Export settings: \n\n"
             "WARNING: not all settings are honoured in the export!\n"
             "         Do not expect too much!\n\n" "Include: \n\n"));

    output(_("- annotations"));
    outputf("\r\t\t\t\t\t\t: %s\n", exsExport.fIncludeAnnotation ? _("yes") : _("no"));
    output(_("- analysis"));
    outputf("\r\t\t\t\t\t\t: %s\n", exsExport.fIncludeAnalysis ? _("yes") : _("no"));
    output(_("- statistics"));
    outputf("\r\t\t\t\t\t\t: %s\n", exsExport.fIncludeStatistics ? _("yes") : _("no"));
    output(_("- legend"));
    output(_("- match information"));
    outputf("\r\t\t\t\t\t\t: %s\n\n", exsExport.fIncludeMatchInfo ? _("yes") : _("no"));

    outputl(_("Show: \n"));
    output(_("- board"));
    output("\r\t\t\t\t\t\t: ");
    if (!exsExport.fDisplayBoard)
        outputl(_("never"));
    else
        outputf(_("on every %d move\n"), exsExport.fDisplayBoard);

    output(_("- players"));
    output("\r\t\t\t\t\t\t: ");
    if (exsExport.fSide == 3)
        outputl(_("both"));
    else
        outputf("%s\n", ap[exsExport.fSide - 1].szName);

    outputl(_("\nOutput move analysis:\n"));

    output(_("- show at most"));
    output("\r\t\t\t\t\t\t: ");
    outputf(_("%d moves\n"), exsExport.nMoves);

    output(_("- show detailed probabilities"));
    output("\r\t\t\t\t\t\t: ");
    outputl(exsExport.fMovesDetailProb ? _("yes") : _("no"));

    output(_("- show evaluation parameters"));
    output("\r\t\t\t\t\t\t: ");
    outputl(exsExport.afMovesParameters[0] ? _("yes") : _("no"));

    output(_("- show rollout parameters"));
    output("\r\t\t\t\t\t\t: ");
    outputl(exsExport.afMovesParameters[1] ? _("yes") : _("no"));

    for (i = 0; i < N_SKILLS; i++) {
        if (i == SKILL_NONE)
            output(_("- for unmarked moves"));
        else
            outputf(_("- for moves marked '%s'"), gettext(aszSkillType[i]));

        output("\r\t\t\t\t\t\t: ");
        outputl(exsExport.afMovesDisplay[i] ? _("yes") : _("no"));

    }

    outputl(_("\nOutput cube decision analysis:\n"));

    output(_("- show detailed probabilities"));
    output("\r\t\t\t\t\t\t: ");
    outputl(exsExport.fCubeDetailProb ? _("yes") : _("no"));

    output(_("- show evaluation parameters"));
    output("\r\t\t\t\t\t\t: ");
    outputl(exsExport.afCubeParameters[0] ? _("yes") : _("no"));

    output(_("- show rollout parameters"));
    output("\r\t\t\t\t\t\t: ");
    outputl(exsExport.afCubeParameters[1] ? _("yes") : _("no"));

    for (i = 0; i < N_SKILLS; i++) {
        if (i == SKILL_NONE)
            output(_("- for unmarked cube decisions"));
        else
            outputf(_("- for cube decisions marked '%s'"), gettext(aszSkillType[i]));

        output("\r\t\t\t\t\t\t: ");
        outputl(exsExport.afCubeDisplay[i] ? _("yes") : _("no"));

    }

    output(_("- actual cube decisions"));
    output("\r\t\t\t\t\t\t: ");
    outputl(exsExport.afCubeDisplay[EXPORT_CUBE_ACTUAL] ? _("yes") : _("no"));

    output(_("- missed cube decisions"));
    output("\r\t\t\t\t\t\t: ");
    outputl(exsExport.afCubeDisplay[EXPORT_CUBE_MISSED] ? _("yes") : _("no"));

    output(_("- close cube decisions"));
    output("\r\t\t\t\t\t\t: ");
    outputl(exsExport.afCubeDisplay[EXPORT_CUBE_CLOSE] ? _("yes") : _("no"));

    outputl(_("\nHTML options:\n"));

    outputf(_("- HTML export type used in export\n" "\t%s\n"), aszHTMLExportType[exsExport.het]);


    outputf(_("- URL to pictures used in export\n"
              "\t%s\n"), exsExport.szHTMLPictureURL ? exsExport.szHTMLPictureURL : _("not defined"));

    outputf(_("- size of exported HTML pictures: %dx%d\n"),
            exsExport.nHtmlSize * BOARD_WIDTH, exsExport.nHtmlSize * BOARD_HEIGHT);

    /* PNG options */

    outputl(_("PNG options:\n"));

    outputf(_("- size of exported PNG pictures: %dx%d\n"),
            exsExport.nPNGSize * BOARD_WIDTH, exsExport.nPNGSize * BOARD_HEIGHT);

    outputl("\n");

}



extern void
CommandShowTutor(char *UNUSED(sz))
{

    char *level;

    switch (TutorSkill) {
    default:
    case SKILL_DOUBTFUL:
        level = _("'doubtful' or worse");
        break;

    case SKILL_BAD:
        level = _("'bad' or worse");
        break;

    case SKILL_VERYBAD:
        level = _("'very bad'");
        break;
    }

    if (fTutor && fTutorChequer)
        outputf(_("Warnings are given for %s chequer moves.\n"), level);
    else
        outputl(_("No warnings are given for chequer moves."));

    if (fTutor && fTutorCube)
        outputf(_("Warnings are given for %s cube decisions.\n"), level);
    else
        outputl(_("No warnings are given for cube decisions."));

}


extern void
CommandShowSound(char *UNUSED(sz))
{

    int i;

    outputf(_("Sounds are enabled          : %s\n"), fSound ? _("yes") : _("no"));

    outputl(_("Sounds for:"));

    for (i = 0; i < NUM_SOUNDS; ++i) {
        char *sound = GetSoundFile(i);
        if (!*sound)
            outputf(_("   %-30.30s : no sound\n"), gettext(sound_description[i]));
        else
            outputf("   %-30.30s : \"%s\"\n", gettext(sound_description[i]), sound);
        g_free(sound);
    }
}



extern void
CommandShowRolls(char *sz)
{

#if USE_GTK
    int nDepth = ParseNumber(&sz);
#else
    (void) sz;                  /* suppress unused parameter compiler warning */
#endif

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));

        return;
    }
#if USE_GTK

    if (fX) {
        static evalcontext ec0ply = { TRUE, 0, FALSE, TRUE, 0.0 };
        GTKShowRolls(nDepth, &ec0ply, &ms);
        return;
    }
#endif

    CommandNotImplemented(NULL);

}



extern void
CommandShowTemperatureMap(char *sz)
{

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));

        return;
    }
#if USE_GTK

    if (fX) {

        if (sz && *sz && !strncmp(sz, "=cube", 5)) {

            cubeinfo ci;
            GetMatchStateCubeInfo(&ci, &ms);
            if (GetDPEq(NULL, NULL, &ci)) {

                /* cube is available */

                matchstate ams[2];
                int i;
                gchar *asz[2];

                for (i = 0; i < 2; ++i)
                    memcpy(&ams[i], &ms, sizeof(matchstate));

                ams[1].nCube *= 2;
                ams[1].fCubeOwner = !ams[1].fMove;

                for (i = 0; i < 2; ++i) {
                    asz[i] = g_malloc(200);
                    GetMatchStateCubeInfo(&ci, &ams[i]);
                    FormatCubePosition(asz[i], &ci);
                }

                GTKShowTempMap(ams, 2, asz, FALSE);

                for (i = 0; i < 2; ++i)
                    g_free(asz[i]);

            } else
                outputl(_("Cube is not available."));

        } else
            GTKShowTempMap(&ms, 1, NULL, FALSE);

        return;
    }
#else
    (void) sz;                  /* suppress unused parameter compiler warning */
#endif

    CommandNotImplemented(NULL);

}

extern void
CommandShowVariation(char *UNUSED(sz))
{

    if (ms.gs != GAME_NONE)
        outputf(_("You are playing: %s\n"), gettext(aszVariations[ms.bgv]));

    outputf(_("Default variation is: %s\n"), gettext(aszVariations[bgvDefault]));

}

extern void
CommandShowCheat(char *UNUSED(sz))
{

    outputf(_("Manipulation with dice is %s.\n"), fCheat ? _("enabled") : _("disabled"));

    if (fCheat) {
        PrintCheatRoll(0, afCheatRoll[0]);
        PrintCheatRoll(1, afCheatRoll[1]);
    }

}

extern void
CommandShowRatingOffset(char *UNUSED(sz))
{
    outputf(_("The rating offset for estimating absolute ratings is: %.1f\n"), rRatingOffset);
}


extern void
CommandShowCubeEfficiency(char *UNUSED(sz))
{
    outputf(_("Parameters for cube evaluations:\n"));
    outputf("%s :%7.4f\n", _("Cube efficiency for crashed positions"), rCrashedX);
    outputf("%s :%7.4f\n", _("Cube efficiency for contact positions"), rContactX);
    outputf("%s :%7.4f\n", _("Cube efficiency for one sided bearoff positions"), rOSCubeX);
    outputf("%s * %.5f + %.5f\n", _("Cube efficiency for race: x = pips"), rRaceFactorX, rRaceCoefficientX);
    outputf(_("(min value %.4f, max value %.4f)\n"), rRaceMin, rRaceMax);
}

extern void
show_bearoff(TanBoard an, char *szTemp)
{
    strcpy(szTemp, _("The following numbers are for money games only.\n\n"));
    switch (ms.bgv) {
    case VARIATION_STANDARD:
    case VARIATION_NACKGAMMON:
        if (isBearoff(pbcTS, (ConstTanBoard) an)) {
            BearoffDump(pbcTS, (ConstTanBoard) an, szTemp);
        } else if (isBearoff(pbc2, (ConstTanBoard) an)) {
            BearoffDump(pbc2, (ConstTanBoard) an, szTemp);
        } else
            strcpy(szTemp, _("Position not in any two-sided database\n"));
        break;

    case VARIATION_HYPERGAMMON_1:
    case VARIATION_HYPERGAMMON_2:
    case VARIATION_HYPERGAMMON_3:

        if (isBearoff(apbcHyper[ms.bgv - VARIATION_HYPERGAMMON_1], (ConstTanBoard) an)) {
            BearoffDump(apbcHyper[ms.bgv - VARIATION_HYPERGAMMON_1], (ConstTanBoard) an, szTemp);
            outputl(szTemp);
        }

        break;

    default:

        g_assert_not_reached();

    }

}

extern void
CommandShowBearoff(char *sz)
{
    char out[500];
    TanBoard an;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game is being played."));
        return;
    }

    if (ParsePosition(an, &sz, NULL) < 0)
        return;
#if USE_GTK
    if (fX) {
        GTKShowRace(an);
        return;
    }
#endif
    show_bearoff(an, out);
}



extern void
CommandShowMatchResult(char *UNUSED(sz))
{

    float arSum[2] = { 0.0f, 0.0f };
    float arSumSquared[2] = { 0.0f, 0.0f };
    int n = 0;
    moverecord *pmr;
    xmovegameinfo *pmgi;
    statcontext *psc;
    listOLD *pl;
    float r;

    updateStatisticsMatch(&lMatch);

    outputf(_("Actual and luck adjusted results for %s\n\n"), ap[0].szName);
    outputf("%-10s %-10s %-10s\n\n", _("Game"), _("Actual"), _("Luck adj."));

    for (pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext, ++n) {

        pmr = (moverecord *) ((listOLD *) pl->p)->plNext->p;
        pmgi = &pmr->g;
        g_assert(pmr->mt == MOVE_GAMEINFO);

        psc = &pmgi->sc;

        if (psc->fDice) {
            if (ms.nMatchTo)
                outputf("%10d %9.2f%% %9.2f%%\n",
                        n,
                        100.0 * (0.5f + psc->arActualResult[0]),
                        100.0 * (0.5f + psc->arActualResult[0] - psc->arLuck[0][1] + psc->arLuck[1][1]));
            else
                outputf("%10d %9.3f%% %9.3f%%\n",
                        n, psc->arActualResult[0], psc->arActualResult[0] - psc->arLuck[0][1] + psc->arLuck[1][1]);
        } else
            outputf(_("%10d no info available\n"), n);

        r = psc->arActualResult[0];
        arSum[0] += r;
        arSumSquared[0] += r * r;

        r = psc->arActualResult[0] - psc->arLuck[0][1] + psc->arLuck[1][1];
        arSum[1] += r;
        arSumSquared[1] += r * r;


    }

    if (ms.nMatchTo)
        outputf("%10s %9.2f%% %9.2f%%\n", _("Final"), 100.0 * (0.5f + arSum[0]), 100.0 * (0.5f + arSum[1]));
    else
        outputf("%10s %+9.3f %+9.3f\n", _("Sum"), arSum[0], arSum[1]);

    if (n && !ms.nMatchTo) {
        outputf("%10s %+9.3f %+9.3f\n", _("Average"), arSum[0] / n, arSum[1] / n);
        outputf("%10s %9.3f %9.3f\n", "95%CI",
                1.95996f *
                sqrt(arSumSquared[0] / n -
                     arSum[0] * arSum[0] / (n * n)) / sqrt(n),
                1.95996f * sqrt(arSumSquared[1] / n - arSum[1] * arSum[1] / (n * n)) / sqrt(n));
    }

}


#ifndef WEB
extern void
CommandShowManualWeb(char *UNUSED(sz))
{
    char *path = g_build_filename(getDocDir(), "gnubg.html", NULL);
    OpenURL(path);
    g_free(path);
}

extern void
CommandShowManualAbout(char *UNUSED(sz))
{
    char *path = g_build_filename(getDocDir(), "allabout.html", NULL);
    OpenURL(path);
    g_free(path);
}
#endif /* WEB */

extern void
CommandShowAutoSave(char *UNUSED(sz))
{
    outputf(ngettext
            ("Auto save frequency every %d minute\n", "Auto save every %d minutes\n", nAutoSaveTime), nAutoSaveTime);
    outputf(fAutoSaveRollout ? _("Match will be autosaved during and after rollouts\n") :
            _("Match will not be autosaved during and after rollouts\n"));
    outputf(fAutoSaveAnalysis ? _("Match will be autosaved during and after analysis\n") :
            _("Match will not be autosaved during and after analysis\n"));
}
