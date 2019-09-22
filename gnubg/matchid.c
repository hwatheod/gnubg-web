/*
 * matchid.c
 *
 * by Jørn Thyssen <jthyssen@dk.ibm.com>, 2002
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
 * $Id: matchid.c,v 1.26 2013/11/20 23:04:14 plm Exp $
 */

#include "config.h"

#include "backgammon.h"
#include "positionid.h"
#include "matchequity.h"
#include "matchid.h"
#include <string.h>

/*
 * Calculate log2 of Cube value.
 *
 * Input:
 *   n: cube value
 *
 * Returns:
 *   log(n)/log(2)
 *
 */

extern int
LogCube(int n)
{
    int i = 0;

    while (n >>= 1)
        i++;

    return i;
}


static void
SetBit(unsigned char *pc, unsigned int bitPos, int test, unsigned int iBit)
{

    const unsigned int k = bitPos / 8;
    const unsigned char rbit = (unsigned char) (0x1 << (bitPos % 8));

    unsigned char c;

    if (test & (0x1 << iBit))
        c = rbit;
    else
        c = 0;
    pc[k] = (pc[k] & (0xFF ^ rbit)) | c;

}

static void
SetBits(unsigned char *pc, unsigned int bitPos, unsigned int nBits, int iContent)
{

    unsigned int i;

    /* FIXME: rewrite SetBit, SetBits to be faster */


    for (i = 0; i < nBits; i++) {

        SetBit(pc, bitPos + i, iContent, i);

    }

}


static void
GetBits(const unsigned char *pc, const unsigned int bitPos, const unsigned int nBits, int *piContent)
{


    unsigned int i, j;
    unsigned int k, r;

    unsigned char c[2];

    /* FIXME: rewrite GetBits to be faster */

    c[0] = 0;
    c[1] = 0;

    for (i = 0, j = bitPos; i < nBits; i++, j++) {

        k = j / 8;
        r = j % 8;

        SetBit(c, i, pc[k], r);

    }

    *piContent = c[0] | (c[1] << 8);
}


extern char
*
MatchIDFromKey(unsigned char auchKey[9])
{

    unsigned char *puch = auchKey;
    static char szID[L_MATCHID + 1];
    char *pch = szID;
    static char aszBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i;

    for (i = 0; i < 3; i++) {
        *pch++ = aszBase64[puch[0] >> 2];
        *pch++ = aszBase64[((puch[0] & 0x03) << 4) | (puch[1] >> 4)];
        *pch++ = aszBase64[((puch[1] & 0x0F) << 2) | (puch[2] >> 6)];
        *pch++ = aszBase64[puch[2] & 0x3F];

        puch += 3;
    }

    *pch = 0;

    return szID;
}


extern char *
MatchID(const unsigned int anDice[2],
        const int fTurn,
        const int fResigned,
        const int fDoubled,
        const int fMove,
        const int fCubeOwner, const int fCrawford, const int nMatchTo, const int anScore[2], const int nCube,
        const int fJacoby, const gamestate gs)
{

    unsigned char auchKey[9];

    memset(auchKey, 0, 9);

    SetBits(auchKey, 0, 4, LogCube(nCube));
    SetBits(auchKey, 4, 2, fCubeOwner & 0x3);
    SetBits(auchKey, 6, 1, fMove);
    SetBits(auchKey, 7, 1, fCrawford);
    SetBits(auchKey, 8, 3, (int) gs);
    SetBits(auchKey, 11, 1, fTurn);
    SetBits(auchKey, 12, 1, fDoubled);
    SetBits(auchKey, 13, 2, fResigned);
    if (anDice[0] >= anDice[1]) {
        SetBits(auchKey, 15, 3, anDice[0] & 0x7);
        SetBits(auchKey, 18, 3, anDice[1] & 0x7);
    } else {
        SetBits(auchKey, 15, 3, anDice[1] & 0x7);
        SetBits(auchKey, 18, 3, anDice[0] & 0x7);
    }
    SetBits(auchKey, 21, 15, nMatchTo & 0x7FFF);
    SetBits(auchKey, 36, 15, anScore[0] & 0x7FFF);
    SetBits(auchKey, 51, 15, anScore[1] & 0x7FFF);
    SetBits(auchKey, 66, 1, (!fJacoby));

    return MatchIDFromKey(auchKey);


}

