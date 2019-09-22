/*
 * makebearoff.c
 *
 * by Gary Wong <gary@cs.arizona.edu>, 1997-1999.
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
 * $Id: makebearoff.c,v 1.96 2014/12/03 15:43:37 plm Exp $
 */

#include "config.h"

#include <stdio.h>
#ifdef WIN32
#include <io.h>
#endif
#include <stdlib.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <math.h>
#include <errno.h>
#include "eval.h"
#include "positionid.h"
#include "bearoff.h"
#include "util.h"
#include "backgammon.h"
#include <glib/gstdio.h>
#include "glib-ext.h"
#include "multithread.h"

void
MT_CloseThreads(void)
{
    return;
}

typedef struct _xhashent {
    void *p;
    unsigned int iKey;
} xhashent;

typedef struct _xhash {
    unsigned long int nQueries, nHits, nEntries, nOverwrites;
    int nHashSize;
    xhashent *phe;
} xhash;


static long cLookup;

static int
XhashPosition(xhash * ph, const int iKey)
{

    return iKey % ph->nHashSize;

}

static void
XhashStatus(xhash * ph)
{

    fprintf(stderr, "Xhash status:\n");
    fprintf(stderr, "Size:    %d elements\n", ph->nHashSize);
    fprintf(stderr, "Queries: %lu (hits: %lu)\n", ph->nQueries, ph->nHits);
    fprintf(stderr, "Entries: %lu (overwrites: %lu)\n", ph->nEntries, ph->nOverwrites);

}


static int
XhashCreate(xhash * ph, const int nHashSize)
{

    int i;

    if (!(ph->phe = (xhashent *) malloc(nHashSize * sizeof(xhashent)))) {
        perror("xhashtable");
        return -1;
    }

    ph->nQueries = 0;
    ph->nHits = 0;
    ph->nEntries = 0;
    ph->nOverwrites = 0;
    ph->nHashSize = nHashSize;

    for (i = 0; i < nHashSize; ++i)
        ph->phe[i].p = NULL;

    return 0;

}


static void
XhashDestroy(xhash * ph)
{

    int i;

    for (i = 0; i < ph->nHashSize; ++i)
        if (ph->phe[i].p)
            free(ph->phe[i].p);
    free(ph->phe);
}


static void
XhashAdd(xhash * ph, const unsigned int iKey, const void *data, const int size)
{

    int l = XhashPosition(ph, iKey);

    if (ph->phe[l].p) {
        /* occupied */
        free(ph->phe[l].p);
        ph->nOverwrites++;
    } else {

        /* free */
        ph->nEntries++;

    }

    ph->phe[l].iKey = iKey;
    ph->phe[l].p = malloc(size);
    memcpy(ph->phe[l].p, data, size);

}


static void *
XhashLookup(xhash * ph, const unsigned int iKey)
{


    int l = XhashPosition(ph, iKey);

    ++ph->nQueries;

    if (ph->phe[l].p && ph->phe[l].iKey == iKey) {
        /* hit */
        ++ph->nHits;
        return (ph->phe[l].p);
    } else
        /* miss */
        return NULL;

}


static int
OSLookup(const unsigned int iPos,
         const int UNUSED(nPoints),
         unsigned short int aProb[64], const int fGammon, const int fCompress, FILE * pfOutput, FILE * pfTmp)
{

    unsigned int i, j;
    unsigned char ac[128];

    g_assert(pfOutput != stdin);

    if (fCompress) {

        long iOffset;
        size_t nBytes;
        unsigned int ioff, nz, ioffg = 0, nzg = 0;
        unsigned short int us;
        unsigned int index_entry_size = fGammon ? 8 : 6;

        /* find offsets and no. of non-zero elements */

        if (fseek(pfOutput, 40 + iPos * index_entry_size, SEEK_SET) < 0) {
            perror("output file");
            exit(-1);
        }

        if (fread(ac, 1, index_entry_size, pfOutput) < index_entry_size) {
            if (errno)
                perror("output file");
            else
                fprintf(stderr, "error reading output file\n");
            exit(-1);
        }

        iOffset = ac[0] | ac[1] << 8 | ac[2] << 16 | ac[3] << 24;

        nz = ac[4];
        ioff = ac[5];

        if (fGammon) {
            nzg = ac[6];
            ioffg = ac[7];
        }

        /* re-position at EOF */

        if (fseek(pfOutput, 0L, SEEK_END) < 0) {
            perror("output file");
            exit(-1);
        }

        /* read distributions */

        iOffset = 2 * iOffset;

        nBytes = 2 * (nz + nzg);

        if (fseek(pfTmp, iOffset, SEEK_SET) < 0) {
            perror("fseek'ing temp file");
            exit(-1);
        }

        /* read data */

        if (fread(ac, 1, nBytes, pfTmp) < nBytes) {
            if (errno)
                perror("reading temp file");
            else
                fprintf(stderr, "error reading temp file");
            exit(-1);
        }

        memset(aProb, 0, 128);

        i = 0;
        for (j = 0; j < nz; ++j, i += 2) {
            us = (unsigned short) (ac[i] | ac[i + 1] << 8);
            aProb[ioff + j] = us;
        }

        /* if ( fGammon ) */
        for (j = 0; j < nzg; ++j, i += 2) {
            us = (unsigned short) (ac[i] | ac[i + 1] << 8);
            aProb[32 + ioffg + j] = us;
        }

        /* re-position at EOF */

        if (fseek(pfTmp, 0L, SEEK_END) < 0) {
            perror("fseek'ing to EOF on temp file");
            exit(-1);
        }

    } else {

        /* look up position by seeking */

        if (fseek(pfOutput, 40 + iPos * (fGammon ? 128 : 64), SEEK_SET) < 0) {
            fprintf(stderr, "error seeking in pfOutput\n");
            exit(-1);
        }

        /* read distribution */

        if (fread(ac, 1, fGammon ? 128 : 64, pfOutput) < (fGammon ? 128 : 64)) {
            fprintf(stderr, "error readung from pfOutput\n");
            exit(-1);
        }

        /* save distribution */

        i = 0;
        for (j = 0; j < 32; ++j, i += 2)
            aProb[j] = (unsigned short) (ac[i] | ac[i + 1] << 8);

        if (fGammon)
            for (j = 0; j < 32; ++j, i += 2)
                aProb[j + 32] = (unsigned short) (ac[i] | ac[i + 1] << 8);

        /* position cursor at end of file */

        if (fseek(pfOutput, 0L, SEEK_END) < 0) {
            fprintf(stderr, "error seeking to end!\n");
            exit(-1);
        }

    }

    ++cLookup;

    return 0;

}


