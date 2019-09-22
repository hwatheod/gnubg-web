/*
 * sgf.c
 *
 * by Gary Wong <gtw@gnu.org>, 2000, 2001.
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
 * $Id: sgf.c,v 1.155 2015/02/01 18:11:53 plm Exp $
 */

#include "config.h"
#include "common.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backgammon.h"
#include "dice.h"
#include "eval.h"
#if USE_GTK
#include "gtkgame.h"
#endif
#include "analysis.h"
#include "positionid.h"
#include "sgf.h"

static const char *szFile;
static int fError;

static int CheckSGFVersion(char **sz);
static void
ErrorHandler(const char *sz, int UNUSED(fParseError))
{

    if (!fError) {
        fError = TRUE;
        outputerrf("%s: %s", szFile, sz);
    }
}

static void
FreeList(listOLD * pl, int nLevel)
{
    /* Levels are:
     *  0 - GameTreeSeq
     *  1 - GameTree
     *  2 - Sequence
     *  3 - Node
     *  4 - Property
     */

    if (nLevel == 1) {
        FreeList(pl->plNext->p, 2);     /* initial sequence */
        ListDelete(pl->plNext);
        nLevel = 0;             /* remainder of list is more GameTreeSeqs */
    }

    while (pl->plNext != pl) {
        if (nLevel == 3) {
            FreeList(((property *) pl->plNext->p)->pl, 4);
            free(pl->plNext->p);
        } else if (nLevel == 4)
            free(pl->plNext->p);
        else
            FreeList(pl->plNext->p, nLevel + 1);

        ListDelete(pl->plNext);
    }

    if (nLevel != 4)
        free(pl);
}

static void
FreeGameTreeSeq(listOLD * pl)
{

    FreeList(pl, 0);
}

static listOLD *
LoadCollection(char *sz)
{

    listOLD *plCollection, *pl, *plRoot, *plProp;
    FILE *pf;
    int fBackgammon;
    property *pp;

    fError = FALSE;
    SGFErrorHandler = ErrorHandler;

    if (strcmp(sz, "-")) {
        if (!(pf = g_fopen(sz, "r"))) {
            outputerr(sz);
            return NULL;
        }
        szFile = sz;
    } else {
        /* FIXME does it really make sense to try to load from stdin? */
        pf = stdin;
        szFile = "(stdin)";
    }

    plCollection = SGFParse(pf);

    if (pf != stdin)
        fclose(pf);

    /* Traverse collection, looking for backgammon games. */
    if (plCollection) {
        pl = plCollection->plNext;
        while (pl != plCollection) {
            plRoot = ((listOLD *) ((listOLD *) pl->p)->plNext->p)->plNext->p;
            fBackgammon = FALSE;
            for (plProp = plRoot->plNext; plProp != plRoot; plProp = plProp->plNext) {
                pp = plProp->p;

                if (pp->ach[0] == 'G' && pp->ach[1] == 'M' && pp->pl->plNext->p && atoi((char *)
                                                                                        pp->pl->plNext->p) == 6) {
                    fBackgammon = TRUE;
                    break;
                }
            }

            pl = pl->plNext;

            if (!fBackgammon) {
                FreeList(pl->plPrev->p, 1);
                ListDelete(pl->plPrev);
            }
        }

        if (ListEmpty(plCollection)) {
            ErrorHandler(_("warning: no backgammon games in SGF file"), TRUE);
            free(plCollection);
            plCollection = NULL;
        }
    }

    return plCollection;
}

static void
CopyName(int i, char *sz)
{

    char *pc;

    /* FIXME sanity check the name as in CommandSetPlayerName */

    pc = g_strdup(sz);

    if (strlen(pc) > 31)
        pc[31] = 0;

    strcpy(ap[i].szName, pc);

    g_free(pc);

}

static void
SetScore(xmovegameinfo * pmgi, int fBlack, int n)
{

    if (n >= 0 && (!pmgi->nMatch || n < pmgi->nMatch))
        pmgi->anScore[fBlack] = n;
}

static void
RestoreMI(listOLD * pl, moverecord * pmr)
{

    char *pch;
    xmovegameinfo *pmgi = &pmr->g;

    for (pl = pl->plNext; (pch = pl->p); pl = pl->plNext)
        if (!strncmp(pch, "length:", 7)) {
            pmgi->nMatch = atoi(pch + 7);

            if (pmgi->nMatch < 0)
                pmgi->nMatch = 0;
        } else if (!strncmp(pch, "game:", 5)) {
            pmgi->i = atoi(pch + 5);

            if (pmgi->i < 0)
                pmgi->i = 0;
        } else if (!strncmp(pch, "ws:", 3) || !strncmp(pch, "bs:", 3))
            SetScore(pmgi, *pch == 'b', atoi(pch + 3));

}

