/*
 *
 * mec - match equity calculator for backgammon. calculate equity table
 * given match length, gammon rate and winning probabilities.
 *
 * Copyright (C) 1996  Claes Thornberg (claest@it.kth.se)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Modified for usage with gnubg by Joern Thyssen <jth@gnubg.org>:
 *
 * (1) make external entry for usage in gnubg
 * (2) change "double" to "metentry", and typedef metentry to float
 *     or double depending on MEC_STANDALONE
 * (3) Compile with MEC_STANDALONE to get Thornberg's original program, e.g.,
 *
 *     gcc -DMEC_STANDALONE mec.c -o mec
 *
 * $Id: mec.c,v 1.9 2013/07/22 18:51:01 mdpetch Exp $
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef MEC_STANDALONE
#include "mec.h"
#endif

struct dp {
    double e;
    double w;
};

typedef struct dp dp;

void post_crawford(double, double, int, double **, double, double);

void crawford(double, double, int, double **);

void pre_crawford(double, double, int, double **);

dp dpt(int, int, int, double, double, double **);

/*
 * Arguments are (in this order):
 * match length
 * gammon rate
 * winning percentage (favorite)
 */

#ifdef MEC_STANDALONE

int
main(int argc, char **argv)
{
    /* match length */

    int ml = argc > 1 ? atoi(argv[1]) : 9;

    /* gammon rate, i.e. how many of games won/lost will be gammons */

    double gr = argc > 2 ? atof(argv[2]) : 0;

    /* Here one could argue for different approaches.  Does the underdog
     * in match (current score) have different gammon rate.  Or does the
     * underdog in match (equity at current score) have different gammon
     * rate.  This could be solved, but for now, we assume same rates. */

    /* winning percentage for favorite in one game.  NB: when one player
     * is favorite, there is no symmetry in the equity table! I.e.
     * E[i][1] = 1 - E[1][i] doesn't necessarily hold.  This figure must
     * be higher than or equal to .5, i.e. 50% */

    double wpf = argc > 3 ? atof(argv[3]) : 0.5;

    /* Equity chart, E[p][o] = equity for p when p is p away, and o is o away.
     * If there is a game favorite, i.e. wpf > 0.5, p above is considered
     * to be the favorite in each game.  For now we also assume no free drop
     * vigourish.  This will only affect the computation of post crawford
     * equities, and is quite easy to correct.  */

    double **E = (double **) malloc((ml + 1) * sizeof(double *));

    double *ec = (double *) calloc((ml + 1) * (ml + 1), sizeof(double));

    {
        int i;

        /* Initialize E to point to correct positions */

        for (i = 0; i <= ml; i++) {
            E[i] = &ec[i * (ml + 1)];
        }

        /* Initialize E for 0-away scores */

        for (i = 1; i <= ml; i++) {
            E[0][i] = 1;
            E[i][0] = 0;
        }
    }

    /* Compute post crawford equities, given gammon rate, winning percentage
     * for favorite (game), match length.  Fill in the equity table. */

    post_crawford(gr, wpf, ml, E, 0.0, 0.0);

    {
        int i;
        printf("Post-crawford:\n");
        for (i = 1; i < ml; ++i)
            printf("%6.3f", E[i][1]);
        printf("\n");
        for (i = 1; i < ml; ++i)
            printf("%6.3f", E[1][i]);
        printf("\n");
    }


    /* Compute crawford equities, given gammon rate, winning percentage
     * for favorite (game), match length, and post crawford equities.
     * Fill in the table. */

    crawford(gr, wpf, ml, E);

    /* Compute pre-crawford equities,  given gammon rate, winning percentage
     * for favorite (game), match length, and crawford equities.
     * Fill in the table. */

    pre_crawford(gr, wpf, ml, E);

    /* Print the equity table.  Nothing fancy. */

    {
        int i, j;

        printf("Gammon rate %5.4f\n" "Winning %%   %5.4f\n\n", gr, wpf);


        printf("%3s", "");

        for (i = 1; i <= ml; i++) {
            printf("%6d", i);
        }

        printf("\n");

        for (i = 1; i <= ml; i++) {
            printf("%3d", i);
            for (j = 1; j <= ml; j++)
                printf("%6.3f", E[i][j]);
            printf("\n");
        }
    }

    free(ec);
    free(E);
}

#else                           /* MEC_STANDALONE */

