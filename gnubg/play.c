/*
 * play.c
 *
 * by Gary Wong <gtw@gnu.org>, 1999, 2000, 2001, 2002.
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
 * $Id: play.c,v 1.434 2014/12/04 14:51:22 mdpetch Exp $
 */

#include "config.h"

#include <glib.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "analysis.h"
#include "backgammon.h"
#include "dice.h"
#include "drawboard.h"
#include "external.h"
#include "eval.h"
#include "positionid.h"
#include "matchid.h"
#include "matchequity.h"
#include "sound.h"
#include "renderprefs.h"
#include "md5.h"
#if USE_GTK
#include "gtkboard.h"
#include "gtkwindows.h"
#endif
#if USE_BOARD3D
#include "fun3d.h"
#endif

static moverecord *pmr_hint = NULL;

const char *aszGameResult[] = {
    N_("single game"),
    N_("gammon"),
    N_("backgammon")
};

const char *aszSkillType[] = {
    N_("very bad"),
    N_("bad"),
    N_("doubtful"),
    NULL,
};

const char *aszSkillTypeCommand[] = {
    "verybad",
    "bad",
    "doubtful",
    "none",
};
const char *aszSkillTypeAbbr[] = { "??", "? ", "?!", "  ", "  " };

const char *aszLuckTypeCommand[] = {
    "veryunlucky",
    "unlucky",
    "none",
    "lucky",
    "verylucky"
};

const char *aszLuckType[] = {
    N_("very unlucky"),
    N_("unlucky"),
    NULL,
    N_("lucky"),
    N_("very lucky")
};
const char *aszLuckTypeAbbr[] = { "--", "-", "", "+", "++" };

listOLD lMatch, *plGame, *plLastMove;
statcontext scMatch;
static int fComputerDecision = FALSE;
static int fEndGame = FALSE;
int automaticTask = FALSE;

typedef enum _annotatetype {
    ANNOTATE_ACCEPT, ANNOTATE_CUBE, ANNOTATE_DOUBLE, ANNOTATE_DROP,
    ANNOTATE_MOVE, ANNOTATE_ROLL, ANNOTATE_RESIGN, ANNOTATE_REJECT
} annotatetype;

static annotatetype at;

extern moverecord *
NewMoveRecord(void)
{

    moverecord *pmr = malloc(sizeof *pmr);

    if (!pmr) {
        outputerr("NewMoveRecord");
        g_assert_not_reached();
        return NULL;
    }

    memset(pmr, 0, sizeof *pmr);

    pmr->mt = -1;
    pmr->sz = NULL;
    pmr->fPlayer = 0;
    pmr->anDice[0] = pmr->anDice[1] = 0;
    pmr->lt = LUCK_NONE;
    pmr->rLuck = (float) ERR_VAL;
    pmr->esChequer.et = EVAL_NONE;
    pmr->nAnimals = 0;
    pmr->CubeDecPtr = &pmr->CubeDec;
    pmr->CubeDecPtr->esDouble.et = EVAL_NONE;
    pmr->CubeDecPtr->cmark = CMARK_NONE;
    pmr->stCube = SKILL_NONE;
    pmr->ml.cMoves = 0;
    pmr->ml.amMoves = NULL;

    /* movenormal */
    pmr->n.stMove = SKILL_NONE;
    pmr->n.anMove[0] = pmr->n.anMove[1] = -1;
    pmr->n.iMove = UINT_MAX;

    /* moveresign */
    pmr->r.esResign.et = EVAL_NONE;


    return pmr;

}

static int
 CheatDice(unsigned int anDice[2], matchstate * pms, const int fBest);


#if USE_GTK
#include "gtkgame.h"

static int anLastMove[8], fLastMove, fLastPlayer;
#endif

static void
PlayMove(matchstate * pms, int const anMove[8], int const fPlayer)
{
    int i, nSrc, nDest;

#if USE_GTK
    if (pms == &ms) {
        memcpy(anLastMove, anMove, sizeof anLastMove);
        CanonicalMoveOrder(anLastMove);
        fLastPlayer = fPlayer;
        fLastMove = anMove[0] >= 0;
    }
#endif

    if (pms->fMove != -1 && fPlayer != pms->fMove)
        SwapSides(pms->anBoard);

    for (i = 0; i < 8; i += 2) {
        nSrc = anMove[i];
        nDest = anMove[i | 1];

        if (nSrc < 0)
            /* move is finished */
            break;

        if (!pms->anBoard[1][nSrc])
            /* source point is empty; ignore */
            continue;

        pms->anBoard[1][nSrc]--;
        if (nDest >= 0)
            pms->anBoard[1][nDest]++;

        if (nDest >= 0 && nDest <= 23) {
            pms->anBoard[0][24] += pms->anBoard[0][23 - nDest];
            pms->anBoard[0][23 - nDest] = 0;
        }
    }

    pms->fMove = pms->fTurn = !fPlayer;
    SwapSides(pms->anBoard);
}

static void
ApplyGameOver(matchstate * pms, const listOLD * plGame)
{
    moverecord *pmr = (moverecord *) plGame->plNext->p;
    xmovegameinfo *pmgi = &pmr->g;

    g_assert(pmr->mt == MOVE_GAMEINFO);

    if (pmgi->fWinner < 0)
        return;

    pms->anScore[pmgi->fWinner] += pmgi->nPoints;
    pms->cGames++;
    if (fEndGame)
        outputf(ngettext("End Game done.\n%s wins %d point\n", "End Game done.\n%s wins %d points\n", pmgi->nPoints),
                ap[pmgi->fWinner].szName, pmgi->nPoints);
}

extern void
ApplyMoveRecord(matchstate * pms, const listOLD * plGame, const moverecord * pmr)
{
    int n;
    moverecord *pmrx = (moverecord *) plGame->plNext->p;
    xmovegameinfo *pmgi;

    if (!pmr) {
        g_assert_not_reached();
        outputerr("Applying null move record!");
        return;
    }

    /* FIXME this is wrong -- plGame is not necessarily the right game */

    g_assert(pmr->mt == MOVE_GAMEINFO || (pmrx && pmrx->mt == MOVE_GAMEINFO));

    pms->gs = GAME_PLAYING;
    pms->fResigned = pms->fResignationDeclined = 0;

#if USE_GTK
    if (pms == &ms)
        fLastMove = FALSE;
#endif

    switch (pmr->mt) {
    case MOVE_GAMEINFO:
        InitBoard(pms->anBoard, pmr->g.bgv);

        pms->nMatchTo = pmr->g.nMatch;
        pms->anScore[0] = pmr->g.anScore[0];
        pms->anScore[1] = pmr->g.anScore[1];
        pms->cGames = pmr->g.i;

        pms->gs = GAME_NONE;
        pms->fMove = pms->fTurn = pms->fCubeOwner = -1;
        pms->anDice[0] = pms->anDice[1] = pms->cBeavers = 0;
        pms->fDoubled = FALSE;
        pms->nCube = 1 << pmr->g.nAutoDoubles;
        pms->fCrawford = pmr->g.fCrawfordGame;
        pms->fPostCrawford = !pms->fCrawford &&
            (pms->anScore[0] == pms->nMatchTo - 1 || pms->anScore[1] == pms->nMatchTo - 1);
        pms->bgv = pmr->g.bgv;
        pms->fCubeUse = pmr->g.fCubeUse;
        pms->fJacoby = pmr->g.fJacoby;
        break;

    case MOVE_DOUBLE:

        if (pms->fMove < 0)
            pms->fMove = pmr->fPlayer;

        if (pms->nCube >= MAX_CUBE)
            break;

        if (pms->fDoubled) {
            pms->cBeavers++;
            pms->nCube <<= 1;
            pms->fCubeOwner = !pms->fMove;
        } else
            pms->fDoubled = TRUE;

        pms->fTurn = !pmr->fPlayer;

        break;

    case MOVE_TAKE:
        if (!pms->fDoubled)
            break;

        pms->nCube <<= 1;
        pms->cBeavers = 0;
        pms->fDoubled = FALSE;
        pms->fCubeOwner = !pms->fMove;
        pms->fTurn = pms->fMove;
        break;

    case MOVE_DROP:
        if (!pms->fDoubled)
            break;

        pms->fDoubled = FALSE;
        pms->cBeavers = 0;
        pms->gs = GAME_DROP;
        pmgi = &pmrx->g;
        pmgi->nPoints = pms->nCube;
        pmgi->fWinner = !pmr->fPlayer;
        pmgi->fResigned = FALSE;

        ApplyGameOver(pms, plGame);
        break;

    case MOVE_NORMAL:
        pms->fDoubled = FALSE;

        PlayMove(pms, pmr->n.anMove, pmr->fPlayer);
        pms->anDice[0] = pms->anDice[1] = 0;

        if ((n = GameStatus((ConstTanBoard) pms->anBoard, pms->bgv))) {

            if (pms->fJacoby && pms->fCubeOwner == -1 && !pms->nMatchTo)
                /* gammons do not count on a centred cube during money
                 * sessions under the Jacoby rule */
                n = 1;

            pms->gs = GAME_OVER;
            pmgi = &pmrx->g;
            pmgi->nPoints = pms->nCube * n;
            pmgi->fWinner = pmr->fPlayer;
            pmgi->fResigned = FALSE;
            ApplyGameOver(pms, plGame);
        }

        break;

    case MOVE_RESIGN:
        pms->gs = GAME_RESIGNED;
        pmgi = &pmrx->g;
        pmgi->nPoints = pms->nCube * (pms->fResigned = pmr->r.nResigned);
        pmgi->fWinner = !pmr->fPlayer;
        pmgi->fResigned = TRUE;

        ApplyGameOver(pms, plGame);
        break;

    case MOVE_SETBOARD:
        PositionFromKey(pms->anBoard, &pmr->sb.key);

        if (pms->fMove < 0)
            pms->fTurn = pms->fMove = 0;

        if (pms->fMove)
            SwapSides(pms->anBoard);

        break;

    case MOVE_SETDICE:
        pms->anDice[0] = pmr->anDice[0];
        pms->anDice[1] = pmr->anDice[1];
        if (pms->fMove != pmr->fPlayer)
            SwapSides(pms->anBoard);
        pms->fTurn = pms->fMove = pmr->fPlayer;
        pms->fDoubled = FALSE;
        break;

    case MOVE_SETCUBEVAL:
        if (pms->fMove < 0)
            pms->fMove = 0;

        pms->nCube = pmr->scv.nCube;
        pms->fDoubled = FALSE;
        pms->fTurn = pms->fMove;
        break;

    case MOVE_SETCUBEPOS:
        if (pms->fMove < 0)
            pms->fMove = 0;

        pms->fCubeOwner = pmr->scp.fCubeOwner;
        pms->fDoubled = FALSE;
        pms->fTurn = pms->fMove;
        break;
    }

}

extern void
CalculateBoard(void)
{
    listOLD *pl;

    pl = plGame;
    do {
        pl = pl->plNext;

        if (!pl) {
            g_assert_not_reached();
            break;
        }

        ApplyMoveRecord(&ms, plGame, pl->p);

        if (pl->plNext && pl->plNext->p)
            FixMatchState(&ms, pl->plNext->p);


    } while (pl != plLastMove);

#if USE_BOARD3D
    RestrictiveRedraw();
#endif
}

static void
FreeMoveRecord(moverecord * pmr)
{
    if (!pmr)
        return;

    switch (pmr->mt) {
    case MOVE_NORMAL:
        if (pmr->ml.cMoves && pmr->ml.amMoves) {
            free(pmr->ml.amMoves);
            pmr->ml.amMoves = NULL;
        }
        break;

    default:
        break;
    }

    if (pmr->sz)
        free(pmr->sz);

    free(pmr);
}

static void
FreeGame(listOLD * pl)
{

    while (pl->plNext != pl) {
        FreeMoveRecord(pl->plNext->p);
        ListDelete(pl->plNext);
    }

    free(pl);
}

static int
PopGame(listOLD * plDelete, int fInclusive)
{

    listOLD *pl;
    int i;

    for (i = 0, pl = lMatch.plNext; pl != &lMatch && pl->p != plDelete; pl = pl->plNext, i++);

    if (pl->p && !fInclusive) {
        pl = pl->plNext;
        i++;
    }

    if (!pl->p)
        /* couldn't find node to delete to */
        return -1;

#if USE_GTK
    if (fX)
        GTKPopGame(i);
#endif

    do {
        pl = pl->plNext;
        FreeGame(pl->plPrev->p);
        ListDelete(pl->plPrev);
    } while (pl->p);

    pmr_hint_destroy();
    return 0;
}

static int
PopMoveRecord(listOLD * plDelete)
{

    listOLD *pl;

    for (pl = plGame->plNext; pl != plGame && pl != plDelete; pl = pl->plNext);

    if (pl == plGame)
        /* couldn't find node to delete to */
        return -1;

#if USE_GTK
    if (fX) {
        GTKPopMoveRecord(pl->p);
        GTKSetMoveRecord(pl->plPrev->p);
    }
#endif

    pl = pl->plPrev;

    while (pl->plNext->p) {
        if (pl->plNext == plLastMove)
            plLastMove = pl;
        FreeMoveRecord(pl->plNext->p);
        ListDelete(pl->plNext);
    }
    pmr_hint_destroy();

    return 0;
}

static void
copy_from_pmr_cur(moverecord * pmr, gboolean get_move, gboolean get_cube)
{
    moverecord *pmr_cur;
    pmr_cur = get_current_moverecord(NULL);
    if (!pmr_cur)
        return;
    if (get_move && pmr_cur->ml.cMoves > 0) {
        if (pmr->ml.cMoves > 0)
            free(pmr->ml.amMoves);
        CopyMoveList(&pmr->ml, &pmr_cur->ml);
        pmr->n.iMove = locateMove(msBoard(), pmr->n.anMove, &pmr->ml);
    }

    if (get_cube && pmr_cur->CubeDecPtr->esDouble.et != EVAL_NONE) {
        memcpy(pmr->CubeDecPtr->aarOutput, pmr_cur->CubeDecPtr->aarOutput, sizeof pmr->CubeDecPtr->aarOutput);
        memcpy(pmr->CubeDecPtr->aarStdDev, pmr_cur->CubeDecPtr->aarStdDev, sizeof pmr->CubeDecPtr->aarStdDev);
        memcpy(&pmr->CubeDecPtr->esDouble, &pmr_cur->CubeDecPtr->esDouble, sizeof pmr->CubeDecPtr->esDouble);
        pmr->CubeDecPtr->cmark = pmr_cur->CubeDecPtr->cmark;
    }

}

static void
add_moverecord_get_cur(moverecord * pmr)
{
    switch (pmr->mt) {
    case MOVE_NORMAL:
    case MOVE_RESIGN:
        copy_from_pmr_cur(pmr, TRUE, TRUE);
        pmr_hint_destroy();
        find_skills(pmr, &ms, FALSE, -1);
        break;
    case MOVE_DOUBLE:
        copy_from_pmr_cur(pmr, FALSE, TRUE);
        find_skills(pmr, &ms, TRUE, -1);
        break;
    case MOVE_TAKE:
        copy_from_pmr_cur(pmr, FALSE, TRUE);
        pmr_hint_destroy();
        find_skills(pmr, &ms, -1, TRUE);
        break;
    case MOVE_DROP:
        copy_from_pmr_cur(pmr, FALSE, TRUE);
        pmr_hint_destroy();
        find_skills(pmr, &ms, -1, FALSE);
        break;
    case MOVE_SETDICE:
        copy_from_pmr_cur(pmr, FALSE, TRUE);
        find_skills(pmr, &ms, FALSE, -1);
        break;
    default:
        pmr_hint_destroy();
        break;
    }
}

