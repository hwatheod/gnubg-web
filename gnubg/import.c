/*
 * import.c
 *
 * by Øystein Johansen, 2000, 2001, 2002
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
 * $Id: import.c,v 1.204 2015/04/12 22:00:25 plm Exp $
 */

#include "config.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <time.h>
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include "backgammon.h"
#include "drawboard.h"
#if USE_GTK
#include "gtkgame.h"
#endif
#include "import.h"
#include "file.h"
#include "positionid.h"
#include "matchequity.h"

#if !GLIB_CHECK_VERSION (2,26,0)
#ifdef WIN32
#define GStatBuf struct _g_stat_struct
#else
typedef struct stat GStatBuf;
#endif
#endif

static int
ParseSnowieTxt(char *sz,
               int *pnMatchTo, int *pfJacoby, int *pfUnused1, int *pfUnused2,
               int *pfTurn, char aszPlayer[2][MAX_NAME_LEN], int *pfCrawfordGame,
               int anScore[2], int *pnCube, int *pfCubeOwner, TanBoard anBoard, int anDice[2]);


static int
IsValidMove(const TanBoard anBoard, const int anMove[8])
{

    TanBoard anBoardTemp;
    int anMoveTemp[8];

    memcpy(anBoardTemp, anBoard, 2 * 25 * sizeof(int));
    memcpy(anMoveTemp, anMove, 8 * sizeof(int));

    if (!ApplyMove(anBoardTemp, anMoveTemp, TRUE))
        return 1;
    else
        return 0;

}


static void
ParseSetDate(char *szFilename)
{
    /* Gamesgrid mat files contain date in filename.
     * for other files use last file access date
     */

    GStatBuf filestat;
    struct tm *matchdate = NULL;

#if HAVE_STRPTIME
    char *pch;
    struct tm mdate;
    for (pch = szFilename; *pch != '\0'; pch++);        /* goto end of filename */
    while (pch != szFilename) {
        if (*--pch == '-') {    /* go backwards until '-' */
            /* check if 8 digits before - are valid date */
            if (strptime(pch - 8, "%Y%m%d", &mdate) == pch) {
                matchdate = &mdate;
                SetMatchInfo(&mi.pchPlace, "GamesGrid", NULL);
            }
        }
    }
#endif
    /* date could not be parsed, use date of last modification */
    if (matchdate == NULL) {
        if (g_stat(szFilename, &filestat) == 0) {
            matchdate = localtime((time_t *) & filestat.st_mtime);
        }
    }

    if (matchdate != NULL) {
        mi.nYear = matchdate->tm_year + 1900;
        mi.nMonth = matchdate->tm_mon + 1;
        mi.nDay = matchdate->tm_mday;
    }
}


static int
ReadInt16(FILE * pf, int *n)
{

    /* Read a little-endian, signed (2's complement) 16-bit integer.
     * This is inefficient on hardware which is already little-endian
     * or 2's complement, but at least it's portable. */

    /* FIXME what about error handling? */

    unsigned char auch[2];

    if (fread(auch, 2, 1, pf) != 1)
        return FALSE;
    *n = auch[0] | (auch[1] << 8);

    if (*n >= 0x8000)
        *n -= 0x10000;

    return TRUE;
}

static int
ParseJF(FILE * fp, int *pnMatchTo, int *pfJacoby, int *pfTurn, char aszPlayer[2][MAX_NAME_LEN],
        int *pfCrawfordGame, int *pfPostCrawford, int anScore[2], int *pnCube, int *pfCubeOwner,
        TanBoard anBoard, int anDice[2], int *pfCubeUse, int *pfBeavers)
{

    int nVersion, nCubeOwner, nOnRoll, nMovesLeft, nMovesRight;
    int nGameOrMatch, nOpponent, nLevel, nScore1, nScore2;
    int nDie1, nDie2;
    int fCaution, fSwapDice, fJFplayedLast;
    int i, idx, anNew[26], anOld[26];
    char szLastMove[25];
    unsigned char c;
    int no_val;
    int val_crawford;

    if (!ReadInt16(fp, &nVersion))
        goto read_failed;

    if (nVersion < 124 || nVersion > 126) {
        outputl(_("File not recognised as Jellyfish file."));
        return -1;
    }
    if (nVersion == 126) {
        /* 3.0 */
        if (!ReadInt16(fp, &fCaution))
            goto read_failed;
        if (!ReadInt16(fp, &no_val))
            goto read_failed;   /* Not in use */
    }

    if (nVersion == 125 || nVersion == 126) {
        /* 1.6 or newer */
        /* 3 variables not used by older version */
        if (!ReadInt16(fp, &*pfCubeUse))
            goto read_failed;
        if (!ReadInt16(fp, &*pfJacoby))
            goto read_failed;
        if (!ReadInt16(fp, &*pfBeavers))
            goto read_failed;

        if (nVersion == 125) {
            /* If reading, caution can be set here use caution = */
        }
    }

    if (nVersion == 124) {
        /* 1.0, 1.1 or 1.2 If reading, the other variables can be set here */
        /* use cube = jacoby = beaver = use caution = */
    }

    if (!ReadInt16(fp, &*pnCube))
        goto read_failed;
    if (!ReadInt16(fp, &nCubeOwner))
        goto read_failed;
    /* Owner: 1 or 2 is player 1 or 2, respectively, 0 means cube in the middle */

    *pfCubeOwner = nCubeOwner - 1;

    if (!ReadInt16(fp, &nOnRoll))
        goto read_failed;
    /* 0 means starting position. If you add 2 to the player (to get 3 or 4) Sure? it means that the player is on
     * roll but the dice have not been rolled yet. */

    if (!ReadInt16(fp, &nMovesLeft))
        goto read_failed;
    if (!ReadInt16(fp, &nMovesRight))
        goto read_failed;
    /* These two variables are used when you use movement #1, (two buttons) and tells how many moves you have left
     * to play with the left and the right die, respectively. Initialized to 1 (if you roll a double, left = 4 and
     * right = 0). If movement #2 (one button), only the first one (left) is used to store both dice.  */

    if (!ReadInt16(fp, &no_val))        /* Not in use */
        goto read_failed;

    if (!ReadInt16(fp, &nGameOrMatch))
        goto read_failed;
    /* 1 = match, 3 = game */

    if (!ReadInt16(fp, &nOpponent))
        goto read_failed;
    /* 1 = 2 players, 2 = JF plays one side */

    if (!ReadInt16(fp, &nLevel))
        goto read_failed;

    if (!ReadInt16(fp, &*pnMatchTo))
        goto read_failed;
    /* 0 if single game */

    if (nGameOrMatch == 3)
        *pnMatchTo = 0;

    if (!ReadInt16(fp, &nScore1))
        goto read_failed;
    if (!ReadInt16(fp, &nScore2))
        goto read_failed;
    /* Can be whatever if match length = 0 */

    anScore[0] = nScore1;
    anScore[1] = nScore2;

    if (fread(&c, 1, 1, fp) != 1)
        goto read_failed;
    for (i = 0; i < c; i++) {
        if (fread(&aszPlayer[0][i], 1, 1, fp) != 1)
            goto read_failed;
    }

    aszPlayer[0][c] = '\0';

    if (nOpponent == 2)
        strcpy(aszPlayer[0], "Jellyfish");

    if (fread(&c, 1, 1, fp) != 1)
        goto read_failed;

    for (i = 0; i < c; i++) {
        if (fread(&aszPlayer[1][i], 1, 1, fp) != 1)
            goto read_failed;
    }
    aszPlayer[1][c] = '\0';

    if (!ReadInt16(fp, &fSwapDice))
        goto read_failed;
    /* TRUE if lower die is to be drawn to the left */

    if (!ReadInt16(fp, &val_crawford))
        goto read_failed;
    switch (val_crawford) {
    case 2:                    /* Crawford */
        *pfPostCrawford = FALSE;
        *pfCrawfordGame = TRUE;
        break;

    case 3:                    /* post-Crawford */
        *pfPostCrawford = TRUE;
        *pfCrawfordGame = FALSE;
        break;

    default:
        *pfCrawfordGame = *pfPostCrawford = FALSE;
        break;
    }

    if (nGameOrMatch == 3)
        *pfCrawfordGame = *pfPostCrawford = FALSE;

    if (!ReadInt16(fp, &fJFplayedLast))
        goto read_failed;

    if (fread(&c, 1, 1, fp) != 1)
        goto read_failed;

    for (i = 0; i < c; i++) {
        if (fread(&szLastMove[i], 1, 1, fp) != 1)
            goto read_failed;
    }
    szLastMove[c] = '\0';
    /* Stores whether the last move was played by JF If so, the move is stored in a string to be displayed in the
     * 'Show last' dialog box */

    if (!ReadInt16(fp, &nDie1))
        goto read_failed;
    if (!ReadInt16(fp, &nDie2))
        goto read_failed;
    nDie1 = abs(nDie1);
    /* if ( nDie1 < 0 ) { nDie1=65536; } *//* What?? */


    /* In the end the position itself is stored, as well as the old position to be able to undo. The two position
     * arrays can be read like this: */

    for (i = 0; i < 26; i++) {
        if (!ReadInt16(fp, &(anNew[i])))
            goto read_failed;
        if (!ReadInt16(fp, &(anOld[i])))
            goto read_failed;
        anNew[i] -= 20;
        anOld[i] -= 20;
        /* 20 has been added to each number when storing */
    }
    /* Player 1's checkers are represented with negative numbers, player 2's with positive. The arrays are
     * representing the 26 different points on the board, starting with anNew[0] which is the upper bar and ending
     * with anNew[25] which is the bottom bar. The remaining numbers are in the opposite direction of the numbers
     * you see if you choose 'Numbers' from the 'View' menu, so anNew[1] is marked number 24 on the screen. */


    if (nOnRoll == 1 || nOnRoll == 3)
        idx = 0;
    else
        idx = 1;

    *pfTurn = idx;

    if (nOnRoll == 0)
        *pfTurn = -1;

    anDice[0] = nDie1;
    anDice[1] = nDie2;

    for (i = 0; i < 25; i++) {
        if (anNew[i + 1] < 0)
            anBoard[idx][i] = -anNew[i + 1];
        else
            anBoard[idx][i] = 0;

        if (anNew[24 - i] > 0)
            anBoard[!idx][i] = anNew[24 - i];
        else
            anBoard[!idx][i] = 0;
    }

    SwapSides(anBoard);

    return 0;

  read_failed:
    outputerr(_("Failed reading Jellyfish file"));
    fclose(fp);
    return -2;
}

static int
ImportJF(FILE * fp, char *UNUSED(szFileName))
{

    moverecord *pmr;
    int nMatchTo = 0;
    int fJacoby = 0;
    int fTurn = 0;
    int fCrawfordGame = 0;
    int fPostCrawford = 0;
    int anScore[2] = { 0, 0 };
    int nCube = 0;
    int fCubeOwner = 0;
    int anDice[2] = { 0, 0 };
    TanBoard anBoard;
    int fCubeUse = 0;
    int fBeavers = 0;
    char aszPlayer[2][MAX_NAME_LEN];
    int i;

    if (!get_input_discard())
        return -1;

#if USE_GTK
    if (fX) {                   /* Clear record to avoid ugly updates */
        GTKClearMoveRecord();
        GTKFreeze();
    }
#endif

    if (ParseJF(fp, &nMatchTo, &fJacoby, &fTurn, aszPlayer,
                &fCrawfordGame, &fPostCrawford, anScore, &nCube, &fCubeOwner,
                anBoard, anDice, &fCubeUse, &fBeavers) < 0) {
        outputl(_("This file is not a valid Jellyfish .pos file!\n"));
        return -1;
    }

    FreeMatch();
    ClearMatch();

    InitBoard(ms.anBoard, ms.bgv);

    ClearMoveRecord();

    ListInsert(&lMatch, plGame);

    /* game info */

    pmr = NewMoveRecord();

    pmr->mt = MOVE_GAMEINFO;
    pmr->g.i = 0;
    pmr->g.nMatch = nMatchTo;
    pmr->g.anScore[0] = anScore[0];
    pmr->g.anScore[1] = anScore[1];
    pmr->g.fCrawford = TRUE;
    pmr->g.fCrawfordGame = fCrawfordGame;
    pmr->g.fJacoby = fJacoby;
    pmr->g.fWinner = -1;
    pmr->g.nPoints = 0;
    pmr->g.fResigned = FALSE;
    pmr->g.nAutoDoubles = 0;
    pmr->g.bgv = VARIATION_STANDARD;    /* assume standard backgammon */
    pmr->g.fCubeUse = fCubeUse; /* assume use of cube */
    IniStatcontext(&pmr->g.sc);

    AddMoveRecord(pmr);

    ms.fTurn = ms.fMove = fTurn;

    for (i = 0; i < 2; ++i)
        strcpy(ap[i].szName, aszPlayer[i]);

    /* dice */

    if (anDice[0]) {
        pmr = NewMoveRecord();

        pmr->mt = MOVE_SETDICE;
        pmr->fPlayer = fTurn;
        pmr->anDice[0] = anDice[0];
        pmr->anDice[1] = anDice[1];
        pmr->lt = LUCK_NONE;
        pmr->rLuck = (float) ERR_VAL;
        AddMoveRecord(pmr);
    }

    /* board */

    pmr = NewMoveRecord();

    pmr->mt = MOVE_SETBOARD;
    if (fTurn)
        SwapSides(anBoard);
    PositionKey((ConstTanBoard) anBoard, &pmr->sb.key);
    AddMoveRecord(pmr);

    /* cube value */

    pmr = NewMoveRecord();

    pmr->mt = MOVE_SETCUBEVAL;
    pmr->scv.nCube = nCube;
    AddMoveRecord(pmr);

    /* cube position */

    pmr = NewMoveRecord();

    pmr->mt = MOVE_SETCUBEPOS;
    pmr->scp.fCubeOwner = fCubeOwner;
    AddMoveRecord(pmr);

    /* update menus etc */

    UpdateSettings();

#if USE_GTK
    if (fX) {
        GTKThaw();
        GTKSet(ap);
    }
#endif

    return 0;

}

