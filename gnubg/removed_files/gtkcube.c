
/*
 * gtkcube.c
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
 * $Id: gtkcube.c,v 1.87 2015/01/25 20:43:46 plm Exp $
 */

#include "config.h"
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "backgammon.h"
#include "eval.h"
#include "export.h"
#include "rollout.h"
#include "gtkgame.h"
#include "gtkcube.h"
#include "gtkwindows.h"
#include "progress.h"
#include "format.h"
#include "gtklocdefs.h"


typedef struct _cubehintdata {
    GtkWidget *pwFrame;         /* the table */
    GtkWidget *pw;              /* the box */
    GtkWidget *pwTools;         /* the tools */
    moverecord *pmr;
    matchstate ms;
    int did_double;
    int did_take;
    int hist;
} cubehintdata;


static GtkWidget *
OutputPercentsTable(const float ar[])
{
    GtkWidget *pwTable;
    GtkWidget *pw;
    int i;
    static gchar *headings[] = { N_("Win"), N_("W(g)"), N_("W(bg)"),
        "-", N_("Lose"), N_("L(g)"), N_("L(bg)")
    };
    pwTable = gtk_table_new(2, 7, FALSE);

    for (i = 0; i < 7; i++) {
        pw = gtk_label_new(gettext(headings[i]));
        gtk_table_attach(GTK_TABLE(pwTable), pw, i, i + 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
    }

    pw = gtk_label_new(OutputPercent(ar[OUTPUT_WIN]));
    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
    pw = gtk_label_new(OutputPercent(ar[OUTPUT_WINGAMMON]));
    gtk_table_attach(GTK_TABLE(pwTable), pw, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
    pw = gtk_label_new(OutputPercent(ar[OUTPUT_WINBACKGAMMON]));
    gtk_table_attach(GTK_TABLE(pwTable), pw, 2, 3, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);

    pw = gtk_label_new(" - ");
    gtk_table_attach(GTK_TABLE(pwTable), pw, 3, 4, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);

    pw = gtk_label_new(OutputPercent(1.0f - ar[OUTPUT_WIN]));
    gtk_table_attach(GTK_TABLE(pwTable), pw, 4, 5, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
    pw = gtk_label_new(OutputPercent(ar[OUTPUT_LOSEGAMMON]));
    gtk_table_attach(GTK_TABLE(pwTable), pw, 5, 6, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);
    pw = gtk_label_new(OutputPercent(ar[OUTPUT_LOSEBACKGAMMON]));
    gtk_table_attach(GTK_TABLE(pwTable), pw, 6, 7, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 2, 0);

    return pwTable;
}


static GtkWidget *
TakeAnalysis(cubehintdata * pchd)
{
    cubeinfo ci;
    GtkWidget *pw;
    GtkWidget *pwTable;
    GtkWidget *pwFrame;
    int iRow;
    int i;
    cubedecision cd;
    int ai[2];
    const char *aszCube[] = {
        NULL, NULL,
        N_("Take"),
        N_("Pass")
    };
    float arDouble[4];
    gchar *sz;
    cubedecisiondata *cdec = pchd->pmr->CubeDecPtr;
    const evalsetup *pes = &cdec->esDouble;


    if (pes->et == EVAL_NONE)
        return NULL;

    GetMatchStateCubeInfo(&ci, &pchd->ms);

    cd = FindCubeDecision(arDouble, cdec->aarOutput, &ci);

    /* header */

    pwFrame = gtk_frame_new(_("Take analysis"));
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 8);

    pwTable = gtk_table_new(6, 4, FALSE);
    gtk_container_add(GTK_CONTAINER(pwFrame), pwTable);

    /* if EVAL_EVAL include cubeless equity and winning percentages */

    iRow = 0;

    InvertEvaluationR(cdec->aarOutput[0], &ci);
    InvertEvaluationR(cdec->aarOutput[1], &ci);

    switch (pes->et) {
    case EVAL_EVAL:
        if (ci.nMatchTo)
            sz = g_strdup_printf(_("Cubeless %d-ply %s: %s (Money: %s)"),
                                 pes->ec.nPlies,
                                 fOutputMWC ? _("MWC") : _("equity"),
                                 OutputEquity(cdec->aarOutput[0][OUTPUT_EQUITY],
                                              &ci, TRUE), OutputMoneyEquity(cdec->aarOutput[0], TRUE));
        else
            sz = g_strdup_printf(_("Cubeless %d-ply equity: %s"),
                                 pes->ec.nPlies, OutputMoneyEquity(cdec->aarOutput[0], TRUE));

        break;

    case EVAL_ROLLOUT:
        if (ci.nMatchTo)
            sz = g_strdup_printf(_("Cubeless rollout %s: %s (Money: %s)"),
                                 fOutputMWC ? _("MWC") : _("equity"),
                                 OutputEquity(cdec->aarOutput[0][OUTPUT_EQUITY],
                                              &ci, TRUE), OutputMoneyEquity(cdec->aarOutput[0], TRUE));
        else
            sz = g_strdup_printf(_("Cubeless rollout equity: %s"), OutputMoneyEquity(cdec->aarOutput[0], TRUE));

        break;

    default:

        sz = g_strdup("");

    }

    pw = gtk_label_new(sz);
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
    g_free(sz);

    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
    iRow++;

    /* winning percentages */

    switch (pes->et) {

    case EVAL_EVAL:

        pw = OutputPercentsTable(cdec->aarOutput[0]);
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);
        iRow++;

        break;

    case EVAL_ROLLOUT:

        pw = OutputPercentsTable(cdec->aarOutput[1]);
        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);
        iRow++;

        break;

    default:

        g_assert_not_reached();

    }

    InvertEvaluationR(cdec->aarOutput[0], &ci);
    InvertEvaluationR(cdec->aarOutput[1], &ci);

    /* sub-header */

    pw = gtk_label_new(_("Cubeful equities:"));
    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);
    gtk_misc_set_alignment(GTK_MISC(pw), 0.0, 0.5);
    iRow++;

    /* ordered take actions with equities */

    if (arDouble[OUTPUT_TAKE] < arDouble[OUTPUT_DROP]) {
        ai[0] = OUTPUT_TAKE;
        ai[1] = OUTPUT_DROP;
    } else {
        ai[0] = OUTPUT_DROP;
        ai[1] = OUTPUT_TAKE;
    }

    for (i = 0; i < 2; i++) {

        /* numbering */

        sz = g_strdup_printf("%d.", i + 1);
        pw = gtk_label_new(sz);
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
        g_free(sz);

        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         0, 1, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);

        /* label */

        pw = gtk_label_new(gettext(aszCube[ai[i]]));
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         1, 2, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);

        /* equity */

        if (!ci.nMatchTo || (ci.nMatchTo && !fOutputMWC))
            sz = g_strdup_printf("%+7.3f", -arDouble[ai[i]]);
        else
            sz = g_strdup_printf("%7.3f%%", 100.0f * (1.0f - eq2mwc(arDouble[ai[i]], &ci)));

        pw = gtk_label_new(sz);
        gtk_misc_set_alignment(GTK_MISC(pw), 1, 0.5);
        g_free(sz);

        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         2, 3, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);

        /* difference */

        if (i) {

            if (!ci.nMatchTo || (ci.nMatchTo && !fOutputMWC))
                sz = g_strdup_printf("%+7.3f", arDouble[ai[0]] - arDouble[ai[i]]);
            else
                sz = g_strdup_printf("%+7.3f%%",
                                     100.0f * eq2mwc(arDouble[ai[0]], &ci) - 100.0f * eq2mwc(arDouble[ai[i]], &ci));

            pw = gtk_label_new(sz);
            gtk_misc_set_alignment(GTK_MISC(pw), 1, 0.5);
            g_free(sz);

            gtk_table_attach(GTK_TABLE(pwTable), pw,
                             3, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);

        }

        iRow++;

    }

    /* proper cube action */

    pw = gtk_label_new(_("Correct response: "));
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 2, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);

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
        pw = gtk_label_new(_("Take"));
        break;

    case DOUBLE_PASS:
    case TOOGOOD_PASS:
    case REDOUBLE_PASS:
    case TOOGOODRE_PASS:
    case OPTIONAL_DOUBLE_PASS:
    case OPTIONAL_REDOUBLE_PASS:
        pw = gtk_label_new(_("Pass"));
        break;

    case DOUBLE_BEAVER:
    case NODOUBLE_BEAVER:
    case NO_REDOUBLE_BEAVER:
    case OPTIONAL_DOUBLE_BEAVER:
        pw = gtk_label_new(_("Beaver!"));
        break;

    case NOT_AVAILABLE:
        pw = gtk_label_new(_("Eat it!"));
        break;

    }

    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

    gtk_table_attach(GTK_TABLE(pwTable), pw, 2, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);

    return pwFrame;

}