static void
add_moverecord_sanity_check(moverecord * pmr)
{
    g_assert(pmr->fPlayer >= 0 && pmr->fPlayer <= 1);
    g_assert(pmr->ml.cMoves < MAX_MOVES);
    switch (pmr->mt) {
    case MOVE_GAMEINFO:
        g_assert(pmr->g.nMatch >= 0);
        g_assert(pmr->g.i >= 0);
        if (pmr->g.nMatch) {
            g_assert(pmr->g.i <= pmr->g.nMatch * 2 + 1);
            g_assert(pmr->g.anScore[0] < pmr->g.nMatch);
            g_assert(pmr->g.anScore[1] < pmr->g.nMatch);
        }
        if (!pmr->g.fCrawford)
            g_assert(!pmr->g.fCrawfordGame);

        break;

    case MOVE_NORMAL:
        if (pmr->ml.cMoves)
            g_assert(pmr->n.iMove <= pmr->ml.cMoves);

        break;

    case MOVE_RESIGN:
        g_assert(pmr->r.nResigned >= 1 && pmr->r.nResigned <= 3);
        break;

    case MOVE_DOUBLE:
    case MOVE_TAKE:
    case MOVE_DROP:
    case MOVE_SETDICE:
    case MOVE_SETBOARD:
    case MOVE_SETCUBEVAL:
    case MOVE_SETCUBEPOS:
        break;

    default:
        g_assert_not_reached();
    }
}

extern void
AddMoveRecord(void *pv)
{
    moverecord *pmr = pv, *pmrOld;

    add_moverecord_get_cur(pmr);

    add_moverecord_sanity_check(pmr);


    /* Delete all games after plGame, and all records after plLastMove. */
    PopGame(plGame, FALSE);
    /* FIXME when we can handle variations, we should save the old moves
     * as an alternate variation. */
    PopMoveRecord(plLastMove->plNext);

    if (pmr->mt == MOVE_NORMAL && (pmrOld = plLastMove->p)->mt == MOVE_SETDICE && pmrOld->fPlayer == pmr->fPlayer) {
        PopMoveRecord(plLastMove);
    }

    /* FIXME perform other elision (e.g. consecutive "set" records) */

    /* Partial fix. For edited positions it would be nice to look
     * further back, but this is still an improvement */

    if (pmr->mt >= MOVE_SETBOARD && (pmrOld = plLastMove->p)->mt == pmr->mt && pmrOld->fPlayer == pmr->fPlayer) {
        PopMoveRecord(plLastMove);
    }
#if USE_GTK
    if (fX)
        GTKAddMoveRecord(pmr);
#endif

    FixMatchState(&ms, pmr);

    ApplyMoveRecord(&ms, plGame, pmr);

    plLastMove = ListInsert(plGame, pmr);

    SetMoveRecord(pmr);
}

static gboolean
move_is_last_in_match(void)
{
    return (!plLastMove || !plLastMove->plNext || !plLastMove->plNext->p);
}

static gboolean
move_not_last_in_match_ok(void)
{
    if (automaticTask)
        return TRUE;
    if (move_is_last_in_match())
        return TRUE;

    return GetInputYN(_("The current move is not the last in the match.\n"
                        "Continuing will destroy the remainder of the match. Continue?"));
}

extern void
SetMoveRecord(void *pv)
{

#if USE_GTK
    if (fX)
        GTKSetMoveRecord(pv);
#else
    (void) pv;                  /* suppress unused parameter compiler warning */
#endif
}

extern void
ClearMoveRecord(void)
{

#if USE_GTK
    if (fX)
        GTKClearMoveRecord();
#endif

    plLastMove = plGame = malloc(sizeof(*plGame));
    ListCreate(plGame);
}

#if USE_GTK
static guint nTimeout, fDelaying;

static gint
DelayTimeout(gpointer UNUSED(p))
{

    if (fDelaying)
        fEndDelay = TRUE;

    nTimeout = 0;

    return FALSE;
}

static void
ResetDelayTimer(void)
{

    if (fX && nDelay && fDisplay) {
        if (nTimeout)
            g_source_remove(nTimeout);

        nTimeout = g_timeout_add(nDelay, DelayTimeout, NULL);
    }
}
#else
#define ResetDelayTimer()
#endif

extern void
AddGame(moverecord * pmr)
{
    g_assert(pmr->mt == MOVE_GAMEINFO);

#if USE_GTK
    if (fX)
        GTKAddGame(pmr);
#else
    (void) pmr;                 /* silence compiler warning */
#endif
}

static void
DiceRolled(void)
{
    playSound(SOUND_ROLL);

#if USE_GTK
    if (fX) {
        BoardData *bd = BOARD(pwBoard)->board_data;
        /* Make sure dice are updated */
        bd->diceRoll[0] = 0;
        bd->diceShown = DICE_ROLLING;
        ShowBoard();
    } else
#endif
    if (fDisplay)
        ShowBoard();
}

static int
NewGame(void)
{
    moverecord *pmr;
    listOLD *plLastMove_store = plLastMove;
    listOLD *plGame_store = lMatch.plNext->p;
    int fError;

    if (!fRecord && !ms.nMatchTo && lMatch.plNext->p) {
        /* only recording the active game of a session; discard any others */

        if (!get_input_discard())
            return -1;

        PopGame(lMatch.plNext->p, TRUE);
    }

    InitBoard(ms.anBoard, ms.bgv);

    ClearMoveRecord();

    ListInsert(&lMatch, plGame);

    pmr = NewMoveRecord();
    pmr->mt = MOVE_GAMEINFO;
    pmr->sz = NULL;

    pmr->g.i = ms.cGames;
    pmr->g.nMatch = ms.nMatchTo;
    pmr->g.anScore[0] = ms.anScore[0];
    pmr->g.anScore[1] = ms.anScore[1];
    pmr->g.fCrawford = fAutoCrawford && ms.nMatchTo > 1;
    pmr->g.fCrawfordGame = ms.fCrawford;
    pmr->g.fJacoby = ms.fJacoby && !ms.nMatchTo;
    pmr->g.fWinner = -1;
    pmr->g.nPoints = 0;
    pmr->g.fResigned = FALSE;
    pmr->g.nAutoDoubles = 0;
    pmr->g.bgv = ms.bgv;
    pmr->g.fCubeUse = ms.fCubeUse;
    IniStatcontext(&pmr->g.sc);
    AddMoveRecord(pmr);


    UpdateSetting(&ms.nCube);
    UpdateSetting(&ms.fCubeOwner);
    UpdateSetting(&ms.fTurn);

#if USE_BOARD3D
    if (fX) {
        BoardData *bd = BOARD(pwBoard)->board_data;
        InitBoardData(bd);
    }
#endif

    AddGame(pmr);

  reroll:
    fError = RollDice(ms.anDice, &rngCurrent, rngctxCurrent);

    if (fInterrupt || fError) {
        PopGame(plGame, TRUE);
        plGame = plGame_store;
        plLastMove = plLastMove_store;
        if (!plGame)
            return -1;
        ChangeGame(plGame);
        if (!plLastMove)
            return -1;
        CalculateBoard();
        SetMoveRecord(plLastMove->p);
        ShowBoard();
        return -1;
    }

    if (fDisplay) {
        outputnew();
        outputf(_("%s rolls %d, %s rolls %d.\n"), ap[0].szName, ms.anDice[0], ap[1].szName, ms.anDice[1]);
    }

    if (ms.anDice[0] == ms.anDice[1]) {
        if (!ms.nMatchTo && ms.nCube < (1 << cAutoDoubles) && ms.fCubeUse && ms.nCube < MAX_CUBE) {
            pmr->g.nAutoDoubles++;
            if (fDisplay)
                outputf(_("The cube is now at %d.\n"), ms.nCube <<= 1);
            UpdateSetting(&ms.nCube);
        }

        goto reroll;
    }

    g_assert(ms.nCube <= MAX_CUBE);
    g_assert(ms.anDice[1] != ms.anDice[0]);

    outputx();

    pmr = NewMoveRecord();
    pmr->mt = MOVE_SETDICE;

    pmr->anDice[0] = MAX(ms.anDice[0], ms.anDice[1]);
    pmr->anDice[1] = MIN(ms.anDice[0], ms.anDice[1]);
    pmr->fPlayer = ms.anDice[1] > ms.anDice[0];


    AddMoveRecord(pmr);

#if USE_BOARD3D
    RestrictiveRedraw();
#endif

    UpdateSetting(&ms.fTurn);
    UpdateSetting(&ms.gs);
    /* Play sound after initial dice decided */
    DiceRolled();
#if USE_GTK
    ResetDelayTimer();
#endif
#ifdef WEB
    ShowBoard(); // added in web interface so that dice roll shows up in the GUI before computer starts thinking
#endif /* WEB */
    return 0;
}

static void
ShowAutoMove(const TanBoard anBoard, int anMove[8])
{

    char sz[40];

    if (!fDisplay)
        return;

    if (anMove[0] == -1)
        outputf(_("%s cannot move.\n"), ap[ms.fTurn].szName);
    else
        outputf(_("%s moves %s.\n"), ap[ms.fTurn].szName, FormatMove(sz, anBoard, anMove));
}

static void
get_eq_before_resign(cubeinfo * pci, decisionData * pdd)
{
    const evalcontext ecResign = { FALSE, 0, FALSE, TRUE, 0.0 };

    pdd->pboard = msBoard();
    pdd->pci = pci;
    pdd->pec = &ecResign;

    if (ms.anDice[0] > 0) {
        float t;
        /* Opponent has rolled the dice and then resigned. We want to find out if the resignation is OK after
         * the roll */
        RunAsyncProcess((AsyncFun) asyncEvalRoll, pdd, _("Considering resignation..."));
        /* Swap the equities as evaluation is for other player */
        pdd->aarOutput[0][OUTPUT_WIN] = 1 - pdd->aarOutput[0][OUTPUT_WIN];
        t = pdd->aarOutput[0][OUTPUT_WINGAMMON];
        pdd->aarOutput[0][OUTPUT_WINGAMMON] = pdd->aarOutput[0][OUTPUT_LOSEGAMMON];
        pdd->aarOutput[0][OUTPUT_LOSEGAMMON] = t;
        t = pdd->aarOutput[0][OUTPUT_WINBACKGAMMON];
        pdd->aarOutput[0][OUTPUT_WINBACKGAMMON] = pdd->aarOutput[0][OUTPUT_LOSEBACKGAMMON];
        pdd->aarOutput[0][OUTPUT_LOSEBACKGAMMON] = t;
    } else {
        RunAsyncProcess((AsyncFun) asyncMoveDecisionE, pdd, _("Considering resignation..."));
    }
}

extern int
check_resigns(cubeinfo * pci)
{
    float rEqBefore, rEqAfter;
    const float max_cost = 0.05f;
    const float max_gain = 1e-6f;
    decisionData dd;
    cubeinfo ci;
    int resigned = 1;

    if (pci == NULL) {
        GetMatchStateCubeInfo(&ci, &ms);
        pci = &ci;
    }

    get_eq_before_resign(pci, &dd);
    do {
        getResignEquities(dd.aarOutput[0], pci, resigned, &rEqBefore, &rEqAfter);
        if (rEqBefore - rEqAfter > max_cost) {
            resigned = 4;
            break;
        } else if (rEqAfter - rEqBefore < max_gain)
            break;
    }
    while (resigned++ <= 3);
    return resigned == 4 ? -1 : resigned;
}

static void
parsemove_to_anmove(int c, int anMove[])
{
    int i;
    for (i = 0; i < 4; i++) {
        if (i < c) {
            anMove[i << 1]--;
            anMove[(i << 1) + 1]--;
        } else {
            anMove[i << 1] = -1;
            anMove[(i << 1) + 1] = -1;
        }
    }
    CanonicalMoveOrder(anMove);
}

/*! \brief parses the move and finds the new board in the list of moves/
 * \param pml the list of moves
 * \param old_board the board before move
 * \param board the new board 
 * \return 1 if found, 0 otherwise
 */
static gboolean
parse_move_is_legal(char *sz, const matchstate * pms, int *an)
{
    int c;
    TanBoard anBoardNew;
    movelist ml;
    c = ParseMove(sz, an);
    if (c < 0)
        return FALSE;
    parsemove_to_anmove(c, an);
    memcpy(anBoardNew, ms.anBoard, sizeof(TanBoard));
    ApplyMove(anBoardNew, an, FALSE);
    GenerateMoves(&ml, pms->anBoard, pms->anDice[0], pms->anDice[1], FALSE);
    return board_in_list(&ml, pms->anBoard, (ConstTanBoard) anBoardNew, an);
}


