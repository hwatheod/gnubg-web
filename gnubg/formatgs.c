/*
 * formatgs.c
 *
 * by Joern Thyssen  <jth@gnubg.org>, 2003
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
 * $Id: formatgs.c,v 1.34 2013/06/16 02:16:12 mdpetch Exp $
 */

#include "config.h"
#include "backgammon.h"

#include <glib.h>

#include "formatgs.h"
#include "format.h"
#include "analysis.h"

#include "export.h"


static const char *
total_text(int nMatchTo)
{
    if (nMatchTo)
        return _("EMG (MWC)");
    else
        return _("EMG (Points)");
}

static const char *
rate_text(int nMatchTo)
{
    if (nMatchTo)
        return _("mEMG (MWC)");
    else
        return _("mEMG (Points)");
}

static char **
numberEntry(const char *sz, const int n0, const int n1)
{

    char **aasz;

    aasz = g_malloc(3 * sizeof(*aasz));

    aasz[0] = g_strdup(sz);
    aasz[1] = g_strdup_printf("%3d", n0);
    aasz[2] = g_strdup_printf("%3d", n1);

    return aasz;

}

static char *
errorRate(const float rn, const float ru, const int nMatchTo)
{
    int n = fOutputDigits - (int) (log10(rErrorRateFactor) - 0.5);
    n = MAX(n, 0);

    if (nMatchTo) {
        return g_strdup_printf("     %+*.*f (%+7.3f%%)", n + 5, n + 2, rn, ru * 100.0f);
    } else {
        return g_strdup_printf("     %+*.*f (%+7.3f)", n + 5, n + 2, rn, ru);
    }
}

static char *
errorRateMP(const float rn, const float ru, const int nMatchTo)
{

    int n = fOutputDigits - (int) (log10(rErrorRateFactor) - 0.5);
    n = MAX(n, 0);

    if (nMatchTo) {
        return g_strdup_printf("   %+*.*f   (%+7.3f%%)", n + 5, n, rErrorRateFactor * rn, ru * 100.0f);
    } else {
        return g_strdup_printf("   %+*.*f   (%+7.3f)", n + 5, n, rErrorRateFactor * rn, ru);
    }
}

static char *
cubeEntry(const int n, const float rn, const float ru, const int nMatchTo)
{

    if (!n)
        return g_strdup("  0");
    else {
        if (nMatchTo)
            return g_strdup_printf("%3d (%+6.3f (%+7.3f%%))", n, rn, 100.0f * ru);
        else
            return g_strdup_printf("%3d (%+6.3f (%+7.3f))", n, rn, ru);
    }

}


static char **
luckAdjust(const char *sz, const float ar[2], const int nMatchTo)
{

    char **aasz;
    int i;

    aasz = g_malloc(3 * sizeof(*aasz));

    aasz[0] = g_strdup(sz);

    for (i = 0; i < 2; ++i)
        if (nMatchTo)
            aasz[i + 1] = g_strdup_printf("%+7.2f%%", 100.0f * ar[i]);
        else
            aasz[i + 1] = g_strdup_printf("%+7.3f", ar[i]);

    return aasz;

}

