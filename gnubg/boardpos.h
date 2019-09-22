/*
 * boardpos.h
 *
 * by Jørn Thyssen <jth@gnubg.org>, 2003
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
 * $Id: boardpos.h,v 1.9 2013/06/16 02:16:10 mdpetch Exp $
 */

#ifndef BOARDPOS_H
#define BOARDPOS_H

#define POINT_UNUSED0 28        /* the top unused bearoff tray */
#define POINT_UNUSED1 29        /* the bottom unused bearoff tray */
#define POINT_DICE 30
#define POINT_CUBE 31
#define POINT_RIGHT 32
#define POINT_LEFT 33
#define POINT_RESIGN 34

extern int positions[2][30][3];

extern void
 ChequerPosition(const int clockwise, const int point, const int chequer, int *px, int *py);

extern void
 PointArea(const int fClockwise, const int nSize, const int n, int *px, int *py, int *pcx, int *pcy);


extern void


CubePosition(const int crawford_game, const int cube_use,
             const int doubled, const int cube_owner, int fClockwise, int *px, int *py, int *porient);

extern void ArrowPosition(const int clockwise, int turn, const int nSize, int *px, int *py);


extern void
 ResignPosition(const int resigned, int *px, int *py, int *porient);


#endif
