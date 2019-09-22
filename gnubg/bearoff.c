/*
 * bearoff.c
 *
 * by Joern Thyssen <jthyssen@dk.ibm.com>, 2002
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
 * $Id: bearoff.c,v 1.98 2014/12/03 15:43:04 plm Exp $
 */
#include "config.h"
/*must be first here because of strange warning from mingw */
#include "multithread.h"

#include "backgammon.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "bearoffgammon.h"
#include "positionid.h"

#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define HEURISTIC_C 15
#define HEURISTIC_P 6

static int
setGammonProb(const TanBoard anBoard, unsigned int bp0, unsigned int bp1, float *g0, float *g1)
{
    int i;
    unsigned short int prob[32];

    unsigned int tot0 = 0;
    unsigned int tot1 = 0;

    for (i = 5; i >= 0; --i) {
        tot0 += anBoard[0][i];
        tot1 += anBoard[1][i];
    }

    {
        g_assert(tot0 == 15 || tot1 == 15);
    }

    *g0 = 0.0;
    *g1 = 0.0;

    if (tot0 == 15) {
        struct GammonProbs *gp = getBearoffGammonProbs(anBoard[0]);
        double make[3];

        if (BearoffDist(pbc1, bp1, NULL, NULL, NULL, prob, NULL))
            return -1;

        make[0] = gp->p0 / 36.0;
        make[1] = (make[0] + gp->p1 / (36.0 * 36.0));
        make[2] = (make[1] + gp->p2 / (36.0 * 36.0 * 36.0));

        *g1 = (float) ((prob[1] / 65535.0) +
                       (1 - make[0]) * (prob[2] / 65535.0) +
                       (1 - make[1]) * (prob[3] / 65535.0) + (1 - make[2]) * (prob[4] / 65535.0));
    }

    if (tot1 == 15) {
        struct GammonProbs *gp = getBearoffGammonProbs(anBoard[1]);
        double make[3];

        if (BearoffDist(pbc1, bp0, NULL, NULL, NULL, prob, NULL))
            return -1;

        make[0] = gp->p0 / 36.0;
        make[1] = (make[0] + gp->p1 / (36.0 * 36.0));
        make[2] = (make[1] + gp->p2 / (36.0 * 36.0 * 36.0));

        *g0 = (float) ((prob[1] / 65535.0) * (1 - make[0]) + (prob[2] / 65535.0) * (1 - make[1])
                       + (prob[3] / 65535.0) * (1 - make[2]));
    }
    return 0;
}


static void
AverageRolls(const float arProb[32], float *ar)
{
    float sx;
    float sx2;
    int i;

    sx = sx2 = 0.0f;

    for (i = 1; i < 32; i++) {
        float p = i * arProb[i];
        sx += p;
        sx2 += i * p;
    }

    ar[0] = sx;
    ar[1] = sqrtf(sx2 - sx * sx);
}

/* Make a plausible bearoff move (used to create approximate bearoff database). */
static unsigned int
HeuristicBearoff(unsigned int anBoard[6], const unsigned int anRoll[2])
{
    unsigned int i,             /* current die being played */
     c,                         /* number of dice to play */
     nMax,                      /* highest occupied point */
     anDice[4], j, iSearch, nTotal;
    int n;                      /* point to play from */

    if (anRoll[0] == anRoll[1]) {
        /* doubles */
        anDice[0] = anDice[1] = anDice[2] = anDice[3] = anRoll[0];
        c = 4;
    } else {
        /* non-doubles */
        g_assert(anRoll[0] > anRoll[1]);

        anDice[0] = anRoll[0];
        anDice[1] = anRoll[1];
        c = 2;
    }

    for (i = 0; i < c; i++) {
        for (nMax = 5; nMax > 0; nMax--) {
            if (anBoard[nMax])
                break;
        }

        if (!anBoard[nMax])
            /* finished bearoff */
            break;

        do {
            if (anBoard[anDice[i] - 1]) {
                /* bear off exactly */
                n = (int) anDice[i] - 1;
                break;
            }

            if (anDice[i] - 1 > nMax) {
                /* bear off highest chequer */
                n = (int) nMax;
                break;
            }

            nTotal = anDice[i] - 1;
            for (n = -1, j = i + 1; j < c; j++) {
                nTotal += anDice[j];
                if (nTotal < 6 && anBoard[nTotal]) {
                    /* there's a chequer we can bear off with subsequent dice;
                     * do it */
                    n = (int) nTotal;
                    break;
                }
            }
            if (n >= 0)
                break;

            for (n = -1, iSearch = anDice[i]; iSearch <= nMax; iSearch++) {
                if (anBoard[iSearch] >= 2 &&    /* at least 2 on source point */
                    !anBoard[iSearch - anDice[i]] &&    /* dest empty */
                    (n == -1 || anBoard[iSearch] > anBoard[n]))
                    n = (int) iSearch;
            }
            if (n >= 0)
                break;

            /* find the point with the most on it (or least on dest) */
            for (iSearch = anDice[i]; iSearch <= nMax; iSearch++)
                if (n == -1 || anBoard[iSearch] > anBoard[n] ||
                    (anBoard[iSearch] == anBoard[n] &&
                     anBoard[iSearch - anDice[i]] < anBoard[(unsigned int) n - anDice[i]]))
                    n = (int) iSearch;

            g_assert(n >= 0);
        } while (n < 0);        /* Dummy loop to remove goto's */

        g_assert(anBoard[n]);
        anBoard[n]--;

        if (n >= (int) anDice[i])
            anBoard[n - (int) anDice[i]]++;
    }

    return PositionBearoff(anBoard, HEURISTIC_P, HEURISTIC_C);
}

