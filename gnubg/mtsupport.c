/*
 * mtsupport.c
 *
 * by Jon Kinsey, 2008
 *
 * Multithreaded support functions, moved out of multithread.c
 *
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
 * $Id: mtsupport.c,v 1.10 2015/01/17 21:56:19 mdpetch Exp $
 */

#include "config.h"
#include "multithread.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if USE_GTK
#include <gtk/gtk.h>
#include <gtkgame.h>
#endif

#include "speed.h"
#include "rollout.h"
#include "util.h"
#include "lib/simd.h"

SSE_ALIGN(ThreadData td);

extern ThreadLocalData *
MT_CreateThreadLocalData(int id)
{
    ThreadLocalData *tld = (ThreadLocalData *) malloc(sizeof(ThreadLocalData));
    tld->id = id;
    tld->pnnState = (NNState *) malloc(sizeof(NNState) * 3);
    memset(tld->pnnState, 0, sizeof(NNState) * 3);
    tld->pnnState[CLASS_RACE - CLASS_RACE].savedBase = malloc(nnRace.cHidden * sizeof(float));
    memset(tld->pnnState[CLASS_RACE - CLASS_RACE].savedBase, 0, nnRace.cHidden * sizeof(float));
    tld->pnnState[CLASS_RACE - CLASS_RACE].savedIBase = malloc(nnRace.cInput * sizeof(float));
    memset(tld->pnnState[CLASS_RACE - CLASS_RACE].savedIBase, 0, nnRace.cInput * sizeof(float));
    tld->pnnState[CLASS_CRASHED - CLASS_RACE].savedBase = malloc(nnCrashed.cHidden * sizeof(float));
    memset(tld->pnnState[CLASS_CRASHED - CLASS_RACE].savedBase, 0, nnCrashed.cHidden * sizeof(float));
    tld->pnnState[CLASS_CRASHED - CLASS_RACE].savedIBase = malloc(nnCrashed.cInput * sizeof(float));
    memset(tld->pnnState[CLASS_CRASHED - CLASS_RACE].savedIBase, 0, nnCrashed.cInput * sizeof(float));
    tld->pnnState[CLASS_CONTACT - CLASS_RACE].savedBase = malloc(nnContact.cHidden * sizeof(float));
    memset(tld->pnnState[CLASS_CONTACT - CLASS_RACE].savedBase, 0, nnContact.cHidden * sizeof(float));
    tld->pnnState[CLASS_CONTACT - CLASS_RACE].savedIBase = malloc(nnContact.cInput * sizeof(float));
    memset(tld->pnnState[CLASS_CONTACT - CLASS_RACE].savedIBase, 0, nnContact.cInput * sizeof(float));

    tld->aMoves = (move *) malloc(sizeof(move) * MAX_INCOMPLETE_MOVES);
    memset(tld->aMoves, 0, sizeof(move) * MAX_INCOMPLETE_MOVES);
    return tld;
}

#if USE_MULTITHREAD

#if defined(DEBUG_MULTITHREADED) && defined(WIN32)
unsigned int mainThreadID;
#endif

#ifdef GLIB_THREADS
#if GLIB_CHECK_VERSION (2,32,0)
static GMutex condMutex;        /* Extra mutex needed for waiting */
#else
static GMutex *condMutex = NULL;        /* Extra mutex needed for waiting */
#endif
#endif

#ifdef GLIB_THREADS

#if GLIB_CHECK_VERSION (2,32,0)
/* Dynamic allocation of GPrivate is deprecated */
static GPrivate private_item = G_PRIVATE_INIT(free);

extern void
TLSCreate(TLSItem * pItem)
{
    *pItem = &private_item;
}
#else
extern void
TLSCreate(TLSItem * pItem)
{
    *pItem = g_private_new(g_free);
}
#endif

extern void
TLSFree(TLSItem UNUSED(pItem))
{                               /* Done automaticaly by glib */
}

extern void
TLSSetValue(TLSItem pItem, size_t value)
{
    size_t *pNew = (size_t *) malloc(sizeof(size_t));
    *pNew = value;
    g_private_set(pItem, (gpointer) pNew);
}

#define TLSGet(item) *((size_t*)g_private_get(item))

extern void
InitManualEvent(ManualEvent * pME)
{
    ManualEvent pNewME = g_malloc(sizeof(*pNewME));
#if GLIB_CHECK_VERSION (2,32,0)
    g_cond_init(&pNewME->cond);
#else
    pNewME->cond = g_cond_new();
#endif
    pNewME->signalled = FALSE;
    *pME = pNewME;
}

extern void
FreeManualEvent(ManualEvent ME)
{
#if GLIB_CHECK_VERSION (2,32,0)
    g_cond_clear(&ME->cond);
#else
    g_cond_free(ME->cond);
#endif
    g_free(ME);
}

