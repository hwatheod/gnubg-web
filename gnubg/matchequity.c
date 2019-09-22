
/*
 * matchequity.c
 * by Joern Thyssen, 1999-2002
 *
 * Calculate matchequity table based on 
 * formulas from [insert the Managent Science ref here].
 *
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
 * $Id: matchequity.c,v 1.96 2014/09/13 19:47:05 plm Exp $
 */

#include "config.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "list.h"
#include "mec.h"
#include "util.h"

#define GAMMONRATE    0.25

#include "eval.h"
#include "matchequity.h"
#include "backgammon.h"


typedef struct _parameter {

    char *szName;
    float value;

} parameter;


typedef struct _metparameters {

    char *szName;
    listOLD lParameters;

} metparameters;

typedef struct _metdata {

    /* Data saved */

    /* Pre crawford MET */

    float aarMET[MAXSCORE][MAXSCORE];
    metparameters mpPreCrawford;

    /* post-crawford MET */

    float aarMETPostCrawford[2][MAXSCORE];
    metparameters ampPostCrawford[2];

    /* MET info */

    metinfo mi;

} metdata;

/*
 * A1 (A2) is the match equity of player 1 (2)
 * Btilde is the post-crawford match equities.
 */

float aafMET[MAXSCORE][MAXSCORE];
float aafMETPostCrawford[2][MAXSCORE];

/* gammon prices (calculated once for efficiency) */

float aaaafGammonPrices[MAXCUBELEVEL]
    [MAXSCORE][MAXSCORE][4];
float aaaafGammonPricesPostCrawford[MAXCUBELEVEL]
    [MAXSCORE][2][4];


metinfo miCurrent;


/*
 * Calculate area under normal distribution curve (with mean rMu and 
 * std.dev rSigma) from rMax to rMin.
 * 
 * Input:
 *   rMin, rMax: upper and lower integral limits
 *   rMu, rSigma: normal distribution parameters.
 *
 * Returns:
 *  area
 *
 */

static float
NormalDistArea(float rMin, float rMax, float rMu, float rSigma)
{
    float rtMin, rtMax;
    float rInt1, rInt2;

    rtMin = (rMin - rMu) / rSigma;
    rtMax = (rMax - rMu) / rSigma;

    rInt1 = (erff(rtMin / sqrtf(2)) + 1.0f) / 2.0f;
    rInt2 = (erff(rtMax / sqrtf(2)) + 1.0f) / 2.0f;

    return rInt2 - rInt1;
}



static int
GetCubePrimeValue(int i, int j, int nCubeValue)
{

    if ((i < 2 * nCubeValue) && (j >= 2 * nCubeValue))

        /* automatic double */

        return 2 * nCubeValue;
    else
        return nCubeValue;

}


/*
 * Calculate post-Crawford match equity table
 *
 * Formula taken from Zadeh, Management Science xx
 *
 * Input:
 *    rG: gammon rate for trailer
 *    rFD2: value of free drop at 1-away, 2-away
 *    rFD4: value of free drop at 1-away, 4-away
 *    iStart: score where met starts, e.g.
 *       iStart = 0 : 1-away, 1-away
 *       iStart = 15: 1-away, 16-away
 *    
 *   
 * Output:
 *    afMETPostCrawford: post-Crawford match equity table
 *
 */

static void
initPostCrawfordMET(float afMETPostCrawford[MAXSCORE],
                    const int iStart, const float rG, const float rFD2, const float rFD4)
{

    int i;

    /*
     * Calculate post-crawford match equities
     */

    for (i = iStart; i < MAXSCORE; i++) {

        afMETPostCrawford[i] = rG * 0.5f * ((i - 4 >= 0) ? afMETPostCrawford[i - 4] : 1.0f)
            + (1.0f - rG) * 0.5f * ((i - 2 >= 0) ? afMETPostCrawford[i - 2] : 1.0f);

        /*"insane post crawford equity" */
        g_assert(afMETPostCrawford[i] >= 0.0f && afMETPostCrawford[i] <= 1.0f);
        /*
         * add 1.5% at 1-away, 2-away for the free drop
         * add 0.4% at 1-away, 4-away for the free drop
         */

        if (i == 1)
            afMETPostCrawford[i] -= rFD2;
        /*"insane post crawford equity(1)" */
        g_assert(afMETPostCrawford[i] >= 0.0f && afMETPostCrawford[i] <= 1.0f);

        if (i == 3)
            afMETPostCrawford[i] -= rFD4;
        /*"insane post crawford equity(2)" */
        g_assert(afMETPostCrawford[i] >= 0.0f && afMETPostCrawford[i] <= 1.0f);

    }


}


/*
 * Calculate match equity table from Zadeh's formula published
 * in Management Science xx.xx,xx.
 *
 * Input:
 *    rG1: gammon rate for leader of match
 *    rG2: gammon rate for trailer of match
 *    rDelta, rDeltaBar: parameters in the Zadeh model
 *      (something to do with cube efficiencies)
 *    rG3: post-Crawford gammon rate for trailer of match
 *    afMETPostCrawford: post-Crawford match equity table
 *
 * Output:
 *    aafMET: match equity table
 *
 */

