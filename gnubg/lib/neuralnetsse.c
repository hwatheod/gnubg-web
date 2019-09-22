/*
 * neuralnetsse.c
 * by Jon Kinsey, 2006
 *
 * SSE (Intel) specific code
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
 * $Id: neuralnetsse.c,v 1.29 2014/11/23 19:45:06 plm Exp $
 */

#include "config.h"
#include "common.h"

#if USE_SIMD_INSTRUCTIONS

#define DEBUG_SSE 0

#include "simd.h"
#include "neuralnet.h"
#include <string.h>

#if defined(USE_AVX)
#include <immintrin.h>
#elif defined(USE_SSE2)
#include <emmintrin.h>
#else
#include <xmmintrin.h>
#endif

#include <glib.h>
#include "sigmoid.h"

float *
sse_malloc(size_t size)
{
    return (float *) _mm_malloc(size, ALIGN_SIZE);
}

void
sse_free(float *ptr)
{
    _mm_free(ptr);
}

#if defined(USE_AVX) || defined(USE_SSE2)
#include <stdint.h>

static const union {
    float f[VEC_SIZE];
    float_vector ps;
#if defined(USE_AVX)
} ones = { {
1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}};
#else
} ones = { {
1.0f, 1.0f, 1.0f, 1.0f}};
#endif

static const union {
    float f[VEC_SIZE];
    float_vector ps;
#if defined(USE_AVX)
} tens = { {
10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f}};
#else
} tens = { {
10.0f, 10.0f, 10.0f, 10.0f}};
#endif

static const union {
    int32_t i32[VEC_SIZE];
    float_vector ps;
#if defined(USE_AVX)
} abs_mask = { {
0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF}};
#else
} abs_mask = { {
0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF}};
#endif

static inline float_vector
sigmoid_positive_ps(float_vector xin)
{
    union {
        int_vector i;
        int32_t i32[VEC_SIZE];
    } i;
    float_vector ex;
    float *ex_elem = (float *) &ex;
#if defined(USE_AVX)
    float_vector x1 = _mm256_min_ps(xin, tens.ps);
#else
    float_vector x1 = _mm_min_ps(xin, tens.ps);
#endif

#if defined(USE_AVX)
    x1 = _mm256_mul_ps(x1, tens.ps);
    i.i = _mm256_cvttps_epi32(x1);
#else
    x1 = _mm_mul_ps(x1, tens.ps);
    i.i = _mm_cvttps_epi32(x1);
#endif
    ex_elem[0] = e[i.i32[0]];
    ex_elem[1] = e[i.i32[1]];
    ex_elem[2] = e[i.i32[2]];
    ex_elem[3] = e[i.i32[3]];
#if defined(USE_AVX)
    ex_elem[4] = e[i.i32[4]];
    ex_elem[5] = e[i.i32[5]];
    ex_elem[6] = e[i.i32[6]];
    ex_elem[7] = e[i.i32[7]];
#endif

#if defined(USE_AVX)
    x1 = _mm256_sub_ps(x1, _mm256_cvtepi32_ps(i.i));
    x1 = _mm256_add_ps(x1, tens.ps);
    x1 = _mm256_mul_ps(x1, ex);
    x1 = _mm256_add_ps(x1, ones.ps);
#ifdef __FAST_MATH__
    return _mm256_rcp_ps(x1);
#else
    return _mm256_div_ps(ones.ps, x1);
#endif
#else
    x1 = _mm_sub_ps(x1, _mm_cvtepi32_ps(i.i));
    x1 = _mm_add_ps(x1, tens.ps);
    x1 = _mm_mul_ps(x1, ex);
    x1 = _mm_add_ps(x1, ones.ps);
#ifdef __FAST_MATH__
    return _mm_rcp_ps(x1);
#else
    return _mm_div_ps(ones.ps, x1);
#endif
#endif
}

