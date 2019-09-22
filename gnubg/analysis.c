/*
 * analysis.c
 *
 * by Joern Thyssen <joern@thyssen.nu>, 2000
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
 * $Id: analysis.c,v 1.242 2015/03/01 13:14:20 plm Exp $
 */

#include "config.h"

#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include "backgammon.h"
#include "drawboard.h"
#include "eval.h"
#if USE_GTK
#include "gtkgame.h"
#endif
#include "positionid.h"
#include "analysis.h"
#include "sound.h"
#include "matchequity.h"
#include "export.h"
#include "formatgs.h"
#include "progress.h"
#include "multithread.h"
#include "format.h"
#include "lib/simd.h"

const char *aszRating[N_RATINGS] = {
    N_("rating|Awful!"),
    N_("rating|Beginner"),
    N_("rating|Casual player"),
    N_("rating|Intermediate"),
    N_("rating|Advanced"),
    N_("rating|Expert"),
    N_("rating|World class"),
    N_("rating|Supernatural"),
    N_("rating|N/A")
};

const char *aszLuckRating[N_LUCKS] = {
    N_("luck|Go to bed"),
    N_("luck|Bad dice, man!"),
    N_("luck|None"),
    N_("luck|Good dice, man!"),
    N_("luck|Go to Las Vegas"),
};

static const float arThrsRating[RAT_SUPERNATURAL + 1] = {
    1e38f, 0.035f, 0.026f, 0.018f, 0.012f, 0.008f, 0.005f, 0.002f
};

/* 1e38, 0.060, 0.030, 0.025, 0.020, 0.015, 0.010, 0.005 }; */

int afAnalysePlayers[2] = { TRUE, TRUE };

evalcontext ecLuck = { TRUE, 0, FALSE, TRUE, 0.0 };

extern ratingtype
GetRating(const float rError)
{

    int i;

    for (i = RAT_SUPERNATURAL; i >= 0; i--)
        if (rError < arThrsRating[i])
            return (ratingtype) i;

    return RAT_UNDEFINED;
}

static float
LuckFirst(const TanBoard anBoard, const int n0, const int n1, cubeinfo * pci, const evalcontext * pec)
{

    TanBoard anBoardTemp;
    int i, j;
    float aar[6][6], ar[NUM_ROLLOUT_OUTPUTS], rMean = 0.0f;
    cubeinfo ciOpp;
    movelist ml;

    /* first with player pci->fMove on roll */

    memcpy(&ciOpp, pci, sizeof(cubeinfo));
    ciOpp.fMove = !pci->fMove;

    for (i = 0; i < 6; i++)
        for (j = 0; j < i; j++) {
            memcpy(&anBoardTemp[0][0], &anBoard[0][0], 2 * 25 * sizeof(int));

            /* Find the best move for each roll at ply 0 only. */
            if (FindnSaveBestMoves(&ml, i + 1, j + 1, (ConstTanBoard) anBoardTemp, NULL, 0.0f,
                                   pci, pec, defaultFilters) < 0)
                return (float) ERR_VAL;

            if (!ml.cMoves) {

                SwapSides(anBoardTemp);

                if (GeneralEvaluationE(ar, (ConstTanBoard) anBoardTemp, &ciOpp, pec) < 0)
                    return (float) ERR_VAL;

                if (pec->fCubeful) {
                    if (pci->nMatchTo)
                        aar[i][j] = -mwc2eq(ar[OUTPUT_CUBEFUL_EQUITY], &ciOpp);
                    else
                        aar[i][j] = -ar[OUTPUT_CUBEFUL_EQUITY];
                } else
                    aar[i][j] = -ar[OUTPUT_EQUITY];

            } else {
                aar[i][j] = ml.amMoves[0].rScore;
                free(ml.amMoves);
            }

            rMean += aar[i][j];

        }

    /* with other player on roll */

    for (i = 0; i < 6; i++)
        for (j = i + 1; j < 6; j++) {
            memcpy(&anBoardTemp[0][0], &anBoard[0][0], 2 * 25 * sizeof(int));
            SwapSides(anBoardTemp);

            /* Find the best move for each roll at ply 0 only. */
            if (FindnSaveBestMoves(&ml, i + 1, j + 1, (ConstTanBoard) anBoardTemp, NULL, 0.0f,
                                   &ciOpp, pec, defaultFilters) < 0)
                return (float) ERR_VAL;

            if (!ml.cMoves) {

                SwapSides(anBoardTemp);

                if (GeneralEvaluationE(ar, (ConstTanBoard) anBoardTemp, pci, pec) < 0)
                    return (float) ERR_VAL;

                if (pec->fCubeful) {
                    if (pci->nMatchTo)
                        aar[i][j] = mwc2eq(ar[OUTPUT_CUBEFUL_EQUITY], pci);
                    else
                        aar[i][j] = ar[OUTPUT_CUBEFUL_EQUITY];
                } else
                    aar[i][j] = ar[OUTPUT_EQUITY];

            } else {
                aar[i][j] = -ml.amMoves[0].rScore;
                free(ml.amMoves);
            }

            rMean += aar[i][j];

        }

    if (n0 > n1)
        return aar[n0][n1] - rMean / 30.0f;
    else
        return aar[n1][n0] - rMean / 30.0f;

}

static float
LuckNormal(const TanBoard anBoard, const int n0, const int n1, const cubeinfo * pci, const evalcontext * pec)
{

    TanBoard anBoardTemp;
    int i, j;
    float aar[6][6], ar[NUM_ROLLOUT_OUTPUTS], rMean = 0.0f;
    cubeinfo ciOpp;
    movelist ml;

    memcpy(&ciOpp, pci, sizeof(cubeinfo));
    ciOpp.fMove = !pci->fMove;

    for (i = 0; i < 6; i++)
        for (j = 0; j <= i; j++) {
            memcpy(&anBoardTemp[0][0], &anBoard[0][0], 2 * 25 * sizeof(int));

            /* Find the best move for each roll at ply 0 only. */
            if (FindnSaveBestMoves(&ml, i + 1, j + 1, (ConstTanBoard) anBoardTemp, NULL, 0.0f,
                                   pci, pec, defaultFilters) < 0)
                return (float) ERR_VAL;

            if (!ml.cMoves) {

                SwapSides(anBoardTemp);

                if (GeneralEvaluationE(ar, (ConstTanBoard) anBoardTemp, &ciOpp, pec) < 0)
                    return (float) ERR_VAL;

                if (pec->fCubeful) {
                    if (pci->nMatchTo)
                        aar[i][j] = -mwc2eq(ar[OUTPUT_CUBEFUL_EQUITY], &ciOpp);
                    else
                        aar[i][j] = -ar[OUTPUT_CUBEFUL_EQUITY];
                } else
                    aar[i][j] = -ar[OUTPUT_EQUITY];

            } else {
                aar[i][j] = ml.amMoves[0].rScore;
                free(ml.amMoves);
            }

            rMean += (i == j) ? aar[i][j] : aar[i][j] * 2.0f;

        }

    return aar[n0][n1] - rMean / 36.0f;

}

extern float
LuckAnalysis(const TanBoard anBoard, int n0, int n1, matchstate * pms)
{
    cubeinfo ci;
    int is_init_board;
    TanBoard init_board;

    GetMatchStateCubeInfo(&ci, pms);
    InitBoard(init_board, pms->bgv);
    is_init_board = !memcmp(init_board, pms->anBoard, 2 * 25 * sizeof(int));

    if (n0-- < n1--)            /* -- because as input n0 and n1 are dice [1..6] but in calls to LuckXX() they are array indexes [0..5] */
        swap(&n0, &n1);

    if (is_init_board && n0 != n1)      /* FIXME: this fails if we return to the inital position after a few moves */
        return LuckFirst(anBoard, n0, n1, &ci, &ecLuck);
    else
        return LuckNormal(anBoard, n0, n1, &ci, &ecLuck);
}

extern lucktype
Luck(float r)
{

    if (r > arLuckLevel[LUCK_VERYGOOD])
        return LUCK_VERYGOOD;
    else if (r > arLuckLevel[LUCK_GOOD])
        return LUCK_GOOD;
    else if (r < -arLuckLevel[LUCK_VERYBAD])
        return LUCK_VERYBAD;
    else if (r < -arLuckLevel[LUCK_BAD])
        return LUCK_BAD;
    else
        return LUCK_NONE;
}

extern skilltype
Skill(float r)
{
    if (r < -arSkillLevel[SKILL_VERYBAD])
        return SKILL_VERYBAD;
    else if (r < -arSkillLevel[SKILL_BAD])
        return SKILL_BAD;
    else if (r < -arSkillLevel[SKILL_DOUBTFUL])
        return SKILL_DOUBTFUL;
    else
        return SKILL_NONE;
}

/*
 * update statcontext for given move
 *
 * Input:
 *   psc: the statcontext to update
 *   pmr: the given move
 *   pms: the current match state
 *
 * Output:
 *   psc: the updated stat context
 *
 */

