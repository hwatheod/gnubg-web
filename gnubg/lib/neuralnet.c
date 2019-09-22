/*
 * neuralnet.c
 *
 * by Gary Wong <gtw@gnu.org>, 1998, 1999, 2000, 2001, 2002.
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
 * $Id: neuralnet.c,v 1.85 2014/11/23 20:17:48 plm Exp $
 */

#include "config.h"
#include "backgammon.h"
#include "common.h"
#include <glib.h>
#include <errno.h>
#include <isaac.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "neuralnet.h"
#include "simd.h"
#include "sigmoid.h"

extern int
NeuralNetCreate(neuralnet * pnn, unsigned int cInput, unsigned int cHidden,
                unsigned int cOutput, float rBetaHidden, float rBetaOutput)
{
    pnn->cInput = cInput;
    pnn->cHidden = cHidden;
    pnn->cOutput = cOutput;
    pnn->rBetaHidden = rBetaHidden;
    pnn->rBetaOutput = rBetaOutput;
    pnn->nTrained = 0;

    if ((pnn->arHiddenWeight = sse_malloc(cHidden * cInput * sizeof(float))) == NULL)
        return -1;

    if ((pnn->arOutputWeight = sse_malloc(cOutput * cHidden * sizeof(float))) == NULL) {
        sse_free(pnn->arHiddenWeight);
        return -1;
    }

    if ((pnn->arHiddenThreshold = sse_malloc(cHidden * sizeof(float))) == NULL) {
        sse_free(pnn->arOutputWeight);
        sse_free(pnn->arHiddenWeight);
        return -1;
    }

    if ((pnn->arOutputThreshold = sse_malloc(cOutput * sizeof(float))) == NULL) {
        sse_free(pnn->arHiddenThreshold);
        sse_free(pnn->arOutputWeight);
        sse_free(pnn->arHiddenWeight);
        return -1;
    }

    return 0;
}

extern void
NeuralNetDestroy(neuralnet * pnn)
{
    sse_free(pnn->arHiddenWeight);
    pnn->arHiddenWeight = 0;
    sse_free(pnn->arOutputWeight);
    pnn->arOutputWeight = 0;
    sse_free(pnn->arHiddenThreshold);
    pnn->arHiddenThreshold = 0;
    sse_free(pnn->arOutputThreshold);
    pnn->arOutputThreshold = 0;
}

#if !defined(USE_SIMD_INSTRUCTIONS)

/* separate context for race, crashed, contact
 * -1: regular eval
 * 0: save base
 * 1: from base
 */

static inline NNEvalType
NNevalAction(NNState * pnState)
{
    if (!pnState)
        return NNEVAL_NONE;

    switch (pnState->state) {
    case NNSTATE_NONE:
        {
            /* incremental evaluation not useful */
            return NNEVAL_NONE;
        }
    case NNSTATE_INCREMENTAL:
        {
            /* next call should return FROMBASE */
            pnState->state = NNSTATE_DONE;

            /* starting a new context; save base in the hope it will be useful */
            return NNEVAL_SAVE;
        }
    case NNSTATE_DONE:
        {
            /* context hit!  use the previously computed base */
            return NNEVAL_FROMBASE;
        }
    }
    /* never reached */
    return NNEVAL_NONE;         /* for the picky compiler */
}

static void
Evaluate(const neuralnet * pnn, const float arInput[], float ar[], float arOutput[], float *saveAr)
{
    const unsigned int cHidden = pnn->cHidden;
    unsigned int i, j;
    float *prWeight;

    /* Calculate activity at hidden nodes */
    for (i = 0; i < cHidden; i++)
        ar[i] = pnn->arHiddenThreshold[i];

    prWeight = pnn->arHiddenWeight;

    for (i = 0; i < pnn->cInput; i++) {
        float const ari = arInput[i];

        if (ari == 0.0f)
            prWeight += cHidden;
        else {
            float *pr = ar;

            if (ari == 1.0f)
                for (j = cHidden; j; j--)
                    *pr++ += *prWeight++;
            else
                for (j = cHidden; j; j--)
                    *pr++ += *prWeight++ * ari;
        }
    }

    if (saveAr)
        memcpy(saveAr, ar, cHidden * sizeof(*saveAr));

    for (i = 0; i < cHidden; i++)
        ar[i] = sigmoid(-pnn->rBetaHidden * ar[i]);

    /* Calculate activity at output nodes */
    prWeight = pnn->arOutputWeight;

    for (i = 0; i < pnn->cOutput; i++) {
        float r = pnn->arOutputThreshold[i];

        for (j = 0; j < cHidden; j++)
            r += ar[j] * *prWeight++;

        arOutput[i] = sigmoid(-pnn->rBetaOutput * r);
    }
}