static void
GenerateBearoff(unsigned char *p, unsigned int nId)
{
    unsigned int anRoll[2], anBoard[6], aProb[32];
    unsigned int i, iBest;
    unsigned short us;

    for (i = 0; i < 32; i++)
        aProb[i] = 0;

    for (anRoll[0] = 1; anRoll[0] <= 6; anRoll[0]++)
        for (anRoll[1] = 1; anRoll[1] <= anRoll[0]; anRoll[1]++) {
            PositionFromBearoff(anBoard, nId, HEURISTIC_P, HEURISTIC_C);
            iBest = HeuristicBearoff(anBoard, anRoll);

            g_assert(iBest < nId);

            if (anRoll[0] == anRoll[1])
                for (i = 0; i < 31; i++)
                    aProb[i + 1] += p[(iBest << 6) | (i << 1)] + (p[(iBest << 6) | (i << 1) | 1] << 8);
            else
                for (i = 0; i < 31; i++)
                    aProb[i + 1] += (p[(iBest << 6) | (i << 1)] +
                                     ((unsigned int) p[(iBest << 6) | (i << 1) | 1] << 8)) << 1;
        }

    for (i = 0; i < 32; i++) {
        us = (unsigned short) ((aProb[i] + 18) / 36);
        p[(nId << 6) | (i << 1)] = us & 0xFF;
        p[(nId << 6) | (i << 1) | 1] = us >> 8;
    }
}

static unsigned char *
HeuristicDatabase(void (*pfProgress) (unsigned int))
{
    unsigned char *pm = malloc(40 + 54264 * 64);
    unsigned char *p;
    unsigned int i;

    if (!pm)
        return NULL;

    p = pm + 40;
    p[0] = p[1] = 0xFF;
    for (i = 2; i < 64; i++)
        p[i] = 0;

    for (i = 1; i < 54264; i++) {
        GenerateBearoff(p, i);
        if (pfProgress && !(i % 1000))
            pfProgress(i);
    }

    return pm;
}


static void
ReadBearoffFile(const bearoffcontext * pbc, unsigned int offset, unsigned char *buf, unsigned int nBytes)
{
    MT_Exclusive();

    if ((fseek(pbc->pf, (long) offset, SEEK_SET) < 0) || (fread(buf, 1, nBytes, pbc->pf) < nBytes)) {
        if (errno)
            perror("OS bearoff database");
        else
            fprintf(stderr, "error reading OS bearoff database");

        memset(buf, 0, nBytes);
        return;
    }

    MT_Release();
}

/* BEAROFF_GNUBG: read two sided bearoff database */
static void
ReadTwoSidedBearoff(const bearoffcontext * pbc, const unsigned int iPos, float ar[4], unsigned short int aus[4])
{
    unsigned int i, k = (pbc->fCubeful) ? 4 : 1;
    unsigned char ac[8];
    unsigned char *pc = NULL;
    unsigned short int us;

    if (pbc->p)
        pc = pbc->p + 40 + 2 * iPos * k;
    else {
        ReadBearoffFile(pbc, 40 + 2 * iPos * k, ac, k * 2);
        pc = ac;
    }
    /* add to cache */

    for (i = 0; i < k; ++i) {
        us = pc[2 * i] | (unsigned short) (pc[2 * i + 1] << 8);
        if (aus)
            aus[i] = us;
        if (ar)
            ar[i] = us / 32767.5f - 1.0f;
    }
}

extern int
BearoffCubeful(const bearoffcontext * pbc, const unsigned int iPos, float ar[4], unsigned short int aus[4])
{
    g_return_val_if_fail(pbc, -1);
    g_return_val_if_fail(pbc->fCubeful, -1);

    ReadTwoSidedBearoff(pbc, iPos, ar, aus);
    return 0;
}


static int
BearoffEvalTwoSided(const bearoffcontext * pbc, const TanBoard anBoard, float arOutput[])
{

    unsigned int nUs = PositionBearoff(anBoard[1], pbc->nPoints, pbc->nChequers);
    unsigned int nThem = PositionBearoff(anBoard[0], pbc->nPoints, pbc->nChequers);
    unsigned int n = Combination(pbc->nPoints + pbc->nChequers, pbc->nPoints);
    unsigned int iPos = nUs * n + nThem;
    float ar[4];

    ReadTwoSidedBearoff(pbc, iPos, ar, NULL);

    memset(arOutput, 0, 5 * sizeof(float));
    arOutput[OUTPUT_WIN] = ar[0] / 2.0f + 0.5f;

    return 0;

}