static void
initMETZadeh(float aafMET[MAXSCORE][MAXSCORE],
             const float afMETPostCrawford[MAXSCORE],
             const float rG1, const float rG2, const float rDelta, const float rDeltaBar)
{

    int i, j, k;
    int nCube;
    int nCubeValue, nCubePrimeValue;

    /*
     * D1bar (D2bar) is the equity of player 1 (2) at the drop point
     * of player 2 (1) assuming dead cubes. 
     * D1 (D2) is the equity of player 1 (2) at the drop point
     * of player 2 (1) assuming semiefficient recubes.
     */

    float aaafD1[MAXSCORE][MAXSCORE][MAXCUBELEVEL];
    float aaafD2[MAXSCORE][MAXSCORE][MAXCUBELEVEL];
    float aaafD1bar[MAXSCORE][MAXSCORE][MAXCUBELEVEL];
    float aaafD2bar[MAXSCORE][MAXSCORE][MAXCUBELEVEL];


    /*
     * Calculate 1-away,n-away match equities
     */

    for (i = 0; i < MAXSCORE; i++) {

        aafMET[i][0] = rG1 * 0.5f * ((i - 2 >= 0) ? afMETPostCrawford[i - 2] : 1.0f)
            + (1.0f - rG1) * 0.5f * ((i - 1 >= 0) ? afMETPostCrawford[i - 1] : 1.0f);
        aafMET[0][i] = 1.0f - aafMET[i][0];

    }

    for (i = 0; i < MAXSCORE; i++) {

        for (j = 0; j <= i; j++) {

            for (nCube = MAXCUBELEVEL - 1; nCube >= 0; nCube--) {

                nCubeValue = 1;
                for (k = 0; k < nCube; k++)
                    nCubeValue *= 2;

                /* Calculate D1bar */

                nCubePrimeValue = GetCubePrimeValue(i, j, nCubeValue);

                aaafD1bar[i][j][nCube] = (GET_MET(i - nCubeValue, j, aafMET)
                                          - rG2 * GET_MET(i, j - 4 * nCubePrimeValue, aafMET)
                                          - (1.0f - rG2) * GET_MET(i, j - 2 * nCubePrimeValue, aafMET))
                    / (rG1 * GET_MET(i - 4 * nCubePrimeValue, j, aafMET)
                       + (1.0f - rG1) * GET_MET(i - 2 * nCubePrimeValue, j, aafMET)
                       - rG2 * GET_MET(i, j - 4 * nCubePrimeValue, aafMET)
                       - (1.0f - rG2) * GET_MET(i, j - 2 * nCubePrimeValue, aafMET));



                if (i != j) {

                    nCubePrimeValue = GetCubePrimeValue(j, i, nCubeValue);

                    aaafD1bar[j][i][nCube] = (GET_MET(j - nCubeValue, i, aafMET)
                                              - rG2 * GET_MET(j, i - 4 * nCubePrimeValue, aafMET)
                                              - (1.0f - rG2) * GET_MET(j, i - 2 * nCubePrimeValue, aafMET))
                        / (rG1 * GET_MET(j - 4 * nCubePrimeValue, i, aafMET)
                           + (1.0f - rG1) * GET_MET(j - 2 * nCubePrimeValue, i, aafMET)
                           - rG2 * GET_MET(j, i - 4 * nCubePrimeValue, aafMET)
                           - (1.0f - rG2) * GET_MET(j, i - 2 * nCubePrimeValue, aafMET));



                }

                /* Calculate D2bar */

                nCubePrimeValue = GetCubePrimeValue(j, i, nCubeValue);

                aaafD2bar[i][j][nCube] = (GET_MET(j - nCubeValue, i, aafMET)
                                          - rG2 * GET_MET(j, i - 4 * nCubePrimeValue, aafMET)
                                          - (1.0f - rG2) * GET_MET(j, i - 2 * nCubePrimeValue, aafMET))
                    / (rG1 * GET_MET(j - 4 * nCubePrimeValue, i, aafMET)
                       + (1.0f - rG1) * GET_MET(j - 2 * nCubePrimeValue, i, aafMET)
                       - rG2 * GET_MET(j, i - 4 * nCubePrimeValue, aafMET)
                       - (1.0f - rG2) * GET_MET(j, i - 2 * nCubePrimeValue, aafMET));


                if (i != j) {

                    nCubePrimeValue = GetCubePrimeValue(i, j, nCubeValue);

                    aaafD2bar[j][i][nCube] = (GET_MET(i - nCubeValue, j, aafMET)
                                              - rG2 * GET_MET(i, j - 4 * nCubePrimeValue, aafMET)
                                              - (1.0f - rG2) * GET_MET(i, j - 2 * nCubePrimeValue, aafMET))
                        / (rG1 * GET_MET(i - 4 * nCubePrimeValue, j, aafMET)
                           + (1.0f - rG1) * GET_MET(i - 2 * nCubePrimeValue, j, aafMET)
                           - rG2 * GET_MET(i, j - 4 * nCubePrimeValue, aafMET)
                           - (1.0f - rG2) * GET_MET(i, j - 2 * nCubePrimeValue, aafMET));



                }

                /* Calculate D1 */

                if ((i < 2 * nCubeValue) || (j < 2 * nCubeValue)) {

                    aaafD1[i][j][nCube] = aaafD1bar[i][j][nCube];
                    if (i != j)
                        aaafD1[j][i][nCube] = aaafD1bar[j][i][nCube];

                } else {

                    aaafD1[i][j][nCube] = 1.0f + (aaafD2[i][j][nCube + 1] + rDelta)
                        * (aaafD1bar[i][j][nCube] - 1.0f);
                    if (i != j)
                        aaafD1[j][i][nCube] = 1.0f + (aaafD2[j][i][nCube + 1] + rDelta)
                            * (aaafD1bar[j][i][nCube] - 1.0f);

                }


                /* Calculate D2 */

                if ((i < 2 * nCubeValue) || (j < 2 * nCubeValue)) {

                    aaafD2[i][j][nCube] = aaafD2bar[i][j][nCube];
                    if (i != j)
                        aaafD2[j][i][nCube] = aaafD2bar[j][i][nCube];

                } else {

                    aaafD2[i][j][nCube] = 1.0f + (aaafD1[i][j][nCube + 1] + rDelta)
                        * (aaafD2bar[i][j][nCube] - 1.0f);

                    if (i != j)
                        aaafD2[j][i][nCube] = 1.0f + (aaafD1[j][i][nCube + 1] + rDelta)
                            * (aaafD2bar[j][i][nCube] - 1.0f);
                }


                /* Calculate A1 & A2 */

                if ((nCube == 0) && (i > 0) && (j > 0)) {

                    aafMET[i][j] = ((aaafD2[i][j][0] + rDeltaBar - 0.5f)
                                    * GET_MET(i - 1, j, aafMET)
                                    + (aaafD1[i][j][0] + rDeltaBar - 0.5f)
                                    * GET_MET(i, j - 1, aafMET))
                        / (aaafD1[i][j][0] + rDeltaBar + aaafD2[i][j][0] + rDeltaBar - 1.0f);


                    if (i != j)
                        aafMET[j][i] = 1.0f - aafMET[i][j];

                }

            }

        }

    }

}

