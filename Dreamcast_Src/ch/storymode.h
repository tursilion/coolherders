/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* storymode.h                          */
/****************************************/

extern int gHumanPlayer;
extern int gStoryModeSpecialFlags;

char *doStory(int *nPhase);
int FindPhase(int nLevel);
void doOmake();

// Special flags for Story Mode effects (bitfields)
// No flag for timer ticking up, set the timer in gOptions to -1
// to get that effect
#define EFFECT_EVERY_CRATE_GIVES_BOOT	0x00000001
#define EFFECT_CREATE_NEW_SHEEP			0x00000002
#define EFFECT_SHEEP_RIDE_CONVEYORS		0x00000004
#define EFFECT_LOOKING_FOR_HADES		0x00000008
#define EFFECT_SHEEP_STUN_PLAYER		0x00000010
#define EFFECT_SHEEP_IGNORE_LIGHTNING	0x00000020
#define EFFECT_END_IF_ZAPPED			0x00000040
#define EFFECT_NO_SHEEP					0x00000080
#define EFFECT_WIN_BY_SHEEP				0x00000100	// only used in the bonus function
#define EFFECT_GRADUAL_SPEEDUP			0x00000200	// for the computer, that is
