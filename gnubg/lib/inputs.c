/*
 * Subset of inputs computed with SSE
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
 * $Id: inputs.c,v 1.9 2014/07/20 21:18:29 plm Exp $
 */

#include "config.h"
#include "gnubg-types.h"
#include "simd.h"
#include "eval.h"

#if USE_SIMD_INSTRUCTIONS
#if defined(USE_AVX)
#include <immintrin.h>
#elif defined(USE_SSE2)
#include <emmintrin.h>
#else
#include <xmmintrin.h>
#endif
#else
typedef float float_vector[4];
#endif /* USE_SIMD_INSTRUCTIONS */

typedef SSE_ALIGN(float float_vec_aligned[sizeof(float_vector)/sizeof(float)]);

SSE_ALIGN (static float_vec_aligned inpvec[16]) = {
    /*  0 */  {
    0.0, 0.0, 0.0, 0.0},
        /*  1 */  {
    1.0, 0.0, 0.0, 0.0},
        /*  2 */  {
    0.0, 1.0, 0.0, 0.0},
        /*  3 */  {
    0.0, 0.0, 1.0, 0.0},
        /*  4 */  {
    0.0, 0.0, 1.0, 0.5},
        /*  5 */  {
    0.0, 0.0, 1.0, 1.0},
        /*  6 */  {
    0.0, 0.0, 1.0, 1.5},
        /*  7 */  {
    0.0, 0.0, 1.0, 2.0},
        /*  8 */  {
    0.0, 0.0, 1.0, 2.5},
        /*  9 */  {
    0.0, 0.0, 1.0, 3.0},
        /* 10 */  {
    0.0, 0.0, 1.0, 3.5},
        /* 11 */  {
    0.0, 0.0, 1.0, 4.0},
        /* 12 */  {
    0.0, 0.0, 1.0, 4.5},
        /* 13 */  {
    0.0, 0.0, 1.0, 5.0},
        /* 14 */  {
    0.0, 0.0, 1.0, 5.5},
        /* 15 */  {
0.0, 0.0, 1.0, 6.0}};

SSE_ALIGN(static float_vec_aligned inpvecb[16]) = {
    /*  0 */  {
    0.0, 0.0, 0.0, 0.0},
        /*  1 */  {
    1.0, 0.0, 0.0, 0.0},
        /*  2 */  {
    1.0, 1.0, 0.0, 0.0},
        /*  3 */  {
    1.0, 1.0, 1.0, 0.0},
        /*  4 */  {
    1.0, 1.0, 1.0, 0.5},
        /*  5 */  {
    1.0, 1.0, 1.0, 1.0},
        /*  6 */  {
    1.0, 1.0, 1.0, 1.5},
        /*  7 */  {
    1.0, 1.0, 1.0, 2.0},
        /*  8 */  {
    1.0, 1.0, 1.0, 2.5},
        /*  9 */  {
    1.0, 1.0, 1.0, 3.0},
        /* 10 */  {
    1.0, 1.0, 1.0, 3.5},
        /* 11 */  {
    1.0, 1.0, 1.0, 4.0},
        /* 12 */  {
    1.0, 1.0, 1.0, 4.5},
        /* 13 */  {
    1.0, 1.0, 1.0, 5.0},
        /* 14 */  {
    1.0, 1.0, 1.0, 5.5},
        /* 15 */  {
1.0, 1.0, 1.0, 6.0}};

#if USE_SIMD_INSTRUCTIONS
extern SIMD_AVX_STACKALIGN void
baseInputs(const TanBoard anBoard, float arInput[])
{
    int i = 3;

    const unsigned int *pB = &anBoard[0][0];
    float *pInput = &arInput[0];
    register __m128 vec0;
    register __m128 vec1;
    register __m128 vec2;
    register __m128 vec3;
    register __m128 vec4;
    register __m128 vec5;
    register __m128 vec6;
    register __m128 vec7;

    while (i--) {
        vec0 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput, vec0);
        vec1 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec1);
        vec2 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec2);
        vec3 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec3);
        vec4 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec4);
        vec5 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec5);
        vec6 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec6);
        vec7 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec7);
        pInput += 4;
    }

    /* bar */
    vec0 = _mm_load_ps(inpvecb[*pB++]);
    _mm_store_ps(pInput, vec0);
    pInput += 4;

    i = 3;
    while (i--) {
        vec0 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput, vec0);
        vec1 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec1);
        vec2 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec2);
        vec3 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec3);
        vec4 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec4);
        vec5 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec5);
        vec6 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec6);
        vec7 = _mm_load_ps(inpvec[*pB++]);
        _mm_store_ps(pInput += 4, vec7);
        pInput += 4;
    }

    /* bar */
    vec0 = _mm_load_ps(inpvecb[*pB]);
    _mm_store_ps(pInput, vec0);

#if defined(USE_AVX)
    _mm256_zeroupper();
#endif

    return;
}
#else
extern void
baseInputs(const TanBoard anBoard, float arInput[])
{
    int j, i;

    for (j = 0; j < 2; ++j) {
        float *afInput = arInput + j * 25 * 4;
        const unsigned int *board = anBoard[j];

        /* Points */
        for (i = 0; i < 24; i++) {
            int nc = board[i];

            afInput[i * 4 + 0] = inpvec[nc][0];
            afInput[i * 4 + 1] = inpvec[nc][1];
            afInput[i * 4 + 2] = inpvec[nc][2];
            afInput[i * 4 + 3] = inpvec[nc][3];
        }

        /* Bar */
        {
            int nc = board[24];

            afInput[24 * 4 + 0] = inpvecb[nc][0];
            afInput[24 * 4 + 1] = inpvecb[nc][1];
            afInput[24 * 4 + 2] = inpvecb[nc][2];
            afInput[24 * 4 + 3] = inpvecb[nc][3];
        }
    }
}
#endif