extern int
GetPoints(float arOutput[5], const cubeinfo * pci, float arCP[2])
{

    /*
     * Input:
     * - arOutput: we need the gammon and backgammon ratios
     *   (we assume arOutput is evaluate for pci -> fMove)
     * - anScore: the current score.
     * - nMatchTo: matchlength
     * - pci: value of cube, who's turn is it
     * 
     *
     * Output:
     * - arCP : cash points with live cube
     * These points are necesary for the linear
     * interpolation used in cubeless -> cubeful equity 
     * transformation.
     */

    /* Match play */

    /* normalize score */

    int i = pci->nMatchTo - pci->anScore[0] - 1;
    int j = pci->nMatchTo - pci->anScore[1] - 1;

    int nCube = pci->nCube;

    float arCPLive[2][MAXCUBELEVEL];
    float arCPDead[2][MAXCUBELEVEL];
    float arG[2], arBG[2];

    float rDP, rRDP, rDTW, rDTL;

    int nDead, n, nMax, nCubeValue, k;


    float aarMETResults[2][DTLBP1 + 1];

    /* Gammon and backgammon ratio's. 
     * Avoid division by zero in extreme cases. */

    if (!pci->fMove) {

        /* arOutput evaluated for player 0 */

        if (arOutput[OUTPUT_WIN] > 0.0f) {
            arG[0] = (arOutput[OUTPUT_WINGAMMON] - arOutput[OUTPUT_WINBACKGAMMON]) / arOutput[OUTPUT_WIN];
            arBG[0] = arOutput[OUTPUT_WINBACKGAMMON] / arOutput[OUTPUT_WIN];
        } else {
            arG[0] = 0.0;
            arBG[0] = 0.0;
        }

        if (arOutput[OUTPUT_WIN] < 1.0f) {
            arG[1] = (arOutput[OUTPUT_LOSEGAMMON] - arOutput[OUTPUT_LOSEBACKGAMMON]) / (1.0f - arOutput[OUTPUT_WIN]);
            arBG[1] = arOutput[OUTPUT_LOSEBACKGAMMON] / (1.0f - arOutput[OUTPUT_WIN]);
        } else {
            arG[1] = 0.0;
            arBG[1] = 0.0;
        }

    } else {

        /* arOutput evaluated for player 1 */

        if (arOutput[OUTPUT_WIN] > 0.0f) {
            arG[1] = (arOutput[OUTPUT_WINGAMMON] - arOutput[OUTPUT_WINBACKGAMMON]) / arOutput[OUTPUT_WIN];
            arBG[1] = arOutput[OUTPUT_WINBACKGAMMON] / arOutput[OUTPUT_WIN];
        } else {
            arG[1] = 0.0;
            arBG[1] = 0.0;
        }

        if (arOutput[OUTPUT_WIN] < 1.0f) {
            arG[0] = (arOutput[OUTPUT_LOSEGAMMON] - arOutput[OUTPUT_LOSEBACKGAMMON]) / (1.0f - arOutput[OUTPUT_WIN]);
            arBG[0] = arOutput[OUTPUT_LOSEBACKGAMMON] / (1.0f - arOutput[OUTPUT_WIN]);
        } else {
            arG[0] = 0.0;
            arBG[0] = 0.0;
        }
    }

    /* Find out what value the cube has when you or your
     * opponent give a dead cube. */

    nDead = nCube;
    nMax = 0;

    while ((i >= 2 * nDead) && (j >= 2 * nDead)) {
        nMax++;
        nDead *= 2;
    }

    for (nCubeValue = nDead, n = nMax; nCubeValue >= nCube; nCubeValue >>= 1, n--) {

        /* Calculate dead and live cube cash points.
         * See notes by me (Joern Thyssen) available from the
         * 'doc' directory.  (FIXME: write notes :-) ) */

        /* Even though it's a dead cube we take account of the opponents
         * automatic redouble. */

        /* Dead cube cash point for player 0 */

        getMEMultiple(pci->anScore[0], pci->anScore[1], pci->nMatchTo, nCubeValue, GetCubePrimeValue(i, j, nCubeValue), /* 0 */
                      GetCubePrimeValue(j, i, nCubeValue),      /* 1 */
                      pci->fCrawford, aafMET, aafMETPostCrawford, aarMETResults[0], aarMETResults[1]);

        for (k = 0; k < 2; k++) {



            /* Live cube cash point for player */

            if ((i < 2 * nCubeValue) || (j < 2 * nCubeValue)) {

                rDTL = (1.0f - arG[!k] - arBG[!k]) * aarMETResults[k][k ? DTLP1 : DTLP0]
                    + arG[!k] * aarMETResults[k][k ? DTLGP1 : DTLGP0]
                    + arBG[!k] * aarMETResults[k][k ? DTLBP1 : DTLBP0];


                rDP = aarMETResults[k][DP];

                rDTW = (1.0f - arG[k] - arBG[k]) * aarMETResults[k][k ? DTWP1 : DTWP0]
                    + arG[k] * aarMETResults[k][k ? DTWGP1 : DTWGP0]
                    + arBG[k] * aarMETResults[k][k ? DTWBP1 : DTWBP0];

                arCPDead[k][n] = (rDTL - rDP) / (rDTL - rDTW);

                /* The doubled cube is going to be dead */
                arCPLive[k][n] = arCPDead[k][n];

            } else {

                /* Doubled cube is alive */

                /* redouble, pass */
                rRDP = aarMETResults[k][DTL];

                /* double, pass */
                rDP = aarMETResults[k][DP];

                /* double, take win */

                rDTW = (1.0f - arG[k] - arBG[k]) * aarMETResults[k][DTW]
                    + arG[k] * aarMETResults[k][DTWG]
                    + arBG[k] * aarMETResults[k][DTWB];

                arCPLive[k][n] = 1.0f - arCPLive[!k][n + 1] * (rDP - rDTW) / (rRDP - rDTW);

            }

        }                       /* loop k */

    }

    /* return cash point for current cube level */

    arCP[0] = arCPLive[0][0];
    arCP[1] = arCPLive[1][0];

#if 0
    for (n = nMax; n >= 0; n--) {

        printf("Cube %i\n"
               "Dead cube:    cash point 0 %6.3f\n"
               "              cash point 1 %6.3f\n"
               "Live cube:    cash point 0 %6.3f\n"
               "              cash point 1 %6.3f\n\n",
               n, arCPDead[0][n], arCPDead[1][n], arCPLive[0][n], arCPLive[1][n]);

    }
#endif

    return 0;

}

extern float
GetDoublePointDeadCube(float arOutput[5], cubeinfo * pci)
{
    /*
     * Calculate double point for dead cubes
     */

    int player = pci->fMove;

    if (!pci->nMatchTo) {

        /* Use Rick Janowski's formulas
         * [insert proper reference here] */

        float rW, rL;

        if (arOutput[0] > 0.0f)
            rW = 1.0f + (arOutput[1] + arOutput[2]) / arOutput[0];
        else
            rW = 1.0f;

        if (arOutput[0] < 1.0f)
            rL = 1.0f + (arOutput[3] + arOutput[4]) / (1.0f - arOutput[0]);
        else
            rL = 1.0f;

        if (pci->fCubeOwner == -1 && pci->fJacoby) {

            /* centered cube */

            if (pci->fBeavers) {
                return (rL - 0.25f) / (rW + rL - 0.5f);
            } else {
                return (rL - 0.5f) / (rW + rL - 1.0f);
            }
        } else {

            /* redoubles or Jacoby rule not in effect */

            return rL / (rL + rW);
        }

    } else {

        /* Match play */

        float aarMETResults[2][DTLBP1 + 1];

        /* normalize score */

        float rG1, rBG1, rG2, rBG2, rDTW, rNDW, rDTL, rNDL;
        float rRisk, rGain;

        /* FIXME: avoid division by zero */
        if (arOutput[OUTPUT_WIN] > 0.0f) {
            rG1 = (arOutput[OUTPUT_WINGAMMON] - arOutput[OUTPUT_WINBACKGAMMON]) / arOutput[OUTPUT_WIN];
            rBG1 = arOutput[OUTPUT_WINBACKGAMMON] / arOutput[OUTPUT_WIN];
        } else {
            rG1 = 0.0;
            rBG1 = 0.0;
        }

        if (arOutput[OUTPUT_WIN] < 1.0f) {
            rG2 = (arOutput[OUTPUT_LOSEGAMMON] - arOutput[OUTPUT_LOSEBACKGAMMON]) / (1.0f - arOutput[OUTPUT_WIN]);
            rBG2 = arOutput[OUTPUT_LOSEBACKGAMMON] / (1.0f - arOutput[OUTPUT_WIN]);
        } else {
            rG2 = 0.0;
            rBG2 = 0.0;
        }

        getMEMultiple(pci->anScore[0], pci->anScore[1], pci->nMatchTo,
                      pci->nCube, -1, -1,
                      pci->fCrawford, aafMET, aafMETPostCrawford, aarMETResults[0], aarMETResults[1]);

        /* double point */

        /* double point */

        rDTW = (1.0f - rG1 - rBG1) * aarMETResults[player][DTW]
            + rG1 * aarMETResults[player][DTWG]
            + rBG1 * aarMETResults[player][DTWB];

        rNDW = (1.0f - rG1 - rBG1) * aarMETResults[player][NDW]
            + rG1 * aarMETResults[player][NDWG]
            + rBG1 * aarMETResults[player][NDWB];

        rDTL = (1.0f - rG2 - rBG2) * aarMETResults[player][DTL]
            + rG2 * aarMETResults[player][DTLG]
            + rBG2 * aarMETResults[player][DTLB];

        rNDL = (1.0f - rG2 - rBG2) * aarMETResults[player][NDL]
            + rG2 * aarMETResults[player][NDLG]
            + rBG2 * aarMETResults[player][NDLB];

        /* risk & gain */

        rRisk = rNDL - rDTL;
        rGain = rDTW - rNDW;

        return rRisk / (rRisk + rGain);

    }

}

/* 
 * Extend match equity table to MAXSCORE using
 * David Montgomery's extension algorithm. The formula
 * is independent of the values for score < nMaxScore.
 *
 * Input:
 *    nMaxScore: the length of the native met
 *    aarMET: the match equity table for scores below nMaxScore
 *
 * Output:
 *    aarMET: the match equity table for scores up to MAXSCORE.
 *
 */

