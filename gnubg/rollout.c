/*
 * rollout.c
 *
 * by Gary Wong <gary@cs.arizona.edu>, 1999, 2000, 2001.
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
 * $Id: rollout.c,v 1.246 2015/03/01 13:14:20 plm Exp $
 */

#include "config.h"

#include <errno.h>
#include <isaac.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <time.h>

#include "backgammon.h"
#if USE_GTK
#include "gtkgame.h"
#endif
#include "matchid.h"
#include "positionid.h"
#include "format.h"
#include "multithread.h"
#include "rollout.h"

#if !LOCKING_VERSION

f_BasicCubefulRollout BasicCubefulRollout = BasicCubefulRolloutNoLocking;
#define BasicCubefulRollout BasicCubefulRolloutNoLocking

int log_rollouts = 0;
char *log_file_name = 0;
static unsigned int initial_game_count;

/* make sgf files of rollouts if log_rollouts is true and we have a file 
 * name template to work with
 */

extern void
log_cube(FILE * logfp, const char *action, int side)
{
    if (!logfp)
        return;
    fprintf(logfp, ";%s[%s]\n", side ? "B" : "W", action);
}

extern void
log_move(FILE * logfp, const int *anMove, int side, int die0, int die1)
{
    int i;
    if (!logfp)
        return;

    fprintf(logfp, ";%s[%d%d", side ? "B" : "W", die0, die1);

    for (i = 0; i < 8; i += 2) {
        if (anMove[i] < 0)
            break;

        if (anMove[i] > 23)
            fprintf(logfp, "y");
        else if (!side)
            fprintf(logfp, "%c", 'a' + anMove[i]);
        else
            fprintf(logfp, "%c", 'x' - anMove[i]);

        if (anMove[i + 1] < 0)
            fprintf(logfp, "z");
        else if (!side)
            fprintf(logfp, "%c", 'a' + anMove[i + 1]);
        else
            fprintf(logfp, "%c", 'x' - anMove[i + 1]);

    }

    fprintf(logfp, "%s", "]\n");

}

static void
board_to_sgf(FILE * logfp, const unsigned int anBoard[25], int direction)
{
    unsigned int i, j;
    int c = direction > 0 ? 'a' : 'x';
    if (!logfp)
        return;

    for (i = 0; i < 24; ++i) {
        for (j = 0; j < anBoard[i]; ++j)
            fprintf(logfp, "[%c]", c);

        c += direction;
    }

    for (j = 0; j < anBoard[24]; ++j)
        fprintf(logfp, "[y]");
}

extern FILE *
log_game_start(const char *name, const cubeinfo * pci, int fCubeful, TanBoard anBoard)
{
    time_t t = time(0);
    struct tm *now = localtime(&t);
    const char *rule;
    FILE *logfp = NULL;

    if (pci->nMatchTo == 0) {
        if (!fCubeful)
            rule = "RU[NoCube:Jacoby]";
        else if (!pci->fJacoby) {
            rule = "";
        } else {
            rule = "RU[Jacoby]";
        }
    } else {
        if (!fCubeful) {
            rule = "RU[NoCube:Crawford]";
        } else if (fAutoCrawford) {
            rule = (ms.fCrawford) ? "RU[Crawford:CrawfordGame]" : "RU[Crawford]";
        } else {
            rule = "";
        }
    }

    if ((logfp = g_fopen(name, "w")) == 0)
        return NULL;

    fprintf(logfp, "(;FF[4]GM[6]CA[UTF-8]AP[GNU Backgammon:%s]MI"
            "[length:%d][game:0][ws:%d][bs:%d][wtime:0][btime:0]"
            "[wtimeouts:0][btimeouts:0]PW[White]PB[Black]DT[%d-%02d-%02d]"
            "%s\n", VERSION, pci->nMatchTo, pci->anScore[0], pci->anScore[1],
            1900 + now->tm_year, 1 + now->tm_mon, now->tm_mday, rule);

    /* set the rest of the things up */
    fprintf(logfp, ";PL[%s]\n", pci->fMove ? "B" : "W");
    fprintf(logfp, ";CP[%s]\n", pci->fCubeOwner == 0 ? "w" : pci->fCubeOwner == 1 ? "b" : "c");
    fprintf(logfp, ";CV[%d]\n", pci->nCube);
    fprintf(logfp, ";AE[a:y]AW");
    if (!pci->fMove) {
        board_to_sgf(logfp, anBoard[1], 1);
        fprintf(logfp, "AB");
        board_to_sgf(logfp, anBoard[0], -1);
    } else {
        board_to_sgf(logfp, anBoard[0], 1);
        fprintf(logfp, "AB");
        board_to_sgf(logfp, anBoard[1], -1);
    }
    fprintf(logfp, "\n");
    return logfp;
}

extern void
log_game_over(FILE * logfp)
{
    if (!logfp)
        return;
    fprintf(logfp, ")");
    fclose(logfp);
}

extern void
QuasiRandomSeed(perArray * pArray, int n)
{

    int i, j, r;
    unsigned char k, t;
    randctx rc;

    if (pArray->nPermutationSeed == n)
        return;

    for (i = 0; i < RANDSIZ; i++)
        rc.randrsl[i] = (ub4) n;

    irandinit(&rc, TRUE);

    for (i = 0; i < 6; i++)
        for (j = i /* no need for permutations below the diagonal */ ; j < 128; j++) {
            for (k = 0; k < 36; k++)
                pArray->aaanPermutation[i][j][k] = k;
            for (k = 0; k < 35; k++) {
                r = irand(&rc) % (36 - k);
                t = pArray->aaanPermutation[i][j][k + r];
                pArray->aaanPermutation[i][j][k + r] = pArray->aaanPermutation[i][j][k];
                pArray->aaanPermutation[i][j][k] = t;
            }
        }

    pArray->nPermutationSeed = n;
}

static int nSkip;

extern int
RolloutDice(int iTurn, int iGame,
            int fInitial,
            unsigned int anDice[2], rng * rngx, void *rngctx, const int fRotate, const perArray * dicePerms)
{

    if (fInitial && !iTurn) {
        /* rollout of initial position: no doubles allowed */
        if (fRotate) {
            unsigned int j;

            if (!iGame)
                nSkip = 0;

            for (;; nSkip++) {
                j = dicePerms->aaanPermutation[0][0][(iGame + nSkip) % 36];

                anDice[0] = j / 6 + 1;
                anDice[1] = j % 6 + 1;

                if (anDice[0] != anDice[1])
                    break;
            }

            return 0;
        } else {
            do {
                int n;
                if ((n = RollDice(anDice, rngx, rngctx)) != 0)
                    return n;
            } while (fInitial && !iTurn && anDice[0] == anDice[1]);

            return 0;
        }
    } else if (fRotate && iTurn < 128) {
        unsigned int i,         /* the "generation" of the permutation */
         j,                     /* the number we're permuting */
         k;                     /* 36**i */

        for (i = 0, j = 0, k = 1; i < 6 && i <= (unsigned int) iTurn; i++, k *= 36)
            j = dicePerms->aaanPermutation[i][iTurn][((iGame + nSkip) / k + j) % 36];

        anDice[0] = j / 6 + 1;
        anDice[1] = j % 6 + 1;
        return 0;
    } else
        return RollDice(anDice, rngx, rngctx);
}


extern void
ClosedBoard(int afClosedBoard[2], const TanBoard anBoard)
{

    int i, j, n;

    for (i = 0; i < 2; i++) {

        n = 0;
        for (j = 0; j < 6; j++) {
            if (anBoard[i][j] > 1)
                n++;
        }

        afClosedBoard[i] = (n == 6);

    }

}

#else

#define BasicCubefulRollout BasicCubefulRolloutWithLocking

static volatile unsigned int initial_game_count;

#endif

extern void initRolloutstat(rolloutstat * prs);

/* called with 
 * cube decision                  move rollout
 * aanBoard       2 copies of same board         1 board
 * aarOutput      2 arrays for eval              1 array
 * iTurn          player on roll                 same
 * iGame          game number                    same
 * cubeinfo       2 structs for double/nodouble  1 cubeinfo
 * or take/pass
 * CubeDecTop     array of 2 boolean             1 boolean
 * (TRUE if a cube decision is valid on turn 0)
 * cci            2 (number of rollouts to do)   1
 * prc            1 rollout context              same
 * aarsStatistics 2 arrays of stats for the      NULL
 * two alternatives of 
 * cube rollouts 
 * 
 * returns -1 on error/interrupt, fInterrupt TRUE if stopped by user
 * aarOutput array(s) contain results
 */