extern void
WaitForManualEvent(ManualEvent ME)
{
#if GLIB_CHECK_VERSION (2,32,0)
    gint64 end_time;
#else
    GTimeVal tv;
#endif
    multi_debug("wait for manual event locks");
#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_lock(&condMutex);
    end_time = g_get_monotonic_time() + 10 * G_TIME_SPAN_SECOND;
#else
    g_mutex_lock(condMutex);
#endif
    while (!ME->signalled) {
        multi_debug("waiting for manual event");
#if GLIB_CHECK_VERSION (2,32,0)
        if (!g_cond_wait_until(&ME->cond, &condMutex, end_time))
#else
        g_get_current_time(&tv);
        g_time_val_add(&tv, 10 * 1000 * 1000);
        if (g_cond_timed_wait(ME->cond, condMutex, &tv))
#endif
            break;
        else {
            multi_debug("still waiting for manual event");
        }
    }

#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_unlock(&condMutex);
#else
    g_mutex_unlock(condMutex);
#endif
    multi_debug("wait for manual event unlocks");
}

extern void
ResetManualEvent(ManualEvent ME)
{
    multi_debug("reset manual event locks");
#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_lock(&condMutex);
    ME->signalled = FALSE;
    g_mutex_unlock(&condMutex);
#else
    g_mutex_lock(condMutex);
    ME->signalled = FALSE;
    g_mutex_unlock(condMutex);
#endif
    multi_debug("reset manual event unlocks");
}

extern void
SetManualEvent(ManualEvent ME)
{
    multi_debug("reset manual event locks");
#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_lock(&condMutex);
    ME->signalled = TRUE;
    g_cond_broadcast(&ME->cond);
    g_mutex_unlock(&condMutex);
#else
    g_mutex_lock(condMutex);
    ME->signalled = TRUE;
    g_cond_broadcast(ME->cond);
    g_mutex_unlock(condMutex);
#endif
    multi_debug("reset manual event unlocks");
}

#if GLIB_CHECK_VERSION (2,32,0)
extern void
InitMutex(Mutex * pMutex)
{
    g_mutex_init(pMutex);
}

extern void
FreeMutex(Mutex * mutex)
{
    g_mutex_clear(mutex);
}
#else
extern void
InitMutex(Mutex * pMutex)
{
    *pMutex = g_mutex_new();
}

extern void
FreeMutex(Mutex * mutex)
{
    g_mutex_free(*mutex);
}
#endif

extern void
Mutex_Lock(Mutex * mutex, const char *reason)
{
#ifdef DEBUG_MULTITHREADED
    multi_debug(reason);
#else
    (void) reason;
#endif
#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_lock(mutex);
#else
    g_mutex_lock(*mutex);
#endif
}

extern void
Mutex_Release(Mutex * mutex)
{
#ifdef DEBUG_MULTITHREADED
    multi_debug("Releasing lock");
#endif
#if GLIB_CHECK_VERSION (2,32,0)
    g_mutex_unlock(mutex);
#else
    g_mutex_unlock(*mutex);
#endif
}

#else                           /* win32 */

extern void
TLSCreate(TLSItem * ppItem)
{
    *ppItem = TlsAlloc();
    if (*ppItem == TLS_OUT_OF_INDEXES)
        PrintSystemError("calling TlsAlloc");
}

extern void
TLSFree(TLSItem pItem)
{
    free(TlsGetValue(pItem));
    TlsFree(pItem);
}

extern void
TLSSetValue(TLSItem pItem, int value)
{
    int *pNew = (int *) malloc(sizeof(int));
    *pNew = value;
    if (TlsSetValue(pItem, pNew) == 0)
        PrintSystemError("calling TLSSetValue");
}

#define TLSGet(item) *((int*)TlsGetValue(item))

extern void
InitManualEvent(ManualEvent * pME)
{
    *pME = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (*pME == NULL)
        PrintSystemError("creating manual event");
}

extern void
FreeManualEvent(ManualEvent ME)
{
    CloseHandle(ME);
}

extern void
InitMutex(Mutex * pMutex)
{
    *pMutex = CreateMutex(NULL, FALSE, NULL);
    if (*pMutex == NULL)
        PrintSystemError("creating mutex");
}

extern void
FreeMutex(Mutex * mutex)
{
    CloseHandle(*mutex);
}

#ifdef DEBUG_MULTITHREADED
void
Mutex_Lock(Mutex mutex, const char *reason)
{
    if (WaitForSingleObject(mutex, 0) == WAIT_OBJECT_0) {       /* Got mutex */
        multi_debug("%s: lock acquired", reason);
    } else {
        multi_debug("%s: waiting on lock", reason);
        WaitForSingleObject(mutex, INFINITE);
        multi_debug("lock relinquished");
    }
}

void
Mutex_Release(Mutex mutex)
{
    multi_debug("Releasing lock");
    ReleaseMutex(mutex);
}

#else
#define Mutex_Lock(mutex, reason) WaitForSingleObject(mutex, INFINITE)
#define Mutex_Release(mutex) ReleaseMutex(mutex)
#endif

#endif

