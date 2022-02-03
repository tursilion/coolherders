/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* collide.h                            */
/****************************************/
 
extern unsigned char CollisionBuffer[640*480];

void clearbuffer();
void drawbuffer(int x, int y, uint32 type);
void mergebuffer(int x, int y, uint32 type);
uint32 checkblock(int x, int y, uint32 type);

// Bitflags (note: only 8 bits available)
#define TYPE_NONE 0		// Empty
#define TYPE_WALL 1		// Impassable Wall
#define TYPE_PLAY 2		// Player
#define TYPE_BOX  4		// Destructible box
#define TYPE_GHOSTWALL 8// Walls that ghost sheep honor

