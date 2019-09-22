/*
 * external.c
 *
 * by Gary Wong <gtw@gnu.org>, 2001, 2002.
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
 * $Id: external.c,v 1.94 2015/02/08 15:19:07 plm Exp $
 */

#include "config.h"

#if HAVE_SOCKETS

#define DEBUG_PREFIX "DBG: "

#include <stdlib.h>
#include <signal.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "glib-ext.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif                          /* #if HAVE_UNISTD_H */

#ifndef WIN32

#include <stdio.h>
#include <errno.h>
#include <string.h>

#if HAVE_SYS_SOCKET_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#endif                          /* #if HAVE_SYS_SOCKET_H */

#else                           /* #ifndef WIN32 */

#include <winsock2.h>
#include <ws2tcpip.h>

#define EWOULDBLOCK             WSAEWOULDBLOCK
#define EINPROGRESS             WSAEINPROGRESS
#define EALREADY                WSAEALREADY
#define ENOTSOCK                WSAENOTSOCK
#define EDESTADDRREQ            WSAEDESTADDRREQ
#define EMSGSIZE                WSAEMSGSIZE
#define EPROTOTYPE              WSAEPROTOTYPE
#define ENOPROTOOPT             WSAENOPROTOOPT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define EOPNOTSUPP              WSAEOPNOTSUPP
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#define EADDRINUSE              WSAEADDRINUSE
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define ENETDOWN                WSAENETDOWN
#define ENETUNREACH             WSAENETUNREACH
#define ENETRESET               WSAENETRESET
#define ECONNABORTED            WSAECONNABORTED
#define ECONNRESET              WSAECONNRESET
#define ENOBUFS                 WSAENOBUFS
#define EISCONN                 WSAEISCONN
#define ENOTCONN                WSAENOTCONN
#define ESHUTDOWN               WSAESHUTDOWN
#define ETOOMANYREFS            WSAETOOMANYREFS
#define ETIMEDOUT               WSAETIMEDOUT
#define ECONNREFUSED            WSAECONNREFUSED
#define ELOOP                   WSAELOOP
#define ENAMETOOLONG            WSAENAMETOOLONG
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
#define ENOTEMPTY               WSAENOTEMPTY
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE
#define EINVAL                  WSAEINVAL
#define EINTR                   WSAEINTR

#define inet_aton(ip,addr)  (addr)->s_addr = inet_addr(ip), 1
#define inet_pton(fam,ip,addr) (addr)->s_addr = inet_addr(ip), 1

#endif                          /* #ifndef WIN32 */

#endif                          /* HAVE_SOCKETS */

#include "backgammon.h"
#include "drawboard.h"
#include "external.h"
#include "rollout.h"
#include "eval.h"
#include "matchid.h"
#include "lib/gnubg-types.h"

#if HAVE_SOCKETS

#if defined(AF_UNIX) && !defined(AF_LOCAL)
#define AF_LOCAL AF_UNIX
#define PF_LOCAL PF_UNIX
#endif

#ifdef WIN32
void
OutputWin32SocketError(const char *action)
{
    LPVOID lpMsgBuf;
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) & lpMsgBuf, 0, NULL) != 0) {
        outputerrf("%s (%s):\n%s", _("Windows socket error"), action, (LPCTSTR) lpMsgBuf);
        if (LocalFree(lpMsgBuf) != NULL)
            g_print("LocalFree() failed\n");
    }
}
#endif