static int
ReadHypergammon(const bearoffcontext * pbc, const unsigned int iPos, float arOutput[NUM_OUTPUTS], float arEquity[4])
{

    unsigned char ac[28];
    unsigned char *pc = NULL;
    unsigned int us;
    int i;
    const int x = 28;

    if (pbc->p)
        pc = pbc->p + 40 + x * iPos;
    else {
        ReadBearoffFile(pbc, 40 + x * iPos, ac, x);
        pc = ac;
    }

    if (arOutput)
        for (i = 0; i < NUM_OUTPUTS; ++i) {
            us = pc[3 * i] | (pc[3 * i + 1]) << 8 | (pc[3 * i + 2]) << 16;
            arOutput[i] = us / 16777215.0f;
        }

    if (arEquity)
        for (i = 0; i < 4; ++i) {
            us = pc[15 + 3 * i] | (pc[15 + 3 * i + 1]) << 8 | (pc[15 + 3 * i + 2]) << 16;
            arEquity[i] = (us / 16777215.0f - 0.5f) * 6.0f;
        }

    return 0;

}


static int
BearoffEvalOneSided(const bearoffcontext * pbc, const TanBoard anBoard, float arOutput[])
{
    int i, j;
    float aarProb[2][32];
    float aarGammonProb[2][32];
    float r;
    unsigned int anOn[2];
    unsigned int an[2];
    float ar[2][4];

    /* get bearoff probabilities */

    for (i = 0; i < 2; ++i) {

        an[i] = PositionBearoff(anBoard[i], pbc->nPoints, pbc->nChequers);
        if (BearoffDist(pbc, an[i], aarProb[i], aarGammonProb[i], ar[i], NULL, NULL))
            return -1;
    }

    /* calculate winning chance */

    r = 0.0;
    for (i = 0; i < 32; ++i)
        for (j = i; j < 32; ++j)
            r += aarProb[1][i] * aarProb[0][j];

    arOutput[OUTPUT_WIN] = r;

    /* calculate gammon chances */

    for (i = 0; i < 2; ++i)
        for (j = 0, anOn[i] = 0; j < 25; ++j)
            anOn[i] += anBoard[i][j];

    if (anOn[0] == 15 || anOn[1] == 15) {

        if (pbc->fGammon) {

            /* my gammon chance: I'm out in i rolls and my opponent isn't inside
             * home quadrant in less than i rolls */

            r = 0;
            for (i = 0; i < 32; i++)
                for (j = i; j < 32; j++)
                    r += aarProb[1][i] * aarGammonProb[0][j];

            arOutput[OUTPUT_WINGAMMON] = r;

            /* opp gammon chance */

            r = 0;
            for (i = 0; i < 32; i++)
                for (j = i + 1; j < 32; j++)
                    r += aarProb[0][i] * aarGammonProb[1][j];

            arOutput[OUTPUT_LOSEGAMMON] = r;

        } else {

            if (setGammonProb(anBoard, an[0], an[1], &arOutput[OUTPUT_LOSEGAMMON], &arOutput[OUTPUT_WINGAMMON]))
                return -1;

        }
    } else {
        /* no gammons possible */
        arOutput[OUTPUT_WINGAMMON] = 0.0f;
        arOutput[OUTPUT_LOSEGAMMON] = 0.0f;
    }


    /* no backgammons possible */

    arOutput[OUTPUT_LOSEBACKGAMMON] = 0.0f;
    arOutput[OUTPUT_WINBACKGAMMON] = 0.0f;

    return 0;
}


extern int
BearoffHyper(const bearoffcontext * pbc, const unsigned int iPos, float arOutput[], float arEquity[])
{

    return ReadHypergammon(pbc, iPos, arOutput, arEquity);

}


static int
BearoffEvalHypergammon(const bearoffcontext * pbc, const TanBoard anBoard, float arOutput[])
{

    unsigned int nUs = PositionBearoff(anBoard[1], pbc->nPoints, pbc->nChequers);
    unsigned int nThem = PositionBearoff(anBoard[0], pbc->nPoints, pbc->nChequers);
    unsigned int n = Combination(pbc->nPoints + pbc->nChequers, pbc->nPoints);
    unsigned int iPos = nUs * n + nThem;

    return ReadHypergammon(pbc, iPos, arOutput, NULL);
}

extern int
BearoffEval(const bearoffcontext * pbc, const TanBoard anBoard, float arOutput[])
{
    g_return_val_if_fail(pbc, 0);

    switch (pbc->bt) {
    case BEAROFF_TWOSIDED:
        return BearoffEvalTwoSided(pbc, anBoard, arOutput);
    case BEAROFF_ONESIDED:
        return BearoffEvalOneSided(pbc, anBoard, arOutput);
    case BEAROFF_HYPERGAMMON:
        return BearoffEvalHypergammon(pbc, anBoard, arOutput);
    case BEAROFF_INVALID:
    default:
        g_warning("Invalid type in BearoffEval");
        return 0;
    }
}

