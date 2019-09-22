/*
 * eval.h
 *
 * by Gary Wong <gary@cs.arizona.edu>, 1998-2001.
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
 * $Id: eval.h,v 1.192 2015/02/09 21:55:33 plm Exp $
 */

#ifndef EVAL_H
#define EVAL_H

#include "dice.h"
#include "bearoff.h"
#include "neuralnet.h"
#include "cache.h"

#define EXP_LOCK_FUN(ret, name, ...) \
	typedef ret (*f_##name)( __VA_ARGS__); \
	extern f_##name name; \
	extern ret name##NoLocking( __VA_ARGS__); \
	extern ret name##WithLocking( __VA_ARGS__)

#define WEIGHTS_VERSION "1.00"
#define WEIGHTS_VERSION_BINARY 1.00f
#define WEIGHTS_MAGIC_BINARY 472.3782f

#define NUM_OUTPUTS 5
#define NUM_CUBEFUL_OUTPUTS 4
#define NUM_ROLLOUT_OUTPUTS 7

#define BETA_HIDDEN 0.1f
#define BETA_OUTPUT 1.0f

#define OUTPUT_WIN 0
#define OUTPUT_WINGAMMON 1
#define OUTPUT_WINBACKGAMMON 2
#define OUTPUT_LOSEGAMMON 3
#define OUTPUT_LOSEBACKGAMMON 4

#define OUTPUT_EQUITY 5         /* NB: neural nets do not output equity, only
                                 * rollouts do. */
#define OUTPUT_CUBEFUL_EQUITY 6

/* Cubeful evalutions */
typedef enum {
    OUTPUT_OPTIMAL = 0,
    OUTPUT_NODOUBLE,
    OUTPUT_TAKE,
    OUTPUT_DROP
} CubefulOutputs;

/* A trivial upper bound on the number of (complete or incomplete)
 * legal moves of a single roll: if all 15 chequers are spread out,
 * then there are 18 C 4 + 17 C 3 + 16 C 2 + 15 C 1 = 3875
 * combinations in which a roll of 11 could be played (up to 4 choices from
 * 15 chequers, and a chequer may be chosen more than once).  The true
 * bound will be lower than this (because there are only 26 points,
 * some plays of 15 chequers must "overlap" and map to the same
 * resulting position), but that would be more difficult to
 * compute. */
#define MAX_INCOMPLETE_MOVES 3875
#define MAX_MOVES 3060

typedef struct movefilter_s {
    int Accept;                 /* always allow this many moves. 0 means don't use this */
    /* level, since at least 1 is needed when used. */
    int Extra;                  /* and add up to this many more... */
    float Threshold;            /* ...if they are within this equity difference */
} movefilter;

/* we'll have filters for 1..4 ply evaluation */
#define MAX_FILTER_PLIES	4
extern movefilter defaultFilters[MAX_FILTER_PLIES][MAX_FILTER_PLIES];

typedef struct {
    /* FIXME expand this... e.g. different settings for different position
     * classes */
    unsigned int fCubeful:1;    /* cubeful evaluation */
    unsigned int nPlies:4;
    unsigned int fUsePrune:1;
    unsigned int fDeterministic:1;
    float rNoise;               /* standard deviation */
} evalcontext;

/* identifies the format of evaluation info in .sgf files
 * early (pre extending rollouts) had no version numbers
 * extendable rollouts have a version number of 1
 * with pruning nets, we have two possibilities - reduction could still
 * be enabled (version = 2) or, given the speedup and performance 
 * improvements, I assume we will drop reduction entirely. (ver = 3 or more)
 * When presented with an .sgf file, gnubg will attempt to work out what
 * data is present in the file based on the version number
 */

#define SGF_FORMAT_VER 3

typedef struct {
    evalcontext aecCube[2], aecChequer[2];      /* evaluation parameters */
    evalcontext aecCubeLate[2], aecChequerLate[2];      /* ... for later moves */
    evalcontext aecCubeTrunc, aecChequerTrunc;  /* ... at truncation point */
    movefilter aaamfChequer[2][MAX_FILTER_PLIES][MAX_FILTER_PLIES];
    movefilter aaamfLate[2][MAX_FILTER_PLIES][MAX_FILTER_PLIES];

    unsigned int fCubeful:1;    /* Cubeful rollout */
    unsigned int fVarRedn:1;    /* variance reduction */
    unsigned int fInitial:1;    /* roll out as opening position */
    unsigned int fRotate:1;     /* quasi-random dice */
    unsigned int fTruncBearoff2:1;      /* cubeless rollout: trunc at BEAROFF2 */
    unsigned int fTruncBearoffOS:1;     /* cubeless rollout: trunc at BEAROFF_OS */
    unsigned int fLateEvals:1;  /* enable different evals for later moves */
    unsigned int fDoTruncate:1; /* enable truncated rollouts */
    unsigned int fStopOnSTD:1;  /* stop when std's are small enough */
    unsigned int fStopOnJsd:1;
    unsigned int fStopMoveOnJsd:1;      /* stop multi-line rollout when jsd
                                         * is small enough */
    unsigned short nTruncate;   /* truncation */
    unsigned int nTrials;       /* number of rollouts */
    unsigned short nLate;       /* switch evaluations on move nLate of game */
    rng rngRollout;
    unsigned long nSeed;
    unsigned int nMinimumGames; /* but always do at least this many */
    float rStdLimit;           /* stop when abs( value / std ) < this */
    unsigned int nMinimumJsdGames;
    float rJsdLimit;
    unsigned int nGamesDone;
    float rStoppedOnJSD;
    int nSkip;
} rolloutcontext;

typedef struct {
    float rEquity;
    float rJSD;
    int nOrder;
    int nRank;
} jsdinfo;

typedef enum {
    EVAL_NONE, EVAL_EVAL, EVAL_ROLLOUT
} evaltype;

/* enumeration of variations of backgammon
 * (starting position and/or special rules) */

extern bgvariation bgvDefault;

extern int anChequers[NUM_VARIATIONS];
extern const char *aszVariations[NUM_VARIATIONS];
extern const char *aszVariationCommands[NUM_VARIATIONS];

/*
 * Cubeinfo contains the information necesary for evaluation
 * of a position.
 * These structs are placed here so that the move struct can be defined
 */

typedef struct {
    /*
     * nCube: the current value of the cube,
     * fCubeOwner: the owner of the cube,
     * fMove: the player for which we are
     *        calculating equity for,
     * fCrawford, fJacoby, fBeavers: optional rules in effect,
     * arGammonPrice: the gammon prices;
     *   [ 0 ] = gammon price for player 0,
     *   [ 1 ] = gammon price for player 1,
     *   [ 2 ] = backgammon price for player 0,
     *   [ 3 ] = backgammon price for player 1.
     *
     */
    int nCube, fCubeOwner, fMove, nMatchTo, anScore[2], fCrawford, fJacoby, fBeavers;
    float arGammonPrice[4];
    bgvariation bgv;
} cubeinfo;

typedef struct {
    evaltype et;
    evalcontext ec;
    rolloutcontext rc;
} evalsetup;

typedef enum {
    DOUBLE_TAKE,
    DOUBLE_PASS,
    NODOUBLE_TAKE,
    TOOGOOD_TAKE,
    TOOGOOD_PASS,
    DOUBLE_BEAVER,
    NODOUBLE_BEAVER,
    REDOUBLE_TAKE,
    REDOUBLE_PASS,
    NO_REDOUBLE_TAKE,
    TOOGOODRE_TAKE,
    TOOGOODRE_PASS,
    NO_REDOUBLE_BEAVER,
    NODOUBLE_DEADCUBE,          /* cube is dead (match play only) */
    NO_REDOUBLE_DEADCUBE,       /* cube is dead (match play only) */
    NOT_AVAILABLE,              /* Cube not available */
    OPTIONAL_DOUBLE_TAKE,
    OPTIONAL_REDOUBLE_TAKE,
    OPTIONAL_DOUBLE_BEAVER,
    OPTIONAL_DOUBLE_PASS,
    OPTIONAL_REDOUBLE_PASS
} cubedecision;

typedef enum {
    DT_NORMAL,
    DT_BEAVER,
    DT_RACCOON,
    NUM_DOUBLE_TYPES
} doubletype;

/*
 * TT_NA can happen if a single position was loaded from a sgf file.
 * To check for beavers, use "> TT_NORMAL", not "!= TT_NORMAL".
 */
typedef enum {
    TT_NA,
    TT_NORMAL,
    TT_BEAVER
} taketype;

extern const char *aszDoubleTypes[NUM_DOUBLE_TYPES];

/*
 * prefined settings
 */

#define NUM_SETTINGS            9
#define SETTINGS_4PLY           8
#define SETTINGS_GRANDMASTER    7
#define SETTINGS_SUPREMO        6
#define SETTINGS_WORLDCLASS     5
#define SETTINGS_EXPERT         4
#define SETTINGS_ADVANCED       3
#define SETTINGS_INTERMEDIATE   2
#define SETTINGS_NOVICE         1
#define SETTINGS_BEGINNER       0

extern evalcontext aecSettings[NUM_SETTINGS];
extern evalcontext ecBasic;
extern int aiSettingsMoveFilter[NUM_SETTINGS];
extern const char *aszSettings[NUM_SETTINGS];

#define NUM_MOVEFILTER_SETTINGS 5

extern const char *aszMoveFilterSettings[NUM_MOVEFILTER_SETTINGS];
extern movefilter aaamfMoveFilterSettings[NUM_MOVEFILTER_SETTINGS][MAX_FILTER_PLIES][MAX_FILTER_PLIES];

typedef enum {
    CMARK_NONE,
    CMARK_ROLLOUT
} CMark;

typedef struct {
    int anMove[8];
    positionkey key;
    unsigned int cMoves, cPips;
    /* scores for this move */
    float rScore, rScore2;
    /* evaluation for this move */
    float arEvalMove[NUM_ROLLOUT_OUTPUTS];
    float arEvalStdDev[NUM_ROLLOUT_OUTPUTS];
    evalsetup esMove;
    CMark cmark;
} move;

extern int fInterrupt;
extern cubeinfo ciCubeless;
extern const char *aszEvalType[(int) EVAL_ROLLOUT + 1];

extern bearoffcontext *pbc1;
extern bearoffcontext *pbc2;
extern bearoffcontext *pbcOS;
extern bearoffcontext *pbcTS;
extern bearoffcontext *apbcHyper[3];

typedef struct {
    unsigned int cMoves;        /* and current move when building list */
    unsigned int cMaxMoves, cMaxPips;
    int iMoveBest;
    float rBestScore;
    move *amMoves;
} movelist;

/* cube efficiencies */

extern float rOSCubeX;
extern float rRaceFactorX;
extern float rRaceCoefficientX;
extern float rRaceMax;
extern float rRaceMin;
extern float rCrashedX;
extern float rContactX;

/* position classes */

typedef enum {
    CLASS_OVER = 0,             /* Game already finished */
    CLASS_HYPERGAMMON1,         /* hypergammon with 1 chequers */
    CLASS_HYPERGAMMON2,         /* hypergammon with 2 chequers */
    CLASS_HYPERGAMMON3,         /* hypergammon with 3 chequers */
    CLASS_BEAROFF2,             /* Two-sided bearoff database (in memory) */
    CLASS_BEAROFF_TS,           /* Two-sided bearoff database (on disk) */
    CLASS_BEAROFF1,             /* One-sided bearoff database (in memory) */
    CLASS_BEAROFF_OS,           /* One-sided bearoff database /on disk) */
    CLASS_RACE,                 /* Race neural network */
    CLASS_CRASHED,              /* Contact, one side has less than 7 active checkers */
    CLASS_CONTACT               /* Contact neural network */
} positionclass;

#define N_CLASSES (CLASS_CONTACT + 1)

#define CLASS_PERFECT CLASS_BEAROFF_TS
#define CLASS_GOOD CLASS_BEAROFF_OS   /* Good enough to not need SanityCheck */

typedef int (*classevalfunc) (const TanBoard anBoard, float arOutput[], const bgvariation bgv, NNState * nnStates);

extern classevalfunc acef[N_CLASSES];

/* Evaluation cache size is 2^SIZE entries */
#define CACHE_SIZE_DEFAULT 19
#define CACHE_SIZE_GUIMAX 23

#define CFMONEY(arEquity,pci) \
   ( ( (pci)->fCubeOwner == -1 ) ? arEquity[ 2 ] : \
   ( ( (pci)->fCubeOwner == (pci)->fMove ) ? arEquity[ 1 ] : arEquity[ 3 ] ) )

#define CFHYPER(arEquity,pci) \
   ( ( (pci)->fCubeOwner == -1 ) ? \
     ( ( (pci)->fJacoby ) ? arEquity[ 2 ] : arEquity[ 1 ] ) : \
     ( ( (pci)->fCubeOwner == (pci)->fMove ) ? arEquity[ 0 ] : arEquity[ 3 ] ) )

extern void EvalInitialise(char *szWeights, char *szWeightsBinary, int fNoBearoff, void (*pfProgress) (unsigned int));

extern int EvalShutdown(void);

extern void EvalStatus(char *szOutput);

extern int EvalNewWeights(int nSize);

extern int EvalSave(const char *szWeights);


EXP_LOCK_FUN(int, EvaluatePosition, NNState * nnStates, const TanBoard anBoard, float arOutput[],
             cubeinfo * const pci, const evalcontext * pec);

extern void
 InvertEvaluationR(float ar[NUM_ROLLOUT_OUTPUTS], const cubeinfo * pci);

extern void
 InvertEvaluation(float ar[NUM_OUTPUTS]);

EXP_LOCK_FUN(int, FindBestMove, int anMove[8], int nDice0, int nDice1,
             TanBoard anBoard, const cubeinfo * pci, evalcontext * pec, movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]);