static void
updateStatcontext(statcontext * psc, const moverecord * pmr, const matchstate * pms, const listOLD * plGame)
{
    cubeinfo ci;
    static positionkey key;
    float rSkill, rChequerSkill, rCost;
    unsigned int i;
    float arDouble[4];
    const xmovegameinfo *pmgi = &((moverecord *) plGame->plNext->p)->g;
    doubletype dt;
    taketype tt;

    switch (pmr->mt) {

    case MOVE_GAMEINFO:
        /* update luck adjusted result */

        psc->arActualResult[0] = psc->arActualResult[1] = 0.0f;

        if (pmr->g.fWinner != -1) {

            if (pmr->g.nMatch) {

                psc->arActualResult[pmr->g.fWinner] =
                    getME(pmr->g.anScore[0], pmr->g.anScore[1], pmr->g.nMatch,
                          pmr->g.fWinner, pmr->g.nPoints, pmr->g.fWinner,
                          pmr->g.fCrawfordGame,
                          aafMET, aafMETPostCrawford) -
                    getMEAtScore(pmr->g.anScore[0], pmr->g.anScore[1],
                                 pmr->g.nMatch, pmr->g.fWinner, pmr->g.fCrawfordGame, aafMET, aafMETPostCrawford);

                psc->arActualResult[!pmr->g.fWinner] = -psc->arActualResult[pmr->g.fWinner];
            } else {
                psc->arActualResult[pmr->g.fWinner] = (float) pmr->g.nPoints;
                psc->arActualResult[!pmr->g.fWinner] = (float) -pmr->g.nPoints;
            }

        }

        for (i = 0; i < 2; ++i)
            psc->arLuckAdj[i] = psc->arActualResult[i];

        break;

    case MOVE_NORMAL:

        /* 
         * Cube analysis; check for
         *   - missed doubles
         */

        GetMatchStateCubeInfo(&ci, pms);

        if (pmr->CubeDecPtr->esDouble.et != EVAL_NONE && fAnalyseCube && pmgi->fCubeUse) {

            FindCubeDecision(arDouble, pmr->CubeDecPtr->aarOutput, &ci);

            psc->anTotalCube[pmr->fPlayer]++;

            /* Count doubles less than very bad */
            if (isCloseCubedecision(arDouble))
                psc->anCloseCube[pmr->fPlayer]++;

            if (arDouble[OUTPUT_NODOUBLE] < arDouble[OUTPUT_OPTIMAL]) {
                /* it was a double */

                rSkill = arDouble[OUTPUT_NODOUBLE] - arDouble[OUTPUT_OPTIMAL];

                rCost = pms->nMatchTo ? eq2mwc(rSkill, &ci) - eq2mwc(0.0f, &ci) : pms->nCube * rSkill;

                if (arDouble[OUTPUT_TAKE] > 1.0f) {
                    /* missed double above cash point */
                    psc->anCubeMissedDoubleTG[pmr->fPlayer]++;
                    psc->arErrorMissedDoubleTG[pmr->fPlayer][0] -= rSkill;
                    psc->arErrorMissedDoubleTG[pmr->fPlayer][1] -= rCost;
                } else {
                    /* missed double below cash point */
                    psc->anCubeMissedDoubleDP[pmr->fPlayer]++;
                    psc->arErrorMissedDoubleDP[pmr->fPlayer][0] -= rSkill;
                    psc->arErrorMissedDoubleDP[pmr->fPlayer][1] -= rCost;
                }

            }
            /* missed double */
        }



        /* EVAL_NONE */
        /*
         * update luck statistics for roll
         */
        if (fAnalyseDice && pmr->rLuck != (float) ERR_VAL) {

            float r = pms->nMatchTo ? eq2mwc(pmr->rLuck, &ci) - eq2mwc(0.0f, &ci) : pms->nCube * pmr->rLuck;

            psc->arLuck[pmr->fPlayer][0] += pmr->rLuck;
            psc->arLuck[pmr->fPlayer][1] += r;

            psc->arLuckAdj[pmr->fPlayer] -= r;
            psc->arLuckAdj[!pmr->fPlayer] += r;

            psc->anLuck[pmr->fPlayer][pmr->lt]++;

        }


        /*
         * update chequerplay statistics 
         */

        /* Count total regradless of analysis */
        psc->anTotalMoves[pmr->fPlayer]++;

        /* hmm, MOVE_NORMALs which has no legal moves have
         * pmr->n.esChequer.et == EVAL_NONE
         * (Joseph) But we can detect them by checking if a move was actually
         * made or not.
         */

        if (fAnalyseMove && (pmr->esChequer.et != EVAL_NONE || pmr->n.anMove[0] < 0)) {
            /* find skill */
            TanBoard anBoardMove;
            memcpy(anBoardMove, pms->anBoard, sizeof(anBoardMove));
            ApplyMove(anBoardMove, pmr->n.anMove, FALSE);
            PositionKey((ConstTanBoard) anBoardMove, &key);
            rChequerSkill = 0.0f;
            if (pmr->ml.amMoves) {
                for (i = 0; i < pmr->ml.cMoves; i++)

                    if (EqualKeys(key, pmr->ml.amMoves[i].key)) {

                        rChequerSkill = pmr->ml.amMoves[i].rScore - pmr->ml.amMoves[0].rScore;

                        break;
                    }
            }
            /* update statistics */

            rCost = pms->nMatchTo ? eq2mwc(rChequerSkill, &ci) - eq2mwc(0.0f, &ci) : pms->nCube * rChequerSkill;

            psc->anMoves[pmr->fPlayer][Skill(rChequerSkill)]++;

            if (pmr->ml.cMoves > 1) {
                psc->anUnforcedMoves[pmr->fPlayer]++;
                psc->arErrorCheckerplay[pmr->fPlayer][0] -= rChequerSkill;
                psc->arErrorCheckerplay[pmr->fPlayer][1] -= rCost;
            }

        } else {
            /* unmarked move */
            psc->anMoves[pmr->fPlayer][SKILL_NONE]++;
        }

        break;

    case MOVE_DOUBLE:

        dt = DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);
        if (dt != DT_NORMAL)
            break;

        GetMatchStateCubeInfo(&ci, pms);
        if (fAnalyseCube && pmgi->fCubeUse && pmr->CubeDecPtr->esDouble.et != EVAL_NONE) {

            FindCubeDecision(arDouble, pmr->CubeDecPtr->aarOutput, &ci);

            rSkill = arDouble[OUTPUT_TAKE] <
                arDouble[OUTPUT_DROP] ?
                arDouble[OUTPUT_TAKE] - arDouble[OUTPUT_OPTIMAL] : arDouble[OUTPUT_DROP] - arDouble[OUTPUT_OPTIMAL];

            psc->anTotalCube[pmr->fPlayer]++;
            psc->anDouble[pmr->fPlayer]++;
            psc->anCloseCube[pmr->fPlayer]++;

            if (rSkill < 0.0f) {
                /* it was not a double */

                rCost = pms->nMatchTo ? eq2mwc(rSkill, &ci) - eq2mwc(0.0f, &ci) : pms->nCube * rSkill;

                if (arDouble[OUTPUT_NODOUBLE] > 1.0f) {
                    /* wrong double above too good point */
                    psc->anCubeWrongDoubleTG[pmr->fPlayer]++;
                    psc->arErrorWrongDoubleTG[pmr->fPlayer][0] -= rSkill;
                    psc->arErrorWrongDoubleTG[pmr->fPlayer][1] -= rCost;
                } else {
                    /* wrong double below double point */
                    psc->anCubeWrongDoubleDP[pmr->fPlayer]++;
                    psc->arErrorWrongDoubleDP[pmr->fPlayer][0] -= rSkill;
                    psc->arErrorWrongDoubleDP[pmr->fPlayer][1] -= rCost;
                }
            }

        }

        break;

    case MOVE_TAKE:

        tt = (taketype) DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);
        if (tt > TT_NORMAL)
            break;

        GetMatchStateCubeInfo(&ci, pms);
        if (fAnalyseCube && pmgi->fCubeUse && pmr->CubeDecPtr->esDouble.et != EVAL_NONE) {

            FindCubeDecision(arDouble, pmr->CubeDecPtr->aarOutput, &ci);

            psc->anTotalCube[pmr->fPlayer]++;
            psc->anTake[pmr->fPlayer]++;
            psc->anCloseCube[pmr->fPlayer]++;

            if (-arDouble[OUTPUT_TAKE] < -arDouble[OUTPUT_DROP]) {

                /* it was a drop */

                rSkill = -arDouble[OUTPUT_TAKE] - -arDouble[OUTPUT_DROP];

                rCost = pms->nMatchTo ? eq2mwc(rSkill, &ci) - eq2mwc(0.0f, &ci) : pms->nCube * rSkill;

                psc->anCubeWrongTake[pmr->fPlayer]++;
                psc->arErrorWrongTake[pmr->fPlayer][0] -= rSkill;
                psc->arErrorWrongTake[pmr->fPlayer][1] -= rCost;
            }
        }

        break;

    case MOVE_DROP:

        GetMatchStateCubeInfo(&ci, pms);
        if (fAnalyseCube && pmgi->fCubeUse && pmr->CubeDecPtr->esDouble.et != EVAL_NONE) {

            FindCubeDecision(arDouble, pmr->CubeDecPtr->aarOutput, &ci);

            psc->anTotalCube[pmr->fPlayer]++;
            psc->anPass[pmr->fPlayer]++;
            psc->anCloseCube[pmr->fPlayer]++;

            if (-arDouble[OUTPUT_DROP] < -arDouble[OUTPUT_TAKE]) {

                /* it was a take */

                rSkill = -arDouble[OUTPUT_DROP] - -arDouble[OUTPUT_TAKE];

                rCost = pms->nMatchTo ? eq2mwc(rSkill, &ci) - eq2mwc(0.0f, &ci) : pms->nCube * rSkill;

                psc->anCubeWrongPass[pmr->fPlayer]++;
                psc->arErrorWrongPass[pmr->fPlayer][0] -= rSkill;
                psc->arErrorWrongPass[pmr->fPlayer][1] -= rCost;
            }
        }

        break;

    case MOVE_SETDICE:

        /*
         * update luck statistics for roll
         */


        GetMatchStateCubeInfo(&ci, pms);
        if (fAnalyseDice && pmr->rLuck != (float) ERR_VAL) {

            float r = pms->nMatchTo ? eq2mwc(pmr->rLuck, &ci) - eq2mwc(0.0f, &ci) : pms->nCube * pmr->rLuck;

            psc->arLuck[pmr->fPlayer][0] += pmr->rLuck;
            psc->arLuck[pmr->fPlayer][1] += r;

            psc->arLuckAdj[pmr->fPlayer] -= r;
            psc->arLuckAdj[!pmr->fPlayer] += r;

            psc->anLuck[pmr->fPlayer][pmr->lt]++;

        }

        break;

    default:

        break;

    }                           /* switch */

}