static int
ComputerTurn(void)
{

    moverecord *pmr;
    cubeinfo ci;
    float arDouble[4], rDoublePoint;
#if HAVE_SOCKETS
    char szBoard[256], szResponse[256];
    int fTurnOrig;
    TanBoard anBoardTemp;
#endif

    if (fInterrupt || ms.gs != GAME_PLAYING)
        return -1;

    GetMatchStateCubeInfo(&ci, &ms);

    switch (ap[ms.fTurn].pt) {
    case PLAYER_GNU:
        if (ms.fResigned) {
            int resign;
            if (ms.fResigned == -1)
                resign = check_resigns(&ci);
            else {
                float rEqBefore, rEqAfter;
                const float max_gain = 1e-6f;
                decisionData dd;
                get_eq_before_resign(&ci, &dd);
                getResignEquities(dd.aarOutput[0], &ci, ms.fResigned, &rEqBefore, &rEqAfter);
                if (rEqAfter - rEqBefore < max_gain)
                    resign = ms.fResigned;
                else
                    resign = -1;
            }

            fComputerDecision = TRUE;
            if (resign > 0) {
                ms.fResigned = resign;
                CommandAgree(NULL);
            } else {
                CommandDecline(NULL);
            }

            fComputerDecision = FALSE;
            return 0;
        } else if (ms.fDoubled) {
            decisionData dd;
            cubedecision cd;

            /* Consider cube action */

            /* 
             * We may get here in three different scenarios: 
             * (1) normal double by opponent: fMove != fTurn and fCubeOwner is
             *     either -1 (centered cube) or = fMove.
             * (2) beaver by opponent: fMove = fTurn and fCubeOwner = !
             *     fMove
             * (3) raccoon by opponent: fMove != fTurn and fCubeOwner =
             *     fTurn.
             *
             */

            if (ms.fMove != ms.fTurn && ms.fCubeOwner == ms.fTurn) {

                /* raccoon: consider this a normal double, i.e. 
                 * fCubeOwner = fMove */

                SetCubeInfo(&ci, ci.nCube,
                            ci.fMove, ci.fMove, ci.nMatchTo, ci.anScore, ci.fCrawford, ci.fJacoby, ci.fBeavers, ci.bgv);

            }

            if (ms.fMove == ms.fTurn && ms.fCubeOwner != ms.fMove) {

                /* opponent beavered: consider this a normal double by me */

                SetCubeInfo(&ci, ci.nCube,
                            ci.fMove, ci.fMove, ci.nMatchTo, ci.anScore, ci.fCrawford, ci.fJacoby, ci.fBeavers, ci.bgv);

            }

            /* Evaluate cube decision */
            dd.pboard = msBoard();
            dd.pci = &ci;
            dd.pes = &ap[ms.fTurn].esCube;
            if (RunAsyncProcess((AsyncFun) asyncCubeDecision, &dd, _("Considering cube action...")) != 0)
                return -1;

            current_pmr_cubedata_update(dd.pes, dd.aarOutput, dd.aarStdDev);

            cd = FindCubeDecision(arDouble, dd.aarOutput, &ci);

            fComputerDecision = TRUE;

            if (ms.fTurn == ms.fMove) {

                /* opponent has beavered */

                switch (cd) {

                case DOUBLE_TAKE:
                case REDOUBLE_TAKE:
                case TOOGOOD_TAKE:
                case TOOGOODRE_TAKE:
                case DOUBLE_PASS:
                case TOOGOOD_PASS:
                case REDOUBLE_PASS:
                case TOOGOODRE_PASS:
                case OPTIONAL_DOUBLE_TAKE:
                case OPTIONAL_REDOUBLE_TAKE:
                case OPTIONAL_DOUBLE_PASS:
                case OPTIONAL_REDOUBLE_PASS:

                    /* Opponent out of his right mind: Raccoon if possible */

                    if (ms.cBeavers < nBeavers && !ms.nMatchTo && ms.nCube < (MAX_CUBE >> 1))
                        /* he he: raccoon */
                        CommandRedouble(NULL);
                    else
                        /* Damn, no raccoons allowed */
                        CommandTake(NULL);

                    break;


                case NODOUBLE_TAKE:
                case NO_REDOUBLE_TAKE:

                    /* hmm, oops: opponent beavered us:
                     * consider dropping the beaver */

                    /* Note, this should not happen as the computer plays
                     * "perfectly"!! */

                    if (arDouble[OUTPUT_TAKE] <= -1.0f)
                        /* drop beaver */
                        CommandDrop(NULL);
                    else
                        /* take */
                        CommandTake(NULL);

                    break;


                case DOUBLE_BEAVER:
                case NODOUBLE_BEAVER:
                case NO_REDOUBLE_BEAVER:
                case OPTIONAL_DOUBLE_BEAVER:

                    /* opponent beaver was correct */

                    CommandTake(NULL);
                    break;

                default:

                    g_assert_not_reached();

                }               /* switch cubedecision */

            } /* consider beaver */
            else {

                /* normal double by opponent */

                switch (cd) {

                case DOUBLE_TAKE:
                case NODOUBLE_TAKE:
                case TOOGOOD_TAKE:
                case REDOUBLE_TAKE:
                case NO_REDOUBLE_TAKE:
                case TOOGOODRE_TAKE:
                case NODOUBLE_DEADCUBE:
                case NO_REDOUBLE_DEADCUBE:
                case OPTIONAL_DOUBLE_TAKE:
                case OPTIONAL_REDOUBLE_TAKE:

                    CommandTake(NULL);
                    break;

                case DOUBLE_PASS:
                case TOOGOOD_PASS:
                case REDOUBLE_PASS:
                case TOOGOODRE_PASS:
                case OPTIONAL_DOUBLE_PASS:
                case OPTIONAL_REDOUBLE_PASS:

                    CommandDrop(NULL);
                    break;

                case DOUBLE_BEAVER:
                case NODOUBLE_BEAVER:
                case NO_REDOUBLE_BEAVER:
                case OPTIONAL_DOUBLE_BEAVER:

                    if (ms.cBeavers < nBeavers && !ms.nMatchTo && ms.nCube < (MAX_CUBE >> 1))
                        /* Beaver all night! */
                        CommandRedouble(NULL);
                    else
                        /* Damn, no beavers allowed */
                        CommandTake(NULL);

                    break;

                default:

                    g_assert_not_reached();

                }               /* switch cubedecision */

            }                   /* normal cube */

            fComputerDecision = FALSE;

            return 0;

        } else {
            findData fd;
            TanBoard anBoardMove;
            float arResign[NUM_ROLLOUT_OUTPUTS];
            static char achResign[3] = { 'n', 'g', 'b' };
            char ach[2];

            /* Don't use the global board for this call, to avoid
             * race conditions with updating the board and aborting the
             * move with an interrupt. */
            memcpy(anBoardMove, msBoard(), sizeof(TanBoard));

            /* Consider resigning -- no point wasting time over the decision,
             * so only evaluate at 0 plies. */

            if (ClassifyPosition(msBoard(), ms.bgv) <= CLASS_RACE) {
                int nResign;

                evalcontext ecResign = { FALSE, 0, FALSE, TRUE, 0.0 };
                evalsetup esResign;

                esResign.et = EVAL_EVAL;
                esResign.ec = ecResign;

                nResign = getResignation(arResign, anBoardMove, &ci, &esResign);

                if (nResign > 0 && nResign > ms.fResignationDeclined) {

                    fComputerDecision = TRUE;
                    ach[0] = achResign[nResign - 1];
                    ach[1] = 0;
                    CommandResign(ach);
                    fComputerDecision = FALSE;

                    return 0;
                }
            }

            /* Consider doubling */

            if (ms.fCubeUse && !ms.anDice[0] && ms.nCube < MAX_CUBE && GetDPEq(NULL, NULL, &ci)) {
                evalcontext ecDH;
                float arOutput[NUM_ROLLOUT_OUTPUTS];
                memcpy(&ecDH, &ap[ms.fTurn].esCube.ec, sizeof ecDH);
                ecDH.fCubeful = FALSE;
                if (ecDH.nPlies)
                    ecDH.nPlies--;

                /* We have access to the cube */

                /* Determine market window */

                if (EvaluatePosition(NULL, (ConstTanBoard) anBoardMove, arOutput, &ci, &ecDH))
                    return -1;

                rDoublePoint = GetDoublePointDeadCube(arOutput, &ci);

                if (arOutput[0] >= rDoublePoint) {

                    /* We're in market window */
                    decisionData dd;
                    cubedecision cd;

                    /* Consider cube action */
                    dd.pboard = msBoard();
                    dd.pci = &ci;
                    dd.pes = &ap[ms.fTurn].esCube;
                    if (RunAsyncProcess((AsyncFun) asyncCubeDecision, &dd, _("Considering cube action...")) != 0)
                        return -1;


                    cd = FindCubeDecision(arDouble, dd.aarOutput, &ci);

                    switch (cd) {

                    case DOUBLE_TAKE:
                    case REDOUBLE_TAKE:
                    case DOUBLE_PASS:
                    case REDOUBLE_PASS:
                    case DOUBLE_BEAVER:
                        if (fTutor && fTutorCube)
                            /* only store cube if tutoring or we get comments
                             * where people don't want them */
                            current_pmr_cubedata_update(dd.pes, dd.aarOutput, dd.aarStdDev);
                        fComputerDecision = TRUE;
                        CommandDouble(NULL);
                        fComputerDecision = FALSE;
                        return 0;

                    case NODOUBLE_TAKE:
                    case TOOGOOD_TAKE:
                    case NO_REDOUBLE_TAKE:
                    case TOOGOODRE_TAKE:
                    case TOOGOOD_PASS:
                    case TOOGOODRE_PASS:
                    case NODOUBLE_BEAVER:
                    case NO_REDOUBLE_BEAVER:

                        current_pmr_cubedata_update(dd.pes, dd.aarOutput, dd.aarStdDev);
                        /* better leave cube where it is: no op */
                        break;

                    case OPTIONAL_DOUBLE_BEAVER:
                    case OPTIONAL_DOUBLE_TAKE:
                    case OPTIONAL_REDOUBLE_TAKE:
                    case OPTIONAL_DOUBLE_PASS:
                    case OPTIONAL_REDOUBLE_PASS:

                        if (ap[ms.fTurn].esCube.et == EVAL_EVAL && ap[ms.fTurn].esCube.ec.nPlies == 0 && arOutput[0] > 0.001f ) {
                            /* double if 0-ply except when about to lose game */
                            if (fTutor && fTutorCube)
                                current_pmr_cubedata_update(dd.pes, dd.aarOutput, dd.aarStdDev);
                            fComputerDecision = TRUE;
                            CommandDouble(NULL);
                            fComputerDecision = FALSE;
                            return 0;
                        } else
                            current_pmr_cubedata_update(dd.pes, dd.aarOutput, dd.aarStdDev);
                        break;

                    default:

                        g_assert_not_reached();

                    }

                }               /* market window */
            }

            /* access to cube */
            /* Roll dice and move */
            if (!ms.anDice[0]) {
                if (!fCheat || CheatDice(ms.anDice, &ms, afCheatRoll[ms.fMove]))
                    if (RollDice(ms.anDice, &rngCurrent, rngctxCurrent) < 0)
                        return -1;

                DiceRolled();
                /* write line to status bar if using GTK */
#if USE_GTK
                if (fX) {

                    outputnew();
                    outputf(_("%s rolls %1i and %1i.\n"), ap[ms.fTurn].szName, ms.anDice[0], ms.anDice[1]);
                    outputx();

                }
#endif
                ResetDelayTimer();      /* Start the timer again -- otherwise the time
                                         * we spent contemplating the cube could replace
                                         * the delay. */

#ifdef WEB
		return 0;  // added in web interface so that dice roll shows up in the GUI before computer starts thinking
#endif /* WEB */
            }

            pmr = NewMoveRecord();

            pmr->mt = MOVE_NORMAL;
            pmr->anDice[0] = MAX(ms.anDice[0], ms.anDice[1]);
            pmr->anDice[1] = MIN(ms.anDice[0], ms.anDice[1]);
            pmr->fPlayer = ms.fTurn;
            pmr->esChequer = ap[ms.fTurn].esChequer;


            fd.pml = &pmr->ml;
            fd.pboard = (ConstTanBoard) anBoardMove;
            fd.keyMove = NULL;
            fd.rThr = 0.0f;
            fd.pci = &ci;
            fd.pec = &ap[ms.fTurn].esChequer.ec;
            fd.aamf = ap[ms.fTurn].aamf;
            if ((RunAsyncProcess((AsyncFun) asyncFindMove, &fd, _("Considering move...")) != 0) || fInterrupt) {
                free(pmr);
                return -1;
            }
            /* resorts the moves according to cubeful (if applicable),
             * cubeless and chequer on highest point to avoid some silly
             * looking moves */

            RefreshMoveList(&pmr->ml, NULL);


            /* make the move found above */
            if (pmr->ml.cMoves) {
                memcpy(pmr->n.anMove, pmr->ml.amMoves[0].anMove, sizeof(pmr->n.anMove));
                pmr->n.iMove = 0;
            }

            /* write move to status bar or stdout */
            outputnew();
            ShowAutoMove(msBoard(), pmr->n.anMove);
            outputx();

            if (pmr->n.anMove[0] < 0)
                playSound(SOUND_BOT_DANCE);

            AddMoveRecord(pmr);

            return 0;
        }

    case PLAYER_EXTERNAL:
#if HAVE_SOCKETS
        fTurnOrig = ms.fTurn;

#if 1
        /* FIXME handle resignations -- there must be some way of indicating
         * that a resignation has been offered to the external player. */
        if (ms.fResigned == 3) {
            fComputerDecision = TRUE;
            CommandAgree(NULL);
            fComputerDecision = FALSE;
            return 0;
        } else if (ms.fResigned) {
            fComputerDecision = TRUE;
            CommandDecline(NULL);
            fComputerDecision = FALSE;
            return 0;
        }
#endif

        if (!ms.anDice[0] && !ms.fDoubled && !ms.fResigned &&
            (!ms.fCubeUse || ms.nCube >= MAX_CUBE || !GetDPEq(NULL, NULL, &ci))) {
            if (RollDice(ms.anDice, &rngCurrent, rngctxCurrent) < 0)
                return -1;

            DiceRolled();
        }

        memcpy(anBoardTemp, msBoard(), sizeof(TanBoard));
        if (!ms.fMove)
            SwapSides(anBoardTemp);

        FIBSBoard(szBoard, anBoardTemp, ms.fMove,
                  ap[ms.fMove].szName, ap[!ms.fMove].szName,
                  ms.nMatchTo, ms.anScore[ms.fMove], ms.anScore[!ms.fMove],
                  ms.anDice[0], ms.anDice[1], ms.nCube, ms.fCubeOwner, ms.fDoubled, 0 /* turn */ , ms.fCrawford,
                  anChequers[ms.bgv]);
        strcat(szBoard, "\n");

        if (ExternalWrite(ap[ms.fTurn].h, szBoard, strlen(szBoard) + 1) < 0)
            return -1;

        if (ExternalRead(ap[ms.fTurn].h, szResponse, sizeof(szResponse)) < 0)
            return -1;

#if 0
        /* Resignations for external players -- not yet implemented. */
        if (ms.fResigned) {
            fComputerDecision = TRUE;

            if (tolower(*szResponse) == 'a')    /* accept or agree */
                CommandAgree(NULL);
            else if (tolower(*szResponse) == 'd' || tolower(*szResponse) == 'r')        /* decline or reject */
                CommandDecline(NULL);
            else {
                outputl(_("Warning: badly formed resignation response from " "external player"));
                fComputerDecision = FALSE;
                return -1;
            }

            fComputerDecision = FALSE;

            return ms.fTurn == fTurnOrig ? -1 : 0;
        }
#endif
#define OLD_ERROR_START "Error ("
#define ERROR_START "Error: "
        if (!strncmp(szResponse, ERROR_START, strlen(ERROR_START)) ||
            !strncmp(szResponse, OLD_ERROR_START, strlen(OLD_ERROR_START))) {
            outputl(szResponse);
            fComputerDecision = FALSE;
            return -1;
        }

        if (ms.fDoubled) {
            fComputerDecision = TRUE;

            if (tolower(*szResponse) == 'a' || tolower(*szResponse) == 't')     /* accept or take */
                CommandTake(NULL);
            else if (tolower(*szResponse) == 'd' ||     /* drop */
                     tolower(*szResponse) == 'p' ||     /* pass */
                     !StrNCaseCmp(szResponse, "rej", 3))        /* reject */
                CommandDrop(NULL);
            else if (tolower(*szResponse) == 'b' ||     /* beaver */
                     !StrNCaseCmp(szResponse, "red", 3))        /* redouble */
                CommandRedouble(NULL);
            else {
                outputl(_("Warning: badly formed cube response from " "external player"));
                fComputerDecision = FALSE;
                return -1;
            }

            fComputerDecision = FALSE;

            return ms.fTurn != fTurnOrig || ms.gs >= GAME_OVER ? 0 : -1;
        } else if (tolower(szResponse[0]) == 'r' && tolower(szResponse[1]) == 'e') {    /* resign */
            char *r = szResponse;
            NextToken(&r);

            fComputerDecision = TRUE;
            CommandResign(r);
            fComputerDecision = FALSE;

            return ms.fTurn == fTurnOrig ? -1 : 0;
        } else if (!ms.anDice[0]) {
            if (tolower(*szResponse) == 'r') {  /* roll */
                if (RollDice(ms.anDice, &rngCurrent, rngctxCurrent) < 0)
                    return -1;

                DiceRolled();

                return 0;       /* we'll end up back here to play the roll, but
                                 * that's OK */
            } else if (tolower(*szResponse) == 'd') {   /* double */
                fComputerDecision = TRUE;
                CommandDouble(NULL);
                fComputerDecision = FALSE;
            }

            return ms.fTurn == fTurnOrig ? -1 : 0;
        } else {
            int an[8];
            parse_move_is_legal(szResponse, &ms, an);
            pmr = NewMoveRecord();
            pmr->mt = MOVE_NORMAL;
            pmr->anDice[0] = MAX(ms.anDice[0], ms.anDice[1]);
            pmr->anDice[1] = MIN(ms.anDice[0], ms.anDice[1]);
            pmr->fPlayer = ms.fTurn;
            memcpy(pmr->n.anMove, an, sizeof pmr->n.anMove);
            if (pmr->n.anMove[0] < 0) {
                /* illegal or no move */
                playSound(SOUND_BOT_DANCE);
            }
            AddMoveRecord(pmr);
            return 0;
        }
#else
        /* fall through */
#endif

    case PLAYER_HUMAN:
        /* fall through */
        ;
    }

    g_assert_not_reached();
    return -1;
}

static void
TurnDone(void)
{

#if USE_GTK
    if (fX) {
        if (!nNextTurn)
            nNextTurn = g_idle_add(NextTurnNotify, NULL);
    } else
#endif
        fNextTurn = TRUE;

    outputx();
}

extern void
CancelCubeAction(void)
{

    if (ms.fDoubled) {
        ms.fDoubled = FALSE;

        if (fDisplay)
            ShowBoard();

        /* FIXME should fTurn be set to fMove? */
        /* TurnDone(); Removed. Causes problems when called during
         * analysematch */
        /* FIXME delete all MOVE_DOUBLE records */
    }
}


static void
StartAutomaticPlay(void)
{
    if (ap[0].pt == PLAYER_GNU && ap[1].pt == PLAYER_GNU) {
        automaticTask = TRUE;
#if USE_GTK
        GTKSuspendInput();
        ProcessEvents();
#endif
    }
}

extern void
StopAutomaticPlay(void)
{
    if (automaticTask) {
#if USE_GTK
        GTKResumeInput();
#endif
        automaticTask = FALSE;
    }
}

/* Try to automatically bear off as many chequers as possible.  Only do it
 * if it's a non-contact position, and each die can be used to bear off
 * a chequer. */
