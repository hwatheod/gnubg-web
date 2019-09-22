/*
 * export.c
 *
 * by Joern Thyssen  <jthyssen@dk.ibm.com>, 2002
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
 * $Id: export.c,v 1.82 2015/04/12 22:00:25 plm Exp $
 */

#include "config.h"
#include "backgammon.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_LIBPNG
#include <png.h>
#endif

#include "analysis.h"
#include "drawboard.h"
#include "export.h"
#include "eval.h"
#include "positionid.h"
#include "renderprefs.h"
#include "matchid.h"
#include "boardpos.h"
#include "boarddim.h"

#if HAVE_PANGOCAIRO
#include <cairo.h>
#include <cairo-svg.h>
#include <cairo-pdf.h>
#include <cairo-ps.h>
#include "simpleboard.h"
#endif

/*
 * Get filename from base file and game number:
 *
 * Number 0 gets the value of szBase.
 * Assume szBase is "blah.*", then number 1 will be
 * "blah_001.*", and so forth.
 *
 * Input:
 *   szBase: base filename
 *   iGame: game number
 *
 * Garbage collect:
 *   Caller must free returned pointer if not NULL
 * 
 */

extern char *
filename_from_iGame(const char *szBase, const int iGame)
{
    if (!iGame)
        return g_strdup(szBase);
    else {
        char *sz = g_malloc(strlen(szBase) + 5);
        char *szExtension = strrchr(szBase, '.');
        char *pc;
        if (!szExtension) {
            sprintf(sz, "%s_%03d", szBase, iGame + 1);
            return sz;
        } else {
            strcpy(sz, szBase);
            pc = strrchr(sz, '.');
            sprintf(pc, "_%03d%s", iGame + 1, szExtension);
            return sz;
        }
    }
}

#if HAVE_PANGOCAIRO
static gchar *
export_get_filename(char *sz)
{
    gchar *filename = NULL;

    filename = g_strdup(NextToken(&sz));

    if (!filename || !*filename) {
        outputl(_("You must specify a file to export to."));
        g_free(filename);
        return NULL;
    }

    if (!confirmOverwrite(filename, fConfirmSave)) {
        g_free(filename);
        return NULL;
    }
    return filename;
}

static moverecord *
export_get_moverecord(int *game_nr, int *move_nr)
{
    int history = 0;
    moverecord *pmr = NULL;

    pmr = get_current_moverecord(&history);

    if (!pmr) {
        *move_nr = -1;
        return NULL;
    }

    if (history)
        *move_nr = getMoveNumber(plGame, pmr) - 1;
    else if (plLastMove)
        *move_nr = getMoveNumber(plGame, plLastMove->p);
    else
        *move_nr = -1;
    *game_nr = getGameNumber(plGame);
    return pmr;
}

/* default simple board paper size A4 */
#define SIMPLE_BOARD_WIDTH 210.0/25.4*72.0
#define SIMPLE_BOARD_HEIGHT 297.0/25.4*72.0

static int
draw_simple_board_on_cairo(matchstate * sb_pms,
                           moverecord * sb_pmr, int move_nr, int game_nr, cairo_t * cairo, float size)
{
    SimpleBoard *board;
    GString *header = NULL;
    GString *annotation = NULL;

    g_return_val_if_fail(cairo, 0);
    g_return_val_if_fail(sb_pms->gs != GAME_NONE, 0);

    board = simple_board_new(sb_pms, cairo, size);
    board->surface_x = SIMPLE_BOARD_WIDTH;
    board->surface_y = SIMPLE_BOARD_HEIGHT;

    header = g_string_new(NULL);
    TextPrologue(header, sb_pms, game_nr);
    TextBoardHeader(header, sb_pms, game_nr, move_nr);
    board->header = header->str;

    if (sb_pmr) {
        annotation = g_string_new(NULL);
        TextAnalysis(annotation, sb_pms, sb_pmr);
        board->annotation = annotation->str;
    }

    simple_board_draw(board);
    g_free(board);
    g_string_free(header, TRUE);
    if (annotation)
        g_string_free(annotation, TRUE);
    return 1;
}

static int
export_boards(moverecord * pmr, matchstate * msExport, int *iMove, int iGame, cairo_t * cairo, float size)
{
    FixMatchState(msExport, pmr);
    switch (pmr->mt) {
    case MOVE_NORMAL:
        if (pmr->fPlayer != msExport->fMove) {
            SwapSides(msExport->anBoard);
            msExport->fMove = pmr->fPlayer;
        }
        msExport->fTurn = msExport->fMove = pmr->fPlayer;
        msExport->anDice[0] = pmr->anDice[0];
        msExport->anDice[1] = pmr->anDice[1];
        draw_simple_board_on_cairo(msExport, pmr, *iMove, iGame, cairo, size);
        (*iMove)++;
        break;
    case MOVE_TAKE:
    case MOVE_DROP:
    case MOVE_DOUBLE:
        draw_simple_board_on_cairo(msExport, pmr, *iMove, iGame, cairo, size);
        (*iMove)++;
        break;
    default:
        break;
    }
    ApplyMoveRecord(msExport, plGame, pmr);
    return 1;
}

