/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* rand.h                               */
/****************************************/

extern unsigned int seed;

void ch_srand(unsigned long newseed);
unsigned int ch_rand();

#define rand() ch_rand()


