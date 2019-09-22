/*
 * ------------------------------------------------------------------------------
 * Standard definitions and types, Bob Jenkins
 * Modified for inclusion with GNU Backgammon by Gary Wong
 * $Id: isaacs.h,v 1.5 2013/09/09 21:06:21 plm Exp $
 * ------------------------------------------------------------------------------
 */
#ifndef ISAACS_H
#define ISAACS_H

#if 0

/* Pre-64bits relics. Most of them are not used anyway */

typedef unsigned long int ub4;  /* unsigned 4-byte quantities */
#define UB4MAXVAL 0xffffffff
typedef signed long int sb4;
#define SB4MAXVAL 0x7fffffff
typedef unsigned short int ub2;
#define UB2MAXVAL 0xffff
typedef signed short int sb2;
#define SB2MAXVAL 0x7fff
typedef unsigned char ub1;
#define UB1MAXVAL 0xff
typedef signed char sb1;        /* signed 1-byte quantities */
#define SB1MAXVAL 0x7f

#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
typedef unsigned int uint32_t;
#endif

typedef uint32_t ub4;           /* unsigned 4-byte quantities */
#define UB4MAXVAL 0xffffffff

typedef int word;               /* fastest type available */

#endif