/*
 * Make cube analysis widget 
 *
 * Input:
 *   aarOutput, aarStdDev: evaluations
 *   pes: evaluation setup
 *
 * Returns:
 *   nice and pretty widget with cube analysis
 *
 */

static GtkWidget *
CubeAnalysis(cubehintdata * pchd)
{
    cubeinfo ci;
    GtkWidget *pw;
    GtkWidget *pwTable;
    GtkWidget *pwFrame;
    int iRow;
    int i;
    cubedecision cd;
    float r;
    int ai[3];
    const char *aszCube[] = {
        NULL,
        N_("No double"),
        N_("Double, take"),
        N_("Double, pass")
    };
    float arDouble[4];
    gchar *sz;
    cubedecisiondata *cdec = pchd->pmr->CubeDecPtr;
    const evalsetup *pes = &cdec->esDouble;


    if (pes->et == EVAL_NONE)
        return NULL;

    GetMatchStateCubeInfo(&ci, &pchd->ms);

    cd = FindCubeDecision(arDouble, cdec->aarOutput, &ci);

    if (!GetDPEq(NULL, NULL, &ci) && !(pchd->pmr->mt == MOVE_DOUBLE))
        /* No cube action possible */
        return NULL;

    /* header */

    pwFrame = gtk_frame_new(_("Cube analysis"));
    gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 8);

    pw = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pwFrame), pw);

    pwTable = gtk_table_new(8, 4, FALSE);
    gtk_box_pack_start(GTK_BOX(pw), pwTable, FALSE, FALSE, 0);

    iRow = 0;

    /* cubeless equity */

    /* Define first row of text (in sz) */
    switch (pes->et) {
    case EVAL_EVAL:
        if (ci.nMatchTo)
            sz = g_strdup_printf(_("Cubeless %d-ply %s: %s (Money: %s)"),
                                 pes->ec.nPlies,
                                 fOutputMWC ? _("MWC") : _("equity"),
                                 OutputEquity(cdec->aarOutput[0][OUTPUT_EQUITY],
                                              &ci, TRUE), OutputMoneyEquity(cdec->aarOutput[0], TRUE));
        else
            sz = g_strdup_printf(_("Cubeless %d-ply equity: %s"),
                                 pes->ec.nPlies, OutputMoneyEquity(cdec->aarOutput[0], TRUE));

        break;

    case EVAL_ROLLOUT:
        if (ci.nMatchTo)
            sz = g_strdup_printf(_("Cubeless rollout %s: %s (Money: %s)"),
                                 fOutputMWC ? _("MWC") : _("equity"),
                                 OutputEquity(cdec->aarOutput[0][OUTPUT_EQUITY],
                                              &ci, TRUE), OutputMoneyEquity(cdec->aarOutput[0], TRUE));
        else
            sz = g_strdup_printf(_("Cubeless rollout equity: %s"), OutputMoneyEquity(cdec->aarOutput[0], TRUE));

        break;

    default:

        sz = g_strdup("");

    }

    pw = gtk_label_new(sz);
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
    g_free(sz);

    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);

    iRow++;


    /* if EVAL_EVAL include cubeless equity and winning percentages */

    if (pes->et == EVAL_EVAL) {

        pw = OutputPercentsTable(cdec->aarOutput[0]);

        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);

        iRow++;

    }

    pw = gtk_label_new(_("Cubeful equities:"));
    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);
    gtk_misc_set_alignment(GTK_MISC(pw), 0.0, 0.5);
    iRow++;


    getCubeDecisionOrdering(ai, arDouble, cdec->aarOutput, &ci);

    for (i = 0; i < 3; i++) {

        /* numbering */

        sz = g_strdup_printf("%d.", i + 1);
        pw = gtk_label_new(sz);
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);
        g_free(sz);

        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         0, 1, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);

        /* label */

        pw = gtk_label_new(gettext(aszCube[ai[i]]));
        gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         1, 2, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);

        /* equity */

        sz = OutputEquity(arDouble[ai[i]], &ci, TRUE);

        pw = gtk_label_new(sz);
        gtk_misc_set_alignment(GTK_MISC(pw), 1, 0.5);

        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         2, 3, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);

        /* difference */

        if (i) {

            pw = gtk_label_new(OutputEquityDiff(arDouble[ai[i]], arDouble[OUTPUT_OPTIMAL], &ci));
            gtk_misc_set_alignment(GTK_MISC(pw), 1, 0.5);

            gtk_table_attach(GTK_TABLE(pwTable), pw,
                             3, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);

        }

        iRow++;

        /* rollout details */

        if (pes->et == EVAL_ROLLOUT && (ai[i] == OUTPUT_TAKE || ai[i] == OUTPUT_NODOUBLE)) {

            /* FIXME: output cubeless equities and percentages for rollout */
            /*        probably along with rollout details */

            pw = gtk_label_new(OutputPercents(cdec->aarOutput[ai[i] - 1], TRUE));
            gtk_misc_set_alignment(GTK_MISC(pw), 0.5, 0.5);

            gtk_table_attach(GTK_TABLE(pwTable), pw,
                             0, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 4);

            iRow++;

        }

    }

    /* proper cube action */

    pw = gtk_label_new(_("Proper cube action: "));
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

    gtk_table_attach(GTK_TABLE(pwTable), pw, 0, 2, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);

    pw = gtk_label_new(GetCubeRecommendation(cd));
    gtk_misc_set_alignment(GTK_MISC(pw), 0, 0.5);

    gtk_table_attach(GTK_TABLE(pwTable), pw, 2, 3, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);

    /* percent */

    if ((r = getPercent(cd, arDouble)) >= 0.0f) {

        sz = g_strdup_printf("(%.1f%%)", 100.0f * r);
        pw = gtk_label_new(sz);
        g_free(sz);
        gtk_misc_set_alignment(GTK_MISC(pw), 1, 0.5);

        gtk_table_attach(GTK_TABLE(pwTable), pw,
                         3, 4, iRow, iRow + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 8);

    }

    return pwFrame;
}

