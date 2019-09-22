/*
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
 * $Id: multithread.h,v 1.54 2014/07/26 06:23:35 mdpetch Exp $
 */

#ifndef MULTITHREAD_H
#define MULTITHREAD_H

#include "config.h"

#if defined(WIN32)
#include <process.h>
#include <windows.h>
#endif

#if defined(USE_MULTITHREAD)
#include <glib.h>
#endif

#include "backgammon.h"

#define UI_UPDATETIME 250

/*#define DEBUG_MULTITHREADED 1 */
#if defined (USE_MULTITHREAD) && defined(DEBUG_MULTITHREADED)
void multi_debug(const char *str, ...);
#else
#define multi_debug(x)
#endif

typedef struct _Task {
    AsyncFun fun;
    void *data;
    struct _Task *pLinkedTask;
} Task;

typedef struct _AnalyseMoveTask {
    Task task;
    moverecord *pmr;
    listOLD *plGame;
    statcontext *psc;
    matchstate ms;
} AnalyseMoveTask;

typedef struct _ThreadLocalData {
    int id;
    move *aMoves;
    NNState *pnnState;
} ThreadLocalData;

#if defined(GLIB_THREADS)
typedef struct _ManualEvent {
#if GLIB_CHECK_VERSION (2,32,0)
    GCond cond;
#else
    GCond *cond;
#endif
    int signalled;
} *ManualEvent;
typedef GPrivate *TLSItem;

#if GLIB_CHECK_VERSION (2,32,0)
typedef GMutex Mutex;
#else
typedef GMutex *Mutex;
#endif
#elif defined(WIN32)            /* GLIB_THREAD */
typedef HANDLE ManualEvent;
typedef DWORD TLSItem;
typedef HANDLE Event;
typedef HANDLE Mutex;
#define WaitForManualEvent(ME) WaitForSingleObject(ME, INFINITE)
#define ResetManualEvent(ME) ResetEvent(ME)
#define SetManualEvent(ME) SetEvent(ME)
extern void TLSSetValue(TLSItem pItem, int value);

#if defined (DEBUG_MULTITHREADED)
extern void Mutex_Lock(Mutex mutex, const char *reason);
#else
#define Mutex_Lock(mutex, reason) WaitForSingleObject(mutex, INFINITE)
#define Mutex_Release(mutex) ReleaseMutex(mutex)
#endif

#endif

#if defined(USE_MULTITHREAD)

typedef struct _ThreadData {
    int doneTasks;
    GList *tasks;
    int result;
    ThreadLocalData *tld;

    ManualEvent activity;
    TLSItem tlsItem;
    Mutex queueLock;
    Mutex multiLock;
    ManualEvent syncStart;
    ManualEvent syncEnd;

    int addedTasks;
    int totalTasks;

    int closingThreads;
    unsigned int numThreads;
} ThreadData;
#else
typedef struct _ThreadData {
    int doneTasks;
    GList *tasks;
    int result;
    ThreadLocalData *tld;
} ThreadData;
#endif

extern int MT_GetDoneTasks(void);
extern void MT_AbortTasks(void);
extern void MT_AddTask(Task * pt, gboolean lock);
extern void mt_add_tasks(unsigned int num_tasks, AsyncFun pFun, void *taskData, gpointer linked);
extern int MT_WaitForTasks(gboolean(*pCallback) (gpointer), int callbackTime, int autosave);
extern void MT_InitThreads(void);
extern void MT_Close(void);
extern void MT_CloseThreads(void);
extern void CloseThread(void *unused);
extern ThreadLocalData *MT_CreateThreadLocalData(int id);

extern ThreadData td;

#if defined(USE_MULTITHREAD)

