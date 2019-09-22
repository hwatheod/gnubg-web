/*
 * latex.c
 *
 * by Gary Wong <gtw@gnu.org>, 2001, 2002
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
 * $Id: latex.c,v 1.54 2014/01/24 23:59:39 plm Exp $
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "analysis.h"
#include "backgammon.h"
#include "drawboard.h"
#include "export.h"
#include "format.h"
#include <glib/gstdio.h>

static const char *aszLuckTypeLaTeXAbbr[] = { "$--$", "$-$", "", "$+$", "$++$" };

static void
Points1_12(FILE * pf, int y)
{

    int i;

    if (fClockwise)
        for (i = 1; i <= 12; i++)
            fprintf(pf, "\\put(%d,%d){\\makebox(20,10){\\textsl{\\tiny %d}}}\n", 50 + i * 20 + (i > 6) * 20, y, i);
    else
        for (i = 1; i <= 12; i++)
            fprintf(pf, "\\put(%d,%d){\\makebox(20,10){\\textsl{\\tiny %d}}}\n", 330 - i * 20 - (i > 6) * 20, y, i);
}

static void
Points13_24(FILE * pf, int y)
{

    int i;

    if (fClockwise)
        for (i = 13; i <= 24; i++)
            fprintf(pf, "\\put(%d,%d){\\makebox(20,10){\\textsl{\\tiny %d}}}\n", 570 - i * 20 - (i > 18) * 20, y, i);
    else
        for (i = 13; i <= 24; i++)
            fprintf(pf, "\\put(%d,%d){\\makebox(20,10){\\textsl{\\tiny %d}}}\n", i * 20 - 190 + (i > 18) * 20, y, i);
}

static void
LaTeXPrologue(FILE * pf)
{

    fputs("\\documentclass{article}\n"
          "\\usepackage{epic,eepic,textcomp,ucs}\n"
          "\\usepackage[utf8x]{inputenc}\n"
          "\\newcommand{\\board}{\n"
          "\\shade\\path(70,20)(80,120)(90,20)(110,20)(120,120)(130,20)"
          "(150,20)(160,120)\n"
          "(170,20)(70,20)\n"
          "\\path(70,20)(80,120)(90,20)(100,120)(110,20)(120,120)(130,20)"
          "(140,120)(150,20)\n"
          "(160,120)(170,20)(180,120)(190,20)\n"
          "\\shade\\path(90,240)(100,140)(110,240)(130,240)(140,140)(150,240)"
          "(170,240)\n"
          "(180,140)(190,240)(90,240)\n"
          "\\path(70,240)(80,140)(90,240)(100,140)(110,240)(120,140)(130,240)"
          "(140,140)\n"
          "(150,240)(160,140)(170,240)(180,140)(190,240)\n"
          "\\shade\\path(210,20)(220,120)(230,20)(250,20)(260,120)(270,20)"
          "(290,20)(300,120)\n"
          "(310,20)(210,20)\n"
          "\\path(210,20)(220,120)(230,20)(240,120)(250,20)(260,120)(270,20)"
          "(280,120)\n"
          "(290,20)(300,120)(310,20)(320,120)(330,20)\n"
          "\\shade\\path(230,240)(240,140)(250,240)(270,240)(280,140)"
          "(290,240)(310,240)\n"
          "(320,140)(330,240)(230,240)\n"
          "\\path(210,240)(220,140)(230,240)(240,140)(250,240)(260,140)"
          "(270,240)(280,140)\n"
          "(290,240)(300,140)(310,240)(320,140)(330,240)\n"
          "\\path(60,10)(340,10)(340,250)(60,250)(60,10)\n"
          "\\path(70,20)(190,20)(190,240)(70,240)(70,20)\n"
          "\\path(210,20)(330,20)(330,240)(210,240)(210,20)}\n\n", pf);

    fputs("\\newcommand{\\blackboard}{\n" "\\board\n", pf);
    Points1_12(pf, 10);
    Points13_24(pf, 240);
    fputs("}\n\n", pf);

    fputs("\\newcommand{\\whiteboard}{\n" "\\board\n", pf);
    Points1_12(pf, 240);
    Points13_24(pf, 10);
    fputs("}\n\n" "\\addtolength\\textwidth{144pt}\n" "\\addtolength\\textheight{144pt}\n" "\\addtolength\\oddsidemargin{-72pt}\n" "\\addtolength\\evensidemargin{-72pt}\n" "\\addtolength\\topmargin{-72pt}\n\n" "\\setlength{\\unitlength}{0.20mm}\n\n"       /* user should set size */
          "\\begin{document}\n", pf);

    /* FIXME define commands for \cubetext, \labeltext, etc. here, which
     * are scaled to a size specified by the user.  The other functions
     * should then use those definitions. */
}

