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
 * cache.h
 *
 * by Gary Wong, 1997-2000
 * $Id: cache.h,v 1.24 2014/07/29 15:21:48 plm Exp $
 */

#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>

#include "config.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
typedef unsigned int uint32_t;
#endif

#include "gnubg-types.h"

/* Set to calculate simple cache stats */
#define CACHE_STATS 0

typedef struct _cacheNodeDetail {
    positionkey key;
    int nEvalContext;
    float ar[6];
} cacheNodeDetail;

typedef struct _cacheNode {
    cacheNodeDetail nd_primary;
    cacheNodeDetail nd_secondary;
#if USE_MULTITHREAD
    volatile int lock;
#endif
} cacheNode;

/* name used in eval.c */
typedef cacheNodeDetail evalcache;

typedef struct _cache {
    cacheNode *entries;

    unsigned int size;
    uint32_t hashMask;

#if CACHE_STATS
    unsigned int nAdds;
    unsigned int cLookup;
    unsigned int cHit;
#endif
} evalCache;

/* Cache size will be adjusted to a power of 2 */
int CacheCreate(evalCache * pc, unsigned int size);
int CacheResize(evalCache * pc, unsigned int cNew);

#define CACHEHIT ((uint32_t)-1)
/* returns a value which is passed to CacheAdd (if a miss) */
unsigned int CacheLookupWithLocking(evalCache * pc, const cacheNodeDetail * e, float *arOut, float *arCubeful);
unsigned int CacheLookupNoLocking(evalCache * pc, const cacheNodeDetail * e, float *arOut, float *arCubeful);

void CacheAddWithLocking(evalCache * pc, const cacheNodeDetail * e, uint32_t l);
static inline void
CacheAddNoLocking(evalCache * pc, const cacheNodeDetail * e, const uint32_t l)
{
    pc->entries[l].nd_secondary = pc->entries[l].nd_primary;
    pc->entries[l].nd_primary = *e;
#if CACHE_STATS
    ++pc->nAdds;
#endif
}

void CacheFlush(const evalCache * pc);
void CacheDestroy(const evalCache * pc);
void CacheStats(const evalCache * pc, unsigned int *pcLookup, unsigned int *pcHit, unsigned int *pcUsed);

uint32_t GetHashKey(uint32_t hashMask, const cacheNodeDetail * e);

#endif