extern int
AnalyzeMove(moverecord * pmr, matchstate * pms, const listOLD * plParentGame,
            statcontext * psc, const evalsetup * pesChequer, evalsetup * pesCube,
            /* const */ movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES], const int analysePlayers[2],
            float *doubleError)
{
    TanBoard anBoardMove;
    cubeinfo ci;
    float rSkill, rChequerSkill;
    float aarOutput[2][NUM_ROLLOUT_OUTPUTS];
    float aarStdDev[2][NUM_ROLLOUT_OUTPUTS];

    doubletype dt;
    taketype tt;
    const xmovegameinfo *pmgi = &((moverecord *) plParentGame->plNext->p)->g;
    int is_initial_position = 1;

    /* analyze this move */

    FixMatchState(pms, pmr);

    /* check if it's the initial position: no cube analysis and special
     * luck analysis */

    if (pmr->mt != MOVE_GAMEINFO) {
        InitBoard(anBoardMove, pms->bgv);
        is_initial_position = !memcmp(anBoardMove, pms->anBoard, 2 * 25 * sizeof(int));
    }

    MT_Exclusive();
    switch (pmr->mt) {
    case MOVE_GAMEINFO:

        if (psc) {
            IniStatcontext(psc);
            updateStatcontext(psc, pmr, pms, plParentGame);
        }
        break;
    case MOVE_NORMAL:
        if (pmr->fPlayer != pms->fMove) {
            SwapSides(pms->anBoard);
            pms->fMove = pmr->fPlayer;
        }

        if (analysePlayers && !analysePlayers[pmr->fPlayer])
            break;

        rChequerSkill = 0.0f;
        GetMatchStateCubeInfo(&ci, pms);

        /* cube action? */

        if (!is_initial_position && fAnalyseCube && pmgi->fCubeUse && GetDPEq(NULL, NULL, &ci)) {
            float arDouble[NUM_CUBEFUL_OUTPUTS];

            if (cmp_evalsetup(pesCube, &pmr->CubeDecPtr->esDouble) > 0) {
                MT_Release();
                if (GeneralCubeDecision(aarOutput, aarStdDev, NULL,
                                        (ConstTanBoard) pms->anBoard, &ci, pesCube, NULL, NULL) < 0)
                    return -1;
                MT_Exclusive();

                pmr->CubeDecPtr->esDouble = *pesCube;

                memcpy(pmr->CubeDecPtr->aarOutput, aarOutput, sizeof(aarOutput));
                memcpy(pmr->CubeDecPtr->aarStdDev, aarStdDev, sizeof(aarStdDev));
            }

            FindCubeDecision(arDouble, pmr->CubeDecPtr->aarOutput, &ci);

            rSkill = arDouble[OUTPUT_NODOUBLE] - arDouble[OUTPUT_OPTIMAL];
            pmr->stCube = Skill(rSkill);

        } else
            pmr->CubeDecPtr->esDouble.et = EVAL_NONE;

        /* luck analysis */

        if (fAnalyseDice) {
            pmr->rLuck = LuckAnalysis((ConstTanBoard) pms->anBoard, pmr->anDice[0], pmr->anDice[1], pms);
            pmr->lt = Luck(pmr->rLuck);
        }

        /* evaluate move */

        if (fAnalyseMove) {
            positionkey key;

            /* evaluate move */

            memcpy(anBoardMove, pms->anBoard, sizeof(anBoardMove));
            ApplyMove(anBoardMove, pmr->n.anMove, FALSE);
            PositionKey((ConstTanBoard) anBoardMove, &key);

            if (cmp_evalsetup(pesChequer, &pmr->esChequer) > 0) {

                if (pmr->ml.cMoves) {
                    free(pmr->ml.amMoves);
                    pmr->ml.amMoves = NULL;
                }

                /* find best moves */

                {
                    movelist ml;
                    MT_Release();
                    if (FindnSaveBestMoves(&ml, pmr->anDice[0],
                                           pmr->anDice[1],
                                           (ConstTanBoard) pms->anBoard, &key,
                                           arSkillLevel[SKILL_DOUBTFUL], &ci, &pesChequer->ec, aamf) < 0)
                        return -1;
                    MT_Exclusive();
                    CopyMoveList(&pmr->ml, &ml);
                    if (ml.cMoves)
                        free(ml.amMoves);
                }

            }

            for (pmr->n.iMove = 0; pmr->n.iMove < pmr->ml.cMoves; pmr->n.iMove++)
                if (EqualKeys(key, pmr->ml.amMoves[pmr->n.iMove].key)) {
                    rChequerSkill = pmr->ml.amMoves[pmr->n.iMove].rScore - pmr->ml.amMoves[0].rScore;

                    break;
                }

            pmr->n.stMove = Skill(rChequerSkill);
            pmr->esChequer = *pesChequer;
        }

        if (psc)
            updateStatcontext(psc, pmr, pms, plParentGame);

        break;

    case MOVE_DOUBLE:

        /* always analyse MOVE_DOUBLEs as they are shared with the subsequent
         * MOVE_TAKEs or MOVE_DROPs. */

        dt = DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);

        if (dt != DT_NORMAL)
            break;

        /* cube action */
        if (fAnalyseCube && pmgi->fCubeUse) {
            GetMatchStateCubeInfo(&ci, pms);

            if (GetDPEq(NULL, NULL, &ci) || ci.fCubeOwner < 0 || ci.fCubeOwner == ci.fMove) {
                float arDouble[NUM_CUBEFUL_OUTPUTS];

                if (cmp_evalsetup(pesCube, &pmr->CubeDecPtr->esDouble) > 0) {
                    MT_Release();
                    if (GeneralCubeDecision(aarOutput, aarStdDev,
                                            NULL, (ConstTanBoard) pms->anBoard, &ci, pesCube, NULL, NULL) < 0)
                        return -1;
                    MT_Exclusive();

                    pmr->CubeDecPtr->esDouble = *pesCube;
                } else {
                    memcpy(aarOutput, pmr->CubeDecPtr->aarOutput, sizeof(aarOutput));
                    memcpy(aarStdDev, pmr->CubeDecPtr->aarStdDev, sizeof(aarStdDev));
                }

                FindCubeDecision(arDouble, aarOutput, &ci);
                if (doubleError)
                    *doubleError = arDouble[OUTPUT_TAKE] - arDouble[OUTPUT_DROP];

                memcpy(pmr->CubeDecPtr->aarOutput, aarOutput, sizeof(aarOutput));
                memcpy(pmr->CubeDecPtr->aarStdDev, aarStdDev, sizeof(aarStdDev));

                rSkill = arDouble[OUTPUT_TAKE] <
                    arDouble[OUTPUT_DROP] ?
                    arDouble[OUTPUT_TAKE] - arDouble[OUTPUT_OPTIMAL] : arDouble[OUTPUT_DROP] - arDouble[OUTPUT_OPTIMAL];

                pmr->stCube = Skill(rSkill);
            } else if (doubleError)
                *doubleError = (float) ERR_VAL;
        }

        if (psc)
            updateStatcontext(psc, pmr, pms, plParentGame);

        break;

    case MOVE_TAKE:

        if (analysePlayers && !analysePlayers[pmr->fPlayer])
            /* we do not analyse this player */
            break;

        tt = (taketype) DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);
        if (tt > TT_NORMAL)
            break;

        if (fAnalyseCube && pmgi->fCubeUse && doubleError && (*doubleError != (float) ERR_VAL)) {
            GetMatchStateCubeInfo(&ci, pms);
            pmr->stCube = Skill(-*doubleError);
        }

        if (psc)
            updateStatcontext(psc, pmr, pms, plParentGame);

        break;

    case MOVE_DROP:

        if (analysePlayers && !analysePlayers[pmr->fPlayer])
            /* we do not analyse this player */
            break;

        tt = (taketype) DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);
        if (tt > TT_NORMAL)
            break;

        if (fAnalyseCube && pmgi->fCubeUse && doubleError && (*doubleError != (float) ERR_VAL)) {
            GetMatchStateCubeInfo(&ci, pms);
            pmr->stCube = Skill(*doubleError);
        }

        if (psc)
            updateStatcontext(psc, pmr, pms, plParentGame);

        break;

    case MOVE_RESIGN:

        /* swap board if player not on roll resigned */

        if (pmr->fPlayer != pms->fMove) {
            SwapSides(pms->anBoard);
            pms->fMove = pmr->fPlayer;
        }

        if (analysePlayers && !analysePlayers[pmr->fPlayer])
            /* we do not analyse this player */
            break;

        if (pesCube->et != EVAL_NONE) {

            float rBefore, rAfter;

            GetMatchStateCubeInfo(&ci, pms);

            if (cmp_evalsetup(pesCube, &pmr->r.esResign) > 0) {

                getResignation(pmr->r.arResign, pms->anBoard, &ci, pesCube);

            }

            getResignEquities(pmr->r.arResign, &ci, pmr->r.nResigned, &rBefore, &rAfter);

            pmr->r.esResign = *pesCube;

            pmr->r.stResign = pmr->r.stAccept = SKILL_NONE;

            if (rAfter < rBefore) {
                /* wrong resign */
                pmr->r.stResign = Skill(rAfter - rBefore);
                pmr->r.stAccept = SKILL_NONE;
            }

            if (rBefore < rAfter) {
                /* wrong accept */
                pmr->r.stAccept = Skill(rBefore - rAfter);
                pmr->r.stResign = SKILL_NONE;
            }


        }

        break;

    case MOVE_SETDICE:
        if (pmr->fPlayer != pms->fMove) {
            SwapSides(pms->anBoard);
            pms->fMove = pmr->fPlayer;
        }

        if (analysePlayers && !analysePlayers[pmr->fPlayer])
            /* we do not analyse this player */
            break;

        GetMatchStateCubeInfo(&ci, pms);

        if (fAnalyseDice) {
            pmr->rLuck = LuckAnalysis((ConstTanBoard) pms->anBoard, pmr->anDice[0], pmr->anDice[1], pms);
            pmr->lt = Luck(pmr->rLuck);
        }

        if (psc)
            updateStatcontext(psc, pmr, pms, plParentGame);

        break;

    case MOVE_SETBOARD:
    case MOVE_SETCUBEVAL:
    case MOVE_SETCUBEPOS:
        break;
    }

    ApplyMoveRecord(pms, plParentGame, pmr);

    if (psc) {
        psc->fMoves = fAnalyseMove;
        psc->fCube = fAnalyseCube;
        psc->fDice = fAnalyseDice;
    }
    MT_Release();

    return fInterrupt ? -1 : 0;
}