extern void
mec_pc(const float rGammonRate,
       const float rFreeDrop2Away, const float rFreeDrop4Away, const float rWinRate, float arMetPC[MAXSCORE])
{

    unsigned int i;

    /* match length */

    const unsigned int ml = 64;

    /* gammon rate, i.e. how many of games won/lost will be gammons */

    double gr = rGammonRate;

    /* Here one could argue for different approaches.  Does the underdog
     * in match (current score) have different gammon rate.  Or does the
     * underdog in match (equity at current score) have different gammon
     * rate.  This could be solved, but for now, we assume same rates. */

    /* winning percentage for favorite in one game.  NB: when one player
     * is favorite, there is no symmetry in the equity table! I.e.
     * E[i][1] = 1 - E[1][i] doesn't necessarily hold.  This figure must
     * be higher than or equal to .5, i.e. 50% */

    double wpf = rWinRate;

    /* Equity chart, E[p][o] = equity for p when p is p away, and o is o away.
     * If there is a game favorite, i.e. wpf > 0.5, p above is considered
     * to be the favorite in each game.  For now we also assume no free drop
     * vigourish.  This will only affect the computation of post crawford
     * equities, and is quite easy to correct.  */

    double **E = (double **) malloc((ml + 1) * sizeof(double *));

    double *ec = (double *) calloc((ml + 1) * (ml + 1), sizeof(double));

    if (!E || !ec)
        exit(-1);               /* We're in trouble... */

    {

        /* Initialize E to point to correct positions */

        for (i = 0; i <= ml; i++) {
            E[i] = &ec[i * (ml + 1)];
        }

        /* Initialize E for 0-away scores */

        for (i = 1; i <= ml; i++) {
            E[0][i] = 1;
            E[i][0] = 0;
        }
    }

    /* Compute post crawford equities, given gammon rate, winning percentage
     * for favorite (game), match length.  Fill in the equity table. */

    post_crawford(gr, wpf, ml, E, (double) rFreeDrop2Away, (double) rFreeDrop4Away);

    /* save post Crawford equities */

    for (i = 0; i < ml; ++i)
        arMetPC[i] = (float) E[i + 1][1];

    /* garbage collect */

    free(ec);
    free(E);
}


extern void
mec(const float rGammonRate, const float rWinRate,
    /* const *//*lint -e{818} */ float aarMetPC[2][MAXSCORE],
    float aarMet[MAXSCORE][MAXSCORE])
{

    unsigned int i, j;

    /* match length */

    const unsigned int ml = 64;

    /* gammon rate, i.e. how many of games won/lost will be gammons */

    double gr = rGammonRate;

    /* Here one could argue for different approaches.  Does the underdog
     * in match (current score) have different gammon rate.  Or does the
     * underdog in match (equity at current score) have different gammon
     * rate.  This could be solved, but for now, we assume same rates. */

    /* winning percentage for favorite in one game.  NB: when one player
     * is favorite, there is no symmetry in the equity table! I.e.
     * E[i][1] = 1 - E[1][i] doesn't necessarily hold.  This figure must
     * be higher than or equal to .5, i.e. 50% */

    double wpf = rWinRate;

    /* Equity chart, E[p][o] = equity for p when p is p away, and o is o away.
     * If there is a game favorite, i.e. wpf > 0.5, p above is considered
     * to be the favorite in each game.  For now we also assume no free drop
     * vigourish.  This will only affect the computation of post crawford
     * equities, and is quite easy to correct.  */

    double **E = (double **) malloc((ml + 1) * sizeof(double *));

    double *ec = (double *) calloc((ml + 1) * (ml + 1), sizeof(double));

    if (!E || !ec)
        exit(-1);               /* We're in trouble... */

    {

        /* Initialize E to point to correct positions */

        for (i = 0; i <= ml; i++) {
            E[i] = &ec[i * (ml + 1)];
        }

        /* Initialize E for 0-away scores */

        for (i = 1; i <= ml; i++) {
            E[0][i] = 1;
            E[i][0] = 0;
        }
    }

    /* Compute post crawford equities, given gammon rate, winning percentage
     * for favorite (game), match length.  Fill in the equity table. */

    for (i = 0; i < ml; ++i)
        E[i + 1][1] = aarMetPC[0][i];

    for (i = 0; i < ml; ++i)
        E[1][i + 1] = 1.0 - aarMetPC[1][i];

    /* Compute crawford equities, given gammon rate, winning percentage
     * for favorite (game), match length, and post crawford equities.
     * Fill in the table. */

    crawford(gr, wpf, ml, E);

    /* Compute pre-crawford equities,  given gammon rate, winning percentage
     * for favorite (game), match length, and crawford equities.
     * Fill in the table. */

    pre_crawford(gr, wpf, ml, E);

    /* save the match equiy table */

    for (i = 0; i < ml; ++i)
        for (j = 0; j < ml; ++j)
            aarMet[i][j] = (float) E[i + 1][j + 1];

    /* garbage collect */

    free(ec);
    free(E);
}

#endif                          /* ! MEC_STANDALONE */

/* If last bit is zero, then x is even. */
#define even(x)	(((x)&0x1)==0)

/* sq returns x if x is greater than zero, else it returns zero. */
#define sq(x) ((x)>0?(x):0)

