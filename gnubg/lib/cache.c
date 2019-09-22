/* This program is free software; you can redistribute it and/or modify
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
 *
 * cache.c
 *
 * by Gary Wong, 1997-2000
 * $Id: cache.c,v 1.39 2015/01/11 15:59:39 plm Exp $
 */

#include "config.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <stdio.h>
#endif

#include "cache.h"
#include "positionid.h"

#if USE_MULTITHREAD
#include "multithread.h"

#if defined(__GNUC__) && ( __GNUC__ * 100 + __GNUC_MINOR__ >= 401 ) \
  && (defined (__i386) || defined (__x86_64))

#define cache_lock(pc, k) \
    while (__sync_lock_test_and_set(&(pc->entries[k].lock), 1)) \
         while (pc->entries[k].lock) \
            __asm volatile ("pause" ::: "memory")

#define cache_unlock(pc, k) \
    __sync_lock_release(&(pc->entries[k].lock));

#else

#define cache_lock(pc, k) \
if (MT_SafeIncCheck(&(pc->entries[k].lock))) \
	WaitForLock(&(pc->entries[k].lock))

#define cache_unlock(pc, k) MT_SafeDec(&(pc->entries[k].lock))

static void
WaitForLock(volatile int *lock)
{
    do {
        MT_SafeDec(lock);
    } while (MT_SafeIncCheck(lock));
}

#endif

#endif                          /* USE_MULTITHREAD */


int
CacheCreate(evalCache * pc, unsigned int s)
{
#if CACHE_STATS
    pc->cLookup = 0;
    pc->cHit = 0;
    pc->nAdds = 0;
#endif

    if (s > 1u << 31)
        return -1;

    pc->size = s;
    /* adjust size to smallest power of 2 GE to s */
    while ((s & (s - 1)) != 0)
        s &= (s - 1);

    pc->size = (s < pc->size) ? 2 * s : s;
    pc->hashMask = (pc->size >> 1) - 1;

    pc->entries = (cacheNode *) malloc((pc->size / 2) * sizeof(*pc->entries));
    if (pc->entries == 0)
        return -1;

    CacheFlush(pc);
    return 0;
}

/* MurmurHash3  https://code.google.com/p/smhasher/wiki/MurmurHash */

extern uint32_t
GetHashKey(uint32_t hashMask, const cacheNodeDetail * e)
{
    uint32_t hash = (uint32_t) e->nEvalContext;
    int i;

    hash *= 0xcc9e2d51;
    hash = (hash << 15) | (hash >> (32 - 15));
    hash *= 0x1b873593;

    hash = (hash << 13) | (hash >> (32 - 13));
    hash = hash * 5 + 0xe6546b64;

    for (i = 0; i < 7; i++) {
        uint32_t k = e->key.data[i];

        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> (32 - 15));
        k *= 0x1b873593;

        hash ^= k;
        hash = (hash << 13) | (hash >> (32 - 13));
        hash = hash * 5 + 0xe6546b64;
    }

    /* Real MurmurHash3 has a "hash ^= len" here,
     * but for us len is constant. Skip it */

    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;

    return (hash & hashMask);
}


uint32_t
CacheLookupWithLocking(evalCache * pc, const cacheNodeDetail * e, float *arOut, float *arCubeful)
{
    uint32_t const l = GetHashKey(pc->hashMask, e);

#if CACHE_STATS
    ++pc->cLookup;
#endif
#if USE_MULTITHREAD
    cache_lock(pc, l);
#endif
    if (!EqualKeys(pc->entries[l].nd_primary.key, e->key) || pc->entries[l].nd_primary.nEvalContext != e->nEvalContext) {       /* Not in primary slot */
        if (!EqualKeys(pc->entries[l].nd_secondary.key, e->key) || pc->entries[l].nd_secondary.nEvalContext != e->nEvalContext) {       /* Cache miss */
#if USE_MULTITHREAD
            cache_unlock(pc, l);
#endif
            return l;
        } else {                /* Found in second slot, promote "hot" entry */
            cacheNodeDetail tmp = pc->entries[l].nd_primary;

            pc->entries[l].nd_primary = pc->entries[l].nd_secondary;
            pc->entries[l].nd_secondary = tmp;
        }
    }
    /* Cache hit */
    memcpy(arOut, pc->entries[l].nd_primary.ar, sizeof(float) * 5 /*NUM_OUTPUTS */ );
    if (arCubeful)
        *arCubeful = pc->entries[l].nd_primary.ar[5];   /* Cubeful equity stored in slot 5 */

#if USE_MULTITHREAD
    cache_unlock(pc, l);
#endif

#if CACHE_STATS
    ++pc->cHit;
#endif

    return CACHEHIT;
}