static int
NumberMovesGame(listOLD * plGame)
{

    int nMoves = 0;
    listOLD *pl;

    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext)
        nMoves++;

    return nMoves;

}


static gboolean
UpdateProgressBar(gpointer UNUSED(unused))
{
    ProgressValue(MT_GetDoneTasks());
    return TRUE;
}

static void
AnalyseMoveMT(Task * task)
{
    AnalyseMoveTask *amt;
    float doubleError = 0.0f;

  analyzeDouble:
    amt = (AnalyseMoveTask *) task;
    if (AnalyzeMove(amt->pmr, &amt->ms, amt->plGame, amt->psc,
                    &esAnalysisChequer, &esAnalysisCube, aamfAnalysis, afAnalysePlayers, &doubleError) < 0)
        MT_AbortTasks();

    if (task->pLinkedTask) {    /* Need to analyze take/drop decision in sequence */
        task = task->pLinkedTask;
        goto analyzeDouble;
    }
}

static int
AnalyzeGame(listOLD * plGame, int wait)
{
    int result;
    unsigned int i;
    listOLD *pl = plGame->plNext;
    moverecord *pmr = pl->p;
    statcontext *psc = &pmr->g.sc;
    matchstate msAnalyse;
    unsigned int numMoves = NumberMovesGame(plGame);
    AnalyseMoveTask *pt = NULL, *pParentTask = NULL;

    /* Analyse first move record (gameinfo) */
    g_assert(pmr->mt == MOVE_GAMEINFO);
    if (AnalyzeMove(pmr, &msAnalyse, plGame, psc,
                    &esAnalysisChequer, &esAnalysisCube, aamfAnalysis, afAnalysePlayers, NULL) < 0)
        return -1;              /* Interrupted */

    numMoves--;                 /* Done one - the gameinfo */


    for (i = 0; i < numMoves; i++) {
        pl = pl->plNext;
        pmr = pl->p;

        if (pmr == NULL) {
            /* corrupt moves list */
            g_assert_not_reached();
            break;
        }

        if (!pParentTask)
            pt = (AnalyseMoveTask *) malloc(sizeof(AnalyseMoveTask));

        pt->task.fun = (AsyncFun) AnalyseMoveMT;
        pt->task.data = pt;
        pt->task.pLinkedTask = NULL;
        pt->pmr = pmr;
        pt->plGame = plGame;
        pt->psc = psc;
        memcpy(&pt->ms, &msAnalyse, sizeof(msAnalyse));

        if (pmr->mt == MOVE_DOUBLE) {
            doubletype dt = DoubleType(msAnalyse.fDoubled, msAnalyse.fMove, msAnalyse.fTurn);
            moverecord *pNextmr = (moverecord *) pl->plNext->p;
            if (pNextmr && dt == DT_NORMAL) {   /* Need to link the two tasks so executed together */
                pParentTask = pt;
                pt = (AnalyseMoveTask *) malloc(sizeof(AnalyseMoveTask));
                pParentTask->task.pLinkedTask = (Task *) pt;
            }
        } else {
            if (pParentTask) {
                pt = pParentTask;
                pParentTask = NULL;
            }
            multi_debug("add task: analysis");
            MT_AddTask((Task *) pt, TRUE);
        }

        FixMatchState(&msAnalyse, pmr);
        if ((pmr->fPlayer != msAnalyse.fMove)
            && (pmr->mt == MOVE_NORMAL || pmr->mt == MOVE_RESIGN || pmr->mt == MOVE_SETDICE)) {
            SwapSides(msAnalyse.anBoard);
            msAnalyse.fMove = pmr->fPlayer;
        }
        ApplyMoveRecord(&msAnalyse, plGame, pmr);
    }
    g_assert(pl->plNext == plGame);

    if (wait) {
        multi_debug("wait for all task: analysis");
        result = MT_WaitForTasks(UpdateProgressBar, 250, fAutoSaveAnalysis);

        if (result == -1)
            IniStatcontext(psc);

        return result;
    } else
        return 0;
}



static void
UpdateVariance(float *prVariance, const float rSum, const float rSumAdd, const int nGames)
{

    if (!nGames || nGames == 1) {
        *prVariance = 0;
        return;
    } else {

        /* See <URL:http://mathworld.wolfram.com/SampleVarianceComputation.html>
         * for formula */

        float rDelta = rSumAdd;
        float rMuNew = rSum / nGames;
        float rMuOld = (rSum - rDelta) / (nGames - 1);
        float rDeltaMu = rMuNew - rMuOld;

        *prVariance = *prVariance * (1.0f - 1.0f / (nGames - 1.0f)) + nGames * rDeltaMu * rDeltaMu;

        return;

    }

}

extern void
AddStatcontext(const statcontext * pscA, statcontext * pscB)
{

    /* pscB = pscB + pscA */

    int i, j;

    pscB->nGames++;

    pscB->fMoves |= pscA->fMoves;
    pscB->fDice |= pscA->fDice;
    pscB->fCube |= pscA->fCube;

    for (i = 0; i < 2; i++) {

        pscB->anUnforcedMoves[i] += pscA->anUnforcedMoves[i];
        pscB->anTotalMoves[i] += pscA->anTotalMoves[i];

        pscB->anTotalCube[i] += pscA->anTotalCube[i];
        pscB->anCloseCube[i] += pscA->anCloseCube[i];
        pscB->anDouble[i] += pscA->anDouble[i];
        pscB->anTake[i] += pscA->anTake[i];
        pscB->anPass[i] += pscA->anPass[i];

        for (j = 0; j < N_SKILLS; j++)
            pscB->anMoves[i][j] += pscA->anMoves[i][j];

        for (j = 0; j < N_LUCKS; j++)
            pscB->anLuck[i][j] += pscA->anLuck[i][j];

        pscB->anCubeMissedDoubleDP[i] += pscA->anCubeMissedDoubleDP[i];
        pscB->anCubeMissedDoubleTG[i] += pscA->anCubeMissedDoubleTG[i];
        pscB->anCubeWrongDoubleDP[i] += pscA->anCubeWrongDoubleDP[i];
        pscB->anCubeWrongDoubleTG[i] += pscA->anCubeWrongDoubleTG[i];
        pscB->anCubeWrongTake[i] += pscA->anCubeWrongTake[i];
        pscB->anCubeWrongPass[i] += pscA->anCubeWrongPass[i];

        for (j = 0; j < 2; j++) {

            pscB->arErrorCheckerplay[i][j] += pscA->arErrorCheckerplay[i][j];
            pscB->arErrorMissedDoubleDP[i][j] += pscA->arErrorMissedDoubleDP[i][j];
            pscB->arErrorMissedDoubleTG[i][j] += pscA->arErrorMissedDoubleTG[i][j];
            pscB->arErrorWrongDoubleDP[i][j] += pscA->arErrorWrongDoubleDP[i][j];
            pscB->arErrorWrongDoubleTG[i][j] += pscA->arErrorWrongDoubleTG[i][j];
            pscB->arErrorWrongTake[i][j] += pscA->arErrorWrongTake[i][j];
            pscB->arErrorWrongPass[i][j] += pscA->arErrorWrongPass[i][j];
            pscB->arLuck[i][j] += pscA->arLuck[i][j];

        }

    }

    if (pscA->arActualResult[0] >= 0.0f || pscA->arActualResult[1] >= 0.0f) {
        /* actual result is calculated */

        for (i = 0; i < 2; ++i) {
            /* separate loop, else arLuck[ 1 ] is not calculated for i=0 */

            pscB->arActualResult[i] += pscA->arActualResult[i];
            pscB->arLuckAdj[i] += pscA->arLuckAdj[i];
            UpdateVariance(&pscB->arVarianceActual[i], pscB->arActualResult[i], pscA->arActualResult[i], pscB->nGames);
            UpdateVariance(&pscB->arVarianceLuckAdj[i], pscB->arLuckAdj[i], pscA->arLuckAdj[i], pscB->nGames);


        }

    }

}

static int
CheckSettings(void)
{

    if (!fAnalyseCube && !fAnalyseDice && !fAnalyseMove) {
        outputl(_("No analysis selected, you must specify at least one type of analysis to perform"));
        return -1;
    }

    return 0;
}


static int
NumberMovesMatch(listOLD * plMatch)
{

    int nMoves = 0;
    listOLD *pl;

    for (pl = plMatch->plNext; pl != plMatch; pl = pl->plNext)
        nMoves += NumberMovesGame(pl->p);

    return nMoves;

}

extern void
CommandAnalyseGame(char *UNUSED(sz))
{
    int nMoves;
    int fStore_crawford;

    if (!CheckGameExists())
        return;

    if (CheckSettings())
        return;

    fStore_crawford = ms.fCrawford;
    nMoves = NumberMovesGame(plGame);

    ProgressStartValue(_("Analysing game; move:"), nMoves);

    AnalyzeGame(plGame, TRUE);

    ProgressEnd();

#if USE_GTK
    if (fX)
        ChangeGame(NULL);
#endif
    ms.fCrawford = fStore_crawford;

    playSound(SOUND_ANALYSIS_FINISHED);

}