extern void
BearoffStatus(const bearoffcontext * pbc, char *sz)
{
    char buf[256];

    if (!pbc || (pbc->bt != BEAROFF_TWOSIDED && pbc->bt != BEAROFF_ONESIDED && pbc->bt != BEAROFF_HYPERGAMMON))
        return;

    /* Title */
    if (pbc->bt == BEAROFF_HYPERGAMMON) {
        if (pbc->p)
            sprintf(buf, _("In memory 2-sided exact %d-chequer Hypergammon database evaluator"), pbc->nChequers);
        else
            sprintf(buf, _("On disk 2-sided exact %d-chequer Hypergammon database evaluator"), pbc->nChequers);

    } else {
        if (pbc->p)
            sprintf(buf, _("In memory %d-sided bearoff database evaluator"), pbc->bt);
        else
            sprintf(buf, _("On disk %d-sided bearoff database evaluator"), pbc->bt);

    }
    sz += sprintf(sz, " * %s\n", buf);

    sz += sprintf(sz, "   - %s\n", _("generated by GNU Backgammon"));

    sprintf(buf, _("up to %d chequers on %d points (%d positions) per player"), pbc->nChequers, pbc->nPoints,
            Combination(pbc->nChequers + pbc->nPoints, pbc->nPoints));
    sz += sprintf(sz, "   - %s\n", buf);

    switch (pbc->bt) {
    case BEAROFF_TWOSIDED:
        sz += sprintf(sz, "   - %s\n", pbc->fCubeful ? _("database includes both cubeful and cubeless equities")
                      : _("cubeless database"));
        break;

    case BEAROFF_ONESIDED:
        if (pbc->fND)
            sz += sprintf(sz, "   - %s\n", _("distributions are approximated with a normal distribution"));
        if (pbc->fHeuristic)
            sz += sprintf(sz, "   - %s\n", _("with heuristic moves"));

        sz += sprintf(sz, "   - %s\n", pbc->fGammon ? _("database includes gammon distributions")
                      : _("database does not include gammon distributions"));
        break;
    case BEAROFF_HYPERGAMMON:
    case BEAROFF_INVALID:
    default:
        break;
    }
    sprintf(sz, "\n");
}

static int
BearoffDumpTwoSided(const bearoffcontext * pbc, const TanBoard anBoard, char *sz)
{
    unsigned int nUs = PositionBearoff(anBoard[1], pbc->nPoints, pbc->nChequers);
    unsigned int nThem = PositionBearoff(anBoard[0], pbc->nPoints, pbc->nChequers);
    unsigned int n = Combination(pbc->nPoints + pbc->nChequers, pbc->nPoints);
    unsigned int iPos = nUs * n + nThem;
    float ar[4];
    unsigned int i;
    const char *aszEquity[] = {
        N_("Cubeless equity"),
        N_("Owned cube"),
        N_("Centered cube"),
        N_("Opponent owns cube")
    };

    sprintf(sz + strlen(sz), "%19s %14s\n%s %12u  %12u\n\n", _("Player"), _("Opponent"), _("Position"), nUs, nThem);

    ReadTwoSidedBearoff(pbc, iPos, ar, NULL);

    if (pbc->fCubeful)
        for (i = 0; i < 4; ++i)
            sprintf(sz + strlen(sz), "%-30.30s: %+7.4f\n", gettext(aszEquity[i]), ar[i]);
    else
        sprintf(sz + strlen(sz), "%-30.30s: %+7.4f\n", gettext(aszEquity[0]), 2.0f * ar[0] - 1.0f);

    strcat(sz, "\n");

    return 0;

}