static void
EvaluateFromBase(const neuralnet * pnn, const float arInputDif[], float ar[], float arOutput[])
{
    unsigned int i, j;
    float *prWeight;

    /* Calculate activity at hidden nodes */
    /*    for( i = 0; i < pnn->cHidden; i++ )
     * ar[ i ] = pnn->arHiddenThreshold[ i ]; */

    prWeight = pnn->arHiddenWeight;

    for (i = 0; i < pnn->cInput; ++i) {
        float const ari = arInputDif[i];

        if (ari == 0.0f)
            prWeight += pnn->cHidden;
        else {
            float *pr = ar;

            if (ari == 1.0f)
                for (j = pnn->cHidden; j; j--)
                    *pr++ += *prWeight++;
            else if (ari == -1.0f)
                for (j = pnn->cHidden; j; j--)
                    *pr++ -= *prWeight++;
            else
                for (j = pnn->cHidden; j; j--)
                    *pr++ += *prWeight++ * ari;
        }
    }

    for (i = 0; i < pnn->cHidden; i++)
        ar[i] = sigmoid(-pnn->rBetaHidden * ar[i]);

    /* Calculate activity at output nodes */
    prWeight = pnn->arOutputWeight;

    for (i = 0; i < pnn->cOutput; i++) {
        float r = pnn->arOutputThreshold[i];

        for (j = 0; j < pnn->cHidden; j++)
            r += ar[j] * *prWeight++;

        arOutput[i] = sigmoid(-pnn->rBetaOutput * r);
    }
}

extern int
NeuralNetEvaluate(const neuralnet * pnn, float arInput[], float arOutput[], NNState * pnState)
{
    float *ar = (float *) g_alloca(pnn->cHidden * sizeof(float));
    switch (NNevalAction(pnState)) {
    case NNEVAL_NONE:
        {
            Evaluate(pnn, arInput, ar, arOutput, 0);
            break;
        }
    case NNEVAL_SAVE:
        {
            pnState->cSavedIBase = pnn->cInput;
            memcpy(pnState->savedIBase, arInput, pnn->cInput * sizeof(*ar));
            Evaluate(pnn, arInput, ar, arOutput, pnState->savedBase);
            break;
        }
    case NNEVAL_FROMBASE:
        {
            unsigned int i;
            if (pnState->cSavedIBase != (int) pnn->cInput) {
                Evaluate(pnn, arInput, ar, arOutput, 0);
                break;
            }
            memcpy(ar, pnState->savedBase, pnn->cHidden * sizeof(*ar));

            {
                float *r = arInput;
                float *s = pnState->savedIBase;

                for (i = 0; i < pnn->cInput; ++i, ++r, ++s) {
                    if (*r != *s /*lint --e(777) */ ) {
                        *r -= *s;
                    } else {
                        *r = 0.0;
                    }
                }
            }
            EvaluateFromBase(pnn, arInput, ar, arOutput);
            break;
        }
    }
    return 0;
}
#endif

extern int
NeuralNetLoad(neuralnet * pnn, FILE * pf)
{

    unsigned int i;
    float *pr;
    char dummy[16];

    if (fscanf(pf, "%u %u %u %s %f %f\n", &pnn->cInput, &pnn->cHidden,
               &pnn->cOutput, dummy, &pnn->rBetaHidden,
               &pnn->rBetaOutput) < 5 || pnn->cInput < 1 ||
        pnn->cHidden < 1 || pnn->cOutput < 1 || pnn->rBetaHidden <= 0.0f || pnn->rBetaOutput <= 0.0f) {
        errno = EINVAL;
        return -1;
    }

    if (NeuralNetCreate(pnn, pnn->cInput, pnn->cHidden, pnn->cOutput, pnn->rBetaHidden, pnn->rBetaOutput))
        return -1;

    pnn->nTrained = 1;

    for (i = pnn->cInput * pnn->cHidden, pr = pnn->arHiddenWeight; i; i--)
        if (fscanf(pf, "%f\n", pr++) < 1)
            return -1;

    for (i = pnn->cHidden * pnn->cOutput, pr = pnn->arOutputWeight; i; i--)
        if (fscanf(pf, "%f\n", pr++) < 1)
            return -1;

    for (i = pnn->cHidden, pr = pnn->arHiddenThreshold; i; i--)
        if (fscanf(pf, "%f\n", pr++) < 1)
            return -1;

    for (i = pnn->cOutput, pr = pnn->arOutputThreshold; i; i--)
        if (fscanf(pf, "%f\n", pr++) < 1)
            return -1;

    return 0;
}