static void
CalcIndex(const unsigned short int aProb[32], unsigned int *piIdx, unsigned int *pnNonZero)
{

    int i;
    int j = 32;


    /* find max non-zero element */

    for (i = 31; i >= 0; --i)
        if (aProb[i]) {
            j = i;
            break;
        }

    /* find min non-zero element */

    *piIdx = 0;
    for (i = 0; i <= j; ++i)
        if (aProb[i]) {
            *piIdx = i;
            break;
        }


    *pnNonZero = j - *piIdx + 1;

}


static unsigned int
RollsOS(const unsigned short int aus[32])
{

    int i;
    unsigned int j;

    for (j = 0, i = 1; i < 32; i++)
        j += i * aus[i];

    return j;

}

static void
BearOff(int nId, unsigned int nPoints,
        unsigned short int aOutProb[64],
        const int fGammon, xhash * ph, bearoffcontext * pbc, const int fCompress, FILE * pfOutput, FILE * pfTmp)
{
#if !defined(G_DISABLE_ASSERT)
    int iBest;
#endif
    int iMode, j, anRoll[2], aProb[64];
    unsigned int i;
    TanBoard anBoard, anBoardTemp;
    movelist ml;
    int k;
    unsigned int us;
    unsigned int usBest;
    unsigned short int *pusj;
    unsigned short int ausj[64];
    unsigned short int ausBest[32];

    unsigned int usGammonBest;
    unsigned short int ausGammonBest[32];
    unsigned int nBack;

    /* get board for given position */

    PositionFromBearoff(anBoard[1], nId, nPoints, 15);

    /* initialise probabilities */

    for (i = 0; i < 64; i++)
        aProb[i] = aOutProb[i] = 0;

    /* all chequers off is easy :-) */

    if (!nId) {
        aOutProb[0] = 0xFFFF;
        aOutProb[32] = 0xFFFF;
        return;
    }

    /* initialise the remainder of the board */

    for (i = nPoints; i < 25; i++)
        anBoard[1][i] = 0;

    for (i = 0; i < 25; i++)
        anBoard[0][i] = 0;

    nBack = 0;
    for (i = 0, k = 0; i < nPoints; ++i) {
        if (anBoard[1][i])
            nBack = i;
        k += anBoard[1][i];
    }

    /* look for position in existing bearoff file */

    if (pbc && nBack < pbc->nPoints) {
        unsigned int nPosID = PositionBearoff(anBoard[1],
                                              pbc->nPoints, pbc->nChequers);
        BearoffDist(pbc, nPosID, NULL, NULL, NULL, aOutProb, aOutProb + 32);
        return;
    }

    if (k < 15)
        aProb[32] = 0xFFFF * 36;

    for (anRoll[0] = 1; anRoll[0] <= 6; anRoll[0]++)
        for (anRoll[1] = 1; anRoll[1] <= anRoll[0]; anRoll[1]++) {
            GenerateMoves(&ml, (ConstTanBoard) anBoard, anRoll[0], anRoll[1], FALSE);

            usBest = 0xFFFFFFFF;
#if !defined(G_DISABLE_ASSERT)
            iBest = -1;
#endif
            usGammonBest = 0xFFFFFFFF;

            for (i = 0; i < ml.cMoves; i++) {
                PositionFromKey(anBoardTemp, &ml.amMoves[i].key);

                j = PositionBearoff(anBoardTemp[1], nPoints, 15);

                g_assert(j >= 0);
                g_assert(j < nId);


                if (!j) {

                    memset(pusj = ausj, 0, fGammon ? 128 : 64);
                    pusj[0] = 0xFFFF;
                    pusj[32] = 0xFFFF;

                } else if (!(pusj = XhashLookup(ph, j))) {
                    /* look up in file generated so far */
                    pusj = ausj;
                    OSLookup(j, nPoints, pusj, fGammon, fCompress, pfOutput, pfTmp);

                    XhashAdd(ph, j, pusj, fGammon ? 128 : 64);
                }

                /* find best move to win */

                if ((us = RollsOS(pusj)) < usBest) {
#if !defined(G_DISABLE_ASSERT)
                    iBest = j;
#endif
                    usBest = us;
                    memcpy(ausBest, pusj, 64);
                }

                /* find best move to save gammon */

                if (fGammon && ((us = RollsOS(pusj + 32)) < usGammonBest)) {
                    usGammonBest = us;
                    memcpy(ausGammonBest, pusj + 32, 64);
                }

            }

            g_assert(iBest >= 0);
            /* g_assert( iGammonBest >= 0 ); */

            if (anRoll[0] == anRoll[1]) {
                for (i = 0; i < 31; i++) {
                    aProb[i + 1] += ausBest[i];
                    if (k == 15 && fGammon)
                        aProb[32 + i + 1] += ausGammonBest[i];
                }
            } else {
                for (i = 0; i < 31; i++) {
                    aProb[i + 1] += 2 * ausBest[i];
                    if (k == 15 && fGammon)
                        aProb[32 + i + 1] += 2 * ausGammonBest[i];
                }
            }
        }

    for (i = 0, j = 0, iMode = 0; i < 32; i++) {
        j += (aOutProb[i] = (unsigned short) ((aProb[i] + 18) / 36));
        if (aOutProb[i] > aOutProb[iMode])
            iMode = i;
    }

    aOutProb[iMode] -= (j - 0xFFFF);

    /* gammon probs */

    if (fGammon) {
        for (i = 0, j = 0, iMode = 0; i < 32; i++) {
            j += (aOutProb[32 + i] = (unsigned short) ((aProb[32 + i] + 18) / 36));
            if (aOutProb[32 + i] > aOutProb[32 + iMode])
                iMode = i;
        }

        aOutProb[32 + iMode] -= (j - 0xFFFF);
    }

}