static int
BearoffDumpOneSided(const bearoffcontext * pbc, const TanBoard anBoard, char *sz)
{
    unsigned int nUs = PositionBearoff(anBoard[1], pbc->nPoints, pbc->nChequers);
    unsigned int nThem = PositionBearoff(anBoard[0], pbc->nPoints, pbc->nChequers);
    float ar[2][4];
    unsigned int i;
    float aarProb[2][32], aarGammonProb[2][32];
    int f0, f1, f2, f3;
    unsigned int anPips[2];
    const float x = (2 * 3 + 3 * 4 + 4 * 5 + 4 * 6 + 6 * 7 +
                     5 * 8 + 4 * 9 + 2 * 10 + 2 * 11 + 1 * 12 + 1 * 16 + 1 * 20 + 1 * 24) / 36.0f;


    if (BearoffDist(pbc, nUs, aarProb[0], aarGammonProb[0], ar[0], NULL, NULL))
        return -1;
    if (BearoffDist(pbc, nThem, aarProb[1], aarGammonProb[1], ar[1], NULL, NULL))
        return -1;

    sz += sprintf(sz, "%19s %14s\n%s %12u  %12u\n\n", _("Player"), _("Opponent"), _("Position"), nUs, nThem);

    sz += sprintf(sz, "%s \t\t\t\t%s\n", _("Bearing off"), _("Bearing at least one chequer off"));
    sz += sprintf(sz, "%s\t%s\t%s\t%s\t%s\n", _("Rolls"), _("Player"), _("Opponent"), _("Player"), _("Opponent"));

    f0 = f1 = f2 = f3 = FALSE;

    for (i = 0; i < 32; i++) {

        if (aarProb[0][i] > 0.0f)
            f0 = TRUE;

        if (aarProb[1][i] > 0.0f)
            f1 = TRUE;

        if (aarGammonProb[0][i] > 0.0f)
            f2 = TRUE;

        if (aarGammonProb[1][i] > 0.0f)
            f3 = TRUE;

        if (f0 || f1 || f2 || f3) {
            if (f0 && f1 && ((f2 && f3) || !pbc->fGammon) &&
                aarProb[0][i] == 0.0f && aarProb[1][i] == 0.0f &&
                ((aarGammonProb[0][i] == 0.0f && aarGammonProb[1][i] == 0.0f) || !pbc->fGammon))
                break;

            sprintf(sz = sz + strlen(sz),
                    "%5u\t%7.3f\t%7.3f" "\t\t", i, aarProb[0][i] * 100.0f, aarProb[1][i] * 100.0f);

            if (pbc->fGammon)
                sprintf(sz = sz + strlen(sz),
                        "%7.3f\t%7.3f\n", aarGammonProb[0][i] * 100.0f, aarGammonProb[1][i] * 100.0f);
            else
                sprintf(sz = sz + strlen(sz), "%-7.7s\t%-7.7s\n", _("n/a"), _("n/a"));


        }
    }

    sz += sprintf(sz, "\n%s\n", _("Average rolls"));
    sz += sprintf(sz, "%s\t\t\t\t%s\n", _("Bearing off"), _("Saving gammon"));
    sz += sprintf(sz, "\t%s\t%s\t%s\t%s\n", _("Player"), _("Opponent"), _("Player"), _("Opponent"));

    /* mean rolls */

    sz += sprintf(sz, "%s\t%7.3f\t%7.3f\t\t", _("Mean"), ar[0][0], ar[1][0]);

    if (pbc->fGammon)
        sz += sprintf(sz, "%7.3f\t%7.3f\n", ar[0][2], ar[1][2]);
    else
        sz += sprintf(sz, "%-7.7s\t%-7.7s\n", _("n/a"), _("n/a"));

    /* std. dev */

    sz += sprintf(sz, "%s\t%7.3f\t%7.3f\t\t", _("Std dev"), ar[0][1], ar[1][1]);

    if (pbc->fGammon)
        sz += sprintf(sz, "%7.3f\t%7.3f\n", ar[0][3], ar[1][3]);
    else
        sz += sprintf(sz, "%-7.7s\t%-7.7s\n", _("n/a"), _("n/a"));


    /* effective pip count */

    PipCount(anBoard, anPips);

    sz += sprintf(sz, "\n%s:\n", _("Effective pip count"));
    sz += sprintf(sz, "\t%s\t%s\n", _("Player"), _("Opponent"));
    sz += sprintf(sz, "%s\t%7.3f\t%7.3f\n%s\t%7.3f\t%7.3f\n\n", _("EPC"),
                  ar[0][0] * x, ar[1][0] * x, _("Wastage"), ar[0][0] * x - anPips[1], ar[1][0] * x - anPips[0]);

    sprintf(sz, "%s = %5.3f * %s\n%s = %s - %s\n\n",
            _("EPC"), x, _("Average rolls"), _("Wastage"), _("EPC"), _("pips"));

    return 0;

}

static int
BearoffDumpHyper(const bearoffcontext * pbc, const TanBoard anBoard, char *sz)
{
    unsigned int nUs = PositionBearoff(anBoard[1], pbc->nPoints, pbc->nChequers);
    unsigned int nThem = PositionBearoff(anBoard[0], pbc->nPoints, pbc->nChequers);
    unsigned int n = Combination(pbc->nPoints + pbc->nChequers, pbc->nPoints);
    unsigned int iPos = nUs * n + nThem;
    float ar[4];
    unsigned int i;
    const char *aszEquity[] = {
        N_("Owned cube"),
        N_("Centered cube"),
        N_("Centered cube (Jacoby rule)"),
        N_("Opponent owns cube")
    };

    if (BearoffHyper(pbc, iPos, NULL, ar))
        return -1;

    sprintf(sz + strlen(sz), "%19s %14s\n%s %12u  %12u\n\n", _("Player"), _("Opponent"), _("Position"), nUs, nThem);

    for (i = 0; i < 4; ++i)
        sprintf(sz + strlen(sz), "%-30.30s: %+7.4f\n", gettext(aszEquity[i]), ar[i]);

    return 0;
}

extern int
BearoffDump(const bearoffcontext * pbc, const TanBoard anBoard, char *sz)
{
    g_return_val_if_fail(pbc, -1);

    switch (pbc->bt) {
    case BEAROFF_TWOSIDED:
        return BearoffDumpTwoSided(pbc, anBoard, sz);
    case BEAROFF_ONESIDED:
        return BearoffDumpOneSided(pbc, anBoard, sz);
    case BEAROFF_HYPERGAMMON:
        return BearoffDumpHyper(pbc, anBoard, sz);
    case BEAROFF_INVALID:
    default:
        g_warning("Invalid type in BearoffDump");
        return -1;
    }
}