extern void
CommandAnalyseMatch(char *UNUSED(sz))
{
    listOLD *pl;
    moverecord *pmr;
    int nMoves;
    int fStore_crawford;

    if (!CheckGameExists())
        return;

    if (CheckSettings())
        return;

    fStore_crawford = ms.fCrawford;
    nMoves = NumberMovesMatch(&lMatch);

    ProgressStartValue(_("Analysing match; move:"), nMoves);

    IniStatcontext(&scMatch);

    for (pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext) {

        if (AnalyzeGame(pl->p, FALSE) < 0) {
            /* analysis incomplete; erase partial summary */

            IniStatcontext(&scMatch);
            break;
        }
        pmr = (moverecord *) ((listOLD *) pl->p)->plNext->p;
        g_assert(pmr->mt == MOVE_GAMEINFO);
        AddStatcontext(&pmr->g.sc, &scMatch);
    }

    multi_debug("wait for all task: analysis");
    MT_WaitForTasks(UpdateProgressBar, 250, fAutoSaveAnalysis);

    ProgressEnd();

#if USE_GTK
    if (fX)
        ChangeGame(NULL);
#endif
    ms.fCrawford = fStore_crawford;

    playSound(SOUND_ANALYSIS_FINISHED);
}

extern void
CommandAnalyseSession(char *sz)
{

    CommandAnalyseMatch(sz);
}



extern void
IniStatcontext(statcontext * psc)
{

    /* Initialize statcontext with all zeroes */

    int i, j;

    psc->fMoves = psc->fCube = psc->fDice = FALSE;

    for (i = 0; i < 2; i++) {

        psc->anUnforcedMoves[i] = 0;
        psc->anTotalMoves[i] = 0;

        psc->anTotalCube[i] = 0;
        psc->anCloseCube[i] = 0;
        psc->anDouble[i] = 0;
        psc->anTake[i] = 0;
        psc->anPass[i] = 0;

        for (j = 0; j < N_SKILLS; j++)
            psc->anMoves[i][j] = 0;

        for (j = 0; j <= LUCK_VERYGOOD; j++)
            psc->anLuck[i][j] = 0;

        psc->anCubeMissedDoubleDP[i] = 0;
        psc->anCubeMissedDoubleTG[i] = 0;
        psc->anCubeWrongDoubleDP[i] = 0;
        psc->anCubeWrongDoubleTG[i] = 0;
        psc->anCubeWrongTake[i] = 0;
        psc->anCubeWrongPass[i] = 0;

        for (j = 0; j < 2; j++) {

            psc->arErrorCheckerplay[i][j] = 0.0;
            psc->arErrorMissedDoubleDP[i][j] = 0.0;
            psc->arErrorMissedDoubleTG[i][j] = 0.0;
            psc->arErrorWrongDoubleDP[i][j] = 0.0;
            psc->arErrorWrongDoubleTG[i][j] = 0.0;
            psc->arErrorWrongTake[i][j] = 0.0;
            psc->arErrorWrongPass[i][j] = 0.0;
            psc->arLuck[i][j] = 0.0;

        }

        psc->arActualResult[i] = 0.0f;
        psc->arLuckAdj[i] = 0.0f;
        psc->arVarianceActual[i] = 0.0f;
        psc->arVarianceLuckAdj[i] = 0.0f;

    }

    psc->nGames = 0;

}



extern float
relativeFibsRating(float r, int n)
{
    float const x = -2000.0f / (float) sqrt(1.0 * n) * (float) log10(1.0 / r - 1.0);

    return (x < -2100) ? -2100 : x;
}

/*
 * Calculated the amount of rating lost by chequer play errors.
 *
 * a2(N) * EPM
 *
 * where a2(N) = 8798 + 25526/N
 *
 */

extern float
absoluteFibsRatingChequer(const float rChequer, const int n)
{

    return rChequer * (8798.0f + 25526.0f / (n));

}


/*
 * Calculated the amount of rating lost by cube play errors.
 *
 * b(N) * EPM
 *
 * where b(N) = 863 - 519/N.
 *
 */

extern float
absoluteFibsRatingCube(const float rCube, const int n)
{

    return rCube * (863.0f - 519.0f / n);

}


/*
 * Calculate an estimated rating based Kees van den Doels work.
 * (see manual for details)
 *
 * absolute rating = R0 + a2(N)*EPM+b(N)*EPC,
 * where EPM is error rate per move, EPC is the error per cubedecision
 * and a2(N) = 8798 + 2526/N and b(N) = 863 - 519/N.
 *
 */

extern float
absoluteFibsRating(const float rChequer, const float rCube, const int n, const float rOffset)
{

    return rOffset - (absoluteFibsRatingChequer(rChequer, n) + absoluteFibsRatingCube(rCube, n));

}


extern void
getMWCFromError(const statcontext * psc, float aaaar[3][2][2][2])
{

    int i, j;

    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++) {

            /* chequer play */

            aaaar[CHEQUERPLAY][TOTAL][i][j] = psc->arErrorCheckerplay[i][j];

            if (psc->anUnforcedMoves[i])
                aaaar[CHEQUERPLAY][PERMOVE][i][j] = aaaar[0][0][i][j] / psc->anUnforcedMoves[i];
            else
                aaaar[CHEQUERPLAY][PERMOVE][i][j] = 0.0f;

            /* cube decisions */

            aaaar[CUBEDECISION][TOTAL][i][j] = psc->arErrorMissedDoubleDP[i][j]
                + psc->arErrorMissedDoubleTG[i][j]
                + psc->arErrorWrongDoubleDP[i][j]
                + psc->arErrorWrongDoubleTG[i][j]
                + psc->arErrorWrongTake[i][j]
                + psc->arErrorWrongPass[i][j];

            if (psc->anCloseCube[i])
                aaaar[CUBEDECISION][PERMOVE][i][j] = aaaar[CUBEDECISION][TOTAL][i][j] / psc->anCloseCube[i];
            else
                aaaar[CUBEDECISION][PERMOVE][i][j] = 0.0f;

            /* sum chequer play and cube decisions */
            /* FIXME: what average should be used? */

            aaaar[COMBINED][TOTAL][i][j] = aaaar[CHEQUERPLAY][TOTAL][i][j] + aaaar[CUBEDECISION][TOTAL][i][j];


            if (psc->anUnforcedMoves[i] + psc->anCloseCube[i])
                aaaar[COMBINED][PERMOVE][i][j] =
                    aaaar[COMBINED][TOTAL][i][j] / (psc->anUnforcedMoves[i] + psc->anCloseCube[i]);
            else
                aaaar[COMBINED][PERMOVE][i][j] = 0.0f;

        }
}

extern void
DumpStatcontext(char *szOutput, const statcontext * psc, const char *pl, const char *op, int nMatchTo)
{

    /* header */

    sprintf(szOutput, "%-40s %-23s %-23s\n\n", _("Player"), pl, op);

    if (psc->fMoves) {
        GList *list = formatGS(psc, nMatchTo, FORMATGS_CHEQUER);
        GList *pl;

        strcat(szOutput, _("Chequerplay statistics"));
        strcat(szOutput, "\n\n");

        for (pl = g_list_first(list); pl; pl = g_list_next(pl)) {

            char **asz = pl->data;

            sprintf(strchr(szOutput, 0), "%-40s %-23s %-23s\n", asz[0], asz[1], asz[2]);

        }

        strcat(szOutput, "\n\n");

        freeGS(list);

    }


    if (psc->fDice) {
        GList *list = formatGS(psc, nMatchTo, FORMATGS_LUCK);
        GList *pl;

        strcat(szOutput, _("Luck statistics"));
        strcat(szOutput, "\n\n");

        for (pl = g_list_first(list); pl; pl = g_list_next(pl)) {

            char **asz = pl->data;

            sprintf(strchr(szOutput, 0), "%-40s %-23s %-23s\n", asz[0], asz[1], asz[2]);

        }

        strcat(szOutput, "\n\n");

        freeGS(list);

    }


    if (psc->fCube) {
        GList *list = formatGS(psc, nMatchTo, FORMATGS_CUBE);
        GList *pl;

        strcat(szOutput, _("Cube statistics"));
        strcat(szOutput, "\n\n");

        for (pl = g_list_first(list); pl; pl = g_list_next(pl)) {

            char **asz = pl->data;

            sprintf(strchr(szOutput, 0), "%-40s %-23s %-23s\n", asz[0], asz[1], asz[2]);

        }

        strcat(szOutput, "\n\n");

        freeGS(list);

    }

    {

        GList *list = formatGS(psc, nMatchTo, FORMATGS_OVERALL);
        GList *pl;

        strcat(szOutput, _("Overall statistics"));
        strcat(szOutput, "\n\n");

        for (pl = g_list_first(list); pl; pl = g_list_next(pl)) {

            char **asz = pl->data;

            sprintf(strchr(szOutput, 0), "%-40s %-23s %-23s\n", asz[0], asz[1], asz[2]);

        }

        strcat(szOutput, "\n\n");
        freeGS(list);
    }
}

extern void
CommandShowStatisticsMatch(char *UNUSED(sz))
{
    char szOutput[4096];

    updateStatisticsMatch(&lMatch);

#if USE_GTK
    if (fX) {
        GTKDumpStatcontext(0);
        return;
    }
#endif

    DumpStatcontext(szOutput, &scMatch, ap[0].szName, ap[1].szName, ms.nMatchTo);
    outputl(szOutput);
}


extern void
CommandShowStatisticsSession(char *sz)
{

    CommandShowStatisticsMatch(sz);

}


extern void
CommandShowStatisticsGame(char *UNUSED(sz))
{

    moverecord *pmr;
    char szOutput[4096];

    if (!CheckGameExists())
        return;

    updateStatisticsGame(plGame);

    pmr = plGame->plNext->p;

    g_assert(pmr->mt == MOVE_GAMEINFO);

#if USE_GTK
    if (fX) {
        GTKDumpStatcontext(getGameNumber(plGame) + 1);
        return;
    }
#endif

    DumpStatcontext(szOutput, &pmr->g.sc, ap[0].szName, ap[1].szName, ms.nMatchTo);
    outputl(szOutput);
}