extern int
NeuralNetLoadBinary(neuralnet * pnn, FILE * pf)
{

    int dummy;

#define FREAD( p, c ) \
    if( fread( (p), sizeof( *(p) ), (c), pf ) < (unsigned int)(c) ) return -1;

    FREAD(&pnn->cInput, 1);
    FREAD(&pnn->cHidden, 1);
    FREAD(&pnn->cOutput, 1);
    FREAD(&dummy, 1);
    FREAD(&pnn->rBetaHidden, 1);
    FREAD(&pnn->rBetaOutput, 1);

    if (pnn->cInput < 1 || pnn->cHidden < 1 || pnn->cOutput < 1 || pnn->rBetaHidden <= 0.0f || pnn->rBetaOutput <= 0.0f) {
        errno = EINVAL;
        return -1;
    }

    if (NeuralNetCreate(pnn, pnn->cInput, pnn->cHidden, pnn->cOutput, pnn->rBetaHidden, pnn->rBetaOutput))
        return -1;

    pnn->nTrained = 1;

    FREAD(pnn->arHiddenWeight, pnn->cInput * pnn->cHidden);
    FREAD(pnn->arOutputWeight, pnn->cHidden * pnn->cOutput);
    FREAD(pnn->arHiddenThreshold, pnn->cHidden);
    FREAD(pnn->arOutputThreshold, pnn->cOutput);
#undef FREAD

    return 0;
}

extern int
NeuralNetSaveBinary(const neuralnet * pnn, FILE * pf)
{

#define FWRITE( p, c ) \
    if( fwrite( (p), sizeof( *(p) ), (c), pf ) < (unsigned int)(c) ) return -1;

    FWRITE(&pnn->cInput, 1);
    FWRITE(&pnn->cHidden, 1);
    FWRITE(&pnn->cOutput, 1);
    FWRITE(&pnn->nTrained, 1);
    FWRITE(&pnn->rBetaHidden, 1);
    FWRITE(&pnn->rBetaOutput, 1);

    FWRITE(pnn->arHiddenWeight, pnn->cInput * pnn->cHidden);
    FWRITE(pnn->arOutputWeight, pnn->cHidden * pnn->cOutput);
    FWRITE(pnn->arHiddenThreshold, pnn->cHidden);
    FWRITE(pnn->arOutputThreshold, pnn->cOutput);
#undef FWRITE

    return 0;
}


#if defined(USE_SIMD_INSTRUCTIONS)

#if defined(DISABLE_SIMD_TEST)

int
SIMD_Supported(void)
{
    return 1;
}

#else

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h>
#endif

static int
check_for_cpuid(void)
{
    int result;

    asm volatile(
#if defined(ENVIRONMENT32) && defined(__PIC__)
        /* We have to be careful to not destroy ebx if using PIC on 32bit builds */
        "pushl %%ebx\n\t"
#endif
        "mov $1, %%eax\n\t"
        "shl $21, %%eax\n\t"
        "mov %%eax, %%edx\n\t"
#if defined(ENVIRONMENT64)
        "pushf\n\t"
        "pop %%rax\n\t"
#else
        "pushfl\n\t"
        "popl %%eax\n\t"
#endif
        "mov %%eax, %%ecx\n\t"
        "xor %%edx, %%eax\n\t"
#if defined(ENVIRONMENT64)
        "push %%rax\n\t"
        "popf\n\t"
        "pushf\n\t"
        "pop %%rax\n\t"
#else
        "pushl %%eax\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %%eax\n\t"
#endif
        "xor %%ecx, %%eax\n\t"
        "test %%edx, %%eax\n\t"
        "jnz cpuid_success\n\t"
        /* Failed (non-pentium compatible machine) */
        "mov $-1, %%eax\n\t"
        "jmp cpuid_finished\n\t"

"cpuid_success:"
        "xor %%eax, %%eax\n\t"
        "cpuid\n\t"
        "cmp $1, %%eax\n\t" 
        /* If 0 returned processor doesn't hav feature test */
        "jge feature_success\n\t"

        /* Unlucky - somehow cpuid 1 isn't supported */
        "mov $-2, %%eax\n\t" 
        "jmp cpuid_finished\n\t"

"feature_success:"
        "xor %%eax, %%eax\n\t"

"cpuid_finished:"
#if defined(ENVIRONMENT32) && defined(__PIC__)
        "popl %%ebx\n\t"
#endif
        : "=a"(result)
        :
        : "%ecx",
#if !defined(ENVIRONMENT32) || !defined(__PIC__)
          "%ebx", 
#endif 
          "%edx");
    return result;
}