static void
WriteOS(const unsigned short int aus[32], const int fCompress, FILE * output)
{

    unsigned int iIdx, nNonZero;
    unsigned int j;

    if (fCompress)
        CalcIndex(aus, &iIdx, &nNonZero);
    else {
        iIdx = 0;
        nNonZero = 32;
    }

    for (j = iIdx; j < iIdx + nNonZero; j++) {
        putc(aus[j] & 0xFF, output);
        putc(aus[j] >> 8, output);
    }

}

static void
WriteIndex(unsigned int *pnpos, const unsigned short int aus[64], const int fGammon, FILE * output)
{

    unsigned int iIdx, nNonZero;

    /* write offset */

    putc(*pnpos & 0xFF, output);
    putc((*pnpos >> 8) & 0xFF, output);
    putc((*pnpos >> 16) & 0xFF, output);
    putc((*pnpos >> 24) & 0xFF, output);

    /* write index and number of non-zero elements */

    CalcIndex(aus, &iIdx, &nNonZero);

    putc(nNonZero & 0xFF, output);
    putc(iIdx & 0xFF, output);

    *pnpos += nNonZero;

    /* gammon probs: write index and number of non-zero elements */

    if (fGammon) {
        CalcIndex(aus + 32, &iIdx, &nNonZero);
        putc(nNonZero & 0xFF, output);
        putc(iIdx & 0xFF, output);
        *pnpos += nNonZero;
    }
}

static void
WriteFloat(const float r, FILE * output)
{

    int j;
    const unsigned char *pc;

    pc = (const unsigned char *) &r;

    for (j = 0; j < 4; ++j)
        putc(*(pc++), output);

}



/*
 * Generate one sided bearoff database
 *
 * ! fCompress:
 *   write database directly to stdout
 *
 * fCompress:
 *   write index directly to stdout
 *   and write actual db to tmp file
 *   the tmp file is concatenated to the index when
 *   the generation is done.
 *
 */