static void
ExtendMET(float aarMET[MAXSCORE][MAXSCORE], const int nMaxScore)
{

    static const float arStddevTable[] =
        { 0, 1.24f, 1.27f, 1.47f, 1.50f, 1.60f, 1.61f, 1.66f, 1.68f, 1.70f, 1.72f, 1.77f };

    float rStddev0, rStddev1, rGames, rSigma;
    int i, j;
    int nScore0, nScore1;

    /* Extend match equity table */
    for (i = nMaxScore; i < MAXSCORE; i++) {

        nScore0 = i + 1;

        if (nScore0 > 10)
            rStddev0 = 1.77f;
        else
            rStddev0 = arStddevTable[nScore0];

        for (j = 0; j <= i; j++) {

            nScore1 = j + 1;

            rGames = (nScore0 + nScore1) / 2.0f;

            if (nScore1 > 10)
                rStddev1 = 1.77f;
            else
                rStddev1 = arStddevTable[nScore1];

            rSigma = sqrtf(rStddev0 * rStddev0 + rStddev1 * rStddev1) * sqrtf(rGames);

            if (6.0f * rSigma > nScore0 - nScore1)
                aarMET[i][j] = NormalDistArea((float) (nScore0 - nScore1), 6.0f * rSigma, 0.0f, rSigma);
            else
                aarMET[i][j] = 0.0f;
        }
    }

    /* Generate j > i part of MET */

    for (i = 0; i < MAXSCORE; i++)
        for (j = ((i < nMaxScore) ? nMaxScore : i + 1); j < MAXSCORE; j++)
            aarMET[i][j] = 1.0f - aarMET[j][i];

}


static void
initMP(metparameters * pmp)
{

    pmp->szName = NULL;
    ListCreate(&pmp->lParameters);

}


static void
initMD(metdata * pmd)
{

    int i;

    pmd->mi.szName = NULL;
    pmd->mi.szFileName = NULL;
    pmd->mi.szDescription = NULL;

    initMP(&pmd->mpPreCrawford);

    for (i = 0; i < 2; i++)
        initMP(&pmd->ampPostCrawford[i]);

}


static void
freeP(parameter * pp)
{

    if (pp->szName)
        g_free(pp->szName);
    g_free(pp);

}


static void
freeMP(metparameters * pmp)
{

    listOLD *pl;

    if (pmp->szName)
        g_free(pmp->szName);

    pl = &pmp->lParameters;

    while (pl->plNext != pl) {

        freeP(pl->plNext->p);
        ListDelete(pl->plNext);

    }

}


static void
getDefaultMET(metdata * pmd)
{

    int i, j;
    parameter *pp;

    static parameter apPreCrawford[] = { {"gammon-rate-leader", 0.25f},
    {"gammon-rate-trailer", 0.15f},
    {"delta", 0.08f},
    {"deltebar", 0.06f}
    };

    static parameter apPostCrawford[] = { {"gammon-rate-trailer", 0.25f},
    {"free-drop-2-away", 0.015f},
    {"free-drop-4-away", 0.004f}
    };

    /* Setup default met */

    initMD(pmd);

    for (i = 0; i < 4; i++) {
        pp = (parameter *) malloc(sizeof(parameter));
        pp->szName = g_strdup(apPreCrawford[i].szName);
        pp->value = apPreCrawford[i].value;
        ListInsert(&pmd->mpPreCrawford.lParameters, pp);
    }

    for (j = 0; j < 2; j++) {
        for (i = 0; i < 3; i++) {
            pp = (parameter *) malloc(sizeof(parameter));
            pp->szName = g_strdup(apPostCrawford[i].szName);
            pp->value = apPostCrawford[i].value;
            ListInsert(&pmd->ampPostCrawford[j].lParameters, pp);
        }
        pmd->ampPostCrawford[j].szName = g_strdup("zadeh");
    }

    pmd->mpPreCrawford.szName = g_strdup("zadeh");

    pmd->mi.szName = g_strdup("N. Zadeh, Management Science 23, 986 (1977)");
    pmd->mi.szFileName = g_strdup("met/zadeh.xml");
    pmd->mi.szDescription = g_strdup("");
    pmd->mi.nLength = MAXSCORE;

}


static int
initMETFromParameters(float aafMET[MAXSCORE][MAXSCORE], float aafMETPostCrawford[2][MAXSCORE], metparameters * pmp)
{

    if (!strcmp((char *) pmp->szName, "mec")) {

        float rG = 0.15f;
        float rWR = 0.5f;
        listOLD *pl;
        parameter *pp;

        /* 
         */

        /* obtain parameters */

        for (pl = pmp->lParameters.plNext; pl != &pmp->lParameters; pl = pl->plNext) {

            pp = pl->p;

            if (!strcmp((char *) pp->szName, "gammon-rate"))
                rG = pp->value;
            else if (!strcmp((char *) pp->szName, "win-rate"))
                rWR = pp->value;

        }

        /* calculate table */

        mec(rG, rWR, aafMETPostCrawford, aafMET);

        return 0;

    } else if (!strcmp((char *) pmp->szName, "zadeh")) {

        float rG1 = 0.25f;
        float rG2 = 0.15f;
        float rDelta = 0.08f;
        float rDeltaBar = 0.06f;
        listOLD *pl;
        parameter *pp;

        /* 
         * Use Zadeh's formulae 
         */

        /* obtain parameters */

        for (pl = pmp->lParameters.plNext; pl != &pmp->lParameters; pl = pl->plNext) {

            pp = pl->p;

            if (!strcmp((char *) pp->szName, "gammon-rate-leader"))
                rG1 = pp->value;
            else if (!strcmp((char *) pp->szName, "gammon-rate-trailer"))
                rG2 = pp->value;
            else if (!strcmp((char *) pp->szName, "delta"))
                rDelta = pp->value;
            else if (!strcmp((char *) pp->szName, "delta-bar"))
                rDeltaBar = pp->value;

        }

        /* calculate table */

        initMETZadeh(aafMET, aafMETPostCrawford[0], rG1, rG2, rDelta, rDeltaBar);
        return 0;

    } else {

        /* unknown type */

        return -1;

    }


}


static int
initPostCrawfordMETFromParameters(float afMETPostCrawford[MAXSCORE], metparameters * pmp)
{

    if (!strcmp((char *) pmp->szName, "zadeh")) {

        float rG = 0.25f;
        float rFD2 = 0.015f;
        float rFD4 = 0.004f;
        listOLD *pl;
        parameter *pp;

        /* 
         * Use Zadeh's formulae 
         */

        /* obtain parameters */

        for (pl = pmp->lParameters.plNext; pl != &pmp->lParameters; pl = pl->plNext) {

            pp = pl->p;

            if (!strcmp((char *) pp->szName, "gammon-rate-trailer"))
                rG = pp->value;
            else if (!strcmp((char *) pp->szName, "free-drop-2-away"))
                rFD2 = pp->value;
            else if (!strcmp((char *) pp->szName, "free-drop-4-away"))
                rFD4 = pp->value;

        }

        /* calculate table */

        initPostCrawfordMET(afMETPostCrawford, 0, rG, rFD2, rFD4);
        return 0;

    } else if (!strcmp((char *) pmp->szName, "mec")) {

        float rG = 0.25f;
        float rFD2 = 0.015f;
        float rFD4 = 0.004f;
        float rWR = 0.5f;
        listOLD *pl;
        parameter *pp;

        /* obtain parameters */

        for (pl = pmp->lParameters.plNext; pl != &pmp->lParameters; pl = pl->plNext) {

            pp = pl->p;

            if (!strcmp((char *) pp->szName, "gammon-rate"))
                rG = pp->value;
            else if (!strcmp((char *) pp->szName, "free-drop-2-away"))
                rFD2 = pp->value;
            else if (!strcmp((char *) pp->szName, "free-drop-4-away"))
                rFD4 = pp->value;
            else if (!strcmp((char *) pp->szName, "win-rate"))
                rWR = pp->value;

        }

        /* calculate table */

        mec_pc(rG, rFD2, rFD4, rWR, afMETPostCrawford);

        return 0;

    } else {

        /* unknown type */

        return -1;

    }

}