static void
UpdateCubeAnalysis(cubehintdata * pchd)
{

    GtkWidget *pw = 0;
    doubletype dt = DoubleType(ms.fDoubled, ms.fMove, ms.fTurn);

    find_skills(pchd->pmr, &ms, pchd->did_double, pchd->did_take);

    switch (pchd->pmr->mt) {
    case MOVE_NORMAL:
    case MOVE_SETDICE:
    case MOVE_SETCUBEPOS:
    case MOVE_SETCUBEVAL:
        /* See CreateCubeAnalysis() for these  MOVE_SET* cases */
    case MOVE_DOUBLE:
        if (dt == DT_NORMAL)
            pw = CubeAnalysis(pchd);
        else if (dt == DT_BEAVER)
            pw = TakeAnalysis(pchd);
        break;

    case MOVE_DROP:
    case MOVE_TAKE:
        pw = TakeAnalysis(pchd);
        break;

    default:
        g_assert_not_reached();
        break;

    }

    /* destroy current analysis */

    gtk_container_remove(GTK_CONTAINER(pchd->pw), pchd->pwFrame);

    /* re-create analysis + tools */

    gtk_box_pack_start(GTK_BOX(pchd->pw), pchd->pwFrame = pw, FALSE, FALSE, 0);

    gtk_widget_show_all(pchd->pw);

}