extern void
CommandAnalyseMove(char *UNUSED(sz))
{
    if (!CheckGameExists())
        return;

    if (plLastMove && plLastMove->plNext && plLastMove->plNext->p) {    /* analyse move */
        moveData md;
        matchstate msx;

        md.pmr = plLastMove->plNext->p;

        if (md.pmr->mt == MOVE_TAKE) {
            outputerrf("%s", _("Please use 'analyse move' on the double decision"));
            return;
        }

        memcpy(&msx, &ms, sizeof(matchstate));
        md.pms = &msx;
        md.pesChequer = &esAnalysisChequer;
        md.pesCube = &esAnalysisCube;
        md.aamf = aamfAnalysis;
        RunAsyncProcess((AsyncFun) asyncAnalyzeMove, &md, _("Analysing move..."));

#if USE_GTK
        if (fX)
            ChangeGame(NULL);
#endif
    } else
        outputerrf("%s", _("Please use `hint' on unfinished moves"));
}

static void
updateStatisticsMove(const moverecord * pmr, matchstate * pms, const listOLD * plGame, statcontext * psc)
{
    FixMatchState(pms, pmr);

    switch (pmr->mt) {
    case MOVE_GAMEINFO:
        IniStatcontext(psc);
        updateStatcontext(psc, pmr, pms, plGame);
        break;

    case MOVE_NORMAL:
        if (pmr->fPlayer != pms->fMove) {
            SwapSides(pms->anBoard);
            pms->fMove = pmr->fPlayer;
        }

        updateStatcontext(psc, pmr, pms, plGame);
        break;

    case MOVE_DOUBLE:
        if (pmr->fPlayer != pms->fMove) {
            SwapSides(pms->anBoard);
            pms->fMove = pmr->fPlayer;
        }


        updateStatcontext(psc, pmr, pms, plGame);
        break;

    case MOVE_TAKE:
    case MOVE_DROP:

        updateStatcontext(psc, pmr, pms, plGame);
        break;

    default:
        break;

    }

    ApplyMoveRecord(pms, plGame, pmr);

    psc->fMoves = fAnalyseMove;
    psc->fCube = fAnalyseCube;
    psc->fDice = fAnalyseDice;

}



extern void
updateStatisticsGame(const listOLD * plGame)
{

    listOLD *pl;
    moverecord *pmr;
    moverecord *pmrx = plGame->plNext->p;
    matchstate msAnalyse;

    g_assert(pmrx->mt == MOVE_GAMEINFO);

    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext) {

        pmr = pl->p;

        updateStatisticsMove(pmr, &msAnalyse, plGame, &pmrx->g.sc);

    }

}





extern void
updateStatisticsMatch(listOLD * plMatch)
{

    listOLD *pl;
    moverecord *pmr;

    if (ListEmpty(plMatch))
        /* no match in progress */
        return;

    IniStatcontext(&scMatch);

    for (pl = plMatch->plNext; pl != plMatch; pl = pl->plNext) {

        updateStatisticsGame(pl->p);

        pmr = ((listOLD *) pl->p)->plNext->p;
        g_assert(pmr->mt == MOVE_GAMEINFO);
        AddStatcontext(&pmr->g.sc, &scMatch);

    }
}

extern lucktype
getLuckRating(float rLuck)
{
    return Luck(rLuck * 10);
}

static void
AnalyseClearMove(moverecord * pmr)
{

    if (!pmr)
        return;

    switch (pmr->mt) {
    case MOVE_GAMEINFO:
        IniStatcontext(&pmr->g.sc);
        break;

    case MOVE_NORMAL:

        pmr->CubeDecPtr->esDouble.et = pmr->esChequer.et = EVAL_NONE;
        pmr->n.stMove = pmr->stCube = SKILL_NONE;
        pmr->rLuck = (float) ERR_VAL;
        pmr->lt = LUCK_NONE;
        if (pmr->ml.amMoves) {
            free(pmr->ml.amMoves);
            pmr->ml.amMoves = NULL;
        }
        pmr->ml.cMoves = 0;
        break;

    case MOVE_DOUBLE:
    case MOVE_TAKE:
    case MOVE_DROP:

        pmr->CubeDecPtr->esDouble.et = EVAL_NONE;
        pmr->stCube = SKILL_NONE;
        break;

    case MOVE_RESIGN:

        pmr->r.esResign.et = EVAL_NONE;
        pmr->r.stResign = pmr->r.stAccept = SKILL_NONE;
        break;

    case MOVE_SETDICE:

        pmr->lt = LUCK_NONE;
        pmr->rLuck = (float) ERR_VAL;
        break;

    default:
        /* no-op */
        break;

    }

}

static void
AnalyseClearGame(listOLD * plGame)
{

    listOLD *pl;

    if (!plGame || ListEmpty(plGame))
        return;

    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext)
        AnalyseClearMove(pl->p);

}


extern void
CommandAnalyseClearMove(char *UNUSED(sz))
{

    if (plLastMove && plLastMove->plNext && plLastMove->plNext->p) {
        AnalyseClearMove(plLastMove->plNext->p);
#if USE_GTK
        if (fX)
            ChangeGame(NULL);
#endif
    } else
        outputl(_("Cannot clear analysis on this move"));

}

extern void
CommandAnalyseClearGame(char *UNUSED(sz))
{

    if (!CheckGameExists())
        return;

    AnalyseClearGame(plGame);

#if USE_GTK
    if (fX)
        ChangeGame(NULL);
#endif

}

extern void
CommandAnalyseClearMatch(char *UNUSED(sz))
{

    listOLD *pl;

    if (!CheckGameExists())
        return;

    for (pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext)
        AnalyseClearGame(pl->p);

#if USE_GTK
    if (fX)
        ChangeGame(NULL);
#endif

}

static int
MoveAnalysed(moverecord * pmr, matchstate * pms, listOLD * plGame,
             evalsetup * UNUSED(pesChequer), evalsetup * pesCube,
             movefilter UNUSED(aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]))
{
    static TanBoard anBoardMove;
    static positionkey key;
    static cubeinfo ci;
    static evalsetup esDouble;  /* shared between the
                                 * double and subsequent take/drop */
    doubletype dt;
    taketype tt;
    const xmovegameinfo *pmgi = &((moverecord *) plGame->plNext->p)->g;
    int is_initial_position = 1;

    /* analyze this move */

    FixMatchState(pms, pmr);

    /* check if it's the initial position: no cube analysis and special
     * luck analysis */

    if (pmr->mt != MOVE_GAMEINFO) {
        InitBoard(anBoardMove, pms->bgv);
        is_initial_position = !memcmp(anBoardMove, pms->anBoard, 2 * 25 * sizeof(int));
    }

    switch (pmr->mt) {
    case MOVE_GAMEINFO:
        break;

    case MOVE_NORMAL:
        if (pmr->fPlayer != pms->fMove) {
            SwapSides(pms->anBoard);
            pms->fMove = pmr->fPlayer;
        }

        GetMatchStateCubeInfo(&ci, pms);

        /* cube action? */

        if (!is_initial_position && pmgi->fCubeUse && GetDPEq(NULL, NULL, &ci)) {
            if (pmr->CubeDecPtr->esDouble.et == EVAL_NONE)
                return FALSE;
        }

        /* luck analysis */
        if (pmr->rLuck == ERR_VAL)
            return FALSE;

        /* evaluate move */

        memcpy(anBoardMove, pms->anBoard, sizeof(anBoardMove));
        ApplyMove(anBoardMove, pmr->n.anMove, FALSE);
        PositionKey((ConstTanBoard) anBoardMove, &key);

        if (pmr->esChequer.et == EVAL_NONE && pmr->n.iMove != UINT_MAX)
            return FALSE;

        break;

    case MOVE_DOUBLE:

        /* always analyse MOVE_DOUBLEs as they are shared with the subsequent
         * MOVE_TAKEs or MOVE_DROPs. */

        dt = DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);

        if (dt != DT_NORMAL)
            break;

        /* cube action */
        if (pmgi->fCubeUse) {
            GetMatchStateCubeInfo(&ci, pms);

            if (GetDPEq(NULL, NULL, &ci) || ci.fCubeOwner < 0 || ci.fCubeOwner == ci.fMove) {
                if (pmr->CubeDecPtr->esDouble.et == EVAL_NONE)
                    return FALSE;
            }
        }

        break;

    case MOVE_TAKE:

        tt = (taketype) DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);
        if (tt > TT_NORMAL)
            break;

        if (fAnalyseCube && pmgi->fCubeUse && esDouble.et != EVAL_NONE) {
            GetMatchStateCubeInfo(&ci, pms);
            if (pmr->CubeDecPtr->esDouble.et == EVAL_NONE)
                return FALSE;
        }

        break;

    case MOVE_DROP:

        tt = (taketype) DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);
        if (tt > TT_NORMAL)
            break;

        if (pmr->CubeDecPtr->esDouble.et == EVAL_NONE)
            return FALSE;

        break;

    case MOVE_RESIGN:

        /* swap board if player not on roll resigned */
        if (pmr->fPlayer != pms->fMove) {
            SwapSides(pms->anBoard);
            pms->fMove = pmr->fPlayer;
        }

        if (pesCube->et != EVAL_NONE) {

            float rBefore, rAfter;

            GetMatchStateCubeInfo(&ci, pms);

            getResignEquities(pmr->r.arResign, &ci, pmr->r.nResigned, &rBefore, &rAfter);

            pmr->r.esResign = *pesCube;

            pmr->r.stResign = pmr->r.stAccept = SKILL_NONE;

            if (rAfter < rBefore) {
                /* wrong resign */
                pmr->r.stResign = Skill(rAfter - rBefore);
                pmr->r.stAccept = SKILL_NONE;
            }

            if (rBefore < rAfter) {
                /* wrong accept */
                pmr->r.stAccept = Skill(rBefore - rAfter);
                pmr->r.stResign = SKILL_NONE;
            }


        }

        break;

    case MOVE_SETDICE:
        if (pmr->fPlayer != pms->fMove) {
            SwapSides(pms->anBoard);
            pms->fMove = pmr->fPlayer;
        }

        if (!afAnalysePlayers[pmr->fPlayer])
            /* we do not analyse this player */
            break;

        GetMatchStateCubeInfo(&ci, pms);
        break;

    case MOVE_SETBOARD:
    case MOVE_SETCUBEVAL:
    case MOVE_SETCUBEPOS:
        break;
    default:
        g_assert_not_reached();
    }

    ApplyMoveRecord(pms, plGame, pmr);

    return TRUE;
}