static int fWarned, fPostCrawford;

static int
ExpandMatMove(const TanBoard anBoard, int anMove[8], int *pc, const unsigned int anDice[2])
{

    int i, j, k;
    int c = *pc;

    if (anDice[0] != anDice[1]) {

        /* FIXME: accept bearoff moves with overage, like 65: 9/Off */

        if ((unsigned int) (anMove[0] - anMove[1]) == (anDice[0] + anDice[1])) {

            int an[8];

            /* consolidated move, e.g. 61: 11/4. Let's hope that the notation cannot
             * be as bad as 61: 11/4* meaning 11/5* 5/4 */

            for (i = 0; i < 2; ++i) {
                an[0] = anMove[0];
                an[1] = an[0] - anDice[i];

                an[2] = an[1];
                an[3] = an[2] - anDice[!i];

                an[4] = -1;
                an[5] = -1;

                /* no hits on the first part of the move */
                if (anBoard[0][23 - an[1]] != 0)
                    continue;

                if (IsValidMove(anBoard, an)) {
                    memcpy(anMove, an, sizeof(an));
                    ++*pc;
                    break;
                }

            }

        }

    } else {

        for (i = 0; i < c; ++i) {

            /* if this is a consolidated move then "expand" it */

            j = (anDice[0] - 1 + anMove[2 * i] - anMove[2 * i + 1]) / anDice[0];

            for (k = 1; k < j; ++k) {
                /* new move */
                anMove[2 * *pc] = anMove[2 * *pc + 1] = anMove[2 * i];
                anMove[2 * *pc + 1] -= anDice[0];

                /* fix old move */

                anMove[2 * i] -= anDice[0];

                ++*pc;

                if (*pc > 4)
                    /* something is wrong */
                    return -1;

                /* terminator */

                if (*pc < 4) {
                    anMove[2 * *pc] = -1;
                    anMove[2 * *pc + 1] = -1;
                }

            }

        }

    }

    if (c != *pc)
        /* reorder moves */
        CanonicalMoveOrder(anMove);

    return 0;

}

static void
ParseMatMove(char *sz, int iPlayer, int *warned)
{

    char *pch;
    moverecord *pmr;
    int i, c;
    static int fBeaver = FALSE;

    sz += strspn(sz, " \t");

    if (!*sz)
        return;

    for (pch = strchr(sz, 0) - 1; pch >= sz && (*pch == ' ' || *pch == '\t'); pch--)
        *pch = 0;

    if (sz[0] >= '1' && sz[0] <= '6' && sz[1] >= '1' && sz[1] <= '6' && sz[2] == ':') {
        int dice1 = sz[0] - '0';
        int dice2 = sz[1] - '0';

        if (fBeaver) {
            /* look likes the previous beaver was taken */
            pmr = NewMoveRecord();

            pmr->mt = MOVE_TAKE;
            pmr->fPlayer = iPlayer;
            if (!LinkToDouble(pmr)) {
                outputl(_("Take record found but doesn't follow a double"));
                free(pmr);
                return;
            }
            AddMoveRecord(pmr);
        }
        fBeaver = FALSE;

        if (!StrNCaseCmp(sz + 4, "???", 3)) {
            /*
             * Apparently XG writes this when the player resigned after rolling
             * Put it back to '   ' and fall through to "standard" .mat handling
             */
            sz[4] = sz[5] = sz[6] = ' ';
            /*
             * Sometimes it puts a 4th one...
             */
            if (!StrNCaseCmp(sz + 7, "?", 1))
                sz[7] = ' ';
        }

        if (!StrNCaseCmp(sz + 4, "Cannot Move", 11)) {
            /*
             * Sometimes it writes this instead of nothing for dancing rolls
             */
            sz[4] = 0;
        }

        /* XG can have additional spaces between roll and "Illegal play" */
        while (sz[4] == ' ')
            sz++;

        if (!StrNCaseCmp(sz + 4, "illegal play", 12)) {
            /* Snowie type illegal play */

            int nMatchTo, fJacoby, fUnused1, fUnused2, fTurn, fCrawfordGame;
            int anScore[2], nCube, fCubeOwner, anDice[2];
            TanBoard anBoard;
            char aszPlayer[2][MAX_NAME_LEN];

            if ((pch = strchr(sz + 4, '(')) == 0) {
                outputl(_("No '(' following text 'Illegal play'"));
                return;
            }

            /* step over '(' */
            ++pch;

            if (ParseSnowieTxt(pch, &nMatchTo, &fJacoby, &fUnused1, &fUnused2,
                               &fTurn, aszPlayer, &fCrawfordGame, anScore, &nCube, &fCubeOwner, anBoard, anDice)) {
                outputl(_("Illegal Snowie position format for illegal play."));
                return;
            }

            /* FIXME: we only handle illegal moves; not illegal cubes,
             * changes to the match score etc. */

            pmr = NewMoveRecord();
            pmr->mt = MOVE_SETDICE;
            pmr->fPlayer = iPlayer;
            pmr->anDice[0] = dice1;
            pmr->anDice[1] = dice2;
            AddMoveRecord(pmr);

            pmr = NewMoveRecord();
            pmr->mt = MOVE_SETBOARD;
            if (fTurn)
                SwapSides(anBoard);
            PositionKey((ConstTanBoard) anBoard, &pmr->sb.key);
            AddMoveRecord(pmr);

            return;
        }

        pmr = NewMoveRecord();
        pmr->mt = MOVE_NORMAL;
        pmr->anDice[0] = dice1;
        pmr->anDice[1] = dice2;
        pmr->fPlayer = iPlayer;

        c = ParseMove(sz + 3, pmr->n.anMove);

        FixMatchState(&ms, pmr);

        if (c > 0) {

            /* moves found */

            for (i = 0; i < (c << 1); i++)
                pmr->n.anMove[i]--;
            if (c < 4)
                pmr->n.anMove[c << 1] = pmr->n.anMove[(c << 1) | 1] = -1;

            /* remove consolidation */

            if (ExpandMatMove(msBoard(), pmr->n.anMove, &c, pmr->anDice))
                outputf(_("WARNING: Expand move failed. This file " "contains garbage!"));

            /* check if move is valid */

            if (!IsValidMove(msBoard(), pmr->n.anMove)) {
                if (*warned == -1)
                    outputf(_("WARNING: Invalid move: \"%s\" encountered\n"), sz + 3);
                else
                    *warned = TRUE;
                return;
            }

            AddMoveRecord(pmr);

            return;
        } else if (!c) {

            /* roll but no move found; if there are legal moves assume
             * a resignation */
            movelist ml;
            int anDice[2];
            anDice[0] = pmr->anDice[0];
            anDice[1] = pmr->anDice[1];

            switch (GenerateMoves(&ml, msBoard(), pmr->anDice[0], pmr->anDice[1], FALSE)) {

            case 0:

                /* no legal moves; record the movenormal */

                pmr->n.anMove[0] = pmr->n.anMove[1] = -1;
                AddMoveRecord(pmr);

                return;

            case 1:

                /* forced move; record the movenormal */

                memcpy(pmr->n.anMove, ml.amMoves[0].anMove, sizeof(pmr->n.anMove));
                AddMoveRecord(pmr);

                return;

            default:

                /* legal moves; just record roll */

                free(pmr);      /* free movenormal from above */

                pmr = NewMoveRecord();
                pmr->mt = MOVE_SETDICE;
                pmr->fPlayer = iPlayer;
                pmr->anDice[0] = anDice[0];
                pmr->anDice[1] = anDice[1];
                AddMoveRecord(pmr);

                return;

            }

        }

    }

    if (!StrNCaseCmp(sz, "double", 6) || !StrNCaseCmp(sz, "beavers", 7) || !StrNCaseCmp(sz, "raccoons", 7)) {
        pmr = NewMoveRecord();
        pmr->mt = MOVE_DOUBLE;
        pmr->fPlayer = iPlayer;
        LinkToDouble(pmr);
        AddMoveRecord(pmr);
        fBeaver = !StrNCaseCmp(sz, "beavers", 6);

    } else if (!StrNCaseCmp(sz, "take", 4)) {
        pmr = NewMoveRecord();
        pmr->mt = MOVE_TAKE;
        pmr->fPlayer = iPlayer;
        if (!LinkToDouble(pmr)) {
            outputl(_("Take record found but doesn't follow a double"));
            free(pmr);
            return;
        }
        AddMoveRecord(pmr);
    } else if (!StrNCaseCmp(sz, "drop", 4)) {

        pmr = NewMoveRecord();
        pmr->mt = MOVE_DROP;
        pmr->fPlayer = iPlayer;
        if (!LinkToDouble(pmr)) {
            outputl(_("Drop record found but doesn't follow a double"));
            free(pmr);
            return;
        }
        AddMoveRecord(pmr);
    } else if (!StrNCaseCmp(sz, "win", 3)) {
        if (ms.gs == GAME_PLAYING) {
            /* Neither a drop nor a bearoff to win, so we presume the loser
             * resigned. */
            int n = atoi(sz + 4);
            pmr = NewMoveRecord();
            pmr->mt = MOVE_RESIGN;
            pmr->fPlayer = !iPlayer;
            if (n <= ms.nCube)
                pmr->r.nResigned = 1;
            else if (n <= 2 * ms.nCube)
                pmr->r.nResigned = 2;
            else
                pmr->r.nResigned = 3;

            AddMoveRecord(pmr);
        }
    } else if (!StrNCaseCmp(sz, "resign", 6)) {
        return;                 /* Ignore this .gam line */
    } else if (!fWarned) {
        outputf(_("Unrecognised move \"%s\" in .mat file.\n"), sz);
        fWarned = TRUE;
    }
}

#define START_STRING "Game "
#define START_STRING_LEN 5

static char *
GetMatLine(FILE * fp)
{
    static char szLine[1024];

    do {
        if (feof(fp) || !fgets(szLine, sizeof(szLine), fp))
            return NULL;
    } while (strspn(szLine, " \n\r\t") == strlen(szLine));

    return szLine;
}

static int
ImportGame(FILE * fp, int iGame, int nLength, bgvariation bgVariation, int *warned)
{

    char sz0[MAX_NAME_LEN], sz1[MAX_NAME_LEN], *pch, *pchLeft, *pchRight = NULL, *szLine;
    int n0, n1;
    char *colon, *psz, *pszEnd;
    moverecord *pmr;

    /* Process player score line, avoid fscanf(%nn) as buffer may overrun */
    szLine = GetMatLine(fp);
    psz = szLine;
    while (isspace(*psz))
        psz++;
    /* 1st player name */
    colon = strchr(psz, ':');
    if (!colon)
        return 0;
    pszEnd = colon - 1;
    while (isspace(*pszEnd))
        pszEnd--;
    psz[MIN(pszEnd - psz + 1, MAX_NAME_LEN - 1)] = '\0';
    strcpy(sz0, psz);
    /* 1st score */
    psz = colon + 1;
    while (isspace(*psz))
        psz++;
    n0 = atoi(psz);
    /* 2nd player name */
    while (!isspace(*psz))
        psz++;
    while (isspace(*psz))
        psz++;
    colon = strchr(psz, ':');
    if (!colon)
        return 0;
    pszEnd = colon - 1;
    while (isspace(*pszEnd))
        pszEnd--;
    psz[MIN(pszEnd - psz + 1, MAX_NAME_LEN - 1)] = '\0';
    strcpy(sz1, psz);
    /* 2nd score */
    psz = colon + 1;
    while (isspace(*psz))
        psz++;
    n1 = atoi(psz);

    if (nLength && (n0 >= nLength || n1 >= nLength))
        /* match is already over -- ignore extra games */
        return 1;

    InitBoard(ms.anBoard, bgVariation);

    ClearMoveRecord();

    ListInsert(&lMatch, plGame);

    for (pch = sz0; *pch; pch++);
    for (pch--; pch >= sz0 && isspace(*pch);)
        *pch-- = 0;
    for (; pch >= sz0; pch--)
        if (isspace(*pch))
            *pch = ' ';
        else if (*pch == ',' && strtol(pch + 1, NULL, 10) > 0) {        /* GamesGrid mat files have ratings */
            *pch = '\0';        /* after player name                */
            SetMatchInfo(&mi.pchRating[0], pch + 1, NULL);
        }
    strcpy(ap[0].szName, sz0);

    for (pch = sz1; *pch; pch++);
    for (pch--; pch >= sz1 && isspace(*pch);)
        *pch-- = 0;
    for (; pch >= sz1; pch--)
        if (isspace(*pch))
            *pch = ' ';
        else if (*pch == ',' && strtol(pch + 1, NULL, 10) > 0) {        /* GamesGrid mat files have ratings */
            *pch = '\0';        /* after player name                */
            SetMatchInfo(&mi.pchRating[1], pch + 1, NULL);
        }
    strcpy(ap[1].szName, sz1);

    pmr = NewMoveRecord();
    pmr->mt = MOVE_GAMEINFO;
    pmr->g.i = iGame;
    pmr->g.nMatch = nLength;
    pmr->g.anScore[0] = n0;
    pmr->g.anScore[1] = n1;
    pmr->g.fCrawford = (nLength > 0);   /* assume JF always uses Crawford rule */
    if ((pmr->g.fCrawfordGame = !fPostCrawford && (n0 == nLength - 1) ^ (n1 == nLength - 1)))
        fPostCrawford = TRUE;
    pmr->g.fJacoby = fJacoby && (nLength == 0);
    pmr->g.fWinner = -1;
    pmr->g.nPoints = 0;
    pmr->g.fResigned = FALSE;
    pmr->g.nAutoDoubles = 0;
    pmr->g.bgv = bgVariation;
    pmr->g.fCubeUse = fCubeUse;
    IniStatcontext(&pmr->g.sc);
    AddMoveRecord(pmr);

    while ((szLine = GetMatLine(fp)) && !g_strrstr(szLine, START_STRING)) {
        pchRight = pchLeft = NULL;

        if ((pch = strpbrk(szLine, "\n\r")) != 0)
            *pch = 0;

        if (!strncmp(szLine, "; Set Pos=", 10) && szLine[36] == '/') {
            /* XG Set Pos extension */
            char *pos, *posid;
            TanBoard anBoard;
            char szcv[5], szco[2];

            pos = g_strndup(szLine + 10, 26);
            PositionFromXG(anBoard, pos);
            g_free(pos);

            ms.gs = GAME_PLAYING;
            ms.fMove = 0;

            if (szLine[37] == '0') {
                sprintf(szcv, "1");
                CommandSetCubeCentre(NULL);
            } else if (szLine[37] == '-') {
                sprintf(szcv, "%d", 1 << (szLine[38] - '0'));
                /* FIXME:
                 * CommandSetCubeOwner() needs its argument to be writable
                 * CommandSetCubeOwner("1") coredumps
                 */
                sprintf(szco, "0");
                CommandSetCubeOwner(szco);
            } else {
                sprintf(szcv, "%d", 1 << (szLine[37] - '0'));
                sprintf(szco, "1");
                CommandSetCubeOwner(szco);
            }

            CommandSetCubeValue(szcv);

            ms.fMove = 0;

            posid = g_strdup(PositionID((ConstTanBoard) anBoard));
            CommandSetBoard(posid);
            g_free(posid);

            continue;
        }

        if ((pchLeft = strchr(szLine, ':')) && (pchRight = strchr(pchLeft + 1, ':')) && pchRight > szLine + 3)
            *((pchRight -= 2) - 1) = 0;
        else if (strlen(szLine) > 15 && (pchRight = strstr(szLine + 15, "  ")))
            *pchRight++ = 0;

        if ((pchLeft = strchr(szLine, ')')) != 0)
            pchLeft++;
        else
            pchLeft = szLine;

        ParseMatMove(pchLeft, 0, warned);

        if (pchRight && *warned < TRUE)
            ParseMatMove(pchRight, 1, warned);
        if (*warned == TRUE)
            return 1;
    }

    AddGame(pmr);

    return (ms.nMatchTo && (ms.anScore[0] >= ms.nMatchTo || ms.anScore[1] >= ms.nMatchTo));
}

