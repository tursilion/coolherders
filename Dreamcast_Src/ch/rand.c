/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* rand.c                               */
/****************************************/

// define this to use the c-runtime rand(), undef to use the fancy maths
//#define C_RAND

#ifdef C_RAND
#include <stdlib.h>
#endif

/* Thanks to Albert Veli for this clever routine! */
#define MASK 0xa3000000		// 32 bit random number

#ifdef WIN32
unsigned int seed = 1;
#else
unsigned int seed __attribute__ ((aligned(4))) = 1;
#endif

void ch_srand(unsigned long newseed)
{
#ifdef C_RAND
	srand(newseed);
#else
	if (newseed) {
		seed=newseed;
	} else {
		seed=1;
	}
#endif
}

// Will return a semi-random value - returns each possible number only once
// before repeating! Can never return 0, however.
int ch_rand()
{
#ifdef C_RAND
	return rand();
#else
	if(seed & 1){ /* If the bit shifted out is a 1, perform the xor */
		seed >>= 1;
		seed ^= MASK;
	}
	else { /* Else just shift */
		seed >>= 1;
	}
	
	return(seed);
#endif
}
