/*
 * drawboard.c
 *
 * by Gary Wong <gtw@gnu.org>, 1999, 2000, 2001, 2002
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
 * $Id: drawboard.c,v 1.72 2015/02/01 21:48:42 plm Exp $
 */

#include "config.h"

#include <glib.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backgammon.h"
#include "drawboard.h"
#include "positionid.h"
#include "matchequity.h"

int fClockwise = FALSE;         /* Player 1 moves clockwise */

/*
 *  GNU Backgammon  Position ID: 0123456789ABCD 
 *                  Match ID   : 0123456789ABCD
 *  +13-14-15-16-17-18------19-20-21-22-23-24-+     O: gnubg (Cube: 2)
 *  |                  |   | O  O  O  O     O | OO  0 points
 *  |                  |   | O     O          | OO  Cube offered at 2
 *  |                  |   |       O          | O
 *  |                  |   |                  | O
 *  |                  |   |                  | O   
 * v|                  |BAR|                  |     Cube: 1 (7 point match)
 *  |                  |   |                  | X    
 *  |                  |   |                  | X
 *  |                  |   |                  | X
 *  |                  |   |       X  X  X  X | X   Rolled 11
 *  |                  |   |    X  X  X  X  X | XX  0 points
 *  +12-11-10--9--8--7-------6--5--4--3--2--1-+     X: Gary (Cube: 2)
 *
 */