static int
ImportMatVariation(FILE * fp, char *szFilename, bgvariation bgVariation, int warned)
{
    int n = 0, nLength = -1, game;
    char ch;
    gchar *pchComment = NULL;
    char *szLine;

    fWarned = fPostCrawford = FALSE;

    do {
        szLine = GetMatLine(fp);
        if (!szLine) {
            outputerrf(_("%s: not a valid .mat file"), szFilename);
            g_free(pchComment);
            return -1;
        }

        if (*szLine == (char) 0xef && *(szLine + 1) == (char) 0xbb && *(szLine + 2) == (char) 0xbf)
            szLine += 3;        /* skip UTF junk at the start of XG mat files */

        if (*szLine == '#' || *szLine == ';') {
            /* comment */
            char *pchOld = pchComment;
            char *pch;
            if ((pch = strpbrk(szLine, "\n\r")) != 0)
                *pch = 0;
            pch = szLine + 1;
            while (isspace(*pch))
                ++pch;
            if (*pch) {
                gchar *p;

                if (g_str_has_prefix(pch, "[Event ")) {
                    if ((p = g_strrstr(pch, "\"]")))
                        *p = 0;
                    SetMatchInfo(&mi.pchEvent, pch + 8, NULL);
                } else if (g_str_has_prefix(pch, "[Round ")) {
                    if ((p = g_strrstr(pch, "\"]")))
                        *p = 0;
                    SetMatchInfo(&mi.pchRound, pch + 8, NULL);
                } else if (g_str_has_prefix(pch, "[Site ")) {
                    if ((p = g_strrstr(pch, "\"]")))
                        *p = 0;
                    SetMatchInfo(&mi.pchPlace, pch + 7, NULL);
                } else if (g_str_has_prefix(pch, "[Transcriber ")) {
                    if ((p = g_strrstr(pch, "\"]")))
                        *p = 0;
                    SetMatchInfo(&mi.pchAnnotator, pch + 14, NULL);
                } else if (g_str_has_prefix(pch, "[EventDate ")) {
                    sscanf(pch + 12, "%4u.%2u.%2u", &mi.nYear, &mi.nMonth, &mi.nDay);
                } else if (g_str_has_prefix(pch, "[EventTime ")) {
                    ;           /* discard ; we don't have this in matchinfo */
                } else if (g_str_has_prefix(pch, "[Unrated ")) {
                    ;           /* discard */
                } else if (g_str_has_prefix(pch, "[Player ")) {
                    ;           /* discard. We don't have player names
                                 * in matchinfo and rely on games
                                 * headers. Elos (even if not left at
                                 * default 1600) are useless if their
                                 * source is unknown. Maybe keep them
                                 * for known Site values ? */
                } else if (g_str_has_prefix(pch, "[Match ID ")) {
                    ;           /* discard */
                } else if (g_str_has_prefix(pch, "[CubeLimit ")) {
                    ;           /* discard ; maybe keep as comment if != 1024 ? */
                } else if (g_str_has_prefix(pch, "[Variation \"Backgammon")) {
                    bgVariation = VARIATION_STANDARD;
                } else if (g_str_has_prefix(pch, "[Variation \"NackGammon")) {
                    bgVariation = VARIATION_NACKGAMMON;
                } else if (g_str_has_prefix(pch, "[Variation \"HyperGammon (1)")) {
                    bgVariation = VARIATION_HYPERGAMMON_1;
                } else if (g_str_has_prefix(pch, "[Variation \"HyperGammon (2)")) {
                    bgVariation = VARIATION_HYPERGAMMON_2;
                } else if (g_str_has_prefix(pch, "[Variation \"HyperGammon (3)")) {
                    bgVariation = VARIATION_HYPERGAMMON_3;
                } else if (g_str_has_prefix(pch, "[Crawford ")) {
                    ;           /* discard for now */
                } else if (g_str_has_prefix(pch, "[Jacoby ")) {
                    ;           /* discard for now */
                } else if (g_str_has_prefix(pch, "[Beaver ")) {
                    ;           /* discard for now */
                } else if (g_str_has_prefix(pch, "[Game ")) {

                    /* Discard useless BGNJ comment. Its format is :
                     * ; [Game 1, Move 1: Dice set to Manual Rolls]
                     * or
                     * ; [Game 2, Move 1: Dice set to Mersenne Twister, seed=1290023973, rollNum=586]
                     */
                    gchar **token;

                    token = g_strsplit(pch, " ", 8);
                    if (!strcmp(token[2], "Move")
                        && !strcmp(token[4], "Dice")
                        && !strcmp(token[5], "set")
                        && !strcmp(token[6], "to"));    /* discard */
                    else {
                        pchComment = g_strconcat(pchComment ? pchComment : "", pch, "\n", NULL);
                        g_free(pchOld);
                    }
                    g_strfreev(token);

                } else {
                    pchComment = g_strconcat(pchComment ? pchComment : "", pch, "\n", NULL);
                    g_free(pchOld);
                }
            }
        } else
            /* Look for start of mat file */
            n = sscanf(szLine, "%10d %*1[Pp]oint %*1[Mm]atch%c", &nLength, &ch);
    } while (n != 2);

    if (nLength < 0) {
        outputerrf(_("Invalid match length %d found in mat file\n"), nLength);
        return -1;
    } else if (nLength > MAXSCORE)
        outputerrf(("GNU Backgammon doesn't support the match length(%d), "
                    "maximum is %d. Proceeding anyway, but expect the " "roof to fall down!"), nLength, MAXSCORE);

#if USE_GTK
    if (fX) {                   /* Clear record to avoid ugly updates */
        GTKClearMoveRecord();
        GTKFreeze();
    }
#endif

    if (mi.nYear == 0)
        ParseSetDate(szFilename);

    if (pchComment)
        mi.pchComment = g_strdup(pchComment);   /* no need to free mi.pchComment as it's
                                                 * already done in ClearMatch */
    g_free(pchComment);

    szLine = GetMatLine(fp);
    while (szLine) {
        if (g_strrstr(szLine, START_STRING)) {
            game = atoi(g_strchug(szLine) + START_STRING_LEN);
            if (!game)
                outputf(_("WARNING! Unrecognized line in mat file: '%s'\n"), szLine);
            {
                if (ImportGame(fp, game - 1, nLength, bgVariation, &warned))
                    break;      /* import failed */
            }
        } else
            szLine = GetMatLine(fp);

    }

    UpdateSettings();

    {
        gchar **token;
        int i = 0;

        token = g_strsplit_set(player1aliases, ";", -1);

        while (token[i] != NULL)
            if (!strcmp(token[i++], ap[0].szName)) {
                CommandSwapPlayers(NULL);
                break;
            }

        g_strfreev(token);
    }

#if USE_GTK
    if (fX) {
        GTKSet(ap);
        GTKThaw();
    }
#endif

    return (warned > 0);
}

static int
ImportMat(FILE * fp, char *szFilename)
{
    bgvariation bgv;

    if (!get_input_discard())
        return -1;
    FreeMatch();
    ClearMatch();

    for (bgv = VARIATION_STANDARD; bgv < NUM_VARIATIONS; bgv++) {
        gchar *str;
        if (ImportMatVariation(fp, szFilename, bgv, FALSE) == 0)
            return 0;
        str = g_strdup_printf(N_("Import as a %s match had errors. Try a different variation?"), aszVariations[bgv]);
        if (!GetInputYN(str)) {
            g_free(str);
            return 0;
        }
        g_free(str);

        /* reset and try a different format */
        if (fseek(fp, 0L, SEEK_SET) != 0)
            return -1;
        FreeMatch();
        ClearMatch();
    }

    return ImportMatVariation(fp, szFilename, VARIATION_STANDARD, -1);
}

static int
isAscending(const int anMove[8])
{
    if (anMove[0] < 0)
        return FALSE;

    if (anMove[0] != 24) {
        /* not moving from the bar */
        if (anMove[1] > -1)
            /* not moving off */
            return anMove[0] < anMove[1];
        else
            return anMove[0] > 17;
    } else
        return anMove[1] < 6;
}

static void
ParseOldmove(char *sz, int fInvert)
{
    int iPlayer, i, c;
    moverecord *pmr;
    char *pch;

    switch (*sz) {
    case 'X':
        iPlayer = fInvert;
        break;
    case 'O':
        iPlayer = !fInvert;
        break;
    default:
        goto error;
    }

    if (sz[3] == '(' && strlen(sz) >= 8) {
        pmr = NewMoveRecord();
        pmr->mt = MOVE_NORMAL;
        pmr->anDice[0] = sz[4] - '0';
        pmr->anDice[1] = sz[6] - '0';
        pmr->fPlayer = iPlayer;

        if (!StrNCaseCmp(sz + 9, "can't move", 10))
            c = 0;
        else {
            for (pch = sz + 9; *pch; pch++)
                if (*pch == '-')
                    *pch = '/';

            c = ParseMove(sz + 9, pmr->n.anMove);
        }

        if (c >= 0) {

            for (i = 0; i < (c << 1); i++)
                pmr->n.anMove[i]--;

            /* if the moves are ascending invert it.
             * In theory this is only necessary when iPlayer == fInvert,
             * but some programs may write incompatible files.
             * Joseph's fibs2html is one such program. */

            if (c && isAscending(pmr->n.anMove)) {
                /* ascending moves: invert */
                for (i = 0; i < (c << 1); ++i)
                    if (pmr->n.anMove[i] > -1 && pmr->n.anMove[i] < 24)
                        /* invert numbers (other than bar=24 and off=-1) */
                        pmr->n.anMove[i] = 23 - pmr->n.anMove[i];
            }

            if (c < 4)
                pmr->n.anMove[c << 1] = pmr->n.anMove[(c << 1) | 1] = -1;

            /* We have to order the moves before calling AddMoveRecord
             * Think about the opening roll of 6 5, and the legal move
             * 18/13 24/18 is entered. AddMoveRecord will then silently
             * fail in the sub-call to PlayMove(). This problem should
             * maybe be fixed in PlayMove (drawboard.c). Opinions? */

            CanonicalMoveOrder(pmr->n.anMove);

            if (!IsValidMove(msBoard(), pmr->n.anMove)) {
                outputf(_("WARNING! Illegal or invalid move: '%s'\n"), sz);
                free(pmr);
            } else {

                /* Now we're ready */

                AddMoveRecord(pmr);

            }
        } else
            free(pmr);
        return;
    } else if (!StrNCaseCmp(sz + 3, "doubles", 7)) {
        pmr = NewMoveRecord();
        pmr->mt = MOVE_DOUBLE;
        pmr->fPlayer = iPlayer;
        if (!LinkToDouble(pmr)) {
            pmr->CubeDecPtr = &pmr->CubeDec;
            pmr->CubeDecPtr->esDouble.et = EVAL_NONE;
            pmr->nAnimals = 0;
        }
        pmr->stCube = SKILL_NONE;
        AddMoveRecord(pmr);
        return;
    } else if (!StrNCaseCmp(sz + 3, "accepts", 7)) {

        pmr = NewMoveRecord();
        pmr->mt = MOVE_TAKE;
        pmr->fPlayer = iPlayer;
        if (!LinkToDouble(pmr)) {
            outputl(_("Take record found but doesn't follow a double"));
            free(pmr);
            return;
        }
        pmr->stCube = SKILL_NONE;
        AddMoveRecord(pmr);
        return;
    } else if (!StrNCaseCmp(sz + 3, "rejects", 7)) {

        pmr = NewMoveRecord();
        pmr->mt = MOVE_DROP;
        pmr->fPlayer = iPlayer;
        if (!LinkToDouble(pmr)) {
            outputl(_("Drop record found but doesn't follow a double"));
            free(pmr);
            return;
        }
        pmr->stCube = SKILL_NONE;
        AddMoveRecord(pmr);
        return;
    } else if (!StrNCaseCmp(sz + 3, "wants to resign", 15)) {
        /* ignore */
        return;
    } else if (!StrNCaseCmp(sz + 3, "wins", 4)) {
        if (ms.gs == GAME_PLAYING) {
            /* Neither a drop nor a bearoff to win, so we presume the loser
             * resigned. */
            pmr = NewMoveRecord();
            pmr->mt = MOVE_RESIGN;
            pmr->r.esResign.et = EVAL_NONE;
            pmr->fPlayer = !iPlayer;
            pmr->r.nResigned = 1;       /* FIXME determine from score */
            AddMoveRecord(pmr);
        }
        return;
    }

  error:
    if (!fWarned) {
        outputf(_("Unrecognised move \"%s\" in oldmoves file.\n"), sz);
        fWarned = TRUE;
    }
}