extern void
BearoffClose(bearoffcontext * pbc)
{
    if (!pbc)
        return;

    if (pbc->pf)
        fclose(pbc->pf);

#if GLIB_CHECK_VERSION(2,8,0)
    if (pbc->map) {
#if GLIB_CHECK_VERSION(2,22,0)
        g_mapped_file_unref(pbc->map);
#else
        g_mapped_file_free(pbc->map);
#endif
        pbc->p = NULL;
    }
#endif

    if (pbc->p)
        free(pbc->p);

    if (pbc->szFilename)
        g_free(pbc->szFilename);

    g_free(pbc);
}

#if GLIB_CHECK_VERSION(2,8,0)
static unsigned char *
ReadIntoMemory(bearoffcontext * pbc)
{
    GError *error = NULL;
    pbc->map = g_mapped_file_new(pbc->szFilename, FALSE, &error);
    if (!pbc->map) {
        g_printerr(_("%s: Failed to map bearoffdatabase %s\n"), pbc->szFilename, error->message);
        g_error_free(error);
        return NULL;
    }
    pbc->p = (unsigned char *) g_mapped_file_get_contents(pbc->map);
    return pbc->p;
}
#endif

/*
 * Check whether this is a exact bearoff file 
 *
 * The first long must be 73457356
 * The second long must be 100
 *
 */

static unsigned int
MakeInt(unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
    return (a | (unsigned char) b << 8 | (unsigned char) c << 16 | (unsigned char) d << 24);
}

static void
InvalidDb(bearoffcontext * pbc)
{
    if (errno) {
        const char *fn = pbc->szFilename ? pbc->szFilename : "";
        g_printerr("%s(%s): %s\n", _("Bearoff Database"), fn, g_strerror(errno));
    }
    BearoffClose(pbc);
}

/*
 * Initialise bearoff database
 *
 * Input:
 *   szFilename: the filename of the database to open
 *
 * Returns:
 *   pointer to bearoff context on succes; NULL on error
 *
 * Garbage collect:
 *   caller must free returned pointer if not NULL.
 *
 */
extern bearoffcontext *
BearoffInit(const char *szFilename, const int bo, void (*p) (unsigned int))
{
    bearoffcontext *pbc;
    char sz[41];

    pbc = g_new0(bearoffcontext, 1);

    if (bo & (int) BO_HEURISTIC) {
        pbc->bt = BEAROFF_ONESIDED;
        pbc->nPoints = HEURISTIC_P;
        pbc->nChequers = HEURISTIC_C;
        pbc->fHeuristic = TRUE;
        pbc->p = HeuristicDatabase(p);
        return pbc;
    }

    errno = 0;

    if (!szFilename || !*szFilename) {
        g_printerr("%s\n", _("No database filename provided"));
        InvalidDb(pbc);
        return NULL;
    }
    pbc->szFilename = g_strdup(szFilename);

    if (!g_file_test(szFilename, G_FILE_TEST_IS_REGULAR)) {
        /* fail silently */
        errno = 0;
        InvalidDb(pbc);
        return NULL;
    }


    if ((pbc->pf = g_fopen(szFilename, "rb")) == 0) {
        g_printerr("%s\n", _("Invalid or nonexistent database"));
        InvalidDb(pbc);
        return NULL;
    }
    /* 
     * Read header bearoff file
     */

    /* read header */

    if (fread(sz, 1, 40, pbc->pf) < 40) {
        g_printerr("%s\n", _("Database read failed"));
        InvalidDb(pbc);
        return NULL;
    }

    /* detect bearoff program */

    if (strncmp(sz, "gnubg", 5) != 0) {
        g_printerr("%s\n", _("Unknown bearoff database"));
        InvalidDb(pbc);
        return NULL;
    }

    /* one sided or two sided? */

    if (!strncmp(sz + 6, "TS", 2))
        pbc->bt = BEAROFF_TWOSIDED;
    else if (!strncmp(sz + 6, "OS", 2))
        pbc->bt = BEAROFF_ONESIDED;
    else if (*(sz + 6) == 'H')
        pbc->bt = BEAROFF_HYPERGAMMON;
    else {
        g_printerr("%s: %s\n (%s: '%2s')\n", szFilename, _("incomplete bearoff database"), _("illegal bearoff type"),
                   sz + 6);
        InvalidDb(pbc);
        return NULL;
    }

    if (pbc->bt == BEAROFF_TWOSIDED || pbc->bt == BEAROFF_ONESIDED) {

        /* normal onesided or twosided bearoff database */

        /* number of points */

        pbc->nPoints = (unsigned) atoi(sz + 9);
        if (pbc->nPoints < 1 || pbc->nPoints >= 24) {
            g_printerr("%s: %s\n (%s: %d)\n", szFilename, _("incomplete bearoff database"),
                       _("illegal number of points"), pbc->nPoints);
            InvalidDb(pbc);
            return NULL;
        }

        /* number of chequers */

        pbc->nChequers = (unsigned) atoi(sz + 12);
        if (pbc->nChequers < 1 || pbc->nChequers > 15) {
            g_printerr("%s: %s\n (%s: %d)", szFilename, _("incomplete bearoff database"),
                       _("illegal number of chequers"), pbc->nChequers);
            InvalidDb(pbc);
            return NULL;
        }

    } else {

        /* hypergammon database */

        pbc->nPoints = 25;
        pbc->nChequers = (unsigned) atoi(sz + 7);

    }
    switch (pbc->bt) {
    case BEAROFF_TWOSIDED:
        /* options for two-sided dbs */
        pbc->fCubeful = atoi(sz + 15);
        break;
    case BEAROFF_ONESIDED:
        /* options for one-sided dbs */
        pbc->fGammon = atoi(sz + 15);
        pbc->fCompressed = atoi(sz + 17);
        pbc->fND = atoi(sz + 19);
        break;
    case BEAROFF_HYPERGAMMON:
    case BEAROFF_INVALID:
    default:
        break;
    }

    /* 
     * read database into memory if requested 
     */

#if GLIB_CHECK_VERSION(2,8,0)
    if (bo & (int) BO_IN_MEMORY) {
        fclose(pbc->pf);
        pbc->pf = NULL;
        if ((ReadIntoMemory(pbc) == NULL))
            if ((pbc->pf = g_fopen(szFilename, "rb")) == 0) {
                g_printerr("%s\n", _("Invalid or nonexistent database"));
                InvalidDb(pbc);
                return NULL;
            }
    }
#endif
    return pbc;
}