static void
LaTeXEpilogue(FILE * pf)
{

    fputs("\\end{document}\n", pf);
}

static void
DrawLaTeXPoint(FILE * pf, int i, int fPlayer, int c)
{

    int j, x, y = 0;

    if (i < 6)
        x = 320 - 20 * i;
    else if (i < 12)
        x = 300 - 20 * i;
    else if (i < 18)
        x = 20 * i - 160;
    else if (i < 24)
        x = 20 * i - 140;
    else if (i == 24)           /* bar */
        x = 200;
    else                        /* off */
        x = 365;

    if (fClockwise)
        x = 400 - x;

    for (j = 0; j < c; j++) {
        if (j == 5 || (i == 24 && j == 3)) {
            fprintf(pf, "\\whiten\\path(%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)\n"
                    "\\path(%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)\n"
                    "\\put(%d,%d){\\makebox(10,10){\\textsf{\\tiny %d}}}\n",
                    x - 5, y - 5, x + 5, y - 5, x + 5, y + 5, x - 5, y + 5,
                    x - 5, y - 5,
                    x - 5, y - 5, x + 5, y - 5, x + 5, y + 5, x - 5, y + 5, x - 5, y - 5, x - 5, y - 5, c);
            break;
        }

        if (i == 24)
            /* bar */
            y = fPlayer ? 60 + 20 * j : 200 - 20 * j;
        else if (fPlayer)
            y = i < 12 || i == 25 ? 30 + 20 * j : 230 - 20 * j;
        else
            y = i >= 12 && i != 25 ? 30 + 20 * j : 230 - 20 * j;

        fprintf(pf, fPlayer ? "\\put(%d,%d){\\circle{10}}"
                "\\put(%d,%d){\\blacken\\circle{20}}\n" :
                "\\put(%d,%d){\\whiten\\circle{20}}" "\\put(%d,%d){\\circle{20}}\n", x, y, x, y);
    }
}

static void
PrintLaTeXBoard(FILE * pf, matchstate * pms, int fPlayer)
{

    int anOff[2] = { 15, 15 }, i, x, y;

    /* FIXME print position ID and pip count, and the player on roll.
     * Print player names too? */

    fprintf(pf, "\\bigskip\\pagebreak[1]\\begin{center}\\begin{picture}"
            "(356,240)(22,10)\n" "\\%sboard\n", fPlayer ? "black" : "white");

    for (i = 0; i < 25; i++) {
        anOff[0] -= pms->anBoard[0][i];
        anOff[1] -= pms->anBoard[1][i];

        DrawLaTeXPoint(pf, i, 0, pms->anBoard[!fPlayer][i]);
        DrawLaTeXPoint(pf, i, 1, pms->anBoard[fPlayer][i]);
    }

    DrawLaTeXPoint(pf, 25, 0, anOff[!fPlayer]);
    DrawLaTeXPoint(pf, 25, 1, anOff[fPlayer]);

    if (fClockwise)
        x = 365;
    else
        x = 35;

    if (pms->fCubeOwner < 0)
        y = 130;
    else if (pms->fCubeOwner)
        y = 30;
    else
        y = 230;

    fprintf(pf, "\\path(%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)"
            "\\put(%d,%d){\\makebox(24,24){\\textsf{\\large %d}}}\n",
            x - 12, y - 12, x + 12, y - 12, x + 12, y + 12, x - 12, y + 12,
            x - 12, y - 12, x - 12, y - 12, pms->nCube == 1 ? 64 : pms->nCube);

    fputs("\\end{picture}\\end{center}\\vspace{-4mm}\n\n\\nopagebreak[4]\n", pf);
}