static int
TryBearoff(void)
{

    movelist ml;
    unsigned int i, iMove, cMoves;
    moverecord *pmr;

    if (ClassifyPosition(msBoard(), VARIATION_STANDARD) > CLASS_RACE)
        /* It's a contact position; don't automatically bear off */
        /* note that we use VARIATION_STANDARD instead of ms.bgv in order
         * to avoid automatic bearoff in contact positions for hypergammon */
        return -1;

    GenerateMoves(&ml, msBoard(), ms.anDice[0], ms.anDice[1], FALSE);

    cMoves = (ms.anDice[0] == ms.anDice[1]) ? 4 : 2;

    for (i = 0; i < ml.cMoves; i++)
        for (iMove = 0; iMove < cMoves; iMove++)
            if ((ml.amMoves[i].anMove[iMove << 1] < 0) || (ml.amMoves[i].anMove[(iMove << 1) + 1] != -1))
                break;
            else if (iMove == cMoves - 1) {
                /* All dice bear off */
                pmr = NewMoveRecord();

                pmr->mt = MOVE_NORMAL;
                pmr->anDice[0] = MAX(ms.anDice[0], ms.anDice[1]);
                pmr->anDice[1] = MIN(ms.anDice[0], ms.anDice[1]);
                pmr->fPlayer = ms.fTurn;
                memcpy(pmr->n.anMove, ml.amMoves[i].anMove, sizeof(pmr->n.anMove));

                ShowAutoMove(msBoard(), pmr->n.anMove);


                AddMoveRecord(pmr);


                return 0;
            }

    return -1;
}

extern int
NextTurn(int fPlayNext)
{

    g_assert(!fComputing);

#if USE_GTK
    if (fX) {
        if (nNextTurn) {
            g_source_remove(nNextTurn);
            nNextTurn = 0;
        } else
            return -1;
    } else
#endif
    if (fNextTurn)
        fNextTurn = FALSE;
    else
        return -1;

    if (!plGame)
        return -1;

    fComputing = TRUE;

#if USE_GTK
    if (fX && fDisplay) {
        if (nDelay && nTimeout) {
            fDelaying = TRUE;
            GTKDelay();
            fDelaying = FALSE;
            ResetDelayTimer();
        }

        if (fLastMove) {
            board_animate(BOARD(pwBoard), anLastMove, fLastPlayer);
            if (fInterrupt && !automaticTask)
                fInterrupt = FALSE;

            playSound(SOUND_MOVE);
            fLastMove = FALSE;
        }
    }
#endif

    UpdateSetting(&ms.nCube);
    UpdateSetting(&ms.fCubeOwner);
    UpdateSetting(&ms.fTurn);
    UpdateSetting(&ms.gs);

    if (GameStatus(msBoard(), ms.bgv) || ms.gs == GAME_DROP || ms.gs == GAME_RESIGNED) {
        moverecord *pmr = (moverecord *) plGame->plNext->p;
        xmovegameinfo *pmgi = &pmr->g;
        int n;

        if (ms.fJacoby && ms.fCubeOwner == -1 && !ms.nMatchTo)
            /* gammons do not count on a centred cube during money
             * sessions under the Jacoby rule */
            n = 1;
        else if (ms.gs == GAME_DROP)
            n = 1;
        else if (ms.gs == GAME_RESIGNED)
            n = ms.fResigned;
        else
            n = GameStatus(msBoard(), ms.bgv);



        playSound(ap[pmgi->fWinner].pt == PLAYER_HUMAN ? SOUND_HUMAN_WIN_GAME : SOUND_BOT_WIN_GAME);

        outputf(ngettext("%s wins a %s and %d point.\n",
                         "%s wins a %s and %d points.\n",
                         pmgi->nPoints), ap[pmgi->fWinner].szName, gettext(aszGameResult[n - 1]), pmgi->nPoints);


#if USE_GTK
        if (fX) {
            if (fDisplay) {
#if USE_BOARD3D
                BoardData *bd = BOARD(pwBoard)->board_data;
                if (ms.fResigned && display_is_3d(bd->rd))
                    StopIdle3d(bd, bd->bd3d);   /* Stop flag waving */

#endif
                /* Clear the resignation flag */
                ms.fResigned = 0;

                ShowBoard();
            } else
                outputx();
        }
#endif

        if (ms.nMatchTo && fAutoCrawford) {
            ms.fPostCrawford |= ms.fCrawford && ms.anScore[pmgi->fWinner] < ms.nMatchTo;
            ms.fCrawford = !ms.fPostCrawford && !ms.fCrawford &&
                ms.anScore[pmgi->fWinner] == ms.nMatchTo - 1 && ms.anScore[!pmgi->fWinner] != ms.nMatchTo - 1;
        }
#if USE_GTK
        if (!fX || fDisplay)
#endif
            CommandShowScore(NULL);

        if (ms.nMatchTo && ms.anScore[pmgi->fWinner] >= ms.nMatchTo) {
            playSound(ap[pmgi->fWinner].pt == PLAYER_HUMAN ? SOUND_HUMAN_WIN_MATCH : SOUND_BOT_WIN_MATCH);

            outputf(_("%s has won the match.\n"), ap[pmgi->fWinner].szName);
            outputx();
            fComputing = FALSE;

            ShowBoard();

            StopAutomaticPlay();

            return -1;
        }

        outputx();

        if (fAutoGame) {
            if (NewGame() < 0) {
                fComputing = FALSE;
                return -1;
            }

            if (ap[ms.fTurn].pt == PLAYER_HUMAN)
                ShowBoard();
        } else {
            fComputing = FALSE;
            StopAutomaticPlay();
            return -1;
        }
    }

    g_assert(ms.gs == GAME_PLAYING);

    if (fDisplay || ap[ms.fTurn].pt == PLAYER_HUMAN) {
        ShowBoard();
    }

    /* We have reached a safe point to check for interrupts.  Until now,
     * the board could have been in an inconsistent state. */
    if (fInterrupt || !fPlayNext) {
        fComputing = FALSE;
        return -1;
    }

    if (ap[ms.fTurn].pt == PLAYER_HUMAN) {
        /* Roll for them, if:
         * 
         * * "auto roll" is on;
         * * they haven't already rolled;
         * * they haven't just been doubled;
         * * somebody hasn't just resigned;
         * * at least one of the following:
         * - cube use is disabled;
         * - it's the Crawford game;
         * - the cube is dead. */
        if (fAutoRoll && !ms.anDice[0] && !ms.fDoubled && !ms.fResigned &&
            (!ms.fCubeUse || ms.fCrawford ||
             (ms.fCubeOwner >= 0 && ms.fCubeOwner != ms.fTurn) ||
             (ms.nMatchTo > 0 && ms.anScore[ms.fTurn] + ms.nCube >= ms.nMatchTo)))
            CommandRoll(NULL);

        fComputing = FALSE;
        return -1;
    }
#if USE_GTK
    if (fX) {
        if (!ComputerTurn() && !nNextTurn)
            nNextTurn = g_idle_add(NextTurnNotify, NULL);
    } else
#endif
        fNextTurn = !ComputerTurn();

    fComputing = FALSE;
    return 0;
}

#if USE_GTK
extern int
quick_roll(void)
{
    if (ms.gs == GAME_NONE && move_is_last_in_match()) {
        UserCommand("new match");
        return (ms.gs == GAME_PLAYING);
    }
    if (ms.gs != GAME_PLAYING && move_is_last_in_match()) {
        UserCommand("new game");
        return (ms.gs == GAME_PLAYING);
    }
    if (ms.gs != GAME_PLAYING)
        /* we are not playing and the move isn't the last in the match */
        return 0;
    if (!ms.anDice[0]) {
        UserCommand("roll");
        return (ms.anDice[0]);
    }
    return 0;
}
#endif


extern void
CommandAccept(char *sz)
{

    if (ms.fResigned)
        CommandAgree(sz);
    else if (ms.fDoubled)
        CommandTake(sz);
    else
        outputl(_("You can only accept if the cube or a resignation has been " "offered."));
}

extern void
CommandAgree(char *UNUSED(sz))
{

    moverecord *pmr;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));

        return;
    }

    if (ap[ms.fTurn].pt != PLAYER_HUMAN && !fComputerDecision) {
        outputl(_("It is the computer's turn -- type `play' to force it to " "move immediately."));
        return;
    }

    if (!ms.fResigned) {
        outputl(_("No resignation was offered."));

        return;
    }

    if (!move_not_last_in_match_ok())
        return;

    if (fDisplay)
        outputf(_("%s accepts and wins a %s.\n"), ap[ms.fTurn].szName, gettext(aszGameResult[ms.fResigned - 1]));

    playSound(SOUND_AGREE);

    pmr = NewMoveRecord();

    pmr->mt = MOVE_RESIGN;
    pmr->fPlayer = !ms.fTurn;

    pmr->r.nResigned = ms.fResigned;
    pmr->r.esResign.et = EVAL_NONE;

    AddMoveRecord(pmr);

    TurnDone();
}

static void
AnnotateMove(skilltype st)
{

    moverecord *pmr;

    if (!plLastMove || !plLastMove->plNext || (!(pmr = plLastMove->plNext->p))) {
        outputl(_("You must select a move to annotate first."));
        return;
    }

    switch (pmr->mt) {
    case MOVE_NORMAL:

        switch (at) {
        case ANNOTATE_MOVE:
            pmr->n.stMove = st; /* fixme */
            break;
        case ANNOTATE_CUBE:
        case ANNOTATE_DOUBLE:
            pmr->stCube = st;   /* fixme */
            break;
        default:
            outputl(_("Invalid annotation"));
            break;
        }

        break;

    case MOVE_DOUBLE:

        switch (at) {
        case ANNOTATE_CUBE:
        case ANNOTATE_DOUBLE:
            pmr->stCube = st;
            break;
        default:
            outputl(_("Invalid annotation"));
            break;
        }

        break;

    case MOVE_TAKE:

        switch (at) {
        case ANNOTATE_CUBE:
        case ANNOTATE_ACCEPT:
            pmr->stCube = st;
            break;
        default:
            outputl(_("Invalid annotation"));
            break;
        }

        break;

    case MOVE_DROP:

        switch (at) {
        case ANNOTATE_CUBE:
        case ANNOTATE_REJECT:
        case ANNOTATE_DROP:
            pmr->stCube = st;
            break;
        default:
            outputl(_("Invalid annotation"));
            break;
        }

        break;

    case MOVE_RESIGN:

        switch (at) {
        case ANNOTATE_RESIGN:
            pmr->r.stResign = st;
            break;
        case ANNOTATE_ACCEPT:
        case ANNOTATE_REJECT:
            pmr->r.stAccept = st;
            break;
        default:
            outputl(_("Invalid annotation"));
            break;
        }

        break;

    default:
        outputl(_("You cannot annotate this move."));
        return;
    }

    if (st == SKILL_NONE)
        outputl(_("Skill annotation cleared."));
    else
        outputf(_("Move marked as %s.\n"), gettext(aszSkillType[st]));

#if USE_GTK
    if (fX)
        ChangeGame(NULL);
#endif

}

static void
AnnotateRoll(lucktype lt)
{

    moverecord *pmr;

    if (!plLastMove || !plLastMove->plNext || (!(pmr = plLastMove->plNext->p))) {
        outputl(_("You must select a move to annotate first."));
        return;
    }

    switch (pmr->mt) {
    case MOVE_NORMAL:
        pmr->lt = lt;
        break;

    case MOVE_SETDICE:
        pmr->lt = lt;
        break;

    default:
        outputl(_("You cannot annotate this move."));
        return;
    }

    if (lt == LUCK_NONE)
        outputl(_("Luck annotation cleared."));
    else
        outputf(_("Roll marked as %s.\n"), gettext(aszLuckType[lt]));

#if USE_GTK
    if (fX)
        ChangeGame(NULL);
#endif

}


extern void
CommandAnnotateAccept(char *sz)
{

    at = ANNOTATE_ACCEPT;
    HandleCommand(sz, acAnnotateMove);

}

extern void
CommandAnnotateCube(char *sz)
{

    at = ANNOTATE_CUBE;
    HandleCommand(sz, acAnnotateMove);

}

extern void
CommandAnnotateDouble(char *sz)
{

    at = ANNOTATE_DOUBLE;
    HandleCommand(sz, acAnnotateMove);

}

extern void
CommandAnnotateDrop(char *sz)
{

    at = ANNOTATE_DROP;
    HandleCommand(sz, acAnnotateMove);

}

extern void
CommandAnnotateMove(char *sz)
{

    at = ANNOTATE_MOVE;
    HandleCommand(sz, acAnnotateMove);

}

extern void
CommandAnnotateReject(char *sz)
{

    at = ANNOTATE_REJECT;
    HandleCommand(sz, acAnnotateMove);

}

extern void
CommandAnnotateResign(char *sz)
{

    at = ANNOTATE_RESIGN;
    HandleCommand(sz, acAnnotateMove);

}

extern void
CommandAnnotateBad(char *UNUSED(sz))
{

    AnnotateMove(SKILL_BAD);
}

extern void
CommandAnnotateAddComment(char *sz)
{

    moverecord *pmr;

    if (!plLastMove || !plLastMove->plNext || (!(pmr = plLastMove->plNext->p))) {
        outputl(_("You must select a move to which to add the comment."));
        return;
    }

    if (pmr->sz)
        free(pmr->sz);

    pmr->sz = g_strdup(sz);

    outputl(_("Commentary for this move added."));

#if USE_GTK
    if (fX)
        ChangeGame(NULL);
#endif

}

extern void
CommandAnnotateClearComment(char *UNUSED(sz))
{

    moverecord *pmr;

    if (!plLastMove || !plLastMove->plNext || (!(pmr = plLastMove->plNext->p))) {
        outputl(_("You must select a move to clear the comment from."));
        return;
    }

    if (pmr->sz)
        free(pmr->sz);

    pmr->sz = NULL;

    outputl(_("Commentary for this move cleared."));

#if USE_GTK
    if (fX)
        ChangeGame(NULL);
#endif

}

extern void
CommandAnnotateClearLuck(char *UNUSED(sz))
{

    AnnotateRoll(LUCK_NONE);
}

extern void
CommandAnnotateClearSkill(char *UNUSED(sz))
{

    AnnotateMove(SKILL_NONE);
}

extern void
CommandAnnotateDoubtful(char *UNUSED(sz))
{

    AnnotateMove(SKILL_DOUBTFUL);
}

extern void
CommandAnnotateLucky(char *UNUSED(sz))
{

    AnnotateRoll(LUCK_GOOD);
}

extern void
CommandAnnotateUnlucky(char *UNUSED(sz))
{

    AnnotateRoll(LUCK_BAD);
}

extern void
CommandAnnotateVeryBad(char *UNUSED(sz))
{

    AnnotateMove(SKILL_VERYBAD);
}

extern void
CommandAnnotateVeryLucky(char *UNUSED(sz))
{

    AnnotateRoll(LUCK_VERYGOOD);
}

extern void
CommandAnnotateVeryUnlucky(char *UNUSED(sz))
{

    AnnotateRoll(LUCK_VERYBAD);
}

extern void
CommandDecline(char *UNUSED(sz))
{

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));

        return;
    }

    if (ap[ms.fTurn].pt != PLAYER_HUMAN && !fComputerDecision) {
        outputl(_("It is the computer's turn -- type `play' to force it to " "move immediately."));
        return;
    }

    if (!ms.fResigned) {
        outputl(_("No resignation was offered."));

        return;
    }

    if (fDisplay) {
        {
            if (ms.fResigned > 0)
                outputf(_("%s declines the %s.\n"), ap[ms.fTurn].szName, gettext(aszGameResult[ms.fResigned - 1]));
            else
                outputf(_("%s declines the resignation\n"), ap[ms.fTurn].szName);
        }
    }

    ms.fResignationDeclined = ms.fResigned;
    ms.fResigned = FALSE;
    ms.fTurn = !ms.fTurn;

    TurnDone();
}

static skilltype
tutor_double(int did_double)
{
    moverecord *pmr;
    cubeinfo ci;
    GetMatchStateCubeInfo(&ci, &ms);

    /* reasons that doubling is not an issue */
    if ((ms.gs != GAME_PLAYING)
        || ap[ms.fTurn].pt != PLAYER_HUMAN || !GetDPEq(NULL, NULL, &ci))
        return SKILL_NONE;

    hint_double(FALSE, did_double);
    pmr = get_current_moverecord(NULL);
    return pmr ? pmr->stCube : SKILL_NONE;
}