#if 0

static int
IncreasingScore(const int anScore[2], const int n0, const int n1)
{

    /* check for decreasing score */

    if (n0 == anScore[0]) {

        /* n0 is unchanged, n1 must be increasing */

        if (n1 <= anScore[1])
            /* score is not increasing */
            return 0;

    } else if (n1 == anScore[0]) {

        if (n0 <= anScore[1])
            /* score is not increasing */
            return 0;

    } else if (n0 == anScore[1]) {

        if (n1 <= anScore[0])
            /* score is not increasing */
            return 0;

    } else if (n1 == anScore[1]) {

        if (n0 <= anScore[0])
            /* score is not increasing */
            return 0;

    } else {
        /* most likely a new game */
        return 0;
    }

    return 1;

}

#endif

static int
NewPlayers(const char *szOld0, const char *szOld1, const char *szNew0, const char *szNew1)
{


    if (!strcmp(szOld0, szNew0))
        /* player 0 have not changed */
        return strcmp(szOld1, szNew1);
    else if (!strcmp(szOld0, szNew1))
        /* swap of player1 and player 0 */
        return strcmp(szOld1, szNew0);
    else
        /* definitely a new player */
        return 1;
}


static int
ImportOldmovesGame(FILE * pf, int iGame, int nLength, int n0, int n1)
{

    char sz[80], sz0[MAX_NAME_LEN], sz1[MAX_NAME_LEN], *pch;
    char buf[256], *pNext;
    moverecord *pmr;
    int fInvert;
    static int anExpectedScore[2];

    /* FIXME's:
     *
     * what about multiple matches?
     * pmr->r.nResigned is not correct in ParseOldMove
     *
     * Possible solution: read through file one time and collect match scores.
     * If the score is decreasing or the players's names changes stop.
     *
     * Since we now know the score after every game it's easy to 
     * have pmr->r.nResigned correct. 
     *
     */

    /* Process player score line, avoid fscanf(%nn) as buffer may overrun */
    if (fgets(buf, sizeof(buf), pf) == NULL) {
        outputerr("oldmoves");
        return 0;
    }
    pch = strstr(buf, "is X");
    if (!pch)
        return 0;
    pNext = pch + strlen("is X -");
    do
        pch--;
    while (isspace(*pch));
    pch[1] = '\0';
    pch = buf;
    while (isspace(*pch))
        pch++;
    strcpy(sz0, pch);

    pch = strstr(pNext, "is O");
    if (!pch)
        return 0;
    do
        pch--;
    while (isspace(*pch));
    pch[1] = '\0';
    pch = pNext;
    while (isspace(*pch))
        pch++;
    strcpy(sz1, pch);

    /* consistency checks */

    if (iGame) {

        if (NewPlayers(ap[0].szName, ap[1].szName, sz0, sz1))
            return 1;

        if (ms.nMatchTo != nLength)
            /* match length have changed */
            return 1;

#if 0
        if (!IncreasingScore(ms.anScore, n0, n1)) {
            printf("not increasing score %d %d %d %d\n", ms.anScore[0], ms.anScore[1], n0, n1);
            return 1;
        }
#endif

    }

    /* initialise */

    InitBoard(ms.anBoard, ms.bgv);

    ClearMoveRecord();

    ListInsert(&lMatch, plGame);

    if (!iGame) {
        strcpy(ap[0].szName, sz0);
        strcpy(ap[1].szName, sz1);
        fInvert = FALSE;
    } else
        fInvert = strcmp(ap[0].szName, sz0) != 0;

    /* add game info */


    pmr = NewMoveRecord();
    pmr->mt = MOVE_GAMEINFO;
    pmr->g.i = iGame;
    pmr->g.nMatch = nLength;
    pmr->g.anScore[fInvert] = n0;
    pmr->g.anScore[!fInvert] = n1;
    pmr->g.fCubeUse = TRUE;     /* always use cube */

    /* check if score is swapped (due to fibs oldmoves bug) */

    if (iGame && (pmr->g.anScore[0] == anExpectedScore[1] || pmr->g.anScore[1] == anExpectedScore[0])) {

        int n = pmr->g.anScore[0];
        pmr->g.anScore[0] = pmr->g.anScore[1];
        pmr->g.anScore[1] = n;

    }

    pmr->g.fCrawford = nLength != 0;    /* assume matches use Crawford rule */
    if ((pmr->g.fCrawfordGame = !fPostCrawford && (n0 == nLength - 1) ^ (n1 == nLength - 1)))
        fPostCrawford = TRUE;
    pmr->g.fJacoby = !nLength;  /* assume matches never use Jacoby rule */
    pmr->g.fWinner = -1;
    pmr->g.nPoints = 0;
    pmr->g.fResigned = FALSE;
    pmr->g.nAutoDoubles = 0;
    pmr->g.bgv = VARIATION_STANDARD;    /* only standard bg on FIBS */
    IniStatcontext(&pmr->g.sc);
    AddMoveRecord(pmr);

    do
        if (!fgets(sz, 80, pf)) {
            sz[0] = 0;
            break;
        }
    while (strspn(sz, " \n\r\t") == strlen(sz));

    do {

        if ((pch = strpbrk(sz, "\n\r")) != 0)
            *pch = 0;

        ParseOldmove(sz, fInvert);

        if (ms.gs != GAME_PLAYING)
            break;

        if (!fgets(sz, 80, pf))
            break;
    } while (strspn(sz, " \n\r\t") != strlen(sz));

    anExpectedScore[0] = pmr->g.anScore[0];
    anExpectedScore[1] = pmr->g.anScore[1];
    anExpectedScore[pmr->g.fWinner] += pmr->g.nPoints;

    AddGame(pmr);

    return 0;
}

#define MAXLINE 1024
static char *
FindScoreIs(FILE * pf, char *buffer)
{

    char *p;

    while (fgets(buffer, MAXLINE, pf)) {
        if ((p = strstr(buffer, "Score is ")) != 0)
            return p;
    }

    return 0;
}

static int
ImportOldmoves(FILE * pf, char *szFilename)
{
    char buffer[1024];
    char *p;
    int n, n0, n1, nLength, i;

    if (!get_input_discard())
        return -1;

    fWarned = fPostCrawford = FALSE;

    p = FindScoreIs(pf, buffer);
    if (p == 0) {
        outputerrf(_("%s: not a valid oldmoves file"), szFilename);
        return -1;
    }

    if ((n = sscanf(p, "Score is %10d-%10d in a %10d", &n0, &n1, &nLength)) < 2) {
        outputerrf(_("%s: not a valid oldmoves file"), szFilename);
        return -1;
    }

    if (n == 2) {
        /* assume a money game */
        nLength = 0;
    }
#if USE_GTK
    if (fX) {                   /* Clear record to avoid ugly updates */
        GTKClearMoveRecord();
        GTKFreeze();
    }
#endif

    FreeMatch();
    ClearMatch();

    i = 0;

    while (ImportOldmovesGame(pf, i++, nLength, n0, n1) == 0) {
        p = FindScoreIs(pf, buffer);
        if (p == 0)
            break;

        n = sscanf(p, "Score is %10d-%10d in a %10d", &n0, &n1, &nLength);
        if (n < 2)
            break;
    }

    UpdateSettings();

#if USE_GTK
    if (fX) {
        GTKThaw();
        GTKSet(ap);
    }
#endif

    return 0;
}