extern float
fnd(const float x, const float mu, const float sigma)
{

    const float epsilon = 1.0e-7f;

    if (sigma <= epsilon)
        /* dirac delta function */
        return (fabsf(mu - x) < epsilon) ? 1.0f : 0.0f;
    else {

        float xm = (x - mu) / sigma;

        return 1.0f / (sigma * sqrtf(2.0f * (float) G_PI) * expf(-xm * xm / 2.0f));

    }

}



static int
ReadBearoffOneSidedND(const bearoffcontext * pbc,
                      const unsigned int nPosID,
                      float arProb[32], float arGammonProb[32],
                      float *ar, unsigned short int ausProb[32], unsigned short int ausGammonProb[32])
{

    unsigned char ac[16];
    float arx[4];
    int i;
    float r;

    ReadBearoffFile(pbc, 40 + nPosID * 16, ac, 16);

    memcpy(arx, ac, 16);

    if (arProb || ausProb)
        for (i = 0; i < 32; ++i) {
            r = fnd(1.0f * i, arx[0], arx[1]);
            if (arProb)
                arProb[i] = r;
            if (ausProb)
                ausProb[i] = (unsigned short) (r * 65535.0f);
        }



    if (arGammonProb || ausGammonProb)
        for (i = 0; i < 32; ++i) {
            r = fnd(1.0f * i, arx[2], arx[3]);
            if (arGammonProb)
                arGammonProb[i] = r;
            if (ausGammonProb)
                ausGammonProb[i] = (unsigned short) (r * 65535.0f);
        }

    if (ar)
        memcpy(ar, arx, 16);

    return 0;

}


static void
AssignOneSided(float arProb[32], float arGammonProb[32],
               float ar[4],
               unsigned short int ausProb[32],
               unsigned short int ausGammonProb[32],
               const unsigned short int ausProbx[32], const unsigned short int ausGammonProbx[32])
{

    int i;
    float arx[64];

    if (ausProb)
        memcpy(ausProb, ausProbx, 32 * sizeof(ausProb[0]));

    if (ausGammonProb)
        memcpy(ausGammonProb, ausGammonProbx, 32 * sizeof(ausGammonProbx[0]));

    if (ar || arProb || arGammonProb) {
        for (i = 0; i < 32; ++i)
            arx[i] = ausProbx[i] / 65535.0f;

        for (i = 0; i < 32; ++i)
            arx[32 + i] = ausGammonProbx[i] / 65535.0f;

        if (arProb)
            memcpy(arProb, arx, 32 * sizeof(float));
        if (arGammonProb)
            memcpy(arGammonProb, arx + 32, 32 * sizeof(float));
        if (ar) {
            AverageRolls(arx, ar);
            AverageRolls(arx + 32, ar + 2);
        }
    }
}


static void
CopyBytes(unsigned short int aus[64],
          const unsigned char ac[128],
          const unsigned int nz, const unsigned int ioff, const unsigned int nzg, const unsigned int ioffg)
{

    unsigned int i, j;

    i = 0;
    memset(aus, 0, 64 * sizeof(unsigned short int));
    for (j = 0; j < nz; ++j, i += 2)
        aus[ioff + j] = ac[i] | (unsigned short int) (ac[i + 1] << 8);

    for (j = 0; j < nzg; ++j, i += 2)
        aus[32 + ioffg + j] = (unsigned short int) (ac[i] | ac[i + 1] << 8);
}