static int
GameAnalysed(listOLD * plGame)
{
    listOLD *pl;
    matchstate msAnalyse;

#if !defined(G_DISABLE_ASSERT)
    moverecord *pmrx = (moverecord *) plGame->plNext->p;

    g_assert(pmrx->mt == MOVE_GAMEINFO);
#endif

    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext) {
        if (!MoveAnalysed(pl->p, &msAnalyse, plGame, &esAnalysisChequer, &esAnalysisCube, aamfAnalysis))
            return FALSE;
    }

    return TRUE;
}

extern int
MatchAnalysed(void)
{
    listOLD *pl;

    for (pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext) {
        if (!GameAnalysed(pl->p))
            return FALSE;
    }
    return TRUE;
}

static void
cmark_cube_show(GString * gsz, const matchstate * UNUSED(pms), const moverecord * pmr, int movenr)
{
    g_return_if_fail(pmr);
    g_return_if_fail(pmr->mt == MOVE_NORMAL || pmr->mt == MOVE_DOUBLE || pmr->mt == MOVE_TAKE || pmr->mt == MOVE_DROP);
    g_return_if_fail(pmr->CubeDecPtr);

    if (pmr->CubeDecPtr->cmark)
        g_string_append_printf(gsz, _("Move %d\nCube marked\n"), movenr);
}

static void
cmark_cube_set(moverecord * pmr, CMark cmark)
{
    g_return_if_fail(pmr);
    g_return_if_fail(pmr->mt == MOVE_NORMAL || pmr->mt == MOVE_DOUBLE || pmr->mt == MOVE_TAKE || pmr->mt == MOVE_DROP);
    g_return_if_fail(pmr->CubeDecPtr);

    pmr->CubeDecPtr->cmark = cmark;
}

static void
cmark_move_show(GString * gsz, const matchstate * UNUSED(pms), const moverecord * pmr, int movenr)
{
    guint i;
    gchar sz[40];
    int found = 0;

    g_return_if_fail(pmr);
    g_return_if_fail(gsz);

    for (i = 0; i < pmr->ml.cMoves; i++) {
        if (pmr->ml.amMoves[i].cmark) {
            if (!found++)
                g_string_append_printf(gsz, _("Move %d\n"), movenr);
            FormatMove(sz, msBoard(), pmr->ml.amMoves[i].anMove);
            g_string_append_printf(gsz, _("%i (%s) marked\n"), i + 1, sz);
        }
    }
}

static void
cmark_game_show(GString * gsz, listOLD * game, int game_number)
{
    listOLD *pl, *pl_hint = NULL;
    matchstate ms_local;
    moverecord *pmr;
    int movenr = 1;

    g_return_if_fail(gsz);
    g_return_if_fail(game);

    if (game_is_last(game))
        pl_hint = game_add_pmr_hint(game);

    g_string_append_printf(gsz, _("Game %d\n"), game_number);
    for (pl = game->plNext; pl != game; pl = pl->plNext) {
        pmr = pl->p;
        FixMatchState(&ms_local, pmr);
        switch (pmr->mt) {
        case MOVE_GAMEINFO:
            ApplyMoveRecord(&ms_local, game, pmr);
            break;
        case MOVE_NORMAL:
            if (pmr->fPlayer != ms_local.fMove) {
                SwapSides(ms_local.anBoard);
                ms_local.fMove = pmr->fPlayer;
            }
            ms_local.fTurn = ms_local.fMove = pmr->fPlayer;
            cmark_cube_show(gsz, &ms_local, pmr, movenr);
            ms_local.anDice[0] = pmr->anDice[0];
            ms_local.anDice[1] = pmr->anDice[1];
            cmark_move_show(gsz, &ms_local, pmr, movenr);
            movenr++;
            ApplyMoveRecord(&ms_local, game, pmr);
            break;
        case MOVE_DOUBLE:
            cmark_cube_show(gsz, &ms_local, pmr, movenr);
            movenr++;
            ApplyMoveRecord(&ms_local, game, pmr);
            break;
        case MOVE_TAKE:
        case MOVE_DROP:
            movenr++;
            ApplyMoveRecord(&ms_local, game, pmr);
            break;
        default:
            ApplyMoveRecord(&ms_local, game, pmr);
            break;
        }
    }
    if (pl_hint)
        game_remove_pmr_hint(pl_hint);
}

static void
cmark_match_show(GString * gsz, const listOLD * match)
{
    listOLD *pl;
    int game_number = 1;

    for (pl = match->plNext; pl != match; pl = pl->plNext) {
        cmark_game_show(gsz, pl->p, game_number++);
    }
}