/*
 * This was removed at first when unicode encoding was used, but we
 * still need to escape what could cause problems (like % $ _
 * etc...). Just to be safe, we escape everything but simple accented
 * characters.
 */
static void
LaTeXEscape(FILE * pf, char *pch)
{

    /* Translation table from GNU recode, by François Pinard. */
    static struct translation {
        unsigned char uch;
        char *sz;
    } at[] = {
        {
        10, "\n\n"},            /* \n */
        {
        35, "\\#"},             /* # */
        {
        36, "\\$"},             /* $ */
        {
        37, "\\%"},             /* % */
        {
        38, "\\&"},             /* & */
        {
        60, "\\mbox{$<$}"},     /* < */
        {
        62, "\\mbox{$>$}"},     /* > */
        {
        92, "\\mbox{$\\backslash{}$}"}, /* \ */
        {
        94, "\\^{}"},           /* ^ */
        {
        95, "\\_"},             /* _ */
        {
        123, "\\{"},            /* { */
        {
        124, "\\mbox{$|$}"},    /* | */
        {
        125, "\\}"},            /* } */
        {
        126, "\\~{}"},          /* ~ */
        {
        160, "~"},              /* no-break space */
        {
        161, "!`"},             /* inverted exclamation mark */
        {
        163, "\\pound{}"},      /* pound sign */
        {
        167, "\\S{}"},          /* paragraph sign, section sign */
        {
        168, "\\\"{}"},         /* diaeresis */
        {
        169, "\\copyright{}"},  /* copyright sign */
        {
        171, "``"},             /* left angle quotation mark */
        {
        172, "\\neg{}"},        /* not sign */
        {
        173, "\\-"},            /* soft hyphen */
        {
        176, "\\mbox{$^\\circ$}"},      /* degree sign */
        {
        177, "\\mbox{$\\pm$}"}, /* plus-minus sign */
        {
        178, "\\mbox{$^2$}"},   /* superscript two */
        {
        179, "\\mbox{$^3$}"},   /* superscript three */
        {
        180, "\\'{}"},          /* acute accent */
        {
        181, "\\mbox{$\\mu$}"}, /* small greek mu, micro sign */
        {
        183, "\\cdotp"},        /* middle dot */
        {
        184, "\\,{}"},          /* cedilla */
        {
        185, "\\mbox{$^1$}"},   /* superscript one */
        {
        187, "''"},             /* right angle quotation mark */
        {
        188, "\\frac1/4{}"},    /* vulgar fraction one quarter */
        {
        189, "\\frac1/2{}"},    /* vulgar fraction one half */
        {
        190, "\\frac3/4{}"},    /* vulgar fraction three quarters */
        {
        191, "?`"},             /* inverted question mark */
        {
        0, NULL}
    };
    int i;

    if (!pch)
        return;

    while (*pch) {
        for (i = 0; at[i].uch; i++)
            if (at[i].uch > *pch) {
                /* no translation required */
                putc(*pch, pf);
                break;
            } else if (at[i].uch == *pch) {
                /* translation found */
                fputs(at[i].sz, pf);
                break;
            }
        pch++;
    }
}

static void
PrintLaTeXComment(FILE * pf, char *pch)
{

    LaTeXEscape(pf, pch);
    if (pch != NULL)
        fputs(pch, pf);
    fputs("\n\n", pf);
}

static void
PrintLaTeXCubeAnalysis(FILE * pf, const matchstate * pms, int fPlayer,
                       float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                       float aarStdDev[2][NUM_ROLLOUT_OUTPUTS], const evalsetup * pes)
{

    cubeinfo ci;

    if (pes->et == EVAL_NONE)
        return;

    SetCubeInfo(&ci, pms->nCube, pms->fCubeOwner, fPlayer, pms->nMatchTo,
                pms->anScore, pms->fCrawford, pms->fJacoby, nBeavers, pms->bgv);

    /* FIXME use center and tabular environment instead of verbatim */
    fputs("{\\begin{quote}\\footnotesize\\begin{verbatim}\n", pf);
    fputs(OutputCubeAnalysis(aarOutput, aarStdDev, pes, &ci), pf);
    fputs("\\end{verbatim}\\end{quote}}\n", pf);
}