extern int
BasicCubefulRollout(unsigned int aanBoard[][2][25],
                    float aarOutput[][NUM_ROLLOUT_OUTPUTS],
                    int iTurn, int iGame,
                    const cubeinfo aci[], int afCubeDecTop[], unsigned int cci,
                    rolloutcontext * prc,
                    rolloutstat aarsStatistics[][2],
                    int nBasisCube, perArray * dicePerms, rngcontext * rngctxRollout, FILE * logfp)
{

    unsigned int anDice[2];
    unsigned int cUnfinished = cci;
    cubeinfo *pci;
    cubedecision cd;
    int *pf;
    unsigned int i, j, k, ici;
    evalcontext ec;

    positionclass pc, pcBefore;
    unsigned int nPipsBefore = 0, nPipsAfter, nPipsDice;
    unsigned int anPips[2];
    int afClosedBoard[2];

    float arDouble[NUM_CUBEFUL_OUTPUTS];
    float aar[2][NUM_ROLLOUT_OUTPUTS];

    unsigned int aiBar[2];

    int afClosedOut[2] = { FALSE, FALSE };
    int afHit[2] = { FALSE, FALSE };

    float rDP;
    float r;

    int nTruncate = prc->fDoTruncate ? prc->nTruncate : 0x7fffffff;

    int nLateEvals = prc->fLateEvals ? prc->nLate : 0x7fffffff;

    /* Make local copy of cubeinfo struct, since it
     * may be modified */
    cubeinfo *pciLocal = g_alloca(cci * sizeof(cubeinfo));
    int *pfFinished = g_alloca(cci * sizeof(int));
    float (*aarVarRedn)[NUM_ROLLOUT_OUTPUTS] = g_alloca(cci * NUM_ROLLOUT_OUTPUTS * sizeof(float));

    /* variables for variance reduction */

    evalcontext aecVarRedn[2];
    evalcontext aecZero[2];
    float arMean[NUM_ROLLOUT_OUTPUTS];
    unsigned int aaanBoard[6][6][2][25];
    int aanMoves[6][6][8];
    float aaar[6][6][NUM_ROLLOUT_OUTPUTS];

    evalcontext ecCubeless0ply = { FALSE, 0, FALSE, TRUE, 0.0 };
    evalcontext ecCubeful0ply = { TRUE, 0, FALSE, TRUE, 0.0 };

    /* local pointers to the eval contexts to use */
    evalcontext *pecCube[2], *pecChequer[2];

    if (prc->fVarRedn) {

        /*
         * Create evaluation context one ply deep
         */

        for (ici = 0; ici < cci; ici++)
            for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
                aarVarRedn[ici][i] = 0.0f;

        for (i = 0; i < 2; i++) {
            aecZero[i] = aecVarRedn[i] = prc->aecChequer[i];
            aecZero[i].nPlies = 0;
            if (aecVarRedn[i].nPlies)
                aecVarRedn[i].nPlies--;
            aecZero[i].fDeterministic = aecVarRedn[i].fDeterministic = 1;
            aecZero[i].rNoise = aecVarRedn[i].rNoise = 0.0f;
        }

    }

    for (ici = 0; ici < cci; ici++)
        pfFinished[ici] = TRUE;

    memcpy(pciLocal, aci, cci * sizeof(cubeinfo));

    while ((!nTruncate || iTurn < nTruncate) && cUnfinished) {
        if (iTurn < nLateEvals) {
            pecCube[0] = prc->aecCube;
            pecCube[1] = prc->aecCube + 1;
            pecChequer[0] = prc->aecChequer;
            pecChequer[1] = prc->aecChequer + 1;
        } else {
            pecCube[0] = prc->aecCubeLate;
            pecCube[1] = prc->aecCubeLate + 1;
            pecChequer[0] = prc->aecChequerLate;
            pecChequer[1] = prc->aecChequerLate + 1;
        }

        /* Cube decision */

        for (ici = 0, pci = pciLocal, pf = pfFinished; ici < cci; ici++, pci++, pf++) {

            /* check for truncation at bearoff databases */

            pc = ClassifyPosition((ConstTanBoard) aanBoard[ici], pci->bgv);

            if (prc->fTruncBearoff2 && pc <= CLASS_PERFECT &&
                prc->fCubeful && *pf && !pci->nMatchTo && ((afCubeDecTop[ici] && !prc->fInitial) || iTurn > 0)) {

                /* truncate at two sided bearoff if money game */

                if (GeneralEvaluationE(aarOutput[ici], (ConstTanBoard) aanBoard[ici], pci, &ecCubeful0ply) < 0)
                    return -1;

                if (iTurn & 1)
                    InvertEvaluationR(aarOutput[ici], pci);

                *pf = FALSE;
                cUnfinished--;

            } else if (((prc->fTruncBearoff2 && pc <= CLASS_PERFECT) ||
                        (prc->fTruncBearoffOS && pc <= CLASS_BEAROFF_OS)) && !prc->fCubeful && *pf) {

                /* cubeless rollout, requested to truncate at bearoff db */

                if (GeneralEvaluationE(aarOutput[ici], (ConstTanBoard) aanBoard[ici], pci, &ecCubeless0ply) < 0)
                    return -1;

                /* rollout result is for player on play (even iTurn).
                 * This point is pre play, so if opponent is on roll, invert */

                if (iTurn & 1)
                    InvertEvaluationR(aarOutput[ici], pci);

                *pf = FALSE;
                cUnfinished--;

            }

            if (*pf) {

                if (prc->fCubeful && GetDPEq(NULL, &rDP, pci) && (iTurn > 0 || (afCubeDecTop[ici] && !prc->fInitial))) {

                    if (GeneralCubeDecisionE(aar, (ConstTanBoard) aanBoard[ici], pci, pecCube[pci->fMove], 0) < 0)
                        return -1;

                    cd = FindCubeDecision(arDouble, aar, pci);

                    switch (cd) {

                    case DOUBLE_TAKE:
                    case DOUBLE_BEAVER:
                    case REDOUBLE_TAKE:
                        if (logfp) {
                            log_cube(logfp, "double", pci->fMove);
                            log_cube(logfp, "take", !pci->fMove);
                        }

                        /* update statistics */
                        if (aarsStatistics)
                            MT_SafeInc(&aarsStatistics[ici][pci->fMove].acDoubleTake[LogCube(pci->nCube)]);

                        SetCubeInfo(pci, 2 * pci->nCube, !pci->fMove, pci->fMove, pci->nMatchTo,
                                    pci->anScore, pci->fCrawford, pci->fJacoby, pci->fBeavers, pci->bgv);

                        break;

                    case DOUBLE_PASS:
                    case REDOUBLE_PASS:
                        if (logfp) {
                            log_cube(logfp, "double", pci->fMove);
                            log_cube(logfp, "drop", !pci->fMove);
                        }

                        *pf = FALSE;
                        cUnfinished--;

                        /* assign outputs */

                        for (i = 0; i <= OUTPUT_EQUITY; i++)
                            aarOutput[ici][i] = aar[0][i];

                        /* 
                         * assign equity for double, pass:
                         * - mwc for match play
                         * - normalized equity for money play (i.e, rDP=1)
                         */

                        aarOutput[ici][OUTPUT_CUBEFUL_EQUITY] = rDP;

                        /* invert evaluations if required */

                        if (iTurn & 1)
                            InvertEvaluationR(aarOutput[ici], pci);

                        /* update statistics */

                        if (aarsStatistics) {
                            MT_SafeInc(&aarsStatistics[ici][pci->fMove].acDoubleDrop[LogCube(pci->nCube)]);
                            MT_SafeInc(&aarsStatistics[ici][pci->fMove].acWin[LogCube(pci->nCube)]);
                        };

                        break;

                    case NODOUBLE_TAKE:
                    case TOOGOOD_TAKE:
                    case TOOGOOD_PASS:
                    case NODOUBLE_BEAVER:
                    case NO_REDOUBLE_TAKE:
                    case TOOGOODRE_TAKE:
                    case TOOGOODRE_PASS:
                    case NO_REDOUBLE_BEAVER:
                    case OPTIONAL_DOUBLE_BEAVER:
                    case OPTIONAL_DOUBLE_TAKE:
                    case OPTIONAL_REDOUBLE_TAKE:
                    case OPTIONAL_DOUBLE_PASS:
                    case OPTIONAL_REDOUBLE_PASS:
                    case NODOUBLE_DEADCUBE:
                    case NO_REDOUBLE_DEADCUBE:
                    case NOT_AVAILABLE:
                    default:

                        /* no op */
                        break;

                    }
                }               /* cube */
            }
        }                       /* loop over ci */

        /* Chequer play */

        if (RolloutDice(iTurn, iGame, prc->fInitial, anDice,
                        &prc->rngRollout, rngctxRollout, prc->fRotate, dicePerms) < 0)
            return -1;

        if (anDice[0] < anDice[1])
            swap_us(anDice, anDice + 1);


        for (ici = 0, pci = pciLocal, pf = pfFinished; ici < cci; ici++, pci++, pf++) {

            if (*pf) {

                /* Save number of chequers on bar */

                for (i = 0; i < 2; i++)
                    aiBar[i] = aanBoard[ici][i][24];

                /* Save number of pips (for bearoff only) */

                pcBefore = ClassifyPosition((ConstTanBoard) aanBoard[ici], pci->bgv);
                if (aarsStatistics && pcBefore <= CLASS_BEAROFF1) {
                    PipCount((ConstTanBoard) aanBoard[ici], anPips);
                    nPipsBefore = anPips[1];
                }

                /* Find best move :-) */

                if (prc->fVarRedn) {

                    /* Variance reduction */

                    for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
                        arMean[i] = 0.0f;

                    for (i = 0; i < 6; i++)
                        for (j = 0; j <= i; j++) {

                            if (prc->fInitial && !iTurn && j == i)
                                /* no doubles possible for first roll when rolling
                                 * out as initial position */
                                continue;

                            memcpy(&aaanBoard[i][j][0][0], &aanBoard[ici][0][0], 2 * 25 * sizeof(int));

                            /* Find the best move for each roll on ply 0 only */

                            if (FindBestMove(aanMoves[i][j], i + 1, j + 1,
                                             aaanBoard[i][j], pci, &aecZero[pci->fMove], defaultFilters) < 0)
                                return -1;

                            SwapSides(aaanBoard[i][j]);

                            /* re-evaluate the chosen move at ply n-1 */

                            pci->fMove = !pci->fMove;
                            if (GeneralEvaluationE(aaar[i][j],
                                                   (ConstTanBoard) aaanBoard[i][j], pci, &aecVarRedn[pci->fMove]) < 0)
                                return -1;
                            pci->fMove = !pci->fMove;

                            if (!(iTurn & 1))
                                InvertEvaluationR(aaar[i][j], pci);

                            /* Calculate arMean: the n-ply evaluation of the position */

                            for (k = 0; k < NUM_ROLLOUT_OUTPUTS; k++)
                                arMean[k] += ((i == j) ? aaar[i][j][k] : (aaar[i][j][k] * 2.0f));

                        }

                    if (prc->fInitial && !iTurn)
                        /* no doubles ... */
                        for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
                            arMean[i] /= 30.0f;
                    else
                        for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
                            arMean[i] /= 36.0f;

                    /* Find best move */

                    if (pecChequer[pci->fMove]->nPlies ||
                        prc->fCubeful != pecChequer[pci->fMove]->fCubeful || pecChequer[pci->fMove]->rNoise > 0.0f)

                        /* the user requested n-ply (n>0). Another call to
                         * FindBestMove is required */

                        FindBestMove(aanMoves[anDice[0] - 1][anDice[1] - 1],
                                     anDice[0], anDice[1],
                                     aanBoard[ici], pci,
                                     pecChequer[pci->fMove],
                                     (iTurn < nLateEvals) ? prc->aaamfChequer[pci->fMove] : prc->aaamfLate[pci->fMove]);

                    else {

                        /* 0-ply play: best move is already recorded */

                        memcpy(&aanBoard[ici][0][0],
                               &aaanBoard[anDice[0] - 1][anDice[1] - 1][0][0], 2 * 25 * sizeof(int));

                        SwapSides(aanBoard[ici]);

                    }


                    /* Accumulate variance reduction terms */

                    if (pci->nMatchTo)
                        for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
                            aarVarRedn[ici][i] += arMean[i] - aaar[anDice[0] - 1][anDice[1] - 1][i];
                    else {
                        for (i = 0; i <= OUTPUT_EQUITY; i++)
                            aarVarRedn[ici][i] += arMean[i] - aaar[anDice[0] - 1][anDice[1] - 1][i];

                        r = arMean[OUTPUT_CUBEFUL_EQUITY] - aaar[anDice[0] - 1][anDice[1] - 1]
                            [OUTPUT_CUBEFUL_EQUITY];
                        aarVarRedn[ici][OUTPUT_CUBEFUL_EQUITY] += r * pci->nCube / aci[ici].nCube;
                    }

                } else {

                    /* no variance reduction */

                    FindBestMove(aanMoves[anDice[0] - 1][anDice[1] - 1],
                                 anDice[0], anDice[1],
                                 aanBoard[ici], pci,
                                 pecChequer[pci->fMove],
                                 (iTurn < nLateEvals) ? prc->aaamfChequer[pci->fMove] : prc->aaamfLate[pci->fMove]);

                }

                if (logfp) {
                    log_move(logfp, aanMoves[anDice[0] - 1][anDice[1] - 1], pci->fMove, anDice[0], anDice[1]);
                }

                /* Save hit statistics */

                /* FIXME: record double hit, triple hits etc. ? */

                if (aarsStatistics && !afHit[pci->fMove] && (aiBar[0] < aanBoard[ici][0][24])) {
                    MT_SafeInc(&aarsStatistics[ici][pci->fMove].nOpponentHit);
                    MT_SafeAdd(&aarsStatistics[ici][pci->fMove].rOpponentHitMove, iTurn);
                    afHit[pci->fMove] = TRUE;

                }

                if (fInterrupt)
                    return -1;

                /* Calculate number of wasted pips */

                pc = ClassifyPosition((ConstTanBoard) aanBoard[ici], pci->bgv);

                if (aarsStatistics && pc <= CLASS_BEAROFF1 && pcBefore <= CLASS_BEAROFF1) {

                    PipCount((ConstTanBoard) aanBoard[ici], anPips);
                    nPipsAfter = anPips[1];
                    nPipsDice = anDice[0] + anDice[1];
                    if (anDice[0] == anDice[1])
                        nPipsDice *= 2;

                    MT_SafeInc(&aarsStatistics[ici][pci->fMove].nBearoffMoves);
                    MT_SafeAdd(&aarsStatistics[ici][pci->fMove].nBearoffPipsLost,
                               nPipsDice - (nPipsBefore - nPipsAfter));

                }

                /* Opponent closed out */

                if (aarsStatistics && !afClosedOut[pci->fMove]
                    && aanBoard[ici][0][24]) {

                    /* opponent is on bar */

                    ClosedBoard(afClosedBoard, (ConstTanBoard) aanBoard[ici]);

                    if (afClosedBoard[pci->fMove]) {
                        MT_SafeInc(&aarsStatistics[ici][pci->fMove].nOpponentClosedOut);
                        MT_SafeAdd(&aarsStatistics[ici][pci->fMove].rOpponentClosedOutMove, iTurn);
                        afClosedOut[pci->fMove] = TRUE;
                    }

                }


                /* check if game is over */

                if (pc == CLASS_OVER) {
                    if (GeneralEvaluationE(aarOutput[ici], (ConstTanBoard) aanBoard[ici], pci, pecCube[pci->fMove]) < 0)
                        return -1;

                    /* Since the game is over: cubeless equity = cubeful equity
                     * (convert to mwc for match play) */

                    aarOutput[ici][OUTPUT_CUBEFUL_EQUITY] =
                        (pci->nMatchTo) ? eq2mwc(aarOutput[ici][OUTPUT_EQUITY], pci) : aarOutput[ici][OUTPUT_EQUITY];

                    if (iTurn & 1)
                        InvertEvaluationR(aarOutput[ici], pci);

                    *pf = FALSE;
                    cUnfinished--;

                    /* update statistics */

                    if (aarsStatistics)
                        switch (GameStatus((ConstTanBoard) aanBoard[ici], pci->bgv)) {
                        case 1:
                            MT_SafeInc(&aarsStatistics[ici][pci->fMove].acWin[LogCube(pci->nCube)]);
                            break;
                        case 2:
                            MT_SafeInc(&aarsStatistics[ici][pci->fMove].acWinGammon[LogCube(pci->nCube)]);
                            break;
                        case 3:
                            MT_SafeInc(&aarsStatistics[ici][pci->fMove].acWinBackgammon[LogCube(pci->nCube)]);
                            break;
                        }

                }

                /* Invert board and more */

                SwapSides(aanBoard[ici]);

                SetCubeInfo(pci, pci->nCube, pci->fCubeOwner,
                            !pci->fMove, pci->nMatchTo,
                            pci->anScore, pci->fCrawford, pci->fJacoby, pci->fBeavers, pci->bgv);
            }
        }

        iTurn++;

    }                           /* loop truncate */


    /* evaluation at truncation */

    for (ici = 0, pci = pciLocal, pf = pfFinished; ici < cci; ici++, pci++, pf++) {

        if (*pf) {

            /* ensure cubeful evaluation at truncation */

            memcpy(&ec, &prc->aecCubeTrunc, sizeof(ec));
            ec.fCubeful = prc->fCubeful;

            /* evaluation at truncation */

            if (GeneralEvaluationE(aarOutput[ici], (ConstTanBoard) aanBoard[ici], pci, &ec) < 0)
                return -1;

            if (iTurn & 1)
                InvertEvaluationR(aarOutput[ici], pci);

        }

        /* the final output is the sum of the resulting evaluation and
         * all variance reduction terms */

        if (!pci->nMatchTo)
            aarOutput[ici][OUTPUT_CUBEFUL_EQUITY] *= pci->nCube / aci[ici].nCube;

        if (prc->fVarRedn)
            for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
                aarOutput[ici][i] += aarVarRedn[ici][i];

        /* multiply money equities */

        if (!pci->nMatchTo)
            aarOutput[ici][OUTPUT_CUBEFUL_EQUITY] *= aci[ici].nCube / nBasisCube;



        /*        if ( pci->nMatchTo ) */
        /*          aarOutput[ ici ][ OUTPUT_CUBEFUL_EQUITY ] = */
        /*            eq2mwc ( aarOutput[ ici ][ OUTPUT_CUBEFUL_EQUITY ], pci ); */
        /*        else */
        /*          aarOutput[ ici ][ OUTPUT_CUBEFUL_EQUITY ] *= */
        /*            pci->nCube / aci [ ici ].nCube; */

    }

    return 0;
}

