/*
 * external.h
 *
 * by Gary Wong <gtw@gnu.org>, 2001.
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
 * $Id: external.h,v 1.23 2014/06/26 08:08:38 mdpetch Exp $
 */

#ifndef EXTERNAL_H
#define EXTERNAL_H

#include <glib.h>
#include <glib-object.h>
#include "backgammon.h"

/* Stuff for the yacc/lex parser */
extern void ExtStartParse(void *scanner, const char *szCommand);
extern int ExtInitParse(void **scancontext);
extern void ExtDestroyParse(void *scancontext);

#define MAX_RFBF_ELEMENTS 53

#define KEY_STR_BEAVERS "beavers"
#define KEY_STR_RESIGNATION "resignation"
#define KEY_STR_DETERMINISTIC "deterministic"
#define KEY_STR_JACOBYRULE "jacobyrule"
#define KEY_STR_CRAWFORDRULE "crawfordrule"
#define KEY_STR_PRUNE "prune"
#define KEY_STR_NOISE "noise"
#define KEY_STR_CUBEFUL "cubeful"
#define KEY_STR_PLIES "plies"
#define KEY_STR_NEWINTERFACE "newinterface"
#define KEY_STR_DEBUG "debug"
#define KEY_STR_PROMPT "prompt"

typedef enum _cmdtype {
    COMMAND_NONE = 0,
    COMMAND_FIBSBOARD = 1,
    COMMAND_EVALUATION = 2,
    COMMAND_EXIT = 3,
    COMMAND_VERSION = 4,
    COMMAND_SET = 5,
    COMMAND_HELP = 6,
    COMMAND_LIST = 7
} cmdtype;

typedef struct _commandinfo {
    cmdtype cmdType;
    void *pvData;
} commandinfo;

typedef struct _FIBSBoardInfo {
    /* These must be in the same order of the fibs board string definition */
    int nMatchTo;
    int nScore;
    int nScoreOpp;
    int anFIBSBoard[26];
    int nTurn;
    int anDice[2];
    int anOppDice[2];
    int nCube;
    int fCanDouble;
    int fOppCanDouble;
    int fDoubled;
    int nColor;
    int nDirection;
    int fNonCrawford;
    int fPostCrawford;
    int unused[8];
    int nVersion;
    int padding[51];

    /* These must be last */
    GString *gsName;
    GString *gsOpp;
} FIBSBoardInfo;

typedef struct _ProcessedFIBSBoard {
    char szPlayer[MAX_NAME_LEN];
    char szOpp[MAX_NAME_LEN];
    int nMatchTo;
    int nScore;
    int nScoreOpp;
    int anDice[2];
    int nCube;
    int fCubeOwner;
    int fDoubled;
    int fCrawford;
    int fJacoby;
    int nResignation;
    TanBoard anBoard;
} ProcessedFIBSBoard;

typedef struct _scancontext {
    /* scanner ptr must be first element in structure */
    void *scanner;
    void (*ExtErrorHandler) (struct _scancontext *, const char *);
    int fError;
    int fDebug;
    int fNewInterface;
    char *szError;

    /* command type */
    cmdtype ct;
    void *pCmdData;
    
    /* evalcontext */
    int nPlies;
    float rNoise;
    int fDeterministic;
    int fCubeful;
    int fUsePrune;

    /* session rules */
    int fJacobyRule;
    int fCrawfordRule;
    int nResignation;
    int fBeavers;

    /* fibs board */
    union {
        FIBSBoardInfo bi;
        int anList[50];
    };
} scancontext;

#if HAVE_SOCKETS

#ifndef WIN32

#define closesocket close

#if HAVE_SYS_SOCKET_H
#include <sys/types.h>
#include <sys/socket.h>
#endif                          /* #if HAVE_SYS_SOCKET_H */

#else                           /* #ifndef WIN32 */
#include <winsock2.h>
#endif                          /* #ifndef WIN32 */

#define EXTERNAL_INTERFACE_VERSION "2"
#define RFBF_VERSION_SUPPORTED "0"

extern int ExternalSocket(struct sockaddr **ppsa, int *pcb, char *sz);
extern int ExternalRead(int h, char *pch, size_t cch);
extern int ExternalWrite(int h, char *pch, size_t cch);
#ifdef WIN32
extern void OutputWin32SocketError(const char *action);
#define SockErr OutputWin32SocketError
#else
#define SockErr outputerr
#endif

#endif                          /* #if HAVE_SOCKETS */

#endif                          /* #ifndef EXTERNAL_H */