static const char *
PlayerSymbol(int fPlayer)
{

    return fPlayer ? "\\textbullet{}" : "\\textopenbullet{}";
}

static void
ExportGameLaTeX(FILE * pf, listOLD * plGame)
{

    listOLD *pl;
    moverecord *pmr;
    matchstate msExport;
    int fTook = FALSE;
    unsigned int i;
    char sz[1024];
    doubletype dt = DT_NORMAL;
    listOLD *pl_hint = NULL;

    updateStatisticsGame(plGame);

    if (game_is_last(plGame))
        pl_hint = game_add_pmr_hint(plGame);
    for (pl = plGame->plNext; pl != plGame; pl = pl->plNext) {
        pmr = pl->p;
        FixMatchState(&msExport, pmr);
        switch (pmr->mt) {
        case MOVE_GAMEINFO:
            dt = DT_NORMAL;
            fputs("\\clearpage\n", pf);
            fputs("\\noindent{\\Large ", pf);
            if (pmr->g.nMatch)
                fprintf(pf, _("%d point%s match (game %d)"),
                        pmr->g.nMatch, (pmr->g.nMatch > 1 ? "s" : ""), pmr->g.i + 1);
            else
                fprintf(pf, _("Money session (game %d)"), pmr->g.i + 1);

            fputs("}\n\n\\vspace{\\baselineskip}\n\n", pf);

            fprintf(pf, "\\noindent\n\\makebox[0.5\\textwidth][s]" "{\\large %s ", PlayerSymbol(0));
            LaTeXEscape(pf, ap[0].szName);
            fprintf(pf, " (%d points)\\hfill}", pmr->g.anScore[0]);

            fprintf(pf, "\\makebox[0.5\\textwidth][s]" "{\\large %s ", PlayerSymbol(1));
            LaTeXEscape(pf, ap[1].szName);
            fprintf(pf, " (%d points)\\hfill}\n\n", pmr->g.anScore[1]);

            break;

        case MOVE_NORMAL:
            dt = DT_NORMAL;
            msExport.fTurn = msExport.fMove = pmr->fPlayer;
            if (fTook)
                /* no need to print board following a double/take */
                fTook = FALSE;
            else
                PrintLaTeXBoard(pf, &msExport, pmr->fPlayer);

            PrintLaTeXCubeAnalysis(pf, &msExport, pmr->fPlayer,
                                   pmr->CubeDecPtr->aarOutput, pmr->CubeDecPtr->aarStdDev, &pmr->CubeDecPtr->esDouble);
            /* FIXME: output cube skill */

            sprintf(sz, "%s %u%u%s: ", PlayerSymbol(pmr->fPlayer),
                    pmr->anDice[0], pmr->anDice[1], aszLuckTypeLaTeXAbbr[pmr->lt]);
            FormatMove(strchr(sz, 0), (ConstTanBoard) msExport.anBoard, pmr->n.anMove);
            fprintf(pf, "\\begin{center}%s%s\\end{center}\n\n", sz, aszSkillTypeAbbr[pmr->n.stMove]);

            /* FIXME use center and tabular environment instead of verbatim */
            fputs("{\\footnotesize\\begin{verbatim}\n", pf);
            for (i = 0; i < pmr->ml.cMoves; i++) {
                if (i >= exsExport.nMoves && i != pmr->n.iMove)
                    continue;

                putc(i == pmr->n.iMove ? '*' : ' ', pf);
                FormatMoveHint(sz, &msExport, &pmr->ml, i,
                               i != pmr->n.iMove || i != pmr->ml.cMoves - 1 || i < exsExport.nMoves, TRUE, TRUE);
                fputs(sz, pf);
            }
            fputs("\\end{verbatim}}", pf);

            PrintLaTeXComment(pf, pmr->sz);

            break;

        case MOVE_DOUBLE:
            dt = DoubleType(msExport.fDoubled, msExport.fMove, msExport.fTurn);
            if (dt != DT_NORMAL) {
                fprintf(pf, "\\begin{center}\\emph{Cannot analyse doubles nor raccoons!}\\end{center}");
                break;
            }
            PrintLaTeXBoard(pf, &msExport, pmr->fPlayer);

            PrintLaTeXCubeAnalysis(pf, &msExport, pmr->fPlayer,
                                   pmr->CubeDecPtr->aarOutput, pmr->CubeDecPtr->aarStdDev, &pmr->CubeDecPtr->esDouble);

            /* FIXME what about beavers? */
            fprintf(pf, "\\begin{center}%s %s%s\\end{center}\n\n",
                    PlayerSymbol(pmr->fPlayer), _("Double"), aszSkillTypeAbbr[pmr->stCube]);

            PrintLaTeXComment(pf, pmr->sz);

            break;

        case MOVE_TAKE:
            if (dt != DT_NORMAL) {
                dt = DT_NORMAL;
                fprintf(pf, "\\begin{center}\\emph{Cannot analyse doubles nor raccoons!}\\end{center}");
                break;
            }
            fTook = TRUE;
            fprintf(pf, "\\begin{center}%s %s%s\\end{center}\n\n",
                    PlayerSymbol(pmr->fPlayer), _("Take"), aszSkillTypeAbbr[pmr->stCube]);

            PrintLaTeXComment(pf, pmr->sz);

            break;

        case MOVE_DROP:
            if (dt != DT_NORMAL) {
                dt = DT_NORMAL;
                fprintf(pf, "\\begin{center}\\emph{Cannot analyse doubles nor raccoons!}\\end{center}");
                break;
            }
            fprintf(pf, "\\begin{center}%s %s%s\\end{center}\n\n",
                    PlayerSymbol(pmr->fPlayer), _("Drop"), aszSkillTypeAbbr[pmr->stCube]);

            PrintLaTeXComment(pf, pmr->sz);

            break;

        case MOVE_RESIGN:
            fprintf(pf, "\\begin{center}%s %s %s\\end{center}\n\n",
                    PlayerSymbol(pmr->fPlayer), _("Resigns"), gettext(aszGameResult[pmr->r.nResigned - 1]));
            /* FIXME print resignation analysis, if available */
            PrintLaTeXComment(pf, pmr->sz);
            break;

        case MOVE_SETDICE:
            /* ignore */
            break;

        case MOVE_SETBOARD:
        case MOVE_SETCUBEVAL:
        case MOVE_SETCUBEPOS:
            /* FIXME print something? */
            break;
        }

        ApplyMoveRecord(&msExport, plGame, pmr);
    }

    if (pl_hint)
        game_remove_pmr_hint(pl_hint);

    /* FIXME print game result */
}