EXP_LOCK_FUN(int, FindnSaveBestMoves, movelist * pml,
             int nDice0, int nDice1, const TanBoard anBoard,
             positionkey * keyMove, const float rThr,
             const cubeinfo * pci, const evalcontext * pec, movefilter aamf[MAX_FILTER_PLIES][MAX_FILTER_PLIES]);

extern void
 PipCount(const TanBoard anBoard, unsigned int anPips[2]);

extern int
 ThorpCount(const TanBoard anBoard, int *pnLeader, float *adjusted, int *pnTrailer);

extern int
 KeithCount(const TanBoard anBoard, int pn[2]);

extern int
 DumpPosition(const TanBoard anBoard, char *szOutput,
             const evalcontext * pec, cubeinfo * pci, int fOutputMWC,
             int fOutputWinPC, int fOutputInvert, const char *szMatchID);

extern void
 SwapSides(TanBoard anBoard);

extern int
 GameStatus(const TanBoard anBoard, const bgvariation bgv);

extern void EvalCacheFlush(void);
extern int EvalCacheResize(unsigned int cNew);
extern int EvalCacheStats(unsigned int *pcUsed, unsigned int *pcLookup, unsigned int *pcHit);
extern double GetEvalCacheSize(void);
void SetEvalCacheSize(unsigned int size);
extern unsigned int GetEvalCacheEntries(void);
extern int GetCacheMB(int size);