typedef enum {
    STATE_NONE,
    STATE_MET,
    STATE_INFO,
    STATE_NAME,
    STATE_DESCRIPTION, STATE_LENGTH,
    STATE_PRECRAWFORD,
    STATE_PRECRAWFORD_ROW,
    STATE_PRECRAWFORD_ME,
    STATE_PRECRAWFORD_PARAMS,
    STATE_PRECRAWFORD_PARAM,
    STATE_POSTCRAWFORD,
    STATE_POSTCRAWFORD_ROW,
    STATE_POSTCRAWFORD_ME,
    STATE_POSTCRAWFORD_PARAMS,
    STATE_POSTCRAWFORD_PARAM
} parserstate;

typedef enum {
    TYPE_EXPLICIT,
    TYPE_MEC,
    TYPE_ZADEH
} tabletype;

typedef enum {
    MP_PLAYER_0 = 0,
    MP_PLAYER_1,
    MP_PLAYER_BOTH = 2
} metplayer;

typedef struct _MatchEquityParser {
    parserstate state;
    tabletype type;
    metplayer player;
    metdata *data;
    guint i, j;                 /* table index */
    parameter *current_param;
    gchar *filename;
} MatchEquityParser;

static void met_parser_start_element(GMarkupParseContext * context,
                                     const gchar * element_name,
                                     const gchar ** attribute_names,
                                     const gchar ** attribute_values, gpointer user_data, GError ** error);

static void met_parser_end_element(GMarkupParseContext * context,
                                   const gchar * element_name, gpointer user_data, GError ** error);

static void met_parser_characters(GMarkupParseContext * context,
                                  const gchar * text, gsize text_len, gpointer user_data, GError ** error);

static void met_parser_error(GMarkupParseContext * context, GError * error, gpointer user_data);

static GMarkupParser markup_parser = {
    met_parser_start_element,
    met_parser_end_element,
    met_parser_characters,
    NULL,
    met_parser_error
};

static void
met_parser_destroy(gpointer data)
{
    MatchEquityParser *parser = (MatchEquityParser *) data;
    g_free(parser->filename);
    g_free(parser);
}

static int
readMET(metdata * met, const char *filename)
{
    GMarkupParseContext *context;
    gchar *contents;
    MatchEquityParser *parser;
    gsize size;
    GError *error = NULL;

    initMD(met);
    met->mi.szFileName = g_strdup(filename);

    /* Make and initialize a new parser */
    parser = g_new0(MatchEquityParser, 1);
    parser->filename = g_strdup(filename);
    parser->data = met;
    parser->state = STATE_NONE;
    parser->i = 0;
    parser->j = 0;

    if (!(context = g_markup_parse_context_new(&markup_parser, 0, parser, met_parser_destroy))) {
        g_warning("Can't create XML parser\n");
        g_free(parser);
        return -1;
    }

    /* Read the file */
    if (!g_file_get_contents(filename, &contents, &size, &error)) {
        g_warning("Error reading XML file: %s\n", error->message);
        goto err;
    }

    /* Parse the content */
    if (!g_markup_parse_context_parse(context, contents, size, &error)) {
        g_warning("Error parsing XML file: %s\n", error->message);
        goto err;
    }

  err:
#ifndef WEB
    g_markup_parse_context_free(context);
#endif /* WEB */  // somehow g_markup_parse_context_free crashes here when compiled with Emscripten
    g_free(contents);

    if (error) {
        g_error_free(error);
        return -1;
    } else {
        return 0;
    }
}

static void
parse_attributes(MatchEquityParser * parser, const gchar ** attr_names, const gchar ** attr_values)
{
    gint i = 0;
    while (attr_names[i]) {
        if (strcmp(attr_names[i], "type") == 0) {
            if (strcmp(attr_values[i], "explicit") == 0)
                parser->type = TYPE_EXPLICIT;
            else if (strcmp(attr_values[i], "mec") == 0)
                parser->type = TYPE_MEC;
            else if (strcmp(attr_values[i], "zadeh") == 0)
                parser->type = TYPE_ZADEH;
        } else if (strcmp(attr_names[i], "player") == 0) {
            if (strcmp(attr_values[i], "both") == 0)
                parser->player = MP_PLAYER_BOTH;
            if (strcmp(attr_values[i], "0") == 0)
                parser->player = MP_PLAYER_0;
            if (strcmp(attr_values[i], "1") == 0)
                parser->player = MP_PLAYER_1;
        }
        i++;
    }
}

static void
start_param(MatchEquityParser * parser, const gchar ** attr_names, const gchar ** attr_values)
{
    gint i = 0;
    while (attr_names[i]) {
        if (strcmp(attr_names[i], "name") == 0) {
            parser->current_param = g_new0(parameter, 1);
            parser->current_param->szName = g_strdup(attr_values[i]);
            break;
        }
        i++;
    }
}

static void
SetNameFromType(char **pName, tabletype type)
{
    switch (type) {
    case TYPE_EXPLICIT:
        *pName = g_strdup("explicit");
        break;
    case TYPE_MEC:
        *pName = g_strdup("mec");
        break;
    case TYPE_ZADEH:
        *pName = g_strdup("zadeh");
        break;
    default:
        g_error("Invalid met table type");
    }
}

static void
met_parser_start_element(GMarkupParseContext * UNUSED(context),
                         const gchar * element_name,
                         const gchar ** attribute_names,
                         const gchar ** attribute_values, gpointer user_data, GError ** UNUSED(error))
{

    MatchEquityParser *parser = (MatchEquityParser *) user_data;

    switch (parser->state) {
    case STATE_NONE:
        if (strcmp(element_name, "met") == 0)
            parser->state = STATE_MET;
        break;
    case STATE_MET:
        if (strcmp(element_name, "info") == 0) {
            parser->state = STATE_INFO;
        } else if (strcmp(element_name, "pre-crawford-table") == 0) {
            parser->state = STATE_PRECRAWFORD;
            parser->i = 0;
            parser->j = 0;
            parser->type = TYPE_EXPLICIT;
            parse_attributes(parser, attribute_names, attribute_values);
            SetNameFromType(&parser->data->mpPreCrawford.szName, parser->type);
        } else if (strcmp(element_name, "post-crawford-table") == 0) {
            parser->state = STATE_POSTCRAWFORD;
            parser->i = 0;
            parser->type = TYPE_EXPLICIT;
            parser->player = MP_PLAYER_BOTH;
            parse_attributes(parser, attribute_names, attribute_values);
            SetNameFromType(&parser->data->ampPostCrawford[0].szName, parser->type);
            SetNameFromType(&parser->data->ampPostCrawford[1].szName, parser->type);
        }
        break;                  /* STATE_MET */

    case STATE_INFO:
        if (strcmp(element_name, "name") == 0)
            parser->state = STATE_NAME;
        else if (strcmp(element_name, "description") == 0)
            parser->state = STATE_DESCRIPTION;
        else if (strcmp(element_name, "length") == 0)
            parser->state = STATE_LENGTH;
        break;                  /* STATE_INFO */

    case STATE_PRECRAWFORD:
        if (strcmp(element_name, "row") == 0)
            parser->state = STATE_PRECRAWFORD_ROW;
        else if (strcmp(element_name, "parameters") == 0) {
            parser->state = STATE_PRECRAWFORD_PARAMS;
        }
        break;                  /* STATE_PRECRAWFORD */

    case STATE_PRECRAWFORD_ROW:
        if (strcmp(element_name, "me") == 0)
            parser->state = STATE_PRECRAWFORD_ME;
        break;                  /* STATE_PRECRAWFORD_ROW */

    case STATE_PRECRAWFORD_PARAMS:
        if (strcmp(element_name, "parameter") == 0) {
            parser->state = STATE_PRECRAWFORD_PARAM;
            start_param(parser, attribute_names, attribute_values);
        }
        break;                  /* STATE_PRECRAWFORD_PARAMS */

    case STATE_POSTCRAWFORD:
        if (strcmp(element_name, "row") == 0)
            parser->state = STATE_POSTCRAWFORD_ROW;
        else if (strcmp(element_name, "parameters") == 0) {
            parser->state = STATE_POSTCRAWFORD_PARAMS;
        }
        break;                  /* STATE_POSTCRAWFORD */

    case STATE_POSTCRAWFORD_ROW:
        if (strcmp(element_name, "me") == 0)
            parser->state = STATE_POSTCRAWFORD_ME;
        break;                  /* STATE_POSTCRAWFORD_ROW */

    case STATE_POSTCRAWFORD_PARAMS:
        if (strcmp(element_name, "parameter") == 0) {
            parser->state = STATE_POSTCRAWFORD_PARAM;
            start_param(parser, attribute_names, attribute_values);
        }
        break;                  /* STATE_POSTCRAWFORD_PARAMS */

    case STATE_NAME:
    case STATE_DESCRIPTION:
    case STATE_LENGTH:
    case STATE_PRECRAWFORD_ME:
    case STATE_POSTCRAWFORD_ME:
    case STATE_PRECRAWFORD_PARAM:
    case STATE_POSTCRAWFORD_PARAM:
    default:
        break;
    }
}