extern void
CommandExportGameLaTeX(char *sz)
{

    FILE *pf;

    sz = NextToken(&sz);

    if (!plGame) {
        outputl(_("No game in progress (type `new game' to start one)."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to (see `help export " "game latex')."));
        return;
    }

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if ((pf = g_fopen(sz, "w")) == 0) {
        outputerr(sz);
        return;
    }

    LaTeXPrologue(pf);

    ExportGameLaTeX(pf, plGame);

    LaTeXEpilogue(pf);

    if (pf != stdout)
        fclose(pf);

    setDefaultFileName(sz);

}

extern void
CommandExportMatchLaTeX(char *sz)
{

    FILE *pf;
    listOLD *pl;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify a file to export to (see `help export " "match latex')."));
        return;
    }

    if (!confirmOverwrite(sz, fConfirmSave))
        return;

    if (!strcmp(sz, "-"))
        pf = stdout;
    else if ((pf = g_fopen(sz, "w")) == 0) {
        outputerr(sz);
        return;
    }

    LaTeXPrologue(pf);

    /* FIXME write match introduction? */

    for (pl = lMatch.plNext; pl != &lMatch; pl = pl->plNext)
        ExportGameLaTeX(pf, pl->p);

    LaTeXEpilogue(pf);

    if (pf != stdout)
        fclose(pf);

    setDefaultFileName(sz);

}