extern void
MT_InitThreads(void)
{
#if !GLIB_CHECK_VERSION (2,32,0)
    if (!g_thread_supported())
        g_thread_init(NULL);
    g_assert(g_thread_supported());
#endif
    td.tasks = NULL;
    td.doneTasks = td.addedTasks = 0;
    td.totalTasks = -1;
    InitManualEvent(&td.activity);
    TLSCreate(&td.tlsItem);
    TLSSetValue(td.tlsItem, (size_t) MT_CreateThreadLocalData(0));      /* Main thread shares id 0 */

#if defined(DEBUG_MULTITHREADED) && defined(WIN32)
    mainThreadID = GetCurrentThreadId();
#endif
    InitMutex(&td.multiLock);
    InitMutex(&td.queueLock);
    InitManualEvent(&td.syncStart);
    InitManualEvent(&td.syncEnd);
#if defined(GLIB_THREADS) && !GLIB_CHECK_VERSION (2,32,0)
    if (condMutex == NULL)
        condMutex = g_mutex_new();
#endif
    td.numThreads = 0;
}

extern void
CloseThread(void *UNUSED(unused))
{
    int i;
    NNState *pnnState = ((ThreadLocalData *) TLSGet(td.tlsItem))->pnnState;

    g_assert(td.closingThreads);

    ThreadLocalData *pTLD = (ThreadLocalData *) TLSGet(td.tlsItem);
    if (pTLD->aMoves)
        free(pTLD->aMoves);

    for (i = 0; i < 3; i++) {
        free(pnnState[i].savedBase);
        free(pnnState[i].savedIBase);
    }
    free(((ThreadLocalData *) TLSGet(td.tlsItem))->pnnState);
    free((void *) TLSGet(td.tlsItem));
    MT_SafeInc(&td.result);
}

/* Revisit this later for inclusion */
#if 0
extern void
MT_CloseThreads(void)
{
    td.closingThreads = TRUE;
    mt_add_tasks(td.numThreads, CloseThread, NULL, NULL);
    if (MT_WaitForTasks(NULL, 0, FALSE) != (int) td.numThreads)
        g_print("Error closing threads!\n");
}
#endif

extern void
MT_Close(void)
{
    MT_CloseThreads();

    FreeManualEvent(td.activity);
    FreeMutex(&td.multiLock);
    FreeMutex(&td.queueLock);

    FreeManualEvent(td.syncStart);
    FreeManualEvent(td.syncEnd);
}

extern void
MT_Exclusive(void)
{
    Mutex_Lock(&td.multiLock, "Exclusive lock");
}

extern void
MT_Release(void)
{
    Mutex_Release(&td.multiLock);
}

#ifdef DEBUG_MULTITHREADED
extern void
multi_debug(const char *str, ...)
{
    int id;
    va_list vl;
    char tn[5];
    char buf[1024];

    /* Sync output so order makes some sense */
#ifdef WIN32
    WaitForSingleObject(td.multiLock, INFINITE);
#endif
    /* With glib threads, locking here doesn't seem to work :
     * GLib (gthread-posix.c): Unexpected error from C library during 'pthread_mutex_lock': Resource deadlock avoided.  Aborting.
     * Timestamp the output instead.
     * Maybe the g_get_*_time() calls will sync output as well anyway... */

    va_start(vl, str);
    vsprintf(buf, str, vl);

    id = MT_GetThreadID();
#if defined(WIN32)
    if (id == 0 && GetCurrentThreadId() == mainThreadID)
#else
    if (id == 0)
#endif
        strcpy(tn, "MT");
    else
        sprintf(tn, "T%d", id + 1);

#ifdef GLIB_THREADS
#if GLIB_CHECK_VERSION (2,28,0)
    printf("%" G_GINT64_FORMAT " %s: %s\n", g_get_monotonic_time(), tn, buf);
#else
    {
        GTimeVal tv;

        g_get_current_time(&tv);
        printf("%lu %s: %s\n", 1000000UL * tv.tv_sec + tv.tv_usec, tn, buf);
    }
#endif
#else
    printf("%s: %s\n", tn, buf);
#endif

    va_end(vl);

#ifdef WIN32
    ReleaseMutex(td.multiLock);
#endif
}

#endif

#else
#include "multithread.h"
#include <stdlib.h>
#if USE_GTK
#include <gtk/gtk.h>
#include <gtkgame.h>
#endif

SSE_ALIGN(ThreadData td);

extern void
MT_InitThreads(void)
{
    td.tld = MT_CreateThreadLocalData(0);       /* Main thread shares id 0 */
}

extern void
MT_Close(void)
{
    int i;
    NNState *pnnState;

    if (!td.tld)
        return;

    free(td.tld->aMoves);
    pnnState = td.tld->pnnState;
    for (i = 0; i < 3; i++) {
        free(pnnState[i].savedBase);
        free(pnnState[i].savedIBase);
    }
    free(pnnState);
    free(td.tld);
}

#endif