static void
CubeAnalysisRollout(GtkWidget * pw, cubehintdata * pchd)
{

    cubeinfo ci;
    float aarOutput[2][NUM_ROLLOUT_OUTPUTS];
    float aarStdDev[2][NUM_ROLLOUT_OUTPUTS];
    cubedecisiondata *cdec = pchd->pmr->CubeDecPtr;
    rolloutstat aarsStatistics[2][2];
    evalsetup *pes = &cdec->esDouble;
    char asz[2][40];
    void *p;

    if (pes->et != EVAL_ROLLOUT) {
        pes->rc = rcRollout;
        pes->rc.nGamesDone = 0;
    } else {
        pes->rc.nTrials = rcRollout.nTrials;
        pes->rc.fStopOnSTD = rcRollout.fStopOnSTD;
        pes->rc.nMinimumGames = rcRollout.nMinimumGames;
        pes->rc.rStdLimit = rcRollout.rStdLimit;
        memcpy(aarOutput, cdec->aarOutput, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));
        memcpy(aarStdDev, cdec->aarStdDev, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    }

    GetMatchStateCubeInfo(&ci, &pchd->ms);

    FormatCubePositions(&ci, asz);
    GTKSetCurrentParent(pw);
    RolloutProgressStart(&ci, 2, aarsStatistics, &pes->rc, asz, FALSE, &p);

    if (GeneralCubeDecisionR(aarOutput, aarStdDev, aarsStatistics,
                             (ConstTanBoard) pchd->ms.anBoard, &ci, &pes->rc, pes, RolloutProgress, p) < 0) {
        RolloutProgressEnd(&p, FALSE);
        return;
    }

    RolloutProgressEnd(&p, FALSE);

    memcpy(cdec->aarOutput, aarOutput, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));
    memcpy(cdec->aarStdDev, aarStdDev, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));

    if (pes->et != EVAL_ROLLOUT)
        memcpy(&pes->rc, &rcRollout, sizeof(rcRollout));

    pes->et = EVAL_ROLLOUT;

    /* If the source widget parent has been destroyed do not attempt
     * to update the hint window */
    if (!GDK_IS_WINDOW(gtk_widget_get_parent_window(pw)))
        return;

    UpdateCubeAnalysis(pchd);
    if (pchd->hist)
        ChangeGame(NULL);
}