#if !LOCKING_VERSION

/* called with a collection of moves or a cube decision to be rolled out.
 * when called with a cube decision, the number of alternatives is always 2
 * (nodouble/double or take/drop). Otherwise the number of moves is
 * a parameter supplied (alternatives)
 * 
 * anBoard - an array[alternatives] of explicit pointers to Boards - the
 * individual boards are not in and of themselves a contiguous set of
 * arrays and can't be treated as int x[alternative][2][25]. 2 copies
 * of the same board for cube decisions, 1 per move for move rollouts
 * asz an array of pointers to strings. These will be a contiguous array of
 * text labels for displaying results. 2 pointers for cube decisions, 
 * 1 per move for move rollouts
 * aarOutput - an array[alternatives] of explicit pointers to arrays for the
 * results of the rollout. Again, these may not be contiguous. 2 arrays for 
 * cube decisions, 1 per move for move rollouts
 * aarStdDev - as above for std's of rollout
 * aarsStatistics - array of statistics used when rolling out cube decisions,
 * not maintained when doing move rollouts
 * pprc - an array of explicit pointers to rollout contexts. There will be
 * 2 pointers to the same context for cube decisions, 1 per move for move 
 * rollouts 
 * aci  - an array of explicit pointers cubeinfo's. 2 for cube decisions, one
 * per move for move rollouts
 * alternatives - a count of the number of things to be rolled out. 2 for 
 * cube decisions, number of different moves for move rollouts
 * fInvert - flag if equities should be inverted (used when doing take/drop
 * decisions, we evaluate the double/nodouble and invert the equities
 * to get take/drop
 * fCubeRollout - set if this is a cube decision rollout. This is needed if 
 * we use RolloutGeneral to rollout an arbitrary list of moves (where not
 * all the moves correspond to a given game state, so that some moves will
 * have been made with a different cube owner or value or even come from
 * different games and have different match scores. If this happens,
 * calls to mwc2eq and se_mwc2eq need to be passed a pointer to the current
 * cubeinfo structure. If we're rolling out a cube decision, we need to 
 * pass the cubeinfo structure before the double is given. This won't be
 * available
 * 
 * returns:
 * -1 on error or if no games were rolled out
 * no of games rolled out otherwise. aarOutput, aarStdDev aarsStatistic arrays
 * will contain results.
 * pprc rollout contexts will be updated with the number of games rolled out for 
 * that position.
 */