static int
generate_os(const int nOS, const int fHeader,
            const int fCompress, const int fGammon, const int nHashSize, bearoffcontext * pbc, FILE * output)
{

    int i;
    int n;
    unsigned short int aus[64];
    xhash h;
    FILE *pfTmp = NULL;
    unsigned int npos;
    char *tmpfile;
    int fTTY = isatty(STDERR_FILENO);

    /* initialise xhash */

    if (XhashCreate(&h, nHashSize / (fGammon ? 128 : 64))) {
        fprintf(stderr, _("Error creating xhash with %d elements\n"), nHashSize / (fGammon ? 128 : 64));
        exit(2);
    }

    XhashStatus(&h);

    /* write header */

    if (fHeader) {
        char sz[41];
        sprintf(sz, "gnubg-OS-%02d-15-%1d-%1d-0xxxxxxxxxxxxxxxxxxx\n", nOS, fGammon, fCompress);
        fputs(sz, output);
    }

    if (fCompress) {
        pfTmp = GetTemporaryFile(NULL, &tmpfile);
        if (pfTmp == NULL)
            exit(2);
    }


    /* loop trough bearoff positions */

    n = Combination(nOS + 15, nOS);
    npos = 0;


    for (i = 0; i < n; ++i) {

        if (i)
            BearOff(i, nOS, aus, fGammon, &h, pbc, fCompress, output, pfTmp);
        else {
            memset(aus, 0, 128);
            aus[0] = 0xFFFF;
            aus[32] = 0xFFFF;
        }
        if (!(i % 100) && fTTY)
            fprintf(stderr, "1:%d/%d        \r", i, n);

        WriteOS(aus, fCompress, fCompress ? pfTmp : output);
        if (fGammon)
            WriteOS(aus + 32, fCompress, fCompress ? pfTmp : output);

        XhashAdd(&h, i, aus, fGammon ? 128 : 64);

        if (fCompress)
            WriteIndex(&npos, aus, fGammon, output);

    }

    if (fCompress) {

        char ac[256];
        size_t n;
        /* write contents of pfTmp to output */

        rewind(pfTmp);

        while (!feof(pfTmp) && (n = fread(ac, 1, sizeof(ac), pfTmp))) {
            if (fwrite(ac, 1, n, output) != n) {
                fprintf(stderr, "failed writing to '%s'\n", tmpfile);
                exit(3);
            }
        }

        if (ferror(pfTmp))
            exit(3);

        fclose(pfTmp);

        g_unlink(tmpfile);

    }
    putc('\n', stderr);

    XhashStatus(&h);

    XhashDestroy(&h);

    return 0;

}


static void
NDBearoff(const int iPos, const unsigned int nPoints, float ar[4], xhash * ph, bearoffcontext * pbc)
{

    int d0, d1;
    movelist ml;
    TanBoard anBoard, anBoardTemp;
    int ii, j, k;
    unsigned int i;
#if !defined(G_DISABLE_ASSERT)
    int iBest;
    int iGammonBest;
#endif
    float rBest;
    float rMean;
    float rVarSum, rGammonVarSum;
    float *prj;
    float arj[4] = { 0.0, 0.0, 0.0, 0.0 };
    float arBest[4] = { 0.0, 0.0, 0.0, 0.0 };
    float arGammonBest[4] = { 0.0, 0.0, 0.0, 0.0 };
    float rGammonBest;

    for (i = 0; i < 4; ++i)
        ar[i] = 0.0f;

    if (!iPos)
        return;

    PositionFromBearoff(anBoard[1], iPos, nPoints, 15);

    for (i = nPoints; i < 25; i++)
        anBoard[1][i] = 0;

    /* 
     * look for position in existing bearoff file 
     */

    if (pbc) {
        for (ii = 24; ii >= 0 && !anBoard[1][ii]; --ii);

        if (ii < (int) pbc->nPoints) {
            unsigned int nPosID = PositionBearoff(anBoard[1],
                                                  pbc->nPoints, pbc->nChequers);
            BearoffDist(pbc, nPosID, NULL, NULL, ar, NULL, NULL);
            return;
        }

    }

    memset(anBoard[0], 0, 25 * sizeof(int));

    for (i = 0, k = 0; i < nPoints; ++i)
        k += anBoard[1][i];

    /* loop over rolls */

    rVarSum = rGammonVarSum = 0.0f;

    for (d0 = 1; d0 <= 6; ++d0)
        for (d1 = 1; d1 <= d0; ++d1) {

            GenerateMoves(&ml, (ConstTanBoard) anBoard, d0, d1, FALSE);

            rBest = 1e10;
            rGammonBest = 1e10;
#if !defined(G_DISABLE_ASSERT)
            iBest = -1;
            iGammonBest = -1;
#endif

            for (i = 0; i < ml.cMoves; ++i) {

                PositionFromKey(anBoardTemp, &ml.amMoves[i].key);

                j = PositionBearoff(anBoardTemp[1], nPoints, 15);

                if (!(prj = XhashLookup(ph, j))) {
                    prj = arj;
                    NDBearoff(j, nPoints, prj, ph, pbc);
                    XhashAdd(ph, j, prj, 16);
                }

                /* find best move to win */

                if (prj[0] < rBest) {
#if !defined(G_DISABLE_ASSERT)
                    iBest = j;
#endif
                    rBest = prj[0];
                    memcpy(arBest, prj, 4 * sizeof(float));
                }

                /* find best move to save gammon */

                if (prj[2] < rGammonBest) {
#if !defined(G_DISABLE_ASSERT)
                    iGammonBest = j;
#endif
                    rGammonBest = prj[2];
                    memcpy(arGammonBest, prj, 4 * sizeof(float));
                }

            }

            g_assert(iBest >= 0);
            g_assert(iGammonBest >= 0);

            rMean = 1.0f + arBest[0];

            ar[0] += (d0 == d1) ? rMean : 2.0f * rMean;

            rMean = arBest[1] * arBest[1] + rMean * rMean;

            rVarSum += (d0 == d1) ? rMean : 2.0f * rMean;

            if (k == 15) {

                rMean = 1.0f + arGammonBest[2];

                ar[2] += (d0 == d1) ? rMean : 2.0f * rMean;

                rMean = arGammonBest[3] * arGammonBest[3] + rMean * rMean;

                rGammonVarSum += (d0 == d1) ? rMean : 2.0f * rMean;

            }

        }

    ar[0] /= 36.0f;
    ar[1] = sqrtf(rVarSum / 36.0f - ar[0] * ar[0]);

    ar[2] /= 36.0f;
    ar[3] = sqrtf(rGammonVarSum / 36.0f - ar[2] * ar[2]);

}