static char *
DrawBoardStd(char *sz, const TanBoard anBoard, int fRoll, char *asz[], char *szMatchID, int nChequers)
{

    char *pch = sz, *pchIn;
    unsigned int x, y;
    unsigned int cOffO = nChequers, cOffX = nChequers;
    TanBoard an;
    static char achX[17] = "     X6789ABCDEF", achO[17] = "     O6789ABCDEF";

    for (x = 0; x < 25; x++) {
        cOffO -= anBoard[0][x];
        cOffX -= anBoard[1][x];
    }

    pch += sprintf(pch, " %-15s %s: ", _("GNU Backgammon"), _("Position ID"));

    if (fRoll)
        strcpy(pch, PositionID((ConstTanBoard) anBoard));
    else {
        for (x = 0; x < 25; x++) {
            an[0][x] = anBoard[1][x];
            an[1][x] = anBoard[0][x];
        }

        strcpy(pch, PositionID((ConstTanBoard) an));
    }

    pch += 14;
    *pch++ = '\n';

    /* match id */

    if (szMatchID && *szMatchID) {
        pch += sprintf(pch, "                 %s   : %s\n", _("Match ID"), szMatchID);
    }

    strcpy(pch, fRoll ? " +13-14-15-16-17-18------19-20-21-22-23-24-+     " :
           " +12-11-10--9--8--7-------6--5--4--3--2--1-+     ");
    pch += 49;

    if (asz[0])
        for (pchIn = asz[0]; *pchIn; pchIn++)
            *pch++ = *pchIn;

    *pch++ = '\n';

    for (y = 0; y < 4; y++) {
        *pch++ = ' ';
        *pch++ = '|';

        for (x = 12; x < 18; x++) {
            *pch++ = ' ';
            *pch++ = anBoard[1][x] > y ? 'X' : anBoard[0][23 - x] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = anBoard[0][24] > y ? 'O' : ' ';
        *pch++ = ' ';
        *pch++ = '|';

        for (; x < 24; x++) {
            *pch++ = ' ';
            *pch++ = anBoard[1][x] > y ? 'X' : anBoard[0][23 - x] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';

        for (x = 0; x < 3; x++)
            *pch++ = (cOffO > 5 * x + y) ? 'O' : ' ';

        *pch++ = ' ';

        if (y < 2 && asz[y + 1])
            for (pchIn = asz[y + 1]; *pchIn; pchIn++)
                *pch++ = *pchIn;
        *pch++ = '\n';
    }

    *pch++ = ' ';
    *pch++ = '|';

    for (x = 12; x < 18; x++) {
        *pch++ = ' ';
        *pch++ = anBoard[1][x] ? achX[anBoard[1][x]] : achO[anBoard[0][23 - x]];
        *pch++ = ' ';
    }

    *pch++ = '|';
    *pch++ = ' ';
    *pch++ = achO[anBoard[0][24]];
    *pch++ = ' ';
    *pch++ = '|';

    for (; x < 24; x++) {
        *pch++ = ' ';
        *pch++ = anBoard[1][x] ? achX[anBoard[1][x]] : achO[anBoard[0][23 - x]];
        *pch++ = ' ';
    }

    *pch++ = '|';

    *pch++ = ' ';

    for (x = 0; x < 3; x++)
        *pch++ = (cOffO > 5 * x + 4) ? 'O' : ' ';

    *pch++ = '\n';

    *pch++ = fRoll ? 'v' : '^';
    strcpy(pch, "|                  |BAR|                  |     ");
    pch = strchr(pch, 0);

    if (asz[3])
        for (pchIn = asz[3]; *pchIn; pchIn++)
            *pch++ = *pchIn;

    *pch++ = '\n';

    *pch++ = ' ';
    *pch++ = '|';

    for (x = 11; x > 5; x--) {
        *pch++ = ' ';
        *pch++ = anBoard[1][x] ? achX[anBoard[1][x]] : achO[anBoard[0][23 - x]];
        *pch++ = ' ';
    }

    *pch++ = '|';
    *pch++ = ' ';
    *pch++ = achX[anBoard[1][24]];
    *pch++ = ' ';
    *pch++ = '|';

    for (; x < UINT_MAX; x--) {
        *pch++ = ' ';
        *pch++ = anBoard[1][x] ? achX[anBoard[1][x]] : achO[anBoard[0][23 - x]];
        *pch++ = ' ';
    }

    *pch++ = '|';

    *pch++ = ' ';

    for (x = 0; x < 3; x++)
        *pch++ = (cOffX > 5 * x + 4) ? 'X' : ' ';

    *pch++ = '\n';

    for (y = 3; y < UINT_MAX; y--) {
        *pch++ = ' ';
        *pch++ = '|';

        for (x = 11; x > 5; x--) {
            *pch++ = ' ';
            *pch++ = anBoard[1][x] > y ? 'X' : anBoard[0][23 - x] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = anBoard[1][24] > y ? 'X' : ' ';
        *pch++ = ' ';
        *pch++ = '|';

        for (; x < UINT_MAX; x--) {
            *pch++ = ' ';
            *pch++ = anBoard[1][x] > y ? 'X' : anBoard[0][23 - x] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';

        for (x = 0; x < 3; x++)
            *pch++ = (cOffX > 5 * x + y) ? 'X' : ' ';

        *pch++ = ' ';

        if (y < 2 && asz[5 - y])
            for (pchIn = asz[5 - y]; *pchIn; pchIn++)
                *pch++ = *pchIn;

        *pch++ = '\n';
    }

    strcpy(pch, fRoll ? " +12-11-10--9--8--7-------6--5--4--3--2--1-+     " :
           " +13-14-15-16-17-18------19-20-21-22-23-24-+     ");
    pch += 49;

    if (asz[6])
        for (pchIn = asz[6]; *pchIn; pchIn++)
            *pch++ = *pchIn;

    *pch++ = '\n';
    *pch = 0;

    return sz;
}


/*
 *     GNU Backgammon  Position ID: 0123456789ABCD
 *                     Match ID   : 0123456789ABCD
 *     +24-23-22-21-20-19------18-17-16-15-14-13-+  O: gnubg (Cube: 2)     
 *  OO | O     O  O  O  O |   |                  |  0 points               
 *  OO |          O     O |   |                  |  Cube offered at 2      
 *   O |          O       |   |                  |                         
 *   O |                  |   |                  |                         
 *   O |                  |   |                  |                         
 *     |                  |BAR|                  |v Cube: 1 (7 point match)
 *   X |                  |   |                  |                         
 *   X |                  |   |                  |                         
 *   X |                  |   |                  |                         
 *   X | X  X  X  X       |   |                  |  Rolled 11              
 *  XX | X  X  X  X  X    |   |                  |  0 points               
 *     +-1--2--3--4--5--6-------7--8--9-10-11-12-+  X: Gary (Cube: 2)      
 *
 */

static char *
DrawBoardCls(char *sz, const TanBoard anBoard, int fRoll, char *asz[], char *szMatchID, int nChequers)
{

    char *pch = sz, *pchIn;
    unsigned int x, y, cOffO = nChequers, cOffX = nChequers;
    TanBoard an;
    static char achX[17] = "     X6789ABCDEF", achO[17] = "     O6789ABCDEF";

    for (x = 0; x < 25; x++) {
        cOffO -= anBoard[0][x];
        cOffX -= anBoard[1][x];
    }

    pch += sprintf(pch, "%18s  %s: ", _("GNU Backgammon"), _("Position ID"));

    if (fRoll)
        strcpy(pch, PositionID((ConstTanBoard) anBoard));
    else {
        for (x = 0; x < 25; x++) {
            an[0][x] = anBoard[1][x];
            an[1][x] = anBoard[0][x];
        }

        strcpy(pch, PositionID((ConstTanBoard) an));
    }

    pch += 14;
    *pch++ = '\n';

    /* match id */

    if (szMatchID && *szMatchID) {
        pch += sprintf(pch, "                    %s   : %s\n", _("Match ID"), szMatchID);
    }

    strcpy(pch, fRoll ? "    +24-23-22-21-20-19------18-17-16-15-14-13-+  " :
           "    +-1--2--3--4--5--6-------7--8--9-10-11-12-+  ");
    pch += 49;

    if (asz[0])
        for (pchIn = asz[0]; *pchIn; pchIn++)
            *pch++ = *pchIn;

    *pch++ = '\n';

    for (y = 0; y < 4; y++) {

        for (x = 2; x < UINT_MAX; x--)
            *pch++ = (cOffO > 5 * x + y) ? 'O' : ' ';

        *pch++ = ' ';
        *pch++ = '|';

        for (x = 23; x > 17; x--) {
            *pch++ = ' ';
            *pch++ = anBoard[1][x] > y ? 'X' : anBoard[0][23 - x] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = anBoard[0][24] > y ? 'O' : ' ';
        *pch++ = ' ';
        *pch++ = '|';

        for (; x > 11; x--) {
            *pch++ = ' ';
            *pch++ = anBoard[1][x] > y ? 'X' : anBoard[0][23 - x] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = ' ';
        if (y < 2 && asz[y + 1])
            for (pchIn = asz[y + 1]; *pchIn; pchIn++)
                *pch++ = *pchIn;

        *pch++ = '\n';
    }

    for (x = 2; x < UINT_MAX; x--)
        *pch++ = (cOffO > 5 * x + 4) ? 'O' : ' ';

    *pch++ = ' ';
    *pch++ = '|';

    for (x = 23; x > 17; x--) {
        *pch++ = ' ';
        *pch++ = anBoard[1][x] ? achX[anBoard[1][x]] : achO[anBoard[0][23 - x]];
        *pch++ = ' ';
    }

    *pch++ = '|';
    *pch++ = ' ';
    *pch++ = achO[anBoard[0][24]];
    *pch++ = ' ';
    *pch++ = '|';

    for (; x > 11; x--) {
        *pch++ = ' ';
        *pch++ = anBoard[1][x] ? achX[anBoard[1][x]] : achO[anBoard[0][23 - x]];
        *pch++ = ' ';
    }

    *pch++ = '|';

    *pch++ = ' ';
    *pch++ = ' ';

    *pch++ = '\n';

    strcpy(pch, "    |                  |BAR|                  |");
    pch = strchr(pch, 0);
    *pch++ = fRoll ? 'v' : '^';
    *pch++ = ' ';

    if (asz[3])
        for (pchIn = asz[3]; *pchIn; pchIn++)
            *pch++ = *pchIn;

    *pch++ = '\n';

    for (x = 2; x < UINT_MAX; x--)
        *pch++ = (cOffX > 5 * x + 4) ? 'X' : ' ';

    *pch++ = ' ';
    *pch++ = '|';

    for (x = 0; x < 6; x++) {
        *pch++ = ' ';
        *pch++ = anBoard[1][x] ? achX[anBoard[1][x]] : achO[anBoard[0][23 - x]];
        *pch++ = ' ';
    }

    *pch++ = '|';
    *pch++ = ' ';
    *pch++ = achX[anBoard[1][24]];
    *pch++ = ' ';
    *pch++ = '|';

    for (; x < 12; x++) {
        *pch++ = ' ';
        *pch++ = anBoard[1][x] ? achX[anBoard[1][x]] : achO[anBoard[0][23 - x]];
        *pch++ = ' ';
    }

    *pch++ = '|';

    *pch++ = ' ';
    *pch++ = ' ';

    *pch++ = '\n';

    for (y = 3; y < UINT_MAX; y--) {

        for (x = 2; x < UINT_MAX; x--)
            *pch++ = (cOffX > 5 * x + y) ? 'X' : ' ';

        *pch++ = ' ';
        *pch++ = '|';

        for (x = 0; x < 6; x++) {
            *pch++ = ' ';
            *pch++ = anBoard[1][x] > y ? 'X' : anBoard[0][23 - x] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = anBoard[1][24] > y ? 'X' : ' ';
        *pch++ = ' ';
        *pch++ = '|';

        for (; x < 12; x++) {
            *pch++ = ' ';
            *pch++ = anBoard[1][x] > y ? 'X' : anBoard[0][23 - x] > y ? 'O' : ' ';
            *pch++ = ' ';
        }

        *pch++ = '|';
        *pch++ = ' ';
        *pch++ = ' ';
        if (y < 2 && asz[5 - y])
            for (pchIn = asz[5 - y]; *pchIn; pchIn++)
                *pch++ = *pchIn;

        *pch++ = '\n';
    }

    strcpy(pch, fRoll ? "    +-1--2--3--4--5--6-------7--8--9-10-11-12-+   " :
           "    +24-23-22-21-20-19------18-17-16-15-14-13-+  ");
    pch += 49;

    if (asz[6])
        for (pchIn = asz[6]; *pchIn; pchIn++)
            *pch++ = *pchIn;

    *pch++ = '\n';
    *pch = 0;

    return sz;
}

extern char *
DrawBoard(char *sz, const TanBoard anBoard, int fRoll, char *asz[], char *szMatchID, int nChequers)
{
    if (fClockwise == FALSE)
        return (DrawBoardStd(sz, anBoard, fRoll, asz, szMatchID, nChequers));

    return (DrawBoardCls(sz, anBoard, fRoll, asz, szMatchID, nChequers));
}

static char *
FormatPoint(char *pch, int n)
{

    g_assert(n >= 0);

    /*don't translate 'off' and 'bar' as these may be used in UserCommand at a later
     * point */
    if (!n) {
        strcpy(pch, ("off"));
        return pch + strlen(("off"));
    } else if (n == 25) {
        strcpy(pch, ("bar"));
        return pch + strlen(("bar"));
    } else if (n > 9)
        *pch++ = (char) (n / 10) + '0';

    *pch++ = (n % 10) + '0';

    return pch;
}

static char *
FormatPointPlain(char *pch, int n)
{

    g_assert(n >= 0);

    if (n > 9)
        *pch++ = (char) (n / 10) + '0';

    *pch++ = (n % 10) + '0';

    return pch;
}

extern char *
FormatMovePlain(char *sz, TanBoard anBoard, int anMove[8])
{

    char *pch = sz;
    int i, j;

    for (i = 0; i < 8 && anMove[i] >= 0; i += 2) {
        pch = FormatPointPlain(pch, anMove[i] + 1);
        *pch++ = '/';
        pch = FormatPointPlain(pch, anMove[i + 1] + 1);

        if (anBoard && anMove[i + 1] >= 0 && anBoard[0][23 - anMove[i + 1]]) {
            for (j = 1;; j += 2)
                if (j > i) {
                    *pch++ = '*';
                    break;
                } else if (anMove[i + 1] == anMove[j])
                    break;
        }

        if (i < 6)
            *pch++ = ' ';
    }

    *pch = 0;

    return sz;
}

static int
CompareMovesSimple(const void *p0, const void *p1)
{

    int n0 = *((const int *) p0), n1 = *((const int *) p1);

    if (n0 != n1)
        return n1 - n0;
    else
        return *((const int *) p1 + 1) - *((const int *) p0 + 1);
}

extern void
CanonicalMoveOrder(int an[])
{

    int i;

    for (i = 0; i < 4 && an[2 * i] > -1; i++);

    qsort(an, i, sizeof(int) << 1, CompareMovesSimple);
}

extern char *
FormatMove(char *sz, const TanBoard anBoard, int anMove[8])
{

    char *pch = sz;
    int aanMove[4][4], *pnSource[4], *pnDest[4], i, j;
    int fl = 0;
    int anCount[4], nMoves, nDuplicate, k;

    /* Re-order moves into 2-dimensional array. */
    for (i = 0; i < 4 && anMove[i << 1] >= 0; i++) {
        aanMove[i][0] = anMove[i << 1] + 1;
        aanMove[i][1] = anMove[(i << 1) | 1] + 1;
        pnSource[i] = aanMove[i];
        pnDest[i] = aanMove[i] + 1;
    }

    while (i < 4) {
        aanMove[i][0] = aanMove[i][1] = -1;
        pnSource[i++] = NULL;
    }

    /* Order the moves in decreasing order of source point. */
    qsort(aanMove, 4, 4 * sizeof(int), CompareMovesSimple);

    /* Combine moves of a single chequer. */
    for (i = 0; i < 4; i++)
        for (j = i; j < 4; j++)
            if (pnSource[i] && pnSource[j] && *pnDest[i] == *pnSource[j]) {
                if (anBoard[0][24 - *pnDest[i]])
                    /* Hitting blot; record intermediate point. */
                    *++pnDest[i] = *pnDest[j];
                else
                    /* Non-hit; elide intermediate point. */
                    *pnDest[i] = *pnDest[j];

                pnSource[j] = NULL;
            }

    /* Compact array. */
    i = 0;

    for (j = 0; j < 4; j++)
        if (pnSource[j]) {
            if (j > i) {
                pnSource[i] = pnSource[j];
                pnDest[i] = pnDest[j];
            }

            i++;
        }

    while (i < 4)
        pnSource[i++] = NULL;

    for (i = 0; i < 4; i++)
        anCount[i] = pnSource[i] ? 1 : 0;

    for (i = 0; i < 3; i++) {
        if (pnSource[i]) {
            nMoves = (int) (pnDest[i] - pnSource[i]);
            for (j = i + 1; j < 4; j++) {
                if (pnSource[j]) {
                    nDuplicate = 1;

                    if (pnDest[j] - pnSource[j] != nMoves)
                        nDuplicate = 0;
                    else
                        for (k = 0; k <= nMoves && nDuplicate; k++) {
                            if (pnSource[i][k] != pnSource[j][k])
                                nDuplicate = 0;
                        }
                    if (nDuplicate) {
                        anCount[i]++;
                        pnSource[j] = NULL;
                    }
                }
            }
        }
    }

    /* Compact array. */
    i = 0;

    for (j = 0; j < 4; j++)
        if (pnSource[j]) {
            if (j > i) {
                pnSource[i] = pnSource[j];
                pnDest[i] = pnDest[j];
                anCount[i] = anCount[j];
            }

            i++;
        }

    if (i < 4)
        pnSource[i] = NULL;

    for (i = 0; i < 4 && pnSource[i]; i++) {
        if (i)
            *pch++ = ' ';

        pch = FormatPoint(pch, *pnSource[i]);

        for (j = 1; pnSource[i] + j < pnDest[i]; j++) {
            *pch = '/';
            pch = FormatPoint(pch + 1, pnSource[i][j]);
            *pch++ = '*';
            fl |= 1 << pnSource[i][j];
        }

        *pch = '/';
        pch = FormatPoint(pch + 1, *pnDest[i]);

        if (*pnDest[i] && anBoard[0][24 - *pnDest[i]] && !(fl & (1 << *pnDest[i]))) {
            *pch++ = '*';
            fl |= 1 << *pnDest[i];
        }

        if (anCount[i] > 1) {
            *pch++ = '(';
            *pch++ = '0' + (char) anCount[i];
            *pch++ = ')';
        }
    }

    *pch = 0;

    return sz;
}

extern int
ParseMove(char *pch, int an[8])
{

    int i, j, iBegin, iEnd, n, c = 0, anUser[12];
    unsigned fl = 0;

    while (*pch) {
        if (isspace(*pch)) {
            pch++;
            continue;
        } else if (isdigit(*pch)) {
            if (c == 8) {
                /* Too many points. */
                errno = EINVAL;
                return -1;
            }

            if ((anUser[c] = (int) strtol(pch, &pch, 10)) < 0 || anUser[c] > 25) {
                /* Invalid point number. */
                errno = EINVAL;
                return -1;
            }

            c++;
            continue;
        } else
            switch (*pch) {
            case 'o':
            case 'O':
            case '-':
                if (c == 8) {
                    /* Too many points. */
                    errno = EINVAL;
                    return -1;
                }

                anUser[c++] = 0;

                if (*pch != '-' && (pch[1] == 'f' || pch[1] == 'F')) {
                    pch++;
                    if (pch[1] == 'f' || pch[1] == 'F')
                        pch++;
                }
                break;

            case 'b':
            case 'B':
                if (c == 8) {
                    /* Too many points. */
                    errno = EINVAL;
                    return -1;
                }

                anUser[c++] = 25;

                if (pch[1] == 'a' || pch[1] == 'A') {
                    pch++;
                    if (pch[1] == 'r' || pch[1] == 'R')
                        pch++;
                }
                break;

            case '/':
                if (!c || fl & (1 << c)) {
                    /* Leading '/', or duplicate '/'s. */
                    errno = EINVAL;
                    return -1;
                }

                fl |= 1 << c;
                break;

            case '*':
            case ',':
            case ')':
                /* Currently ignored. */
                break;

            case '(':
                if ((n = (int) strtol(pch + 1, &pch, 10) - 1) < 1) {
                    /* invalid count */
                    errno = EINVAL;
                    return -1;
                }

                if (c < 2) {
                    /* incomplete move before ( -- syntax error */
                    errno = EINVAL;
                    return -1;
                }

                if (fl & (1 << c)) {
                    /* / immediately before ( -- syntax error */
                    errno = EINVAL;
                    return -1;
                }

                for (iBegin = c - 1; iBegin >= 0; iBegin--)
                    if (!(fl & (1 << iBegin)))
                        break;

                if (iBegin < 0) {
                    /* no / anywhere before ( -- syntax error */
                    errno = EINVAL;
                    return -1;
                }

                iEnd = c;

                if (c + (iEnd - iBegin) * n > 8) {
                    /* Too many moves. */
                    errno = EINVAL;
                    return -1;
                }

                for (i = 0; i < n; i++)
                    for (j = iBegin; j < iEnd; j++) {
                        if (fl & (1 << j))
                            fl |= 1 << c;
                        anUser[c++] = anUser[j];
                    }

                break;

            default:
                errno = EINVAL;
                return -1;
            }
        pch++;
    }

    if (fl & (1 << c)) {
        /* Trailing '/'. */
        errno = EINVAL;
        return -1;
    }

    for (i = 0, j = 0; j < c; j++) {
        if (i == 8) {
            /* Too many moves. */
            errno = EINVAL;
            return -1;
        }

        if (((i & 1) && anUser[j] == 25) || (!(i & 1) && !anUser[j])) {
            /* Trying to move from off the board, or to the bar. */
            errno = EINVAL;
            return -1;
        }

        an[i] = anUser[j];

        if ((i & 1) && (fl & (1 << (j + 1)))) {
            /* Combined move; this destination is also the next source. */
            if (i == 7) {
                /* Too many moves. */
                errno = EINVAL;
                return -1;
            }

            if (!an[i] || an[i] == 25) {
                errno = EINVAL;
                return -1;
            }

            an[++i] = anUser[j];
        }

        i++;
    }

    if (i & 1) {
        /* Incomplete last move. */
        errno = EINVAL;
        return -1;
    }

    if (i < 8)
        an[i] = -1;

    CanonicalMoveOrder(an);

    return i >> 1;
}

/*
 * Generate FIBS board
 *
 * (see <URL:http://www.fibs.com/fibs_interface.html#board_state>)
 *
 * gnubg extends the meaning of the "doubled" field, so the sign of
 * the field indicates who doubled. This is necessary to correctly
 * detects beavers and raccoons.
 *
 */

extern char *
FIBSBoard(char *pch, TanBoard anBoard, int fRoll,
          const char *szPlayer, const char *szOpp, int nMatchTo,
          int nScore, int nOpponent, int nDice0, int nDice1,
          int nCube, int fCubeOwner, int fDoubled, int fTurn, int fCrawford, int nChequers)
{
    char *sz = pch;
    int i, anOff[2];

    /* Names and match length/score */
    strcpy(sz, "board:");

    for (sz += 6; *szPlayer; szPlayer++)
        *sz++ = (*szPlayer != ':' ? *szPlayer : '_');

    for (*sz++ = ':'; *szOpp; szOpp++)
        *sz++ = (*szOpp != ':' ? *szOpp : '_');

    sprintf(sz, ":%d:%d:%d:", nMatchTo, nScore, nOpponent);

    /* Opponent on bar */
    sprintf(strchr(sz, 0), "%d:", -(int) anBoard[0][24]);

    /* Board */
    for (i = 0; i < 24; i++) {
        int point = (int) anBoard[0][23 - i];
        sprintf(strchr(sz, 0), "%d:", (point > 0) ? -point : (int) anBoard[1][i]);
    }

    /* Player on bar */
    sprintf(strchr(sz, 0), "%d:", anBoard[1][24]);

    /* Whose turn */
    strcat(sz, fRoll ? "1:" : "-1:");

    anOff[0] = anOff[1] = nChequers ? nChequers : 15;
    for (i = 0; i < 25; i++) {
        anOff[0] -= anBoard[0][i];
        anOff[1] -= anBoard[1][i];
    }

    sprintf(strchr(sz, 0), "%d:%d:%d:%d:%d:%d:%d:%d:1:-1:0:25:%d:%d:0:0:0:"
            "0:%d:0", nDice0, nDice1, nDice0, nDice1, fTurn < 0 ? 1 : nCube,
            fTurn < 0 || fCubeOwner != 0, fTurn < 0 || fCubeOwner != 1,
            fDoubled ? (fTurn ? -1 : 1) : 0, anOff[1], anOff[0], fCrawford);

    return pch;
}

extern int
ProcessFIBSBoardInfo(FIBSBoardInfo * brdInfo, ProcessedFIBSBoard * procBrd)
{

    int i, n, fCanDouble, fOppCanDouble, anOppDice[2];
    int nTmp, fNonCrawford, fPostCrawford;
    int nTurn, nColor, nDirection;
    int anFIBSBoard[26];
    int fMustSwap = 0;
    GString *tmpName;
    
    procBrd->nMatchTo = brdInfo->nMatchTo;
    procBrd->nScore = brdInfo->nScore;
    procBrd->nScoreOpp = brdInfo->nScoreOpp;
    nTurn = brdInfo->nTurn;
    nColor = brdInfo->nColor;
    nDirection = brdInfo->nDirection;
    procBrd->fDoubled = brdInfo->fDoubled;
    procBrd->nCube = brdInfo->nCube;
    fCanDouble = brdInfo->fCanDouble;
    fOppCanDouble = brdInfo->fOppCanDouble;
    memcpy(procBrd->anDice, brdInfo->anDice, sizeof(brdInfo->anDice));
    memcpy(anOppDice, brdInfo->anOppDice, sizeof(brdInfo->anOppDice));
    memcpy(anFIBSBoard, brdInfo->anFIBSBoard, sizeof(brdInfo->anFIBSBoard));
    fNonCrawford = brdInfo->fNonCrawford;
    fPostCrawford = brdInfo->fPostCrawford;

    /* Not yet supported set to zero */
    procBrd->nResignation = 0;
    
    for (i = 0; i < 26; ++i)
        anFIBSBoard[i] = -anFIBSBoard[i];

    /* FIBS has a maximum match length of 99.  Unlimited matches are 
     * encoded with a match length of 9999.
     */
    if (procBrd->nMatchTo == 9999)
        procBrd->nMatchTo = 0;

    if (procBrd->nMatchTo && (procBrd->nMatchTo <= procBrd->nScore || procBrd->nMatchTo <= procBrd->nScoreOpp))
        return -1;

    /* If the match length exceeds MAXSCORE we correct the match length
     * to MAXSCORE and the scores to the closest equivalents.
     */
    if (procBrd->nMatchTo > MAXSCORE) {
        if (procBrd->nMatchTo - procBrd->nScore > MAXSCORE)
            procBrd->nScore = 0;
        else
            procBrd->nScore -= procBrd->nMatchTo - MAXSCORE;

        if (procBrd->nMatchTo - procBrd->nScoreOpp > MAXSCORE)
            procBrd->nScoreOpp = 0;
        else
            procBrd->nScoreOpp -= procBrd->nMatchTo - MAXSCORE;

        procBrd->nMatchTo = MAXSCORE;
    }

    /* Consistency check: 0 is a valid value for nColor but signifies
     * end of game which is invalid for our purposes here.
     */
    if (!nTurn || !nColor || !nDirection)
        return -1;

    /* Check whether the cube was turned.  That is indicated by the
     * pfDoubed flag for the "player" and by setting the may double
     * flag to zero for both players for the opponent.  Actually
     * we could completely ignore the pfDoubled flag but it could
     * still help, when processing data from other sources than
     * fibs.com.
     */
    if (!procBrd->fDoubled && !fCanDouble && !fOppCanDouble)
        procBrd->fDoubled = 1;

    if (procBrd->fDoubled)
        fMustSwap = !fMustSwap;
    if (nTurn * nColor < 0)
        fMustSwap = !fMustSwap;

    /* Opponent's turn? */
    if (fMustSwap) {
        nTmp = procBrd->nScore;
        procBrd->nScore = procBrd->nScoreOpp;
        procBrd->nScoreOpp = nTmp;
        nTmp = fCanDouble;
        fCanDouble = fOppCanDouble;
        fOppCanDouble = nTmp;
        tmpName = brdInfo->gsName;
        brdInfo->gsName = brdInfo->gsOpp; 
        brdInfo->gsOpp = tmpName;
    }

    if (nTurn * nColor < 0)
        nDirection = -nDirection;
    nColor = nTurn > 0 ? 1 : -1;

    for (i = 0; i < 24; ++i) {
        n = nDirection < 0 ? anFIBSBoard[i + 1] : anFIBSBoard[25 - i - 1];
        if (nColor * n < 0) {
            procBrd->anBoard[1][i] = n < 0 ? -n : n;
            procBrd->anBoard[0][23 - i] = 0;
        } else if (nColor * n > 0) {
            procBrd->anBoard[1][i] = 0;
            procBrd->anBoard[0][23 - i] = n < 0 ? -n : n;
        } else {
            procBrd->anBoard[1][i] = procBrd->anBoard[0][23 - i] = 0;
        }
    }

    if (nDirection < 0) {
        n = anFIBSBoard[25];
        procBrd->anBoard[1][24] = n < 0 ? -n : n;
        n = anFIBSBoard[0];
        procBrd->anBoard[0][24] = n < 0 ? -n : n;
    } else {
        n = anFIBSBoard[0];
        procBrd->anBoard[1][24] = n < 0 ? -n : n;
        n = anFIBSBoard[25];
        procBrd->anBoard[0][24] = n < 0 ? -n : n;
    }

    /* See https://savannah.gnu.org/bugs/?36485 for this.  */
    if (procBrd->fDoubled)
        SwapSides(procBrd->anBoard);

    if (!procBrd->anDice[0] && anOppDice[0]) {
        procBrd->anDice[0] = anOppDice[0];
        procBrd->anDice[1] = anOppDice[1];
    }

    /*
     * Crawford detection.  This is rather tricky with FIBS board states
     * because FIBS sets both may-double flags to 1 in the Crawford game.
     *
     * We have to inspect the last and second last field for that.
     * The last field is the post-Crawford flag, the second last
     * field is the non-Crawford flag.
     *
     * Until at least one of the players is 1-away, you cannot deduce whether
     * the Crawford rule is in use or not.  Once one of the players
     * is 1-away, the non-Crawford flag is set to 3 if the Crawford rule
     * is not in use; otherwise everything is still 0.
     *
     * Once the Crawford game is finished, the post-Crawford flag is set
     * to 1.
     *
     * Since we cannot find out whether the Crawford rule is in use or not
     * in the pre-Crawford games, the cubeful evaluation can be slightly
     * biased.  But since we the vast majority of matches on FIBS is played
     * with the Crawford rule and we assume usage of the Crawford rule
     * as a default, the bias is negligible.
     */
    if (!procBrd->nMatchTo) {
        procBrd->fCrawford = 0;
    } else {
        if (((procBrd->nMatchTo - procBrd->nScore) == 1) || ((procBrd->nMatchTo - procBrd->nScoreOpp) == 1)) {
            if (fNonCrawford || fPostCrawford) {
                procBrd->fCrawford = 0;
            } else {
                procBrd->fCrawford = 1;
            }
        } else {
            procBrd->fCrawford = 0;
        }
    }

    procBrd->fCubeOwner = fCanDouble != fOppCanDouble ? fCanDouble : -1;
    g_strlcpy(procBrd->szPlayer, brdInfo->gsName->str, MAX_NAME_LEN);
    g_strlcpy(procBrd->szOpp, brdInfo->gsOpp->str, MAX_NAME_LEN);

    return 0;
}