static unsigned short int *
GetDistCompressed(unsigned short int aus[64], const bearoffcontext * pbc, const unsigned int nPosID)
{
    unsigned char *puch;
    unsigned char ac[128];
    unsigned int iOffset;
    unsigned int nBytes;
    unsigned int ioff, nz, ioffg = 0, nzg = 0;
    unsigned int nPos = Combination(pbc->nPoints + pbc->nChequers, pbc->nPoints);
    unsigned int index_entry_size = pbc->fGammon ? 8 : 6;

    /* find offsets and no. of non-zero elements */

    if (pbc->p)
        /* database is in memory */
        puch = pbc->p + 40 + nPosID * index_entry_size;
    else {
        ReadBearoffFile(pbc, 40 + nPosID * index_entry_size, ac, index_entry_size);
        puch = ac;
    }

    /* find offset */

    iOffset = MakeInt(puch[0], puch[1], puch[2], puch[3]);

    nz = puch[4];
    ioff = puch[5];
    if (pbc->fGammon) {
        nzg = puch[6];
        ioffg = puch[7];
    }
    /* Sanity checks */

    if ((iOffset > 64 * nPos && 64 * nPos > 0) || nz > 32 || ioff > 32 || nzg > 32 || ioffg > 32) {
        fprintf(stderr,
                "The bearoff file '%s' is likely to be corrupted.\n"
                "Please check that the MD5 sum is the same as documented "
                "in the GNU Backgammon manual.\n"
                "Offset %lu, dist size %u (offset %u), "
                "gammon dist size %u (offset %u)\n", pbc->szFilename, (unsigned long) iOffset, nz, ioff, nzg, ioffg);
        g_assert_not_reached();
    }

    /* read prob + gammon probs */

    iOffset = 40                /* the header */
        + nPos * index_entry_size       /* the offset data */
        + 2 * iOffset;          /* offset to current position */

    /* read values */

    nBytes = 2 * (nz + nzg);

    /* get distribution */

    if (pbc->p)
        /* from memory */
        puch = pbc->p + iOffset;
    else {
        /* from disk */
        ReadBearoffFile(pbc, iOffset, ac, nBytes);
        puch = ac;

    }

    CopyBytes(aus, puch, nz, ioff, nzg, ioffg);

    return aus;
}

static unsigned short int *
GetDistUncompressed(unsigned short int aus[64], const bearoffcontext * pbc, const unsigned int nPosID)
{
    unsigned char ac[128];
    unsigned char *puch;
    unsigned int iOffset;

    /* read from file */

    iOffset = 40 + 64 * nPosID * (pbc->fGammon ? 2 : 1);

    if (pbc->p)
        /* from memory */
        puch = pbc->p + iOffset;
    else {
        /* from disk */

        ReadBearoffFile(pbc, iOffset, ac, pbc->fGammon ? 128 : 64);
        puch = ac;
    }

    CopyBytes(aus, puch, 32, 0, 32, 0);

    return aus;

}


static int
ReadBearoffOneSidedExact(const bearoffcontext * pbc, const unsigned int nPosID,
                         float arProb[32], float arGammonProb[32],
                         float ar[4], unsigned short int ausProb[32], unsigned short int ausGammonProb[32])
{
    unsigned short int aus[64];
    unsigned short int *pus = NULL;

    /* get distribution */
    if (pbc->fCompressed)
        pus = GetDistCompressed(aus, pbc, nPosID);
    else
        pus = GetDistUncompressed(aus, pbc, nPosID);

    if (!pus) {
        printf("argh!\n");
        return -1;
    }

    AssignOneSided(arProb, arGammonProb, ar, ausProb, ausGammonProb, pus, pus + 32);

    return 0;
}

extern int
BearoffDist(const bearoffcontext * pbc, const unsigned int nPosID,
            float arProb[32], float arGammonProb[32],
            float ar[4], unsigned short int ausProb[32], unsigned short int ausGammonProb[32])
{
    g_return_val_if_fail(pbc, -1);
    g_return_val_if_fail(pbc->bt == BEAROFF_ONESIDED, -1);
    if (pbc->fND)
        return ReadBearoffOneSidedND(pbc, nPosID, arProb, arGammonProb, ar, ausProb, ausGammonProb);
    else
        return ReadBearoffOneSidedExact(pbc, nPosID, arProb, arGammonProb, ar, ausProb, ausGammonProb);
}

extern int
isBearoff(const bearoffcontext * pbc, const TanBoard anBoard)
{
    unsigned int i, nOppBack, nBack;
    unsigned int n = 0, nOpp = 0;

    if (!pbc)
        return FALSE;

    for (nOppBack = 24; nOppBack > 0; nOppBack--) {
        if (anBoard[0][nOppBack])
            break;
    }
    for (nBack = 24; nBack > 0; nBack--) {
        if (anBoard[1][nBack])
            break;
    }
    if (!anBoard[0][nOppBack] || !anBoard[1][nBack])
        /* the game is over */
        return FALSE;

    if ((nBack + nOppBack > 22) && !(pbc->bt == BEAROFF_HYPERGAMMON))
        /* contact position */
        return FALSE;

    for (i = 0; i <= nOppBack; ++i)
        nOpp += anBoard[0][i];

    for (i = 0; i <= nBack; ++i)
        n += anBoard[1][i];

    if (n <= pbc->nChequers && nOpp <= pbc->nChequers && nBack < pbc->nPoints && nOppBack < pbc->nPoints)
        return TRUE;
    else
        return FALSE;
}