static int
draw_cairo_pages(cairo_t * cairo, listOLD * game_ptr)
{
    matchstate msExport;
    static statcontext scTotal;
    moverecord *pmr;
    statcontext *psc = NULL;
    listOLD *pl;
    int iMove = 0;
    int iGame = 0;
    int page;
    listOLD *pl_hint = NULL;

    IniStatcontext(&scTotal);
    updateStatisticsGame(game_ptr);
    pl = game_ptr->plNext;
    pmr = pl->p;
    FixMatchState(&msExport, pmr);
    ApplyMoveRecord(&msExport, game_ptr, pmr);
    g_assert(pmr->mt == MOVE_GAMEINFO);
    msExport.gs = GAME_PLAYING;
    psc = &pmr->g.sc;
    AddStatcontext(psc, &scTotal);
    iGame = getGameNumber(game_ptr);
    if (game_is_last(plGame))
        pl_hint = game_add_pmr_hint(game_ptr);
    for (pl = pl->plNext, page = 1; pl != game_ptr; pl = pl->plNext, page++) {
        export_boards(pl->p, &msExport, &iMove, iGame, cairo, SIZE_2PERPAGE);
        if (page % 2) {
            cairo_translate(cairo, 0, SIMPLE_BOARD_HEIGHT / 2.0);
        } else {
            cairo_translate(cairo, 0, -SIMPLE_BOARD_HEIGHT / 2.0);
            cairo_show_page(cairo);
        }
    }
    if (!(page % 2)) {
        cairo_translate(cairo, 0, -SIMPLE_BOARD_HEIGHT / 2.0);
        cairo_show_page(cairo);
    }
    if (pl_hint)
        game_remove_pmr_hint(pl_hint);
    return 1;
}

#endif

extern void
CommandExportPositionSVG(char *sz)
{
#if HAVE_PANGOCAIRO
    gchar *filename;
    int move_nr;
    int game_nr;
    moverecord *pmr;
    cairo_surface_t *surface;
    cairo_t *cairo;

    if (!CheckGameExists())
        return;

    filename = export_get_filename(sz);
    if (!filename)
        return;
    pmr = export_get_moverecord(&game_nr, &move_nr);
    if (!pmr) {
        outputerrf(_("Cannot create export for this move"));
        return;
    }
    surface = cairo_svg_surface_create(filename, SIMPLE_BOARD_WIDTH, SIMPLE_BOARD_HEIGHT / 2.0);
    if (surface) {
        cairo = cairo_create(surface);
        draw_simple_board_on_cairo(&ms, pmr, move_nr, game_nr, cairo, SIZE_1PERPAGE);
        cairo_surface_destroy(surface);
        cairo_destroy(cairo);
    } else
#endif
        outputerrf(_("Failed to create cairo surface for %s"), sz);
}

extern void
CommandExportPositionPDF(char *sz)
{
#if HAVE_PANGOCAIRO
    gchar *filename;
    int move_nr;
    int game_nr;
    moverecord *pmr;
    cairo_surface_t *surface;
    cairo_t *cairo;

    if (!CheckGameExists())
        return;

    filename = export_get_filename(sz);
    if (!filename)
        return;
    pmr = export_get_moverecord(&game_nr, &move_nr);
    if (!pmr) {
        outputerrf(_("Cannot create export for this move"));
        return;
    }
    surface = cairo_pdf_surface_create(filename, SIMPLE_BOARD_WIDTH, SIMPLE_BOARD_HEIGHT);
    if (surface) {
        cairo = cairo_create(surface);
        draw_simple_board_on_cairo(&ms, pmr, move_nr, game_nr, cairo, SIZE_1PERPAGE);
        cairo_surface_destroy(surface);
        cairo_destroy(cairo);
    } else
#endif
        outputerrf(_("Failed to create cairo surface for %s"), sz);
}

extern void
CommandExportPositionPS(char *sz)
{
#if HAVE_PANGOCAIRO
    gchar *filename;
    int move_nr;
    int game_nr;
    moverecord *pmr;
    cairo_surface_t *surface;
    cairo_t *cairo;

    if (!CheckGameExists())
        return;

    filename = export_get_filename(sz);
    if (!filename)
        return;
    pmr = export_get_moverecord(&game_nr, &move_nr);
    if (!pmr) {
        outputerrf(_("Cannot create export for this move"));
        return;
    }

    surface = cairo_ps_surface_create(filename, SIMPLE_BOARD_WIDTH, SIMPLE_BOARD_HEIGHT);
    if (surface) {
        cairo = cairo_create(surface);
        draw_simple_board_on_cairo(&ms, pmr, move_nr, game_nr, cairo, SIZE_1PERPAGE);
        cairo_surface_destroy(surface);
        cairo_destroy(cairo);
    } else
#endif
        outputerrf(_("Failed to create cairo surface for %s"), sz);
}