extern evalCache cEval;
extern evalCache cpEval;
extern unsigned int cCache;

extern int
 GenerateMoves(movelist * pml, const TanBoard anBoard, int n0, int n1, int fPartial);

extern int ApplySubMove(TanBoard anBoard, const int iSrc, const int nRoll, const int fCheckLegal);

extern int ApplyMove(TanBoard anBoard, const int anMove[8], const int fCheckLegal);

extern positionclass ClassifyPosition(const TanBoard anBoard, const bgvariation bgv);

/* internal use only */
extern void EvalRaceBG(const TanBoard anBoard, float arOutput[], const bgvariation bgv);

extern float
 Utility(float ar[NUM_OUTPUTS], const cubeinfo * pci);

extern float
 UtilityME(float ar[NUM_OUTPUTS], const cubeinfo * pci);

extern int
 SetCubeInfoMoney(cubeinfo * pci, const int nCube, const int fCubeOwner,
                 const int fMove, const int fJacoby, const int fBeavers, const bgvariation bgv);

extern int
 SetCubeInfoMatch(cubeinfo * pci, const int nCube, const int fCubeOwner,
                 const int fMove, const int nMatchTo, const int anScore[2], const int fCrawford, const bgvariation bgv);

extern int
 SetCubeInfo(cubeinfo * pci, const int nCube, const int fCubeOwner,
            const int fMove, const int nMatchTo, const int anScore[2],
            const int fCrawford, const int fJacoby, const int fBeavers, const bgvariation bgv);

