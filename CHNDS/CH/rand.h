/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* rand.h                               */
/****************************************/

#include <stdlib.h>		// for the real rand()

extern unsigned int seed;

void ch_srand(unsigned int newseed);
volatile unsigned int ch_rand();
