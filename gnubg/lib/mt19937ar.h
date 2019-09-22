/*
 * mt19937int.h
 *
 * $Id: mt19937ar.h,v 1.5 2013/06/16 02:16:24 mdpetch Exp $
 */

#ifndef MT19937AR_H
#define MT19937AR_H

#define N 624

extern void init_genrand(unsigned long s, int *mti, unsigned long mt[N]);
extern unsigned long genrand_int32(int *mti, unsigned long mt[N]);
void init_by_array(unsigned long init_key[], int key_length, int *mti, unsigned long mt[N]);

#endif