extern void
 swap_us(unsigned int *p0, unsigned int *p1);

extern void
 swap(int *p0, int *p1);

extern void
 SanityCheck(const TanBoard anBoard, float arOutput[]);

extern int
 EvalBearoff1(const TanBoard anBoard, float arOutput[], const bgvariation bgv, NNState * nnStates);

extern int
 EvalOver(const TanBoard anBoard, float arOutput[], const bgvariation bgv, NNState * nnStates);

extern float
 KleinmanCount(int nPipOnRoll, int nPipNotOnRoll);

extern int
 GetDPEq(int *pfCube, float *prDPEq, const cubeinfo * pci);

extern float
 mwc2eq(const float rMwc, const cubeinfo * ci);

extern float
 eq2mwc(const float rEq, const cubeinfo * ci);

extern float
 se_mwc2eq(const float rMwc, const cubeinfo * ci);

extern float
 se_eq2mwc(const float rEq, const cubeinfo * ci);

extern char
*FormatEval(char *sz, evalsetup * pes);

extern cubedecision FindCubeDecision(float arDouble[], float aarOutput[][NUM_ROLLOUT_OUTPUTS], const cubeinfo * pci);

EXP_LOCK_FUN(int, GeneralCubeDecisionE, float aarOutput[2][NUM_ROLLOUT_OUTPUTS],
             const TanBoard anBoard, cubeinfo * const pci, const evalcontext * pec, const evalsetup * pes);