static inline float_vector
sigmoid_ps(float_vector xin)
{
#if defined(USE_AVX)
    float_vector mask = _mm256_cmp_ps(xin, _mm256_setzero_ps(), _CMP_LT_OS);
    float_vector c;
    xin = _mm256_and_ps(xin, abs_mask.ps);      /* Abs. value by clearing signbit */
    c = sigmoid_positive_ps(xin);
    return _mm256_or_ps(_mm256_and_ps(mask, c), _mm256_andnot_ps(mask, _mm256_sub_ps(ones.ps, c)));
#else
    float_vector mask = _mm_cmplt_ps(xin, _mm_setzero_ps());
    float_vector c;
    xin = _mm_and_ps(xin, abs_mask.ps); /* Abs. value by clearing signbit */
    c = sigmoid_positive_ps(xin);
    return _mm_or_ps(_mm_and_ps(mask, c), _mm_andnot_ps(mask, _mm_sub_ps(ones.ps, c)));
#endif
}

#endif                          // USE_SSE2 or USE_AVX

#if defined(USE_SSE2)
#define INPUT_ADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = _mm_load_ps(pr); \
    vec1 = _mm_load_ps(prWeight); \
    sum = _mm_add_ps(vec0, vec1); \
    _mm_store_ps(pr, sum); \
}
#define INPUT_MULTADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = _mm_load_ps(pr); \
    vec1 = _mm_load_ps(prWeight); \
    vec3 = _mm_mul_ps(vec1, scalevec); \
    sum = _mm_add_ps(vec0, vec3); \
    _mm_store_ps(pr, sum); \
}
#endif
#if defined(USE_AVX)
#define INPUT_ADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = _mm256_load_ps(pr); \
    vec1 = _mm256_load_ps(prWeight); \
    sum = _mm256_add_ps(vec0, vec1); \
    _mm256_store_ps(pr, sum); \
}
#define INPUT_MULTADD() \
for (j = (cHidden >> LOG2VEC_SIZE); j; j--, pr += VEC_SIZE, prWeight += VEC_SIZE) { \
    vec0 = _mm256_load_ps(pr); \
    vec1 = _mm256_load_ps(prWeight); \
    vec3 = _mm256_mul_ps(vec1, scalevec); \
    sum = _mm256_add_ps(vec0, vec3); \
    _mm256_store_ps(pr, sum); \
}
#endif