static void
cmark_move_set(moverecord * pmr, gchar * sz, CMark cmark)
{
    gint c;
    gint n;
    GSList *list = NULL, *pl = NULL;

    g_return_if_fail(sz);
    g_return_if_fail(pmr);
    g_return_if_fail(pmr->ml.cMoves);

    c = pmr->ml.cMoves;

    /* Mike Petch - This is a hack and is not thread safe under GLIB version < 2.12.5
     * Was added to allow cygwin to compile */

#if GLIB_CHECK_VERSION(2,12,5)
    while ((n = (int) g_ascii_strtoll(sz, &sz, 10)) != 0) {
#else
    while ((n = (int) strtoll(sz, &sz, 10)) != 0) {
#endif
        if (n > c) {
            outputerrf("Only %d moves in movelist\n", c);
            g_slist_free(list);
            return;
        }
        if (!g_slist_find(list, GINT_TO_POINTER(n)))
            list = g_slist_append(list, GINT_TO_POINTER(n));
    }
    if (g_slist_length(list) == 0) {
        outputerrf("Not a valid list of moves\n");
        return;
    }

    for (pl = list; pl; pl = g_slist_next(pl)) {
        gint i = GPOINTER_TO_INT(pl->data) - 1;
        pmr->ml.amMoves[i].cmark = cmark;
    }
    g_slist_free(list);
}

static void
cmark_move_clear(moverecord * pmr)
{
    guint j;
    g_return_if_fail(pmr);

    for (j = 0; j < pmr->ml.cMoves; j++)
        pmr->ml.amMoves[j].cmark = CMARK_NONE;
}

static void
cmark_game_clear(listOLD * game)
{
    listOLD *pl, *pl_hint = NULL;

    g_return_if_fail(game);

    if (game_is_last(game))
        pl_hint = game_add_pmr_hint(game);

    for (pl = game->plNext; pl != game; pl = pl->plNext) {
        moverecord *pmr = pl->p;

        if (!pmr)
            continue;

        switch (pmr->mt) {
        case MOVE_NORMAL:
            cmark_move_clear(pmr);
            cmark_cube_set(pmr, CMARK_NONE);
            break;
        case MOVE_DOUBLE:
            cmark_cube_set(pmr, CMARK_NONE);
            break;
        default:
            break;
        }
    }
    if (pl_hint)
        game_remove_pmr_hint(pl_hint);

}

static void
cmark_match_clear(listOLD * match)
{
    listOLD *pl;

    for (pl = match->plNext; pl != match; pl = pl->plNext) {
        cmark_game_clear(pl->p);
    }
}

static int
cmark_move_rollout(moverecord * pmr, gboolean destroy)
{
    gchar(*asz)[40];
    cubeinfo ci;
    cubeinfo **ppci;
    GSList *pl = NULL;
    gint c;
    guint j;
    gint res;
    move *m;
    move **ppm;
    void *p;
    GSList *list = NULL;
    positionkey key = { {0, 0, 0, 0, 0, 0, 0} };

    g_return_val_if_fail(pmr, -1);

    for (j = 0; j < pmr->ml.cMoves; j++) {
        if (pmr->ml.amMoves[j].cmark == CMARK_ROLLOUT)
            list = g_slist_append(list, GINT_TO_POINTER(j));
    }

    if ((c = g_slist_length(list)) == 0) {
        return 0;
    }

    ppm = g_new(move *, c);
    ppci = g_new(cubeinfo *, c);
    asz = (char (*)[40]) g_malloc(40 * c);
    if (pmr->n.iMove != UINT_MAX)
        CopyKey(pmr->ml.amMoves[pmr->n.iMove].key, key);
    GetMatchStateCubeInfo(&ci, &ms);

    for (pl = list, j = 0; pl; pl = g_slist_next(pl), j++) {
        gint i = GPOINTER_TO_INT(pl->data);
        m = ppm[j] = &pmr->ml.amMoves[i];
        ppci[j] = &ci;
        FormatMove(asz[j], msBoard(), m->anMove);
    }

    RolloutProgressStart(&ci, c, NULL, &rcRollout, asz, TRUE, &p);
    ScoreMoveRollout(ppm, ppci, c, RolloutProgress, p);
    res = RolloutProgressEnd(&p, destroy);

    g_free(asz);
    g_free(ppm);
    g_free(ppci);

    RefreshMoveList(&pmr->ml, NULL);

    if (pmr->n.iMove != UINT_MAX)
        for (pmr->n.iMove = 0; pmr->n.iMove < pmr->ml.cMoves; pmr->n.iMove++)
            if (EqualKeys(key, pmr->ml.amMoves[pmr->n.iMove].key)) {
                pmr->n.stMove = Skill(pmr->ml.amMoves[pmr->n.iMove].rScore - pmr->ml.amMoves[0].rScore);

                break;
            }
#if USE_GTK
    if (fX)
        ChangeGame(NULL);
    else
#endif
        ShowBoard();
    return res == 0 ? c : res;
}

static evalsetup *
setup_cube_rollout(evalsetup * pes, moverecord * pmr,
                   float aarOutput[][NUM_ROLLOUT_OUTPUTS], float aarStdDev[][NUM_ROLLOUT_OUTPUTS])
{
    if (pes->et != EVAL_ROLLOUT) {
        pes->rc = rcRollout;
        pes->rc.nGamesDone = 0;
    } else {
        pes->rc.nTrials = rcRollout.nTrials;
        pes->rc.fStopOnSTD = rcRollout.fStopOnSTD;
        pes->rc.nMinimumGames = rcRollout.nMinimumGames;
        pes->rc.rStdLimit = rcRollout.rStdLimit;
        memcpy(aarOutput, pmr->CubeDecPtr->aarOutput, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));
        memcpy(aarStdDev, pmr->CubeDecPtr->aarStdDev, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    }
    return pes;
}

static int
cmark_cube_rollout(moverecord * pmr, gboolean destroy)
{
    evalsetup *pes;
    cubeinfo ci;
    float aarOutput[2][NUM_ROLLOUT_OUTPUTS];
    float aarStdDev[2][NUM_ROLLOUT_OUTPUTS];
    rolloutstat aarsStatistics[2][2];
    gchar asz[2][40];
    void *p;
    int res;

    if (!pmr->CubeDecPtr || pmr->CubeDecPtr->cmark != CMARK_ROLLOUT)
        return 0;

    pes = setup_cube_rollout(&pmr->CubeDecPtr->esDouble, pmr, aarOutput, aarStdDev);

    GetMatchStateCubeInfo(&ci, &ms);

    FormatCubePositions(&ci, asz);
    RolloutProgressStart(&ci, 2, aarsStatistics, &pes->rc, asz, TRUE, &p);
    GeneralCubeDecisionR(aarOutput, aarStdDev, aarsStatistics,
                         (ConstTanBoard) msBoard(), &ci, &pes->rc, pes, RolloutProgress, p);

    res = RolloutProgressEnd(&p, destroy);

    memcpy(pmr->CubeDecPtr->aarOutput, aarOutput, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    memcpy(pmr->CubeDecPtr->aarStdDev, aarStdDev, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));

    if (pes->et != EVAL_ROLLOUT)
        memcpy(&pmr->CubeDecPtr->esDouble.rc, &rcRollout, sizeof(rcRollout));

    pmr->CubeDecPtr->esDouble.et = EVAL_ROLLOUT;

#if USE_GTK
    if (fX)
        ChangeGame(NULL);
#endif
    ShowBoard();
    return res;
}

static int
move_change(listOLD * new_game, const listOLD * new_move)
{
    g_return_val_if_fail(new_game, FALSE);
    g_return_val_if_fail(new_move, FALSE);

    if (plGame != new_game)
        ChangeGame(new_game);

    if (plLastMove == new_move)
        return 1;
    while (plLastMove->plNext->p && plLastMove != new_move) {
        plLastMove = plLastMove->plNext;
        FixMatchState(&ms, plLastMove->p);
        ApplyMoveRecord(&ms, plGame, plLastMove->p);
    }
    UpdateGame(FALSE);

    if (plLastMove->plNext && plLastMove->plNext->p)
        FixMatchState(&ms, plLastMove->plNext->p);

    SetMoveRecord(plLastMove->p);
    return (plLastMove == new_move);
}

static int
cmark_game_rollout(listOLD * game)
{
    listOLD *pl, *pl_hint = NULL;

    g_return_val_if_fail(game, -1);

    if (game_is_last(game))
        pl_hint = game_add_pmr_hint(game);

    ChangeGame(game);

    for (pl = game->plNext; pl != game; pl = pl->plNext) {
        moverecord *pmr_prev;
        moverecord *pmr = pl->p;

        if (!pmr)
            continue;

        switch (pmr->mt) {
        case MOVE_NORMAL:
            if (!move_change(game, pl->plPrev))
                goto finished;
            if (cmark_move_rollout(pmr, TRUE) < -1)
                goto finished;
            if (cmark_cube_rollout(pmr, TRUE) < -1)
                goto finished;
            break;
        case MOVE_DOUBLE:
            pmr_prev = game->plPrev->p;
            if (pmr_prev->mt == MOVE_DOUBLE)
                break;
            if (!move_change(game, pl->plPrev))
                goto finished;
            if (cmark_cube_rollout(pmr, TRUE) < -1)
                goto finished;
            break;
        default:
            break;
        }
    }
    if (pl_hint)
        game_remove_pmr_hint(pl_hint);
    return 0;
  finished:
    if (pl_hint)
        game_remove_pmr_hint(pl_hint);
    return -1;
}

static void
cmark_match_rollout(listOLD * match)
{
    listOLD *pl;

    for (pl = match->plNext; pl != match; pl = pl->plNext) {
        if (cmark_game_rollout(pl->p) < 0)
            break;
    }
}

static gint
check_cube_in_pmr(const moverecord * pmr)
{
    if (!pmr) {
        outputerrf(_("No moverecord stored for this cube."));
        return 0;
    }

    if (pmr->mt != MOVE_NORMAL && pmr->mt != MOVE_DOUBLE && pmr->mt != MOVE_TAKE && pmr->mt != MOVE_DROP) {
        outputerrf(_("This move doesn't imply a cubeaction. Cannot mark."));
        return 0;
    }
    return 1;
}

static gint
check_cmoves_in_pmr(const moverecord * pmr)
{
    gint c;

    if (!pmr) {
        outputerrf(_("No moverecord stored for this move."));
        return 0;
    }

    if (pmr->mt != MOVE_NORMAL) {
        outputerrf(_("This is not a normal chequer move. " "Cannot mark."));
        return 0;
    }

    if ((c = pmr->ml.cMoves) == 0) {
        outputerrf(_("No moves to analyse"));
        return 0;
    }

    return c;
}

extern void
CommandCMarkCubeShow(char *UNUSED(sz))
{
    GString *gsz;
    moverecord *pmr = get_current_moverecord(NULL);

    if (!check_cube_in_pmr(pmr))
        return;

    gsz = g_string_new(NULL);
    cmark_cube_show(gsz, &ms, pmr, getMoveNumber(plGame, pmr));
    outputf("%s", gsz->str);
    g_string_free(gsz, TRUE);
}

extern void
CommandCMarkCubeSetNone(char *UNUSED(sz))
{
    moverecord *pmr = get_current_moverecord(NULL);

    if (!check_cube_in_pmr(pmr))
        return;

    cmark_cube_set(pmr, CMARK_NONE);
}

extern void
CommandCMarkCubeSetRollout(char *UNUSED(sz))
{
    moverecord *pmr = get_current_moverecord(NULL);

    if (!check_cube_in_pmr(pmr))
        return;

    cmark_cube_set(pmr, CMARK_ROLLOUT);
}

extern void
CommandCMarkMoveClear(char *UNUSED(sz))
{
    moverecord *pmr = get_current_moverecord(NULL);

    if (!check_cmoves_in_pmr(pmr))
        return;

    cmark_move_clear(pmr);
}

extern void
CommandCMarkGameClear(char *UNUSED(sz))
{
    if (!CheckGameExists())
        return;

    cmark_game_clear(plGame);
}

extern void
CommandCMarkMatchClear(char *UNUSED(sz))
{
    if (!CheckGameExists())
        return;
    cmark_match_clear(&lMatch);
}

extern void
CommandCMarkMoveSetNone(char *sz)
{
    moverecord *pmr = get_current_moverecord(NULL);

    if (!check_cmoves_in_pmr(pmr))
        return;

    if (sz && *sz)
        cmark_move_set(pmr, sz, CMARK_NONE);
    else
        outputerrf(_("`cmark move set none' requires a list of moves to set"));
}

extern void
CommandCMarkMoveSetRollout(char *sz)
{
    moverecord *pmr = get_current_moverecord(NULL);

    if (!check_cmoves_in_pmr(pmr))
        return;

    if (sz && *sz)
        cmark_move_set(pmr, sz, CMARK_ROLLOUT);
    else
        outputerrf(_("`cmark move set rollout' requires a list of moves to set"));
}

extern void
CommandCMarkMoveShow(char *UNUSED(sz))
{
    GString *gsz;
    moverecord *pmr = get_current_moverecord(NULL);

    if (!check_cmoves_in_pmr(pmr))
        return;

    gsz = g_string_new(NULL);
    cmark_move_show(gsz, &ms, pmr, getMoveNumber(plGame, pmr));
    outputf("%s", gsz->str);
    g_string_free(gsz, TRUE);
}

extern void
CommandCMarkGameShow(char *UNUSED(sz))
{
    GString *gsz;

    if (!CheckGameExists())
        return;

    gsz = g_string_new(NULL);
    cmark_game_show(gsz, plGame, getGameNumber(plGame));
    outputf("%s", gsz->str);
    g_string_free(gsz, TRUE);
}

extern void
CommandCMarkMatchShow(char *UNUSED(sz))
{
    GString *gsz;

    if (!CheckGameExists())
        return;

    if ((gsz = g_string_new(NULL))) {
        cmark_match_show(gsz, &lMatch);
        outputf("%s", gsz->str);
        g_string_free(gsz, TRUE);
    } else
        g_assert_not_reached();
}

extern void
CommandAnalyseRolloutCube(char *UNUSED(sz))
{
    moverecord *pmr = get_current_moverecord(NULL);

    if (!check_cube_in_pmr(pmr))
        return;

    cmark_cube_set(pmr, CMARK_ROLLOUT);
    cmark_cube_rollout(pmr, FALSE);
    cmark_cube_set(pmr, CMARK_NONE);
}

extern void
CommandAnalyseRolloutMove(char *sz)
{
    moverecord *pmr = get_current_moverecord(NULL);

    if (!check_cmoves_in_pmr(pmr))
        return;

    if (sz && *sz)
        cmark_move_set(pmr, sz, CMARK_ROLLOUT);

    if (cmark_move_rollout(pmr, FALSE) == 0) {
        outputerrf("No moves marked for rollout\n");
        return;
    }

    cmark_move_clear(pmr);
}

extern void
CommandAnalyseRolloutGame(char *UNUSED(sz))
{
    if (!CheckGameExists())
        return;

    cmark_game_rollout(plGame);
}

extern void
CommandAnalyseRolloutMatch(char *UNUSED(sz))
{
    if (!CheckGameExists())
        return;

    cmark_match_rollout(&lMatch);
}