static void
RestoreGS(listOLD * pl, statcontext * psc)
{

    char *pch;
    lucktype lt;
    skilltype st;

    for (pl = pl->plNext; (pch = pl->p); pl = pl->plNext)
        switch (*pch) {
        case 'M':              /* moves */
            psc->fMoves = TRUE;

            psc->anUnforcedMoves[0] = (int)strtol(pch + 2, &pch, 10);
            psc->anUnforcedMoves[1] = (int)strtol(pch, &pch, 10);
            psc->anTotalMoves[0] = (int)strtol(pch, &pch, 10);
            psc->anTotalMoves[1] = (int)strtol(pch, &pch, 10);

            for (st = SKILL_VERYBAD; st <= SKILL_NONE; st++) {
                psc->anMoves[0][st] = (int)strtol(pch, &pch, 10);
                psc->anMoves[1][st] = (int)strtol(pch, &pch, 10);
            }

            psc->arErrorCheckerplay[0][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorCheckerplay[0][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorCheckerplay[1][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorCheckerplay[1][1] = (float)g_ascii_strtod(pch, &pch);

            break;

        case 'C':              /* cube */
            psc->fCube = TRUE;

            psc->anTotalCube[0] = (int)strtol(pch + 2, &pch, 10);
            psc->anTotalCube[1] = (int)strtol(pch, &pch, 10);
            psc->anDouble[0] = (int)strtol(pch, &pch, 10);
            psc->anDouble[1] = (int)strtol(pch, &pch, 10);
            psc->anTake[0] = (int)strtol(pch, &pch, 10);
            psc->anTake[1] = (int)strtol(pch, &pch, 10);
            psc->anPass[0] = (int)strtol(pch, &pch, 10);
            psc->anPass[1] = (int)strtol(pch, &pch, 10);

            psc->anCubeMissedDoubleDP[0] = (int)strtol(pch, &pch, 10);
            psc->anCubeMissedDoubleDP[1] = (int)strtol(pch, &pch, 10);
            psc->anCubeMissedDoubleTG[0] = (int)strtol(pch, &pch, 10);
            psc->anCubeMissedDoubleTG[1] = (int)strtol(pch, &pch, 10);
            psc->anCubeWrongDoubleDP[0] = (int)strtol(pch, &pch, 10);
            psc->anCubeWrongDoubleDP[1] = (int)strtol(pch, &pch, 10);
            psc->anCubeWrongDoubleTG[0] = (int)strtol(pch, &pch, 10);
            psc->anCubeWrongDoubleTG[1] = (int)strtol(pch, &pch, 10);
            psc->anCubeWrongTake[0] = (int)strtol(pch, &pch, 10);
            psc->anCubeWrongTake[1] = (int)strtol(pch, &pch, 10);
            psc->anCubeWrongPass[0] = (int)strtol(pch, &pch, 10);
            psc->anCubeWrongPass[1] = (int)strtol(pch, &pch, 10);

            psc->arErrorMissedDoubleDP[0][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorMissedDoubleDP[0][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorMissedDoubleTG[0][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorMissedDoubleTG[0][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongDoubleDP[0][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongDoubleDP[0][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongDoubleTG[0][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongDoubleTG[0][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongTake[0][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongTake[0][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongPass[0][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongPass[0][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorMissedDoubleDP[1][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorMissedDoubleDP[1][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorMissedDoubleTG[1][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorMissedDoubleTG[1][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongDoubleDP[1][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongDoubleDP[1][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongDoubleTG[1][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongDoubleTG[1][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongTake[1][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongTake[1][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongPass[1][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arErrorWrongPass[1][1] = (float)g_ascii_strtod(pch, &pch);

            break;

        case 'D':              /* dice */
            psc->fDice = TRUE;

            pch += 2;

            for (lt = LUCK_VERYBAD; lt <= LUCK_VERYGOOD; lt++) {
                psc->anLuck[0][lt] = (int)strtol(pch, &pch, 10);
                psc->anLuck[1][lt] = (int)strtol(pch, &pch, 10);
            }

            psc->arLuck[0][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arLuck[0][1] = (float)g_ascii_strtod(pch, &pch);
            psc->arLuck[1][0] = (float)g_ascii_strtod(pch, &pch);
            psc->arLuck[1][1] = (float)g_ascii_strtod(pch, &pch);

            break;

        default:
            /* ignore */
            break;
        }

    AddStatcontext(psc, &scMatch);
}

static char *
CopyEscapedString(const char *pchOrig)
{

    char *sz, *pch;

    for (pch = sz = malloc(strlen(pchOrig) + 1); *pchOrig; pchOrig++) {
        if (*pchOrig == '\\') {
            if (pchOrig[1] == '\\') {
                *pch++ = '\\';
                pchOrig++;
            }
            continue;
        }

        if (isspace(*pchOrig) && *pchOrig != '\n') {
            *pch++ = ' ';
            continue;
        }

        *pch++ = *pchOrig;
    }

    *pch = 0;

    return sz;

}

static void
RestoreText(char *sz, char **ppch)
{

    if (!sz || !*sz)
        return;

    free(*ppch);

    *ppch = CopyEscapedString(sz);
}

static void
RestoreRules(xmovegameinfo * pmgi, const char *sz)
{
    char *pch;
    char *pchx;

    /* split string at colons */

    pch = g_strdup(sz);

    pchx = strtok(pch, ":");
    while (pchx) {

        if (!strcmp(pchx, "Crawford"))
            pmgi->fCrawford = TRUE;
        else if (!strcmp(pchx, "CrawfordGame"))
            pmgi->fCrawfordGame = TRUE;
        else if (!strcmp(pchx, "Jacoby"))
            pmgi->fJacoby = TRUE;
        else if (!strcmp(pchx, "Nackgammon"))
            pmgi->bgv = VARIATION_NACKGAMMON;
        else if (!strcmp(pchx, "Hypergammon1"))
            pmgi->bgv = VARIATION_HYPERGAMMON_1;
        else if (!strcmp(pchx, "Hypergammon2"))
            pmgi->bgv = VARIATION_HYPERGAMMON_2;
        else if (!strcmp(pchx, "Hypergammon3"))
            pmgi->bgv = VARIATION_HYPERGAMMON_3;
        else if (!strcmp(pchx, "NoCube"))
            pmgi->fCubeUse = FALSE;

        pchx = strtok(NULL, ":");

    }

    g_free(pch);
}



static void
RestoreRootNode(listOLD * pl)
{

    property *pp;
    moverecord *pmr;
    char *pch;
    int i;

    pmr = NewMoveRecord();

    pmr->mt = MOVE_GAMEINFO;

    pmr->g.i = 0;
    pmr->g.nMatch = 0;
    pmr->g.anScore[0] = 0;
    pmr->g.anScore[1] = 0;
    pmr->g.fCrawford = FALSE;
    pmr->g.fCrawfordGame = FALSE;
    pmr->g.fJacoby = FALSE;
    pmr->g.fWinner = -1;
    pmr->g.nPoints = 0;
    pmr->g.fResigned = FALSE;
    pmr->g.nAutoDoubles = 0;
    pmr->g.bgv = VARIATION_STANDARD;
    pmr->g.fCubeUse = TRUE;
    IniStatcontext(&pmr->g.sc);

    for (pl = pl->plNext; (pp = pl->p); pl = pl->plNext)
        if (pp->ach[0] == 'M' && pp->ach[1] == 'I')
            /* MI - Match info property */
            RestoreMI(pp->pl, pmr);
        else if (pp->ach[0] == 'P' && pp->ach[1] == 'B')
            /* PB - Black player property */
            CopyName(1, pp->pl->plNext->p);
        else if (pp->ach[0] == 'P' && pp->ach[1] == 'W')
            /* PW - White player property */
            CopyName(0, pp->pl->plNext->p);
        else if (pp->ach[0] == 'R' && pp->ach[1] == 'E') {
            /* RE - Result property */
            pch = pp->pl->plNext->p;

            pmr->g.fWinner = -1;

            if (toupper(*pch) == 'B')
                pmr->g.fWinner = 1;
            else if (toupper(*pch) == 'W')
                pmr->g.fWinner = 0;

            if (pmr->g.fWinner == -1)
                continue;

            if (*++pch != '+')
                continue;

            pmr->g.nPoints = (int)strtol(pch, &pch, 10);
            if (pmr->g.nPoints < 1)
                pmr->g.nPoints = 1;

            pmr->g.fResigned = toupper(*pch) == 'R';
        } else if (pp->ach[0] == 'R' && pp->ach[1] == 'U') {
            /* RU - Rules property */
            RestoreRules(&pmr->g, (const char *) pp->pl->plNext->p);
        } else if (pp->ach[0] == 'C' && pp->ach[1] == 'V') {
            /* CV - Cube value (i.e. automatic doubles) */
            for (i = 0; (1 << i) <= MAX_CUBE; i++)
                if (atoi(pp->pl->plNext->p) == 1 << i) {
                    pmr->g.nAutoDoubles = i;
                    break;
                }
        } else if (pp->ach[0] == 'G' && pp->ach[1] == 'S')
            /* GS - Game statistics */
            RestoreGS(pp->pl, &pmr->g.sc);
        else if (pp->ach[0] == 'W' && pp->ach[1] == 'R')
            /* WR - White rank */
            RestoreText(pp->pl->plNext->p, &mi.pchRating[0]);
        else if (pp->ach[0] == 'B' && pp->ach[1] == 'R')
            /* BR - Black rank */
            RestoreText(pp->pl->plNext->p, &mi.pchRating[1]);
        else if (pp->ach[0] == 'D' && pp->ach[1] == 'T') {
            /* DT - Date */
            int nYear, nMonth, nDay;

            if (pp->pl->plNext->p && sscanf(pp->pl->plNext->p, "%d-%d-%d", &nYear, &nMonth, &nDay) == 3) {
                mi.nYear = nYear;
                mi.nMonth = nMonth;
                mi.nDay = nDay;
            }
        } else if (pp->ach[0] == 'E' && pp->ach[1] == 'V')
            /* EV - Event */
            RestoreText(pp->pl->plNext->p, &mi.pchEvent);
        else if (pp->ach[0] == 'R' && pp->ach[1] == 'O')
            /* RO - Round */
            RestoreText(pp->pl->plNext->p, &mi.pchRound);
        else if (pp->ach[0] == 'P' && pp->ach[1] == 'C')
            /* PC - Place */
            RestoreText(pp->pl->plNext->p, &mi.pchPlace);
        else if (pp->ach[0] == 'A' && pp->ach[1] == 'N')
            /* AN - Annotator */
            RestoreText(pp->pl->plNext->p, &mi.pchAnnotator);
        else if (pp->ach[0] == 'G' && pp->ach[1] == 'C')
            /* GC - Game comment */
            RestoreText(pp->pl->plNext->p, &mi.pchComment);

    AddMoveRecord(pmr);
}

static int
Point(char ch, int f)
{

    if (ch == 'y')
        return 24;              /* bar */
    else if (ch <= 'x' && ch >= 'a')
        return f ? 'x' - ch : ch - 'a';
    else
        return -1;              /* off */
}

static void
RestoreRolloutScore(move * pm, const char *sz)
{

    char *pc = strstr(sz, "Score");

    pm->rScore = -99999;
    pm->rScore2 = -99999;

    if (!pc)
        return;
    pc += 6;
    pm->rScore = (float)g_ascii_strtod(pc, &pc);
    pm->rScore2 = (float)g_ascii_strtod(pc, &pc);
}

static void
RestoreRolloutTrials(unsigned int *piTrials, const char *sz)
{

    char *pc = strstr(sz, "Trials");

    *piTrials = 0;

    if (!pc)
        return;

    sscanf(pc, "Trials %ud", piTrials);

}

static void
RestoreRolloutOutput(float ar[NUM_ROLLOUT_OUTPUTS], const char *sz, const char *szKeyword)
{

    char *pc = strstr(sz, szKeyword);
    int i;

    for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
        ar[i] = 0.0;

    if (!pc)
        return;

    pc += strlen(szKeyword) + 1;

    for (i = 0; i < 7; i++)
        ar[i] = (float)g_ascii_strtod(pc, &pc);

}

static void
InitEvalContext(evalcontext * pec)
{

    pec->nPlies = 0;
    pec->fCubeful = FALSE;
    pec->fUsePrune = FALSE;
    pec->fDeterministic = FALSE;
    pec->rNoise = 0.0;

}


static void
RestoreEvalContext(evalcontext * pec, char *pc)
{

    int fUsePrune = 0;
    int red __attribute((unused)) = 0;
    int ver = CheckSGFVersion(&pc);

    InitEvalContext(pec);
    pec->nPlies = (unsigned int)strtol(pc, &pc, 10);
    if (*pc == 'C') {
        pec->fCubeful = TRUE;
        pc++;
    }
    if (ver < 3) {
        red = (int)strtol(pc, &pc, 10);
        pec->fDeterministic = (unsigned int)strtol(pc, &pc, 10);
        pec->rNoise = (float)g_ascii_strtod(pc, &pc);
    } else {
        pec->fDeterministic = (unsigned int)strtol(pc, &pc, 10);
        pec->rNoise = (float)g_ascii_strtod(pc, &pc);
        fUsePrune = (unsigned int)strtol(pc, &pc, 10);
        pec->fUsePrune = fUsePrune;
    }
}

static void
RestoreRolloutContextEvalContext(evalcontext * pec, const char *sz, const char *szKeyword)
{

    char *pc = strstr(sz, szKeyword);

    InitEvalContext(pec);

    if (!pc)
        return;

    pc = strchr(pc, ' ');

    if (!pc)
        return;

    RestoreEvalContext(pec, pc);

}


static void
RestoreRolloutRolloutContext(rolloutcontext * prc, const char *sz)
{

    char *pc = strstr(sz, "RC");
    char szTemp[1024];
    int fCubeful, fVarRedn, fInitial, fRotate, fTruncBearoff2, fTruncBearoffOS;

    fCubeful = FALSE;
    fVarRedn = FALSE;
    fInitial = FALSE;
    prc->nTruncate = 0;
    prc->nTrials = 0;
    prc->rngRollout = RNG_MERSENNE;
    prc->nSeed = 0;
    fRotate = TRUE;
    fTruncBearoff2 = FALSE;
    fTruncBearoffOS = FALSE;
    /* set usable, but ignored values for everything else */
    prc->fLateEvals = 0;
    prc->fStopOnSTD = 0;
    prc->nLate = 0;
    prc->nMinimumGames = 144;
    prc->rStdLimit = 0.01f;

    if (!pc)
        return;

    sscanf(pc, "RC %d %d %d %hu %u \"%[^\"]\" %lu %d %d %d",
           &fCubeful,
           &fVarRedn,
           &fInitial, &prc->nTruncate, &prc->nTrials, szTemp, &prc->nSeed, &fRotate, &fTruncBearoff2, &fTruncBearoffOS);

    prc->fCubeful = fCubeful;
    prc->fVarRedn = fVarRedn;
    prc->fRotate = fRotate;
    prc->fInitial = fInitial;
    prc->fTruncBearoff2 = fTruncBearoff2;
    prc->fTruncBearoffOS = fTruncBearoffOS;

    RestoreRolloutContextEvalContext(&prc->aecCube[0], sz, "cube0");
    RestoreRolloutContextEvalContext(&prc->aecCube[1], sz, "cube1");
    RestoreRolloutContextEvalContext(&prc->aecChequer[0], sz, "cheq0");
    RestoreRolloutContextEvalContext(&prc->aecChequer[1], sz, "cheq1");

}


static void
RestoreRollout(move * pm, const char *sz)
{
    unsigned int n;

    pm->esMove.et = EVAL_ROLLOUT;
    RestoreRolloutScore(pm, sz);
    RestoreRolloutTrials(&n, sz);
    RestoreRolloutOutput(pm->arEvalMove, sz, "Output");
    RestoreRolloutOutput(pm->arEvalStdDev, sz, "StdDev");
    RestoreRolloutRolloutContext(&pm->esMove.rc, sz);

}


static void
RestoreCubeRolloutOutput(float arOutput[], float arStdDev[], const char *sz, const char *szKeyword)
{

    char *pc = strstr(sz, szKeyword);

    memset(arOutput, 0, NUM_ROLLOUT_OUTPUTS * sizeof(float));
    memset(arStdDev, 0, NUM_ROLLOUT_OUTPUTS * sizeof(float));

    if (!pc)
        return;

    RestoreRolloutOutput(arOutput, pc, "Output");
    RestoreRolloutOutput(arStdDev, pc, "StdDev");

}


static void
RestoreCubeRollout(const char *sz,
                   float aarOutput[][NUM_ROLLOUT_OUTPUTS], float aarStdDev[][NUM_ROLLOUT_OUTPUTS], evalsetup * pes)
{

    RestoreRolloutTrials(&pes->rc.nGamesDone, sz);
    RestoreCubeRolloutOutput(aarOutput[0], aarStdDev[0], sz, "NoDouble");
    RestoreCubeRolloutOutput(aarOutput[1], aarStdDev[1], sz, "DoubleTake");
    RestoreRolloutRolloutContext(&pes->rc, sz);

}



static void
RestoreRolloutInternals(evalsetup * pes, const char *sz)
{

    char *pc;
    if ((pc = strstr(sz, "SK")) != 0)
        sscanf(pc, "SK %d", &pes->rc.nSkip);
}

static int
CheckSGFVersion(char **sz)
{

    int n;
    static int ver = 0;
    char *pch = strstr(*sz, "ver");
    if ((pch == 0) || (sscanf(pch, "ver %d", &n) != 1))
        return ver;
    ver = n;
    /* skip over version info */
    pch += 4;
    while (*pch != ' ') {
        ++pch;
    }
    *sz = pch;
    return ver;
}

static void
RestoreRolloutMoveFilter(const char *sz, char *name, movefilter mf[MAX_FILTER_PLIES][MAX_FILTER_PLIES], int nPlies)
{

    char *pc = strstr(sz, name);
    int i;

    if (pc == 0)
        return;

    pc += strlen(name);
    for (i = 0; i < nPlies; ++i) {
        mf[nPlies - 1][i].Accept = (int)strtol(pc, &pc, 10);
        mf[nPlies - 1][i].Extra = (int)strtol(pc, &pc, 10);
        mf[nPlies - 1][i].Threshold = (float)g_ascii_strtod(pc, &pc);
    }
}

static void
RestoreExtendedRolloutContext(rolloutcontext * prc, const char *sz)
{

    char *pc = strstr(sz, "RC");
    char szTemp[1024];
    int fCubeful, fVarRedn, fInitial, fRotate, fTruncBearoff2, fTruncBearoffOS;
    int fLateEvals, fDoTruncate;
    int i;


    if (!pc)
        return;

    if (sscanf(pc, "RC %d %d %d %d %d %d %hu %d %d %hu \"%[^\"]\" %lu",
               &fCubeful,
               &fVarRedn,
               &fInitial,
               &fRotate,
               &fLateEvals,
               &fDoTruncate,
               &prc->nTruncate, &fTruncBearoff2, &fTruncBearoffOS, &prc->nLate, szTemp, &prc->nSeed) != 12)
        return;

    prc->fCubeful = fCubeful;
    prc->fVarRedn = fVarRedn;
    prc->fInitial = fInitial;
    prc->fRotate = fRotate;
    prc->fLateEvals = fLateEvals;
    prc->fDoTruncate = fDoTruncate;
    prc->fTruncBearoff2 = fTruncBearoff2;
    prc->fTruncBearoffOS = fTruncBearoffOS;
    prc->rngRollout = RNG_MERSENNE;
    prc->nMinimumGames = 144;
    prc->rStdLimit = 0.01f;

    for (i = 0; i < 2; ++i) {
        sprintf(szTemp, "latecube%d ", i);
        RestoreRolloutContextEvalContext(&prc->aecCube[i], sz, szTemp + 4);
        RestoreRolloutContextEvalContext(&prc->aecCubeLate[i], sz, szTemp);
        sprintf(szTemp, "latecheq%d", i);
        RestoreRolloutContextEvalContext(&prc->aecChequer[i], sz, szTemp + 4);
        RestoreRolloutContextEvalContext(&prc->aecChequerLate[i], sz, szTemp);
        sprintf(szTemp, "latefilt%d ", i);
        if (prc->aecChequer[i].nPlies) {
            RestoreRolloutMoveFilter(sz, szTemp + 4, prc->aaamfChequer[i], prc->aecChequer[i].nPlies);
        }
        if (prc->aecChequerLate[i].nPlies) {
            RestoreRolloutMoveFilter(sz, szTemp, prc->aaamfLate[i], prc->aecChequerLate[i].nPlies);
        }
    }
    RestoreRolloutContextEvalContext(&prc->aecCubeTrunc, sz, "cubetrunc");
    RestoreRolloutContextEvalContext(&prc->aecChequerTrunc, sz, "cheqtrunc");


}

static void
RestoreExtendedCubeRollout(char *sz,
                           float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                           float aarStdDev[][NUM_ROLLOUT_OUTPUTS], evalsetup * pes)
{
    /* we assume new versions will still begin with Eq 4 floats
     * Trials int 
     */
    RestoreRolloutTrials(&pes->rc.nGamesDone, sz);
    RestoreCubeRolloutOutput(aarOutput[0], aarStdDev[0], sz, "NoDouble");
    RestoreCubeRolloutOutput(aarOutput[1], aarStdDev[1], sz, "DoubleTake");
    if (CheckSGFVersion(&sz)) {
        RestoreRolloutInternals(pes, sz);
        RestoreExtendedRolloutContext(&pes->rc, sz);
    }
}

static void
RestoreExtendedRollout(move * pm, char *sz)
{

    evalsetup *pes = &pm->esMove;

    /* we assume new versions will still begin with Score 2 floats
     * Trials int 
     */
    pes->et = EVAL_ROLLOUT;
    RestoreRolloutScore(pm, sz);
    RestoreRolloutTrials(&pes->rc.nGamesDone, sz);
    RestoreRolloutOutput(pm->arEvalMove, sz, "Output");
    RestoreRolloutOutput(pm->arEvalStdDev, sz, "StdDev");
    if (CheckSGFVersion(&sz)) {
        RestoreRolloutInternals(pes, sz);
        RestoreExtendedRolloutContext(&pes->rc, sz);
    }
}

static void
RestoreDoubleAnalysis(property * pp,
                      float aarOutput[][NUM_ROLLOUT_OUTPUTS], float aarStdDev[][NUM_ROLLOUT_OUTPUTS], evalsetup * pes)
{

    char *pch = pp->pl->plNext->p;
    int nReduced __attribute((unused));
    int fUsePrune = 0;
    /* leftovers from earlier formats */
    float arUnused[4] __attribute__ ((unused));
    int ver;
    int i, j;

    switch (*pch) {
    case 'E':
        /* EVAL_EVAL */
        ++pch;
        pes->et = EVAL_EVAL;
        ver = CheckSGFVersion(&pch);
        nReduced = 0;
        pes->ec.rNoise = 0.0f;
        memset(aarOutput[0], 0, NUM_ROLLOUT_OUTPUTS * sizeof(float));
        memset(aarOutput[1], 0, NUM_ROLLOUT_OUTPUTS * sizeof(float));
        aarOutput[0][OUTPUT_CUBEFUL_EQUITY] = -20000.0;
        aarOutput[1][OUTPUT_CUBEFUL_EQUITY] = -20000.0;

        if (ver < 2) {
            arUnused[0] = (float)g_ascii_strtod(pch, &pch);
            arUnused[1] = (float)g_ascii_strtod(pch, &pch);
            arUnused[2] = (float)g_ascii_strtod(pch, &pch);
            arUnused[3] = (float)g_ascii_strtod(pch, &pch);
            pes->ec.nPlies = (unsigned int)strtol(pch, &pch, 10);
            if (*pch == 'C') {
                pes->ec.fCubeful = TRUE;
                pch++;
            }
            nReduced = (int)strtol(pch, &pch, 10);
            pes->ec.fDeterministic = (unsigned int)strtol(pch, &pch, 10);
            pes->ec.rNoise = (float)g_ascii_strtod(pch, &pch);
        } else if (ver == 2) {
            pes->ec.nPlies = (unsigned int)strtol(pch, &pch, 10);
            if (*pch == 'C') {
                pes->ec.fCubeful = TRUE;
                pch++;
            }
            nReduced = (int)strtol(pch, &pch, 10);
            pes->ec.fDeterministic = (unsigned int)strtol(pch, &pch, 10);
            pes->ec.rNoise = (float)g_ascii_strtod(pch, &pch);
            fUsePrune = (int)strtol(pch, &pch, 10);
        } else {
            pes->ec.nPlies = (unsigned int)strtol(pch, &pch, 10);
            if (*pch == 'C') {
                pes->ec.fCubeful = TRUE;
                pch++;
            }
            pes->ec.fDeterministic = (unsigned int)strtol(pch, &pch, 10);
            pes->ec.rNoise = (float)g_ascii_strtod(pch, &pch);
            fUsePrune = (int)strtol(pch, &pch, 10);
        }
        pes->ec.fUsePrune = fUsePrune;

        for (i = 0; i < 2; i++)
            for (j = 0; j < 7; j++)
                aarOutput[i][j] = (float)g_ascii_strtod(pch, &pch);
        break;

    case 'R':

        pes->et = EVAL_ROLLOUT;
        RestoreCubeRollout(pch + 1, aarOutput, aarStdDev, pes);
        break;

    case 'X':

        pes->et = EVAL_ROLLOUT;
        RestoreExtendedCubeRollout(pch + 1, aarOutput, aarStdDev, pes);
        break;

    default:
        break;
    }
}


static void
RestoreMarkedMoves(property * pp, movelist * pml)
{
    int val;
    char *pch = pp->pl->plNext->p;
    while ((val = (int)strtol(pch, &pch, 10))) {
        if (val > (int) pml->cMoves || val < 1)
            continue;
        pml->amMoves[val - 1].cmark = CMARK_ROLLOUT;
    }
}

static void
RestoreMoveAnalysis(property * pp, int fPlayer,
                    movelist * pml, unsigned int *piMove, evalsetup * pesChequer, const matchstate * pms)
{
    listOLD *pl = pp->pl->plNext;
    const char *pc;
    char *pch;
    char ch;
    move *pm;
    int i;
    int fDeterministic __attribute((unused));
    int nReduced __attribute((unused));
    int fUsePrune = 0;
    TanBoard anBoardMove;
    int ver;
    *piMove = atoi(pl->p);

    for (pml->cMoves = 0, pl = pl->plNext; pl->p; pl = pl->plNext)
        pml->cMoves++;

    if (pml->cMoves == 0) {
        g_assert_not_reached();
        return;
    }

    /* FIXME we could work these out, but it hardly seems worth it */
    pml->cMaxMoves = pml->cMaxPips = pml->iMoveBest = 0;
    pml->rBestScore = 0;

    pm = pml->amMoves = calloc(pml->cMoves, sizeof(move));

    pesChequer->et = EVAL_NONE;

    for (pl = pp->pl->plNext->plNext; pl->p; pl = pl->plNext, pm++) {
        pc = pl->p;

        /* FIXME we could work these out, but it hardly seems worth it */
        pm->cMoves = pm->cPips = 0;
        pm->rScore2 = 0;

        for (i = 0; i < 4 && pc[0] && pc[1] && pc[0] != ' '; pc += 2, i++) {
            pm->anMove[i << 1] = Point(pc[0], fPlayer);
            pm->anMove[(i << 1) | 1] = Point(pc[1], fPlayer);
        }

        if (i < 4)
            pm->anMove[i << 1] = -1;

        /* restore auch */

        memcpy(anBoardMove, pms->anBoard, sizeof(anBoardMove));
        ApplyMove(anBoardMove, pm->anMove, FALSE);
        PositionKey((ConstTanBoard) anBoardMove, &pm->key);

        pch = (char *) pc;
        while (isspace(ch = *pch++)) {
        };
        pch++;

        switch (ch) {
        case 'E':
            ver = CheckSGFVersion(&pch);
            /* EVAL_EVAL */
            pm->esMove.et = EVAL_EVAL;
            nReduced = 0;
            fDeterministic = 0;
            for (i = 0; i < 5; i++)
                pm->arEvalMove[i] = (float)g_ascii_strtod(pch, &pch);
            pm->rScore = (float)g_ascii_strtod(pch, &pch);
            if (ver < 2) {
                pm->esMove.ec.nPlies = (unsigned int)strtol(pch, &pch, 10);
                if (*pch == 'C') {
                    pm->esMove.ec.fCubeful = TRUE;
                    pch++;
                }
                nReduced = (int)strtol(pch, &pch, 10);
                pm->esMove.ec.fDeterministic = (unsigned int)strtol(pch, &pch, 10);
                pm->esMove.ec.rNoise = (float)g_ascii_strtod(pch, &pch);
            } else if (ver == 2) {
                pm->esMove.ec.nPlies = (unsigned int)strtol(pch, &pch, 10);
                if (*pch == 'C') {
                    pm->esMove.ec.fCubeful = TRUE;
                    pch++;
                }
                nReduced = (int)strtol(pch, &pch, 10);
                pm->esMove.ec.fDeterministic = (unsigned int)strtol(pch, &pch, 10);
                pm->esMove.ec.rNoise = (float)g_ascii_strtod(pch, &pch);
                fUsePrune = (int)strtol(pch, &pch, 10);
            } else {
                pm->esMove.ec.nPlies = (unsigned int)strtol(pch, &pch, 10);
                if (*pch == 'C') {
                    pm->esMove.ec.fCubeful = TRUE;
                    pch++;
                }
                /* FIXME: reduced shouldn't be saved/loaded */
                nReduced = (int)strtol(pch, &pch, 10);
                pm->esMove.ec.fDeterministic = (unsigned int)strtol(pch, &pch, 10);
                pm->esMove.ec.rNoise = (float)g_ascii_strtod(pch, &pch);
                fUsePrune = (int)strtol(pch, &pch, 10);
            }
            pm->esMove.ec.fUsePrune = fUsePrune;
            break;

        case 'R':
            RestoreRollout(pm, pch);
            break;

        case 'X':
            RestoreExtendedRollout(pm, pch);
            break;

        default:
            /* FIXME */
            break;
        }

        /* save "largest" evalsetup */

        if (cmp_evalsetup(pesChequer, &pm->esMove) < 0)
            memcpy(pesChequer, &pm->esMove, sizeof(evalsetup));


    }
}

static void
PointList(listOLD * pl, int an[])
{

    int i;
    char *pch, ch0, ch1;

    for (i = 0; i < 25; i++)
        an[i] = 0;

    for (; pl->p; pl = pl->plNext) {
        pch = pl->p;

        if (strchr(pch, ':')) {
            ch0 = ch1 = 0;
            sscanf(pch, "%c:%c", &ch0, &ch1);
            if (ch0 >= 'a' && ch1 <= 'y' && ch0 < ch1)
                for (i = ch0 - 'a'; i <= ch1 - 'a'; i++)
                    an[i]++;
        } else if (*pch >= 'a' && *pch <= 'y')
            an[*pch - 'a']++;
    }
}

static void
RestoreNode(listOLD * pl)
{

    property *pp, *ppDA = NULL, *ppA = NULL, *ppC = NULL, *ppMR = NULL, *ppCR = NULL;
    moverecord *pmr = NULL;
    char *pch;
    int i, fPlayer = 0, fSetBoard = FALSE, an[25];
    skilltype ast[2] = { SKILL_NONE, SKILL_NONE };
    lucktype lt = LUCK_NONE;
    float rLuck = ERR_VAL;


    for (pl = pl->plNext; (pp = pl->p); pl = pl->plNext) {
        if (pp->ach[1] == 0 && (pp->ach[0] == 'B' || pp->ach[0] == 'W')) {
            /* B or W - Move property. */

            if (pmr)
                /* Duplicate move -- ignore. */
                continue;

            pch = pp->pl->plNext->p;
            fPlayer = pp->ach[0] == 'B';

            if (!strcmp(pch, "double")) {

                pmr = NewMoveRecord();

                pmr->mt = MOVE_DOUBLE;
                pmr->fPlayer = fPlayer;
                LinkToDouble(pmr);

            } else if (!strcmp(pch, "take")) {

                pmr = NewMoveRecord();

                pmr->mt = MOVE_TAKE;
                pmr->fPlayer = fPlayer;
                LinkToDouble(pmr);

            } else if (!strcmp(pch, "drop")) {

                pmr = NewMoveRecord();

                pmr->mt = MOVE_DROP;
                pmr->fPlayer = fPlayer;
                LinkToDouble(pmr);

            } else {

                pmr = NewMoveRecord();

                pmr->mt = MOVE_NORMAL;
                pmr->fPlayer = fPlayer;

                pmr->anDice[1] = 0;

                for (i = 0; i < 2 && *pch; pch++, i++)
                    pmr->anDice[i] = *pch - '0';

                for (i = 0; i < 4 && pch[0] && pch[1]; pch += 2, i++) {
                    pmr->n.anMove[i << 1] = Point(pch[0], pmr->fPlayer);
                    pmr->n.anMove[(i << 1) | 1] = Point(pch[1], pmr->fPlayer);
                }

                if (i < 4)
                    pmr->n.anMove[i << 1] = -1;

                if (pmr->anDice[0] < 1 || pmr->anDice[0] > 6 || pmr->anDice[1] < 1 || pmr->anDice[1] > 6) {
                    /* illegal roll -- ignore */
                    free(pmr);
                    pmr = NULL;
                }
            }
        } else if (pp->ach[0] == 'A' && pp->ach[1] == 'E') {
            fSetBoard = TRUE;
            PointList(pp->pl->plNext, an);
            for (i = 0; i < 25; i++)
                if (an[i]) {
                    ms.anBoard[0][i == 24 ? i : 23 - i] = 0;
                    ms.anBoard[1][i] = 0;
                }
        } else if (pp->ach[0] == 'A' && pp->ach[1] == 'B') {
            fSetBoard = TRUE;
            PointList(pp->pl->plNext, an);
            for (i = 0; i < 25; i++)
                ms.anBoard[0][i == 24 ? i : 23 - i] += an[i];
        } else if (pp->ach[0] == 'A' && pp->ach[1] == 'W') {
            fSetBoard = TRUE;
            PointList(pp->pl->plNext, an);
            for (i = 0; i < 25; i++)
                ms.anBoard[1][i] += an[i];
        } else if (pp->ach[0] == 'P' && pp->ach[1] == 'L') {
            int fTurnNew = *((char *) pp->pl->plNext->p) == 'B';
            if (ms.fMove != fTurnNew)
                SwapSides(ms.anBoard);
            ms.fTurn = ms.fMove = fTurnNew;
        } else if (pp->ach[0] == 'C' && pp->ach[1] == 'V') {
            for (i = 1; i <= MAX_CUBE; i <<= 1)
                if (atoi(pp->pl->plNext->p) == i) {
                    pmr = NewMoveRecord();

                    pmr->mt = MOVE_SETCUBEVAL;
                    pmr->scv.nCube = i;
                    AddMoveRecord(pmr);
                    pmr = NULL;
                    break;
                }
        } else if (pp->ach[0] == 'C' && pp->ach[1] == 'P') {
            pmr = NewMoveRecord();

            pmr->mt = MOVE_SETCUBEPOS;
            switch (*((char *) pp->pl->plNext->p)) {
            case 'c':
                pmr->scp.fCubeOwner = -1;
                break;
            case 'b':
                pmr->scp.fCubeOwner = 1;
                break;
            case 'w':
                pmr->scp.fCubeOwner = 0;
                break;
            default:
                free(pmr);
                pmr = NULL;
            }

            if (pmr) {
                AddMoveRecord(pmr);
                pmr = NULL;
            }
        } else if (pp->ach[0] == 'D' && pp->ach[1] == 'I') {
            char ach[2];

            sscanf(pp->pl->plNext->p, "%2c", ach);

            if (ach[0] >= '1' && ach[0] <= '6' && ach[1] >= '1' && ach[1] <= '6') {
                pmr = NewMoveRecord();

                pmr->mt = MOVE_SETDICE;
                pmr->fPlayer = ms.fMove;
                pmr->anDice[0] = ach[0] - '0';
                pmr->anDice[1] = ach[1] - '0';
            }
        } else if (pp->ach[0] == 'D' && pp->ach[1] == 'A')
            /* double analysis */
            ppDA = pp;
        else if (pp->ach[0] == 'A' && !pp->ach[1])
            /* move analysis */
            ppA = pp;
        else if (pp->ach[0] == 'M' && pp->ach[1] == 'R')
            /* marked moves */
            ppMR = pp;
        else if (pp->ach[0] == 'C' && pp->ach[1] == 'R')
            /* marked moves */
            ppCR = pp;
        else if (pp->ach[0] == 'C' && !pp->ach[1])
            /* comment */
            ppC = pp;
        else if (pp->ach[0] == 'B' && pp->ach[1] == 'M')
            ast[0]
                = *((char *) pp->pl->plNext->p) == '2' ? SKILL_VERYBAD : SKILL_BAD;
        else if (pp->ach[0] == 'D' && pp->ach[1] == 'O')
            ast[0] = SKILL_DOUBTFUL;
        else if (pp->ach[0] == 'B' && pp->ach[1] == 'C')
            ast[1]
                = *((char *) pp->pl->plNext->p) == '2' ? SKILL_VERYBAD : SKILL_BAD;
        else if (pp->ach[0] == 'D' && pp->ach[1] == 'C')
            ast[1] = SKILL_DOUBTFUL;
        else if (pp->ach[0] == 'L' && pp->ach[1] == 'U')
            rLuck = (float)g_ascii_strtod(pp->pl->plNext->p, NULL);
        else if (pp->ach[0] == 'G' && pp->ach[1] == 'B')
            /* good for black */
            lt = *((char *) pp->pl->plNext->p) == '2' ? LUCK_VERYGOOD : LUCK_GOOD;
        else if (pp->ach[0] == 'G' && pp->ach[1] == 'W')
            /* good for white */
            lt = *((char *) pp->pl->plNext->p) == '2' ? LUCK_VERYBAD : LUCK_BAD;
    }

    if (fSetBoard && !pmr) {
        pmr = NewMoveRecord();

        pmr->mt = MOVE_SETBOARD;
        ClosestLegalPosition(ms.anBoard);
        PositionKey(msBoard(), &pmr->sb.key);
    }

    if (pmr && ppC)
        pmr->sz = CopyEscapedString(ppC->pl->plNext->p);

    if (pmr) {

        FixMatchState(&ms, pmr);

        switch (pmr->mt) {
        case MOVE_NORMAL:
            if (ppDA)
                RestoreDoubleAnalysis(ppDA,
                                      pmr->CubeDecPtr->aarOutput,
                                      pmr->CubeDecPtr->aarStdDev, &pmr->CubeDecPtr->esDouble);
            if (ppCR)
                pmr->CubeDecPtr->cmark = CMARK_ROLLOUT;
            if (ppA)
                RestoreMoveAnalysis(ppA, pmr->fPlayer, &pmr->ml, &pmr->n.iMove, &pmr->esChequer, &ms);
            if (ppMR)
                RestoreMarkedMoves(ppMR, &pmr->ml);
            /* FIXME: separate st's */
            pmr->n.stMove = ast[0];
            pmr->stCube = ast[1];
            pmr->lt = fPlayer ? lt : LUCK_VERYGOOD - lt;
            pmr->rLuck = rLuck;
            break;

        case MOVE_DOUBLE:
        case MOVE_TAKE:
        case MOVE_DROP:
            if (ppDA)
                RestoreDoubleAnalysis(ppDA,
                                      pmr->CubeDecPtr->aarOutput,
                                      pmr->CubeDecPtr->aarStdDev, &pmr->CubeDecPtr->esDouble);
            if (ppCR)
                pmr->CubeDecPtr->cmark = CMARK_ROLLOUT;
            pmr->stCube = ast[0];
            break;

        case MOVE_SETDICE:
            pmr->lt = lt;
            pmr->rLuck = rLuck;
            break;

        default:
            /* FIXME allow comments for all movetypes */
            break;
        }

        AddMoveRecord(pmr);
    }
}

static void
RestoreSequence(listOLD * pl, int fRoot)
{

    pl = pl->plNext;
    if (fRoot)
        RestoreRootNode(pl->p);
    else
        RestoreNode(pl->p);

    while (pl = pl->plNext, pl->p)
        RestoreNode(pl->p);
}

static void
RestoreTree(listOLD * pl, int fRoot)
{

    pl = pl->plNext;
    RestoreSequence(pl->p, fRoot);

    pl = pl->plNext;
    if (pl->p)
        RestoreTree(pl->p, FALSE);

    /* FIXME restore other variations, once we can handle them */
}

static void
RestoreGame(listOLD * pl)
{

    moverecord *pmr, *pmrResign;

    InitBoard(ms.anBoard, ms.bgv);

    /* FIXME should anything be done with the current game? */

    ClearMoveRecord();

    ListInsert(&lMatch, plGame);

    ms.anDice[0] = ms.anDice[1] = 0;
    ms.fResigned = ms.fDoubled = FALSE;
    ms.nCube = 1;
    ms.fTurn = ms.fMove = ms.fCubeOwner = -1;
    ms.gs = GAME_NONE;

    RestoreTree(pl, TRUE);

    pmr = plGame->plNext->p;
    g_assert(pmr->mt == MOVE_GAMEINFO);

    AddGame(pmr);

    if (pmr->g.fResigned) {
        /* setting fTurn = fMove = -1 results in the board being
         * inverted when shown. /jth 2003-10-12 
         * ms.fTurn = ms.fMove = -1; */

        pmrResign = NewMoveRecord();

        pmrResign->mt = MOVE_RESIGN;

        pmrResign->fPlayer = !pmr->g.fWinner;
        pmrResign->r.nResigned = pmr->g.nPoints / ms.nCube;

        if (pmrResign->r.nResigned < 1)
            pmrResign->r.nResigned = 1;
        else if (pmrResign->r.nResigned > 3)
            pmrResign->r.nResigned = 3;

        AddMoveRecord(pmrResign);
    }

}

extern void
CommandLoadGame(char *sz)
{

    listOLD *pl;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a file to load from (see `help load " "game')."));
        return;
    }

    if ((pl = LoadCollection(sz))) {
        if (!get_input_discard())
            return;
#if USE_GTK
        if (fX) {               /* Clear record to avoid ugly updates */
            GTKClearMoveRecord();
            GTKFreeze();
        }
#endif

        FreeMatch();
        ClearMatch();

        /* FIXME if pl contains multiple games, ask which one to load */

        RestoreGame(pl->plNext->p);

        FreeGameTreeSeq(pl);

        UpdateSettings();

#if USE_GTK
        if (fX) {
            GTKThaw();
            GTKSet(ap);
        }

        setDefaultFileName(sz);

#endif

        /* FIXME : don't do this if we just loaded a single position */
        if (fGotoFirstGame)
            CommandFirstGame(NULL);

    }
}


extern void
CommandLoadPosition(char *sz)
{

    listOLD *pl;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a file to load from (see `help load " "position')."));
        return;
    }

    if ((pl = LoadCollection(sz))) {
        if (!get_input_discard())
            return;
#if USE_GTK
        if (fX) {               /* Clear record to avoid ugly updates */
            GTKClearMoveRecord();
            GTKFreeze();
        }
#endif

        FreeMatch();
        ClearMatch();

        /* FIXME if pl contains multiple games, ask which one to load */

        RestoreGame(pl->plNext->p);

        FreeGameTreeSeq(pl);

        UpdateSettings();

#if USE_GTK
        if (fX) {
            GTKThaw();
            GTKSet(ap);
        }

        setDefaultFileName(sz);

#endif

    }
}


extern void
CommandLoadMatch(char *sz)
{

    listOLD *pl;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a file to load from (see `help load " "match')."));
        return;
    }

    if ((pl = LoadCollection(sz))) {
        /* FIXME make sure the root nodes have MI properties; if not,
         * we're loading a session. */
        if (!get_input_discard())
            return;
#if USE_GTK
        if (fX) {               /* Clear record to avoid ugly updates */
            GTKClearMoveRecord();
            GTKFreeze();
        }
#endif

        FreeMatch();
        ClearMatch();

        for (pl = pl->plNext; pl->p; pl = pl->plNext) {
            RestoreGame(pl->p);
        }

        FreeGameTreeSeq(pl);

        UpdateSettings();

#if USE_GTK
        if (fX) {
            GTKThaw();
            GTKSet(ap);
        }
#endif

        setDefaultFileName(sz);

        /* FIXME : don't do this if we just loaded a single position */
        if (fGotoFirstGame)
            CommandFirstGame(NULL);

    }
}

static void
WriteEscapedString(FILE * pf, char *pch, int fEscapeColons)
{

    char *sz, *pc;

    sz = (char *) malloc(2 * strlen(pch) + 1);

    for (pc = sz; *pch; pch++)
        switch (*pch) {
        case '\\':
            *pc++ = '\\';
            *pc++ = '\\';
            break;
        case ':':
            if (fEscapeColons)
                *pc++ = '\\';
            *pc++ = ':';
            break;
        case ']':
            *pc++ = '\\';
            *pc++ = ']';
            break;
        default:
            *pc++ = *pch;
            break;
        }

    *pc++ = 0;

    fputs(sz, pf);
    free(sz);
}

static void
WriteEvalContext(FILE * pf, const evalcontext * pec)
{
    gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];
    g_ascii_formatd(buffer, sizeof(buffer), "%.6f", pec->rNoise);
    fprintf(pf, "ver %d %u%s %u %s %u",
            SGF_FORMAT_VER, pec->nPlies, pec->fCubeful ? "C" : "", pec->fDeterministic, buffer, pec->fUsePrune);
}

static void
WriteMoveFilters(FILE * pf, movefilter mf[MAX_FILTER_PLIES][MAX_FILTER_PLIES], int nPlies)
{
    int i;
    gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

    for (i = 0; i < nPlies; ++i) {
        g_ascii_formatd(buffer, sizeof(buffer), "%.5f", mf[nPlies - 1][i].Threshold);
        fprintf(pf, "%d %d %s ", mf[nPlies - 1][i].Accept, mf[nPlies - 1][i].Extra, buffer);
    }
}

static void
WriteRolloutContext(FILE * pf, rolloutcontext * prc)
{

    int i;

    fprintf(pf, "RC %u %u %u %u %u %u %d %u %u %d \"%s\" %lu ",
            prc->fCubeful,
            prc->fVarRedn,
            prc->fInitial,
            prc->fRotate,
            prc->fLateEvals,
            prc->fDoTruncate,
            prc->nTruncate, prc->fTruncBearoff2, prc->fTruncBearoffOS, prc->nLate, aszRNG[prc->rngRollout], prc->nSeed);

    for (i = 0; i < 2; i++) {
        fprintf(pf, " cube%d ", i);
        WriteEvalContext(pf, &prc->aecCube[i]);
        fputc(' ', pf);

        fprintf(pf, " cheq%d ", i);
        WriteEvalContext(pf, &prc->aecChequer[i]);
        if (prc->aecChequer[i].nPlies) {
            fprintf(pf, " filt%d ", i);
            WriteMoveFilters(pf, prc->aaamfChequer[i], prc->aecChequer[i].nPlies);
        }
    }

    for (i = 0; i < 2; i++) {
        fprintf(pf, " latecube%d ", i);
        WriteEvalContext(pf, &prc->aecCubeLate[i]);
        fputc(' ', pf);

        fprintf(pf, " latecheq%d ", i);
        WriteEvalContext(pf, &prc->aecChequerLate[i]);
        if (prc->aecChequerLate[i].nPlies) {
            fprintf(pf, " latefilt%d ", i);
            WriteMoveFilters(pf, prc->aaamfLate[i], prc->aecChequerLate[i].nPlies);
        }
    }

    fprintf(pf, " cubetrunc ");
    WriteEvalContext(pf, &prc->aecCubeTrunc);
    fprintf(pf, " cheqtrunc ");
    WriteEvalContext(pf, &prc->aecChequerTrunc);

}

static void
WriteRolloutAnalysis(FILE * pf, int fIsMove,
                     float rScore, float rScore2,
                     float aarOutput0[NUM_ROLLOUT_OUTPUTS],
                     float aarOutput1[NUM_ROLLOUT_OUTPUTS],
                     float aarStdDev0[NUM_ROLLOUT_OUTPUTS], float aarStdDev1[NUM_ROLLOUT_OUTPUTS], evalsetup * pes)
{

    int i;
    gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

    /* identify what version we're writing to avoid future problems 
     * the version is defined in eval.h
     */
    if (fIsMove) {
        fprintf(pf, "X ver %d Score ", SGF_FORMAT_VER);
        g_ascii_formatd(buffer, sizeof(buffer), "%.10g", rScore);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.10g", rScore2);
        fprintf(pf, "%s ", buffer);
    } else {
        fprintf(pf, "X ver %d Eq ", SGF_FORMAT_VER);
    }

    fprintf(pf, "Trials %u ", pes->rc.nGamesDone);
    if (!fIsMove)
        fprintf(pf, "NoDouble ");

    fprintf(pf, "Output ");
    for (i = 0; i < 7; ++i) {
        g_ascii_formatd(buffer, sizeof(buffer), "%.10g", aarOutput0[i]);
        fprintf(pf, "%s ", buffer);
    }

    fprintf(pf, "StdDev ");
    for (i = 0; i < 7; ++i) {
        g_ascii_formatd(buffer, sizeof(buffer), "%.10g", aarStdDev0[i]);
        fprintf(pf, "%s ", buffer);
    }

    if (!fIsMove) {
        fprintf(pf, "DoubleTake Output ");
        for (i = 0; i < 7; ++i) {
            g_ascii_formatd(buffer, sizeof(buffer), "%.10g", aarOutput1[i]);
            fprintf(pf, "%s ", buffer);
        }
        fprintf(pf, "StdDev ");
        for (i = 0; i < 7; ++i) {
            g_ascii_formatd(buffer, sizeof(buffer), "%.10g", aarStdDev1[i]);
            fprintf(pf, "%s ", buffer);
        }
    }

    fprintf(pf, "SK %d ", pes->rc.nSkip);
    WriteRolloutContext(pf, &pes->rc);
}

static void
WriteDoubleAnalysis(FILE * pf,
                    float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                    float aarStdDev[][NUM_ROLLOUT_OUTPUTS], evalsetup * pes, int cmark)
{

    gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];
    int i, j;
    /* FIXME: removing write of 0 0 0 0 will break existing SGF files */

    fputs("DA[", pf);

    switch (pes->et) {
    case EVAL_EVAL:
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", pes->ec.rNoise);
        fprintf(pf, "E ver %d %u%s %u %s %u",
                SGF_FORMAT_VER,
                pes->ec.nPlies, pes->ec.fCubeful ? "C" : "", pes->ec.fDeterministic, buffer, pes->ec.fUsePrune);

        for (i = 0; i < 2; i++) {
            for (j = 0; j < 7; j++) {
                g_ascii_formatd(buffer, sizeof(buffer), "%.6f", aarOutput[i][j]);
                fprintf(pf, " %s", buffer);
            }
        }

        break;

    case EVAL_ROLLOUT:

        WriteRolloutAnalysis(pf, 0, 0.0f, 0.0f, aarOutput[0], aarOutput[1], aarStdDev[0], aarStdDev[1], pes);
        break;

    default:
        g_assert_not_reached();
    }

    fputc(']', pf);
    if (cmark)
        fputs("CR[]", pf);

}

static void
WriteMove(FILE * pf, int fPlayer, int anMove[])
{

    int i;

    for (i = 0; i < 8; i++)
        switch (anMove[i]) {
        case 24:               /* bar */
            putc('y', pf);
            break;
        case -1:               /* off */
            if (!(i & 1))
                return;
            putc('z', pf);
            break;
        default:
            putc(fPlayer ? 'x' - anMove[i] : 'a' + anMove[i], pf);
        }
}


static void
WriteMoveAnalysis(FILE * pf, int fPlayer, movelist * pml, unsigned int iMove)
{

    unsigned int i;
    gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

    fprintf(pf, "A[%u]", iMove);

    for (i = 0; i < pml->cMoves; i++) {
        fputc('[', pf);
        WriteMove(pf, fPlayer, pml->amMoves[i].anMove);
        fputc(' ', pf);

        switch (pml->amMoves[i].esMove.et) {
        case EVAL_NONE:
            break;

        case EVAL_EVAL:
            g_ascii_formatd(buffer, sizeof(buffer), "%.6f", pml->amMoves[i].arEvalMove[0]);
            fprintf(pf, "E ver %d %s ", SGF_FORMAT_VER, buffer);
            g_ascii_formatd(buffer, sizeof(buffer), "%.6f", pml->amMoves[i].arEvalMove[1]);
            fprintf(pf, "%s ", buffer);
            g_ascii_formatd(buffer, sizeof(buffer), "%.6f", pml->amMoves[i].arEvalMove[2]);
            fprintf(pf, "%s ", buffer);
            g_ascii_formatd(buffer, sizeof(buffer), "%.6f", pml->amMoves[i].arEvalMove[3]);
            fprintf(pf, "%s ", buffer);
            g_ascii_formatd(buffer, sizeof(buffer), "%.6f", pml->amMoves[i].arEvalMove[4]);
            fprintf(pf, "%s ", buffer);
            g_ascii_formatd(buffer, sizeof(buffer), "%.6f", pml->amMoves[i].rScore);
            fprintf(pf, "%s ", buffer);
            g_ascii_formatd(buffer, sizeof(buffer), "%.6f", pml->amMoves[i].esMove.ec.rNoise);
            fprintf(pf, "%u%s %d %u %s %u",
                    pml->amMoves[i].esMove.ec.nPlies,
                    pml->amMoves[i].esMove.ec.fCubeful ? "C" : "",
                    0, pml->amMoves[i].esMove.ec.fDeterministic, buffer, pml->amMoves[i].esMove.ec.fUsePrune);
            break;

        case EVAL_ROLLOUT:
            WriteRolloutAnalysis(pf, 1, pml->amMoves[i].rScore,
                                 pml->amMoves[i].rScore2,
                                 pml->amMoves[i].arEvalMove, 0,
                                 pml->amMoves[i].arEvalStdDev, 0, &pml->amMoves[i].esMove);
            break;



        default:
            g_assert_not_reached();
        }

        fputc(']', pf);
    }
    fprintf(pf, "MR[");
    for (i = 0; i < pml->cMoves; i++) {
        if (pml->amMoves[i].cmark)
            fprintf(pf, "%d ", i + 1);
    }
    fputc(']', pf);
}

static void
WriteLuck(FILE * pf, int fPlayer, float rLuck, lucktype lt)
{
    gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];
    if (rLuck != ERR_VAL) {
        g_ascii_formatd(buffer, sizeof(buffer), "%.5f", rLuck);
        fprintf(pf, "LU[%s]", buffer);
    }

    switch (lt) {
    case LUCK_VERYBAD:
        fprintf(pf, "G%c[2]", fPlayer ? 'W' : 'B');
        break;

    case LUCK_BAD:
        fprintf(pf, "G%c[1]", fPlayer ? 'W' : 'B');
        break;

    case LUCK_NONE:
        break;

    case LUCK_GOOD:
        fprintf(pf, "G%c[1]", fPlayer ? 'B' : 'W');
        break;

    case LUCK_VERYGOOD:
        fprintf(pf, "G%c[2]", fPlayer ? 'B' : 'W');
        break;
    }
}

static void
WriteSkill(FILE * pf, const skilltype st)
{

    switch (st) {
    case SKILL_VERYBAD:
        fputs("BM[2]", pf);
        break;

    case SKILL_BAD:
        fputs("BM[1]", pf);
        break;

    case SKILL_DOUBTFUL:
        fputs("DO[]", pf);
        break;

    default:
        break;
    }
}

static void
WriteSkillCube(FILE * pf, const skilltype st)
{

    switch (st) {
    case SKILL_VERYBAD:
        fputs("BC[2]", pf);
        break;

    case SKILL_BAD:
        fputs("BC[1]", pf);
        break;

    case SKILL_DOUBTFUL:
        fputs("DC[]", pf);
        break;

    default:
        break;

    }
}

static void
WriteStatContext(FILE * pf, statcontext * psc)
{

    lucktype lt;
    skilltype st;
    gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

    fputs("GS", pf);

    if (psc->fMoves) {
        fprintf(pf, "[M:%d %d %d %d ", psc->anUnforcedMoves[0],
                psc->anUnforcedMoves[1], psc->anTotalMoves[0], psc->anTotalMoves[1]);
        for (st = SKILL_VERYBAD; st <= SKILL_NONE; st++)
            fprintf(pf, "%d %d ", psc->anMoves[0][st], psc->anMoves[1][st]);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorCheckerplay[0][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorCheckerplay[0][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorCheckerplay[1][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorCheckerplay[1][1]);
        fprintf(pf, "%s]", buffer);
    }

    if (psc->fCube) {
        fprintf(pf, "[C:%d %d %d %d %d %d %d %d ", psc->anTotalCube[0],
                psc->anTotalCube[1], psc->anDouble[0], psc->anDouble[1],
                psc->anTake[0], psc->anTake[1], psc->anPass[0], psc->anPass[1]);
        fprintf(pf, "%d %d %d %d %d %d %d %d %d %d %d %d ",
                psc->anCubeMissedDoubleDP[0],
                psc->anCubeMissedDoubleDP[1],
                psc->anCubeMissedDoubleTG[0],
                psc->anCubeMissedDoubleTG[1],
                psc->anCubeWrongDoubleDP[0], psc->anCubeWrongDoubleDP[1],
                psc->anCubeWrongDoubleTG[0], psc->anCubeWrongDoubleTG[1],
                psc->anCubeWrongTake[0], psc->anCubeWrongTake[1], psc->anCubeWrongPass[0], psc->anCubeWrongPass[1]);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorMissedDoubleDP[0][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorMissedDoubleDP[0][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorMissedDoubleTG[0][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorMissedDoubleTG[0][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongDoubleDP[0][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongDoubleDP[0][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongDoubleTG[0][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongDoubleTG[0][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongTake[0][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongTake[0][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongPass[0][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongPass[0][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorMissedDoubleDP[1][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorMissedDoubleDP[1][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorMissedDoubleTG[1][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorMissedDoubleTG[1][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongDoubleDP[1][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongDoubleDP[1][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongDoubleTG[1][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongDoubleTG[1][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongTake[1][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongTake[1][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongPass[1][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arErrorWrongPass[1][1]);
        fprintf(pf, "%s]", buffer);
    }

    if (psc->fDice) {
        fputs("[D:", pf);
        for (lt = LUCK_VERYBAD; lt <= LUCK_VERYGOOD; lt++)
            fprintf(pf, "%d %d ", psc->anLuck[0][lt], psc->anLuck[1][lt]);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arLuck[0][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arLuck[0][1]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arLuck[1][0]);
        fprintf(pf, "%s ", buffer);
        g_ascii_formatd(buffer, sizeof(buffer), "%.6f", psc->arLuck[1][1]);
        fprintf(pf, "%s]", buffer);
    }
}

static void
WriteProperty(FILE * pf, const char *szName, char *szValue)
{

    if (!szValue || !*szValue)
        return;

    fputs(szName, pf);
    putc('[', pf);
    WriteEscapedString(pf, szValue, FALSE);
    putc(']', pf);
}

static void
AddRule(FILE * pf, const char *sz, int *pfFirst)
{

    fprintf(pf, "%s%s", (*pfFirst) ? "" : ":", sz);
    *pfFirst = FALSE;

}

extern void
SaveGame(FILE * pf, listOLD * plGame)
{
    listOLD *pl;
    moverecord *pmr;
    unsigned int i, j;
    TanBoard anBoard;
    listOLD *pl_hint = NULL;
    int fMoveNormalSeen;

    updateStatisticsGame(plGame);

    pl = plGame->plNext;
    pmr = pl->p;
    g_assert(pmr->mt == MOVE_GAMEINFO);

    /* Fixed header */
    fputs("(;FF[4]GM[6]CA[UTF-8]AP[GNU Backgammon:" VERSION "]", pf);

    /* Match length, if appropriate */
    /* FIXME: isn't it always appropriate to write this? */
    /* If not, money games will be loaded without score and game number */
    /* if( pmr->g.nMatch ) */
    fprintf(pf, "MI[length:%d][game:%d][ws:%d][bs:%d]", pmr->g.nMatch, pmr->g.i, pmr->g.anScore[0], pmr->g.anScore[1]);

    /* Names */
    fputs("PW[", pf);
    WriteEscapedString(pf, ap[0].szName, FALSE);
    fputs("]PB[", pf);
    WriteEscapedString(pf, ap[1].szName, FALSE);
    putc(']', pf);

    if (!pmr->g.i) {
        char szDate[32];

        WriteProperty(pf, "WR", mi.pchRating[0]);
        WriteProperty(pf, "BR", mi.pchRating[1]);
        if (mi.nYear) {
            sprintf(szDate, "%04u-%02u-%02u", mi.nYear, mi.nMonth, mi.nDay);
            WriteProperty(pf, "DT", szDate);
        }
        WriteProperty(pf, "EV", mi.pchEvent);
        WriteProperty(pf, "RO", mi.pchRound);
        WriteProperty(pf, "PC", mi.pchPlace);
        WriteProperty(pf, "AN", mi.pchAnnotator);
        WriteProperty(pf, "GC", mi.pchComment);
    }

    if (pmr->g.fCrawford || pmr->g.fJacoby || pmr->g.bgv != VARIATION_STANDARD || !pmr->g.fCubeUse) {

        static const char *aszSGFVariation[NUM_VARIATIONS] = { NULL, "Nackgammon", "Hypergammon1", "Hypergammon2",
            "Hypergammon3"
        };
        int fFirst = TRUE;

        fputs("RU[", pf);

        if (!pmr->g.fCubeUse)
            AddRule(pf, "NoCube", &fFirst);
        if (pmr->g.fCrawford)
            AddRule(pf, "Crawford", &fFirst);
        if (pmr->g.fCrawfordGame)
            AddRule(pf, "CrawfordGame", &fFirst);
        if (pmr->g.fJacoby)
            AddRule(pf, "Jacoby", &fFirst);
        if (pmr->g.bgv != VARIATION_STANDARD)
            AddRule(pf, aszSGFVariation[pmr->g.bgv], &fFirst);

        fputs("]", pf);
    }

    if (pmr->g.nAutoDoubles)
        fprintf(pf, "CV[%d]", 1 << pmr->g.nAutoDoubles);

    if (pmr->g.fWinner >= 0)
        fprintf(pf, "RE[%c+%d%s]", pmr->g.fWinner ? 'B' : 'W', pmr->g.nPoints, pmr->g.fResigned ? "R" : "");

    if (pmr->g.sc.fMoves || pmr->g.sc.fCube || pmr->g.sc.fDice)
        WriteStatContext(pf, &pmr->g.sc);

    if (game_is_last(plGame))
        pl_hint = game_add_pmr_hint(plGame);

    fMoveNormalSeen = FALSE;

    for (pl = pl->plNext; pl != plGame; pl = pl->plNext) {
        pmr = pl->p;
        switch (pmr->mt) {
        case MOVE_NORMAL:
            fMoveNormalSeen = TRUE;
            /* sanitise the move if from a hint record */
            if (pmr->ml.cMoves && pmr->n.iMove > pmr->ml.cMoves) {
                memcpy(pmr->n.anMove, pmr->ml.amMoves[0].anMove, sizeof(pmr->n.anMove));
                pmr->n.iMove = 0;
            }
            fprintf(pf, "\n;%c[%u%u", pmr->fPlayer ? 'B' : 'W', pmr->anDice[0], pmr->anDice[1]);
            WriteMove(pf, pmr->fPlayer, pmr->n.anMove);
            putc(']', pf);

            if (pmr->CubeDecPtr->esDouble.et != EVAL_NONE)
                WriteDoubleAnalysis(pf,
                                    pmr->CubeDecPtr->aarOutput,
                                    pmr->CubeDecPtr->aarStdDev, &pmr->CubeDecPtr->esDouble, pmr->CubeDecPtr->cmark);

            if (pmr->ml.cMoves)
                WriteMoveAnalysis(pf, pmr->fPlayer, &pmr->ml, pmr->n.iMove);

            WriteLuck(pf, pmr->fPlayer, pmr->rLuck, pmr->lt);
            /* FIXME: separate skill for cube and move */
            WriteSkill(pf, pmr->n.stMove);
            WriteSkillCube(pf, pmr->stCube);

            break;

        case MOVE_DOUBLE:
            /* Last record cannot usually be a legitimate double, it
             * is a placeholder for hint data for a possible double by
             * the player on roll. Skip it. See discussion of bug #36716. */
            if (pl->plNext == plGame &&
                /* Exception. In a scenario like: copy id from position
                 * with dice not rolled yet / paste it into gnubg / hint
                 * / save, it is reasonable to assume that the user is
                 * interested in the doubling decision analysis. There
                 * was no explicit double action but save the
                 * MOVE_DOUBLE record anyway. */
                !(fMoveNormalSeen == FALSE && pmr->CubeDecPtr->esDouble.et != EVAL_NONE))
                break;

            fprintf(pf, "\n;%c[double]", pmr->fPlayer ? 'B' : 'W');

            if (pmr->CubeDecPtr->esDouble.et != EVAL_NONE)
                WriteDoubleAnalysis(pf,
                                    pmr->CubeDecPtr->aarOutput,
                                    pmr->CubeDecPtr->aarStdDev, &pmr->CubeDecPtr->esDouble, pmr->CubeDecPtr->cmark);

            WriteSkill(pf, pmr->stCube);

            break;

        case MOVE_TAKE:
            fprintf(pf, "\n;%c[take]", pmr->fPlayer ? 'B' : 'W');

            if (pmr->CubeDecPtr->esDouble.et != EVAL_NONE)
                WriteDoubleAnalysis(pf,
                                    pmr->CubeDecPtr->aarOutput,
                                    pmr->CubeDecPtr->aarStdDev, &pmr->CubeDecPtr->esDouble, pmr->CubeDecPtr->cmark);

            WriteSkill(pf, pmr->stCube);

            break;

        case MOVE_DROP:
            fprintf(pf, "\n;%c[drop]", pmr->fPlayer ? 'B' : 'W');

            if (pmr->CubeDecPtr->esDouble.et != EVAL_NONE)
                WriteDoubleAnalysis(pf,
                                    pmr->CubeDecPtr->aarOutput,
                                    pmr->CubeDecPtr->aarStdDev, &pmr->CubeDecPtr->esDouble, pmr->CubeDecPtr->cmark);


            WriteSkill(pf, pmr->stCube);

            break;

        case MOVE_RESIGN:
            break;

        case MOVE_SETBOARD:
            PositionFromKey(anBoard, &pmr->sb.key);

            /* Insert a PL record. This will set the right player on
             * roll when loading a before-diceroll position (instead
             * of always player 0, which is usually wrong) and is
             * transparent in the other cases. */
            fprintf(pf, "\n;PL[%c]", pmr->fPlayer ? 'B' : 'W');

            fputs("AE[a:y]", pf);

            for (i = 0, j = 0; i < 25; ++i)
                j += anBoard[1][i];

            if (j) {
                fputs("AW", pf);
                for (i = 0; i < 25; i++)
                    for (j = 0; j < anBoard[1][i]; j++)
                        fprintf(pf, "[%c]", 'a' + i);
            }

            for (i = 0, j = 0; i < 25; ++i)
                j += anBoard[0][i];

            if (j) {
                fputs("AB", pf);
                for (i = 0; i < 25; i++)
                    for (j = 0; j < anBoard[0][i]; j++)
                        fprintf(pf, "[%c]", i == 24 ? 'y' : 'x' - i);
            }

            break;

        case MOVE_SETDICE:
            fprintf(pf, "\n;PL[%c]DI[%u%u]", pmr->fPlayer ? 'B' : 'W', pmr->anDice[0], pmr->anDice[1]);

            WriteLuck(pf, pmr->fPlayer, pmr->rLuck, pmr->lt);

            break;

        case MOVE_SETCUBEVAL:
            fprintf(pf, "\n;CV[%d]", pmr->scv.nCube);
            break;

        case MOVE_SETCUBEPOS:
            fprintf(pf, "\n;CP[%c]", "cwb"[pmr->scp.fCubeOwner + 1]);
            break;

        default:
            g_assert_not_reached();
        }

        if (pmr->sz) {
            fputs("C[", pf);
            WriteEscapedString(pf, pmr->sz, FALSE);
            putc(']', pf);
        }
    }

    if (pl_hint)
        game_remove_pmr_hint(pl_hint);

    /* FIXME if the game is not over and the player on roll is the last
     * player to move, add a PL property */

    fputs(")\n", pf);

}

extern void
CommandSaveGame(char *sz)
{

    FILE *pf;

    sz = NextToken(&sz);

    if (!plGame) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    if (!sz || !*sz) {
        outputl(_("You must specify a file to save to (see `help save " "game')."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if (!(pf = g_fopen(sz, "w"))) {
        outputerr(sz);
        return;
    }

    SaveGame(pf, plGame);

    if (pf != stdout)
        fclose(pf);

    setDefaultFileName(sz);

}

extern void
CommandSaveMatch(char *sz)
{

    FILE *pf;
    listOLD *pl;

    sz = NextToken(&sz);

    if (!plGame) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    /* FIXME what should be done if nMatchTo == 0? */

    if (!sz || !*sz) {
        outputl(_("You must specify a file to save to (see `help save " "match')."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if (!(pf = g_fopen(sz, "w"))) {
        outputerr(sz);
        return;
    }

    for (pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext)
        SaveGame(pf, pl->p);

    if (pf != stdout)
        fclose(pf);

    setDefaultFileName(sz);

    delete_autosave();
}

extern void
CommandSavePosition(char *sz)
{

    FILE *pf;
    listOLD l;
    moverecord *pmgi;
    moverecord *pmsb;
    moverecord *pmsd = NULL;
    moverecord *pmr_cur = NULL;
    moverecord *pmscv;
    moverecord *pmscp;

    sz = NextToken(&sz);

    if (!plGame) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    if (!sz || !*sz) {
        outputl(_("You must specify a file to save to (see `help save " "position')."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if (!(pf = g_fopen(sz, "w"))) {
        outputerr(sz);
        return;
    }

    ListCreate(&l);

    /* gameinfo record */

    pmgi = NewMoveRecord();

    pmgi->mt = MOVE_GAMEINFO;
    pmgi->sz = NULL;
    pmgi->g.i = 0;
    pmgi->g.nMatch = ms.nMatchTo;
    pmgi->g.anScore[0] = ms.anScore[0];
    pmgi->g.anScore[1] = ms.anScore[1];
    pmgi->g.fCrawford = fAutoCrawford && ms.nMatchTo > 1;
    pmgi->g.fCrawfordGame = ms.fCrawford;
    pmgi->g.fJacoby = ms.fJacoby && !ms.nMatchTo;
    pmgi->g.fWinner = -1;
    pmgi->g.nPoints = 0;
    pmgi->g.fResigned = FALSE;
    pmgi->g.nAutoDoubles = 0;
    pmgi->g.bgv = ms.bgv;
    pmgi->g.fCubeUse = ms.fCubeUse;
    IniStatcontext(&pmgi->g.sc);
    ListInsert(&l, pmgi);

    /* setboard record */

    pmsb = NewMoveRecord();

    pmsb->mt = MOVE_SETBOARD;
    pmsb->fPlayer = ms.fMove;
    if (ms.fMove)
        SwapSides(ms.anBoard);
    PositionKey(msBoard(), &pmsb->sb.key);
    if (ms.fMove)
        SwapSides(ms.anBoard);
    ListInsert(&l, pmsb);

    /* set cube value */

    pmscv = NewMoveRecord();

    pmscv->mt = MOVE_SETCUBEVAL;
    pmscv->scv.nCube = ms.nCube;
    ListInsert(&l, pmscv);

    /* cube position */

    pmscp = NewMoveRecord();

    pmscp->mt = MOVE_SETCUBEPOS;
    pmscp->scp.fCubeOwner = ms.fCubeOwner;
    ListInsert(&l, pmscp);

    /* set dice */

    /* FIXME if the dice are not rolled, this should be done with a PL
     * property (which is SaveGame()'s job) */

    pmr_cur = get_current_moverecord(NULL);
    if (pmr_cur && (pmr_cur->ml.cMoves > 0 || pmr_cur->CubeDecPtr->esDouble.et != EVAL_NONE))
        ListInsert(&l, pmr_cur);
    else if (ms.anDice[0] > 0) {
        pmsd = NewMoveRecord();
        pmsd->mt = MOVE_SETDICE;
        pmsd->fPlayer = ms.fMove;
        pmsd->anDice[0] = ms.anDice[0];
        pmsd->anDice[1] = ms.anDice[1];
        pmsd->lt = LUCK_NONE;
        pmsd->rLuck = ERR_VAL;
        ListInsert(&l, pmsd);
    }

    /* FIXME add MOVE_DOUBLE record(s) as appropriate */

    SaveGame(pf, &l);

    if (pf != stdout)
        fclose(pf);

    while (l.plNext->p)
        ListDelete(l.plNext);

    free(pmgi);
    free(pmsb);
    free(pmsd);
    free(pmscv);
    free(pmscp);

    setDefaultFileName(sz);
}