#if defined(GLIB_THREADS)
extern void ResetManualEvent(ManualEvent ME);
extern void Mutex_Lock(Mutex *mutex, const char *reason);
extern void Mutex_Release(Mutex *mutex);
extern void WaitForManualEvent(ManualEvent ME);
extern void SetManualEvent(ManualEvent ME);
extern void TLSSetValue(TLSItem pItem, size_t value);
#endif
extern void TLSFree(TLSItem pItem);
extern void InitManualEvent(ManualEvent * pME);
extern void FreeManualEvent(ManualEvent ME);
extern void InitMutex(Mutex * pMutex);
extern void FreeMutex(Mutex * mutex);

#define UI_UPDATETIME 250

#if defined (GLIB_THREADS)
#define TLSGet(item) *((size_t*)g_private_get(item))

#else                           /* WIN32 */
#define TLSGet(item) *((int*)TlsGetValue(item))
#endif                          /* GLIB_THREADS */

#if !defined(MAX_NUMTHREADS)
#define MAX_NUMTHREADS 48
#endif


extern void MT_Release(void);
extern void MT_Exclusive(void);
extern void MT_StartThreads(void);
extern void MT_SetNumThreads(unsigned int num);
extern void MT_SyncInit(void);
extern void MT_SyncStart(void);
extern double MT_SyncEnd(void);
extern void MT_SetResultFailed(void);
extern void TLSCreate(TLSItem * pItem);
extern unsigned int MT_GetNumThreads(void);

#define MT_GetTLD() ((ThreadLocalData *)TLSGet(td.tlsItem))
#define MT_GetThreadID() ((ThreadLocalData *)TLSGet(td.tlsItem))->id
#define MT_Get_nnState() ((ThreadLocalData *)TLSGet(td.tlsItem))->pnnState
#define MT_Get_aMoves() ((ThreadLocalData *)TLSGet(td.tlsItem))->aMoves

#if defined (GLIB_THREADS)
#if GLIB_CHECK_VERSION (2,30,0)
#define MT_SafeIncValue(x) (g_atomic_int_add(x, 1) + 1)
#define MT_SafeIncCheck(x) (g_atomic_int_add(x, 1))
#else
#define MT_SafeIncValue(x) (g_atomic_int_exchange_and_add(x, 1) + 1)
#define MT_SafeIncCheck(x) (g_atomic_int_exchange_and_add(x, 1))
#endif
/*
 * We don't use the value returned by the next three
 * Pre-2.30 void atomic_int_add() is ok
 */
#define MT_SafeInc(x) g_atomic_int_add(x, 1)
#define MT_SafeAdd(x, y) g_atomic_int_add(x, y)
#define MT_SafeDec(x) g_atomic_int_add(x, -1)
#define MT_SafeDecCheck(x) g_atomic_int_dec_and_test(x)
#else
#define MT_SafeInc(x) (void)InterlockedIncrement((long*)x)
#define MT_SafeIncValue(x) InterlockedIncrement((long*)x)
#define MT_SafeIncCheck(x) (InterlockedIncrement((long*)x) > 1)
#define MT_SafeAdd(x, y) InterlockedExchangeAdd((long*)x, y)
#define MT_SafeDec(x) (void)InterlockedDecrement((long*)x)
#define MT_SafeDecCheck(x) (InterlockedDecrement((long*)x) == 0)
#endif

#else                           /*USE_MULTITHREAD */
#if !defined(MAX_NUMTHREADS)
#define MAX_NUMTHREADS 1
#endif
extern int asyncRet;
#define MT_Exclusive() {}
#define MT_Release() {}
#define MT_GetNumThreads() 1
#define MT_SetResultFailed() asyncRet = -1
#define MT_SafeInc(x) (++(*x))
#define MT_SafeIncValue(x) (++(*x))
#define MT_SafeIncCheck(x) ((*x)++)
#define MT_SafeAdd(x, y) ((*x) += y)
#define MT_SafeDec(x) (--(*x))
#define MT_SafeDecCheck(x) ((--(*x)) == 0)
#define MT_GetThreadID() 0
#define MT_Get_nnState() td.tld->pnnState
#define MT_Get_aMoves() td.tld->aMoves
#define MT_GetTLD() td.tld

#endif

#endif