extern int
ExternalSocket(struct sockaddr **ppsa, int *pcb, char *sz)
{

    int sock, f;
#ifndef WIN32
    struct sockaddr_un *psun;
#endif
    struct sockaddr_in *psin;
    struct hostent *phe;
    char *pch;

    if ((pch = strchr(sz, ':')) && !strchr(sz, '/')) {
        /* Internet domain socket. */
        if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
            return -1;

        f = TRUE;

#ifdef WIN32
        if (setsockopt((SOCKET) sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &f, sizeof f))
#else
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &f, sizeof f))
#endif                          /* WIN32 */
            return -1;

        psin = malloc(*pcb = sizeof(struct sockaddr_in));
        memset(psin, 0, sizeof(*psin));

        psin->sin_family = AF_INET;

        /* split hostname and port */
        *pch = 0;

        if (!*sz)
            /* no host specified */
            psin->sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef WIN32
        else {
#else
        else if (!inet_aton(sz, &psin->sin_addr)) {
#endif                          /* WIN32 */
            if ((phe = gethostbyname(sz)) == 0) {
                *pch = ':';
                errno = EINVAL;
                free(psin);
                return -1;
            }
			memcpy(&(psin->sin_addr), phe->h_addr, phe->h_length); 
        }

        *pch++ = ':';

        psin->sin_port = htons((u_short) atoi(pch));

        *ppsa = (struct sockaddr *) psin;
    } else {
#ifndef WIN32
        /* Local domain socket. */
        if ((sock = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0)
            return -1;

        /* yuck... there's no portable way to obtain the necessary
         * sockaddr_un size, but this is a conservative estimate */
        psun = malloc(*pcb = 16 + (int) strlen(sz));

        psun->sun_family = AF_LOCAL;
        strcpy(psun->sun_path, sz);
        *ppsa = (struct sockaddr *) psun;
#else                           /* #ifndef WIN32 */
        /* FIXME: what will we do on Windows? */
        return -1;
#endif                          /* #ifndef WIN32 */
    }

    return sock;
}
#endif                          /* HAVE_SOCKETS */

#if HAVE_SOCKETS
static void
ExternalUnbind(char *sz)
{

    if (strchr(sz, ':') && !strchr(sz, '/'))
        /* it was a TCP socket; no cleanup necessary */
        return;

    g_unlink(sz);
}
#endif                          /* HAVE_SOCKETS */

#if HAVE_SOCKETS
extern int
ExternalRead(int h, char *pch, size_t cch)
{

    char *p = pch, *pEnd;
#ifndef WIN32
    ssize_t n;
    psighandler sh;
#else
    int n;
#endif

    while (cch) {
        ProcessEvents();

        if (fInterrupt)
            return -2;

#ifndef WIN32
        PortableSignal(SIGPIPE, SIG_IGN, &sh, FALSE);
#endif

#ifdef WIN32
        /* reading from sockets doesn't work on Windows
         * use recv instead */
        n = recv((SOCKET) h, p, cch, 0);
#else
        n = read(h, p, cch);
#endif

#ifndef WIN32
        PortableSignalRestore(SIGPIPE, &sh);
#endif

        if (!n) {
            outputl(_("External connection closed."));
            return -1;
        } else if (n < 0) {
            if (errno == EINTR)
                continue;

            SockErr(_("reading from external connection"));
            return -1;
        }

        if ((pEnd = memchr(p, '\n', n))) {
            *pEnd = 0;
            return 0;
        }

        cch -= n;
        p += n;

    }

    p[cch - 1] = 0;
    return 0;
}
#endif                          /* HAVE_SOCKETS */

#if HAVE_SOCKETS
extern int
ExternalWrite(int h, char *pch, size_t cch)
{

    char *p = pch;
#ifndef WIN32
    ssize_t n;
    psighandler sh;
#else
    int n;
#endif

    while (cch) {
        ProcessEvents();

        if (fInterrupt)
            return -1;

#ifndef WIN32
        PortableSignal(SIGPIPE, SIG_IGN, &sh, FALSE);
#endif

#ifdef WIN32
        /* writing to sockets doesn't work on Windows
         * use send instead */
        n = send((SOCKET) h, (const char *) p, cch, 0);
#else
        n = write(h, p, cch);
#endif

#ifndef WIN32
        PortableSignalRestore(SIGPIPE, &sh);
#endif

        if (!n)
            return 0;
        else if (n < 0) {
            if (errno == EINTR)
                continue;

            SockErr(_("writing to external connection"));
            return -1;
        }

        cch -= n;
        p += n;
    }

    return 0;
}
#endif                          /* HAVE_SOCKETS */


#if HAVE_SOCKETS
static void
unset_scan_context(scancontext * pScanCtx, int bFreeScanner)
{
    if (pScanCtx->bi.gsName)
        g_string_free(pScanCtx->bi.gsName, TRUE);
    if (pScanCtx->bi.gsOpp)
        g_string_free(pScanCtx->bi.gsOpp, TRUE);
    if (pScanCtx->szError)
        g_free(pScanCtx->szError);

    pScanCtx->bi.gsName = NULL;
    pScanCtx->bi.gsOpp = NULL;
    pScanCtx->szError = NULL;
    pScanCtx->fError = 0;

    if (bFreeScanner) {
        ExtDestroyParse(pScanCtx->scanner);
        pScanCtx->scanner = NULL;
    }
}