extern void
CommandExportGamePDF(char *sz)
{
#if HAVE_PANGOCAIRO
    gchar *filename;
    cairo_surface_t *surface;
    cairo_t *cairo;

    if (!CheckGameExists())
        return;

    filename = export_get_filename(sz);
    if (!filename)
        return;
    surface = cairo_pdf_surface_create(filename, SIMPLE_BOARD_WIDTH, SIMPLE_BOARD_HEIGHT);
    if (surface) {
        cairo = cairo_create(surface);
        draw_cairo_pages(cairo, plGame);
        cairo_surface_destroy(surface);
        cairo_destroy(cairo);
    } else
#endif
        outputerrf(_("Failed to create cairo surface for %s"), sz);
}

extern void
CommandExportGamePS(char *sz)
{
#if HAVE_PANGOCAIRO
    gchar *filename;
    cairo_surface_t *surface;
    cairo_t *cairo;

    if (!CheckGameExists())
        return;

    filename = export_get_filename(sz);
    if (!filename)
        return;
    surface = cairo_ps_surface_create(filename, SIMPLE_BOARD_WIDTH, SIMPLE_BOARD_HEIGHT);
    if (surface) {
        cairo = cairo_create(surface);
        draw_cairo_pages(cairo, plGame);
        cairo_surface_destroy(surface);
        cairo_destroy(cairo);
    } else
#endif
        outputerrf(_("Failed to create cairo surface for %s"), sz);
}

extern void
CommandExportMatchPDF(char *sz)
{
#if HAVE_PANGOCAIRO
    listOLD *pl;
    int nGames = 0;
    int i;
    char *filename;
    cairo_surface_t *surface;
    cairo_t *cairo;

    filename = export_get_filename(sz);

    if (!filename)
        return;
    surface = cairo_pdf_surface_create(filename, SIMPLE_BOARD_WIDTH, SIMPLE_BOARD_HEIGHT);
    if (surface) {
        for (pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext, nGames++);

        cairo = cairo_create(surface);
        for (pl = lMatch.plNext, i = 0; pl != &lMatch; pl = pl->plNext, i++) {
            draw_cairo_pages(cairo, pl->p);
        }
        cairo_surface_destroy(surface);
        cairo_destroy(cairo);
    } else
#endif
        outputerrf(_("Failed to create cairo surface for %s"), sz);
}

extern void
CommandExportMatchPS(char *sz)
{
#if HAVE_PANGOCAIRO
    listOLD *pl;
    int nGames = 0;
    int i;
    char *filename;
    cairo_surface_t *surface;
    cairo_t *cairo;

    filename = export_get_filename(sz);

    if (!filename)
        return;
    surface = cairo_ps_surface_create(filename, SIMPLE_BOARD_WIDTH, SIMPLE_BOARD_HEIGHT);
    if (surface) {
        for (pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext, nGames++);

        cairo = cairo_create(surface);
        for (pl = lMatch.plNext, i = 0; pl != &lMatch; pl = pl->plNext, i++) {
            draw_cairo_pages(cairo, pl->p);
        }
        cairo_surface_destroy(surface);
        cairo_destroy(cairo);
    } else
#endif
        outputerrf(_("Failed to create cairo surface for %s"), sz);
}

#if HAVE_LIBPNG

/* size of html images in steps of BOARD_WIDTH x BOARD_HEIGHT
 * as defined in boarddim.h */