static void
ImportSGGGame(FILE * pf, int i, int nLength, int n0, int n1,
              int fCrawford,
              int fCrawfordRule, int UNUSED(fAutoDoubles), int fJacobyRule, bgvariation bgv, int fCubeUse)
{
    char sz[1024];
    char *pch = NULL;
    int c, fPlayer = 0, anRoll[2];
    moverecord *pmgi, *pmr;
    char *szComment = NULL;
    int fBeaver = FALSE;

    int fPlayerOld, nMoveOld;
    int nMove = -1;

    int fResigned = -1;

    InitBoard(ms.anBoard, ms.bgv);

    ClearMoveRecord();

    ListInsert(&lMatch, plGame);

    pmgi = NewMoveRecord();

    pmgi->mt = MOVE_GAMEINFO;
    pmgi->g.i = i - 1;
    pmgi->g.nMatch = nLength;
    pmgi->g.anScore[0] = n0;
    pmgi->g.anScore[1] = n1;
    pmgi->g.fCrawford = fCrawfordRule && nLength;
    pmgi->g.fCrawfordGame = fCrawford;
    pmgi->g.fJacoby = fJacobyRule && !nLength;
    pmgi->g.fWinner = -1;
    pmgi->g.nPoints = 0;
    pmgi->g.fResigned = FALSE;
    pmgi->g.nAutoDoubles = 0;   /* we check for automatic doubles later */
    pmgi->g.bgv = bgv;
    pmgi->g.fCubeUse = fCubeUse;
    IniStatcontext(&pmgi->g.sc);
    AddMoveRecord(pmgi);

    anRoll[0] = 0;
    anRoll[1] = 0;

    while (fgets(sz, 1024, pf)) {

        char *pchtmp;

        nMoveOld = nMove;
        fPlayerOld = fPlayer;

        /* check for game over */
        for (pch = sz; *pch; pch++)
            if ((pchtmp = strstr(pch, " wins "))) {
                pch = pchtmp;
                goto finished;
            } else if (*pch == ':' || *pch == ' ')
                break;

        if (isdigit(*sz)) {
            nMove = atoi(sz);
            for (pch = sz; isdigit(*pch); pch++);

            if (*pch++ == '\t') {
                /* we have a move! */
                if (anRoll[0]) {

                    /* check for
                     * 1        43:  
                     * 1        43:             <---- check for this line
                     * 1        43: 13/9, 13/10 
                     */

                    if (*(pch + 4) == '\n' || !*(pch + 4))
                        continue;

                    /* when beavered there is no explicit "take" */
                    if (fBeaver) {

                        pmr = NewMoveRecord();
                        pmr->mt = MOVE_TAKE;
                        pmr->sz = szComment;
                        pmr->fPlayer = fPlayer;
                        if (!LinkToDouble(pmr)) {
                            outputl(_("Beaver record found but doesn't follow a double"));
                            free(pmr);
                            return;
                        }

                        AddMoveRecord(pmr);
                        szComment = NULL;
                        fBeaver = FALSE;
                    }

                    if (!StrNCaseCmp(pch, "resign", 6) || (strlen(pch) > 4 && !StrNCaseCmp(pch + 4, "resign", 6))) {
                        /* resignation after rolling dice */
                        fResigned = fPlayer;
                        continue;
                    }

                    pmr = NewMoveRecord();
                    pmr->mt = MOVE_NORMAL;
                    pmr->sz = szComment;
                    pmr->anDice[0] = anRoll[0];
                    pmr->anDice[1] = anRoll[1];
                    pmr->fPlayer = fPlayer;

                    if ((c = ParseMove(pch + 4, pmr->n.anMove)) >= 0) {
                        for (i = 0; i < (c << 1); i++)
                            pmr->n.anMove[i]--;
                        if (c < 4)
                            pmr->n.anMove[c << 1] = pmr->n.anMove[(c << 1) | 1] = -1;

                        AddMoveRecord(pmr);
                    } else
                        free(pmr);

                    anRoll[0] = 0;
                    szComment = NULL;

                } else {
                    if ((fPlayer = *pch == '\t') != 0)
                        pch++;

                    if (*pch == ' ' && pch[1] == '-') {
                        /*
                         * Closeout. The SGG file doesn't contain any dice
                         * number but gnubg needs a roll. Set it to 6-6 then
                         * fall through to normal processing.
                         */
                        pch[0] = pch[1] = '6';
                    }

                    if (*pch >= '1' && *pch <= '6' && pch[1] >= '1' && pch[1] <= '6') {
                        /* dice roll */
                        anRoll[0] = *pch - '0';
                        anRoll[1] = pch[1] - '0';

                        if (strstr(pch, "O-O")) {

                            if (nMove == nMoveOld && fPlayerOld == fPlayer) {
                                /* handle duplicate dances:
                                 * 24     62: O-O
                                 * 24     62: O-O
                                 */
                                anRoll[0] = 0;
                                continue;
                            }

                            /* when beavered there is no explicit "take" */
                            if (fBeaver) {

                                pmr = NewMoveRecord();
                                pmr->mt = MOVE_TAKE;
                                pmr->sz = szComment;
                                pmr->fPlayer = fPlayer;
                                if (!LinkToDouble(pmr)) {
                                    outputl(_("Take record found but doesn't follow a double"));
                                    free(pmr);
                                    return;
                                }

                                AddMoveRecord(pmr);
                                szComment = NULL;
                            }

                            pmr = NewMoveRecord();
                            pmr->mt = MOVE_NORMAL;
                            pmr->sz = szComment;
                            pmr->anDice[0] = anRoll[0];
                            pmr->anDice[1] = anRoll[1];
                            pmr->fPlayer = fPlayer;
                            AddMoveRecord(pmr);

                            anRoll[0] = 0;
                            szComment = NULL;
                            fBeaver = FALSE;

                        } else {
                            for (pch += 3; *pch; pch++)
                                if (!isspace(*pch))
                                    break;

                            if (*pch && StrNCaseCmp(pch, "resign", 6)) {
                                /* Apparently SGG files can contain spurious
                                 * duplicate moves -- the only thing we can
                                 * do is ignore them. */

                                /* this catches:
                                 * 1    43: 
                                 * 1    43: 13/9, 13/10
                                 * 1    43: 13/9, 13/10 */

                                anRoll[0] = 0;
                                continue;
                            }

                            /* when beavered there is no explicit "take" */
                            if (fBeaver) {

                                pmr = NewMoveRecord();
                                pmr->mt = MOVE_TAKE;
                                pmr->sz = szComment;
                                pmr->fPlayer = fPlayer;
                                if (!LinkToDouble(pmr)) {
                                    outputl(_("Take record found but doesn't follow a double"));
                                    free(pmr);
                                    return;
                                }

                                AddMoveRecord(pmr);
                                szComment = NULL;
                            }

                            pmr = NewMoveRecord();
                            pmr->mt = MOVE_SETDICE;
                            /* we do not want comments on MOVE_SETDICE */
                            pmr->fPlayer = fPlayer;
                            pmr->anDice[0] = anRoll[0];
                            pmr->anDice[1] = anRoll[1];

                            AddMoveRecord(pmr);
                            fBeaver = FALSE;

                            if (!StrNCaseCmp(pch, "resign", 6)) {
                                /* resignation after rolling dice */
                                fResigned = fPlayer;
                                continue;
                            }

                        }
                    } else {
                        if (!StrNCaseCmp(pch, "double", 6)) {
                            if (ms.fDoubled && ms.fTurn != fPlayer)
                                /* Presumably a duplicated move in the
                                 * SGG file -- ignore */
                                continue;

                            pmr = NewMoveRecord();
                            pmr->mt = MOVE_DOUBLE;
                            pmr->sz = szComment;
                            pmr->fPlayer = fPlayer;
                            LinkToDouble(pmr);

                            AddMoveRecord(pmr);
                            fBeaver = FALSE;
                            szComment = NULL;
                        } else if (!StrNCaseCmp(pch, "beaver", 6)) {
                            /* Beaver to 4 */
                            if (ms.fDoubled && ms.fTurn != fPlayer)
                                /* Presumably a duplicated move in the
                                 * SGG file -- ignore */
                                continue;

                            pmr = NewMoveRecord();
                            pmr->mt = MOVE_DOUBLE;
                            pmr->sz = szComment;
                            pmr->fPlayer = fPlayer;
                            if (!LinkToDouble(pmr)) {
                                outputl(_("Beaver found but doesn't follow a double"));
                                free(pmr);
                                return;
                            }

                            AddMoveRecord(pmr);
                            szComment = NULL;
                            fBeaver = TRUE;
                        } else if (!StrNCaseCmp(pch, "raccoon", 7)) {
                            /* Raccoon to 8 */
                            if (ms.fDoubled && ms.fTurn != fPlayer)
                                /* Presumably a duplicated move in the
                                 * SGG file -- ignore */
                                continue;

                            pmr = NewMoveRecord();
                            pmr->mt = MOVE_DOUBLE;
                            pmr->sz = szComment;
                            pmr->fPlayer = fPlayer;
                            if (!LinkToDouble(pmr)) {
                                outputl(_("Raccoon found but doesn't follow a double"));
                                free(pmr);
                                return;
                            }

                            AddMoveRecord(pmr);
                            szComment = NULL;
                            fBeaver = TRUE;
                        } else if (!StrNCaseCmp(pch, "accept", 6)) {

                            if (!ms.fDoubled)
                                continue;

                            pmr = NewMoveRecord();
                            pmr->mt = MOVE_TAKE;
                            pmr->sz = szComment;
                            pmr->fPlayer = fPlayer;
                            if (!LinkToDouble(pmr)) {
                                outputl(_("Take record found but doesn't follow a double"));
                                free(pmr);
                                return;
                            }

                            AddMoveRecord(pmr);
                            szComment = NULL;
                            fBeaver = FALSE;
                        } else if (!StrNCaseCmp(pch, "pass", 4)) {

                            pmr = NewMoveRecord();
                            pmr->mt = MOVE_DROP;
                            pmr->sz = szComment;
                            pmr->fPlayer = fPlayer;
                            if (!LinkToDouble(pmr)) {
                                outputl(_("Drop record found but doesn't follow a double"));
                                free(pmr);
                                return;
                            }

                            AddMoveRecord(pmr);
                            szComment = NULL;
                            fBeaver = FALSE;
                        } else if (!StrNCaseCmp(pch, "resign", 4)) {
                            /* resignation */
                            fResigned = fPlayer;
                        }
                    }


                }
            } /* *pch++ == '\t' */
            else {

                /* check for automatic doubles:
                 * 
                 * 11: Automatic double
                 * 
                 */

                if (strstr(pch, "Automatic double")) {
                    ++pmgi->g.nAutoDoubles;
                    /* we've already called AddMoveRecord for pmgi, 
                     * so we manually update the cube value */
                    ms.nCube *= 2;
                }


            }


        } /* isdigit */
        else {

            /* the text is most likely a comment */

            if (*sz != '\n' && *sz != '\r') {
                /* non-empty line */
                if (!szComment)
                    szComment = g_strdup(sz);
                else {
                    szComment = (char *) g_realloc(szComment, (gulong) strlen(sz) + strlen(szComment) + 1);
                    strcat(szComment, sz);
                }

            }

        }

    }

  finished:

    if (fResigned > -1) {
        /* game was terminated by an resignation */
        /* parse string, e.g., MosheTissona_MC  wins 2 points. */

        long nr;

        pch += 6;               /* step over ' wins ' */

        nr = strtol(pch, NULL, 10);
        if (nr == 0)
            outputerrf(_("Unparseable line: %s"), sz);
        /* This doesn't really bother us as we rely
         * on the initial score of the next game.
         * The MOVE_RESIGN record below may be inaccurate, though */

        pmr = NewMoveRecord();

        pmr->mt = MOVE_RESIGN;
        pmr->sz = szComment;
        pmr->fPlayer = fResigned;
        pmr->r.nResigned = (int) nr / ms.nCube;
        if (pmr->r.nResigned > 3)
            pmr->r.nResigned = 3;
        if (pmr->r.nResigned < 1)
            pmr->r.nResigned = 1;

        szComment = NULL;

        AddMoveRecord(pmr);
    }

    AddGame(pmgi);

    /* garbage collect */

    if (szComment)
        g_free(szComment);
}

static int
ParseSGGGame(char *pch, int *pi, int *pn0, int *pn1, int *pfCrawford, int *pnLength)
{
    *pfCrawford = FALSE;

    if (strncmp(pch, "Game ", 5))
        return -1;

    pch += 5;

    *pi = (int) strtol(pch, &pch, 10);

    if (*pch != '.')
        return -1;

    pch++;

    *pn0 = (int) strtol(pch, &pch, 10);

    if (*pch == '*') {
        pch++;
        *pfCrawford = TRUE;
    }

    if (*pch != '-')
        return -1;

    pch++;

    *pn1 = (int) strtol(pch, &pch, 10);

    if (*pch == '*') {
        pch++;
        *pfCrawford = TRUE;
    }

    if (*pch != '/') {
        /* assume money session */
        *pnLength = 0;
        return 0;
    }

    pch++;

    *pnLength = (int) strtol(pch, &pch, 10);

    return 0;
}


static char *
GetValue(const char *sz, char *szValue)
{
    const char *pc;
    char *pc2;

    /* skip "Keyword:" */

    pc = strchr(sz, ':');
    if (!pc)
        return NULL;

    ++pc;

    /* string leading blanks */

    while (isspace(*pc))
        ++pc;

    if (!*pc)
        return NULL;

    strcpy(szValue, pc);

    /* strip trailing blanks */

    pc2 = strchr(szValue, 0) - 1;

    while (pc2 >= szValue && isspace(*pc2))
        *pc2-- = 0;

    return szValue;

}

static void
ParseSGGDate(const char *sz, unsigned int *pnDay, unsigned int *pnMonth, unsigned int *pnYear)
{
    static const char *aszMonths[] = {
        "January", "February", "March", "April", "May", "June", "July",
        "August", "September", "October", "November", "December"
    };
    int i;
    char szMonth[80];
    unsigned int nDay, nYear;

    *pnDay = 1;
    *pnMonth = 1;
    *pnYear = 1900;

    if (sscanf(sz, "%*s %79s %2u, %4u", szMonth, &nDay, &nYear) != 3)
        return;

    *pnDay = nDay;
    *pnYear = nYear;

    for (i = 0; i < 12; ++i) {
        if (!strcmp(szMonth, aszMonths[i])) {
            *pnMonth = i + 1;
            break;
        }
    }
}


static void
ParseSGGOptions(const char *sz, matchinfo * pmi, int *pfCrawfordRule,
                int *pfAutoDoubles, int *pfJacobyRule, bgvariation * pbgv, int *pfCubeUse)
{
    char szTemp[80];
    int i;
    static const char *aszOptions[] = {
        "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
        "Saturday", "Sunday",
        "ELO:", "Jacoby:", "Automatic Doubles:", "Crawford:", "Beavers:",
        "Raccoons:", "Doubling Cube:", "Rated:", "Match Length:",
        "Variant:", NULL
    };
    double arRating[2];
    int anExp[2];
    const char *pc;
    char *pc2;

    for (i = 0, pc = aszOptions[i]; pc; ++i, pc = aszOptions[i])
        if (!strncmp(sz, pc, strlen(pc)))
            break;

    switch (i) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:

        /* date */
        ParseSGGDate(sz, &pmi->nDay, &pmi->nMonth, &pmi->nYear);
        break;
    case 7:

        /* ratings */
        /* Players are swapped later (at the end of ImportSGG()) and ratings
         * must be swapped here as well to be correctly attributed at the end
         * Order in SGG file is player1 player0 */
        if ((sscanf(sz, "%*s %20lf %10d %20lf %10d", &arRating[0], &anExp[0], &arRating[1], &anExp[1])) != 4)
            break;

        for (i = 0; i < 2; ++i) {
            if (pmi->pchRating[i])
                free(pmi->pchRating[i]);
            sprintf(szTemp, "%.6g (Exp %d)", arRating[i], anExp[i]);
            pmi->pchRating[i] = g_strdup(szTemp);
        }
        break;

    case 8:

        /* Jacoby rule */

        if (!GetValue(sz, szTemp))
            break;

        *pfJacobyRule = !strcmp(szTemp, "Yes");
        break;

    case 9:

        /* automatic doubles */

        if (!GetValue(sz, szTemp))
            break;

        *pfAutoDoubles = !strcmp(szTemp, "Yes");
        break;

    case 10:

        /* crawford rule */

        if (!GetValue(sz, szTemp))
            break;

        *pfCrawfordRule = !strcmp(szTemp, "Yes");
        break;

    case 11:
    case 12:

        /* beavers & raccons */

        /* ignored as they are not used internally in gnubg */
        break;

    case 13:

        /* doubling cube */

        if (!GetValue(sz, szTemp))
            break;

        *pfCubeUse = !strcmp(szTemp, "Yes");
        break;

    case 14:

        /* rated */
        break;

    case 15:

        /* match length */
        break;

    case 16:

        /* variant */

        if (!GetValue(sz, szTemp))
            break;

        if (!StrCaseCmp(szTemp, "HyperGammon"))
            *pbgv = VARIATION_HYPERGAMMON_3;
        else if (!StrCaseCmp(szTemp, "Nackgammon"))
            *pbgv = VARIATION_NACKGAMMON;
        else if (!StrCaseCmp(szTemp, "Backgammon"))
            *pbgv = VARIATION_STANDARD;
        else {
            outputf("Unknown variant in SGG file\n" "Please send the SGG file to bug-gnubg@gnubg.org!\n");
            outputx();
            g_assert_not_reached();
        }

        break;

    default:

        /* most likely "place" */
        if (pmi->pchPlace)
            free(pmi->pchPlace);

        if ((pc2 = strpbrk(sz, "\n\r")) != NULL)
            *pc2 = 0;
        pmi->pchPlace = g_strdup(sz);
    }
}

static int
ImportSGG(FILE * pf, char *szFilename)
{
    char sz[80], sz0[MAX_NAME_LEN], sz1[MAX_NAME_LEN];
    int n0 = 0, n1 = 0, nLength = 0, i = 0, fCrawford = FALSE;
    int fCrawfordRule = TRUE;
    int fJacobyRule = TRUE;
    int fAutoDoubles = 0;
    int fCubeUse = TRUE;
    char buf[256], *pNext, *psz;
    bgvariation bgv = VARIATION_STANDARD;

    if (!get_input_discard())
        return -1;

    fWarned = FALSE;
    *sz0 = '\0';
    while (fgets(buf, sizeof(buf), pf)) {
        psz = strstr(buf, "vs.");
        if (psz) {              /* Found player line, avoid fscanf(%nn) as buffer may overrun */
            pNext = psz + strlen("vs.");
            do
                psz--;
            while (isspace(*psz));
            psz[1] = '\0';
            psz = buf;
            while (isspace(*psz))
                psz++;
            strcpy(sz0, psz);

            psz = pNext + strlen(pNext);
            do
                psz--;
            while (isspace(*psz));
            psz[1] = '\0';
            psz = pNext;
            while (isspace(*psz))
                psz++;
            strcpy(sz1, psz);
            break;
        }
    }

    if (!*sz0) {                /* No player line found */
        outputerrf(_("%s: not a valid SGG file"), szFilename);
        return -1;
    }
#if USE_GTK
    if (fX) {                   /* Clear record to avoid ugly updates */
        GTKClearMoveRecord();
        GTKFreeze();
    }
#endif

    FreeMatch();
    ClearMatch();

    strcpy(ap[0].szName, sz0);
    strcpy(ap[1].szName, sz1);

    while (fgets(sz, 80, pf)) {
        if (!ParseSGGGame(sz, &i, &n0, &n1, &fCrawford, &nLength))
            break;

        ParseSGGOptions(sz, &mi, &fCrawfordRule, &fAutoDoubles, &fJacobyRule, &bgv, &fCubeUse);

    }

    while (!feof(pf)) {
        ImportSGGGame(pf, i, nLength, n0, n1, fCrawford, fCrawfordRule, fAutoDoubles, fJacobyRule, bgv, fCubeUse);

        while (fgets(sz, 80, pf))
            if (!ParseSGGGame(sz, &i, &n0, &n1, &fCrawford, &nLength))
                break;
    }

    UpdateSettings();

    /* swap players */

    CommandSwapPlayers(NULL);

#if USE_GTK
    if (fX) {
        GTKThaw();
        GTKSet(ap);
    }
#endif

    return 0;

}