static void
ErrorHandler(scancontext * pScanCtx, const char *szMessage)
{
    pScanCtx->fError = TRUE;
    if (pScanCtx->szError)
        g_free(pScanCtx->szError);

    pScanCtx->szError = g_strdup_printf("Error: %s\n", szMessage);
}

static scancontext *
ExtParse(scancontext * scanctx, const char *szCommand)
{
    scanctx->ExtErrorHandler = ErrorHandler;
    scanctx->fError = FALSE;
    scanctx->szError = NULL;

    ExtStartParse(scanctx->scanner, szCommand);
    return scanctx->fError ? NULL : scanctx;
}

static char *
ExtEvaluation(scancontext * pec)
{
    ProcessedFIBSBoard processedBoard;
    float arOutput[NUM_ROLLOUT_OUTPUTS];
    cubeinfo ci;
    int anScore[2];
    char *szResponse;
    float r;
    evalcontext ec;

    if (ProcessFIBSBoardInfo(&pec->bi, &processedBoard)) {
        szResponse = g_strdup_printf("Error: badly formed board\n");
    } else {

        anScore[0] = processedBoard.nScoreOpp;
        anScore[1] = processedBoard.nScore;
        /* If the session isn't using Crawford rule, set crawford flag to false */
        processedBoard.fCrawford = pec->fCrawfordRule ? processedBoard.fCrawford : FALSE;
        /* Set the Jacoby flag appropriately from the external interface settings */
        processedBoard.fJacoby = pec->fJacobyRule;

        /* printf ("Jacoby Setting: %d\n", fJacoby); */
        /* printf ("Crawford Setting: %d\n", fCrawford); */

        SetCubeInfo(&ci, processedBoard.nCube, processedBoard.fCubeOwner, 1, processedBoard.nMatchTo,
                    anScore, processedBoard.fCrawford, processedBoard.fJacoby, nBeavers, bgvDefault);

        ec.fCubeful = pec->fCubeful;
        ec.nPlies = pec->nPlies;
        ec.fUsePrune = pec->fUsePrune;
        ec.fDeterministic = pec->fDeterministic;
        ec.rNoise = pec->rNoise;

        if (GeneralEvaluationE(arOutput, (ConstTanBoard) processedBoard.anBoard, &ci, &ec))
            return NULL;

        if (processedBoard.nMatchTo) {
            if (ec.fCubeful)
                r = arOutput[OUTPUT_CUBEFUL_EQUITY];
            else
                r = eq2mwc(arOutput[OUTPUT_EQUITY], &ci);
        } else
            r = ec.fCubeful ? arOutput[6] : arOutput[5];

        szResponse = g_strdup_printf("%f %f %f %f %f %f\n",
                                     arOutput[0], arOutput[1], arOutput[2], arOutput[3], arOutput[4], r);
    }

    return szResponse;

}

