/*
 * positionid.c
 *
 * by Gary Wong <gary@cs.arizona.edu>, 1998-1999.
 *
 * An implementation of the position "external" key/IDs at:
 *
 *   http://www.gnu.org/manual/gnubg/html_node/A-technical-description-of-the-Position-ID.html
 *
 * Please see that page for more information.
 *
 * For internal use, we use another "key", much faster to encode or decode
 * that is essentially the raw board squeezed down to 4 bits/point.
 *
 * This library also calculates bearoff IDs, which are enumerations of the
 *
 *    c+6
 *       C
 *        6
 *
 * combinations of (up to c) chequers among 6 points.
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
 * $Id: positionid.c,v 1.54 2014/10/07 07:14:28 plm Exp $
 */

#include "config.h"
#include <glib.h>
#include <errno.h>
#include <string.h>
#include "positionid.h"

extern void
PositionKey(const TanBoard anBoard, positionkey * pkey)
{
    unsigned int i, j;
    unsigned int *anpBoard = pkey->data;

    for (i = 0, j = 0; i < 3; i++, j += 8) {
        anpBoard[i] = anBoard[1][j] + (anBoard[1][j + 1] << 4)
            + (anBoard[1][j + 2] << 8) + (anBoard[1][j + 3] << 12)
            + (anBoard[1][j + 4] << 16) + (anBoard[1][j + 5] << 20)
            + (anBoard[1][j + 6] << 24) + (anBoard[1][j + 7] << 28);
        anpBoard[i + 3] = anBoard[0][j] + (anBoard[0][j + 1] << 4)
            + (anBoard[0][j + 2] << 8) + (anBoard[0][j + 3] << 12)
            + (anBoard[0][j + 4] << 16) + (anBoard[0][j + 5] << 20)
            + (anBoard[0][j + 6] << 24) + (anBoard[0][j + 7] << 28);
    }
    anpBoard[6] = anBoard[0][24] + (anBoard[1][24] << 4);
}

extern void
PositionFromKey(TanBoard anBoard, const positionkey * pkey)
{
    unsigned int i, j;
    unsigned int const *anpBoard = pkey->data;

    for (i = 0, j = 0; i < 3; i++, j += 8) {
        anBoard[1][j] = anpBoard[i] & 0x0f;
        anBoard[1][j + 1] = (anpBoard[i] >> 4) & 0x0f;
        anBoard[1][j + 2] = (anpBoard[i] >> 8) & 0x0f;
        anBoard[1][j + 3] = (anpBoard[i] >> 12) & 0x0f;
        anBoard[1][j + 4] = (anpBoard[i] >> 16) & 0x0f;
        anBoard[1][j + 5] = (anpBoard[i] >> 20) & 0x0f;
        anBoard[1][j + 6] = (anpBoard[i] >> 24) & 0x0f;
        anBoard[1][j + 7] = (anpBoard[i] >> 28) & 0x0f;

        anBoard[0][j] = anpBoard[i + 3] & 0x0f;
        anBoard[0][j + 1] = (anpBoard[i + 3] >> 4) & 0x0f;
        anBoard[0][j + 2] = (anpBoard[i + 3] >> 8) & 0x0f;
        anBoard[0][j + 3] = (anpBoard[i + 3] >> 12) & 0x0f;
        anBoard[0][j + 4] = (anpBoard[i + 3] >> 16) & 0x0f;
        anBoard[0][j + 5] = (anpBoard[i + 3] >> 20) & 0x0f;
        anBoard[0][j + 6] = (anpBoard[i + 3] >> 24) & 0x0f;
        anBoard[0][j + 7] = (anpBoard[i + 3] >> 28) & 0x0f;
    }
    anBoard[0][24] = anpBoard[6] & 0x0f;
    anBoard[1][24] = (anpBoard[6] >> 4) & 0x0f;
}

/* In evaluations, the function above is often followed by swapping
 * the board. This is expensive (SwapSides is about as costly as
 * PositionFromKey itself).
 * Provide one that fills the board already swapped. */