extern int
MatchFromKey(int anDice[2],
             int *pfTurn,
             int *pfResigned,
             int *pfDoubled,
             int *pfMove, int *pfCubeOwner, int *pfCrawford, int *pnMatchTo, int anScore[2], int *pnCube,
             int *pfJacoby, gamestate * pgs, const unsigned char *auchKey)
{
    int temp;
    GetBits(auchKey, 0, 4, pnCube);
    *pnCube = 0x1 << *pnCube;

    GetBits(auchKey, 4, 2, pfCubeOwner);
    if (*pfCubeOwner && *pfCubeOwner != 1)
        *pfCubeOwner = -1;

    GetBits(auchKey, 6, 1, pfMove);
    GetBits(auchKey, 7, 1, pfCrawford);
    GetBits(auchKey, 8, 3, &temp);
    *pgs = (gamestate) temp;
    GetBits(auchKey, 11, 1, pfTurn);
    GetBits(auchKey, 12, 1, pfDoubled);
    GetBits(auchKey, 13, 2, pfResigned);
    GetBits(auchKey, 15, 3, &anDice[0]);
    GetBits(auchKey, 18, 3, &anDice[1]);
    GetBits(auchKey, 21, 15, pnMatchTo);
    GetBits(auchKey, 36, 15, &anScore[0]);
    GetBits(auchKey, 51, 15, &anScore[1]);
    GetBits(auchKey, 66, 1, pfJacoby);
    *pfJacoby = !(*pfJacoby);

    /* FIXME: implement a consistency check */

    if (anDice[0] < 0 || anDice[0] > 6)
        return -1;
    if (anDice[1] < 0 || anDice[1] > 6)
        return -1;
    if (*pnMatchTo < 0 || *pnMatchTo > MAXSCORE)
        return -1;

    if (*pnMatchTo) {
        if (anScore[0] < 0 || anScore[0] > *pnMatchTo)
            return -1;
        if (anScore[1] < 0 || anScore[1] > *pnMatchTo)
            return -1;
    } else {
        /* money game */
        if (*pfCrawford)
            /* no Crawford game in money play */
            return -1;
    }

    return 0;

}


extern int
MatchFromID(unsigned int anDice[2],
            int *pfTurn,
            int *pfResigned,
            int *pfDoubled, int *pfMove, int *pfCubeOwner, int *pfCrawford, int *pnMatchTo, int anScore[2], int *pnCube,
            int *pfJacoby, gamestate * pgs, const char *szMatchID)
{

    unsigned char auchKey[9];
    unsigned char *puch = auchKey;
    unsigned char ach[L_MATCHID + 1];
    unsigned char *pch = ach;
    int i;

    memset(ach, 0, sizeof(ach));
    /* decode base64 into key */
    for (i = 0; i < L_MATCHID && szMatchID[i]; i++)
        pch[i] = Base64((unsigned char) szMatchID[i]);

    for (i = 0; i < 3; i++) {
        *puch++ = (unsigned char) (pch[0] << 2) | (pch[1] >> 4);
        *puch++ = (unsigned char) (pch[1] << 4) | (pch[2] >> 2);
        *puch++ = (unsigned char) (pch[2] << 6) | pch[3];

        pch += 4;
    }

    /* get matchstate info from the key */

    return MatchFromKey((int *) anDice, pfTurn, pfResigned, pfDoubled, pfMove, pfCubeOwner, pfCrawford, pnMatchTo,
                        (int *) anScore, pnCube, pfJacoby, pgs, auchKey);
}

/*
 * Generate match ID from matchstate
 *
 * Input:
 *   pms: match state
 *
 */

extern char *
MatchIDFromMatchState(const matchstate * pms)
{

    return MatchID(pms->anDice,
                   pms->fTurn,
                   pms->fResigned,
                   pms->fDoubled, pms->fMove, pms->fCubeOwner, pms->fCrawford, pms->nMatchTo, pms->anScore, pms->nCube,
                   pms->fJacoby, pms->gs);

}