static char *
ExtFIBSBoard(scancontext * pec)
{
    ProcessedFIBSBoard processedBoard;
    TanBoard anBoardOrig;
    int anScore[2], anMove[8], fTurn;
    float arDouble[NUM_CUBEFUL_OUTPUTS], aarOutput[2][NUM_ROLLOUT_OUTPUTS], aarStdDev[2][NUM_ROLLOUT_OUTPUTS];
    rolloutstat aarsStatistics[2][2];
    cubeinfo ci;
    char *szResponse;

    if (ProcessFIBSBoardInfo(&pec->bi, &processedBoard)) {
        szResponse = g_strdup_printf("Error: badly formed board\n");
    } else {

        anScore[0] = processedBoard.nScoreOpp;
        anScore[1] = processedBoard.nScore;

        /* If the session isn't using Crawford rule, set crawford flag to false */
        processedBoard.fCrawford = pec->fCrawfordRule ? processedBoard.fCrawford : FALSE;
        /* Set the Jacoby flag appropriately from the external interface settings */
        processedBoard.fJacoby = pec->fJacobyRule;

        /* printf ("Crawford Setting: %d\n", fCrawford); */
        /* printf ("Jacoby Setting: %d\n", fJacoby); */

        fTurn = 1;
        SetCubeInfo(&ci, processedBoard.nCube, processedBoard.fCubeOwner, fTurn, processedBoard.nMatchTo,
                    anScore, processedBoard.fCrawford, processedBoard.fJacoby, nBeavers, bgvDefault);

        memcpy(anBoardOrig, processedBoard.anBoard, sizeof(processedBoard.anBoard));

        if (processedBoard.fDoubled) {

            /* take decision */
            if (GeneralCubeDecision(aarOutput, aarStdDev,
                                    aarsStatistics, (ConstTanBoard) processedBoard.anBoard, &ci, GetEvalCube(), NULL,
                                    NULL) < 0)
                return NULL;

            switch (FindCubeDecision(arDouble, aarOutput, &ci)) {
            case DOUBLE_PASS:
            case TOOGOOD_PASS:
            case REDOUBLE_PASS:
            case TOOGOODRE_PASS:
                szResponse = g_strdup("drop\n");
                break;

            case NODOUBLE_BEAVER:
            case DOUBLE_BEAVER:
            case NO_REDOUBLE_BEAVER:
                szResponse = g_strdup("beaver\n");
                break;

            default:
                szResponse = g_strdup("take\n");
            }

        } else if (pec->nResignation) {

            /* if opp wants to resign (extension to FIBS board) */

            float arOutput[NUM_ROLLOUT_OUTPUTS];
            float rEqBefore, rEqAfter;
            const float epsilon = 1.0e-6f;

            getResignation(arOutput, processedBoard.anBoard, &ci, &esEvalCube);

            getResignEquities(arOutput, &ci, pec->nResignation, &rEqBefore, &rEqAfter);

            /* if opponent gives up equity by resigning */
            if ((rEqAfter - epsilon) < rEqBefore)
                szResponse = g_strdup("accept\n");
            else
                szResponse = g_strdup("reject\n");

        } else if (processedBoard.anDice[0]) {
            /* move */
            char szMove[64];
            if (FindBestMove(anMove, processedBoard.anDice[0], processedBoard.anDice[1],
                             processedBoard.anBoard, &ci, &GetEvalChequer()->ec, *GetEvalMoveFilter()) < 0)
                return NULL;

            FormatMovePlain(szMove, anBoardOrig, anMove);
            szResponse = g_strconcat(szMove, "\n", NULL);
        } else {
            /* double decision */
            if (GeneralCubeDecision(aarOutput, aarStdDev,
                                    aarsStatistics, (ConstTanBoard) processedBoard.anBoard, &ci, GetEvalCube(),
                                    NULL, NULL) < 0)
                return NULL;

            switch (FindCubeDecision(arDouble, aarOutput, &ci)) {
            case DOUBLE_TAKE:
            case DOUBLE_PASS:
            case DOUBLE_BEAVER:
            case REDOUBLE_TAKE:
            case REDOUBLE_PASS:
                szResponse = g_strdup("double\n");
                break;

            default:
                szResponse = g_strdup("roll\n");
            }
        }

    }

    return szResponse;

}
#endif