extern void
CommandDouble(char *UNUSED(sz))
{

    moverecord *pmr;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));

        return;
    }

    if (ap[ms.fTurn].pt != PLAYER_HUMAN && !fComputerDecision) {
        outputl(_("It is the computer's turn -- type `play' to force it to " "move immediately."));
        return;
    }

    if (ms.fCrawford) {
        outputl(_("Doubling is forbidden by the Crawford rule (see `help set " "crawford')."));

        return;
    }

    if (!ms.fCubeUse) {
        outputl(_("The doubling cube has been disabled (see `help set cube " "use')."));

        return;
    }

    if (!move_not_last_in_match_ok())
        return;

    if (ms.fDoubled) {
        CommandRedouble(NULL);
        return;
    }

    if (ms.fTurn != ms.fMove) {
        outputl(_("You are only allowed to double if you are on roll."));

        return;
    }

    if (ms.anDice[0]) {
        outputl(_("You can't double after rolling the dice -- wait until your " "next turn."));

        return;
    }

    if (ms.fCubeOwner >= 0 && ms.fCubeOwner != ms.fTurn) {
        outputl(_("You do not own the cube."));

        return;
    }

    if (ms.nCube >= MAX_CUBE) {
        outputf(_("The cube is already at %d; you can't double any more.\n"), MAX_CUBE);
        return;
    }

    playSound(SOUND_DOUBLE);

    pmr = NewMoveRecord();

    pmr->mt = MOVE_DOUBLE;
    pmr->fPlayer = ms.fTurn;
    if (fTutor && fTutorCube && !GiveAdvice(tutor_double(TRUE))) {
        free(pmr);
        return;
    }

    if (fDisplay)
        outputf(_("%s doubles.\n"), ap[ms.fTurn].szName);

#if USE_GTK
    /* There's no point delaying here. */
    if (nTimeout) {
        g_source_remove(nTimeout);
        nTimeout = 0;
    }
#endif

    AddMoveRecord(pmr);

    TurnDone();
}

static skilltype
tutor_take(int did_take)
{
    moverecord *pmr;

    /* reasons that taking is not an issue */
    if ((ms.gs != GAME_PLAYING) || ms.anDice[0] || !ms.fDoubled || ms.fResigned || (ap[ms.fTurn].pt != PLAYER_HUMAN))
        return (SKILL_NONE);

    hint_take(FALSE, did_take);
    pmr = get_current_moverecord(NULL);
    return pmr ? pmr->stCube : SKILL_NONE;
}


extern void
CommandDrop(char *UNUSED(sz))
{
    moverecord *pmr;

    if (ms.gs != GAME_PLAYING || !ms.fDoubled) {
        outputl(_("The cube must have been offered before you can drop it."));

        return;
    }

    if (ap[ms.fTurn].pt != PLAYER_HUMAN && !fComputerDecision) {
        outputl(_("It is the computer's turn -- type `play' to force it to " "move immediately."));
        return;
    }

    if (!move_not_last_in_match_ok())
        return;

    playSound(SOUND_DROP);

    pmr = NewMoveRecord();

    pmr->mt = MOVE_DROP;
    pmr->fPlayer = ms.fTurn;
    if (!LinkToDouble(pmr)) {
        free(pmr);
        return;
    }

    if (fTutor && fTutorCube && !GiveAdvice(tutor_take(FALSE))) {
        free(pmr);              /* garbage collect */
        return;
    }

    if (fDisplay)
        outputf(ngettext
                ("%s refuses the cube and gives up %d point.\n", "%s refuses the cube and gives up %d points.\n",
                 ms.nCube), ap[ms.fTurn].szName, ms.nCube);

    AddMoveRecord(pmr);

    TurnDone();
}

static void
DumpGameList(GString * gsz, listOLD * plGame)
{

    listOLD *pl;
    moverecord *pmr;
    int nFileCube = 1;
    TanBoard anBoard;
    char tmp[100];
    int column = 0;
    InitBoard(anBoard, ms.bgv);
    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext) {
        pmr = pl->p;
        if (column == 1)
            g_string_append(gsz, "\n");
        if (pmr->fPlayer == 1 && column == 1)
            g_string_append_printf(gsz, "%42s", "");
        column = pmr->fPlayer;
        switch (pmr->mt) {
        case MOVE_GAMEINFO:
            column = 0;
            break;
        case MOVE_NORMAL:
            g_string_append_printf(gsz, "%d%d%-5s: ", pmr->anDice[0], pmr->anDice[1], aszLuckTypeAbbr[pmr->lt]);
            FormatMove(tmp, (ConstTanBoard) anBoard, pmr->n.anMove);
            g_string_append_printf(gsz, "%-15s", tmp);
            g_string_append_printf(gsz, "%-10s%-10s", aszSkillTypeAbbr[pmr->n.stMove], aszSkillTypeAbbr[pmr->stCube]);
            ApplyMove(anBoard, pmr->n.anMove, FALSE);
            SwapSides(anBoard);
            break;
        case MOVE_DOUBLE:
            g_string_append_printf(gsz, _("Doubles to %4d                              "), nFileCube <<= 1);
            break;
        case MOVE_TAKE:
            g_string_append(gsz, _("Takes                                       "));
            break;
        case MOVE_DROP:
            g_string_append(gsz, _("Drops\n"));
            break;
        case MOVE_RESIGN:
            g_string_append(gsz, _("Resigns\n"));
            break;
        case MOVE_SETDICE:
            g_string_append_printf(gsz, "%d%d%-5s: %-33s",
                                   pmr->anDice[0], pmr->anDice[1], aszLuckTypeAbbr[pmr->lt], _("Rolled"));
            break;
        default:
            g_string_append_printf(gsz, _("Unhandled movetype %d                      \n"), pmr->mt);
            break;
        }
    }

}

extern void
CommandListGame(char *UNUSED(sz))
{
    GString *gsz = g_string_new(NULL);

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));

        return;
    }
    DumpGameList(gsz, plGame);
#if USE_GTK
    if (fX)
        GTKTextWindow(gsz->str, _("Game dump"), DT_INFO, NULL);
    else
#endif
    {
        outputl(gsz->str);
        outputx();
    }
    g_string_free(gsz, TRUE);
}

extern void
CommandListMatch(char *UNUSED(sz))
{
#if USE_GTK
    if (fX) {
        ShowGameWindow();
        return;
    }
#endif

    /* FIXME */
}

/*! \brief finds the new board in the list of moves/
 * \param pml the list of moves
 * \param old_board the board before move
 * \param board the new board 
 * \return 1 if found, 0 otherwise
 */
extern int
board_in_list(const movelist * pml, const TanBoard old_board, const TanBoard board, int *an)
{
    guint j;
    TanBoard list_board;
    g_return_val_if_fail(pml, FALSE);
    g_return_val_if_fail(old_board, FALSE);
    g_return_val_if_fail(board, FALSE);
    for (j = 0; j < pml->cMoves; j++) {
        memcpy(list_board, old_board, sizeof(TanBoard));
        ApplyMove(list_board, pml->amMoves[j].anMove, FALSE);
        if (memcmp(list_board, board, sizeof(TanBoard)) == 0) {
            if (an)
                memcpy(an, pml->amMoves[j].anMove, 8 * sizeof(int));
            return 1;
        }
    }
    if (an)
        *an = -1;
    return 0;
}

extern void
CommandMove(char *sz)
{

    int an[8];
    movelist ml;
    moverecord *pmr;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));

        return;
    }

    if (ap[ms.fTurn].pt != PLAYER_HUMAN) {
        outputl(_("It is the computer's turn -- type `play' to force it to " "move immediately."));
        return;
    }

    if (!ms.anDice[0]) {
        outputl(_("You must roll the dice before you can move."));

        return;
    }

    if (ms.fResigned) {
        outputf(_("Please wait for %s to consider the resignation before " "moving.\n"), ap[ms.fTurn].szName);

        return;
    }

    if (ms.fDoubled) {
        outputf(_("Please wait for %s to consider the cube before " "moving.\n"), ap[ms.fTurn].szName);

        return;
    }

    if (!move_not_last_in_match_ok())
        return;

    if (!*sz) {
        GenerateMoves(&ml, msBoard(), ms.anDice[0], ms.anDice[1], FALSE);

        if (ml.cMoves <= 1) {

            if (!ml.cMoves)
                playSound(ap[ms.fTurn].pt == PLAYER_HUMAN ? SOUND_HUMAN_DANCE : SOUND_BOT_DANCE);
            else
                playSound(SOUND_MOVE);


            pmr = NewMoveRecord();

            pmr->mt = MOVE_NORMAL;
            pmr->sz = NULL;
            pmr->anDice[0] = MAX(ms.anDice[0], ms.anDice[1]);
            pmr->anDice[1] = MIN(ms.anDice[0], ms.anDice[1]);
            pmr->fPlayer = ms.fTurn;
            if (ml.cMoves)
                memcpy(pmr->n.anMove, ml.amMoves[0].anMove, sizeof(pmr->n.anMove));

            ShowAutoMove(msBoard(), pmr->n.anMove);


            AddMoveRecord(pmr);

            TurnDone();

            return;
        }

        if (fAutoBearoff && !TryBearoff()) {
            TurnDone();

            return;
        }

        outputl(_("You must specify a move (type `help move' for " "instructions)."));

        return;
    }

    if (!parse_move_is_legal(sz, &ms, an)) {
        outputerrf(_("Illegal or unparsable move."));
        return;
    }

    /* we have a legal move! */
    playSound(SOUND_MOVE);
    pmr = NewMoveRecord();
    pmr->mt = MOVE_NORMAL;
    pmr->sz = NULL;
    pmr->anDice[0] = MAX(ms.anDice[0], ms.anDice[1]);
    pmr->anDice[1] = MIN(ms.anDice[0], ms.anDice[1]);
    pmr->fPlayer = ms.fTurn;
    memcpy(pmr->n.anMove, an, sizeof pmr->n.anMove);

    if (fTutor && fTutorChequer) {
        moverecord *pmr_cur = get_current_moverecord(NULL);
        g_assert(pmr_cur);
        /* update or set the move */
        memcpy(pmr_cur->n.anMove, an, sizeof an);
        hint_move("", FALSE, NULL);
        if (!GiveAdvice(pmr_cur->n.stMove)) {
            free(pmr);
            return;
        }
    }
#if USE_GTK
    /* There's no point delaying here. */
    if (nTimeout) {
        g_source_remove(nTimeout);
        nTimeout = 0;
    }

    if (fX) {
        outputnew();
        ShowAutoMove(msBoard(), pmr->n.anMove);
        outputx();
    }
#endif

    AddMoveRecord(pmr);

#if USE_GTK
    /* Don't animate this move. */
    fLastMove = FALSE;
#endif
    TurnDone();

    return;
}

static void
StartNewGame(void)
{
    fComputing = TRUE;
    NewGame();
    if (!fInterrupt) {
        if (ap[ms.fTurn].pt == PLAYER_HUMAN)
            ShowBoard();
        else if (!ComputerTurn())
            TurnDone();
    }
    fComputing = FALSE;
}

extern void
CommandNewGame(char *UNUSED(sz))
{
    if (ms.nMatchTo && (ms.anScore[0] >= ms.nMatchTo || ms.anScore[1] >= ms.nMatchTo)) {
        outputl(_("The match is already over."));
        return;
    }

    if (ms.gs == GAME_PLAYING || !move_is_last_in_match()) {
        if (fConfirmNew) {
            if (fInterrupt)
                return;

            if (!GetInputYN(_("Are you sure you want to start a new game, " "and discard the rest of the match? ")))
                return;
        }

        PopGame(plGame, TRUE);
    }

    StartAutomaticPlay();

    StartNewGame();
}

extern void
ClearMatch(void)
{
    char ***pppch;
    static char **appch[] = { &mi.pchRating[0], &mi.pchRating[1],
        &mi.pchEvent, &mi.pchRound, &mi.pchPlace,
        &mi.pchAnnotator, &mi.pchComment, NULL
    };

    ms.nMatchTo = 0;

    ms.cGames = ms.anScore[0] = ms.anScore[1] = 0;
    ms.fMove = ms.fTurn = -1;
    ms.fCrawford = FALSE;
    ms.fPostCrawford = FALSE;
    ms.gs = GAME_NONE;
    ms.fJacoby = fJacoby;

    IniStatcontext(&scMatch);

    for (pppch = appch; *pppch; pppch++)
        if (**pppch) {
            g_free(**pppch);
            **pppch = NULL;
        }

    mi.nYear = 0;
}

extern void
FreeMatch(void)
{

    PopGame(lMatch.plNext->p, TRUE);
    IniStatcontext(&scMatch);
}

extern void
SetMatchDate(matchinfo * pmi)
{

    time_t t = time(NULL);
    struct tm *ptm = localtime(&t);

    pmi->nYear = ptm->tm_year + 1900;;
    pmi->nMonth = ptm->tm_mon + 1;
    pmi->nDay = ptm->tm_mday;
}

extern void
CommandNewMatch(char *sz)
{
    unsigned int n;

    if (!sz || !*sz)
        n = nDefaultLength;
    else
        n = ParseNumber(&sz);

    if (n == 0) {
        CommandNewSession(NULL);
        return;
    }

    /* Check that match equity table is large enough */

    if (n > MAXSCORE) {
        outputf(_("GNU Backgammon is compiled with support only for "
                  "matches of length %i\n" "and below\n"), MAXSCORE);
        return;
    }

    if (!get_input_discard())
        return;

    FreeMatch();
    ClearMatch();

    strcpy(ap[0].szName, default_names[0]);
    strcpy(ap[1].szName, default_names[1]);
    plLastMove = NULL;

    ms.nMatchTo = n;
    ms.bgv = bgvDefault;
    ms.fCubeUse = fCubeUse;
    ms.fJacoby = fJacoby;

    SetMatchDate(&mi);

    UpdateSetting(&ms.nMatchTo);
    UpdateSetting(&ms.fTurn);
    UpdateSetting(&ms.fCrawford);
    UpdateSetting(&ms.fJacoby);
    UpdateSetting(&ms.gs);

    outputf(_("A new %d point match has been started.\n"), n);

#if USE_GTK
    if (fX)
        GTKSet(ap);
#endif

    CommandNewGame(NULL);
}

extern void
CommandNewSession(char *UNUSED(sz))
{
    if (!get_input_discard())
        return;

    FreeMatch();
    ClearMatch();

    ms.bgv = bgvDefault;
    ms.fCubeUse = fCubeUse;
    ms.fJacoby = fJacoby;
    plLastMove = NULL;

    SetMatchDate(&mi);

    UpdateSetting(&ms.nMatchTo);
    UpdateSetting(&ms.fTurn);
    UpdateSetting(&ms.fCrawford);
    UpdateSetting(&ms.fJacoby);
    UpdateSetting(&ms.gs);

    outputl(_("A new session has been started."));

#if USE_GTK
    if (fX)
        ShowBoard();
#endif

    CommandNewGame(NULL);
}

extern void
UpdateGame(int fShowBoard)
{

    UpdateSetting(&ms.nCube);
    UpdateSetting(&ms.fCubeOwner);
    UpdateSetting(&ms.fTurn);
    UpdateSetting(&ms.gs);
    UpdateSetting(&ms.fCrawford);

#if USE_GTK
    if (fX || (fShowBoard && fDisplay))
        ShowBoard();
#else
    if (fShowBoard && fDisplay)
        ShowBoard();
#endif
}

#if USE_GTK
static int
GameIndex(listOLD * plGame)
{

    listOLD *pl;
    int i;

    for (i = 0, pl = lMatch.plNext; pl->p != plGame && pl != &lMatch; pl = pl->plNext, i++);

    if (pl == &lMatch)
        return -1;
    else
        return i;
}
#endif

extern void
ChangeGame(listOLD * plGameNew)
{
    moverecord *pmr_cur;
    gboolean dice_rolled = FALSE;
    movetype reallastmt;
    int reallastplayer;

#if USE_GTK
    listOLD *pl;
#endif

    if (!plGame) {
        return;
    }

    if (plGameNew) {
        plGame = plGameNew;
        plLastMove = plGame->plNext;
    } else {
        if (ms.anDice[0] > 0)
            dice_rolled = TRUE;
    }

#if USE_GTK
    if (fX) {
        GTKFreeze();
        GTKClearMoveRecord();

        for (pl = plGame->plNext; pl->p; pl = pl->plNext) {
            GTKAddMoveRecord(pl->p);
            FixMatchState(&ms, pl->p);
            ApplyMoveRecord(&ms, plGame, pl->p);
        }
        GTKSetGame(GameIndex(plGame));
        /* we need to get the matchstate right before GTKThaw when
         * we are updating the current current game */
        CalculateBoard();
        GTKThaw();
    }
#endif
    CalculateBoard();
    SetMoveRecord(plLastMove->p);

    /* The real last move, before get_current_moverecord()
     * possibly adds an empty hint record. If it does and
     * last move is a MOVE_SETxxx record, its fPlayer will
     * be bogus and we will have to fix it */

    if ((pmr_cur = plLastMove->p)) {
        reallastmt = pmr_cur->mt;
        reallastplayer = pmr_cur->fPlayer;
    } else {
        reallastmt = reallastplayer = -1;
    }

    pmr_cur = get_current_moverecord(NULL);

    if (pmr_cur) {
        if (reallastmt >= MOVE_SETBOARD && pmr_cur->ml.cMoves == 0)
            pmr_cur->fPlayer = reallastplayer;
        if (pmr_cur->fPlayer != ms.fTurn) {
            SetTurn(pmr_cur->fPlayer);
        }
        if (dice_rolled) {
            ms.anDice[0] = pmr_cur->anDice[0];
            ms.anDice[1] = pmr_cur->anDice[1];
        }
    }
    UpdateGame(FALSE);
    ShowBoard();
}

