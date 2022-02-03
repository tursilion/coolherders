/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* rand.c                               */
/****************************************/

#include "rand.h"

/* Thanks to Albert Veli for this clever routine! */
#define MASK 0xa3000000		// 32 bit random number

unsigned int seed = 1;

void debug(char *str, ...);

void ch_srand(unsigned int newseed)
{
	debug("Setting random number seed to %d", newseed);
	if (newseed) {
		seed=newseed;
	} else {
		seed=1;
	}
}

// Will return a semi-random value - returns each possible number only once
// before repeating! Can never return 0, however.
volatile unsigned int ch_rand()
{
	if(seed & 1) { /* If the bit shifted out is a 1, perform the xor */
		seed >>= 1;
		seed ^= MASK;
	}
	else { /* Else just shift */
		seed >>= 1;
	}
	
	return(seed);
}