static void
EvalCube(cubehintdata * pchd, evalcontext * pec)
{
    cubeinfo ci;
    decisionData dd;
    cubedecisiondata *cdec = pchd->pmr->CubeDecPtr;
    evalsetup *pes = &cdec->esDouble;

    GetMatchStateCubeInfo(&ci, &pchd->ms);

    dd.pboard = (ConstTanBoard) pchd->ms.anBoard;
    dd.pci = &ci;
    dd.pec = pec;
    dd.pes = NULL;
    if (RunAsyncProcess((AsyncFun) asyncCubeDecisionE, &dd, _("Considering cube action...")) != 0)
        return;

    /* save evaluation */


    memcpy(cdec->aarOutput, dd.aarOutput, 2 * NUM_ROLLOUT_OUTPUTS * sizeof(float));

    pes->et = EVAL_EVAL;
    memcpy(&pes->ec, dd.pec, sizeof(evalcontext));

    UpdateCubeAnalysis(pchd);
    if (pchd->hist)
        ChangeGame(NULL);
}

static void
CubeAnalysisEvalPly(GtkWidget * pw, cubehintdata * pchd)
{

    char *szPly = (char *) g_object_get_data(G_OBJECT(pw), "ply");
    evalcontext ec = { 0, 0, 0, TRUE, 0.0 };

    ec.fCubeful = esAnalysisCube.ec.fCubeful;
    ec.fUsePrune = esAnalysisCube.ec.fUsePrune;
    ec.nPlies = atoi(szPly);

    EvalCube(pchd, &ec);

}

static void
CubeAnalysisEval(GtkWidget * UNUSED(pw), cubehintdata * pchd)
{

    EvalCube(pchd, &GetEvalCube()->ec);

}

static void
CubeAnalysisEvalSettings(GtkWidget * pw, void *UNUSED(data))
{
    GTKSetCurrentParent(pw);
    SetAnalysis(NULL, 0, NULL);

    /* bring the dialog holding this button to the top */
    gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(pw)));

}

static void
CubeAnalysisRolloutSettings(GtkWidget * pw, void *UNUSED(data))
{
    GTKSetCurrentParent(pw);
    SetRollouts(NULL, 0, NULL);

    /* bring the dialog holding this button to the top */
    gtk_window_present(GTK_WINDOW(gtk_widget_get_toplevel(pw)));

}


static void
CubeRolloutPresets(GtkWidget * pw, cubehintdata * pchd)
{
    const gchar *preset;
    gchar *file = NULL;
    gchar *path = NULL;
    gchar *command = NULL;

    preset = (const gchar *) g_object_get_data(G_OBJECT(pw), "user_data");
    file = g_strdup_printf("%s.rol", preset);
    path = g_build_filename(szHomeDirectory, "rol", file, NULL);
    if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        command = g_strdup_printf("load commands \"%s\"", path);
        outputoff();
        UserCommand(command);
        outputon();
        CubeAnalysisRollout(pw, pchd);
    } else {
        outputerrf(_("You need to save a preset as \"%s\""), file);
        CubeAnalysisRolloutSettings(pw, NULL);
    }
    g_free(file);
    g_free(path);
    g_free(command);
}

static void
CubeAnalysisMWC(GtkWidget * pw, cubehintdata * pchd)
{

    char sz[80];
    int f = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));

    if (f != fOutputMWC) {
        sprintf(sz, "set output mwc %s", fOutputMWC ? "off" : "on");

        UserCommand(sz);
        UserCommand("save settings");
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw), fOutputMWC);

    UpdateCubeAnalysis(pchd);

}