static void
generate_nd(const int nPoints, const int nHashSize, const int fHeader, bearoffcontext * pbc, FILE * output)
{

    int n = Combination(nPoints + 15, nPoints);

    int i, j;
    char sz[41];
    float ar[4];
    int fTTY = isatty(STDERR_FILENO);

    xhash h;

    /* initialise xhash */


    if (XhashCreate(&h, nHashSize / (4 * sizeof(float)))) {
        fprintf(stderr, "Error creating cache\n");
        return;
    }

    XhashStatus(&h);

    if (fHeader) {
        sprintf(sz, "gnubg-OS-%02d-15-1-0-1xxxxxxxxxxxxxxxxxxx\n", nPoints);
        fputs(sz, output);
    }


    for (i = 0; i < n; ++i) {

        if (i)
            NDBearoff(i, nPoints, ar, &h, pbc);
        else
            ar[0] = ar[1] = ar[2] = ar[3] = 0.0f;

        for (j = 0; j < 4; ++j)
            WriteFloat(ar[j], output);

        XhashAdd(&h, i, ar, 16);
        if (!(i % 100) && fTTY)
            fprintf(stderr, "1:%d/%d        \r", i, n);

    }
    putc('\n', stderr);
    XhashStatus(&h);

    XhashDestroy(&h);

}


static short int
CubeEquity(const short int siND, const short int siDT, const short int siDP)
{

    if (siDT >= (siND / 2) && siDP >= siND) {
        /* it's a double */

        if (siDT >= (siDP / 2))
            /* double, pasi */
            return siDP;
        else
            /* double, take */
            return 2 * siDT;

    } else
        /* no double */

        return siND;

}

static int
CalcPosition(const int i, const int j, const int n)
{

    int k;

    if (i + j < n)
        k = (i + j) * (i + j + 1) / 2 + j;
    else
        k = n * n - CalcPosition(n - 1 - i, n - 1 - j, n - 1) - 1;

#if 0
    if (n == 6) {
        fprintf(stderr, "%2d ", k);
        if (j == n - 1)
            fprintf(stderr, "\n");
    }
#endif

    return k;

}



static void
TSLookup(const int nUs, const int nThem,
         const int UNUSED(nTSP), const int UNUSED(nTSC),
         short int arEquity[4], const int n, const int fCubeful, FILE * pfTmp)
{

    int iPos = CalcPosition(nUs, nThem, n);
    unsigned char ac[8];
    int i;
    unsigned short int us;

    /* seek to position */

    if (fseek(pfTmp, iPos * (fCubeful ? 8 : 2), SEEK_SET) < 0) {
        perror("temp file");
        exit(-1);
    }

    if (fread(ac, 1, 8, pfTmp) < 8) {
        if (errno)
            perror("temp file");
        else
            fprintf(stderr, "error reading temp file\n");
        exit(-1);
    }

    /* re-position at EOF */

    if (fseek(pfTmp, 0L, SEEK_END) < 0) {
        perror("temp file");
        exit(-1);
    }

    for (i = 0; i < (fCubeful ? 4 : 1); ++i) {
        us = (unsigned short) (ac[2 * i] | ac[2 * i + 1] << 8);
        arEquity[i] = us - 0x8000;
    }

    ++cLookup;

}


/*
 * Calculate exact equity for position.
 *
 * We store the equity in two bytes:
 * 0x0000 meaning equity=-1 and 0xFFFF meaning equity=+1.
 *
 */