static void
CommandNextGame(char *sz)
{

    int n;
    char *pch;
    listOLD *pl;

    if ((pch = NextToken(&sz)))
        n = ParseNumber(&pch);
    else
        n = 1;

    if (n < 1) {
        outputl(_("If you specify a parameter to the `next game' command, it "
                  "must be a positive number (the count of games to step " "ahead)."));
        return;
    }

    for (pl = lMatch.plNext; pl->p != plGame && pl != &lMatch; pl = pl->plNext);

    if (pl->p != plGame)
        /* current game not found */
        return;

    for (; n && pl->plNext->p; n--, pl = pl->plNext);

    if (pl->p == plGame)
        return;

    ChangeGame(pl->p);
}

extern void
CommandFirstMove(char *UNUSED(sz))
{
    ChangeGame(plGame);
}

extern void
CommandFirstGame(char *UNUSED(sz))
{
    ChangeGame(lMatch.plNext->p);
}

static void
CommandNextRoll(char *UNUSED(sz))
{
    moverecord *pmr;

#if USE_GTK
    BoardData *bd = NULL;

    if (fX) {
        bd = BOARD(pwBoard)->board_data;
        /* Make sure dice aren't rolled */
        bd->diceShown = DICE_ON_BOARD;
    }
#endif

    if (!plLastMove || !plLastMove->plNext || !(pmr = plLastMove->plNext->p))
        /* silently ignore */
        return;

    if (pmr->mt != MOVE_NORMAL || ms.anDice[0])
        /* to skip over the "dice roll" for anything other than a normal
         * move, or if the dice are already rolled, just skip the entire
         * move */
    {
        CommandNext(NULL);
        return;
    }

    CalculateBoard();

    ms.gs = GAME_PLAYING;
    ms.fMove = ms.fTurn = pmr->fPlayer;

    ms.anDice[0] = pmr->anDice[0];
    ms.anDice[1] = pmr->anDice[1];

#if USE_GTK
    /* Make sure dice are shown */
    if (fX) {
        bd->diceRoll[0] = !ms.anDice[0];
    }
#endif

    if (plLastMove->plNext && plLastMove->plNext->p)
        FixMatchState(&ms, plLastMove->plNext->p);

    UpdateGame(FALSE);
#ifdef WEB
    ShowBoard();
#endif /* WEB */
}

static void
CommandNextRolled(char *UNUSED(sz))
{
    moverecord *pmr;

    /* goto next move */

    CommandNext(NULL);

    if (plLastMove && plLastMove->plNext && (pmr = plLastMove->plNext->p) && pmr->mt == MOVE_NORMAL)
        CommandNextRoll(NULL);
}

static int
MoveIsCMarked(moverecord * pmr)
{
    unsigned int j;

    if (pmr == 0)
        return FALSE;

    switch (pmr->mt) {
    case MOVE_NORMAL:
        if (pmr->CubeDecPtr->cmark != CMARK_NONE)
            return TRUE;
        for (j = 0; j < pmr->ml.cMoves; j++) {
            if (pmr->ml.amMoves[j].cmark != CMARK_NONE)
                return TRUE;
        }
        break;
    case MOVE_DOUBLE:
        if (pmr->CubeDecPtr->cmark != CMARK_NONE)
            return TRUE;
        break;

    default:
        break;
    }

    return FALSE;
}

static int
MoveIsMarked(moverecord * pmr)
{
    if (pmr == 0)
        return FALSE;

    switch (pmr->mt) {
    case MOVE_NORMAL:
        return badSkill(pmr->n.stMove) || badSkill(pmr->stCube);

    case MOVE_DOUBLE:
    case MOVE_TAKE:
    case MOVE_DROP:
        return badSkill(pmr->stCube);

    default:
        break;
    }

    return FALSE;
}

static void
ShowMark(moverecord * pmr)
{

#if USE_GTK
    BoardData *bd = NULL;

    if (fX)
        bd = BOARD(pwBoard)->board_data;
#endif
    /* Show the dice roll, if the chequer play is marked but the cube
     * decision is not. */
    if (pmr->mt == MOVE_NORMAL && (pmr->stCube == SKILL_NONE) && pmr->n.stMove != SKILL_NONE) {
        ms.gs = GAME_PLAYING;
        ms.fMove = ms.fTurn = pmr->fPlayer;

        ms.anDice[0] = pmr->anDice[0];
        ms.anDice[1] = pmr->anDice[1];
    }
#if USE_GTK
    if (fX) {
        /* Don't roll dice */
        bd->diceShown = DICE_ON_BOARD;
        /* Make sure dice are shown */
        bd->diceRoll[0] = !ms.anDice[0];
    }
#endif

}

int
InternalCommandNext(int mark, int cmark, int n)
{
    int done = 0;

    if (mark || cmark) {
        listOLD *pgame;
        moverecord *pmr;
        listOLD *p = plLastMove->plNext;

        for (pgame = lMatch.plNext; pgame->p != plGame && pgame != &lMatch; pgame = pgame->plNext);
        if (pgame->p != plGame)
            /* current game not found */
            return 0;

        /* we need to increment the count if we're pointing to a marked move */
        if (p->p && (mark && MoveIsMarked((moverecord *) p->p)))
            ++n;
        if (p->p && (cmark && MoveIsCMarked((moverecord *) p->p)))
            ++n;

        while (p->p) {
            pmr = (moverecord *) p->p;
            if (mark && MoveIsMarked(pmr) && (--n <= 0))
                break;
            if (cmark && MoveIsCMarked(pmr) && (--n <= 0))
                break;
            p = p->plNext;
            if (!(p->p)) {
                if (pgame->plNext == pgame)
                    return 0;
                pgame = pgame->plNext;
                if (!(pgame->p))
                    return 0;
                p = ((listOLD *) pgame->p)->plNext;
            }
        }
        if (p->p == 0)
            return 0;
        if (pgame->p != plGame)
            ChangeGame(pgame->p);

        plLastMove = p->plPrev;
        CalculateBoard();
        ShowMark(pmr);
    } else {
        while (n-- && plLastMove->plNext->p) {
            plLastMove = plLastMove->plNext;
            FixMatchState(&ms, plLastMove->p);
            ApplyMoveRecord(&ms, plGame, plLastMove->p);

            ++done;
        }
    }

    UpdateGame(FALSE);

    if (plLastMove->plNext && plLastMove->plNext->p)
        FixMatchState(&ms, plLastMove->plNext->p);

    SetMoveRecord(plLastMove->p);

#ifdef WEB
    ShowBoard();
#endif /* WEB */

    return done;
}

extern void
CommandNext(char *sz)
{
    int n;
    char *pch;
    int mark = FALSE;
    int cmark = FALSE;

    if (!plGame) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }
#if USE_BOARD3D
    RestrictiveRedraw();
#endif
    n = 1;

    if ((pch = NextToken(&sz))) {
        if (!StrCaseCmp(pch, "game")) {
            CommandNextGame(sz);
            return;
        } else if (!StrCaseCmp(pch, "roll")) {
            CommandNextRoll(sz);
            return;
        } else if (!StrCaseCmp(pch, "rolled")) {
            CommandNextRolled(sz);
            return;
        } else if (!StrCaseCmp(pch, "marked")) {
            mark = TRUE;
            if ((pch = NextToken(&sz))) {
                n = ParseNumber(&pch);
            }
        } else if (!StrCaseCmp(pch, "cmarked")) {
            cmark = TRUE;
            if ((pch = NextToken(&sz))) {
                n = ParseNumber(&pch);
            }
        } else if (!StrCaseCmp(pch, "anymarked")) {
            mark = TRUE;
            cmark = TRUE;
            if ((pch = NextToken(&sz))) {
                n = ParseNumber(&pch);
            }
        } else
            n = ParseNumber(&pch);
    }

    if (n < 1) {
        outputl(_("If you specify a parameter to the `next' command, it must "
                  "be a positive number (the count of moves to step ahead)."));
        return;
    }

    InternalCommandNext(mark, cmark, n);
}

extern void
CommandEndGame(char *UNUSED(sz))
{
    playertype pt_store[2] = { ap[0].pt, ap[1].pt };
    int fAutoGame_store = fAutoGame;
    int fDisplay_store = fDisplay;
    int fQuiet_store = fQuiet;
    const evalcontext ec_quick = { FALSE, 0, FALSE, TRUE, 0.0 };
    int manual_dice = (rngCurrent == RNG_MANUAL);
    evalcontext ec_cheq_store[2];
    evalcontext ec_cube_store[2];
#if USE_BOARD3D
    BoardData *bd = NULL;
    if (fX && pwBoard)
        bd = BOARD(pwBoard)->board_data;
#endif
    ec_cheq_store[0] = ap[0].esChequer.ec;
    ec_cheq_store[1] = ap[1].esChequer.ec;
    ec_cube_store[0] = ap[0].esCube.ec;
    ec_cube_store[1] = ap[1].esCube.ec;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    if (!move_not_last_in_match_ok())
        return;

#if USE_GTK
    else if (!GTKShowWarning(WARN_ENDGAME, NULL))
        return;
#endif
    ap[0].pt = PLAYER_GNU;
    ap[1].pt = PLAYER_GNU;
    ap[0].esChequer.ec = ec_quick;
    ap[1].esChequer.ec = ec_quick;
    ap[0].esCube.ec = ec_quick;
    ap[1].esCube.ec = ec_quick;

    if (manual_dice) {
        outputoff();
        SetRNG(&rngCurrent, rngctxCurrent, RNG_MERSENNE, "");
        outputon();
    }
#if USE_BOARD3D
    if (fX && bd)
        SuspendDiceRolling(bd->rd);
#endif

    fAutoGame = FALSE;
    fQuiet = TRUE;
    fDisplay = FALSE;
    fInterrupt = FALSE;
    fEndGame = TRUE;
    outputnew();

    do {
#if USE_GTK
        UserCommand("play");
        while (nNextTurn && automaticTask)
            NextTurnNotify(NULL);
#else
        {
            char *line = g_strdup("play");
            HandleCommand(line, acTop);
            g_free(line);
            while (fNextTurn)
                NextTurn(TRUE);
        }
#endif
    } while (ms.gs == GAME_PLAYING && automaticTask);

    outputx();
    ap[0].pt = pt_store[0];
    ap[1].pt = pt_store[1];
    ap[0].esChequer.ec = ec_cheq_store[0];
    ap[1].esChequer.ec = ec_cheq_store[1];
    ap[0].esCube.ec = ec_cube_store[0];
    ap[1].esCube.ec = ec_cube_store[1];

    fAutoGame = fAutoGame_store;
    fDisplay = fDisplay_store;
    fQuiet = fQuiet_store;
    fEndGame = FALSE;

    /* If the game ended in a resign then make sure the
     * the state for the user truly reflects the game is over
     * and that no further action from them is necessary */
    if (ms.fResigned) {
        ms.fResigned = 0;
        ShowBoard();
    }

    if (manual_dice) {
        outputoff();
        SetRNG(&rngCurrent, rngctxCurrent, RNG_MANUAL, "");
        outputon();
    }
#if USE_BOARD3D
    if (fX && bd)
        ResumeDiceRolling(bd->rd);
#endif

    if (!automaticTask)
        return;
    StopAutomaticPlay();

    if (fAutoGame && (!ms.nMatchTo || (ms.anScore[0] < ms.nMatchTo && ms.anScore[1] < ms.nMatchTo)))
        StartNewGame();
}

extern void
CommandPlay(char *UNUSED(sz))
{
    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    if (ap[ms.fTurn].pt == PLAYER_HUMAN) {
        outputl(_("It's not the computer's turn to play."));
        return;
    }

    StartAutomaticPlay();

    fComputing = TRUE;

    if (!ComputerTurn())
        TurnDone();

    fComputing = FALSE;
#if USE_GTK
    NextTurnNotify(NULL);
#endif
}

static void
CommandPreviousGame(char *sz)
{

    int n;
    char *pch;
    listOLD *pl;

    if ((pch = NextToken(&sz)))
        n = ParseNumber(&pch);
    else
        n = 1;

    if (n < 1) {
        outputl(_("If you specify a parameter to the `previous game' command, "
                  "it must be a positive number (the count of games to step " "back)."));
        return;
    }

    for (pl = lMatch.plNext; pl->p != plGame && pl != &lMatch; pl = pl->plNext);

    if (pl->p != plGame)
        /* current game not found */
        return;

    for (; n && pl->plPrev->p; n--, pl = pl->plPrev);

    if (pl->p == plGame)
        return;

    ChangeGame(pl->p);
}

static void
CommandPreviousRoll(char *UNUSED(sz))
{
    moverecord *pmr;

    if (!plLastMove || !plLastMove->p)
        /* silently ignore */
        return;
    pmr = plLastMove->plNext->p;
    if (!ms.anDice[0]) {
        /* if the dice haven't been rolled skip the entire move */
        CommandPrevious(NULL);

        g_assert(plLastMove->plNext);

        if (plLastMove->plNext && plLastMove->plNext->p != pmr) {
            pmr = plLastMove->plNext->p;
            if (pmr && (pmr->mt == MOVE_NORMAL || pmr->mt == MOVE_GAMEINFO))
                /* We've stepped back a whole move; now we need to recover the
                 * previous dice roll. */
                CommandNextRoll(NULL);
        }

    } else {

        CalculateBoard();

        ms.anDice[0] = 0;
        ms.anDice[1] = 0;
#if USE_GTK
        if (fX) {
            BoardData *bd = BOARD(pwBoard)->board_data;
            bd->diceRoll[0] = bd->diceRoll[1] = -1;
        }
#endif

        if (pmr)
            FixMatchState(&ms, pmr);

	UpdateGame(FALSE);
	
#ifdef WEB
	ShowBoard();
#endif /* WEB */
    }
}

static void
CommandPreviousRolled(char *UNUSED(sz))
{
    moverecord *pmr;

    if (!plLastMove || !plLastMove->p)
        /* silently ignore */
        return;

    /* step back and boogie */

    CommandPrevious(NULL);

    if (plLastMove && plLastMove->plNext && (pmr = plLastMove->plNext->p) && pmr->mt == MOVE_NORMAL)
        CommandNextRoll(NULL);

}

extern void
CommandPrevious(char *sz)
{

    int n;
    char *pch;
    int mark = FALSE;
    int cmark = FALSE;
    listOLD *p;
    moverecord *pmr = NULL;

    if (!plGame) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }
#if USE_BOARD3D
    RestrictiveRedraw();