static void
met_parser_end_element(GMarkupParseContext * UNUSED(context),
                       const gchar * UNUSED(element_name), gpointer user_data, GError ** UNUSED(error))
{
    MatchEquityParser *parser = (MatchEquityParser *) user_data;

    switch (parser->state) {
    case STATE_NONE:
        g_assert_not_reached();
        break;
    case STATE_MET:
        parser->state = STATE_NONE;
        break;
    case STATE_INFO:
    case STATE_PRECRAWFORD:
    case STATE_POSTCRAWFORD:
        parser->state = STATE_MET;
        break;
    case STATE_NAME:
    case STATE_DESCRIPTION:
    case STATE_LENGTH:
        parser->state = STATE_INFO;
        break;
    case STATE_PRECRAWFORD_ROW:
        parser->state = STATE_PRECRAWFORD;
        parser->j++;
        parser->i = 0;
        break;
    case STATE_PRECRAWFORD_ME:
        parser->state = STATE_PRECRAWFORD_ROW;
        parser->i++;
        break;

    case STATE_PRECRAWFORD_PARAMS:
        parser->state = STATE_PRECRAWFORD;
        break;

    case STATE_PRECRAWFORD_PARAM:
        parser->state = STATE_PRECRAWFORD_PARAMS;
        ListInsert(&parser->data->mpPreCrawford.lParameters, parser->current_param);
        parser->current_param = NULL;
        break;

    case STATE_POSTCRAWFORD_ROW:
    case STATE_POSTCRAWFORD_PARAMS:
        parser->state = STATE_POSTCRAWFORD;
        break;
    case STATE_POSTCRAWFORD_ME:
        parser->state = STATE_POSTCRAWFORD_ROW;
        parser->i++;
        break;
    case STATE_POSTCRAWFORD_PARAM:
        parser->state = STATE_POSTCRAWFORD_PARAMS;
        if (parser->player == MP_PLAYER_BOTH) {
            ListInsert(&parser->data->ampPostCrawford[0].lParameters, parser->current_param);
        } else {
            ListInsert(&parser->data->ampPostCrawford[parser->player].lParameters, parser->current_param);
        }
        parser->current_param = NULL;
        break;

    default:
        g_assert_not_reached();
    }
}

static void
met_parser_characters(GMarkupParseContext * UNUSED(context),
                      const gchar * text, gsize UNUSED(text_len), gpointer user_data, GError ** UNUSED(error))
{
    MatchEquityParser *parser = (MatchEquityParser *) user_data;

    switch (parser->state) {
    case STATE_NAME:
        parser->data->mi.szName = g_strdup(text);
        break;
    case STATE_DESCRIPTION:
        parser->data->mi.szDescription = g_strdup(text);
        break;
    case STATE_LENGTH:
        /* bad bug in glib-2.12.x - see gnome bug 416062 */
        parser->data->mi.nLength = (gint) g_ascii_strtoull(text, NULL, 10);
        break;
    case STATE_PRECRAWFORD_ME:
        parser->data->aarMET[parser->j][parser->i] = (float) g_ascii_strtod(text, NULL);
        break;
    case STATE_POSTCRAWFORD_ME:
        if (parser->player == MP_PLAYER_BOTH) {
            gfloat r = (gfloat) g_ascii_strtod(text, NULL);
            parser->data->aarMETPostCrawford[0][parser->i] = r;
            parser->data->aarMETPostCrawford[1][parser->i] = r;
        } else {
            gfloat r = (gfloat) g_ascii_strtod(text, NULL);
            parser->data->aarMETPostCrawford[parser->player][parser->i] = r;
        }
        break;

    case STATE_PRECRAWFORD_PARAM:
    case STATE_POSTCRAWFORD_PARAM:
        g_assert(parser->current_param);
        parser->current_param->value = (float) g_ascii_strtod(text, NULL);
    default:
        break;
    }
}

static void
met_parser_error(GMarkupParseContext * UNUSED(context), GError * UNUSED(error), gpointer user_data)
{
    MatchEquityParser *parser = (MatchEquityParser *) user_data;
    g_warning("An error occured while parsing file: %s\n", parser->filename);
}

/*
 * Calculate gammon and backgammon price at the specified score
 * with specified cube.
 *
 * Input:
 *    aafMET, aafMETPostCrawford: match equity tables.
 *    nCube: value of cube
 *    fCrawford: is this the Crawford game
 *    nScore0, nScore1, nMatchTo: current score and match length.
 *
 * Output:
 *   arGammonPrice: gammon and backgammon prices.
 *
 */