/*
 * Parse TMG files 
 *
 */

static int
ParseTMGOptions(const char *sz, matchinfo * pmi, int *pfCrawfordRule,
                int *UNUSED(pfAutoDoubles), int *pfJacobyRule, int *pnLength, bgvariation * pbgv, int *pfCubeUse)
{
    char szTemp[80];
    int i, j;
    static const char *aszOptions[] = {
        "MatchID:", "Player1:", "Player2:", "Stake:", "RakePct:",
        "MaxRakeAbs:", "Startdate:", "Jacoby:", "AutoDistrib:", "Beavers:",
        "Raccoons:", "Crawford:", "Cube:", "MaxCube:", "Length:", "MaxGames:",
        "Variant:", "PlayMoney:", NULL
    };
    const char *pc;
    char *pc2;
    char szName[80];

    for (i = 0, pc = aszOptions[i]; pc; ++i, pc = aszOptions[i])
        if (!strncmp(sz, pc, strlen(pc)))
            break;

    switch (i) {
    case 0:                    /* MatchID */
        pc = strchr(sz, ':') + 2;
        sprintf(szTemp, _("TMG MatchID: %s"), pc);
        if ((pc2 = strchr(szTemp, '\n')) != 0)
            *pc2 = 0;
        pmi->pchComment = g_strdup(szTemp);
        return 0;

    case 1:                    /* Player1: */
    case 2:                    /* Player2: */
        if (sscanf(sz, "Player%1d: %*d %79s %*f", &j, szName) == 2)
            if ((j == 1 || j == 2) && *szName)
                strcpy(ap[j - 1].szName, szName);
        return 0;

    case 3:                    /* Stake: */
        /* ignore, however, we could read into pmi->szComment, but I guess 
         * people do not want to see their stakes in, say, html export */
        break;

    case 4:                    /* RakePct: */
    case 5:                    /* MaxRakeAbs: */
    case 8:                    /* AutoDistrib */
    case 13:                   /* MaxCube */
    case 15:                   /* MaxGames */
        /* ignore */
        return 0;

    case 6:                    /* Startdate */

        if ((sscanf(sz, "Startdate: %4u-%2u-%2u", &pmi->nYear, &pmi->nMonth, &pmi->nDay)) != 3)
            pmi->nYear = pmi->nMonth = pmi->nDay = 0;
        return 0;

    case 7:                    /* Jacoby */
        if ((pc = strchr(sz, ':')) == 0)
            return 0;
        *pfJacobyRule = atoi(pc + 2);
        return 0;

    case 9:                    /* Beavers */
        break;

    case 10:                   /* Raccoons */
        break;

    case 11:                   /* Crawford */
        if ((pc = strchr(sz, ':')) == 0)
            return 0;
        *pfCrawfordRule = atoi(pc + 2);
        return 0;

    case 12:                   /* Cube */
        if ((pc = strchr(sz, ':')) == 0)
            return 0;
        *pfCubeUse = atoi(pc + 2);
        return 0;

    case 14:                   /* Length */
        if ((pc = strchr(sz, ':')) == 0)
            return 0;
        *pnLength = atoi(pc + 2);
        return 0;

    case 16:                   /* Variant */

        if ((pc = strchr(sz, ':')) == 0)
            return 0;
        switch (atoi(pc + 2)) {
        case 1:
            *pbgv = VARIATION_STANDARD;
            break;
        case 2:
            *pbgv = VARIATION_NACKGAMMON;
            break;
        default:
            outputf("Unknown variation in TMG file\n" "Please send the TMG file to bug-gnubg@gnubg.org!\n");
            outputx();
            g_assert_not_reached();
            return -1;
        }

        return 0;

    case 17:                   /* PlayMoney */
        break;

    }

    /* code not reachable */

    return -1;
}

static int
ParseTMGGame(const char *sz, int *piGame, int *pn0, int *pn1, int *pfCrawford, int *post_crawford, const int nLength)
{
    int i = sscanf(sz, "Game %10d: %10d-%10d", piGame, pn0, pn1) == 3;

    if (!i)
        return FALSE;

    if (nLength) {
        if (!*post_crawford)
            *post_crawford = *pfCrawford = (*pn0 == (nLength - 1))
                || (*pn1 == (nLength - 1));
        else
            *pfCrawford = FALSE;
    }

    return TRUE;
}

static void
ImportTMGGame(FILE * pf, int i, int nLength, int n0, int n1,
              int fCrawford,
              int fCrawfordRule, int UNUSED(fAutoDoubles), int fJacobyRule, bgvariation bgv, int fCubeUse)
{
    char sz[1024];
    char *pch;
    int c, fPlayer = 0, anRoll[2];
    moverecord *pmgi, *pmr;
    int j;

    typedef enum _tmgrecordtype {
        TMG_ROLL = 1,
        TMG_AUTO_DOUBLE = 2,
        TMG_MOVE = 4,
        TMG_DOUBLE = 5,
        TMG_TAKE = 7,
        TMG_BEAVER = 8,
        TMG_PASS = 10,
        TMG_WIN_SINGLE = 14,
        TMG_WIN_GAMMON = 15,
        TMG_WIN_BACKGAMMON = 16,
        TMG_OUT_OF_TIME = 17,
        TMG_TABLE_STAKE = 19,
        TMG_OUT_OF_TIME_1 = 22
    } tmgrecordtype;
    tmgrecordtype trt;

    InitBoard(ms.anBoard, ms.bgv);

    ClearMoveRecord();

    ListInsert(&lMatch, plGame);

    pmgi = NewMoveRecord();
    pmgi->mt = MOVE_GAMEINFO;
    pmgi->g.i = i - 1;
    pmgi->g.nMatch = nLength;
    pmgi->g.anScore[0] = n0;
    pmgi->g.anScore[1] = n1;
    pmgi->g.fCrawford = fCrawfordRule && nLength;
    pmgi->g.fCrawfordGame = fCrawford;
    pmgi->g.fJacoby = fJacobyRule && !nLength;
    pmgi->g.fWinner = -1;
    pmgi->g.nPoints = 0;
    pmgi->g.fResigned = FALSE;
    pmgi->g.nAutoDoubles = 0;   /* we check for automatic doubles later */
    pmgi->g.bgv = bgv;
    pmgi->g.fCubeUse = fCubeUse;
    IniStatcontext(&pmgi->g.sc);
    AddMoveRecord(pmgi);

    anRoll[0] = 0;
    anRoll[1] = 0;

    while (fgets(sz, 1024, pf)) {

        /* skip white space */

        for (pch = sz; isspace(*pch); ++pch);

        /* 
         * is it:
         *   -1 4 13/11 24/23
         * or
         * Beavers: 0
         */

        if (isdigit(*pch) || *pch == '-') {

            fPlayer = (*pch == '-');    /* player: +n for player 0,
                                         * -n for player 1 */

            /* step over move number */

            for (; !isspace(*pch); ++pch);
            for (; isspace(*pch); ++pch);

            /* determine record type */

            trt = (tmgrecordtype) atoi(pch);

            /* step over record type */

            for (; !isspace(*pch); ++pch);
            for (; isspace(*pch); ++pch);

            switch (trt) {
            case TMG_ROLL:     /* roll:    1 1 21: */

                if (*pch == '?') {
                    /* no roll recorded, because the game is lost on time out */
                    continue;
                }

                anRoll[0] = *pch - '0';
                anRoll[1] = *(pch + 1) - '0';

                pmr = NewMoveRecord();
                pmr->mt = MOVE_SETDICE;
                pmr->fPlayer = fPlayer;
                pmr->anDice[0] = anRoll[0];
                pmr->anDice[1] = anRoll[1];

                AddMoveRecord(pmr);

                break;

            case TMG_AUTO_DOUBLE:      /* automatic double:  0 2 Automatic to 2 */

                ++pmgi->g.nAutoDoubles;
                /* we've already called AddMoveRecord for pmgi, 
                 * so we manually update the cube value */
                ms.nCube *= 2;

                break;

            case TMG_MOVE:     /* move:    1 4 24/18 13/10 */

                pmr = NewMoveRecord();
                pmr->mt = MOVE_NORMAL;
                pmr->anDice[0] = anRoll[0];
                pmr->anDice[1] = anRoll[1];
                pmr->fPlayer = fPlayer;

                if (!strncmp(pch, "0/0", 3)) {  /* See if fan is legal (i.e. no moves available) - otherwise skip */
                    movelist ml;
                    if (GenerateMoves(&ml, msBoard(), pmr->anDice[0], pmr->anDice[1], FALSE) == 0) {
                        /* fans */
                        AddMoveRecord(pmr);
                    }
                } else if ((c = ParseMove(pch, pmr->n.anMove)) >= 0) {
                    for (i = 0; i < (c << 1); i++)
                        pmr->n.anMove[i]--;
                    if (c < 4)
                        pmr->n.anMove[c << 1] = pmr->n.anMove[(c << 1) | 1] = -1;

                    AddMoveRecord(pmr);
                } else
                    free(pmr);

                break;

            case TMG_DOUBLE:   /* double:   -7 5 Double to 2 */
            case TMG_BEAVER:   /* beaver:      25 8 Beaver to 8 */

                pmr = NewMoveRecord();
                pmr->mt = MOVE_DOUBLE;
                pmr->fPlayer = fPlayer;
                LinkToDouble(pmr);
                AddMoveRecord(pmr);

                break;

            case TMG_TAKE:     /* take:   -6 7 Take */

                pmr = NewMoveRecord();
                pmr->mt = MOVE_TAKE;
                pmr->fPlayer = fPlayer;
                if (!LinkToDouble(pmr)) {

                    outputl(_("Take record found but doesn't follow a double"));
                    free(pmr);
                    return;
                }

                AddMoveRecord(pmr);

                break;

            case TMG_PASS:     /* pass:    5 10 Pass */

                pmr = NewMoveRecord();
                pmr->mt = MOVE_DROP;
                pmr->fPlayer = fPlayer;
                if (!LinkToDouble(pmr)) {

                    outputl(_("Drop record found but doesn't follow a double"));
                    free(pmr);
                    return;
                }

                AddMoveRecord(pmr);

                break;

            case TMG_WIN_SINGLE:       /* win single:    4 14 1 thj wins 1 point */
            case TMG_WIN_GAMMON:       /* resign:  -30 15 2 Saltyzoo7 wins 2 points */
            case TMG_WIN_BACKGAMMON:   /* win backgammon: ??? */

                if (ms.gs == GAME_PLAYING) {
                    /* game is in progress: the opponent resigned */
                    pmr = NewMoveRecord();
                    pmr->mt = MOVE_RESIGN;
                    pmr->fPlayer = !fPlayer;
                    pmr->r.nResigned = (atoi(pch) + ms.nCube - 1) / ms.nCube;   /* rounding up */
                    AddMoveRecord(pmr);

                }

                goto finished;

            case TMG_TABLE_STAKE:
                /* Table stake:   0 19 57.68 0.00 43.57 0.00 Settlement */

                /* ignore: FIXME: this should be stored in order to analyse
                 * tables stakes correctly */

                break;

            case TMG_OUT_OF_TIME:
            case TMG_OUT_OF_TIME_1:
                break;

            default:

                outputf("Please send the TMG file to bug-gnubg@gnubg.org!\n");
                outputx();
                g_assert_not_reached();

            }


        } else {

            /* it's most likely an option: the players may enable or
             * disable beavers and auto doubles during play */

            ParseTMGOptions(sz, &mi, &pmgi->g.fCrawford, &j, &pmgi->g.fJacoby, &j, &bgv, &fCubeUse);

        }

    }

  finished:
    AddGame(pmgi);
}

static int
ImportTMG(FILE * pf, const char *UNUSED(szFilename))
{
    int fCrawfordRule = TRUE;
    int fJacobyRule = TRUE;
    int fAutoDoubles = 0;
    int nLength = 0;
    int fCrawford = FALSE;
    int fCubeUse = TRUE;
    int n0 = 0, n1 = 0;
    int i = 0, j;
    int post_crawford = FALSE;
    char sz[80];
    bgvariation bgv = VARIATION_STANDARD;

#if USE_GTK
    if (fX) {                   /* Clear record to avoid ugly updates */
        GTKClearMoveRecord();
        GTKFreeze();
    }
#endif

    FreeMatch();
    ClearMatch();

    /* clear matchinfo */

    for (j = 0; j < 2; ++j)
        SetMatchInfo(&mi.pchRating[j], NULL, NULL);
    SetMatchInfo(&mi.pchPlace, "TrueMoneyGames", NULL);
    SetMatchInfo(&mi.pchEvent, NULL, NULL);
    SetMatchInfo(&mi.pchRound, NULL, NULL);
    SetMatchInfo(&mi.pchAnnotator, NULL, NULL);
    SetMatchInfo(&mi.pchComment, NULL, NULL);
    mi.nYear = mi.nMonth = mi.nDay = 0;

    /* search for options (until first game is found) */

    while (fgets(sz, 80, pf)) {
        if (ParseTMGGame(sz, &i, &n0, &n1, &fCrawford, &post_crawford, nLength))
            break;

        ParseTMGOptions(sz, &mi, &fCrawfordRule, &fAutoDoubles, &fJacobyRule, &nLength, &bgv, &fCubeUse);

    }

    /* set remainder of match info */

    while (!feof(pf)) {

        ImportTMGGame(pf, i, nLength, n0, n1, fCrawford, fCrawfordRule, fAutoDoubles, fJacobyRule, bgv, fCubeUse);

        while (fgets(sz, 80, pf))
            if (ParseTMGGame(sz, &i, &n0, &n1, &fCrawford, &post_crawford, nLength))
                break;

    }

    UpdateSettings();

    /* swap players */

    if (ms.gs != GAME_NONE)
        CommandSwapPlayers(NULL);

#if USE_GTK
    if (fX) {
        GTKThaw();
        GTKSet(ap);
    }
#endif

    return 0;
}