extern void
CommandExternal(char *sz)
{

#if !HAVE_SOCKETS
    outputl(_("This installation of GNU Backgammon was compiled without\n"
              "socket support, and does not implement external controllers."));
#else
    int h, hPeer, cb;
    struct sockaddr *psa;
    char szCommand[256];
    char *szResponse = NULL;
    struct sockaddr_in saRemote;
    socklen_t saLen;
    scancontext scanctx;
    int fExit;
    int fRestart = TRUE;
    int retval;

    sz = NextToken(&sz);

    if (!sz || !*sz) {
        outputl(_("You must specify the name of the socket to the external controller."));
        return;
    }

    memset(&scanctx, 0, sizeof(scanctx));
    ExtInitParse(&scanctx.scanner);

  listenloop:
    {
        fExit = FALSE;
        scanctx.fDebug = FALSE;
        scanctx.fNewInterface = FALSE;

        if ((h = ExternalSocket(&psa, &cb, sz)) < 0) {
            SockErr(sz);
            ExtDestroyParse(scanctx.scanner);
            return;
        }

        if (bind(h, psa, cb) < 0) {
            SockErr(sz);
            closesocket(h);
            free(psa);
            ExtDestroyParse(scanctx.scanner);
            return;
        }

        free(psa);

        if (listen(h, 1) < 0) {
            SockErr("listen");
            closesocket(h);
            ExternalUnbind(sz);
            ExtDestroyParse(scanctx.scanner);
            return;
        }
        outputf(_("Waiting for a connection from %s...\n"), sz);
        outputx();
        ProcessEvents();

        /* Must set length when using windows */
        saLen = sizeof(struct sockaddr);
        while ((hPeer = accept(h, (struct sockaddr *) &saRemote, &saLen)) < 0) {
            if (errno == EINTR) {
                ProcessEvents();

                if (fInterrupt) {
                    closesocket(h);
                    ExternalUnbind(sz);
                    ExtDestroyParse(scanctx.scanner);
                    return;
                }

                continue;
            }

            SockErr("accept");
            closesocket(h);
            ExternalUnbind(sz);
            ExtDestroyParse(scanctx.scanner);
            return;
        }

        closesocket(h);
        ExternalUnbind(sz);

        /* print info about remove client */

        outputf(_("Accepted connection from %s.\n"), inet_ntoa(saRemote.sin_addr));
        outputx();
        ProcessEvents();

        while (!fExit && !(retval = ExternalRead(hPeer, szCommand, sizeof(szCommand)))) {
            /* To keep lexer happy terminate each line with \n */
            if (szCommand[strlen(szCommand)-1] != '\n')
                strcat(szCommand, "\n");

            if ((ExtParse(&scanctx, szCommand)) == 0) {
                /* parse error */
                szResponse = scanctx.szError;
            } else {
                ProcessedFIBSBoard processedBoard;
                GValue *optionsmapgv;
                GValue *boarddatagv;
                GString *dbgStr;
                int anScore[2];
                int fCrawford, fJacoby;
                char *asz[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                char szBoard[10000];
                char **aszLines;
                char **aszLinesOrig;
                char *szMatchID;
                gchar *szOptStr;

                switch (scanctx.ct) {
                case COMMAND_HELP:
                    szResponse = g_strdup("\tNo help information available\n");
                    break;

                case COMMAND_SET:
                    szOptStr = g_value_get_gstring_gchar(g_list_nth_data(scanctx.pCmdData, 0));
                    if (g_ascii_strcasecmp(szOptStr, KEY_STR_DEBUG) == 0) {
                        scanctx.fDebug = g_value_get_int(g_list_nth_data(scanctx.pCmdData, 1));
                        szResponse = g_strdup_printf("Debug output %s\n", scanctx.fDebug ? "ON" : "OFF");
                    } else if (g_ascii_strcasecmp(szOptStr, KEY_STR_NEWINTERFACE) == 0) {
                        scanctx.fNewInterface = g_value_get_int(g_list_nth_data(scanctx.pCmdData, 1));
                        szResponse = g_strdup_printf("New interface %s\n", scanctx.fNewInterface ? "ON" : "OFF");
                    } else {
                        szResponse = g_strdup_printf("Error: set option '%s' not supported\n", szOptStr);
                    }
                    g_list_gv_boxed_free(scanctx.pCmdData);

                    break;

                case COMMAND_VERSION:
                    szResponse = g_strdup("Interface: " EXTERNAL_INTERFACE_VERSION "\n"
                                          "RFBF: " RFBF_VERSION_SUPPORTED "\n"
                                          "Engine: " WEIGHTS_VERSION "\n" "Software: " VERSION "\n");

                    break;

                case COMMAND_NONE:
                    szResponse = g_strdup("Error: no command given\n");
                    break;

                case COMMAND_FIBSBOARD:
                case COMMAND_EVALUATION:
                    if (scanctx.fDebug) {
                        optionsmapgv = (GValue *) g_list_nth_data(g_value_get_boxed(scanctx.pCmdData), 1);
                        boarddatagv = (GValue *) g_list_nth_data(g_value_get_boxed(scanctx.pCmdData), 0);
                        dbgStr = g_string_new(DEBUG_PREFIX);
                        g_value_tostring(dbgStr, optionsmapgv, 0);
                        g_string_append(dbgStr, "\n" DEBUG_PREFIX);
                        g_value_tostring(dbgStr, boarddatagv, 0);
                        g_string_append(dbgStr, "\n" DEBUG_PREFIX "\n");
                        ExternalWrite(hPeer, dbgStr->str, strlen(dbgStr->str));
                        ProcessFIBSBoardInfo(&scanctx.bi, &processedBoard);

                        anScore[0] = processedBoard.nScoreOpp;
                        anScore[1] = processedBoard.nScore;
                        /* If the session isn't using Crawford rule, set crawford flag to false */
                        fCrawford = scanctx.fCrawfordRule ? processedBoard.fCrawford : FALSE;
                        /* Set the Jacoby flag appropriately from the external interface settings */
                        fJacoby = scanctx.fJacobyRule;

                        szMatchID = MatchID((unsigned int *) processedBoard.anDice, 1, processedBoard.nResignation,
                                            processedBoard.fDoubled, 1, processedBoard.fCubeOwner, fCrawford,
                                            processedBoard.nMatchTo, anScore, processedBoard.nCube, fJacoby,
                                            GAME_PLAYING);

                        DrawBoard(szBoard, (ConstTanBoard) & processedBoard.anBoard, 1, asz, szMatchID, 15);

                        aszLines = g_strsplit(&szBoard[0], "\n", 32);
                        aszLinesOrig = aszLines;
                        while (*aszLines) {
                            ExternalWrite(hPeer, DEBUG_PREFIX, strlen(DEBUG_PREFIX));
                            ExternalWrite(hPeer, *aszLines, strlen(*aszLines));
                            ExternalWrite(hPeer, "\n", 1);
                            aszLines++;
                        }

                        dbgStr = g_string_assign(dbgStr, "");
                        g_string_append_printf(dbgStr, DEBUG_PREFIX "X is %s, O is %s\n", processedBoard.szPlayer,
                                               processedBoard.szOpp);
                        if (processedBoard.nMatchTo) {
                            g_string_append_printf(dbgStr, DEBUG_PREFIX "Match Play %s Crawford Rule\n",
                                                   scanctx.fCrawfordRule ? "with" : "without");
                            g_string_append_printf(dbgStr, DEBUG_PREFIX "Score: %d-%d/%d%s, ", processedBoard.nScore,
                                                   processedBoard.nScoreOpp, processedBoard.nMatchTo,
                                                   fCrawford ? "*" : "");
                        } else {
                            g_string_append_printf(dbgStr, DEBUG_PREFIX "Money Session %s Jacoby Rule, %s Beavers\n",
                                                   scanctx.fJacobyRule ? "with" : "without",
                                                   scanctx.fBeavers ? "with" : "without");
                            g_string_append_printf(dbgStr, DEBUG_PREFIX "Score: %d-%d, ", processedBoard.nScore,
                                                   processedBoard.nScoreOpp);
                        }
                        g_string_append_printf(dbgStr, "Roll: %d%d\n",
                                               processedBoard.anDice[0], processedBoard.anDice[1]);
                        g_string_append_printf(dbgStr,
                                               DEBUG_PREFIX
                                               "CubeOwner: %d, Cube: %d, Turn: %c, Doubled: %d, Resignation: %d\n",
                                               processedBoard.fCubeOwner, processedBoard.nCube, 'X',
                                               processedBoard.fDoubled, processedBoard.nResignation);
                        g_string_append(dbgStr, DEBUG_PREFIX "\n");
                        ExternalWrite(hPeer, dbgStr->str, strlen(dbgStr->str));

                        g_string_free(dbgStr, TRUE);
                        g_strfreev(aszLinesOrig);
                    }
                    g_value_unsetfree(scanctx.pCmdData);

                    if (scanctx.ct == COMMAND_EVALUATION)
                        szResponse = ExtEvaluation(&scanctx);
                    else
                        szResponse = ExtFIBSBoard(&scanctx);

                    break;

                case COMMAND_EXIT:
                    closesocket(hPeer);
                    fExit = TRUE;
                    break;

                default:
                    szResponse = g_strdup("Unsupported Command\n");
                }
                unset_scan_context(&scanctx, FALSE);
            }

            if (szResponse) {
                if (ExternalWrite(hPeer, szResponse, strlen(szResponse)))
                    break;

                g_free(szResponse);
                szResponse = NULL;
            }

        }
        /* Interrupted : get out of listen loop */
        if (retval == -2) {
            ProcessEvents();
            fRestart = FALSE;
        }

        closesocket(hPeer);
        if (szResponse)
            g_free(szResponse);

        szResponse = NULL;
        scanctx.szError = NULL;
    }
    if (fRestart)
        goto listenloop;

    unset_scan_context(&scanctx, TRUE);
#endif
}