#endif

    n = 1;

    if ((pch = NextToken(&sz))) {
        if (!StrCaseCmp(pch, "game")) {
            CommandPreviousGame(sz);
            return;
        } else if (!StrCaseCmp(pch, "rolled")) {
            CommandPreviousRolled(sz);
            return;
        } else if (!StrCaseCmp(pch, "roll")) {
            CommandPreviousRoll(sz);
            return;
        } else if (!StrCaseCmp(pch, "marked")) {
            mark = TRUE;
            if ((pch = NextToken(&sz))) {
                n = ParseNumber(&pch);
            }
        } else if (!StrCaseCmp(pch, "cmarked")) {
            cmark = TRUE;
            if ((pch = NextToken(&sz))) {
                n = ParseNumber(&pch);
            }
        } else if (!StrCaseCmp(pch, "anymarked")) {
            mark = TRUE;
            cmark = TRUE;
            if ((pch = NextToken(&sz))) {
                n = ParseNumber(&pch);
            }
        } else
            n = ParseNumber(&pch);
    }

    if (n < 1) {
        outputl(_("If you specify a parameter to the `previous' command, it "
                  "must be a positive number (the count of moves to step " "back)."));
        return;
    }

    if (mark || cmark) {
        listOLD *pgame;
        p = plLastMove;

        for (pgame = lMatch.plNext; pgame->p != plGame && pgame != &lMatch; pgame = pgame->plNext);

        if (pgame->p != plGame)
            return;

        while ((p->p) != 0) {
            pmr = (moverecord *) p->p;
            if (mark && MoveIsMarked(pmr) && (--n <= 0))
                break;
            if (cmark && MoveIsCMarked(pmr) && (--n <= 0))
                break;

            p = p->plPrev;
            if (!(p->p)) {
                if (pgame->plPrev == pgame)
                    return;
                pgame = pgame->plPrev;
                if (!(pgame->p))
                    return;
                p = ((listOLD *) pgame->p)->plNext;
                while (p->plNext->p)
                    p = p->plNext;
                ;
            }
        }

        if (p->p == 0)
            return;

        if (pgame->p != plGame)
            ChangeGame(pgame->p);

        plLastMove = p->plPrev;
    } else {
        while (n-- && plLastMove->plPrev->p)
            plLastMove = plLastMove->plPrev;
    }

    CalculateBoard();

    if (pmr)
        ShowMark(pmr);

    UpdateGame(FALSE);

    if (plLastMove->plNext && plLastMove->plNext->p)
        FixMatchState(&ms, plLastMove->plNext->p);

    SetMoveRecord(plLastMove->p);
#ifdef WEB
    ShowBoard();
#endif /* WEB */
}

extern void
CommandRedouble(char *UNUSED(sz))
{
    moverecord *pmr;

    if (ms.nMatchTo > 0) {
        outputl(_("Beavers and Raccoons are not permitted during match play."));

        return;
    }

    if (!nBeavers) {
        outputl(_("Beavers are disabled (see `help set beavers')."));

        return;
    }

    if (ms.cBeavers >= nBeavers) {
        if (nBeavers == 1)
            outputl(_("Only one beaver is permitted (see `help set " "beavers')."));
        else
            outputf(_("Only %d beavers are permitted (see `help set " "beavers').\n"), nBeavers);

        return;
    }

    if (ms.gs != GAME_PLAYING || !ms.fDoubled) {
        outputl(_("The cube must have been offered before you can redouble " "it."));

        return;
    }

    if (ap[ms.fTurn].pt != PLAYER_HUMAN && !fComputerDecision) {
        outputl(_("It is the computer's turn -- type `play' to force it to " "move immediately."));
        return;
    }

    if (ms.nCube >= (MAX_CUBE >> 1)) {
        outputf(_("The cube is already at %d; you can't double any more.\n"), MAX_CUBE);
        return;
    }

    if (!move_not_last_in_match_ok())
        return;

    pmr = NewMoveRecord();

    playSound(SOUND_REDOUBLE);

    if (fDisplay)
        outputf(_("%s accepts and immediately redoubles to %d.\n"), ap[ms.fTurn].szName, ms.nCube << 2);

    ms.fCubeOwner = !ms.fMove;
    UpdateSetting(&ms.fCubeOwner);

    pmr->mt = MOVE_DOUBLE;
    pmr->fPlayer = ms.fTurn;
    LinkToDouble(pmr);
    AddMoveRecord(pmr);

    TurnDone();
}

extern void
CommandReject(char *sz)
{

    if (ms.fResigned)
        CommandDecline(sz);
    else if (ms.fDoubled)
        CommandDrop(sz);
    else
        outputl(_("You can only reject if the cube or a resignation has been " "offered."));
}

extern void
CommandResign(char *sz)
{

    char *pch;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("You must be playing a game if you want to resign it."));

        return;
    }

    if (ap[ms.fTurn].pt != PLAYER_HUMAN && !fComputerDecision) {
        outputl(_("It is the computer's turn -- type `play' to force it to " "move immediately."));
        return;
    }

    /* FIXME cancel cube action?  or refuse resignations while doubled?
     * or treat resignations while doubled as drops? */

    if ((pch = NextToken(&sz))) {
        const size_t cch = strlen(pch);

        if (!StrNCaseCmp("single", pch, cch) || !StrNCaseCmp("normal", pch, cch))
            ms.fResigned = 1;
        else if (!StrNCaseCmp("gammon", pch, cch))
            ms.fResigned = 2;
        else if (!StrNCaseCmp("backgammon", pch, cch))
            ms.fResigned = 3;
        else
            ms.fResigned = atoi(pch);
    } else
        ms.fResigned = 1;

    if (ms.fResigned != -1 && (ms.fResigned < 1 || ms.fResigned > 3)) {
        ms.fResigned = 0;

        outputf(_("Unknown keyword `%s' (see `help resign').\n"), pch);

        return;
    }

    if (ms.fResigned >= 0 && ms.fResigned <= ms.fResignationDeclined) {
        ms.fResigned = 0;

        outputf(_("%s has already declined your offer of a %s.\n"),
                ap[!ms.fTurn].szName, gettext(aszGameResult[ms.fResignationDeclined - 1]));

        return;
    }

    if (fDisplay) {
        if (ms.fResigned > 0)
            outputf(_("%s offers to resign a %s.\n"), ap[ms.fTurn].szName, gettext(aszGameResult[ms.fResigned - 1]));
        else
            outputf(_("%s offers to resign.\n"), ap[ms.fTurn].szName);
    }


    ms.fTurn = !ms.fTurn;
    playSound(SOUND_RESIGN);

    TurnDone();
}

/* evaluate wisdom of not having doubled/redoubled */

extern void
CommandRoll(char *UNUSED(sz))
{
    movelist ml;
    moverecord *pmr;

    if (ms.gs != GAME_PLAYING) {
        outputl(_("No game in progress (type `new game' to start one)."));

        return;
    }

    if (ap[ms.fTurn].pt != PLAYER_HUMAN) {
        outputl(_("It is the computer's turn -- type `play' to force it to " "move immediately."));
        return;
    }

    if (ms.fDoubled) {
        outputf(_("Please wait for %s to consider the cube before " "moving.\n"), ap[ms.fTurn].szName);

        return;
    }

    if (ms.fResigned) {
        outputl(_("Please resolve the resignation first."));

        return;
    }

    if (ms.anDice[0]) {
        outputl(_("You have already rolled the dice."));

        return;
    }

    if (!move_not_last_in_match_ok())
        return;

    if (fTutor && fTutorCube && !GiveAdvice(tutor_double(FALSE)))
        return;

    if (!fCheat || CheatDice(ms.anDice, &ms, afCheatRoll[ms.fMove]))
        if (RollDice(ms.anDice, &rngCurrent, rngctxCurrent) < 0)
            return;

    pmr = NewMoveRecord();

    pmr->mt = MOVE_SETDICE;
    pmr->anDice[0] = MAX(ms.anDice[0], ms.anDice[1]);
    pmr->anDice[1] = MIN(ms.anDice[0], ms.anDice[1]);
    pmr->fPlayer = ms.fTurn;

    AddMoveRecord(pmr);

    DiceRolled();

#if USE_GTK
    if (fX) {

        outputnew();
        outputf(_("%s rolls %1i and %1i.\n"), ap[ms.fTurn].szName, ms.anDice[0], ms.anDice[1]);
        outputx();

    }
#endif

    ResetDelayTimer();

    if (!GenerateMoves(&ml, msBoard(), ms.anDice[0], ms.anDice[1], FALSE)) {

        playSound(ap[ms.fTurn].pt == PLAYER_HUMAN ? SOUND_HUMAN_DANCE : SOUND_BOT_DANCE);

        pmr = NewMoveRecord();

        pmr->mt = MOVE_NORMAL;
        pmr->anDice[0] = MAX(ms.anDice[0], ms.anDice[1]);
        pmr->anDice[1] = MIN(ms.anDice[0], ms.anDice[1]);
        pmr->fPlayer = ms.fTurn;

        ShowAutoMove(msBoard(), pmr->n.anMove);

        AddMoveRecord(pmr);

        TurnDone();
    } else if (ml.cMoves == 1 && (fAutoMove || (ClassifyPosition(msBoard(), ms.bgv)
                                                <= CLASS_BEAROFF1 && fAutoBearoff))) {

        pmr = NewMoveRecord();

        pmr->mt = MOVE_NORMAL;
        pmr->anDice[0] = MAX(ms.anDice[0], ms.anDice[1]);
        pmr->anDice[1] = MIN(ms.anDice[0], ms.anDice[1]);
        pmr->fPlayer = ms.fTurn;
        memcpy(pmr->n.anMove, ml.amMoves[0].anMove, sizeof(pmr->n.anMove));

        ShowAutoMove(msBoard(), pmr->n.anMove);

        AddMoveRecord(pmr);
        TurnDone();
    } else if (fAutoBearoff && !TryBearoff())
        TurnDone();
}



extern void
CommandTake(char *UNUSED(sz))
{
    moverecord *pmr;

    if (ms.gs != GAME_PLAYING || !ms.fDoubled) {
        outputl(_("The cube must have been offered before you can take it."));

        return;
    }

    if (ap[ms.fTurn].pt != PLAYER_HUMAN && !fComputerDecision) {
        outputl(_("It is the computer's turn -- type `play' to force it to " "move immediately."));
        return;
    }

    if (!move_not_last_in_match_ok())

        playSound(SOUND_TAKE);

    pmr = NewMoveRecord();

    pmr->mt = MOVE_TAKE;
    pmr->fPlayer = ms.fTurn;
    if (!LinkToDouble(pmr)) {
        free(pmr);
        return;
    }

    pmr->stCube = SKILL_NONE;

    if (fTutor && fTutorCube && !GiveAdvice(tutor_take(TRUE))) {
        free(pmr);              /* garbage collect */
        return;
    }

    if (fDisplay)
        outputf(_("%s accepts the cube at %d.\n"), ap[ms.fTurn].szName, ms.nCube << 1);

    AddMoveRecord(pmr);

    UpdateSetting(&ms.nCube);
    UpdateSetting(&ms.fCubeOwner);

    TurnDone();
}

extern void
SetMatchID(const char *szMatchID)
{

    int anScore[2];
    unsigned int anDice[2];
    int nMatchTo, fCubeOwner, fMove, fCrawford, nCube;
    int fTurn, fDoubled, fResigned, oneAway;
    int lfJacoby;               /* for local copy of global jacoby variable */

    gamestate gs;
    char szID[15];
    moverecord *pmr;

    if (!szMatchID || !*szMatchID)
        return;

    if (ms.gs == GAME_PLAYING)
        strcpy(szID, PositionID(msBoard()));
    else
        strcpy(szID, "");

    lfJacoby = fJacoby;
    if (MatchFromID(anDice, &fTurn, &fResigned, &fDoubled, &fMove, &fCubeOwner, &fCrawford,
                    &nMatchTo, anScore, &nCube, &lfJacoby, &gs, szMatchID) < 0) {
        outputf(_("Illegal match ID '%s'\n"), szMatchID);
        outputf(_("Dice %d %d, "), anDice[0], anDice[1]);
        outputf(_("player on roll %d (turn %d), "), fMove, fTurn);
        outputf(_("resigned %d,\n"), fResigned);
        outputf(_("doubled %d, "), fDoubled);
        outputf(_("cube owner %d, "), fCubeOwner);
        outputf(_("crawford game %d,\n"), fCrawford);
        outputf(_("jacoby %d,\n"), lfJacoby);
        outputf(_("match length %d, "), nMatchTo);
        outputf(_("score %d-%d, "), anScore[0], anScore[1]);
        outputf(_("cube %d, "), nCube);
        outputf(_("game state %d\n"), (int) gs);
        outputx();
        return;
    }

    if (fDoubled) {
        outputl(_("SetMatchID cannot handle positions where a double has been offered."));
        outputf(_("Stepping back to the offering of the cube. "));
        fMove = fTurn = !fTurn;
        fDoubled = 0;
    }

    if (nMatchTo == 1)
        fCrawford = 0;

    /* start new match or session */

    FreeMatch();

    ms.cGames = 0;
    ms.nMatchTo = nMatchTo;
    ms.anScore[0] = anScore[0];
    ms.anScore[1] = anScore[1];
    oneAway = (anScore[0] == nMatchTo - 1) || (anScore[1] == nMatchTo - 1);
    ms.fCrawford = fCrawford && oneAway;
    ms.fPostCrawford = !fCrawford && oneAway;
    ms.bgv = bgvDefault;        /* FIXME: include bgv in match ID */
    ms.fCubeUse = fCubeUse;     /* FIXME: include cube use in match ID */
    ms.fJacoby = lfJacoby;      /* FIXME: include Jacoby in match ID */

    /* start new game */

    PopGame(plGame, TRUE);

    InitBoard(ms.anBoard, ms.bgv);

    ClearMoveRecord();

    ListInsert(&lMatch, plGame);

    pmr = NewMoveRecord();

    pmr->mt = MOVE_GAMEINFO;

    pmr->g.i = ms.cGames;
    pmr->g.nMatch = ms.nMatchTo;
    pmr->g.anScore[0] = ms.anScore[0];
    pmr->g.anScore[1] = ms.anScore[1];
    pmr->g.fCrawford = fAutoCrawford && ms.nMatchTo > 1;
    pmr->g.fCrawfordGame = ms.fCrawford;
    pmr->g.fJacoby = ms.fJacoby && !ms.nMatchTo;
    pmr->g.fWinner = -1;
    pmr->g.nPoints = 0;
    pmr->g.fResigned = FALSE;
    pmr->g.nAutoDoubles = 0;
    pmr->g.bgv = ms.bgv;
    pmr->g.fCubeUse = ms.fCubeUse;
    IniStatcontext(&pmr->g.sc);
    AddMoveRecord(pmr);
    AddGame(pmr);

    ms.gs = gs;
    ms.fMove = fMove;
    ms.fTurn = fTurn;
    ms.fResigned = fResigned;
    ms.fDoubled = fDoubled;
    ms.fJacoby = lfJacoby;

    if (anDice[0]) {
        char sz[10];
        sprintf(sz, "%u %u", anDice[0], anDice[1]);
        CommandSetDice(sz);

    }

    if (fCubeOwner != -1) {
        pmr = NewMoveRecord();
        pmr->mt = MOVE_SETCUBEPOS;
        pmr->scp.fCubeOwner = fCubeOwner;
        AddMoveRecord(pmr);
    }

    if (nCube != 1) {
        char sz[10];
        sprintf(sz, "%d", nCube);
        CommandSetCubeValue(sz);
    }

    if (strlen(szID))
        CommandSetBoard(szID);

    UpdateSetting(&ms.gs);
    UpdateSetting(&ms.nCube);
    UpdateSetting(&ms.fCubeOwner);
    UpdateSetting(&ms.fTurn);
    UpdateSetting(&ms.fCrawford);
    UpdateSetting(&ms.fJacoby);

    /* make sure that the hint record has the player on turn */
    get_current_moverecord(NULL);

    ShowBoard();
}


extern void
FixMatchState(matchstate * pms, const moverecord * pmr)
{
    switch (pmr->mt) {
    case MOVE_NORMAL:
        if (pms->fTurn != pmr->fPlayer) {
            /* previous moverecord is missing */
            SwapSides(pms->anBoard);
            pms->fMove = pms->fTurn = pmr->fPlayer;
        }
        break;
    case MOVE_DOUBLE:
        if (pms->fTurn != pmr->fPlayer) {
            /* previous record is missing: this must be an normal double */
            SwapSides(pms->anBoard);
            pms->fMove = pms->fTurn = pmr->fPlayer;
        }
        break;
    default:
        /* no-op */
        break;
    }

}

extern void
pmr_cubedata_set(moverecord * pmr, evalsetup * pes, float output[][NUM_ROLLOUT_OUTPUTS],
                 float stddev[][NUM_ROLLOUT_OUTPUTS])
{
    pmr->CubeDecPtr->esDouble = *pes;
    pmr->stCube = SKILL_NONE;
    memcpy(pmr->CubeDecPtr->aarOutput, output, sizeof(pmr->CubeDecPtr->aarOutput));
    memcpy(pmr->CubeDecPtr->aarStdDev, stddev, sizeof(pmr->CubeDecPtr->aarStdDev));
}