static int
ParseSnowieTxt(char *sz,
               int *pnMatchTo, int *pfJacoby, int *UNUSED(p1), int *UNUSED(p2),
               int *pfTurn, char aszPlayer[2][MAX_NAME_LEN], int *pfCrawfordGame,
               int anScore[2], int *pnCube, int *pfCubeOwner, TanBoard anBoard, int anDice[2])
{
    int c;
    char *pc;
    int i, j;

    memset(anBoard, 0, 2 * 25 * sizeof(int));

    c = 0;
    i = 0;
    pc = NextTokenGeneral(&sz, ";");
    while (pc) {

        switch (c) {
        case 0:
            /* match length */
            *pnMatchTo = atoi(pc);
            break;
        case 1:
            /* Jacoby rule */
            *pfJacoby = atoi(pc);
            break;
        case 2:
        case 3:
            /* unknown??? */
            break;
        case 4:
            /* player on roll */
            *pfTurn = atoi(pc);
            break;
        case 5:
        case 6:
            /* player names */
            j = *pfTurn;
            if (c == 6)
                j = !j;

            memset(aszPlayer[j], 0, MAX_NAME_LEN);
            if (*pc)
                strncpy(aszPlayer[j], pc, MAX_NAME_LEN - 1);
            else
                sprintf(aszPlayer[j], "Player %d", j);
            break;
        case 7:
            /* Crawford Game */
            *pfCrawfordGame = atoi(pc);
            break;
        case 8:
            /* Score */
            anScore[*pfTurn] = atoi(pc);
            break;
        case 9:
            /* Score */
            anScore[!*pfTurn] = atoi(pc);
            break;
        case 10:
            /* cube value */
            *pnCube = atoi(pc);
            break;
        case 11:
            /* cube owner */
            j = atoi(pc);
            if (j == 1)
                *pfCubeOwner = *pfTurn;
            else if (j == -1)
                *pfCubeOwner = !*pfTurn;
            else
                *pfCubeOwner = -1;
            break;
        case 12:
            /* chequers on bar for player on roll */
            anBoard[1][24] = abs(atoi(pc));
            break;
        case 37:
            /* chequers on bar for player opponent */
            anBoard[0][24] = abs(atoi(pc));
            break;
        case 38:
        case 39:
            /* dice rolled */
            anDice[c - 38] = atoi(pc);
            break;
        default:
            /* points */
            j = atoi(pc);
            anBoard[j < 0][(j < 0) ? 23 - i : i] = abs(j);
            ++i;

            break;
        }

        pc = NextTokenGeneral(&sz, ";");

        ++c;
        if (c == 40)
            break;

    }

    if (c != 40)
        return -1;

    return 0;
}

/*
 * Snowie .txt files
 *
 * The files are a single line with fields separated by
 * semicolons. Fields are numeric, except for player names.
 *
 * Field no    meaning
 * 0           length of match (0 in money games)
 * 1           1 if Jacoby enabled, 0 in match play or disabled
 * 2           Don't know, all samples had 0. Maybe it's Nack gammon or
 *            some other variant?
 * 3           Don't know. It was one in all money game samples, 0 in
 *            match samples
 * 4           Player on roll 0 = 1st player
 * 5,6         Player names
 * 7           1 = Crawford game
 * 8,9         Scores for player 0, 1
 * 10          Cube value
 * 11          Cube owner 1 = player on roll, 0 = centred, -1 opponent
 * 12          Chequers on bar for player on roll
 * 13..36      Points 1..24 from player on roll's point of view
 *            0 = empty, positive nos. for player on roll, negative
 *            for opponent
 * 37          Chequers on bar for opponent
 * 38.39       Current roll (0,0 if not rolled)
 *
 */

static int
ImportSnowieTxt(FILE * pf)
{
    char sz[2048];
    char *pc;
    int c;
    moverecord *pmr;
    moverecord *pmgi;

    int nMatchTo, fJacoby, fUnused1, fUnused2, fTurn, fCrawfordGame;
    int fCubeOwner, nCube;
    int anScore[2], anDice[2];
    TanBoard anBoard;
    char aszPlayer[2][MAX_NAME_LEN];

    if (!get_input_discard())
        return -1;

#if USE_GTK
    if (fX) {                   /* Clear record to avoid ugly updates */
        GTKClearMoveRecord();
        GTKFreeze();
    }
#endif

    /* 
     * Import Snowie .txt file 
     */

    /* read file into string */

    pc = sz;
    while ((c = fgetc(pf)) > -1) {
        *pc++ = (char) c;
        if ((pc - sz) == (sizeof(sz) - 2))
            break;
    }

    *pc++ = 0;

    /* parse string */

    if (ParseSnowieTxt(sz,
                       &nMatchTo, &fJacoby, &fUnused1, &fUnused2,
                       &fTurn, aszPlayer, &fCrawfordGame, anScore, &nCube, &fCubeOwner, anBoard, anDice) < 0) {
        outputl("This file is not a valid Snowie .txt file!");
        return -1;
    }

    FreeMatch();
    ClearMatch();

    InitBoard(ms.anBoard, ms.bgv);

    ClearMoveRecord();

    ListInsert(&lMatch, plGame);

    /* Game info */

    pmgi = NewMoveRecord();

    pmgi->mt = MOVE_GAMEINFO;
    pmgi->g.i = 0;
    pmgi->g.nMatch = nMatchTo;
    pmgi->g.anScore[0] = anScore[0];
    pmgi->g.anScore[1] = anScore[1];
    pmgi->g.fCrawford = TRUE;
    pmgi->g.fCrawfordGame = fCrawfordGame;
    pmgi->g.fJacoby = fJacoby;
    pmgi->g.fWinner = -1;
    pmgi->g.nPoints = 0;
    pmgi->g.fResigned = FALSE;
    pmgi->g.nAutoDoubles = 0;
    pmgi->g.bgv = VARIATION_STANDARD;   /* assume standard backgammon */
    pmgi->g.fCubeUse = TRUE;    /* assume use of cube */
    IniStatcontext(&pmgi->g.sc);

    AddMoveRecord(pmgi);

    ms.fTurn = ms.fMove = fTurn;

    for (c = 0; c < 2; ++c)
        strcpy(ap[c].szName, aszPlayer[c]);

    /* dice */

    if (anDice[0]) {
        pmr = NewMoveRecord();
        pmr->mt = MOVE_SETDICE;
        pmr->fPlayer = fTurn;
        pmr->anDice[0] = anDice[0];
        pmr->anDice[1] = anDice[1];
        AddMoveRecord(pmr);
    }

    /* board */

    pmr = NewMoveRecord();
    pmr->mt = MOVE_SETBOARD;
    if (!fTurn)
        SwapSides(anBoard);
    PositionKey((ConstTanBoard) anBoard, &pmr->sb.key);
    AddMoveRecord(pmr);

    /* cube value */

    pmr = NewMoveRecord();
    pmr->mt = MOVE_SETCUBEVAL;
    pmr->scv.nCube = nCube;
    AddMoveRecord(pmr);

    /* cube position */

    pmr = NewMoveRecord();
    pmr->mt = MOVE_SETCUBEPOS;
    pmr->scp.fCubeOwner = fCubeOwner;
    AddMoveRecord(pmr);

    /* update menus etc */

    UpdateSettings();

#if USE_GTK
    if (fX) {
        GTKThaw();
        GTKSet(ap);
    }
#endif

    return 0;
}

static int
ImportGAM(FILE * fp, char *UNUSED(szFilename))
{
    char *pch, *pchLeft, *pchRight, *szLine;
    moverecord *pmgi;
    int warned = -1;

#if USE_GTK
    if (fX) {                   /* Clear record to avoid ugly updates */
        GTKClearMoveRecord();
        GTKFreeze();
    }
#endif

    FreeMatch();
    ClearMatch();

    while ((szLine = GetMatLine(fp)) != 0) {
        szLine += strspn(szLine, " \t");

        if (!StrNCaseCmp(szLine, "win", 3))
            continue;           /* Skip this line (in between games) */

        /* Read player names */
        pchRight = szLine;
        while (*pchRight != ' ' && *pchRight != '\t')
            pchRight++;
        *pchRight = '\0';
        strcpy(ap[0].szName, szLine);

        pchLeft = pchRight + 1;
        while (*pchLeft == ' ' || *pchLeft == '\t')
            pchLeft++;
        pchRight = pchLeft;
        while (*pchRight != ' ' && *pchRight != '\t' && *pchRight != '\n')
            pchRight++;
        *pchRight = '\0';
        strcpy(ap[1].szName, pchLeft);

        InitBoard(ms.anBoard, ms.bgv);
        ClearMoveRecord();
        ListInsert(&lMatch, plGame);

        /* Game info */

        pmgi = NewMoveRecord();

        pmgi->mt = MOVE_GAMEINFO;
        pmgi->g.i = ms.cGames;
        pmgi->g.nMatch = 0;
        pmgi->g.anScore[0] = ms.anScore[0];
        pmgi->g.anScore[1] = ms.anScore[1];
        pmgi->g.fCrawford = FALSE;
        pmgi->g.fCrawfordGame = FALSE;
        pmgi->g.fJacoby = FALSE;
        pmgi->g.fWinner = -1;
        pmgi->g.nPoints = 0;
        pmgi->g.fResigned = FALSE;
        pmgi->g.nAutoDoubles = 0;
        pmgi->g.bgv = VARIATION_STANDARD;       /* assume standard backgammon */
        pmgi->g.fCubeUse = TRUE;        /* assume use of cube */
        IniStatcontext(&pmgi->g.sc);


        AddMoveRecord(pmgi);

        /* Read game */
        while ((szLine = GetMatLine(fp)) != 0) {
            pchRight = pchLeft = NULL;

            if ((pch = strpbrk(szLine, "\n\r")) != 0)
                *pch = 0;

            if ((pchLeft = strchr(szLine, ':')) && (pchRight = strchr(pchLeft + 1, ':')) && pchRight > szLine + 3)
                *((pchRight -= 2) - 1) = 0;
            else if (strlen(szLine) > 15 && (pchRight = strstr(szLine + 15, "  ")))
                *pchRight++ = 0;

            if ((pchLeft = strchr(szLine, ')')) != 0)
                pchLeft++;
            else
                pchLeft = szLine;

            ParseMatMove(pchLeft, 0, &warned);

            if (pchRight)
                ParseMatMove(pchRight, 1, &warned);

            if (ms.gs != GAME_PLAYING) {
                AddGame(pmgi);
                break;
            }
        }
    }

    UpdateSettings();

#if USE_GTK
    if (fX) {
        GTKThaw();
        GTKSet(ap);
    }
#endif

    return TRUE;
}

static void
WritePartyGame(FILE * fp, char *gameStr, int ns)
{
    char *move;
    char *data = gameStr;
    int moveNum = 1;
    int side = -1;
    while ((move = NextTokenGeneral(&data, "#")) != NULL) {
        char *roll, buf[100];
        char *moveStr = NULL;
        side = (move[0] == '2');
        if ((side == 0) || (moveNum == 1 && side == 1)) {
            fprintf(fp, "%3d) ", moveNum);
            if (moveNum == 1 && side == 1)
                fprintf(fp, "%28s", " ");
            moveNum++;
        }

        move = strchr(move, ' ') + 1;
        if (isdigit(*move)) {
            move = strchr(move, ' ') + 1;
            move = strchr(move, ' ') + 1;
            roll = move;
            move = strchr(move, ' ');
            if (move) {
                char *dest, *src;
                *move = '\0';
                moveStr = move + 1;
                /* Change bar -> 25, off -> 0 */
                src = dest = moveStr;
                while (*src) {
                    if (*src == 'b') {
                        *dest++ = '2';
                        *dest++ = '5';
                        src += 3;
                    } else if (*src == 'o') {
                        *dest++ = '0';
                        src += 3;
                    } else {
                        *dest = *src;
                        dest++, src++;
                    }
                }
                *dest = '\0';
            }
            sprintf(buf, "%s: %s", roll, moveStr ? moveStr : "");
        } else
            strcpy(buf, move);  /* Double/Take */

        if (side == 0)
            fprintf(fp, "%-30s ", buf);
        else
            fprintf(fp, "%s\n", buf);
    }
    if (side == 0 && ns < 0)
        fprintf(fp, "\n  ");
    else if (side == 1 && ns > 0)
        fprintf(fp, "%36s", " ");
    fprintf(fp, "Wins %d point%s\n\n", abs(ns), (abs(ns) == 1) ? "" : "s");
}

typedef struct _PartyGame {
    int s1, s2;
    char *gameStr;
} PartyGame;