static void
getGammonPrice(float arGammonPrice[4],
               const int nScore0, const int nScore1, const int nMatchTo,
               const int nCube, const int fCrawford,
               float aafMET[MAXSCORE][MAXSCORE], float aafMETPostCrawford[2][MAXSCORE])
{

    const float epsilon = 1.0E-7f;

    float rWin = getME(nScore0, nScore1, nMatchTo,
                       0, nCube, 0, fCrawford,
                       aafMET, aafMETPostCrawford);

    float rWinGammon = getME(nScore0, nScore1, nMatchTo,
                             0, 2 * nCube, 0, fCrawford,
                             aafMET, aafMETPostCrawford);

    float rWinBG = getME(nScore0, nScore1, nMatchTo,
                         0, 3 * nCube, 0, fCrawford,
                         aafMET, aafMETPostCrawford);

    float rLose = getME(nScore0, nScore1, nMatchTo,
                        0, nCube, 1, fCrawford,
                        aafMET, aafMETPostCrawford);

    float rLoseGammon = getME(nScore0, nScore1, nMatchTo,
                              0, 2 * nCube, 1, fCrawford,
                              aafMET, aafMETPostCrawford);

    float rLoseBG = getME(nScore0, nScore1, nMatchTo,
                          0, 3 * nCube, 1, fCrawford,
                          aafMET, aafMETPostCrawford);

    float rCenter = (rWin + rLose) / 2.0f;

    /* FIXME: correct numerical problems in a better way, than done
     * below. If cube is dead gammon or backgammon price might be a
     * small negative number. For example, at -2,-3 with cube on 2
     * the current code gives: 0.9090..., 0, -2.7e-8, 0 instead
     * of the correct 0.9090..., 0, 0, 0. */

    /* avoid division by zero */

    if (fabsf(rWin - rCenter) > epsilon) {

        /* this expression can be reduced to: 
         * 2 * ( rWinGammon - rWin ) / ( rWin - rLose )
         * which is twice the "usual" gammon value */

        arGammonPrice[0] = (rWinGammon - rCenter) / (rWin - rCenter) - 1.0f;

        /* this expression can be reduced to:
         * 2 * ( rLose - rLoseGammon ) / ( rWin - rLose )
         * which is twice the "usual" gammon value */

        arGammonPrice[1] = (rCenter - rLoseGammon) / (rWin - rCenter) - 1.0f;

        arGammonPrice[2] = (rWinBG - rCenter) / (rWin - rCenter) - (arGammonPrice[0] + 1.0f);
        arGammonPrice[3] = (rCenter - rLoseBG) / (rWin - rCenter) - (arGammonPrice[1] + 1.0f);

    } else
        arGammonPrice[0] = arGammonPrice[1] = arGammonPrice[2] = arGammonPrice[3] = 0.0f;


    /* Correct numerical problems */
    if (arGammonPrice[0] < 0.0f)
        arGammonPrice[0] = 0.0f;
    if (arGammonPrice[1] < 0.0f)
        arGammonPrice[1] = 0.0f;
    if (arGammonPrice[2] < 0.0f)
        arGammonPrice[2] = 0.0f;
    if (arGammonPrice[3] < 0.0f)
        arGammonPrice[3] = 0.0f;

}

/*
 * Calculate all gammon and backgammon prices
 *
 * Input:
 *   aafMET, aafMETPostCrawford: match equity tables
 *
 * Output:
 *   aaaafGammonPrices: all gammon prices
 *
 */

static void
calcGammonPrices(float aafMET[MAXSCORE][MAXSCORE],
                 float aafMETPostCrawford[2][MAXSCORE], float aaaafGammonPrices[MAXCUBELEVEL]
                 [MAXSCORE][MAXSCORE][4], float aaaafGammonPricesPostCrawford[MAXCUBELEVEL]
                 [MAXSCORE][2][4])
{

    int i, j, k;
    int nCube;

    for (i = 0, nCube = 1; i < MAXCUBELEVEL; i++, nCube *= 2)
        for (j = 0; j < MAXSCORE; j++)
            for (k = 0; k < MAXSCORE; k++)
                getGammonPrice(aaaafGammonPrices[i][j][k],
                               MAXSCORE - j - 1, MAXSCORE - k - 1, MAXSCORE,
                               nCube, (MAXSCORE == j) || (MAXSCORE == k), aafMET, aafMETPostCrawford);

    for (i = 0, nCube = 1; i < MAXCUBELEVEL; i++, nCube *= 2)
        for (j = 0; j < MAXSCORE; j++) {
            getGammonPrice(aaaafGammonPricesPostCrawford[i][j][0],
                           MAXSCORE - 1, MAXSCORE - j - 1, MAXSCORE, nCube, FALSE, aafMET, aafMETPostCrawford);
            getGammonPrice(aaaafGammonPricesPostCrawford[i][j][1],
                           MAXSCORE - j - 1, MAXSCORE - 1, MAXSCORE, nCube, FALSE, aafMET, aafMETPostCrawford);
        }

}

extern void
InitMatchEquity(const char *szFileName)
{
    int i, j;
    metdata md;

    /* Read match equity table from XML file */
    if (readMET(&md, szFileName) != 0) {        /* load failed - make default as must have a met */
        getDefaultMET(&md);
    }

    /* Copy met to current met, extend met (if needed) */

    /* post-Crawford table */
    for (j = 0; j < 2; j++) {
        if (!strcmp((char *) md.ampPostCrawford[j].szName, "explicit")) {

            /* copy and extend table */

            /* FIXME: implement better extension of post-Crawford table */

            /* Note that the post Crawford table is extended from
             * n - 1 as the  post Crawford table of a n match equity table
             * might not include the post Crawford equity at n-away, since
             * the first "legal" post Crawford score is n-1. */

            for (i = 0; i < md.mi.nLength - 1; i++)
                aafMETPostCrawford[j][i] = md.aarMETPostCrawford[j][i];

            initPostCrawfordMET(aafMETPostCrawford[j], md.mi.nLength - 1, GAMMONRATE, 0.015f, 0.004f);

        } else {

            /* generate match equity table using Zadeh's formula */

            if (initPostCrawfordMETFromParameters(aafMETPostCrawford[j], &md.ampPostCrawford[j]) < 0) {

                fprintf(stderr, _("Error generating post-Crawford MET\n"));
                return;

            }

        }
    }

    /* pre-Crawford table */
    if (!strcmp((char *) md.mpPreCrawford.szName, "explicit")) {        /* copy table */
        for (i = 0; i < md.mi.nLength; i++)
            for (j = 0; j < md.mi.nLength; j++)
                aafMET[i][j] = md.aarMET[i][j];
    } else {
        /* generate match equity table using Zadeh's formula */
        if (initMETFromParameters(aafMET, aafMETPostCrawford, &md.mpPreCrawford) < 0) {

            fprintf(stderr, _("Error generating pre-Crawford MET\n"));
            return;
        }
    }

    /* Extend match equity table */
    ExtendMET(aafMET, md.mi.nLength);

    /* garbage collect */
    freeMP(&md.mpPreCrawford);

    for (i = 0; i < 2; i++)
        freeMP(&md.ampPostCrawford[i]);

    if (miCurrent.szName)
        g_free(miCurrent.szName);
    if (miCurrent.szFileName)
        g_free(miCurrent.szFileName);
    if (miCurrent.szDescription)
        g_free(miCurrent.szDescription);
    /* save match equity table information */
    memcpy(&miCurrent, &md.mi, sizeof(metinfo));

    /* initialise gammon prices */
    calcGammonPrices(aafMET, aafMETPostCrawford, aaaafGammonPrices, aaaafGammonPricesPostCrawford);
}


/*
 * Return match equity (mwc) assuming player fWhoWins wins nPoints points.
 *
 * If fCrawford then afMETPostCrawford is used, otherwise
 * aafMET is used.
 *
 * Input:
 *    nAway0: points player 0 needs to win
 *    nAway1: points player 1 needs to win
 *    fPlayer: get mwc for this player
 *    fCrawford: is this the Crawford game
 *    aafMET: match equity table for player 0
 *    afMETPostCrawford: post-Crawford match equity table for player 0
 *
 */


extern float
getME(const int nScore0, const int nScore1, const int nMatchTo,
      const int fPlayer,
      const int nPoints, const int fWhoWins,
      const int fCrawford, float aafMET[MAXSCORE][MAXSCORE], float aafMETPostCrawford[2][MAXSCORE])
{

    int n0 = nMatchTo - (nScore0 + (!fWhoWins) * nPoints) - 1;
    int n1 = nMatchTo - (nScore1 + fWhoWins * nPoints) - 1;

    /* check if any player has won the match */

    if (unlikely(n0 < 0))
        /* player 0 has won the game */
        return (fPlayer) ? 0.0f : 1.0f;
    else if (unlikely(n1 < 0))
        /* player 1 has won the game */
        return (fPlayer) ? 1.0f : 0.0f;

    /* the match is not finished */

    if (fCrawford || (nMatchTo - nScore0 == 1) || (nMatchTo - nScore1 == 1)) {

        /* the next game will be post-Crawford */

        if (!n0)
            /* player 0 is leading match */
            /* FIXME: use pc-MET for player 0 */
            return (fPlayer) ? aafMETPostCrawford[1][n1] : 1.0f - aafMETPostCrawford[1][n1];
        else
            /* player 1 is leading the match */
            return (fPlayer) ? 1.0f - aafMETPostCrawford[0][n0] : aafMETPostCrawford[0][n0];

    } else
        /* non-post-Crawford games */
        return (fPlayer) ? 1.0f - aafMET[n0][n1] : aafMET[n0][n1];

}