extern GList *
formatGS(const statcontext * psc, const int nMatchTo, const enum _formatgs fg)
{

    GList *list = NULL;
    char **aasz;
    float aaaar[3][2][2][2];

    getMWCFromError(psc, aaaar);

    switch (fg) {
    case FORMATGS_CHEQUER:
        {

            static int ai[4] = { SKILL_NONE, SKILL_DOUBTFUL,
                SKILL_BAD, SKILL_VERYBAD
            };
            static const char *asz[4] = {
                N_("Unmarked moves"),
                N_("Moves marked doubtful"),
                N_("Moves marked bad"),
                N_("Moves marked very bad")
            };
            int i;

            /* chequer play part */

            list = g_list_append(list, numberEntry(_("Total moves"), psc->anTotalMoves[0], psc->anTotalMoves[1]));

            list = g_list_append(list, numberEntry(_("Unforced moves"),
                                                   psc->anUnforcedMoves[0], psc->anUnforcedMoves[1]));
            for (i = 0; i < 4; ++i)
                list = g_list_append(list,
                                     numberEntry(gettext(asz[i]), psc->anMoves[0][ai[i]], psc->anMoves[1][ai[i]]));

            /* total error rate */

            aasz = g_malloc(3 * sizeof(*aasz));

            aasz[0] = g_strdup_printf(_("Error total %s"), total_text(nMatchTo));

            for (i = 0; i < 2; ++i)
                aasz[i + 1] = errorRate(-aaaar[CHEQUERPLAY][TOTAL][i][NORMALISED],
                                        -aaaar[CHEQUERPLAY][TOTAL][i][UNNORMALISED], nMatchTo);

            list = g_list_append(list, aasz);

            /* error rate per move */

            aasz = g_malloc(3 * sizeof(*aasz));

            aasz[0] = g_strdup_printf(_("Error rate %s"), rate_text(nMatchTo));

            for (i = 0; i < 2; ++i)
                aasz[i + 1] = errorRateMP(-aaaar[CHEQUERPLAY][PERMOVE][i][NORMALISED],
                                          -aaaar[CHEQUERPLAY][PERMOVE][i][UNNORMALISED], nMatchTo);

            list = g_list_append(list, aasz);

            /* chequer play rating */

            aasz = g_malloc(3 * sizeof(*aasz));

            aasz[0] = g_strdup(_("Chequerplay rating"));

            for (i = 0; i < 2; ++i)
                if (psc->anUnforcedMoves[i])
                    aasz[i + 1] = g_strdup(Q_(aszRating[GetRating(aaaar[CHEQUERPLAY][PERMOVE][i][NORMALISED])]));
                else
                    aasz[i + 1] = g_strdup(_("n/a"));

            list = g_list_append(list, aasz);

        }

        break;

    case FORMATGS_CUBE:
        {
            static const char *asz[] = {
                N_("Total cube decisions"),
                N_("Close or actual cube decisions"),
                N_("Doubles"),
                N_("Takes"),
                N_("Passes")
            };

            static const char *asz2[] = {
                N_("Missed doubles below CP"),
                N_("Missed doubles above CP"),
                N_("Wrong doubles below DP"),
                N_("Wrong doubles above TG"),
                N_("Wrong takes"),
                N_("Wrong passes")
            };

            int i, j;

            const int *ai[5], *ai2[6];
            const float *af2[2][6];

            ai[0] = psc->anTotalCube;
            ai[1] = psc->anCloseCube;
            ai[2] = psc->anDouble;
            ai[3] = psc->anTake;
            ai[4] = psc->anPass;

            ai2[0] = psc->anCubeMissedDoubleDP;
            ai2[1] = psc->anCubeMissedDoubleTG;
            ai2[2] = psc->anCubeWrongDoubleDP;
            ai2[3] = psc->anCubeWrongDoubleTG;
            ai2[4] = psc->anCubeWrongTake;
            ai2[5] = psc->anCubeWrongPass;

            af2[0][0] = psc->arErrorMissedDoubleDP[0];
            af2[0][1] = psc->arErrorMissedDoubleTG[0];
            af2[0][2] = psc->arErrorWrongDoubleDP[0];
            af2[0][3] = psc->arErrorWrongDoubleTG[0];
            af2[0][4] = psc->arErrorWrongTake[0];
            af2[0][5] = psc->arErrorWrongPass[0];
            af2[1][0] = psc->arErrorMissedDoubleDP[1];
            af2[1][1] = psc->arErrorMissedDoubleTG[1];
            af2[1][2] = psc->arErrorWrongDoubleDP[1];
            af2[1][3] = psc->arErrorWrongDoubleTG[1];
            af2[1][4] = psc->arErrorWrongTake[1];
            af2[1][5] = psc->arErrorWrongPass[1];

            for (i = 0; i < 5; ++i)
                list = g_list_append(list, numberEntry(gettext(asz[i]), ai[i][0], ai[i][1]));
            for (i = 0; i < 6; ++i) {
                aasz = g_malloc(3 * sizeof(*aasz));

                aasz[0] = g_strdup_printf("%s (%s)", gettext(asz2[i]), total_text(nMatchTo));

                for (j = 0; j < 2; ++j)
                    aasz[j + 1] = cubeEntry(ai2[i][j], -af2[j][i][0], -af2[j][i][1], nMatchTo);

                list = g_list_append(list, aasz);

            }

            /* total error rate */

            aasz = g_malloc(3 * sizeof(*aasz));

            aasz[0] = g_strdup_printf(_("Error total %s"), total_text(nMatchTo));

            for (i = 0; i < 2; ++i)
                aasz[i + 1] = errorRate(-aaaar[CUBEDECISION][TOTAL][i][NORMALISED],
                                        -aaaar[CUBEDECISION][TOTAL][i][UNNORMALISED], nMatchTo);

            list = g_list_append(list, aasz);

            /* error rate per cube decision */

            aasz = g_malloc(3 * sizeof(*aasz));

            aasz[0] = g_strdup_printf(_("Error rate %s"), rate_text(nMatchTo));

            for (i = 0; i < 2; ++i)
                aasz[i + 1] = errorRateMP(-aaaar[CUBEDECISION][PERMOVE][i][NORMALISED],
                                          -aaaar[CUBEDECISION][PERMOVE][i][UNNORMALISED], nMatchTo);

            list = g_list_append(list, aasz);

            /* cube decision rating */

            aasz = g_malloc(3 * sizeof(*aasz));

            aasz[0] = g_strdup(_("Cube decision rating"));

            for (i = 0; i < 2; ++i)
                if (psc->anCloseCube[i])
                    aasz[i + 1] = g_strdup(Q_(aszRating[GetRating(aaaar[CUBEDECISION][PERMOVE][i][NORMALISED])]));
                else
                    aasz[i + 1] = g_strdup(_("n/a"));

            list = g_list_append(list, aasz);



        }
        break;

    case FORMATGS_LUCK:
        {

            static const char *asz[] = {
                N_("Rolls marked very lucky"),
                N_("Rolls marked lucky"),
                N_("Rolls unmarked"),
                N_("Rolls marked unlucky"),
                N_("Rolls marked very unlucky")
            };
            int i;

            for (i = 0; i < 5; ++i)
                list = g_list_append(list, numberEntry(gettext(asz[i]), psc->anLuck[0][4 - i], psc->anLuck[1][4 - i]));


            /* total luck */

            aasz = g_malloc(3 * sizeof(*aasz));

            aasz[0] = g_strdup_printf(_("Luck total %s"), total_text(nMatchTo));

            for (i = 0; i < 2; ++i)
                aasz[i + 1] = errorRate(psc->arLuck[i][0], psc->arLuck[i][1], nMatchTo);

            list = g_list_append(list, aasz);

            /* luck rate per move */

            aasz = g_malloc(3 * sizeof(*aasz));

            aasz[0] = g_strdup_printf(_("Luck rate %s"), rate_text(nMatchTo));

            for (i = 0; i < 2; ++i)
                if (psc->anTotalMoves[i])
                    aasz[i + 1] = errorRateMP(psc->arLuck[i][0] /
                                              psc->anTotalMoves[i], psc->arLuck[i][1] / psc->anTotalMoves[i], nMatchTo);
                else
                    aasz[i + 1] = g_strdup(_("n/a"));

            list = g_list_append(list, aasz);

            /* chequer play rating */

            aasz = g_malloc(3 * sizeof(*aasz));

            aasz[0] = g_strdup(_("Luck rating"));

            for (i = 0; i < 2; ++i)
                if (psc->anTotalMoves[i])
                    aasz[i + 1] = g_strdup(Q_(aszLuckRating[getLuckRating(psc->arLuck[i][0] / psc->anTotalMoves[i])]));
                else
                    aasz[i + 1] = g_strdup(_("n/a"));

            list = g_list_append(list, aasz);

        }
        break;

    case FORMATGS_OVERALL:

        {
            int i, n;

            /* total error rate */

            aasz = g_malloc(3 * sizeof(*aasz));

            if (psc->fCube || psc->fMoves) {

                aasz[0] = g_strdup_printf(_("Error total %s"), total_text(nMatchTo));

                for (i = 0; i < 2; ++i)
                    aasz[i + 1] = errorRate(-aaaar[COMBINED][TOTAL][i][NORMALISED],
                                            -aaaar[COMBINED][TOTAL][i][UNNORMALISED], nMatchTo);

                list = g_list_append(list, aasz);

                /* error rate per decision */

                aasz = g_malloc(3 * sizeof(*aasz));

                aasz[0] = g_strdup_printf(_("Error rate %s"), rate_text(nMatchTo));

                for (i = 0; i < 2; ++i)
                    aasz[i + 1] = errorRateMP(-aaaar[COMBINED][PERMOVE][i][NORMALISED],
                                              -aaaar[COMBINED][PERMOVE][i][UNNORMALISED], nMatchTo);

                list = g_list_append(list, aasz);

                /* eq. snowie error rate */

                aasz = g_malloc(3 * sizeof(*aasz));

                aasz[0] = g_strdup(_("Snowie error rate"));

                for (i = 0; i < 2; ++i)
                    if ((n = psc->anTotalMoves[0] + psc->anTotalMoves[1]) > 0)
                        aasz[i + 1] = errorRateMP(-aaaar[COMBINED][TOTAL][i][NORMALISED] / n, 0.0f, nMatchTo);
                    else
                        aasz[i + 1] = g_strdup(_("n/a"));

                list = g_list_append(list, aasz);

                /* rating */

                aasz = g_malloc(3 * sizeof(*aasz));

                aasz[0] = g_strdup(_("Overall rating"));

                for (i = 0; i < 2; ++i)
                    if (psc->anCloseCube[i] + psc->anUnforcedMoves[i])
                        aasz[i + 1] = g_strdup(Q_(aszRating[GetRating(aaaar[COMBINED][PERMOVE][i][NORMALISED])]));
                    else
                        aasz[i + 1] = g_strdup(_("n/a"));

                list = g_list_append(list, aasz);

            }

            if (psc->fDice) {

                /* luck adj. result */

                if ((psc->arActualResult[0] > 0.0f || psc->arActualResult[1] > 0.0f) && psc->fDice) {

                    list = g_list_append(list, luckAdjust(_("Actual result"), psc->arActualResult, nMatchTo));

                    list = g_list_append(list, luckAdjust(_("Luck adjusted result"), psc->arLuckAdj, nMatchTo));

                    if (nMatchTo) {

                        /* luck based fibs rating */

                        float r = 0.5f + psc->arActualResult[0] - psc->arLuck[0][1] + psc->arLuck[1][1];

                        aasz = g_malloc(3 * sizeof(*aasz));

                        aasz[0] = g_strdup(_("Luck based FIBS rating diff."));
                        aasz[2] = g_strdup("");

                        if (r > 0.0f && r < 1.0f)
                            aasz[1] = g_strdup_printf("%+7.2f", relativeFibsRating(r, ms.nMatchTo));
                        else
                            aasz[1] = g_strdup_printf(_("n/a"));

                        list = g_list_append(list, aasz);

                    }

                }

            }

            if (psc->fCube || psc->fMoves) {

                /* error based fibs rating */

                if (nMatchTo) {

                    aasz = g_malloc(3 * sizeof(*aasz));
                    aasz[0] = g_strdup(_("Error based abs. FIBS rating"));

                    for (i = 0; i < 2; ++i)
                        if (psc->anCloseCube[i] + psc->anUnforcedMoves[i])
                            aasz[i + 1] = g_strdup_printf("%6.1f",
                                                          absoluteFibsRating(aaaar[CHEQUERPLAY][PERMOVE][i][NORMALISED],
                                                                             aaaar[CUBEDECISION][PERMOVE][i]
                                                                             [NORMALISED], nMatchTo, rRatingOffset));
                        else
                            aasz[i + 1] = g_strdup_printf(_("n/a"));

                    list = g_list_append(list, aasz);

                    /* chequer error fibs rating */

                    aasz = g_malloc(3 * sizeof(*aasz));
                    aasz[0] = g_strdup(_("Chequerplay errors rating loss"));

                    for (i = 0; i < 2; ++i)
                        if (psc->anUnforcedMoves[i])
                            aasz[i + 1] = g_strdup_printf("%6.1f",
                                                          absoluteFibsRatingChequer(aaaar[CHEQUERPLAY][PERMOVE][i]
                                                                                    [NORMALISED], nMatchTo));
                        else
                            aasz[i + 1] = g_strdup_printf(_("n/a"));

                    list = g_list_append(list, aasz);

                    /* cube error fibs rating */

                    aasz = g_malloc(3 * sizeof(*aasz));
                    aasz[0] = g_strdup(_("Cube errors rating loss"));

                    for (i = 0; i < 2; ++i)
                        if (psc->anCloseCube[i])
                            aasz[i + 1] = g_strdup_printf("%6.1f",
                                                          absoluteFibsRatingCube(aaaar[CUBEDECISION][PERMOVE][i]
                                                                                 [NORMALISED], nMatchTo));
                        else
                            aasz[i + 1] = g_strdup_printf(_("n/a"));

                    list = g_list_append(list, aasz);

                }

            }

            if (psc->fDice && !nMatchTo && psc->nGames > 1) {

                static const char *asz[2][2] = {
                    {N_("Advantage (actual) in ppg"),
                     /* xgettext: no-c-format */
                     N_("95% confidence interval (ppg)")},
                    {N_("Advantage (luck adjusted) in ppg"),
                     /* xgettext: no-c-format */
                     N_("95% confidence interval (ppg)")}
                };
                int i, j;
                const float *af[2][2];
                af[0][0] = psc->arActualResult;
                af[0][1] = psc->arVarianceActual;
                af[1][0] = psc->arLuckAdj;
                af[1][1] = psc->arVarianceLuckAdj;

                for (i = 0; i < 2; ++i) {

                    /* ppg */

                    aasz = g_malloc(3 * sizeof(*aasz));
                    aasz[0] = g_strdup(gettext(asz[i][0]));

                    for (j = 0; j < 2; ++j)
                        aasz[j + 1] =
                            g_strdup_printf("%+*.*f", fOutputDigits + 3, fOutputDigits, af[i][0][j] / psc->nGames);

                    list = g_list_append(list, aasz);

                    /* std dev. */

                    aasz = g_malloc(3 * sizeof(*aasz));
                    aasz[0] = g_strdup(gettext(asz[i][1]));

                    for (j = 0; j < 2; ++j) {
                        float ci = 1.95996f * sqrtf(af[i][1][j] / psc->nGames);
                        float max = af[i][0][j] + ci;
                        float min = af[i][0][j] - ci;
                        aasz[j + 1] = g_strdup_printf("[%*.*f,%*.*f]",
                                                      fOutputDigits + 3, fOutputDigits, min,
                                                      fOutputDigits + 3, fOutputDigits, max);
                    }
                    list = g_list_append(list, aasz);

                }


            }

        }

        break;

    default:

        g_assert_not_reached();
        break;

    }

    return list;


}

static void
_freeGS(gpointer data, gpointer UNUSED(userdata))
{

    char **aasz = data;
    int i;

    for (i = 0; i < 3; ++i)
        g_free(aasz[i]);

    g_free(aasz);

}


extern void
freeGS(GList * list)
{

    g_list_foreach(list, _freeGS, NULL);

    g_list_free(list);
}