static int
comp_jsdinfo_equity(const void *a, const void *b)
{
    const jsdinfo *aa = a;
    const jsdinfo *bb = b;

    if (aa->rEquity < bb->rEquity)
        return 1;
    else if (aa->rEquity > bb->rEquity)
        return -1;

    return 0;
}

static int
comp_jsdinfo_order(const void *a, const void *b)
{
    const jsdinfo *aa = a;
    const jsdinfo *bb = b;

    if (aa->nOrder < bb->nOrder)
        return -1;
    else if (aa->nOrder > bb->nOrder)
        return 1;

    return 0;
}

/* Lots of shared variables - should probably not be globals... */
static int cGames;
static cubeinfo *aciLocal;
static int show_jsds;

static float (*aarMu)[NUM_ROLLOUT_OUTPUTS];
static float (*aarSigma)[NUM_ROLLOUT_OUTPUTS];
static float (*aarResult)[NUM_ROLLOUT_OUTPUTS];
static float (*aarVariance)[NUM_ROLLOUT_OUTPUTS];
static int *fNoMore;
static jsdinfo *ajiJSD;

static int ro_alternatives = -1;
static evalsetup **ro_apes;
static ConstTanBoard *ro_apBoard;
static const cubeinfo **ro_apci;
static int **ro_apCubeDecTop;
static rolloutstat(*ro_aarsStatistics)[2];
static int ro_fCubeRollout;
static int ro_fInvert;
static int ro_NextTrial;
static unsigned int *altGameCount;
static int *altTrialCount;