static void
BearOff2(int nUs, int nThem,
         const int nTSP, const int nTSC,
         short int asiEquity[4], const int n, const int fCubeful, xhash * ph, bearoffcontext * pbc, FILE * pfTmp)
{

    int j, anRoll[2];
    unsigned int i;
    TanBoard anBoard, anBoardTemp;
    movelist ml;
    int aiBest[4];
    int asiBest[4];
    int aiTotal[4];
    short int k;
    short int *psij;
    short int asij[4];
    const short int EQUITY_P1 = 0x7FFF;
    const short int EQUITY_M1 = ~EQUITY_P1;

    if (!nUs) {

        /* we have won */
        asiEquity[0] = asiEquity[1] = asiEquity[2] = asiEquity[3] = EQUITY_P1;
        return;

    }

    if (!nThem) {

        /* we have lost */
        asiEquity[0] = asiEquity[1] = asiEquity[2] = asiEquity[3] = EQUITY_M1;
        return;

    }

    PositionFromBearoff(anBoard[0], nThem, nTSP, nTSC);
    PositionFromBearoff(anBoard[1], nUs, nTSP, nTSC);

    for (i = nTSP; i < 25; i++)
        anBoard[1][i] = anBoard[0][i] = 0;

    /* look for position in bearoff file */

    if (pbc && isBearoff(pbc, (ConstTanBoard) anBoard)) {
        unsigned short int nUsL = (unsigned short) PositionBearoff(anBoard[1], pbc->nPoints, pbc->nChequers);
        unsigned short int nThemL = (unsigned short) PositionBearoff(anBoard[0], pbc->nPoints, pbc->nChequers);
        int nL = Combination(pbc->nPoints + pbc->nChequers, pbc->nPoints);
        unsigned int iPos = nUsL * nL + nThemL;
        unsigned short int aus[4];

        BearoffCubeful(pbc, iPos, NULL, aus);

        for (i = 0; i < 4; ++i)
            asiEquity[i] = aus[i] - 0x8000;

        return;
    }

    aiTotal[0] = aiTotal[1] = aiTotal[2] = aiTotal[3] = 0;

    for (anRoll[0] = 1; anRoll[0] <= 6; anRoll[0]++)
        for (anRoll[1] = 1; anRoll[1] <= anRoll[0]; anRoll[1]++) {
            GenerateMoves(&ml, (ConstTanBoard) anBoard, anRoll[0], anRoll[1], FALSE);

            asiBest[0] = asiBest[1] = asiBest[2] = asiBest[3] = -0xFFFF;
            aiBest[0] = aiBest[1] = aiBest[2] = aiBest[3] = -1;

            for (i = 0; i < ml.cMoves; i++) {
                PositionFromKey(anBoardTemp, &ml.amMoves[i].key);

                j = PositionBearoff(anBoardTemp[1], nTSP, nTSC);

                g_assert(j >= 0);
                g_assert(j < nUs);

                if (!nThem) {
                    asij[0] = asij[1] = asij[2] = asij[3] = EQUITY_P1;
                } else if (!j) {
                    asij[0] = asij[1] = asij[2] = asij[3] = EQUITY_M1;
                }
                if (!(psij = XhashLookup(ph, n * nThem + j))) {
                    /* lookup in file */
                    psij = asij;
                    TSLookup(nThem, j, nTSP, nTSC, psij, n, fCubeful, pfTmp);
                    XhashAdd(ph, n * nThem + j, psij, fCubeful ? 8 : 2);
                }

                /* cubeless */

                if (psij[0] < -asiBest[0]) {
                    aiBest[0] = j;
                    asiBest[0] = ~psij[0];
                }

                if (fCubeful) {

                    /* I own cube:
                     * from opponent's view he doesn't own cube */

                    if (psij[3] < -asiBest[1]) {
                        aiBest[1] = j;
                        asiBest[1] = ~psij[3];
                    }

                    /* Centered cube (so centered for opponent too) */

                    k = CubeEquity(psij[2], psij[3], EQUITY_P1);
                    if (~k > asiBest[2]) {
                        aiBest[2] = j;
                        asiBest[2] = ~k;
                    }

                    /* Opponent owns cube:
                     * from opponent's view he owns cube */

                    k = CubeEquity(psij[1], psij[3], EQUITY_P1);
                    if (~k > asiBest[3]) {
                        aiBest[3] = j;
                        asiBest[3] = ~k;
                    }

                }

            }

            g_assert(aiBest[0] >= 0);
            g_assert(!fCubeful || aiBest[1] >= 0);
            g_assert(!fCubeful || aiBest[2] >= 0);
            g_assert(!fCubeful || aiBest[3] >= 0);

            if (anRoll[0] == anRoll[1])
                for (k = 0; k < (fCubeful ? 4 : 1); ++k)
                    aiTotal[k] += asiBest[k];
            else
                for (k = 0; k < (fCubeful ? 4 : 1); ++k)
                    aiTotal[k] += 2 * asiBest[k];

        }

    /* final equities */
    for (k = 0; k < (fCubeful ? 4 : 1); ++k)
        asiEquity[k] = (short) (aiTotal[k] / 36);

}



static void
WriteEquity(FILE * pf, const short int si)
{

    unsigned short int us = si + 0x8000;

    putc(us & 0xFF, pf);
    putc((us >> 8) & 0xFF, pf);

}