EXP_LOCK_FUN(int, GeneralEvaluationE, float arOutput[NUM_ROLLOUT_OUTPUTS],
             const TanBoard anBoard, cubeinfo * const pci, const evalcontext * pec);

extern int
 cmp_evalsetup(const evalsetup * pes1, const evalsetup * pes2);

extern int
 cmp_evalcontext(const evalcontext * pec1, const evalcontext * pec2);

extern int
 cmp_rolloutcontext(const rolloutcontext * prc1, const rolloutcontext * prc2);

extern char
*GetCubeRecommendation(const cubedecision cd);

extern cubedecision
FindBestCubeDecision(float arDouble[], float aarOutput[2][NUM_ROLLOUT_OUTPUTS], const cubeinfo * pci);

extern int
 getCurrentGammonRates(float aarRates[2][2],
                      float arOutput[], const TanBoard anBoard, cubeinfo * pci, const evalcontext * pec);

extern void
 calculate_gammon_rates(float aarRates[2][2], float arOutput[], cubeinfo * pci);

extern void
 getMoneyPoints(float aaarPoints[2][7][2], const int fJacoby, const int fBeavers, float aarRates[2][2]);

extern void
 getMatchPoints(float aaarPoints[2][4][2],
               int afAutoRedouble[2], int afDead[2], const cubeinfo * pci, float aarRates[2][2]);

extern void
 getCubeDecisionOrdering(int aiOrder[3],
                        float arDouble[4], float aarOutput[2][NUM_ROLLOUT_OUTPUTS], const cubeinfo * pci);

extern float
 getPercent(const cubedecision cd, const float arDouble[]);

extern void
 RefreshMoveList(movelist * pml, int *ai);

EXP_LOCK_FUN(int, ScoreMove, NNState * nnStates, move * pm, const cubeinfo * pci, const evalcontext * pec, int nPlies);

extern void
 CopyMoveList(movelist * pmlDest, const movelist * pmlSrc);

extern int
 isCloseCubedecision(const float arDouble[]);

extern int
 isMissedDouble(float arDouble[], float aarOutput[2][NUM_ROLLOUT_OUTPUTS], const int fDouble, const cubeinfo * pci);

extern unsigned int
 locateMove(const TanBoard anBoard, const int anMove[8], const movelist * pml);

extern int
 MoveKey(const TanBoard anBoard, const int anMove[8], positionkey * pkey);

extern int
 equal_movefilter(const int i, const movefilter amf1[MAX_FILTER_PLIES], const movefilter amf2[MAX_FILTER_PLIES]);

extern int
 equal_movefilters(movefilter aamf1[MAX_FILTER_PLIES][MAX_FILTER_PLIES],
                   movefilter aamf2[MAX_FILTER_PLIES][MAX_FILTER_PLIES]);

extern doubletype DoubleType(const int fDoubled, const int fMove, const int fTurn);

extern int
 PerfectCubeful(bearoffcontext * pbc, const TanBoard anBoard, float arEquity[]);

extern void
 baseInputs(const TanBoard anBoard, float arInput[]);

extern void
 CalculateRaceInputs(const TanBoard anBoard, float inputs[]);

extern int CompareMoves(const move * pm0, const move * pm1);
extern float EvalEfficiency(const TanBoard anBoard, positionclass pc);
extern float Cl2CfMoney(float arOutput[NUM_OUTPUTS], cubeinfo * pci, float rCubeX);
extern float Cl2CfMatch(float arOutput[NUM_OUTPUTS], cubeinfo * pci, float rCubeX);
extern float Noise(const evalcontext * pec, const TanBoard anBoard, int iOutput);
extern int EvalKey(const evalcontext * pec, const int nPlies, const cubeinfo * pci, int fCubefulEquity);
extern void MakeCubePos(const cubeinfo aciCubePos[], const int cci, const int fTop, cubeinfo aci[], const int fInvert);
extern void GetECF3(float arCubeful[], int cci, float arCf[], cubeinfo aci[]);
extern int EvaluatePerfectCubeful(const TanBoard anBoard, float arEquity[], const bgvariation bgv);

extern neuralnet nnContact, nnRace, nnCrashed;
extern neuralnet nnpContact, nnpRace, nnpCrashed;

#endif
