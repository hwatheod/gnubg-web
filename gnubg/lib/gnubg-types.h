/*
 * gnubg-types.h
 *
 * by Christian Anthon
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
 * $Id: gnubg-types.h,v 1.9 2013/06/16 02:16:23 mdpetch Exp $
 */

#ifndef GNUBG_TYPES_H
#define GNUBG_TYPES_H

typedef unsigned int TanBoard[2][25];
typedef const unsigned int (*ConstTanBoard)[25];

typedef enum _bgvariation {
    VARIATION_STANDARD,         /* standard backgammon */
    VARIATION_NACKGAMMON,       /* standard backgammon with nackgammon starting
                                 * position */
    VARIATION_HYPERGAMMON_1,    /* 1-chequer hypergammon */
    VARIATION_HYPERGAMMON_2,    /* 2-chequer hypergammon */
    VARIATION_HYPERGAMMON_3,    /* 3-chequer hypergammon */
    NUM_VARIATIONS
} bgvariation;

typedef enum _gamestate {
    GAME_NONE, GAME_PLAYING, GAME_OVER, GAME_RESIGNED, GAME_DROP
} gamestate;

typedef struct _matchstate {
    TanBoard anBoard;
    unsigned int anDice[2];     /* (0,0) for unrolled dice */
    int fTurn;                  /* who makes the next decision */
    int fResigned;
    int fResignationDeclined;
    int fDoubled;
    int cGames;
    int fMove;                  /* player on roll */
    int fCubeOwner;
    int fCrawford;
    int fPostCrawford;
    int nMatchTo;
    int anScore[2];
    int nCube;
    unsigned int cBeavers;
    bgvariation bgv;
    int fCubeUse;
    int fJacoby;
    gamestate gs;
} matchstate;

typedef union _positionkey {
    unsigned int data[7];
} positionkey;

typedef union _oldpositionkey {
    unsigned char auch[10];
} oldpositionkey;

#endif
