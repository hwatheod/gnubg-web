/*
 * rollout.h
 *
 * by Gary Wong <gary@cs.arizona.edu>, 1999.
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
 * $Id: rollout.h,v 1.39 2015/03/01 13:14:21 plm Exp $
 */

#ifndef ROLLOUT_H
#define ROLLOUT_H

#define MAXHIT 50               /* for statistics */
#define STAT_MAXCUBE 10

typedef struct _rolloutstat {

    /* Regular win statistics (dimension is cube turns) */

    int acWin[STAT_MAXCUBE];
    int acWinGammon[STAT_MAXCUBE];
    int acWinBackgammon[STAT_MAXCUBE];

    /* Cube statistics (dimension is cube turns) */

    int acDoubleDrop[STAT_MAXCUBE];     /* # of Double, drop */
    int acDoubleTake[STAT_MAXCUBE];     /* # of Double, takes */

    /* Chequer hit statistics (dimension is move number) */

    /* Opponent closed out */

    int nOpponentHit;
    int rOpponentHitMove;

    /* Average loss of pips in bear-off */

    int nBearoffMoves;          /* number of moves with bearoff */
    int nBearoffPipsLost;       /* number of pips lost in these moves */

    /* Opponent closed out */

    int nOpponentClosedOut;
    int rOpponentClosedOutMove;

    /* FIXME: add more stuff */

} rolloutstat;

typedef void
 (rolloutprogressfunc) (float arOutput[][NUM_ROLLOUT_OUTPUTS],
                        float arStdDev[][NUM_ROLLOUT_OUTPUTS],
                        const rolloutcontext * prc,
                        const cubeinfo aci[],
                        unsigned int initial_game_count,
                        const int iGame,
                        const int iAlternative,
                        const int nRank,
                        const float rJsd, const int fStopped, const int fShowRanks, int fCubeRollout, void *pUserData);

extern int


RolloutGeneral(ConstTanBoard * apBoard,
               float (*apOutput[])[NUM_ROLLOUT_OUTPUTS],
               float (*apStdDev[])[NUM_ROLLOUT_OUTPUTS],
               rolloutstat apStatistics[][2],
               evalsetup(*apes[]),
               const cubeinfo(*apci[]),
               int (*apCubeDecTop[]), int alternatives,
               int fInvert, int fCubeRollout, rolloutprogressfunc * pfRolloutProgress, void *pUserData);

extern int


GeneralEvaluation(float arOutput[NUM_ROLLOUT_OUTPUTS],
                  float arStdDev[NUM_ROLLOUT_OUTPUTS],
                  rolloutstat arsStatistics[2],
                  TanBoard anBoard,
                  cubeinfo * const pci, const evalsetup * pes,
                  rolloutprogressfunc * pfRolloutProgress, void *pUserData);

extern int


GeneralEvaluationR(float arOutput[NUM_ROLLOUT_OUTPUTS],
                   float arStdDev[NUM_ROLLOUT_OUTPUTS],
                   rolloutstat arsStatistics[2],
                   const TanBoard anBoard,
                   const cubeinfo * pci, const rolloutcontext * prc,
                   rolloutprogressfunc * pfRolloutProgress, void *pUserData);

extern int


GeneralCubeDecision(float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                    float aarStdDev[2][NUM_ROLLOUT_OUTPUTS],
                    rolloutstat aarsStatistics[2][2],
                    const TanBoard anBoard,
                    cubeinfo * pci, evalsetup * pes, rolloutprogressfunc * pfRolloutProgress, void *pUserData);


extern int


GeneralCubeDecisionR(float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
                     float aarStdDev[2][NUM_ROLLOUT_OUTPUTS],
                     rolloutstat aarsStatistics[2][2],
                     const TanBoard anBoard,
                     cubeinfo * pci, rolloutcontext * prc, evalsetup * pes,
                     rolloutprogressfunc * pfRolloutProgress, void *pUserData);

/* operations on rolloutstat */

/* Resignations */

extern int


getResignation(float arResign[NUM_ROLLOUT_OUTPUTS],
               TanBoard anBoard, cubeinfo * const pci, const evalsetup * pesResign);

extern void
 getResignEquities(float arResign[NUM_ROLLOUT_OUTPUTS], cubeinfo * pci, int nResigned, float *prBefore, float *prAfter);

extern int


ScoreMoveRollout(move ** ppm, cubeinfo ** ppci, int cMoves,
                 rolloutprogressfunc * pfRolloutProgress, void *pUserData);

extern void RolloutLoopMT(void *unused);

/* Quasi-random permutation array: the first index is the "generation" of the
 * permutation (0 permutes each set of 36 rolls, 1 permutes those sets of 36
 * into 1296, etc.); the second is the roll within the game (limited to 128,
 * so we use pseudo-random dice after that); the last is the permutation
 * itself.  6 generations are enough for 36^6 > 2^31 trials. */
typedef struct _perArray {
    unsigned char aaanPermutation[6][128][36];
    int nPermutationSeed;
} perArray;

EXP_LOCK_FUN(int, BasicCubefulRollout, unsigned int aanBoard[][2][25], float aarOutput[][NUM_ROLLOUT_OUTPUTS],
             int iTurn, int iGame, const cubeinfo aci[], int afCubeDecTop[], unsigned int cci, rolloutcontext * prc,
             rolloutstat aarsStatistics[][2], int nBasisCube, perArray * dicePerms, rngcontext * rngctxRollout,
             FILE * logfp);


extern FILE *log_game_start(const char *name, const cubeinfo * pci, int fCubeful, TanBoard anBoard);
extern void log_cube(FILE * logfp, const char *action, int side);
extern void log_move(FILE * logfp, const int *anMove, int side, int die0, int die1);
extern int RolloutDice(int iTurn, int iGame, int fInitial, unsigned int anDice[2], rng * rngx, void *rngctx,
                       const int fRotate, const perArray * dicePerms);
extern void ClosedBoard(int afClosedBoard[2], const TanBoard anBoard);
extern void log_game_over(FILE * logfp);
extern void QuasiRandomSeed(perArray * pArray, int n);
#endif