uint32_t
CacheLookupNoLocking(evalCache * pc, const cacheNodeDetail * e, float *arOut, float *arCubeful)
{
    uint32_t const l = GetHashKey(pc->hashMask, e);

#if CACHE_STATS
    ++pc->cLookup;
#endif
    if (!EqualKeys(pc->entries[l].nd_primary.key, e->key) || pc->entries[l].nd_primary.nEvalContext != e->nEvalContext) {       /* Not in primary slot */
        if (!EqualKeys(pc->entries[l].nd_secondary.key, e->key) || pc->entries[l].nd_secondary.nEvalContext != e->nEvalContext) {       /* Cache miss */
            return l;
        } else {                /* Found in second slot, promote "hot" entry */
            cacheNodeDetail tmp = pc->entries[l].nd_primary;

            pc->entries[l].nd_primary = pc->entries[l].nd_secondary;
            pc->entries[l].nd_secondary = tmp;
        }
    }

    /* Cache hit */
    memcpy(arOut, pc->entries[l].nd_primary.ar, sizeof(float) * 5 /*NUM_OUTPUTS */ );
    if (arCubeful)
        *arCubeful = pc->entries[l].nd_primary.ar[5];   /* Cubeful equity stored in slot 5 */

#if CACHE_STATS
    ++pc->cHit;
#endif

    return CACHEHIT;
}

void
CacheAddWithLocking(evalCache * pc, const cacheNodeDetail * e, uint32_t l)
{
#if USE_MULTITHREAD
    cache_lock(pc, l);
#endif

    pc->entries[l].nd_secondary = pc->entries[l].nd_primary;
    pc->entries[l].nd_primary = *e;

#if USE_MULTITHREAD
    cache_unlock(pc, l);
#endif

#if CACHE_STATS
    ++pc->nAdds;
#endif
}

void
CacheDestroy(const evalCache * pc)
{
    free(pc->entries);
}

void
CacheFlush(const evalCache * pc)
{
    unsigned int k;
    for (k = 0; k < pc->size / 2; ++k) {
        pc->entries[k].nd_primary.key.data[0] = (unsigned int) -1;
        pc->entries[k].nd_secondary.key.data[0] = (unsigned int) -1;
#if USE_MULTITHREAD
        pc->entries[k].lock = 0;
#endif
    }
}

int
CacheResize(evalCache * pc, unsigned int cNew)
{
    if (cNew != pc->size) {
        CacheDestroy(pc);
        if (CacheCreate(pc, cNew) != 0)
            return -1;
    }

    return (int) pc->size;
}

void
CacheStats(const evalCache * pc, unsigned int *pcLookup, unsigned int *pcHit, unsigned int *pcUsed)
{
#if CACHE_STATS
    if (pcLookup)
        *pcLookup = pc->cLookup;

    if (pcHit)
        *pcHit = pc->cHit;

    if (pcUsed)
        *pcUsed = pc->nAdds;
#else
    (void) pc;                  /* suppress unused parameter compiler warning */

    if (pcLookup)
        *pcLookup = 0;

    if (pcHit)
        *pcHit = 0;

    if (pcUsed)
        *pcUsed = 0;
#endif
}