static char *
GetContent(cubehintdata * pchd)
{
    static char *pc;
    cubeinfo ci;
    cubedecisiondata *cdec = pchd->pmr->CubeDecPtr;

    GetMatchStateCubeInfo(&ci, &pchd->ms);
    pc = OutputCubeAnalysis(cdec->aarOutput, cdec->aarStdDev, &cdec->esDouble, &ci);

    return pc;
}

static void
CubeAnalysisCopy(GtkWidget * UNUSED(pw), cubehintdata * pchd)
{

    char *pc = GetContent(pchd);

    if (pc)
        GTKTextWindow(pc, _("Cube details"), DT_INFO, NULL);

}

static void
CubeAnalysisTempMap(GtkWidget * UNUSED(pw), cubehintdata * UNUSED(pchd))
{

    char *sz = g_strdup("show temperaturemap =cube");
    UserCommand(sz);
    g_free(sz);

}

static void
CubeAnalysisCmark(GtkWidget * pw, cubehintdata * pchd)
{
    pchd->pmr->CubeDecPtr->cmark = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pw));
}

static GtkWidget *
CreateCubeAnalysisTools(cubehintdata * pchd)
{


    GtkWidget *pwTools;
    GtkWidget *pwEval = gtk_button_new_with_label(_("Eval"));
    GtkWidget *pwply;
    GtkWidget *pwEvalSettings = gtk_button_new_with_label(_("..."));
    GtkWidget *pwRollout = gtk_button_new_with_label(_("Rollout"));
    GtkWidget *pwRolloutPresets = gtk_hbox_new(FALSE, 0);
    GtkWidget *pwRolloutSettings = gtk_button_new_with_label(_("..."));
    GtkWidget *pwMWC = gtk_toggle_button_new_with_label(_("MWC"));
    GtkWidget *pwCopy = gtk_button_new_with_label(_("Copy"));
    GtkWidget *pwTempMap = gtk_button_new_with_label(_("Temp. Map"));
    GtkWidget *pwCmark = gtk_toggle_button_new_with_label(_("Cmark"));
    GtkWidget *pw;
    int i;
    char *sz;

    /* toolbox on the left with buttons for eval, rollout and more */

    pchd->pwTools = pwTools = gtk_table_new(2, 5, FALSE);

    gtk_table_attach(GTK_TABLE(pwTools), pwEval, 0, 1, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwEvalSettings, 1, 2, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    pw = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(pwTools), pw, 2, 3, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    for (i = 0; i < 5; ++i) {

        sz = g_strdup_printf("%d", i);  /* string is freed by set_data_full */
        pwply = gtk_button_new_with_label(sz);

        gtk_box_pack_start(GTK_BOX(pw), pwply, TRUE, TRUE, 0);

        g_signal_connect(G_OBJECT(pwply), "clicked", G_CALLBACK(CubeAnalysisEvalPly), pchd);

        g_object_set_data_full(G_OBJECT(pwply), "ply", sz, g_free);

        sz = g_strdup_printf(_("Evaluate play on cubeful %d-ply"), i);
        gtk_widget_set_tooltip_text(pwply, sz);
        g_free(sz);

    }

    gtk_table_attach(GTK_TABLE(pwTools), pwMWC, 3, 4, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwTempMap, 4, 5, 0, 1,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwCmark, 4, 5, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);


    gtk_table_attach(GTK_TABLE(pwTools), pwRollout, 0, 1, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwRolloutSettings, 1, 2, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_table_attach(GTK_TABLE(pwTools), pwRolloutPresets, 2, 3, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    for (i = 0; i < 5; ++i) {
        GtkWidget *ro_preset;
        sz = g_strdup_printf("%c", i + 'a');    /* string is freed by set_data_full */
        ro_preset = gtk_button_new_with_label(sz);

        gtk_box_pack_start(GTK_BOX(pwRolloutPresets), ro_preset, TRUE, TRUE, 0);

        g_signal_connect(G_OBJECT(ro_preset), "clicked", G_CALLBACK(CubeRolloutPresets), pchd);

        g_object_set_data_full(G_OBJECT(ro_preset), "user_data", sz, g_free);

        sz = g_strdup_printf(_("Rollout preset %c"), i + 'a');
        gtk_widget_set_tooltip_text(ro_preset, sz);
        g_free(sz);

    }

    gtk_table_attach(GTK_TABLE(pwTools), pwCopy, 3, 4, 1, 2,
                     (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

    gtk_widget_set_sensitive(pwMWC, pchd->ms.nMatchTo);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwCmark), pchd->pmr->CubeDecPtr->cmark);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwMWC), fOutputMWC);

    /* signals */

    g_signal_connect(G_OBJECT(pwRollout), "clicked", G_CALLBACK(CubeAnalysisRollout), pchd);
    g_signal_connect(G_OBJECT(pwEval), "clicked", G_CALLBACK(CubeAnalysisEval), pchd);
    g_signal_connect(G_OBJECT(pwEvalSettings), "clicked", G_CALLBACK(CubeAnalysisEvalSettings), NULL);
    g_signal_connect(G_OBJECT(pwRolloutSettings), "clicked", G_CALLBACK(CubeAnalysisRolloutSettings), NULL);
    g_signal_connect(G_OBJECT(pwMWC), "toggled", G_CALLBACK(CubeAnalysisMWC), pchd);
    g_signal_connect(G_OBJECT(pwCopy), "clicked", G_CALLBACK(CubeAnalysisCopy), pchd);
    g_signal_connect(G_OBJECT(pwTempMap), "clicked", G_CALLBACK(CubeAnalysisTempMap), pchd);
    g_signal_connect(G_OBJECT(pwCmark), "toggled", G_CALLBACK(CubeAnalysisCmark), pchd);

    /* tool tips */

    gtk_widget_set_tooltip_text(pwRollout, _("Rollout cube decision with current settings"));

    gtk_widget_set_tooltip_text(pwEval, _("Evaluate cube decision with current settings"));

    gtk_widget_set_tooltip_text(pwRolloutSettings, _("Modify rollout settings"));

    gtk_widget_set_tooltip_text(pwEvalSettings, _("Modify evaluation settings"));

    gtk_widget_set_tooltip_text(pwMWC, _("Toggle output as MWC or equity"));

    gtk_widget_set_tooltip_text(pwCopy, _("Copy"));

    gtk_widget_set_tooltip_text(pwTempMap, _("Show Sho Sengoku Temperature Map of position " "after selected move"));

    return pwTools;

}


extern GtkWidget *
CreateCubeAnalysis(moverecord * pmr, const matchstate * pms, int did_double, int did_take, int hist)
{
    doubletype dt = DoubleType(pms->fDoubled, pms->fMove, pms->fTurn);
    cubehintdata *pchd = g_new0(cubehintdata, 1);

    GtkWidget *pw, *pwhb, *pwx;

    pchd->pmr = pmr;
    pchd->ms = *pms;
    pchd->did_double = did_double;
    pchd->did_take = did_take;
    pchd->hist = hist;
    pchd->pw = pw = gtk_hbox_new(FALSE, 2);
    switch (pmr->mt) {

    case MOVE_NORMAL:
    case MOVE_SETDICE:
        /*
         * We're stepping back in the game record after rolling and
         * asking for a hint on the missed cube decision, for instance
         */
    case MOVE_SETCUBEPOS:
    case MOVE_SETCUBEVAL:
        /*
         * These two happen in cube decisions loaded from a .sgf file
         */
    case MOVE_DOUBLE:
        if (dt == DT_NORMAL)
            pchd->pwFrame = CubeAnalysis(pchd);
        else if (dt == DT_BEAVER)
            pchd->pwFrame = TakeAnalysis(pchd);
        break;
    case MOVE_TAKE:
    case MOVE_DROP:
        pchd->pwFrame = TakeAnalysis(pchd);
        break;
    default:
        g_assert_not_reached();
        break;

    }

    if (!pchd->pwFrame)
        return NULL;

    gtk_box_pack_start(GTK_BOX(pw), pchd->pwFrame, FALSE, FALSE, 0);

    pwx = gtk_vbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(pwx), pw, FALSE, FALSE, 0);

    pwhb = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwhb), CreateCubeAnalysisTools(pchd), FALSE, FALSE, 8);

    gtk_box_pack_start(GTK_BOX(pwx), pwhb, FALSE, FALSE, 0);

    /* hook to free cubehintdata on widget destruction */
    g_object_set_data_full(G_OBJECT(pw), "cubehintdata", pchd, g_free);

    return pwx;

}
