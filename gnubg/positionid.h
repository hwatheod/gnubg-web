/*
 * positionid.h
 *
 * by Gary Wong, 1998, 1999, 2002
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
 * $Id: positionid.h,v 1.38 2014/10/07 07:14:29 plm Exp $
 */

#ifndef POSITIONID_H
#define POSITIONID_H

#include "gnubg-types.h"

#define L_POSITIONID 14

extern void PositionKey(const TanBoard anBoard, positionkey * pkey);
extern char *PositionID(const TanBoard anBoard);
extern char *PositionIDFromKey(const positionkey * pkey);

extern int PositionFromXG(TanBoard anBoard, const char * pos);

extern
unsigned int PositionBearoff(const unsigned int anBoard[], unsigned int nPoints, unsigned int nChequers);

extern void PositionFromKey(TanBoard anBoard, const positionkey * pkey);
extern void PositionFromKeySwapped(TanBoard anBoard, const positionkey * pkey);

/* Return 1 for success, 0 for invalid id */
extern int PositionFromID(TanBoard anBoard, const char *szID);

extern void PositionFromBearoff(unsigned int anBoard[], unsigned int usID,
                                unsigned int nPoints, unsigned int nChequers);

extern unsigned short PositionIndex(unsigned int g, const unsigned int anBoard[6]);

#define EqualKeys(k1, k2) (k1.data[0]==k2.data[0]&&k1.data[1]==k2.data[1]&&k1.data[2]==k2.data[2]&&k1.data[3]==k2.data[3]&&k1.data[4]==k2.data[4]&&k1.data[5]==k2.data[5]&&k1.data[6]==k2.data[6])

#define CopyKey(ks, kd) kd.data[0]=ks.data[0],kd.data[1]=ks.data[1],kd.data[2]=ks.data[2],kd.data[3]=ks.data[3],kd.data[4]=ks.data[4],kd.data[5]=ks.data[5],kd.data[6]=ks.data[6]

extern int EqualBoards(const TanBoard anBoard0, const TanBoard anBoard1);

/* Return 1 for valid position, 0 for not */
extern int CheckPosition(const TanBoard anBoard);

extern void ClosestLegalPosition(TanBoard anBoard);

extern unsigned int Combination(const unsigned int n, const unsigned int r);

extern unsigned char Base64(const unsigned char ch);

extern void oldPositionFromKey(TanBoard anBoard, const oldpositionkey * pkey);
extern void oldPositionKey(const TanBoard anBoard, oldpositionkey * pkey);

#endif