extern void
PositionFromKeySwapped(TanBoard anBoard, const positionkey * pkey)
{
    unsigned int i, j;
    unsigned int const *anpBoard = pkey->data;

    for (i = 0, j = 0; i < 3; i++, j += 8) {
        anBoard[0][j] = anpBoard[i] & 0x0f;
        anBoard[0][j + 1] = (anpBoard[i] >> 4) & 0x0f;
        anBoard[0][j + 2] = (anpBoard[i] >> 8) & 0x0f;
        anBoard[0][j + 3] = (anpBoard[i] >> 12) & 0x0f;
        anBoard[0][j + 4] = (anpBoard[i] >> 16) & 0x0f;
        anBoard[0][j + 5] = (anpBoard[i] >> 20) & 0x0f;
        anBoard[0][j + 6] = (anpBoard[i] >> 24) & 0x0f;
        anBoard[0][j + 7] = (anpBoard[i] >> 28) & 0x0f;

        anBoard[1][j] = anpBoard[i + 3] & 0x0f;
        anBoard[1][j + 1] = (anpBoard[i + 3] >> 4) & 0x0f;
        anBoard[1][j + 2] = (anpBoard[i + 3] >> 8) & 0x0f;
        anBoard[1][j + 3] = (anpBoard[i + 3] >> 12) & 0x0f;
        anBoard[1][j + 4] = (anpBoard[i + 3] >> 16) & 0x0f;
        anBoard[1][j + 5] = (anpBoard[i + 3] >> 20) & 0x0f;
        anBoard[1][j + 6] = (anpBoard[i + 3] >> 24) & 0x0f;
        anBoard[1][j + 7] = (anpBoard[i + 3] >> 28) & 0x0f;
    }
    anBoard[1][24] = anpBoard[6] & 0x0f;
    anBoard[0][24] = (anpBoard[6] >> 4) & 0x0f;
}

static inline void
addBits(unsigned char auchKey[10], unsigned int bitPos, unsigned int nBits)
{
    unsigned int k = bitPos / 8;
    unsigned int r = (bitPos & 0x7);
    unsigned int b = (((unsigned int) 0x1 << nBits) - 1) << r;

    auchKey[k] |= (unsigned char) b;

    if (k < 8) {
        auchKey[k + 1] |= (unsigned char) (b >> 8);
        auchKey[k + 2] |= (unsigned char) (b >> 16);
    } else if (k == 8) {
        auchKey[k + 1] |= (unsigned char) (b >> 8);
    }
}

extern void
oldPositionKey(const TanBoard anBoard, oldpositionkey * pkey)
{
    unsigned int i, iBit = 0;
    const unsigned int *j;

    memset(pkey, 0, sizeof(oldpositionkey));

    for (i = 0; i < 2; ++i) {
        const unsigned int *const b = anBoard[i];
        for (j = b; j < b + 25; ++j) {
            const unsigned int nc = *j;

            if (nc) {
                addBits(pkey->auch, iBit, nc);
                iBit += nc + 1;
            } else {
                ++iBit;
            }
        }
    }
}

extern void
oldPositionFromKey(TanBoard anBoard, const oldpositionkey * pkey)
{
    int i = 0, j = 0, k;
    const unsigned char *a;

    memset(anBoard[0], 0, sizeof(anBoard[0]));
    memset(anBoard[1], 0, sizeof(anBoard[1]));

    for (a = pkey->auch; a < pkey->auch + 10; ++a) {
        unsigned char cur = *a;

        for (k = 0; k < 8; ++k) {
            if ((cur & 0x1)) {
                if (i >= 2 || j >= 25) {        /* Error, so return - will probably show error message */
                    return;
                }
                ++anBoard[i][j];
            } else {
                if (++j == 25) {
                    ++i;
                    j = 0;
                }
            }
            cur >>= 1;
        }
    }
}


static char *
oldPositionIDFromKey(const oldpositionkey * pkey)
{

    unsigned char const *puch = pkey->auch;
    static char szID[L_POSITIONID + 1];
    char *pch = szID;
    static char aszBase64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i;

    for (i = 0; i < 3; i++) {
        *pch++ = aszBase64[puch[0] >> 2];
        *pch++ = aszBase64[((puch[0] & 0x03) << 4) | (puch[1] >> 4)];
        *pch++ = aszBase64[((puch[1] & 0x0F) << 2) | (puch[2] >> 6)];
        *pch++ = aszBase64[puch[2] & 0x3F];

        puch += 3;
    }

    *pch++ = aszBase64[*puch >> 2];
    *pch++ = aszBase64[(*puch & 0x03) << 4];

    *pch = 0;

    return szID;
}