extern void
pmr_movelist_set(moverecord * pmr, evalsetup * pes, movelist * pml)
{
    float skill_score;
    pmr->esChequer = *pes;
    pmr->stCube = SKILL_NONE;
    pmr->ml = *pml;
    pmr->n.iMove = locateMove(msBoard(), pmr->n.anMove, &pmr->ml);
    if (pmr->ml.cMoves > 0)
        skill_score = pmr->ml.amMoves[pmr->n.iMove].rScore - pmr->ml.amMoves[0].rScore;
    else
        skill_score = 0.0f;
    pmr->n.stMove = Skill(skill_score);
}

extern void
current_pmr_cubedata_update(evalsetup * pes, float output[][NUM_ROLLOUT_OUTPUTS], float stddev[][NUM_ROLLOUT_OUTPUTS])
{
    moverecord *pmr = get_current_moverecord(NULL);
    if (!pmr)
        return;
    if (pmr->CubeDecPtr->esDouble.et == EVAL_NONE)
        pmr_cubedata_set(pmr, pes, output, stddev);
}

extern moverecord *
get_current_moverecord(int *pfHistory)
{
    if (plLastMove && plLastMove->plNext && plLastMove->plNext->p) {
        if (pfHistory)
            *pfHistory = TRUE;
        return plLastMove->plNext->p;
    }

    if (pfHistory)
        *pfHistory = FALSE;

    if (ms.gs != GAME_PLAYING) {
        pmr_hint_destroy();
        return NULL;
    }

    /* invalidate on changed dice */
    if (ms.anDice[0] > 0 && pmr_hint && pmr_hint->anDice[0] > 0 && (pmr_hint->anDice[0] != ms.anDice[0]
                                                                    || pmr_hint->anDice[1] != ms.anDice[1]))
        pmr_hint_destroy();

    if (!pmr_hint) {
        if ((pmr_hint = NewMoveRecord()))
            pmr_hint->fPlayer = ms.fTurn;
        else
            return NULL;
    }

    if (ms.anDice[0] > 0) {
        pmr_hint->mt = MOVE_NORMAL;
        pmr_hint->anDice[0] = MAX(ms.anDice[0], ms.anDice[1]);
        pmr_hint->anDice[1] = MIN(ms.anDice[0], ms.anDice[1]);
    } else if (ms.fDoubled) {
        pmr_hint->mt = MOVE_TAKE;
    } else {
        pmr_hint->mt = MOVE_DOUBLE;
        pmr_hint->fPlayer = ms.fTurn;
    }
    return pmr_hint;
}

extern gboolean
game_is_last(listOLD * plGame)
{
    listOLD *pl;
    for (pl = lMatch.plNext; pl->p != plGame && pl != &lMatch; pl = pl->plNext) {
    }
    if (pl->p != plGame || pl->plNext != &lMatch)
        return FALSE;
    else
        return TRUE;
}

extern listOLD *
game_add_pmr_hint(listOLD * plGame)
{
    g_return_val_if_fail(plGame, NULL);
    g_return_val_if_fail(game_is_last(plGame), NULL);

    if (pmr_hint)
        return (ListInsert(plGame, pmr_hint));
    else
        return NULL;
}

extern void
game_remove_pmr_hint(listOLD * pl_hint)
{
    g_return_if_fail(pl_hint);
    g_return_if_fail(pl_hint->p == pmr_hint);
    ListDelete(pl_hint);
}

extern void
pmr_hint_destroy(void)
{
    if (!pmr_hint)
        return;
    FreeMoveRecord(pmr_hint);
    pmr_hint = NULL;
}

static void
 OptimumRoll(TanBoard anBoard, const cubeinfo * pci, const evalcontext * pec, const int fBest, unsigned int anDice[2]);

static int
CheatDice(unsigned int anDice[2], matchstate * pms, const int fBest)
{

    static evalcontext ec0ply = { FALSE, 0, FALSE, TRUE, 0.0 };
    static cubeinfo ci;

    GetMatchStateCubeInfo(&ci, pms);

    OptimumRoll(pms->anBoard, &ci, &ec0ply, fBest, anDice);

    if (!anDice[0])
        return -1;

    return 0;

}

/* 
 * find best (or worst roll possible)
 *
 */

typedef struct _rollequity {
    float r;
    int anDice[2];
} rollequity;

static int
CompareRollEquity(const void *p1, const void *p2)
{

    const rollequity *per1 = (const rollequity *) p1;
    const rollequity *per2 = (const rollequity *) p2;

    if (per1->r > per2->r)
        return -1;
    else if (per1->r < per2->r)
        return +1;
    else
        return 0;

}

static void
OptimumRoll(TanBoard anBoard, const cubeinfo * pci, const evalcontext * pec, const int fBest, unsigned int anDice[2])
{

    int i, j, k;
    float ar[NUM_ROLLOUT_OUTPUTS];
    rollequity are[21];
    float r;

    anDice[0] = anDice[1] = 0;

    for (i = 1, k = 0; i <= 6; i++)
        for (j = 1; j <= i; j++, ++k) {

            EvaluateRoll(ar, i, j, (ConstTanBoard) anBoard, pci, pec);

            r = (pec->fCubeful) ? ar[OUTPUT_CUBEFUL_EQUITY] : ar[OUTPUT_EQUITY];

            if (pci->nMatchTo)
                r = 1.0f - r;
            else
                r = -r;

            are[k].r = r;
            are[k].anDice[0] = i;
            are[k].anDice[1] = j;

        }

    qsort(are, 21, sizeof(rollequity), CompareRollEquity);

    anDice[0] = are[fBest].anDice[0];
    anDice[1] = are[fBest].anDice[1];

}

extern void
EvaluateRoll(float ar[NUM_ROLLOUT_OUTPUTS], int nDie1, int nDie2, const TanBoard anBoard,
             const cubeinfo * pci, const evalcontext * pec)
{
    TanBoard anBoardTemp;
    cubeinfo ciOpp;

    memcpy(&ciOpp, pci, sizeof(cubeinfo));
    ciOpp.fMove = !pci->fMove;

    memcpy(&anBoardTemp[0][0], &anBoard[0][0], 2 * 25 * sizeof(int));

    if (FindBestMove(NULL, nDie1, nDie2, anBoardTemp, pci, NULL, defaultFilters) < 0)
        g_assert_not_reached();

    SwapSides(anBoardTemp);

    GeneralEvaluationE(ar, (ConstTanBoard) anBoardTemp, &ciOpp, pec);

    return;
}

/* routine to link doubles/beavers/raccoons/etc and their eventual
 * take/drop decisions into a single evaluation data block
 * returns NULL if there is no preceding double record to link to
 */

moverecord *
LinkToDouble(moverecord * pmr)
{

    moverecord *prev;


    if (!plLastMove || ((prev = plLastMove->p) == 0) || prev->mt != MOVE_DOUBLE)
        return 0;

    /* link the evaluation data */
    pmr->CubeDecPtr = prev->CubeDecPtr;

    /* if this is part of a chain of doubles, add to the count
     * nAnimals will be 0 for the first double, 1 for the beaver,
     * 2 for the racoon, etc.
     */
    if (pmr->mt == MOVE_DOUBLE)
        pmr->nAnimals = 1 + prev->nAnimals;

    return pmr;
}

/*
 * getFinalScore:
 * fills anScore[ 2 ] with the final score of the match/session
 */

extern int
getFinalScore(int *anScore)
{
    listOLD *plGame;

    /* find last game */
    for (plGame = lMatch.plNext; plGame->plNext->p; plGame = plGame->plNext);

    if (plGame->p && ((listOLD *) plGame->p)->plNext &&
        ((listOLD *) plGame->p)->plNext->p &&
        ((moverecord *) ((listOLD *) plGame->p)->plNext->p)->mt == MOVE_GAMEINFO) {
        anScore[0] = ((moverecord *) ((listOLD *) plGame->p)->plNext->p)->g.anScore[0];
        anScore[1] = ((moverecord *) ((listOLD *) plGame->p)->plNext->p)->g.anScore[1];
        if (((moverecord *) ((listOLD *) plGame->p)->plNext->p)->g.fWinner != -1)
            anScore[((moverecord *) ((listOLD *) plGame->p)->plNext->p)->g.fWinner] +=
                ((moverecord *) ((listOLD *) plGame->p)->plNext->p)->g.nPoints;
        return TRUE;
    }

    return FALSE;
}

extern const char *
GetMoveString(moverecord * pmr, int *pPlayer, gboolean addSkillMarks)
{
    doubletype dt;
    static char sz[40];
    const char *pch = NULL;
    TanBoard anBoard;
    *pPlayer = 0;

    switch (pmr->mt) {
    case MOVE_GAMEINFO:
        /* no need to list this */
        break;

    case MOVE_NORMAL:
        *pPlayer = pmr->fPlayer;
        pch = sz;
        sz[0] = (char) (MAX(pmr->anDice[0], pmr->anDice[1]) + '0');
        sz[1] = (char) (MIN(pmr->anDice[0], pmr->anDice[1]) + '0');
        sz[2] = ':';
        sz[3] = ' ';
        FormatMove(sz + 4, msBoard(), pmr->n.anMove);
        if (addSkillMarks) {
            strcat(sz, aszSkillTypeAbbr[pmr->n.stMove]);
            strcat(sz, aszSkillTypeAbbr[pmr->stCube]);
        }
        break;

    case MOVE_DOUBLE:
        *pPlayer = pmr->fPlayer;
        pch = sz;

        switch ((dt = DoubleType(ms.fDoubled, ms.fMove, ms.fTurn))) {
        case DT_NORMAL:
            sprintf(sz, (ms.fCubeOwner == -1) ? _("Double to %d") : _("Redouble to %d"), ms.nCube << 1);
            break;
        case DT_BEAVER:
        case DT_RACCOON:
            sprintf(sz, (dt == DT_BEAVER) ? _("Beaver to %d") : _("Raccoon to %d"), ms.nCube << 2);
            break;
        default:
            g_assert_not_reached();
        }
        if (addSkillMarks)
            strcat(sz, aszSkillTypeAbbr[pmr->stCube]);
        break;

    case MOVE_TAKE:
        *pPlayer = pmr->fPlayer;
        strcpy(sz, _("Take"));
        pch = sz;
        if (addSkillMarks)
            strcat(sz, aszSkillTypeAbbr[pmr->stCube]);
        break;

    case MOVE_DROP:
        *pPlayer = pmr->fPlayer;
        strcpy(sz, _("Drop"));
        pch = sz;
        if (addSkillMarks)
            strcat(sz, aszSkillTypeAbbr[pmr->stCube]);
        break;

    case MOVE_RESIGN:
        *pPlayer = pmr->fPlayer;
        pch = _(" Resigns");    /* FIXME show value */
        break;

    case MOVE_SETDICE:
        *pPlayer = pmr->fPlayer;
        sprintf(sz, _("Rolled %d%d"), MAX(pmr->anDice[0], pmr->anDice[1]), MIN(pmr->anDice[0], pmr->anDice[1]));
        pch = sz;
        break;

    case MOVE_SETBOARD:
        *pPlayer = -1;
        /* PositionID assumes player 0 on roll. If this is not
         * the case, swap board first and restore it when done */
        if (pmr->fPlayer) {
            PositionFromKey(anBoard, &pmr->sb.key);
            SwapSides(anBoard);
            PositionKey((ConstTanBoard) anBoard, &pmr->sb.key);
        }
        sprintf(sz, " (set board %s)", PositionIDFromKey(&pmr->sb.key));
        if (pmr->fPlayer) {
            SwapSides(anBoard);
            PositionKey((ConstTanBoard) anBoard, &pmr->sb.key);
        }
        pch = sz;
        break;

    case MOVE_SETCUBEPOS:
        *pPlayer = -1;
        if (pmr->scp.fCubeOwner < 0)
            sprintf(sz, " (set cube centre)");
        else
            sprintf(sz, " (set cube owner %s)", ap[pmr->scp.fCubeOwner].szName);
        pch = sz;
        break;

    case MOVE_SETCUBEVAL:
        *pPlayer = -1;
        sprintf(sz, " (set cube value %d)", pmr->scv.nCube);
        pch = sz;
        break;
    default:
        g_assert_not_reached();
    }
    return pch;
}

#define STRBUF_SIZE 1024

static void
AddString(listOLD * buffers, char *str)
{
    listOLD *pCurrent = buffers->plPrev;
    if (!pCurrent->p || strlen(str) + strlen(pCurrent->p) > STRBUF_SIZE) {
        char *newBuf = malloc(STRBUF_SIZE + 1);
        *newBuf = '\0';
        pCurrent = ListInsert(buffers, newBuf);
    }
    strcat(pCurrent->p, str);
}

extern char *
GetMatchCheckSum(void)
{
    static char auchHex[33];    /* static buffer that holds result, not ideal but not hurting anyone */
    unsigned char auch[16];
    int i;
    /* Work out md5 checksum */
    char *gameStr;
    size_t size;
    char buf[1024];
    listOLD *pList, *plGame, *plMove;
    listOLD buffers;
    ListCreate(&buffers);

    sprintf(buf, "%s vs %s (%d)", ap[0].szName, ap[1].szName, ms.nMatchTo);
    AddString(&buffers, buf);

    for (plGame = lMatch.plNext; plGame->p; plGame = plGame->plNext) {
        listOLD *plStart = plGame->p;
        int move = 1;
        for (plMove = plStart->plNext; plMove->p; plMove = plMove->plNext) {
            char playerStr[4] = ".AB";
            int player;
            moverecord *pmr = plMove->p;
            const char *moveString = GetMoveString(pmr, &player, FALSE);
            if (moveString) {
                sprintf(buf, " %d%c %s", move, playerStr[player + 1], moveString);
                AddString(&buffers, buf);
                if (player == 1)
                    move++;
            }
        }
    }

    /* Create single string from buffers */
    size = 0;
    for (pList = buffers.plNext; pList->p; pList = pList->plNext)
        size += strlen(pList->p);

    gameStr = malloc(size + 1);
    *gameStr = '\0';
    for (pList = buffers.plNext; pList->p; pList = pList->plNext)
        strcat(gameStr, pList->p);

    ListDeleteAll(&buffers);

    md5_buffer(gameStr, strlen(gameStr), auch);
    free(gameStr);
    /* Convert to hex so stores easily in database - is there a better way? */
    for (i = 0; i < 16; i++)
        sprintf(auchHex + (i * 2), "%02x", auch[i]);

    return auchHex;
}

/*
 * get name number
 *
 * Input
 *    plGame: the game for which the game number is requested
 *
 * Returns:
 *    the game number
 *
 */

extern int
getGameNumber(const listOLD * plGame)
{

    listOLD *pl;
    int iGame;

    for (pl = lMatch.plNext, iGame = 0; pl != &lMatch; pl = pl->plNext, iGame++)
        if (pl->p == plGame)
            return iGame;

    return -1;

}


/*
 * get move number
 *
 * Input
 *    plGame: the game 
 *    p: the move
 *
 * Returns:
 *    the move number
 *
 */

extern int
getMoveNumber(const listOLD * plGame, const void *p)
{

    listOLD *pl;
    int iMove;

    for (pl = plGame->plNext, iMove = 0; pl != plGame; pl = pl->plNext, iMove++)
        if (p == pl->p)
            return iMove;

    if (p == pmr_hint)
        return iMove;

    return -1;

}


extern void
CommandClearTurn(char *UNUSED(sz))
{
    moverecord *pmr;
    if (ms.gs != GAME_PLAYING) {
        outputl(_("There must be a game in progress to set a player on roll."));
        return;
    }

    if (ms.fResigned) {
        outputl(_("Please resolve the resignation first."));
        return;
    }

    if (!move_not_last_in_match_ok())
        return;

    while ((pmr = plLastMove->p) && (pmr->mt == MOVE_DOUBLE || pmr->mt == MOVE_SETDICE))
        PopMoveRecord(plLastMove);

    pmr_hint_destroy();
    fNextTurn = FALSE;
#if USE_GTK
    if (fX) {
        BoardData *bd = BOARD(pwBoard)->board_data;
        bd->diceRoll[0] = bd->diceRoll[1] = -1;
    }
#endif
    ms.anDice[0] = ms.anDice[1] = 0;

    UpdateSetting(&ms.fTurn);

#if USE_GTK
    if (fX)
        ShowBoard();
#endif                          /* USE_GTK */
}