static void
EvaluateSSE(const neuralnet * pnn, const float arInput[], float ar[], float arOutput[])
{

    const unsigned int cHidden = pnn->cHidden;
    unsigned int i, j;
    float *prWeight;
#if defined(USE_SSE2) || defined(USE_AVX)
    float *par;
#endif
    float_vector vec0, vec1, vec3, scalevec, sum;

    /* Calculate activity at hidden nodes */
    memcpy(ar, pnn->arHiddenThreshold, cHidden * sizeof(float));

    prWeight = pnn->arHiddenWeight;

    if (pnn->cInput != 214) {   /* everything but the racing net */
        for (i = 0; i < 200;) { /* base inputs */
            float ari = arInput[i++];

            /* 3 binaries, 1 float */

            if (ari == 0.0f)
                prWeight += cHidden;
            else {
                float *pr = ar;
                INPUT_ADD();
            }

            ari = arInput[i++];

            if (ari == 0.0f)
                prWeight += cHidden;
            else {
                float *pr = ar;
                INPUT_ADD();
            }

            ari = arInput[i++];

            if (ari == 0.0f) {
                prWeight += cHidden;
                /* If 3rd element is 0, so is 4th. Skip it */
                prWeight += cHidden;
                i++;
                continue;
            } else {
                float *pr = ar;
                INPUT_ADD();
            }

            ari = arInput[i++];

            if (ari == 0.0f)
                prWeight += cHidden;
            else {
                float *pr = ar;

                if (ari == 1.0f) {
                    INPUT_ADD();
                } else {
#if defined(USE_AVX)
                    scalevec = _mm256_set1_ps(ari);
#else
                    scalevec = _mm_set1_ps(ari);
#endif
                    INPUT_MULTADD();
                }
            }                   /* base inputs are done */
        }

        if (pnn->cInput == 250) /* Pruning nets are over, contact/crashed still have 2 * 25 floats */
            for (i = 200; i < 250; i++) {
                float const ari = arInput[i];

                if (ari == 0.0f)
                    prWeight += cHidden;
                else {
                    float *pr = ar;

#if defined(USE_AVX)
                    scalevec = _mm256_set1_ps(ari);
#else
                    scalevec = _mm_set1_ps(ari);
#endif
                    INPUT_MULTADD();
                }
            }
    }

    else                        /* racing net */
        for (i = 0; i < pnn->cInput; i++) {
            float const ari = arInput[i];

            if (ari == 0.0f)
                prWeight += cHidden;
            else {
                float *pr = ar;
                if (ari == 1.0f) {
                    INPUT_ADD();
                } else {
#if defined(USE_AVX)
                    scalevec = _mm256_set1_ps(ari);
#else
                    scalevec = _mm_set1_ps(ari);
#endif
                    INPUT_MULTADD();
                }
            }
        }

#if defined(USE_SSE2) || defined(USE_AVX)
#if defined(USE_AVX)
    scalevec = _mm256_set1_ps(pnn->rBetaHidden);
#else
    scalevec = _mm_set1_ps(pnn->rBetaHidden);
#endif
    for (par = ar, i = (cHidden >> LOG2VEC_SIZE); i; i--, par += VEC_SIZE) {
#if defined(USE_AVX)
        float_vector vec = _mm256_load_ps(par);
        vec = _mm256_mul_ps(vec, scalevec);
        vec = sigmoid_ps(vec);
        _mm256_store_ps(par, vec);
#else
        float_vector vec = _mm_load_ps(par);
        vec = _mm_mul_ps(vec, scalevec);
        vec = sigmoid_ps(vec);
        _mm_store_ps(par, vec);
#endif
    }
#else
    for (i = 0; i < cHidden; i++)
        ar[i] = sigmoid(-pnn->rBetaHidden * ar[i]);
#endif

    /* Calculate activity at output nodes */
    prWeight = pnn->arOutputWeight;

    for (i = 0; i < pnn->cOutput; i++) {

#if defined(USE_AVX)
        SSE_ALIGN(float r[8]);
#else
        float r;
#endif
        float *pr = ar;
#if defined(USE_AVX)
        sum = _mm256_setzero_ps();
#else
        sum = _mm_setzero_ps();
#endif
        for (j = (cHidden >> LOG2VEC_SIZE); j; j--, prWeight += VEC_SIZE, pr += VEC_SIZE) {
#if defined(USE_AVX)
            vec0 = _mm256_load_ps(pr);  /* Eight floats into vec0 */
            vec1 = _mm256_load_ps(prWeight);    /* Eight weights into vec1 */
            vec3 = _mm256_mul_ps(vec0, vec1);   /* Multiply */
            sum = _mm256_add_ps(sum, vec3);     /* Add */
#else
            vec0 = _mm_load_ps(pr);     /* Four floats into vec0 */
            vec1 = _mm_load_ps(prWeight);       /* Four weights into vec1 */
            vec3 = _mm_mul_ps(vec0, vec1);      /* Multiply */
            sum = _mm_add_ps(sum, vec3);        /* Add */
#endif
        }

#if defined(USE_AVX)
        vec0 = _mm256_hadd_ps(sum, sum);
        vec1 = _mm256_hadd_ps(vec0, vec0);
        _mm256_store_ps(r, vec1);

        _mm256_zeroupper();

        arOutput[i] = sigmoid(-pnn->rBetaOutput * (r[0] + r[4] + pnn->arOutputThreshold[i]));

#else
        vec0 = _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(2, 3, 0, 1));
        vec1 = _mm_add_ps(sum, vec0);
        vec0 = _mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(1, 1, 3, 3));
        sum = _mm_add_ps(vec1, vec0);
        _mm_store_ss(&r, sum);

        arOutput[i] = sigmoid(-pnn->rBetaOutput * (r + pnn->arOutputThreshold[i]));
#endif
    }
}


extern int
NeuralNetEvaluateSSE(const neuralnet * pnn, /*lint -e{818} */ float arInput[],
                     float arOutput[], NNState * UNUSED(pnState))
{
    SSE_ALIGN(float ar[pnn->cHidden]);

#if DEBUG_SSE
    g_assert(sse_aligned(arOutput));
    g_assert(sse_aligned(ar));
    g_assert(sse_aligned(arInput));
#endif

    EvaluateSSE(pnn, arInput, ar, arOutput);
    return 0;
}

#endif