extern char *
PositionIDFromKey(const positionkey * pkey)
{

    TanBoard anBoard;
    oldpositionkey okey;

    PositionFromKey(anBoard, pkey);
    oldPositionKey((ConstTanBoard) anBoard, &okey);

    return oldPositionIDFromKey(&okey);
}

extern char *
PositionID(const TanBoard anBoard)
{

    oldpositionkey key;

    oldPositionKey(anBoard, &key);

    return oldPositionIDFromKey(&key);
}

extern int
PositionFromXG(TanBoard anBoard, const char *pos)
{
    int i;

    for (i = 0; i < 26; i++) {
        int p0, p1;

        if (i == 0) {
            p0 = 24;
            p1 = -1;
        } else if (i == 25) {
            p0 = -1;
            p1 = 24;
        } else {
            p0 = 24 - i;
            p1 = i - 1;
        }

        if (pos[i] >= 'A' && pos[i] <= 'P') {
            if (p0 > -1)
                anBoard[0][p0] = 0;
            anBoard[1][p1] = pos[i] - 'A' + 1;
        } else if (pos[i] >= 'a' && pos[i] <= 'p') {
            anBoard[0][p0] = pos[i] - 'a' + 1;
            if (p1 > -1)
                anBoard[1][p1] = 0;
        } else if (pos[i] == '-') {
            if (p0 > -1)
                anBoard[0][p0] = 0;
            if (p1 > -1)
                anBoard[1][p1] = 0;
        } else {
            return 1;
        }
    }
    return 0;
}

extern int
CheckPosition(const TanBoard anBoard)
{
    unsigned int ac[2], i;

    /* Check for a player with over 15 chequers */
    for (i = ac[0] = ac[1] = 0; i < 25; i++)
        if ((ac[0] += anBoard[0][i]) > 15 || (ac[1] += anBoard[1][i]) > 15) {
            errno = EINVAL;
            return 0;
        }

    /* Check for both players having chequers on the same point */
    for (i = 0; i < 24; i++)
        if (anBoard[0][i] && anBoard[1][23 - i]) {
            errno = EINVAL;
            return 0;
        }

    /* Check for both players on the bar against closed boards */
    for (i = 0; i < 6; i++)
        if (anBoard[0][i] < 2 || anBoard[1][i] < 2)
            return 1;

    if (!anBoard[0][24] || !anBoard[1][24])
        return 1;

    errno = EINVAL;
    return 0;
}

extern void
ClosestLegalPosition(TanBoard anBoard)
{
    unsigned int i, j, ac[2];

    /* Limit each player to 15 chequers */
    for (i = 0; i < 2; i++) {
        ac[i] = 15;
        for (j = 0; j < 25; j++) {
            if (anBoard[i][j] <= ac[i])
                ac[i] -= anBoard[i][j];
            else {
                anBoard[i][j] = ac[i];
                ac[i] = 0;
            }
        }
    }

    /* Forbid both players having a chequer on the same point */
    for (i = 0; i < 24; i++)
        if (anBoard[0][i])
            anBoard[1][23 - i] = 0;

    /* If both players have closed boards, let at least one of them off
     * the bar */
    for (i = 0; i < 6; i++)
        if (anBoard[0][i] < 2 || anBoard[1][i] < 2)
            /* open board */
            return;

    if (anBoard[0][24])
        anBoard[1][24] = 0;
}

extern unsigned char
Base64(const unsigned char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ch - 'A';

    if (ch >= 'a' && ch <= 'z')
        return (ch - 'a') + 26;

    if (ch >= '0' && ch <= '9')
        return (ch - '0') + 52;

    if (ch == '+')
        return 62;

    if (ch == '/')
        return 63;

    return 255;
}

