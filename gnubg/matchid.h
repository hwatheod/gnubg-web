/*
 * matchid.h
 *
 * by Jørn Thyssen <jthyssen@dk.ibm.com>, 2002
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
 * $Id: matchid.h,v 1.16 2013/11/20 23:04:14 plm Exp $
 */

#ifndef MATCHID_H
#define MATCHID_H

#define L_MATCHID 12

typedef struct {
    int anDice[2];
    int fTurn;
    int fResigned;
    int fDoubled;
    gamestate gs;
} posinfo;

extern int
 LogCube(int n);

extern char *MatchID(const unsigned int anDice[2],
                     const int fTurn,
                     const int fResigned,
                     const int fDoubled,
                     const int fMove,
                     const int fCubeOwner,
                     const int fCrawford, const int nMatchTo, const int anScore[2], const int nCube,
                     const int fJacoby, const gamestate gs);

extern char *MatchIDFromKey(unsigned char auchKey[9]);

extern int
MatchFromID(unsigned int anDice[2],
            int *pfTurn,
            int *pfResigned,
            int *pfDoubled, int *pfMove, int *pfCubeOwner, int *pfCrawford, int *pnMatchTo, int anScore[2], int *pnCube,
            int *pfJacoby, gamestate * pgs, const char *szMatchID);

extern int
MatchFromKey(int anDice[2],
             int *pfTurn,
             int *pfResigned,
             int *pfDoubled,
             int *pfMove, int *pfCubeOwner, int *pfCrawford, int *pnMatchTo, int anScore[2], int *pnCube,
             int *pfJacoby, gamestate * pgs, const unsigned char *auchKey);

extern char *MatchIDFromMatchState(const matchstate * pms);

#endif