static int
ConvertPartyGammonFileToMat(FILE * partyFP, FILE * matFP)
{
    PartyGame pg = { -1, -1, NULL };
    int matchLen = -1;
    char p1[MAX_NAME_LEN] = "", p2[MAX_NAME_LEN] = "";
    GList *games = NULL;
    char buffer[1024 * 10];
    while (fgets(buffer, sizeof(buffer), partyFP) != NULL) {
        char *value, *key;

        value = buffer;
        key = NextTokenGeneral(&value, "=");
        if (key) {
            if (!StrNCaseCmp(key, "GAME_", strlen("GAME_"))) {
                key += strlen("GAME_");
                do
                    key++;
                while (key[-1] != '_');
                if (g_ascii_iscntrl(value[strlen(value) - 1]))
                    value[strlen(value) - 1] = '\0';

                if (!StrCaseCmp(key, "PLAYER_1"))
                    strcpy(p1, value);
                else if (!StrCaseCmp(key, "PLAYER_2"))
                    strcpy(p2, value);
                else if (!StrCaseCmp(key, "GAMEPLAY")) {
                    pg.gameStr = (char *) malloc(strlen(value) + 1);
                    strcpy(pg.gameStr, value);
                } else if (!StrCaseCmp(key, "SCORE1"))
                    pg.s1 = atoi(value);
                else if (!StrCaseCmp(key, "SCORE2")) {
                    PartyGame *newGame;
                    pg.s2 = atoi(value);
                    /* Add this game */
                    newGame = malloc(sizeof(PartyGame));
                    memcpy(newGame, &pg, sizeof(PartyGame));
                    games = g_list_append(games, newGame);
                }
            }
            if (!StrCaseCmp(key, "MATCHLENGTH"))
                matchLen = atoi(value);
        }
    }

    if (g_list_length(games) > 0 || matchLen == -1) {   /* Write out mat file */
        unsigned int i;
        int s1 = 0, s2 = 0;
        GList *pl;

        fprintf(matFP, " %d point match\n", matchLen);
        for (i = 0, pl = g_list_first(games); i < g_list_length(games); i++, pl = g_list_next(pl)) {
            int pts;
            PartyGame *pGame = (PartyGame *) (pl->data);
            fprintf(matFP, "\n Game %d\n", i + 1);
            fprintf(matFP, " %s : %d %14s %s : %d\n", p1, s1, " ", p2, s2);
            pts = pGame->s2 - s2 - pGame->s1 + s1;
            WritePartyGame(matFP, pGame->gameStr, pts);
            s1 = pGame->s1;
            s2 = pGame->s2;
            free(pGame->gameStr);
            free(pGame);
        }
        fclose(matFP);
        fclose(partyFP);
        g_list_free(pl);
        return TRUE;
    }
    if (ferror(partyFP))
        outputerr("tomat");
    fclose(partyFP);
    return FALSE;
}

extern void
CommandImportJF(char *sz)
{
    FILE *pf;
    int rc;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a position file to import (see `help " "import pos')."));
        return;
    }

    if ((pf = g_fopen(sz, "rb"))) {
        rc = ImportJF(pf, sz);
        if (rc)
            /* no file imported */
            return;
        fclose(pf);
        setDefaultFileName(sz);
    } else
        outputerr(sz);

    ShowBoard();
}

extern void
CommandImportMat(char *sz)
{
    FILE *pf;
    int rc;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a match file to import (see `help " "import mat')."));
        return;
    }

    if ((pf = g_fopen(sz, "r")) != 0) {
        rc = ImportMat(pf, sz);
        fclose(pf);
        if (rc)
            /* no file imported */
            return;
        setDefaultFileName(sz);
        if (fGotoFirstGame)
            CommandFirstGame(NULL);
    } else
        outputerr(sz);
}

extern void
CommandImportOldmoves(char *sz)
{
    FILE *pf;
    int rc;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify an oldmoves file to import (see `help " "import oldmoves')."));
        return;
    }

    if ((pf = g_fopen(sz, "r")) != 0) {
        rc = ImportOldmoves(pf, sz);
        fclose(pf);
        if (rc)
            /* no file imported */
            return;
        setDefaultFileName(sz);
        if (fGotoFirstGame)
            CommandFirstGame(NULL);
    } else
        outputerr(sz);
}


extern void
CommandImportSGG(char *sz)
{
    FILE *pf;
    int rc;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify an SGG file to import (see `help " "import sgg')."));
        return;
    }

    if ((pf = g_fopen(sz, "r")) != 0) {
        rc = ImportSGG(pf, sz);
        fclose(pf);
        if (rc)
            /* no file imported */
            return;
        setDefaultFileName(sz);
        if (fGotoFirstGame)
            CommandFirstGame(NULL);
    } else
        outputerr(sz);
}

extern void
CommandImportTMG(char *sz)
{
    FILE *pf;
    int rc;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify an TMG file to import (see `help " "import tmg')."));
        return;
    }

    if ((pf = g_fopen(sz, "r")) != 0) {
        rc = ImportTMG(pf, sz);
        fclose(pf);
        if (rc)
            /* no file imported */
            return;
        setDefaultFileName(sz);
        if (fGotoFirstGame)
            CommandFirstGame(NULL);
    } else
        outputerr(sz);
}

extern void
CommandImportSnowieTxt(char *sz)
{

    FILE *pf;
    int rc;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a Snowie Text file to import (see `help " "import snowietxt')."));
        return;
    }

    if ((pf = g_fopen(sz, "r")) != 0) {
        rc = ImportSnowieTxt(pf);
        fclose(pf);
        if (rc)
            /* no file imported */
            return;
        setDefaultFileName(sz);
    } else
        outputerr(sz);
}

extern void
CommandImportEmpire(char *sz)
{
    FILE *pf;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a GammonEmpire file to import (see `help " "import empire')."));
        return;
    }

    if ((pf = g_fopen(sz, "r")) != 0) {
        int res = ImportGAM(pf, sz);
        fclose(pf);
        if (!res)
            /* no file imported */
            return;
        setDefaultFileName(sz);
    } else
        outputerr(sz);
}

extern void
CommandImportParty(char *sz)
{
    FILE *gamf, *matf, *pf;
    char *tmpfile;
    int rc;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a PartyGammon file to import (see `help " "import party')."));
        return;
    }

    if ((gamf = g_fopen(sz, "r")) == 0) {
        outputerr(sz);
        return;
    }

    tmpfile = g_strdup_printf("%s.mat", sz);
    if (g_file_test(tmpfile, G_FILE_TEST_EXISTS)) {
        outputerrf("%s is in the way. Cannot import %s\n", tmpfile, sz);
        g_free(tmpfile);
        fclose(gamf);
        return;
    }

    if ((matf = g_fopen(tmpfile, "w")) == 0) {
        outputerr(tmpfile);
        g_free(tmpfile);
        fclose(gamf);
        return;
    }

    if (ConvertPartyGammonFileToMat(gamf, matf)) {
        if ((pf = g_fopen(tmpfile, "r")) != 0) {
            rc = ImportMat(pf, tmpfile);
            fclose(pf);
            if (!rc) {
                setDefaultFileName(tmpfile);
                if (fGotoFirstGame)
                    CommandFirstGame(NULL);
            }
        } else
            outputerr(tmpfile);
    } else
        outputerrf("Failed to convert gam to mat\n");
    g_unlink(tmpfile);
    g_free(tmpfile);
}

extern void
CommandImportAuto(char *sz)
{
    FilePreviewData *fdp;
    gchar *cmd;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputerrf(_("You must specify a file to import (see `help " "import auto')."));
        return;
    }
    if (!g_file_test(sz, G_FILE_TEST_EXISTS)) {
        outputerrf(_("The file `%s' doesn't exist"), sz);
        return;
    }
    fdp = ReadFilePreview(sz);
    if (!fdp) {
        outputerrf(_("`%s' is not a backgammon file"), sz);
        g_free(fdp);
        return;
    } else if (fdp->type == N_IMPORT_TYPES) {
        outputf(_("The format of '%s' is not recognized"), sz);
        g_free(fdp);
        return;
    } else if (fdp->type == IMPORT_SGF)
        cmd = g_strdup_printf("load match \"%s\"", sz);
    else
        cmd = g_strdup_printf("import %s \"%s\"", import_format[fdp->type].clname, sz);

    HandleCommand(cmd, acTop);
    g_free(cmd);
    g_free(fdp);
}

#define BGR_STRING "BGF version"
static int moveNum;

static void
OutputMove(FILE * fpOut, int side, const char *outBuf)
{
    if ((side == 0) || (moveNum == 1)) {
        fprintf(fpOut, "%3d) ", moveNum);
        if (side == 1)
            fprintf(fpOut, "%28s", " ");
        moveNum++;
    }
    if (side == 0)
        fprintf(fpOut, "%-27s ", outBuf);
    else
        fprintf(fpOut, "%s\n", outBuf);
}

static int
ConvertBGRoomFileToMat(FILE * bgrFP, FILE * matFP)
{

    char *player1 = NULL;
    char *player2 = NULL;
    int p1Score = 0, p2Score = 0;
    int dice1, dice2;
    int doubled, stake, side = 0;
    int gameCount = 0, moveCount;
    char buffer[1024 * 4];
    player1 = malloc(sizeof(char) * 128);
    player2 = malloc(sizeof(char) * 128);

    while (fgets(buffer, sizeof(buffer), bgrFP) != NULL) {
        if (strncmp(buffer, BGR_STRING, strlen(BGR_STRING)) == 0)
            break;
    };

    if (ferror(bgrFP)) {
        outputerr("Error opening file");
        free(player2);
        free(player1);
        return FALSE;
    }

    if (feof(bgrFP)) {
        free(player2);
        free(player1);
        return FALSE;
    }

    while (!feof(bgrFP)) {
        char *value, *ptr, *tempptr;
        gboolean fSwap;

        do {
            if (fgets(buffer, sizeof(buffer), bgrFP) == NULL) {
                if (ferror(bgrFP))
                    outputerr("Error reading file");
                free(player2);
                free(player1);
                return FALSE;
            }
            if (strstr(buffer, "Win the Match"))
                goto done;
        } while (strncmp(buffer, "Game ", strlen("Game ")));

        value = buffer + strlen("Game ");
        gameCount++;
        ptr = NextTokenGeneral(&value, " ");
        if (atoi(ptr) != gameCount) {
            outputerrf("Error parsing file. Wrong game count: expected %d, got %s", gameCount, ptr);
            g_assert_not_reached();
        }

        ptr = NextTokenGeneral(&value, " ");    /* Skip '-' */
        if (strcmp(ptr, "-")) {
            outputerrf("Error parsing file. Expected '-', got '%s'", ptr);
            g_assert_not_reached();
        }
        strcpy(player1, NextTokenGeneral(&value, " "));
        ptr = NextTokenGeneral(&value, " ");    /* Skip '(0|X)' */
        fSwap = *(ptr + 1) == '0';
        ptr = NextTokenGeneral(&value, " ");    /* Skip 'vs.' */
        if (strcmp(ptr, "vs.")) {
            outputerrf("Error parsing file. Expected 'vs.', got '%s'", ptr);
            g_assert_not_reached();
        }
        strcpy(player2, NextTokenGeneral(&value, " "));

        if (fSwap) {
            tempptr = player1;
            player1 = player2;
            player2 = tempptr;
        }

        if (fgets(buffer, sizeof(buffer), bgrFP) == NULL) {
            if (ferror(bgrFP))
                outputerr("Error reading file");
            free(player2);
            free(player1);
            return FALSE;
        }

        value = buffer;
        ptr = NextTokenGeneral(&value, " ");
        if (strcmp(ptr, "Single")) {
            outputerrf("Error parsing file. Expected 'Single', got '%s'", ptr);
            g_assert_not_reached();
        }

        fprintf(matFP, " 0 point match\n\n Game %d\n %s : %-22d %s : %d\n",
                gameCount, player1, p1Score, player2, p2Score);

        /* Game Moves */
        moveCount = 0;
        moveNum = 1;
        doubled = FALSE;
        stake = 1;
        while (!feof(bgrFP)) {
            char outBuf[100];
            if (fgets(buffer, sizeof(buffer), bgrFP) == NULL) {
                if (ferror(bgrFP))
                    outputerr("Error reading file");
                free(player2);
                free(player1);
                return FALSE;
            }
            while (buffer[strlen(buffer) - 1] == '\n' || buffer[strlen(buffer) - 1] == '\r')
                buffer[strlen(buffer) - 1] = '\0';
            if (!*buffer) {
                if (doubled) {  /* Drop */
                    side = !side;
                    OutputMove(matFP, side, " Drops");
                }
                fprintf(matFP, "      Wins %d points\n", stake);
                break;          /* end of game */
            }

            moveCount++;
            if (doubled) {      /* Taken */
                stake *= 2;
                side = !side;
                doubled = FALSE;
                OutputMove(matFP, side, " Takes");
            }

            value = buffer;
            ptr = NextTokenGeneral(&value, ".");
            if (atoi(ptr) != moveCount) {
                outputerrf("Error parsing file. Wrong move count: expected %d, got %s", moveCount, ptr);
                g_assert_not_reached();
            }

            ptr = NextTokenGeneral(&value, ":");
            if (!strcmp(ptr, "X"))
                side = 0;
            else if (!strcmp(ptr, "0"))
                side = 1;
            else {
                printf("Unrecognised data in file: %s", ptr);
                continue;
            }

            if (!strcmp(value, "Double")) {
                doubled = TRUE;
                sprintf(outBuf, " Doubles => %d", stake * 2);
            } else {
                g_assert(*value == '(');
                value++;
                ptr = NextTokenGeneral(&value, " ");
                dice1 = atoi(ptr);
                ptr = NextTokenGeneral(&value, ")");
                dice2 = atoi(ptr);

                if (!strcmp(value, "can't move"))
                    value[0] = '\0';

                sprintf(outBuf, "%d%d: %s", dice1, dice2, value);
            }
            OutputMove(matFP, side, outBuf);
        }
    }
  done:
    fclose(bgrFP);
    fclose(matFP);
    free(player2);
    free(player1);

    return TRUE;
}

extern void
CommandImportBGRoom(char *sz)
{
    FILE *gamf, *matf, *pf;
    char *matfile;
    int rc;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a BGRoom file to import (see `help " "import bgroom')."));
        return;
    }

    if ((gamf = g_fopen(sz, "r")) == 0) {
        outputerr(sz);
        return;
    }

    matfile = g_strdup_printf("%s.mat", sz);
    if ((matf = g_fopen(matfile, "w")) == 0) {
        outputerr(matfile);
        g_free(matfile);
        fclose(gamf);
        return;
    }

    if (ConvertBGRoomFileToMat(gamf, matf)) {
        if ((pf = g_fopen(matfile, "r")) != 0) {
            rc = ImportMat(pf, matfile);
            fclose(pf);
            if (!rc) {
                setDefaultFileName(matfile);
                if (fGotoFirstGame)
                    CommandFirstGame(NULL);
            }
        } else
            outputerr(matfile);
    } else
        outputerrf("Failed to convert gam to mat\n");

    g_unlink(matfile);
    g_free(matfile);
}