extern int
PositionFromID(TanBoard anBoard, const char *pchEnc)
{
    oldpositionkey key;
    unsigned char ach[L_POSITIONID + 1], *pch = ach, *puch = key.auch;
    int i;

    memset(ach, 0, L_POSITIONID + 1);

    for (i = 0; i < L_POSITIONID && pchEnc[i]; i++)
        pch[i] = Base64((unsigned char) pchEnc[i]);

    for (i = 0; i < 3; i++) {
        *puch++ = (unsigned char) (pch[0] << 2) | (pch[1] >> 4);
        *puch++ = (unsigned char) (pch[1] << 4) | (pch[2] >> 2);
        *puch++ = (unsigned char) (pch[2] << 6) | pch[3];

        pch += 4;
    }

    *puch = (unsigned char) (pch[0] << 2) | (pch[1] >> 4);

    oldPositionFromKey(anBoard, &key);

    return CheckPosition((ConstTanBoard) anBoard);
}

extern int
EqualBoards(const TanBoard anBoard0, const TanBoard anBoard1)
{

    int i;

    for (i = 0; i < 25; i++)
        if (anBoard0[0][i] != anBoard1[0][i] || anBoard0[1][i] != anBoard1[1][i])
            return 0;

    return 1;
}

#define MAX_N 40
#define MAX_R 25

static unsigned int anCombination[MAX_N][MAX_R], fCalculated = 0;

static void
InitCombination(void)
{
    unsigned int i, j;

    for (i = 0; i < MAX_N; i++)
        anCombination[i][0] = i + 1;

    for (j = 1; j < MAX_R; j++)
        anCombination[0][j] = 0;

    for (i = 1; i < MAX_N; i++)
        for (j = 1; j < MAX_R; j++)
            anCombination[i][j] = anCombination[i - 1][j - 1] + anCombination[i - 1][j];

    fCalculated = 1;
}

extern unsigned int
Combination(const unsigned int n, const unsigned int r)
{
    g_assert(n <= MAX_N && r <= MAX_R);

    if (!fCalculated)
        InitCombination();

    return anCombination[n - 1][r - 1];
}

static unsigned int
PositionF(unsigned int fBits, unsigned int n, unsigned int r)
{
    if (n == r)
        return 0;

    return (fBits & (1u << (n - 1))) ? Combination(n - 1, r) +
        PositionF(fBits, n - 1, r - 1) : PositionF(fBits, n - 1, r);
}

extern unsigned int
PositionBearoff(const unsigned int anBoard[], unsigned int nPoints, unsigned int nChequers)
{
    unsigned int i, fBits, j;

    for (j = nPoints - 1, i = 0; i < nPoints; i++)
        j += anBoard[i];

    fBits = 1u << j;

    for (i = 0; i < nPoints - 1; i++) {
        j -= anBoard[i] + 1;
        fBits |= (1u << j);
    }

    return PositionF(fBits, nChequers + nPoints, nPoints);
}

static unsigned int
PositionInv(unsigned int nID, unsigned int n, unsigned int r)
{
    unsigned int nC;

    if (!r)
        return 0;
    else if (n == r)
        return (1u << n) - 1;

    nC = Combination(n - 1, r);

    return (nID >= nC) ? (1u << (n - 1)) | PositionInv(nID - nC, n - 1, r - 1) : PositionInv(nID, n - 1, r);
}

extern void
PositionFromBearoff(unsigned int anBoard[], unsigned int usID, unsigned int nPoints, unsigned int nChequers)
{
    unsigned int fBits = PositionInv(usID, nChequers + nPoints, nPoints);
    unsigned int i, j;

    for (i = 0; i < nPoints; i++)
        anBoard[i] = 0;

    j = nPoints - 1;
    for (i = 0; i < (nChequers + nPoints); i++) {
        if (fBits & (1u << i)) {
            if (j == 0)
                break;
            j--;
        } else
            anBoard[j]++;
    }
}

extern unsigned short
PositionIndex(unsigned int g, const unsigned int anBoard[6])
{
    unsigned int i, fBits;
    unsigned int j = g - 1;

    for (i = 0; i < g; i++)
        j += anBoard[i];

    fBits = 1u << j;

    for (i = 0; i < g - 1; i++) {
        j -= anBoard[i] + 1;
        fBits |= (1u << j);
    }

    /* FIXME: 15 should be replaced by nChequers, but the function is
     * only called from bearoffgammon, so this should be fine. */
    return (unsigned short) PositionF(fBits, 15, g);
}