static int
CheckSSE(void)
{
    int result;
    int cpuidchk;

    if ((cpuidchk = check_for_cpuid()) < 0)
        return cpuidchk;

#if USE_AVX

    asm volatile(
#if defined(ENVIRONMENT32) && defined(__PIC__)
        /* We have to be careful to not destroy ebx if using PIC on 32bit builds */
        "pushl %%ebx\n\t"
#endif
        "mov $1, %%eax\n\t" 
        "cpuid\n\t" 
        "and $0x018000000, %%ecx\n\t" /* Check bit 27 (OS uses XSAVE/XRSTOR)*/
        "cmp $0x018000000, %%ecx\n\t" /* and bit 28 (AVX supported by CPU) */
        "jne avx_unsupported\n\t"
        "xor %%ecx, %%ecx\n\t" /* XFEATURE_ENABLED_MASK/XCR0 register number = 0 */
        "xgetbv\n\t" /* XFEATURE_ENABLED_MASK register is in edx:eax" */
        "and $0b110, %%eax\n\t"
        "cmp $0b110, %%eax\n\t" /* check the AVX registers restore at context switch */
        "jne avx_unsupported\n\t"
        "mov $1, %%eax\n\t"
        "jmp avx_end\n\t"

"avx_unsupported:"
        "xor %%eax, %%eax\n\t"

"avx_end:"
#if defined(ENVIRONMENT32) && defined(__PIC__)
        "popl %%ebx\n\t"
#endif
        : "=a"(result) /* Result returned in result variable */
        :
        : "%ecx", 
#if !defined(ENVIRONMENT32) || !defined(__PIC__)
          "%ebx", 
#endif
          "%edx"); /* These are all destroyed by cpuid */

#else
#if defined(__APPLE__) || defined(__FreeBSD__)
    size_t length = sizeof(result);
#if defined(__APPLE__)
    int error = sysctlbyname("hw.optional.sse", &result, &length, NULL, 0);
#endif
#if defined(__FreeBSD__)
    int error = sysctlbyname("hw.instruction_sse", &result, &length, NULL, 0);
#endif
    if (0 != error)
        result = 0;
    return result;

#else

    asm volatile (
#if defined(ENVIRONMENT32) && defined(__PIC__)
        /* We have to be careful to not destroy ebx if using PIC on I386 */
        "pushl %%ebx\n\t"
#endif
        /* Check if sse is supported (bit 25/26 in edx from cpuid 1) */
        "mov $1, %%eax\n\t"
        "cpuid\n\t" 
        "mov $1, %%eax\n\t"
#if USE_SSE2
        "shl $26, %%eax\n\t"
#else
        "shl $25, %%eax\n\t"
#endif
        "test %%eax, %%edx\n\t"
        "jnz sse_success\n\t"
        /* Not supported */
        "mov $0, %%eax\n\t"
        "jmp sse_end\n\t"

"sse_success:"
        /* Supported */
        "mov $1, %%eax\n\t"

"sse_end:"
#if defined(ENVIRONMENT32) && defined(__PIC__)
        "pop %%ebx\n\t"
#endif

        : "=a"(result)
        :
        : "%ecx", 
#if !defined(ENVIRONMENT32) || !defined(__PIC__)
          "%ebx", 
#endif
	  "%edx");
#endif /* APPLE/BSD */

#endif /* USE_AVX */

    return result;
}

int
SIMD_Supported(void)
{
    static int state = -3;

    if (state == -3)
        state = CheckSSE();

    return state;
}

#endif
#endif
