/*
 * sgf.h
 *
 * by Gary Wong <gtw@gnu.org>, 2000
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
 * $Id: sgf.h,v 1.10 2013/06/16 02:16:20 mdpetch Exp $
 */

#ifndef SGF_H
#define SGF_H

#include "list.h"
#include <stdio.h>

typedef struct _property {
    char ach[2];                /* 2 character tag; ach[ 1 ] = 0 for 1 character tags */
    listOLD *pl;                /* Values */
} property;

extern void (*SGFErrorHandler) (const char *szMessage, int fParseError);

/* Parse an SGF file, and return a syntax tree.  The tree is saved as a list
 * of game trees; each game tree is a list where the first element is the
 * initial sequence of SGF nodes and any other elements are alternate
 * variations (each variation is itself a game tree).  Sequences of SGF
 * nodes are also stored as lists; each element is a single SGF node.
 * Nodes consist of yet MORE lists; each element is a "property" struct
 * as defined above.
 * 
 * If there are any errors in the file, SGFParse calls SGFErrorHandler
 * (if set), or complains to stderr (otherwise). */
extern listOLD *SGFParse(FILE * pf);

/* The following properties are defined for GNU Backgammon SGF files:
 * 
 * A  (M)  - analysis (gnubg private)
 * AB (S)  - add black (general SGF)
 * AE (S)  - add empty (general SGF)
 * AN (GI) - annotation (general SGF) (not currently used)
 * AP (R)  - application (general SGF)
 * AR      - arrow (general SGF) (not currently used)
 * AW (S)  - add white (general SGF)
 * B (M)   - black move (general SGF)
 * BL (M)  - black time left (general SGF) (not currently used)
 * BM (M)  - bad move (general SGF)
 * BR (GI) - black rank (general SGF) (not currently used)
 * BT (GI) - black team (general SGF) (not currently used)
 * C       - comment (general SGF)
 * CA (R)  - character set (general SGF) (not currently used)
 * CP      - Whoops!!  We use this for "cube position", but it's actually
 * reserved for "copyright"...
 * CR      - circle (general SGF) (not currently used)
 * CV (S)  - cube value (backgammon)
 * DA (M)  - double analysis (gnubg private)
 * DD      - dim points (general SGF) (not currently used)
 * DI (S)  - dice (should be a standard backgammon property)
 * DM      - even position (general SGF) (not currently used)
 * DO (M)  - doubtful (general SGF)
 * DT (GI) - date (general SGF) (not currently used)
 * EV (GI) - event (general SGF) (not currently used)
 * FF (R)  - file format (general SGF)
 * FG      - figure (general SGF) (not currently used)
 * GB      - good for black (general SGF)
 * GC (GI) - game comment (general SGF) (not currently used)
 * GM (R)  - game (general SGF)
 * GN (GI) - game name (general SGF) (not currently used)
 * GS (GI) - game statistics (gnubg private)
 * GW      - good for white (general SGF)
 * HO      - hotspot (general SGF) (not currently used)
 * IT (M)  - interesting (general SGF)
 * KO (M)  - illegal move (general SGF) (not currently used)
 * LB      - label (general SGF) (not currently used)
 * LN      - line (general SGF) (not currently used)
 * LU (M)  - luck (gnubg private)
 * MA      - mark (general SGF) (not currently used)
 * MI (GI) - match information (backgammon)
 * MN (M)  - move number (general SGF) (not currently used)
 * N       - node name (general SGF) (not currently used)
 * ON (GI) - opening (general SGF) (not currently used)
 * OT (GI) - overtime (general SGF) (not currently used)
 * PB (GI) - player black (general SGF)
 * PC (GI) - place (general SGF) (not currently used)
 * PL (S)  - player (general SGF)
 * PM      - print move (general SGF) (not currently used)
 * PW (GI) - player white (general SGF)
 * RE (GI) - result (general SGF)
 * RO (GI) - round (general SGF) (not currently used)
 * RU (GI) - rules (general SGF)
 * SL      - selected (general SGF) (not currently used)
 * SO (GI) - source (general SGF) (not currently used)
 * SQ      - square (general SGF) (not currently used)
 * ST (R)  - style (general SGF) (not currently used)
 * TE (M)  - good move (general SGF)
 * TM (GI) - time limit (general SGF) (not currently used)
 * TR      - triangle (general SGF) (not currently used)
 * UC      - unclear (general SGF) (not currently used)
 * US (GI) - user (general SGF) (not currently used)
 * V       - value (general SGF) (not currently used)
 * VW      - view (general SGF) (not currently used)
 * W (M)   - white move (general SGF)
 * WL (M)  - white time left (general SGF) (not currently used)
 * WR (GI) - white rank (general SGF) (not currently used)
 * WT (GI) - white team (general SGF) (not currently used)
 * 
 */
#endif