extern float
getMEAtScore(const int nScore0, const int nScore1, const int nMatchTo,
             const int fPlayer, const int fCrawford,
             float aafMET[MAXSCORE][MAXSCORE], float aafMETPostCrawford[2][MAXSCORE])
{

    int n0 = nMatchTo - nScore0 - 1;
    int n1 = nMatchTo - nScore1 - 1;

    /* check if any player has won the match */

    if (n0 < 0)
        /* player 0 has won the game */
        return (fPlayer) ? 0.0f : 1.0f;
    else if (n1 < 0)
        /* player 1 has won the game */
        return (fPlayer) ? 1.0f : 0.0f;

    /* the match is not finished */

    if (!fCrawford && ((nMatchTo - nScore0 == 1) || (nMatchTo - nScore1 == 1))) {

        /* this game is post-Crawford */

        if (!n0)
            /* player 0 is leading match */
            /* FIXME: use pc-MET for player 0 */
            return (fPlayer) ? aafMETPostCrawford[1][n1] : 1.0f - aafMETPostCrawford[1][n1];
        else
            /* player 1 is leading the match */
            return (fPlayer) ? 1.0f - aafMETPostCrawford[0][n0] : aafMETPostCrawford[0][n0];

    } else
        /* non-post-Crawford games */
        return (fPlayer) ? 1.0f - aafMET[n0][n1] : aafMET[n0][n1];

}



extern void
invertMET(void)
{

    int i, j;
    float r;

    for (i = 0; i < MAXSCORE; i++) {

        /* diagonal */

        aafMET[i][i] = 1.0f - aafMET[i][i];

        /* post crawford entries */

        r = aafMETPostCrawford[0][i];
        aafMETPostCrawford[0][i] = aafMETPostCrawford[1][i];
        aafMETPostCrawford[1][i] = r;

        /* off diagonal entries */

        for (j = 0; j < i; j++) {

            r = aafMET[i][j];
            aafMET[i][j] = 1.0f - aafMET[j][i];
            aafMET[j][i] = 1.0f - r;

        }
    }

    calcGammonPrices(aafMET, aafMETPostCrawford, aaaafGammonPrices, aaaafGammonPricesPostCrawford);
}

/* given a match score, return a pair of arrays with the METs for
 * player0 and player 1 winning/losing including gammons & backgammons
 * 
 * if nCubePrime0 < 0, then we're only interested in the first
 * values in each array, using nCube. 
 *
 * Otherwise, if nCubePrime0 >= 0, we do another set of METs with 
 * both sides using nCubePrime0 
 *
 * if nCubePrime1 >= 0, we do a third set using nCubePrime1
 *
 * FIXME ? It looks like if nCubePrime0 >= 0, nCubePrime1 is as well
 *         That could simplify the code below a little
 *
 * This reduces the *huge* number of calls to get equity table entries 
 * when analyzing matches by something like 40 times 
 */

extern void
getMEMultiple(const int nScore0, const int nScore1, const int nMatchTo,
              const int nCube, const int nCubePrime0, const int nCubePrime1,
              const int fCrawford, float aafMET[MAXSCORE][MAXSCORE],
              float aafMETPostCrawford[2][MAXSCORE], float *player0, float *player1)
{

    int scores[2][DTLBP1 + 1];  /* the resulting match scores */
    int i, max_res, s0, s1;
    int *score0, *score1;
    int mult[] = { 1, 2, 3, 4, 6 };
    float *p0, *p1, f;
    int away0, away1;
    int fCrawf = fCrawford;

    /* figure out how many results we'll be returning */
    max_res = (nCubePrime0 < 0) ? DTLB + 1 : (nCubePrime1 < 0) ? DTLBP0 + 1 : DTLBP1 + 1;

    /* set up a table of resulting match scores for all 
     * the results we're calculating */
    score0 = scores[0];
    score1 = scores[1];
    away0 = nMatchTo - nScore0 - 1;
    away1 = nMatchTo - nScore1 - 1;
    fCrawf |= (nMatchTo - nScore0 == 1) || (nMatchTo - nScore1 == 1);

    /* player 0 wins normal, doubled, gammon, backgammon */
    for (i = 0; i < NDL; ++i) {
        *score0++ = away0 - mult[i] * nCube;
        *score1++ = away1;
    }
    /* player 1 wins normal, doubled, etc. */
    for (i = 0; i < NDL; ++i) {
        *score0++ = away0;
        *score1++ = away1 - mult[i] * nCube;
    }
    if (max_res > DPP0) {
        /* same using the second cube value */
        for (i = 0; i < NDL; ++i) {
            *score0++ = away0 - mult[i] * nCubePrime0;
            *score1++ = away1;
        }
        for (i = 0; i < NDL; ++i) {
            *score0++ = away0;
            *score1++ = away1 - mult[i] * nCubePrime0;
        }
        if (max_res > DPP1) {
            /* same using the third cube value */
            for (i = 0; i < NDL; ++i) {
                *score0++ = away0 - mult[i] * nCubePrime1;
                *score1++ = away1;
            }
            for (i = 0; i < NDL; ++i) {
                *score0++ = away0;
                *score1++ = away1 - mult[i] * nCubePrime1;
            }
        }
    }

    score0 = scores[0];
    score1 = scores[1];
    p0 = player0;
    p1 = player1;

    /* now go through the resulting scores, looking up the equities */
    for (i = 0; i < max_res; ++i) {
        s0 = *score0++;
        s1 = *score1++;

        if (unlikely(s0 < 0)) {
            /* player 0 wins */
            *p0++ = 1.0f;
            *p1++ = 0.0f;
        } else if (unlikely(s1 < 0)) {
            *p0++ = 0.0f;
            *p1++ = 1.0f;
        } else if (unlikely(fCrawf)) {
            if (s0 == 0) {      /* player 0 is leading */
                *p0++ = 1.0f - aafMETPostCrawford[1][s1];
                *p1++ = aafMETPostCrawford[1][s1];
            } else {
                *p0++ = aafMETPostCrawford[0][s0];
                *p1++ = 1.0f - aafMETPostCrawford[0][s0];
            }
        } else {                /* non-post-Crawford */
            *p0++ = aafMET[s0][s1];
            *p1++ = 1.0f - aafMET[s0][s1];
        }
    }

    /* results for player 0 are done, results for player 1 have the
     *  losses in cols 0-4 and 8-12, but we want them to be in the same
     *  order as results0 - e.g wins in cols 0-4, and 8-12
     */
    p0 = player1;
    p1 = player1 + NDL;
    for (i = 0; i < NDL; ++i) {
        f = *p0;
        *p0++ = *p1;
        *p1++ = f;
    }

    if (max_res > DTLBP0) {
        p0 += NDL;
        p1 += NDL;
        for (i = 0; i < NDL; ++i) {
            f = *p0;
            *p0++ = *p1;
            *p1++ = f;
        }
    }

    if (max_res > DTLBP1) {
        p0 += NDL;
        p1 += NDL;
        for (i = 0; i < NDL; ++i) {
            f = *p0;
            *p0++ = *p1;
            *p1++ = f;
        }
    }

}