extern int
WritePNG(const char *sz, unsigned char *puch, unsigned int nStride, unsigned int nSizeX, unsigned int nSizeY)
{

    FILE *pf;
    png_structp ppng;
    png_infop pinfo;
    png_text atext[3];
    unsigned int i;

    if (!(pf = g_fopen(sz, "wb")))
        return -1;

    if (!(ppng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
        fclose(pf);
        return -1;

    }

    if (!(pinfo = png_create_info_struct(ppng))) {
        fclose(pf);
        png_destroy_write_struct(&ppng, NULL);
        return -1;
    }

    if (setjmp(png_jmpbuf(ppng))) {
        outputerr(sz);
        fclose(pf);
        png_destroy_write_struct(&ppng, &pinfo);
        return -1;
    }

    png_init_io(ppng, pf);

    png_set_IHDR(ppng, pinfo, nSizeX, nSizeY, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    /* text */

    atext[0].key = "Title";
    atext[0].text = "Backgammon board";
    atext[0].compression = PNG_TEXT_COMPRESSION_NONE;

    atext[1].key = "author";
    atext[1].text = VERSION_STRING;
    atext[1].compression = PNG_TEXT_COMPRESSION_NONE;

#ifdef PNG_iTXt_SUPPORTED
    atext[0].lang = NULL;
    atext[1].lang = NULL;
#endif
    png_set_text(ppng, pinfo, atext, 2);

    png_write_info(ppng, pinfo);

    {
        png_bytep *aprow = (png_bytep *) g_alloca(nSizeY * sizeof(png_bytep));;
        for (i = 0; i < nSizeY; ++i)
            aprow[i] = puch + nStride * i;

        png_write_image(ppng, aprow);

    }

    png_write_end(ppng, pinfo);

    png_destroy_write_struct(&ppng, &pinfo);

    fclose(pf);

    return 0;

}

static int
GenerateImage(renderimages * pri, renderdata * prd,
              const TanBoard anBoard,
              const char *szName,
              const int nSize, const int nSizeX, const int nSizeY,
              const int nOffsetX, const int nOffsetY,
              const int fMove, const int fTurn, const int fCube,
              const unsigned int anDice[2], const int nCube, const int fDoubled, const int fCubeOwner)
{
    TanBoard anBoardTemp;
    unsigned char *puch;
    int anCubePosition[2];
    int anDicePosition[2][2];
    int nOrient;
    int doubled, color;
    /* FIXME: resignations */
    int anResignPosition[2];
    int fResign = 0, nResignOrientation = 0;
    int anArrowPosition[2];
    int cube_owner;

    memcpy(anBoardTemp, anBoard, sizeof anBoardTemp);

    if (!fMove)
        SwapSides(anBoardTemp);

    /* allocate memory for board */

    if (!(puch = (unsigned char *) malloc(BOARD_WIDTH * BOARD_HEIGHT * nSize * nSize * 3))) {
        outputerr("malloc");
        return -1;
    }

    /* calculate cube position */

    if (fDoubled)
        doubled = fTurn ? -1 : 1;
    else
        doubled = 0;

    if (!fCubeOwner)
        cube_owner = 1;
    else if (fCubeOwner == 1)
        cube_owner = -1;
    else
        cube_owner = 0;


    CubePosition(FALSE, fCube, doubled, cube_owner, fClockwise, &anCubePosition[0], &anCubePosition[1], &nOrient);

    /* calculate dice position */

    /* FIXME */

    if (anDice[0]) {
        anDicePosition[0][0] = 22 + fMove * 48;
        anDicePosition[0][1] = 32;
        anDicePosition[1][0] = 32 + fMove * 48;
        anDicePosition[1][1] = 32;
    } else {
        anDicePosition[0][0] = -7 * nSize;
        anDicePosition[0][1] = 0;
        anDicePosition[1][0] = -7 * nSize;
        anDicePosition[1][1] = -1;
    }

    ArrowPosition(fClockwise, fTurn, nSize, &anArrowPosition[0], &anArrowPosition[1]);

    /* render board */

    color = fMove;

    CalculateArea(prd, puch, BOARD_WIDTH * nSize * 3, pri, anBoardTemp, NULL,
                  anDice, anDicePosition,
                  color, anCubePosition,
                  LogCube(nCube) + (doubled != 0),
                  nOrient,
                  anResignPosition, fResign, nResignOrientation,
                  anArrowPosition, ms.gs != GAME_NONE, fMove == 1, 0, 0, BOARD_WIDTH * nSize, BOARD_HEIGHT * nSize);

    /* crop */

    if (nSizeX < BOARD_WIDTH || nSizeY < BOARD_HEIGHT) {
        int i, j, k;

        j = 0;
        k = nOffsetY * nSize * BOARD_WIDTH + nOffsetX;
        for (i = 0; i < nSizeY * nSize; ++i) {
            /* loop over each row */
            memmove(puch + j, puch + k, nSizeX * nSize * 3);

            j += nSizeX * nSize * 3;
            k += BOARD_WIDTH * nSize;

        }

    }

    /* write png */

    WritePNG(szName, puch, nSizeX * nSize * 3, nSizeX * nSize, nSizeY * nSize);

    free(puch);

    return 0;
}

extern void
CommandExportPositionPNG(char *sz)
{
    sz = NextToken(&sz);

    if (!CheckGameExists())
        return;

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    /* generate PNG image */

#if USE_BOARD3D
    if (GetMainAppearance()->fDisplayType == DT_3D) {
        GenerateImage3d(sz, exsExport.nPNGSize, BOARD_WIDTH, BOARD_HEIGHT);
    } else
#endif
    {
        renderimages ri;
        renderdata rd;

        CopyAppearance(&rd);
        rd.nSize = exsExport.nPNGSize;

        g_assert(rd.nSize >= 1);

        RenderImages(&rd, &ri);

        GenerateImage(&ri, &rd, msBoard(), sz,
                      exsExport.nPNGSize, BOARD_WIDTH, BOARD_HEIGHT, 0, 0,
                      ms.fMove, ms.fTurn, fCubeUse, ms.anDice, ms.nCube, ms.fDoubled, ms.fCubeOwner);

        FreeImages(&ri);
    }
}


#endif                          /* HAVE_LIBPNG */


/*
 * For documentation of format, see ImportSnowieTxt in import.c
 */

static void
ExportSnowieTxt(char *sz, const matchstate * pms)
{

    int i, j;

    /* length of match */

    sz += sprintf(sz, "%d;", pms->nMatchTo);

    /* Jacoby rule */

    sz += sprintf(sz, "%d;", (!pms->nMatchTo && fJacoby) ? 1 : 0);

    /* unknown field (perhaps variant */

    sz += sprintf(sz, "%d;", 0);

    /* unknown field */

    sz += sprintf(sz, "%d;", pms->nMatchTo ? 0 : 1);

    /* player on roll (0 = 1st player) */

    sz += sprintf(sz, "%d;", pms->fMove);

    /* player names */

    sz += sprintf(sz, "%s;%s;", ap[pms->fMove].szName, ap[!pms->fMove].szName);

    /* crawford game */

    i = pms->nMatchTo &&
        ((pms->anScore[0] == (pms->nMatchTo - 1)) ||
         (pms->anScore[1] == (pms->nMatchTo - 1))) && pms->fCrawford && !pms->fPostCrawford;

    sz += sprintf(sz, "%d;", i);

    /* scores */

    sz += sprintf(sz, "%d;%d;", pms->anScore[pms->fMove], pms->anScore[!pms->fMove]);

    /* cube value */

    sz += sprintf(sz, "%d;", pms->nCube);

    /* cube owner */

    if (pms->fCubeOwner == -1)
        i = 0;
    else if (pms->fCubeOwner == pms->fMove)
        i = 1;
    else
        i = -1;

    sz += sprintf(sz, "%d;", i);

    /* chequers on the bar for player on roll */

    sz += sprintf(sz, "%d;", -(int) pms->anBoard[0][24]);

    /* chequers on the board */

    for (i = 0; i < 24; ++i) {
        if (pms->anBoard[1][i])
            j = pms->anBoard[1][i];
        else
            j = -(int) pms->anBoard[0][23 - i];

        sz += sprintf(sz, "%d;", j);

    }

    /* chequers on the bar for opponent */

    sz += sprintf(sz, "%d;", pms->anBoard[1][24]);

    /* dice */

    sprintf(sz, "%d;%d;", (pms->anDice[0] > 0) ? pms->anDice[0] : 0, (pms->anDice[1] > 0) ? pms->anDice[1] : 0);

}


extern void
CommandExportPositionSnowieTxt(char *sz)
{

    FILE *pf;
    char buffer[256];

    sz = NextToken(&sz);

    if (!CheckGameExists())
        return;

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if (!(pf = g_fopen(sz, "w"))) {
        outputerr(sz);
        return;
    }

    ExportSnowieTxt(buffer, &ms);
    fputs(buffer, pf);
    fputs("\n", pf);

    if (pf != stdout)
        fclose(pf);

    setDefaultFileName(sz);

}


static int
WriteInt16(FILE * pf, int n)
{

    /* Write a little-endian, signed (2's complement) 16-bit integer. This is inefficient on hardware which is
     * already little-endian or 2's complement, but at least it's portable. */

    return (fwrite(&n, 2, 1, pf) == 1);
}

extern void
CommandExportPositionJF(char *sz)
{
    /* I think this function works now correctly. If there is some inconsistencies between import pos and export
     * pos, it's probably a bug in the import code. --Oystein */

    FILE *fp;
    int i;
    unsigned char c;
    TanBoard anBoard;
    int tmp;

    sz = NextToken(&sz);

    if (!CheckGameExists())
        return;

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to."));
        return;
    }


    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        fp = stdout;
    else if (!(fp = g_fopen(sz, "wb"))) {
        outputerr(sz);
        return;
    }

    if (!WriteInt16(fp, 126))   /* Always write in JellyFish 3.0 format */
        goto write_failed;
    if (!WriteInt16(fp, 0))     /* Never save with Caution */
        goto write_failed;
    if (!WriteInt16(fp, 0))     /* This is unused */
        goto write_failed;

    if (!WriteInt16(fp, fCubeUse))
        goto write_failed;
    if (!WriteInt16(fp, fJacoby))
        goto write_failed;
    if (!WriteInt16(fp, (nBeavers != 0)))
        goto write_failed;

    if (!WriteInt16(fp, ms.nCube))
        goto write_failed;
    if (!WriteInt16(fp, ms.fCubeOwner + 1))
        goto write_failed;

    if ((ms.gs == GAME_OVER) || (ms.gs == GAME_NONE)) {
        if (!WriteInt16(fp, 0))
            goto write_failed;
    } else {
        tmp = ms.anDice[0] > 0 ? ms.fTurn + 1 : ms.fTurn + 3;
        if (!WriteInt16(fp, tmp))
            goto write_failed;
    }
    /* 0 means starting position. If you add 2 to the player (to get 3 or 4) Sure? it means that the player is on
     * roll but the dice have not been rolled yet. */

    if (!WriteInt16(fp, 0))
        goto write_failed;
    if (!WriteInt16(fp, 0))     /* FIXME Test this! */
        goto write_failed;
    /* These two variables are used when you use movement #1, (two buttons) and tells how many moves you have left
     * to play with the left and the right die, respectively. Initialized to 1 (if you roll a double, left = 4 and
     * right = 0). If movement #2 (one button), only the first one (left) is used to store both dice.  */

    if (!WriteInt16(fp, 0))     /* Not in use */
        goto write_failed;

    tmp = ms.nMatchTo ? 1 : 3;
    if (!WriteInt16(fp, tmp))
        goto write_failed;
    /* 1 = match, 3 = game */

    if (!WriteInt16(fp, 2))     /* FIXME */
        goto write_failed;
    /* 1 = 2 players, 2 = JF plays one side */

    if (!WriteInt16(fp, 7))     /* Use level 7 */
        goto write_failed;

    if (!WriteInt16(fp, ms.nMatchTo))
        goto write_failed;
    /* 0 if single game */

    if (ms.nMatchTo == 0) {
        if (!WriteInt16(fp, 0))
            goto write_failed;
        if (!WriteInt16(fp, 0))
            goto write_failed;
    } else {
        if (!WriteInt16(fp, ms.anScore[0]))
            goto write_failed;
        if (!WriteInt16(fp, ms.anScore[1]))
            goto write_failed;
    }

    c = (unsigned char) strlen(ap[0].szName);
    if (fwrite(&c, 1, 1, fp) != 1)
        goto write_failed;
    for (i = 0; i < c; i++) {
        if (fwrite(&ap[0].szName[i], 1, 1, fp) != 1)
            goto write_failed;
    }

    c = (unsigned char) strlen(ap[1].szName);
    if (fwrite(&c, 1, 1, fp) != 1)
        goto write_failed;
    for (i = 0; i < c; i++) {
        if (fwrite(&ap[1].szName[i], 1, 1, fp) != 1)
            goto write_failed;
    }

    if (!WriteInt16(fp, 0))     /* FIXME Check gnubg setting */
        goto write_failed;
    /* TRUE if lower die is to be drawn to the left */

    if ((ms.fPostCrawford == FALSE) && (ms.fCrawford == TRUE))
        tmp = 2;
    else if ((ms.fPostCrawford == TRUE) && (ms.fCrawford == FALSE))
        tmp = 3;
    else
        tmp = 1;
    if (!WriteInt16(fp, tmp))
        goto write_failed;

    if (!WriteInt16(fp, 0))     /* JF played last. Must be FALSE */
        goto write_failed;

    c = 0;
    if (fwrite(&c, 1, 1, fp) != 1)
        goto write_failed;      /* Length of "last move" string */

    if (!WriteInt16(fp, ms.anDice[0]))
        goto write_failed;
    if (!WriteInt16(fp, ms.anDice[1]))
        goto write_failed;

    /* The Jellyfish format saves the current board and the board before the move was done, such that undo should
     * be possible. It's possible to save just the current board twice as done below. */

    memcpy(anBoard, msBoard(), sizeof(TanBoard));

    if (!ms.fMove)
        SwapSides(anBoard);

    /* Player 0 on bar */
    if (!WriteInt16(fp, anBoard[1][24] + 20))
        goto write_failed;
    if (!WriteInt16(fp, anBoard[1][24] + 20))
        goto write_failed;

    /* Board */
    for (i = 24; i > 0; i--) {
        int point = (int) anBoard[0][24 - i];
        if (!WriteInt16(fp, (point > 0 ? -point : (int) anBoard[1][i - 1]) + 20))
            goto write_failed;
        if (!WriteInt16(fp, (point > 0 ? -point : (int) anBoard[1][i - 1]) + 20))
            goto write_failed;
    }
    /* Player on bar */
    if (!WriteInt16(fp, -(int) anBoard[0][24] + 20))
        goto write_failed;
    if (!WriteInt16(fp, -(int) anBoard[0][24] + 20))
        goto write_failed;

    fclose(fp);

    setDefaultFileName(sz);

    return;

  write_failed:
    outputerr(sz);
    fclose(fp);
    return;
}

static void
ExportGameJF(FILE * pf, listOLD * plGame, int iGame, int withScore, int fSst)
{
    listOLD *pl;
    moverecord *pmr;
    matchstate msExport;
    char sz[128], buffer[256];
    int i = 0, n, nFileCube = 1, fWarned = FALSE, diceRolled = 0;
    TanBoard anBoard;

    /* FIXME It would be nice if this function was updated to use the
     * new matchstate struct and ApplyMoveRecord()... but otherwise
     * it's not broken, so I won't fix it. */

    if (fSst && iGame == -1)    /* Snowie export of single game */
        fprintf(pf, " 0 point match\n\n Game 1\n");

    if (iGame >= 0)
        fprintf(pf, " Game %d\n", iGame + 1);

    InitBoard(anBoard, ms.bgv);

    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext) {
        pmr = pl->p;
        switch (pmr->mt) {
        case MOVE_GAMEINFO:
            if (withScore) {
                sprintf(sz, "%s : %d", ap[0].szName, pmr->g.anScore[0]);
                fprintf(pf, " %-31s%s : %d\n", sz, ap[1].szName, pmr->g.anScore[1]);
                msExport.anScore[0] = pmr->g.anScore[0];
                msExport.anScore[1] = pmr->g.anScore[1];
                msExport.fCrawford = pmr->g.fCrawford;
                msExport.fPostCrawford = !pmr->g.fCrawfordGame;
            } else if (fSst && iGame == -1) {   /* Snowie export of single game */
                sprintf(sz, "%s : 0", ap[0].szName);
                fprintf(pf, " %-31s%s : 0\n", sz, ap[1].szName);
            } else
                fprintf(pf, " %-31s%s\n", ap[0].szName, ap[1].szName);
            msExport.fCubeOwner = -1;
            /* FIXME what about automatic doubles? */
            continue;
            break;
        case MOVE_NORMAL:
            diceRolled = 0;
            sprintf(sz, "%u%u: ", pmr->anDice[0], pmr->anDice[1]);
            if (fSst) {         /* Snowie standard text */
                moverecord *pnextmr;
                if (pl->plNext && pl->plNext->p) {
                    pnextmr = pl->plNext->p;
                    if (pnextmr->mt == MOVE_SETBOARD)
                        /* Illegal move entered in gnubg as bogus move followed by
                         * editing position : don't export the move, only the dice */
                        diceRolled = 1;
                    else
                        FormatMovePlain(sz + 4, anBoard, pmr->n.anMove);
                } else
                    FormatMovePlain(sz + 4, anBoard, pmr->n.anMove);
            } else
                FormatMovePlain(sz + 4, anBoard, pmr->n.anMove);
            ApplyMove(anBoard, pmr->n.anMove, FALSE);
            SwapSides(anBoard);
            /*   if (( sz[ strlen(sz)-1 ] == ' ') && (strlen(sz) > 5 ))
             * sz[ strlen(sz) - 1 ] = 0;  Don't need this..  */
            break;
        case MOVE_DOUBLE:
            sprintf(sz, " Doubles => %d", nFileCube <<= 1);
            msExport.fCubeOwner = !(i & 1);
            break;
        case MOVE_TAKE:
            strcpy(sz, " Takes");       /* FIXME beavers? */
            break;
        case MOVE_DROP:
            sprintf(sz, " Drops%sWins %d point%s",
                    (i & 1) ? "\n      " : "                       ", nFileCube / 2, (nFileCube == 2) ? "" : "s");
            break;
        case MOVE_RESIGN:
            if (pmr->fPlayer)
                sprintf(sz, "%s      Wins %d point%s\n", ((i & 1) ^ diceRolled) ? "\n" : "",
                        pmr->r.nResigned * nFileCube, ((pmr->r.nResigned * nFileCube) > 1) ? "s" : "");
            else if (i & 1) {
                sprintf(sz, "%sWins %d point%s\n", ((i & 1) ^ diceRolled) ? " " : "                        ",
                        pmr->r.nResigned * nFileCube, ((pmr->r.nResigned * nFileCube) > 1) ? "s" : "");
            } else {
                sprintf(sz, "%sWins %d point%s\n", ((i & 1) ^ diceRolled) ? " " : "                                  ",
                        pmr->r.nResigned * nFileCube, ((pmr->r.nResigned * nFileCube) > 1) ? "s" : "");
            }
            break;
        case MOVE_SETDICE:
            /* Could be rolled dice just before resign or an illegal move */
            sprintf(sz, "%u%u: ", pmr->anDice[0], pmr->anDice[1]);
            diceRolled = 1;
            break;
        case MOVE_SETBOARD:
        case MOVE_SETCUBEVAL:
        case MOVE_SETCUBEPOS:
            if (!fWarned && !fSst) {
                fWarned = TRUE;
                outputl(_("Warning: this game was edited during play and "
                          "cannot be exported correctly in this format.\n" "Use Snowie text format instead."));
            }
            break;
        }

        if (pmr->mt == MOVE_SETBOARD || pmr->mt == MOVE_SETCUBEVAL || pmr->mt == MOVE_SETCUBEPOS) {
            PositionFromKey(anBoard, &pmr->sb.key);
            if (i & 1)
                SwapSides(anBoard);

            if (fSst) {         /* Snowie standard text */
                moverecord *pnextmr = pl->plNext->p;
                char *ct;

                msExport.nMatchTo = ms.nMatchTo;
                msExport.nCube = nFileCube;
                msExport.fMove = (i & 1);
                msExport.fTurn = (i & 1);
                PositionFromKey(msExport.anBoard, &pmr->sb.key);
                msExport.anDice[0] = pnextmr->anDice[0];
                msExport.anDice[1] = pnextmr->anDice[1];
                if (i & 1)
                    SwapSides(msExport.anBoard);
                ExportSnowieTxt(buffer, &msExport);

                /* I don't understand why we need to swap this field! */
                ct = strstr(buffer, ";");
                ct[7] = '0' + ('1' - ct[7]);
            }
            if (!(i & 1))
                if (fSst)
                    fprintf(pf, "Illegal play (%s)\n", buffer);
                else
                    fprintf(pf, "\n");
            else if (fSst)
                fprintf(pf, "Illegal play (%-22s) ", buffer);
            else
                fprintf(pf, "%-23s ", "");
            continue;
        }

        if (!i && pmr->mt == MOVE_NORMAL && pmr->fPlayer) {
            fputs("  1)                             ", pf);
            i++;
        }

        if ((i & 1) || (pmr->mt == MOVE_RESIGN)) {
            fputs(sz, pf);
            if (pmr->mt != MOVE_SETDICE && !(pmr->mt == MOVE_NORMAL && diceRolled == 1))
                fputc('\n', pf);
        } else if (pmr->mt != MOVE_SETDICE)
            fprintf(pf, "%3d) %-27s ", (i >> 1) + 1, sz);
        else
            fprintf(pf, "%3d) %-3s", (i >> 1) + 1, sz);

        if (pmr->mt == MOVE_DROP) {
            fputc('\n', pf);
            if (!(i & 1))
                fputc('\n', pf);
        }

        if ((n = GameStatus((ConstTanBoard) anBoard, ms.bgv))) {
            fprintf(pf, "%sWins %d point%s%s\n\n",
                    (i & 1) ? "                                  " : "\n      ",
                    n * nFileCube, n * nFileCube > 1 ? "s" : "", "" /* FIXME " and the match" if appropriate */ );
        }
        i++;
    }
}