static void
check_jsds(int *active)
{
    int alt;
    float v, s, denominator;
    rolloutcontext *prc;

    for (alt = 0; alt < ro_alternatives; ++alt) {

        /* 1) For each move, calculate the cubeful (or cubeless if that's what we're doing)
         * equity */
        prc = &ro_apes[alt]->rc;
        if (prc->fCubeful) {
            v = aarMu[alt][OUTPUT_CUBEFUL_EQUITY];
            s = aarSigma[alt][OUTPUT_CUBEFUL_EQUITY];

            /* if we're doing a cube rollout, we need aciLocal[0] for generating the
             * equity. If we're doing moves, we use the cubeinfo that goes with this move. */
            if (ms.nMatchTo && !fOutputMWC) {
                v = mwc2eq(v, &aciLocal[(ro_fCubeRollout ? 0 : alt)]);
                s = se_mwc2eq(s, &aciLocal[(ro_fCubeRollout ? 0 : alt)]);
            }
        } else {
            v = aarMu[alt][OUTPUT_EQUITY];
            s = aarSigma[alt][OUTPUT_EQUITY];

            if (ms.nMatchTo && fOutputMWC) {
                v = eq2mwc(v, &aciLocal[(ro_fCubeRollout ? 0 : alt)]);
                s = se_eq2mwc(s, &aciLocal[(ro_fCubeRollout ? 0 : alt)]);

            }
        }
        ajiJSD[alt].rEquity = v;
        ajiJSD[alt].rJSD = s;
    }

    if (!ro_fCubeRollout) {
        /* 2 sort the list in order of decreasing equity (best move first) */
        qsort((void *) ajiJSD, ro_alternatives, sizeof(jsdinfo), comp_jsdinfo_equity);

        /* 3 replace the equities with the equity difference from the best move (ajiJSD[0]), the JSDs
         * with the number of JSDs the equity difference represents and decide if we should either stop 
         * or resume rolling a move out */
        v = ajiJSD[0].rEquity;
        s = ajiJSD[0].rJSD;
        s *= s;
        for (alt = ro_alternatives - 1; alt > 0; --alt) {

            ajiJSD[alt].nRank = alt;
            ajiJSD[alt].rEquity = v - ajiJSD[alt].rEquity;

            denominator = sqrtf(s + ajiJSD[alt].rJSD * ajiJSD[alt].rJSD);

            if (denominator < 1e-8f)
                denominator = 1e-8f;

            ajiJSD[alt].rJSD = ajiJSD[alt].rEquity / denominator;

            if ((rcRollout.fStopOnJsd) && (altGameCount[ajiJSD[alt].nOrder] >= (rcRollout.nMinimumJsdGames))) {
                if (ajiJSD[alt].rJSD > rcRollout.rJsdLimit) {
                    /* This move is no longer worth rolling out */

                    fNoMore[ajiJSD[alt].nOrder] = 1;
                    ro_apes[alt]->rc.rStoppedOnJSD = ajiJSD[alt].rJSD;

                    (*active)--;

                } else {
                    /* this move needs to roll out further. It may need to be caught up
                     * with other moves, because it's been stopped for a few trials */
                    if (fNoMore[ajiJSD[alt].nOrder]) {
                        /* it was stopped, catch it up to the other moves and resume
                         * rolling it out. While we're catching up, we don't want to do 
                         * these calculations any more so we'll change the minimum
                         * games to do */
                        fNoMore[ajiJSD[alt].nOrder] = 0;
                        (*active)++;
                    }
                }
            }
        }

        /* fill out details of best move */
        ajiJSD[0].rEquity = ajiJSD[0].rJSD = 0.0f;
        ajiJSD[0].nRank = 0;

        /* rearrange ajiJSD in move order rather than equity order */
        qsort((void *) ajiJSD, ro_alternatives, sizeof(jsdinfo), comp_jsdinfo_order);

    } else {
        float eq_dp = fOutputMWC ? eq2mwc(1.0, &aciLocal[0]) : 1.0f;
        float eq_dt = ajiJSD[1].rEquity;

        if (eq_dp < eq_dt) {
            /* compare nd to dp */
            ajiJSD[0].rEquity = ajiJSD[0].rEquity - eq_dp;
            denominator = ajiJSD[0].rJSD;
            if (denominator < 1e-8f)
                denominator = 1e-8f;
            ajiJSD[0].rJSD = fabsf(ajiJSD[0].rEquity / denominator);
        } else {
            /* compare nd to dt */
            ajiJSD[0].rEquity = ajiJSD[0].rEquity - ajiJSD[1].rEquity;
            denominator = sqrtf(ajiJSD[0].rJSD * ajiJSD[0].rJSD + ajiJSD[1].rJSD * ajiJSD[1].rJSD);
            if (denominator < 1e-8f)
                denominator = 1e-8f;
            ajiJSD[0].rJSD = fabsf(ajiJSD[0].rEquity / denominator);
        }
        /* compare dt to dp */
        ajiJSD[1].rEquity = ajiJSD[1].rEquity - eq_dp;
        denominator = ajiJSD[1].rJSD;
        if (denominator < 1e-8f)
            denominator = 1e-8f;
        ajiJSD[1].rJSD = fabsf(ajiJSD[1].rEquity / denominator);
        if (rcRollout.fStopOnJsd &&
            (altGameCount[0] >= (rcRollout.nMinimumJsdGames)) &&
            rcRollout.rJsdLimit < MIN(ajiJSD[0].rJSD, ajiJSD[1].rJSD)) {
            ro_apes[0]->rc.rStoppedOnJSD = ajiJSD[0].rJSD;
            ro_apes[1]->rc.rStoppedOnJSD = ajiJSD[1].rJSD;
            fNoMore[0] = 1;
            fNoMore[1] = 1;
            *active = 0;
        }
    }
}

static void
check_sds(int *active)
{
    int alt;
    for (alt = 0; alt < ro_alternatives; ++alt) {
        float s;
        int output;
        int err_too_big = 0;
        rolloutcontext *prc;
        if (fNoMore[alt] || altGameCount[alt] < (rcRollout.nMinimumGames))
            continue;
        prc = &ro_apes[alt]->rc;
        for (output = OUTPUT_EQUITY; output < NUM_ROLLOUT_OUTPUTS; output++) {
            if (output == OUTPUT_EQUITY) {      /* cubeless */
                if (!ms.nMatchTo) {     /* money game */
                    s = fabsf(aarSigma[alt][output]);
                    if (ro_fCubeRollout) {
                        s *= aciLocal[alt].nCube / aciLocal[0].nCube;
                    }
                } else {        /* match play */
                    s = fabsf(se_mwc2eq(se_eq2mwc(aarSigma[alt][output],
                                                  &aciLocal[alt]), &aciLocal[(ro_fCubeRollout ? 0 : alt)]));
                }
            } else {
                if (!prc->fCubeful)
                    continue;
                /* cubeful */
                if (!ms.nMatchTo) {     /* money game */
                    s = fabsf(aarSigma[alt][output]);
                } else {
                    s = fabsf(se_mwc2eq(aarSigma[alt][output], &aciLocal[(ro_fCubeRollout ? 0 : alt)]));
                }
            }

            if (rcRollout.rStdLimit < s) {
                err_too_big = 1;
                break;
            }
        }                       /* for (output = OUTPUT_EQUITY; output < NUM_ROLLOUT_OUTPUTS; output++) */

        if (!err_too_big) {
            fNoMore[alt] = 1;
            (*active)--;
        }

    }                           /* alt = 0; alt < ro_alternatives; ++alt) */
    if (ro_fCubeRollout && (!fNoMore[0] || !fNoMore[1])) {
        /* cube rollouts should run the same number
         * of trials for nd and dt */
        fNoMore[0] = fNoMore[1] = 0;
        *active = 2;
    }

}

extern void
RolloutLoopMT(void *UNUSED(unused))
{
    TanBoard anBoardEval;
    float aar[NUM_ROLLOUT_OUTPUTS];
    int active_alternatives;
    unsigned int j;
    int alt;
    FILE *logfp = NULL;
    rolloutcontext *prc = NULL;
    /* Each thread gets a copy of the rngctxRollout */
    rngcontext *rngctxMTRollout = CopyRNGContext(rngctxRollout);
    perArray dicePerms;
    dicePerms.nPermutationSeed = -1;

    /* ============ begin rollout loop ============= */

    while (MT_SafeIncValue(&ro_NextTrial) <= cGames) {
        active_alternatives = ro_alternatives;

        for (alt = 0; alt < ro_alternatives; ++alt) {
            int trial = MT_SafeIncValue(&altTrialCount[alt]) - 1;
            /* skip this one if it's already finished */
            if (fNoMore[alt] || (trial > cGames)) {
                MT_SafeDec(&altTrialCount[alt]);
                continue;
            }


            prc = &ro_apes[alt]->rc;

            /* get the dice generator set up... */
            if (prc->fRotate)
                QuasiRandomSeed(&dicePerms, (int) prc->nSeed);

            nSkip = 0;          /* not multi-thread safe do quasi random dice for initial positions */

            /* ... and the RNG */
            if (prc->rngRollout != RNG_MANUAL)
                InitRNGSeed((unsigned int) (prc->nSeed + (trial << 8)), prc->rngRollout, rngctxMTRollout);

            memcpy(&anBoardEval, ro_apBoard[alt], sizeof(anBoardEval));

            /* roll something out */
            if (log_rollouts && log_file_name) {
                char *log_name = g_strdup_printf("%s-%7.7d-%c.sgf", log_file_name, trial, alt + 'a');
                logfp = log_game_start(log_name, ro_apci[alt], prc->fCubeful, anBoardEval);
                g_free(log_name);
            }
            BasicCubefulRollout(&anBoardEval, &aar, 0, trial, ro_apci[alt],
                                ro_apCubeDecTop[alt], 1, prc,
                                ro_aarsStatistics ? ro_aarsStatistics + alt : NULL,
                                aciLocal[ro_fCubeRollout ? 0 : alt].nCube, &dicePerms, rngctxMTRollout, logfp);

            if (logfp) {
                log_game_over(logfp);
            }

            if (fInterrupt)
                break;

            multi_debug("exclusive lock: update result for alternative");
            MT_Exclusive();
            altGameCount[alt]++;

            if (ro_fInvert)
                InvertEvaluationR(aar, ro_apci[alt]);

            /* apply the results */
            for (j = 0; j < NUM_ROLLOUT_OUTPUTS; j++) {
                float rMuNew, rDelta;

                aarResult[alt][j] += aar[j];
                rMuNew = aarResult[alt][j] / (altGameCount[alt]);

                if (altGameCount[alt] > 1) {    /* for i == 0 aarVariance is not defined */

                    rDelta = rMuNew - aarMu[alt][j];

                    aarVariance[alt][j] =
                        aarVariance[alt][j] * (1.0f - 1.0f / (altGameCount[alt] - 1)) +
                        (altGameCount[alt]) * rDelta * rDelta;
                }

                aarMu[alt][j] = rMuNew;

                if (j < OUTPUT_EQUITY) {
                    if (aarMu[alt][j] < 0.0f)
                        aarMu[alt][j] = 0.0f;
                    else if (aarMu[alt][j] > 1.0f)
                        aarMu[alt][j] = 1.0f;
                }

                aarSigma[alt][j] = sqrtf(aarVariance[alt][j] / (altGameCount[alt]));
            }                   /* for (j = 0; j < NUM_ROLLOUT_OUTPUTS; j++ ) */

            /* For normal alternatives nGamesDone and altGameCount will be equal. For cube decisions,
             * however, the two may differ by the number of threads minus 1. So we cheat a little bit, but
             * it would be better if the double and nodouble alternatives weren't linked */
            if (prc->nGamesDone < altGameCount[alt])
                prc->nGamesDone = altGameCount[alt];

            MT_Release();
            multi_debug("exclusive release: update result for alternative");

        }                       /* for (alt = 0; alt < ro_alternatives; ++alt) */

        if (fInterrupt)
            break;

        /* we've rolled everything out for this trial, check stopping conditions */
        /* Stop rolling out moves whose Equity is more than a user selected multiple of the joint standard
         * deviation of the equity difference with the best move in the list. */

#if !USE_MULTITHREAD
        ProcessEvents();
#endif

        multi_debug("exclusive lock: rollout cycle update");
        MT_Exclusive();
        if (show_jsds) {
            check_jsds(&active_alternatives);
        }
        if (rcRollout.fStopOnSTD) {
            check_sds(&active_alternatives);
        }
        if ((active_alternatives < 2 && rcRollout.fStopOnJsd) || active_alternatives < 1) {
            multi_debug("exclusive release: rollout done early");
            MT_Release();
            break;
        }
        multi_debug("exclusive release: rollout cycle update");
        MT_Release();
    }
    free(rngctxMTRollout);
}