static void
generate_ts(const int nTSP, const int nTSC,
            const int fHeader, const int fCubeful, const int nHashSize, bearoffcontext * pbc, FILE * output)
{

    int i, j, k;
    int iPos;
    int n;
    short int asiEquity[4];
    xhash h;
    FILE *pfTmp;
    unsigned char ac[8];
    char *tmpfile;
    int fTTY = isatty(STDERR_FILENO);

    pfTmp = GetTemporaryFile(NULL, &tmpfile);
    if (pfTmp == NULL)
        exit(2);

    /* initialise xhash */

    if (XhashCreate(&h, nHashSize / (fCubeful ? 8 : 2))) {
        fprintf(stderr, _("Error creating xhash with %d elements\n"), nHashSize / (fCubeful ? 8 : 2));
        exit(2);
    }

    XhashStatus(&h);

    /* write header information */

    if (fHeader) {
        char sz[41];
        sprintf(sz, "gnubg-TS-%02d-%02d-%1dxxxxxxxxxxxxxxxxxxxxxxx\n", nTSP, nTSC, fCubeful);
        fputs(sz, output);
    }


    /* generate bearoff database */

    n = Combination(nTSP + nTSC, nTSC);
    iPos = 0;


    /* positions above diagonal */

    for (i = 0; i < n; i++) {
        for (j = 0; j <= i; j++, ++iPos) {

            BearOff2(i - j, j, nTSP, nTSC, asiEquity, n, fCubeful, &h, pbc, pfTmp);

            for (k = 0; k < (fCubeful ? 4 : 1); ++k)
                WriteEquity(pfTmp, asiEquity[k]);

            XhashAdd(&h, (i - j) * n + j, asiEquity, fCubeful ? 8 : 2);

        }
        if (fTTY)
            fprintf(stderr, "%d/%d     \r", iPos, n * n);
    }

    /* positions below diagonal */

    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++, ++iPos) {

            BearOff2(i + n - j, j, nTSP, nTSC, asiEquity, n, fCubeful, &h, pbc, pfTmp);

            for (k = 0; k < (fCubeful ? 4 : 1); ++k)
                WriteEquity(pfTmp, asiEquity[k]);

            XhashAdd(&h, (i + n - j) * n + j, asiEquity, fCubeful ? 8 : 2);

        }
        if (fTTY)
            fprintf(stderr, "%d/%d     \r", iPos, n * n);
    }

    putc('\n', stderr);
    XhashStatus(&h);

    XhashDestroy(&h);

    /* sort file from ordering:
     * 
     * 136       123
     * 258  to   456 
     * 479       789 
     * 
     */

    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            unsigned int count = fCubeful ? 8 : 2;

            k = CalcPosition(i, j, n);

            fseek(pfTmp, count * k, SEEK_SET);
            if (fread(ac, 1, count, pfTmp) != count || fwrite(ac, 1, count, output) != count) {
                fprintf(stderr, "failed to read from or write to database file\n");
                exit(3);
            }
        }

    }

    fclose(pfTmp);

    g_unlink(tmpfile);

}




static void
version(void)
{
    printf("makebearoff $Revision: 1.96 $\n");
}