void
post_crawford(double gr, double wpf, int ml, /*lint -e{818} */ double **E,
              double fd2, double fd4)
{
    int i;

    E[1][1] = wpf;

    for (i = 2; i <= ml; i++) {
        if (even(i)) {
            /* Free drop condition exists */
            E[1][i] = E[1][i - 1];
            E[i][1] = E[i - 1][1];
            /* jth: add empirical values for free drop */
            if (i == 2) {
                /* 2-away */
                E[1][i] += fd2;
                E[i][1] -= fd2;
            } else if (i == 4) {
                /* 4-away */
                E[1][i] += fd4;
                E[i][1] += fd4;
            }
        } else {
            E[1][i] =           /* Equity for favorite when 1-away, i-away */
                E[0][i] * wpf   /* Favorite wins */
                + E[1][sq(i - 2)] * (1 - wpf) * (1 - gr)        /* Favorite loses single */
                +E[1][sq(i - 4)] * (1 - wpf) * gr;      /* Favorite loses gammon */

            E[i][1] =           /* Equity for favorite when i-away, 1-away */
                E[i][0] * (1 - wpf)     /* Favorite loses */
                +E[sq(i - 2)][1] * wpf * (1 - gr)       /* Favorite wins single */
                +E[sq(i - 4)][1] * wpf * gr;    /* Favorite wins gammon */
        }
    }
}

void
crawford(double gr, double wpf, int ml, /*lint -e{818} */ double **E)
{
    int i;

    /* Compute crawford equities.  Do this backwards, since
     * we overwrite post crawford equities with crawford equities.
     * In this way we only overwrite equities no longer needed. */

    for (i = ml; i >= 2; i--) {
        E[1][i] =               /* Equity for favorite when 1-away,i-away */
            E[0][i] * wpf       /* Favorite wins */
            + E[1][i - 1] * (1 - wpf) * (1 - gr)        /* Favorite loses single */
            +E[1][i - 2] * (1 - wpf) * gr;      /* Favorite loses gammon */

        E[i][1] =               /* Equity for favorite when i-away, 1-away */
            E[i][0] * (1 - wpf) /* Favorite loses */
            +E[i - 1][1] * wpf * (1 - gr)       /* Favorite wins single */
            +E[i - 2][1] * wpf * gr;    /* Favorite wins gammon */
    }
}

void
pre_crawford(double gr, double wpf, int ml, double **E)
{
    int i, j;
    dp dpf, dpu;
    double eq;

    for (i = 2; i <= ml; i++)
        for (j = i; j <= ml; j++) {
            dpf = dpt(i, j, 2, gr, wpf, E);
            dpu = dpt(j, i, 2, gr, 1 - wpf, E);

            dpu.e = 1 - dpu.e;
            dpu.w = 1 - dpu.w;

            eq = dpu.e + (dpf.e - dpu.e) * (wpf - dpu.w) / (dpf.w - dpu.w);

            E[i][j] = eq;

            if (i != j) {
                dpf = dpt(j, i, 2, gr, wpf, E);
                dpu = dpt(i, j, 2, gr, 1 - wpf, E);

                dpu.e = 1 - dpu.e;
                dpu.w = 1 - dpu.w;

                eq = dpu.e + (dpf.e - dpu.e) * (wpf - dpu.w) / (dpf.w - dpu.w);

                E[j][i] = eq;
            }
        }
}

/* Compute point when p doubles o to c, assuming gammon rate gr,
 * p wins wpp % of games, and equities E for favorite.  At this
 * point o does equally well passing the double as taking it. */

dp
dpt(int p, int o, int c, double gr, double wpp, double **E)
{
    dp dpo, dpp;
    double e0, edp, wdp;

    if (p <= c / 2) {
        /* No reason for p to double o, since a single win
         * is enough to win the match. */

        dpp.e = 1;
        dpp.w = 1;
        return dpp;
    }

    /* p might double o to c since he needs more than a single
     * game to win the match. */

    /* Find out when o (re)doubles p to 2*c */

    dpo = dpt(o, p, 2 * c, gr, 1 - wpp, E);

    /* Find out equity for o if p does well and wins
     * all games here (assuming no recube from o), i.e.
     * o loses all games sitting on a c-cube. */

    if (wpp > 0.5) {
        /* o isn't game favorite, equity for o at o-away x-away is 1 - E[x][o]. */

        e0 = (1 - E[sq(p - c)][o]) * (1 - gr)
            + (1 - E[sq(p - 2 * c)][o]) * gr;
    } else {
        /* o is game favorite, equity for o at o-away x-away is E[o][x]. */

        e0 = E[o][sq(p - c)] * (1 - gr)
            + E[o][sq(p - 2 * c)] * gr;
    }

    /* Find out o:s equity if o passes the double to c, i.e. loses c / 2 points. */

    if (wpp > 0.5) {
        /* o isn't game favorite, equity for o at o-away x-away is 1 - E[x][o]. */

        edp = 1 - E[sq(p - c / 2)][o];
    } else {
        /* o is game favorite, equity for o at o-away x-away is E[o][x]. */

        edp = E[o][sq(p - c / 2)];
    }

    /* Find the winning percentage, which on the line from
     * (w0?,e0) to (dpo.w,dpo.e) gives o an equity equal to O's
     * equity  passing the double to c (i.e. losing c / 2 pts) */

    wdp = (edp - e0) * dpo.w / (dpo.e - e0);

    /* Now we know when p should double o, expressed as
     * winning percentage and equity for o.  Return this
     * expressed in figures for p */

    dpp.e = 1 - edp;
    dpp.w = 1 - wdp;
    return dpp;
}