static void
ExportGameGam(char *sz, int fSst)
{

    FILE *pf;

    sz = NextToken(&sz);

    if (!CheckGameExists())
        return;

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if (!(pf = g_fopen(sz, "w"))) {
        outputerr(sz);
        return;
    }

    ExportGameJF(pf, plGame, -1, FALSE, fSst);

    if (pf != stdout)
        fclose(pf);

    setDefaultFileName(sz);

}

extern void
CommandExportGameGam(char *sz)
{
    ExportGameGam(sz, FALSE);
}

extern void
CommandExportGameSnowieTxt(char *sz)
{
    ExportGameGam(sz, TRUE);
}


static void
ExportMatchMat(char *sz, int fSst)
{

    FILE *pf;
    int i;
    listOLD *pl;

    /* FIXME what should be done if nMatchTo == 0? */

    sz = NextToken(&sz);

    if (!CheckGameExists())
        return;

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if (!(pf = g_fopen(sz, "w"))) {
        outputerr(sz);
        return;
    }

    if (!fSst) {
        if (mi.pchPlace)
            fprintf(pf, "; [Site \"%s\"]\n", mi.pchPlace);
        if (mi.pchEvent)
            fprintf(pf, "; [Event \"%s\"]\n", mi.pchEvent);
        if (mi.pchRound)
            fprintf(pf, "; [Round \"%s\"]\n", mi.pchRound);
        if (mi.nYear > 1900)
            fprintf(pf, "; [EventDate \"%4u.%02u.%02u\"]\n", mi.nYear, mi.nMonth, mi.nDay);
        switch (ms.bgv) {
        case VARIATION_NACKGAMMON:
            fprintf(pf, "; [Variation \"NackGammon\"]\n");
            break;
        case VARIATION_HYPERGAMMON_1:
            fprintf(pf, "; [Variation \"HyperGammon (1)\"]\n");
            break;
        case VARIATION_HYPERGAMMON_2:
            fprintf(pf, "; [Variation \"HyperGammon (2)\"]\n");
            break;
        case VARIATION_HYPERGAMMON_3:
            fprintf(pf, "; [Variation \"HyperGammon (3)\"]\n");
            break;
        }
        if (mi.pchAnnotator)
            fprintf(pf, "; [Transcriber \"%s\"]\n", mi.pchAnnotator);
        if (mi.pchComment) {
            char *pc;

            fprintf(pf, "\n; ");
            for (pc = mi.pchComment; *pc != 0; pc++)
                if (*pc == '\n' && *(pc + 1) != 0)
                    fputs("\n; ", pf);
                else
                    fputc(*pc, pf);
        }
        fprintf(pf, "\n");
    }

    fprintf(pf, " %d point match\n\n", ms.nMatchTo);

    for (i = 0, pl = lMatch.plNext; pl != &lMatch; i++, pl = pl->plNext)
        ExportGameJF(pf, pl->p, i, TRUE, fSst);

    if (pf != stdout)
        fclose(pf);

    setDefaultFileName(sz);

}

extern void
CommandExportMatchMat(char *sz)
{
    ExportMatchMat(sz, FALSE);
}

extern void
CommandExportMatchSnowieTxt(char *sz)
{
    ExportMatchMat(sz, TRUE);
}