extern int
main(int argc, char **argv)
{
    /* static storage for options to keep picky comipler happy */
    static int nOS = 0;
    static int fHeader = TRUE;
    static int fCompress = TRUE;
    static int fGammon = TRUE;
    static int nHashSize = 100000000;
    static int fCubeful = TRUE;
    static char *szOldBearoff = NULL;
    static int fND = FALSE;
    static char *szOutput = NULL;
    static char *szTwoSided = NULL;
    static int show_version = 0;

    glib_ext_init();
    MT_InitThreads();
    bearoffcontext *pbc = NULL;
    FILE *output;
    double r;
    int nTSP = 0, nTSC = 0;

    GOptionEntry ao[] = {
        {"two-sided", 't', 0, G_OPTION_ARG_STRING, &szTwoSided,
         "Number of points (P) and number of chequers (C) for two-sided database", "PxC"},
        {"one-sided", 'o', 0, G_OPTION_ARG_INT, &nOS,
         "Number of points (P) for one-sided database", "P"},
        {"xhash-size", 's', 0, G_OPTION_ARG_INT, &nHashSize,
         "Use cache of size N bytes", "N"},
        {"old-bearoff", 'O', 0, G_OPTION_ARG_STRING, &szOldBearoff,
         "Reuse already generated bearoff database \"filename\"", "filename"},
        {"no-header", 'H', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &fHeader,
         "Do not write header", NULL},
        {"no-cubeful", 'C', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &fCubeful,
         "Do not calculate cubeful equities for two-sided databases", NULL},
        {"no-compress", 'c', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &fCompress,
         "Do not use compression scheme for one-sided databases", NULL},
        {"no-gammon", 'g', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &fGammon,
         "Do not include gammon distribution for one-sided databases", NULL},
        {"normal-dist", 'n', 0, G_OPTION_ARG_NONE, &fND,
         "Approximate one-sided bearoff database with normal distributions", NULL},
        {"version", 'v', 0, G_OPTION_ARG_NONE, &show_version,
         "Prints version and exits", NULL},
        {"outfile", 'f', 0, G_OPTION_ARG_STRING, &szOutput,
         "Required output filename", "filename"},
        {NULL, 0, 0, 0, NULL, NULL, NULL}
    };

    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, ao, PACKAGE);
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);
    if (error) {
        g_printerr("%s\n", error->message);
        exit(EXIT_FAILURE);
    }


    if (szTwoSided)
        sscanf(szTwoSided, "%2dx%2d", &nTSP, &nTSC);

    if (show_version) {
        version();
        exit(0);
    }

    if (!szOutput) {
        g_printerr("Required argument -f missing\n");
        exit(EXIT_FAILURE);
    }

    if (!(output = g_fopen(szOutput, "w+b"))) {
        perror(szOutput);
        return EXIT_FAILURE;
    }

    /* one sided database */

    if (nOS) {

        if (nOS > 18) {
            fprintf(stderr, _("Size of one-sided bearoff database must be between " "0 and 18\n"));
            exit(2);
        }
        fprintf(stderr, "%-37s\n", _("One-sided database"));
        fprintf(stderr, "%-37s: %12d\n", _("Number of points"), nOS);
        fprintf(stderr, "%-37s: %12d\n", _("Number of chequers"), 15);
        fprintf(stderr, "%-37s: %12u\n", _("Number of positions"), Combination(nOS + 15, nOS));
        fprintf(stderr, "%-37s: %12s\n", _("Approximate by normal distribution"), fND ? _("yes") : _("no"));
        fprintf(stderr, "%-37s: %12s\n", _("Include gammon distributions"), fGammon ? _("yes") : _("no"));
        fprintf(stderr, "%-37s: %12s\n", _("Use compression scheme"), fCompress ? _("yes") : _("no"));
        fprintf(stderr, "%-37s: %12s\n", _("Write header"), fHeader ? _("yes") : _("no"));
        fprintf(stderr, "%-37s: %12d\n", _("Size of cache"), nHashSize);
        fprintf(stderr, "%-37s: %12s %s\n", _("Reuse old bearoff database"), szOldBearoff ? _("yes") : _("no"),
                szOldBearoff ? szOldBearoff : "");

        if (fND) {
            r = Combination(nOS + 15, nOS) * 16.0;
            fprintf(stderr, "%-37s: %.0f (%.1f MB)\n", _("Size of database"), r, r / 1048576.0);
        } else {
            r = Combination(nOS + 15, nOS) * (fGammon ? 128.0f : 64.0f);
            fprintf(stderr, "%-37s: %.0f (%.1f MB)\n", _("Size of database (uncompressed)"), r, r / 1048576.0);
            if (fCompress) {
                r = Combination(nOS + 15, nOS) * (fGammon ? 32.0f : 16.0f);
                fprintf(stderr, "%-37s: %.0f (%.1f MB)\n", _("Estimated size of compressed db"), r, r / 1048576.0);
            }
        }

        if (szOldBearoff && !(pbc = BearoffInit(szOldBearoff, BO_NONE, NULL))) {
            fprintf(stderr, _("Error initialising old bearoff database!\n"));
            exit(2);
        }

        /* bearoff database must be of the same kind as the request one-sided
         * database */

        if (pbc && (pbc->bt != BEAROFF_ONESIDED || pbc->fND != fND || pbc->fGammon < fGammon)) {
            fprintf(stderr, _("The old database is not of the same kind as the" " requested database\n"));
            exit(2);
        }

        if (fND) {
            generate_nd(nOS, nHashSize, fHeader, pbc, output);
        } else {
            generate_os(nOS, fHeader, fCompress, fGammon, nHashSize, pbc, output);
        }

        BearoffClose(pbc);

        fprintf(stderr, "Number of re-reads while generating: %ld\n", cLookup);
    }

    /*
     * Two-sided database
     */

    if (nTSC && nTSP) {

        int n = Combination(nTSP + nTSC, nTSC);

        r = n;
        r = r * r * (fCubeful ? 8.0 : 2.0);
        fprintf(stderr, "%-37s\n", _("Two-sided database:\n"));
        fprintf(stderr, "%-37s: %12d\n", _("Number of points"), nTSP);
        fprintf(stderr, "%-37s: %12d\n", _("Number of chequers"), nTSC);
        fprintf(stderr, "%-37s: %12s\n", _("Calculate equities"),
                fCubeful ? _("cubeless and cubeful") : _("cubeless only"));
        fprintf(stderr, "%-37s: %12s\n", _("Write header"), fHeader ? _("yes") : _("no"));
        fprintf(stderr, "%-37s: %12d\n", _("Number of one-sided positions"), n);
        fprintf(stderr, "%-37s: %12d\n", _("Total number of positions"), n * n);
        fprintf(stderr, "%-37s: %.0f bytes (%.1f MB)\n", _("Size of resulting file"), r, r / 1048576.0);
        fprintf(stderr, "%-37s: %12d\n", _("Size of xhash"), nHashSize);
        fprintf(stderr, "%-37s: %12s %s\n", _("Reuse old bearoff database"), szOldBearoff ? _("yes") : _("no"),
                szOldBearoff ? szOldBearoff : "");
        /* initialise old bearoff database */
        if (szOldBearoff && !(pbc = BearoffInit(szOldBearoff, BO_NONE, NULL))) {
            fprintf(stderr, _("Error initialising old bearoff database!\n"));
            exit(2);
        }

        /* bearoff database must be of the same kind as the request one-sided
         * database */

        if (pbc && (pbc->bt != BEAROFF_TWOSIDED || pbc->fCubeful != fCubeful)) {
            fprintf(stderr, _("The old database is not of the same kind as the" " requested database\n"));
            exit(2);
        }

        generate_ts(nTSP, nTSC, fHeader, fCubeful, nHashSize, pbc, output);

        /* close old bearoff database */

        BearoffClose(pbc);

        fprintf(stderr, "Number of re-reads while generating: %ld\n", cLookup);

    }

    fclose(output);
    return 0;

}