static rolloutprogressfunc *ro_pfProgress;
static void *ro_pUserData;

static gboolean
UpdateProgress(gpointer UNUSED(unused))
{
    if (fShowProgress && ro_alternatives > 0) {
        int alt;
        rolloutcontext *prc;

        multi_debug("exclusive lock: update progress");
        MT_Exclusive();

        for (alt = 0; alt < ro_alternatives; ++alt) {
            prc = &ro_apes[alt]->rc;
            (*ro_pfProgress) (aarMu, aarSigma, prc, aciLocal, initial_game_count, altGameCount[alt] - 1, alt,
                              ajiJSD[alt].nRank + 1, ajiJSD[alt].rJSD, fNoMore[alt], show_jsds, ro_fCubeRollout,
                              ro_pUserData);
        }

        MT_Release();
        multi_debug("exclusive release: update progress");
    }
    return TRUE;
}

extern int
RolloutGeneral(ConstTanBoard * apBoard,
               float (*apOutput[])[NUM_ROLLOUT_OUTPUTS],
               float (*apStdDev[])[NUM_ROLLOUT_OUTPUTS],
               rolloutstat aarsStatistics[][2],
               evalsetup(*apes[]),
               const cubeinfo(*apci[]),
               int (*apCubeDecTop[]), int alternatives,
               int fInvert, int fCubeRollout, rolloutprogressfunc * pfProgress, void *pUserData)
{
    unsigned int j;
    int alt;
    unsigned int i;
    int nFirstTrial;
    unsigned int trialsDone;
    rolloutcontext *prc = NULL, rcRolloutSave;
    evalsetup *pes;
    int nIsCubeless = 0;
    int nIsCubeful = 0;
    int fOutputMWCSave = fOutputMWC;
    int active_alternatives;
    int previous_rollouts = 0;

    show_jsds = 1;

    if (alternatives < 1) {
        errno = EINVAL;
        return -1;
    }

    ajiJSD = g_alloca(alternatives * sizeof(jsdinfo));
    fNoMore = g_alloca(alternatives * sizeof(int));
    aciLocal = g_alloca(alternatives * sizeof(cubeinfo));
    altGameCount = g_alloca(alternatives * sizeof(int));
    altTrialCount = g_alloca(alternatives * sizeof(int));

    aarMu = g_alloca(alternatives * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    aarSigma = g_alloca(alternatives * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    aarResult = g_alloca(alternatives * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    aarVariance = g_alloca(alternatives * NUM_ROLLOUT_OUTPUTS * sizeof(float));

    if (ms.nMatchTo == 0)
        fOutputMWC = 0;

    memcpy(&rcRolloutSave, &rcRollout, sizeof(rcRollout));
    if (alternatives == 1) {
        rcRollout.fStopOnJsd = 0;
    }

    /* make sure cube decisions are rolled out cubeful */
    if (fCubeRollout) {
        rcRollout.fCubeful = rcRollout.aecCubeTrunc.fCubeful = rcRollout.aecChequerTrunc.fCubeful = 1;
        for (i = 0; i < 2; ++i)
            rcRollout.aecCube[i].fCubeful = rcRollout.aecChequer[i].fCubeful =
                rcRollout.aecCubeLate[i].fCubeful = rcRollout.aecChequerLate[i].fCubeful = 1;
    }

    /* quasi random dice may not be thread safe when we need to skip
     * some rolls for initial positions */
    if (rcRollout.fInitial)
        rcRollout.fRotate = FALSE;

    /* nFirstTrial will be the smallest number of trials done for an alternative */
    nFirstTrial = cGames = rcRollout.nTrials;
    initial_game_count = 0;
    for (alt = 0; alt < alternatives; ++alt) {
        pes = apes[alt];
        prc = &pes->rc;

        /* fill out the JSD stuff */
        ajiJSD[alt].rEquity = ajiJSD[alt].rJSD = 0.0f;
        ajiJSD[alt].nRank = 0;
        ajiJSD[alt].nOrder = alt;

        /* save input cubeinfo */
        memcpy(&aciLocal[alt], apci[alt], sizeof(cubeinfo));

        /* Invert cubeinfo */

        if (fInvert)
            aciLocal[alt].fMove = !aciLocal[alt].fMove;

        if ((pes->et != EVAL_ROLLOUT) || (prc->nGamesDone == 0)) {
            /* later the saved context may to be stored with the move, so cubeful/cubeless must be made
             * consistent */
            rcRolloutSave.fCubeful = rcRolloutSave.aecCubeTrunc.fCubeful =
                rcRolloutSave.aecChequerTrunc.fCubeful = (fCubeRollout || rcRolloutSave.fCubeful);
            for (i = 0; i < 2; ++i)
                rcRolloutSave.aecCube[i].fCubeful =
                    rcRolloutSave.aecChequer[i].fCubeful =
                    rcRolloutSave.aecCubeLate[i].fCubeful =
                    rcRolloutSave.aecChequerLate[i].fCubeful = (fCubeRollout || rcRolloutSave.fCubeful);

            memcpy(prc, &rcRollout, sizeof(rolloutcontext));
            prc->nGamesDone = 0;
            prc->nSkip = 0;
            nFirstTrial = 0;
            altTrialCount[alt] = altGameCount[alt] = 0;

            if (aarsStatistics) {
                initRolloutstat(&aarsStatistics[alt][0]);
                initRolloutstat(&aarsStatistics[alt][1]);
            }

            /* initialise internal variables */
            for (j = 0; j < NUM_ROLLOUT_OUTPUTS; ++j) {
                aarResult[alt][j] = aarVariance[alt][j] = aarMu[alt][j] = aarSigma[alt][j] = 0.0f;
            }
        } else {
            int nGames = prc->nGamesDone;
            float r;

            previous_rollouts++;

            /* make sure the saved rollout contexts are consistent for cubeful/not cubeful */
            prc->fCubeful = prc->aecCubeTrunc.fCubeful =
                prc->aecChequerTrunc.fCubeful = (prc->fCubeful || fCubeRollout);
            for (i = 0; i < 2; ++i)
                prc->aecCube[i].fCubeful = prc->aecChequer[i].fCubeful =
                    prc->aecCubeLate[i].fCubeful = prc->aecChequerLate[i].fCubeful = (prc->fCubeful || fCubeRollout);

            altTrialCount[alt] = altGameCount[alt] = nGames;
            initial_game_count += nGames;
            if (nGames < nFirstTrial)
                nFirstTrial = nGames;
            /* restore internal variables from input values */
            for (j = 0; j < NUM_ROLLOUT_OUTPUTS; ++j) {
                r = aarMu[alt][j] = (*apOutput[alt])[j];
                aarResult[alt][j] = r * nGames;
                r = aarSigma[alt][j] = (*apStdDev[alt])[j];
                aarVariance[alt][j] = r * r * nGames;
            }
        }

        /* force all moves/cube decisions to be considered and reset the upper bound on trials */
        fNoMore[alt] = 0;
        prc->nTrials = cGames;

        pes->et = EVAL_ROLLOUT;
        if (prc->fCubeful)
            ++nIsCubeful;
        else
            ++nIsCubeless;

        /* we can't do JSD tricks on initial positions */
        if (prc->fInitial) {
            rcRollout.fStopOnJsd = 0;
            show_jsds = 0;
        }

    }

    /* we can't do JSD tricks if some rollouts are cubeful and some not */
    if (nIsCubeful && nIsCubeless)
        rcRollout.fStopOnJsd = 0;

    /* if we're using stop on JSD, turn off stop on STD error */
    if (rcRollout.fStopOnJsd)
        rcRollout.fStopOnSTD = 0;

    /* Put parameters in global variables - urgh, would be better in task variable really... */
    ro_alternatives = alternatives;
    ro_apes = apes;
    ro_apBoard = apBoard;
    ro_apci = apci;
    ro_apCubeDecTop = apCubeDecTop;
    ro_aarsStatistics = aarsStatistics;
    ro_fCubeRollout = fCubeRollout;
    ro_fInvert = fInvert;
    ro_NextTrial = nFirstTrial;
    ro_pfProgress = pfProgress;
    ro_pUserData = pUserData;

    active_alternatives = ro_alternatives;

    /* check if rollout alternatives are done, but only when extending
     * all candidates */
    if (previous_rollouts == active_alternatives) {
        if (show_jsds) {
            check_jsds(&active_alternatives);
        }
        if (rcRollout.fStopOnSTD) {
            check_sds(&active_alternatives);
        }
    }

    UpdateProgress(NULL);

    if (active_alternatives > 1 || (!rcRollout.fStopOnJsd && active_alternatives > 0)) {
        multi_debug("rollout adding tasks");
        mt_add_tasks(MT_GetNumThreads(), RolloutLoopMT, NULL, NULL);

        multi_debug("rollout waiting for tasks to complete");
        MT_WaitForTasks(UpdateProgress, 2000, fAutoSaveRollout);
        multi_debug("rollout finished waiting for tasks to complete");
    }

    /* Make sure final output is upto date */
#if USE_GTK
    if (!fX)
#endif
        if (!fInterrupt)
            outputf(_("\nRollout done. Printing final results.\n"));

    if (!fInterrupt)
        UpdateProgress(NULL);

    /* Signal to UpdateProgress() called from pending events that no
     * more progress should be displayed.
     */
    ro_alternatives = -1;

    for (alt = 0, trialsDone = 0; alt < alternatives; ++alt) {
        if (apes[alt]->rc.nGamesDone > trialsDone)
            trialsDone = apes[alt]->rc.nGamesDone;
    }

    memcpy(&rcRollout, &rcRolloutSave, sizeof(rcRollout));
    fOutputMWC = fOutputMWCSave;

    /* return -1 if no games rolled out */
    if (trialsDone == 0)
        return -1;

    /* store results */
    for (alt = 0; alt < alternatives; alt++) {
        if (apOutput[alt])
            for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
                (*apOutput[alt])[i] = aarMu[alt][i];

        if (apStdDev[alt])
            for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
                (*apStdDev[alt])[i] = aarSigma[alt][i];
    }

    if (fShowProgress && !fInterrupt
#if USE_GTK
        && !fX
#endif
        ) {
        for (i = 0; i < 79; i++)
            outputc(' ');

        outputc('\r');
        fflush(stdout);
    }

    return trialsDone;
}

/*
 * General evaluation functions.
 */

extern int
GeneralEvaluation(float arOutput[NUM_ROLLOUT_OUTPUTS],
                  float arStdDev[NUM_ROLLOUT_OUTPUTS],
                  rolloutstat arsStatistics[2],
                  TanBoard anBoard, cubeinfo * const pci, const evalsetup * pes, rolloutprogressfunc * pf, void *p)
{

    int i;

    switch (pes->et) {
    case EVAL_EVAL:

        for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
            arStdDev[i] = 0.0f;

        return GeneralEvaluationE(arOutput, (ConstTanBoard) anBoard, pci, &pes->ec);

    case EVAL_ROLLOUT:

        return GeneralEvaluationR(arOutput, arStdDev, arsStatistics, (ConstTanBoard) anBoard, pci, &pes->rc, pf, p);

    case EVAL_NONE:

        for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
            arOutput[i] = arStdDev[i] = 0.0f;

        break;
    }

    return 0;
}

extern int
GeneralEvaluationR(float arOutput[NUM_ROLLOUT_OUTPUTS],
                   float arStdDev[NUM_ROLLOUT_OUTPUTS],
                   rolloutstat arsStatistics[2],
                   const TanBoard anBoard,
                   const cubeinfo * pci, const rolloutcontext * prc, rolloutprogressfunc * pf, void *p)
{
    ConstTanBoard apBoard[1];
    float (*apOutput[1])[NUM_ROLLOUT_OUTPUTS];
    float (*apStdDev[1])[NUM_ROLLOUT_OUTPUTS];
    evalsetup es;
    evalsetup(*apes[1]);
    const cubeinfo(*apci[1]);
    int false = 0;
    int (*apCubeDecTop[1]);

    apBoard[0] = anBoard;
    apOutput[0] = (float (*)[NUM_ROLLOUT_OUTPUTS]) arOutput;
    apStdDev[0] = (float (*)[NUM_ROLLOUT_OUTPUTS]) arStdDev;
    apes[0] = &es;
    apci[0] = pci;
    apCubeDecTop[0] = &false;

    es.et = EVAL_NONE;
    memcpy(&es.rc, prc, sizeof(rolloutcontext));

    if (RolloutGeneral(apBoard,
                       apOutput, apStdDev, (rolloutstat(*)[2]) arsStatistics,
                       apes, apci, apCubeDecTop, 1, FALSE, FALSE, pf, p) < 0)
        return -1;

    return 0;
}

extern int
GeneralCubeDecision(float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                    float aarStdDev[2][NUM_ROLLOUT_OUTPUTS],
                    rolloutstat aarsStatistics[2][2],
                    const TanBoard anBoard, cubeinfo * pci, evalsetup * pes, rolloutprogressfunc * pf, void *p)
{

    int i, j;

    switch (pes->et) {
    case EVAL_EVAL:

        for (j = 0; j < 2; j++)
            for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
                aarStdDev[j][i] = 0.0f;

        return GeneralCubeDecisionE(aarOutput, anBoard, pci, &pes->ec, pes);

    case EVAL_ROLLOUT:

        return GeneralCubeDecisionR(aarOutput, aarStdDev, aarsStatistics, anBoard, pci, &pes->rc, pes, pf, p);

    case EVAL_NONE:

        for (j = 0; j < 2; j++)
            for (i = 0; i < NUM_ROLLOUT_OUTPUTS; i++)
                aarStdDev[j][i] = 0.0f;

        break;
    }

    return 0;
}


extern int
GeneralCubeDecisionR(float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                     float aarStdDev[2][NUM_ROLLOUT_OUTPUTS],
                     rolloutstat aarsStatistics[2][2],
                     const TanBoard anBoard,
                     cubeinfo * pci, rolloutcontext * prc, evalsetup * pes, rolloutprogressfunc * pf, void *p)
{


    evalsetup esLocal;
    evalsetup(*apes[2]);
    cubeinfo aci[2];
    const cubeinfo(*apci[2]);
    int cGames;
    int afCubeDecTop[] = { FALSE, FALSE };      /* no cube decision in 
                                                 * iTurn = 0 */
    ConstTanBoard apBoard[2];
    float (*apOutput[2])[NUM_ROLLOUT_OUTPUTS];
    float (*apStdDev[2])[NUM_ROLLOUT_OUTPUTS];
    int (*apCubeDecTop[2]);
    apCubeDecTop[0] = afCubeDecTop;
    apCubeDecTop[1] = afCubeDecTop;
    apStdDev[0] = (float (*)[NUM_ROLLOUT_OUTPUTS]) aarStdDev[0];
    apStdDev[1] = (float (*)[NUM_ROLLOUT_OUTPUTS]) aarStdDev[1];
    apci[0] = &aci[0];
    apci[1] = &aci[1];
    apBoard[0] = apBoard[1] = anBoard;
    apOutput[0] = (float (*)[NUM_ROLLOUT_OUTPUTS]) aarOutput;
    apOutput[1] = (float (*)[NUM_ROLLOUT_OUTPUTS]) aarOutput + 1;

    if (pes == 0) {
        /* force rollout from sratch */
        pes = &esLocal;
        memcpy(&pes->rc, &rcRollout, sizeof(rcRollout));
        pes->et = EVAL_NONE;
        pes->rc.nGamesDone = 0;
    }

    apes[0] = apes[1] = pes;

    SetCubeInfo(&aci[0], pci->nCube, pci->fCubeOwner, pci->fMove,
                pci->nMatchTo, pci->anScore, pci->fCrawford, pci->fJacoby, pci->fBeavers, pci->bgv);

    SetCubeInfo(&aci[1], 2 * pci->nCube, !pci->fMove, pci->fMove,
                pci->nMatchTo, pci->anScore, pci->fCrawford, pci->fJacoby, pci->fBeavers, pci->bgv);

    if (!GetDPEq(NULL, NULL, &aci[0])) {
        outputl(_("Cube not available!"));
        return -1;
    }

    if (!prc->fCubeful) {
        outputl(_("Setting cubeful on"));
        prc->fCubeful = TRUE;
    }


    if ((cGames = RolloutGeneral(apBoard, apOutput, apStdDev,
                                 aarsStatistics, apes, apci, apCubeDecTop, 2, FALSE, TRUE, pf, p)) <= 0)
        return -1;

    pes->rc.nGamesDone = cGames;
    pes->rc.nSkip = nSkip;

    return 0;

}



/*
 * Initialise rollout stat with zeroes.
 *
 * Input: 
 *    - prs: rollout stat to initialize
 *
 * Output:
 *    None.
 *
 * Returns:
 *    void.
 *
 */

extern void
initRolloutstat(rolloutstat * prs)
{

    memset(prs, 0, sizeof(rolloutstat));

}



/*
 * Calculate whether we should resign or not
 *
 * Input:
 *    anBoard   - current board
 *    pci       - current cube info
 *    pesResign - evaluation parameters
 *
 * Output:
 *    arResign  - evaluation
 *
 * Returns:
 *    -1 on error
 *     0 if we should not resign
 *     1,2, or 3 if we should resign normal, gammon, or backgammon,
 *     respectively.  
 *
 */

extern int
getResignation(float arResign[NUM_ROLLOUT_OUTPUTS], TanBoard anBoard, cubeinfo * const pci, const evalsetup * pesResign)
{

    float arStdDev[NUM_ROLLOUT_OUTPUTS];
    rolloutstat arsStatistics[2];
    float ar[NUM_OUTPUTS] = { 0.0, 0.0, 0.0, 1.0, 1.0 };

    float rPlay;

    /* Evaluate current position */

    if (GeneralEvaluation(arResign, arStdDev, arsStatistics, anBoard, pci, pesResign, NULL, NULL) < 0)
        return -1;

    /* check if we want to resign */

    rPlay = Utility(arResign, pci);

    if (arResign[OUTPUT_LOSEBACKGAMMON] > 0.0f && Utility(ar, pci) == rPlay)
        /* resign backgammon */
        return (!pci->nMatchTo && pci->fJacoby && pci->fCubeOwner == -1) ? 1 : 3;
    else {

        /* worth trying to escape the backgammon */

        ar[OUTPUT_LOSEBACKGAMMON] = 0.0f;

        if (arResign[OUTPUT_LOSEGAMMON] > 0.0f && Utility(ar, pci) == rPlay)
            /* resign gammon */
            return (!pci->nMatchTo && pci->fJacoby && pci->fCubeOwner == -1) ? 1 : 2;
        else {

            /* worth trying to escape gammon */

            ar[OUTPUT_LOSEGAMMON] = 0.0f;

            return Utility(ar, pci) == rPlay;

        }

    }

}


extern void
getResignEquities(float arResign[NUM_ROLLOUT_OUTPUTS], cubeinfo * pci, int nResigned, float *prBefore, float *prAfter)
{

    float ar[NUM_OUTPUTS] = { 0, 0, 0, 0, 0 };

    *prBefore = Utility(arResign, pci);

    if (nResigned > 1)
        ar[OUTPUT_LOSEGAMMON] = 1.0f;
    if (nResigned > 2)
        ar[OUTPUT_LOSEBACKGAMMON] = 1.0f;

    *prAfter = Utility(ar, pci);

}


extern int
ScoreMoveRollout(move ** ppm, cubeinfo ** ppci, int cMoves, rolloutprogressfunc * pf, void *p)
{

    const cubeinfo *pci;
    int fCubeDecTop = TRUE;
    int i;
    int nGamesDone;
    rolloutcontext *prc;

    TanBoard *anBoard = g_alloca(cMoves * 2 * 25 * sizeof(int));
    ConstTanBoard *apBoard = g_alloca(cMoves * sizeof(int *));
    float (**apOutput)[NUM_ROLLOUT_OUTPUTS] = g_alloca(cMoves * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    float (**apStdDev)[NUM_ROLLOUT_OUTPUTS] = g_alloca(cMoves * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    evalsetup(**apes) = g_alloca(cMoves * sizeof(evalsetup *));
    const cubeinfo(**apci) = g_alloca(cMoves * sizeof(cubeinfo *));
    cubeinfo(*aci) = g_alloca(cMoves * sizeof(cubeinfo));
    int (**apCubeDecTop) = g_alloca(cMoves * sizeof(int *));

    /* initialise the arrays we'll need */
    for (i = 0; i < cMoves; ++i) {
        apBoard[i] = (ConstTanBoard) (anBoard + i);
        apOutput[i] = &ppm[i]->arEvalMove;
        apStdDev[i] = &ppm[i]->arEvalStdDev;
        apes[i] = &ppm[i]->esMove;
        apci[i] = aci + i;
        memcpy(aci + i, ppci[i], sizeof(cubeinfo));
        apCubeDecTop[i] = &fCubeDecTop;

        PositionFromKey(anBoard[i], &ppm[i]->key);

        SwapSides(anBoard[i]);

        /* swap fMove in cubeinfo */
        aci[i].fMove = !aci[i].fMove;

    }

    nGamesDone = RolloutGeneral(apBoard,
                                apOutput, apStdDev, NULL, apes, apci, apCubeDecTop, cMoves, TRUE, FALSE, pf, p);
    /* put fMove back again */
    for (i = 0; i < cMoves; ++i) {
        aci[i].fMove = !aci[i].fMove;
    }

    if (nGamesDone < 0)
        return -1;

    for (i = 0; i < cMoves; ++i) {

        /* Score for move:
         * rScore is the primary score (cubeful/cubeless)
         * rScore2 is the secondary score (cubeless) */
        prc = &apes[i]->rc;
        pci = apci[i];

        if (prc->fCubeful) {
            if (pci->nMatchTo)
                ppm[i]->rScore = mwc2eq(ppm[i]->arEvalMove[OUTPUT_CUBEFUL_EQUITY], pci);
            else
                ppm[i]->rScore = ppm[i]->arEvalMove[OUTPUT_CUBEFUL_EQUITY];
        } else
            ppm[i]->rScore = ppm[i]->arEvalMove[OUTPUT_EQUITY];

        ppm[i]->rScore2 = ppm[i]->arEvalMove[OUTPUT_EQUITY];
    }

    return 0;
}

#endif
