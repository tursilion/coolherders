/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* score.c                              */
/****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#else
#include <kos.h>
#endif

#include "sprite.h"
#include "cool.h"
#include "score.h"
#include "font.h"
#include "rand.h"
#include "levels.h"
#include "sound.h"
#include "control.h"
#include "sheep.h"
#include "menu.h"		// for globals
#include "storymode.h"
#include "vmu_logo.h"

/* from KOS itself... so we can tell when the PVR is busy rendering */
#ifndef WIN32
#include "../kernel/arch/dreamcast/hardware/pvr/pvr_internal.h"
#endif
extern volatile pvr_state_t pvr_state;

extern pvr_ptr_t disc_txr;
extern pvr_ptr_t txr_winner[4];
extern pvr_ptr_t txr_smfont;
extern pvr_ptr_t txr_sprites;
extern pvr_ptr_t txr_controlwork;
extern sfxhnd_t snd_score;
extern sfxhnd_t snd_bleat;
extern sfxhnd_t snd_boom;
extern int inDemo;
extern int nFrames;
extern int gReturnToMenu;
extern int gHumanPlayer;
extern int StoryModeTotalScore;
extern int isMultiplayer;
extern int level;
extern int nTimeLeft;
extern char szLastStoryCmd[16];

int colr[4] =   { 255, 255, 0,   255 };
int colg[4] =   { 128, 204, 153, 128 };
int colb[4] =   { 128, 0,   255, 255 };
#define BOXBRIGHT 128

static int bx[5], by[5], bxd[5], byd[5];
/* Player:               1   2    3    4  */
static int StartX[5] = { 4, 136, 371, 503,   268 };

static int DestX[4] =  { 57,  189, 424, 550 };
static int DestY    =  392;

static int TargetScore[4], Flash[4], BonusTotal[4];
static char szTextBanner[64], szBonusBanner[64];

static int nFinalTime=0;

static SPRITE BonusSheep[1610];	// We can draw up to this many! (Though we'll slow down over 550)
static int nBonusSheep[4];
static int nTotalBonusSheep;

#define GOLDSHEEPY 8

#define SCOREY    0

#define SCOREW  132
#define SCOREH   75
#define SCOREO1  25
#define SCOREO2  33
//#define SCOREV1  24
//#define SCOREV2  42
#define SCOREV1  34
#define SCOREV2  52

#define TIMERW  103
#define TIMERO   32
#define TIMERV   34

#define FLASH	128

#define BONUSSHEEP_Y_SPACING 8
#define BONUSSHEEP_Y_SPACING2 16

#define GOATBONUS 0x7ffffff

/* about 2 mins */
#define PAUSETIMEOUT 7200

extern pvr_ptr_t txr_readygopause;

static int nTimePaused=0;
static int nFade=0;

int HandleSheep(int nWinner, int nTime);
void DoChallengeEffects(int nWinner, char *szBest, char *szTime, char *szNeeded, int nRecord, int nPassed);

/* Macros to help with bonuses */
#ifdef min
#undef min
#endif

/* Apply bonus to EACH herder who matches the condition */
#define DOBONUS(N, X_S1, X_COND, X_SCORE)	{	\
	for (idx=0; idx<4; idx++) {						\
		b[idx]=0;									\
		if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;\
		if (X_COND) {								\
			b[idx]=X_SCORE;							\
		}											\
	}												\
	if ((b[0])||(b[1])||(b[2])||(b[3])) {			\
		ScaleIn(X_S1, X_SCORE);				\
		ProcessBonus(N, b[0], b[1], b[2], b[3]);	\
	}												\
}

/* Apply bonus to the herder with the most, if >0, and only if no tie! */
#define DOMOST(N, X_S1, X_VAR, X_SCORE)	{	\
	tmp=-1;											\
	for (idx=1; idx<4; idx++) {						\
		if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;\
		if (herder[idx].X_VAR > herder[tmp].X_VAR) {\
			tmp=idx;								\
		}											\
	}												\
	if (tmp != -1) {								\
		for (idx=0; idx<4; idx++) {						\
			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;\
			if (idx == tmp) continue;					\
			if (herder[idx].X_VAR == herder[tmp].X_VAR){\
				tmp=-1;									\
				break;									\
			}											\
		}												\
	}													\
	if (tmp != -1) {									\
		ScaleIn(X_S1, X_SCORE);					\
		switch (tmp) {									\
		case 0: ProcessBonus(N, X_SCORE, 0, 0, 0); break;	\
		case 1: ProcessBonus(N, 0, X_SCORE, 0, 0); break;	\
		case 2: ProcessBonus(N, 0, 0, X_SCORE, 0); break;	\
		case 3: ProcessBonus(N, 0, 0, 0, X_SCORE); break;	\
		}												\
	}													\
}

/* Apply bonus to the herder with the least, and only if no tie and not 0! */
#define DOLEAST(N, X_S1, X_VAR, X_SCORE)	{		\
	tmp=0;											\
	for (idx=1; idx<4; idx++) {						\
		if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;\
		if (herder[tmp].X_VAR == 0) {				\
			tmp=idx;								\
		} else 										\
		if ((herder[idx].X_VAR < herder[tmp].X_VAR) && \
			(herder[idx].X_VAR > 0)) {				\
			tmp=idx;								\
		}											\
	}												\
	for (idx=0; idx<4; idx++) {						\
		if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;\
		if (idx == tmp) continue;					\
		if (herder[idx].X_VAR == herder[tmp].X_VAR){\
			tmp=-1;									\
			break;									\
		}											\
	}												\
	if ((tmp != -1) && (herder[tmp].X_VAR > 0)) {	\
		ScaleIn(X_S1, X_SCORE);				\
		switch (tmp) {									\
		case 0: ProcessBonus(N, X_SCORE, 0, 0, 0); break;	\
		case 1: ProcessBonus(N, 0, X_SCORE, 0, 0); break;	\
		case 2: ProcessBonus(N, 0, 0, X_SCORE, 0); break;	\
		case 3: ProcessBonus(N, 0, 0, 0, X_SCORE); break;	\
		}												\
	}												\
}

/* simple min function */
inline int min(int a, int b) {
	if (a<b) {
		return a;
	} else {
		return b;
	}
}

/* Prepare the score bars */
void initScoreBar() {
	int idx;

	for (idx=0; idx<5; idx++) {
		bx[idx]=StartX[idx];
		by[idx]=SCOREY;
	}

	nTimePaused=0;
}

/* Write the text for scores */
static void DrawScoreText(int x, int r, int g, int b) {
	char buf[128];
	int xoff;
	int tr,tg,tb;
	int nScore;

	// Score
	xoff=0;
	tr=r; tg=g; tb=b;
	nScore=herder[x].score;
	if (nScore < 0) nScore=0;
	sprintf(buf, "%05d", nScore);
	if (gOptions.Timer != -1) {
		if (gOptions.Win==0) {
			sprintf(buf, "-%05d-", nScore);
			xoff=-16;
			tr=min(255, r+BOXBRIGHT);
			tg=min(255, g+BOXBRIGHT);
			tb=min(255, b+BOXBRIGHT);
		}
	}
	DrawFont(0, bx[x]+SCOREO1+xoff, by[x]+SCOREV1, INT_PACK_COLOR(255, tr, tg, tb), buf);

	// Sheep count
	xoff=0;
	tr=r; tg=g; tb=b;
	sprintf(buf, "@x%02ld", herder[x].sheep);
	if (gOptions.Timer != -1) {
		if (gOptions.Win==1) {
			sprintf(buf, "-@x%02ld-", herder[x].sheep);
			xoff=-16;
			tr=min(255, r+BOXBRIGHT);
			tg=min(255, g+BOXBRIGHT);
			tb=min(255, b+BOXBRIGHT);
		}
	}
	DrawFont(0, bx[x]+SCOREO2+xoff, by[x]+SCOREV2, INT_PACK_COLOR(255, tr, tg, tb), buf);
}

/* Used for herder boxes only - timer is special */
static void DrawOneBox(int x, int r, int g, int b) {
	SortRect((float)(x)/10.0f+527.0f, bx[x], by[x], bx[x]+SCOREW, by[x]+SCOREH, INT_PACK_COLOR(255, r, g, b), INT_PACK_COLOR(25, r, g, b));

	if ((herder[x].type&PLAY_MASK) == PLAY_NONE) return;

	DrawScoreText(x,r,g,b);
}

void DrawTimer(int nTime) {
	char buf[64];
	int old=gGfxDarken;
	gGfxDarken=0;

	SortRect(527.4f, bx[4], by[4], bx[4]+TIMERW, by[4]+SCOREH, INT_PACK_ALPHA(255), INT_PACK_ALPHA(128));
	if (gOptions.Timer) {
		sprintf(buf, "%02d", nTime);
	} else {
		strcpy(buf, "@@");
	}
	DrawFont(1, bx[4]+TIMERO, by[4]+TIMERV, DEFAULT_COLOR, buf);

	gGfxDarken=old;
}

void DrawScoreBar(int isPaused, int nTime) {
	int idx;

	if (isPaused) {
		if ((nFade < 200) || ((nTimePaused/120)%2)) {
			int tmp=gGfxDarken;
			gGfxDarken=0;
			addPage(txr_readygopause, 9, 207, 125, 240);
			gGfxDarken=tmp;
		}

		// Check for screensaver...
		nTimePaused++;
		if (nTimePaused == PAUSETIMEOUT) {
			// If we hit the timeout, initialize the movement
			for (idx=0; idx<5; idx++) {
				do {
					bxd[idx]=(ch_rand()%9)-4;
					byd[idx]=(ch_rand()%9)-4;
				} while ((bxd[idx]==0) || (byd[idx]==0));
			}
		} else {
			if (nTimePaused > PAUSETIMEOUT) {
				// If we already paused the timeout, continue the movement
				if (nTimePaused % 30 == 0) {
					if (nFade < 200) {
						nFade+=2;
					}
				}
		
				if (0 == (gOptions.NightMode&0x8000)) {
					// This fades by sorting a dark rectangle across the screen
					SortRect(526.9f, 0, 0, 640, 480, INT_PACK_COLOR(nFade, 0, 0, 0), INT_PACK_COLOR(nFade, 0, 0, 0));
				}

				for (idx=0; idx<5; idx++) {
					bx[idx]+=bxd[idx];
					if (bx[idx]+(idx==4?TIMERW:SCOREW) > 640) {
						bxd[idx]=-(signed)(ch_rand()%4+1);
					}
					if (bx[idx] < 0) {
						bxd[idx]=(ch_rand()%4+1);
					}
					by[idx]+=byd[idx];
					if (by[idx]+SCOREH > 480) {
						byd[idx]=-(signed)(ch_rand()%4+1);
					}
					if (by[idx] < 0) {
						byd[idx]=(ch_rand()%4)+1;
					}
				}
			}
		}
	} else {
		// Otherwise, we are not paused, move everything towards it's proper place (in case it has moved)
		nTimePaused=0;
		nFade=0;

		for (idx=0; idx<5; idx++) {
			if (by[idx] > 0) {
				by[idx]-=8;
			}
			if (by[idx] < 0) {
				by[idx]=0;
			}
			if (bx[idx] > StartX[idx]) {
				bx[idx]-=8;
			}
			// This one is always tested because we may start negative
			if (bx[idx] < 0) {
				bx[idx]=0;
			}
			if (bx[idx] < StartX[idx]) {
				bx[idx]+=8;
				// This one is nested so we don't jitter
				if (bx[idx] > StartX[idx]) {
					bx[idx]=StartX[idx];
				}
			}
		}
	}

	DrawOneBox(0, colr[0], colg[0], colb[0]);		// player 1
	DrawOneBox(1, colr[1], colg[1], colb[1]);		// player 2
	DrawOneBox(2, colr[2], colg[2], colb[2]);		// player 3
	DrawOneBox(3, colr[3], colg[3], colb[3]);		// player 4

	// Timer
	DrawTimer(nTime);
}

// Helper for HandleBonuses, draws the static stuff in place, leaves the list open
// When nCountdown is zero, then process the scoring
void DrawBonusScreen(int nCountdown) {
	int idx2;
	int scoresnd;
	int fRedraw;

	// Now draw the sheepies - do this BEFORE we start the scene!
	// We first need to move them. If a batch has finished moving, then we need to
	// update the static texture. Otherwise, we just draw it and the moving sheep behind it.
	fRedraw=0;
	for (idx2=nTotalBonusSheep-1; idx2>=0; idx2--) {
		if (BonusSheep[idx2].is3D != -1) {
			if (BonusSheep[idx2].is3D&0x80) {
				// Goat!
				BonusSheep[idx2].y+=16;
				// if finished moving
				if (BonusSheep[idx2].yd <= BonusSheep[idx2].y) {
					// Fix up the X axis and set redraw flag
					BonusSheep[idx2].y=BonusSheep[idx2].yd;
					BonusSheep[idx2].is3D&=0x7f;
					BonusSheep[idx2].x-=StartX[BonusSheep[idx2].is3D];
					BonusSheep[idx2].x+=BonusSheep[idx2].is3D*128;
					BonusSheep[idx2].is3D=-1;
					fRedraw=1;
				}
			} else {
				// Sheep!
				BonusSheep[idx2].y--;
				// if finished moving
				if (BonusSheep[idx2].yd >= BonusSheep[idx2].y) {
					// Fix up the X axis and set redraw flag
					BonusSheep[idx2].x-=StartX[BonusSheep[idx2].is3D];
					BonusSheep[idx2].x+=BonusSheep[idx2].is3D*128;
					BonusSheep[idx2].is3D=-1;
					fRedraw=1;
				}
			}
		}
	}

	if (fRedraw) {
		// Don't redraw while the PVR is rendering! (flicker/tear fix)
		waitPVRDone();

		// nasty trick - hold off the next render while we work
		// This isn't *perfect*, but works 99.9% of the time because
		// we SHOULD have a full vsync till the next test		
		pvr_state.render_busy=1;

		// NOTE: Unfortunately, this seems to take more than one frame,
		// so interrupts the animation just a bit at the end!

		// We need to redraw the static buffer as some sheep stopped moving
		// We probably should track more carefully and only redraw ONE herder,
		// but we'll see if this is quick enough. We don't need to erase because
		// nothing has been REMOVED, but we need to redraw all to keep the overlap
		// correct. (Or at least we'd need to redraw the last row, but that's hard
		// to calculate right now).
		for (idx2=nTotalBonusSheep-1; idx2>=0; idx2--) {
			if (BonusSheep[idx2].is3D == -1) {
				DrawSpriteTxr(disc_txr, &BonusSheep[idx2]);
			}
		}
		
		// We're done, clear the render flag. The render should begin on the
		// next vsync, unless we raced the PVR code. Even so, should only lose one frame
		pvr_state.render_busy=0;
	}

	BeginScene();
	pvr_list_begin(PVR_LIST_OP_POLY);
	pvr_list_finish();
	pvr_list_begin(PVR_LIST_TR_POLY);

	// Stretched score rectangles
	scoresnd=0;

	for (idx2=0; idx2<4; idx2++) {
		// Draw the box with possible brightness
		SortRect((float)(idx2)/10.0f+527.0f, bx[idx2], by[idx2], bx[idx2]+SCOREW, 440, 
			INT_PACK_COLOR(255, min(255, Flash[idx2]+colr[idx2]), min(255, Flash[idx2]+colg[idx2]), min(255, Flash[idx2]+colb[idx2])), 
			INT_PACK_COLOR(min(255, Flash[idx2]+125), min(255, Flash[idx2]+colr[idx2]), min(255, Flash[idx2]+colg[idx2]), min(255, Flash[idx2]+colb[idx2])));
	
		if (nCountdown < 1) {
			if (Flash[idx2]) {
				Flash[idx2]-=8;
				if (Flash[idx2]<0) Flash[idx2]=0;
			}

			// Skip inactive players
			if ((herder[idx2].type&PLAY_MASK) == PLAY_NONE) continue;

			// Update the score if not at target yet
			if (TargetScore[idx2] > herder[idx2].score) {
				herder[idx2].score+=50;
				if (herder[idx2].score > TargetScore[idx2]) {
					herder[idx2].score=TargetScore[idx2];
				}
				scoresnd=1;
			}
			if (TargetScore[idx2] < herder[idx2].score) {
				herder[idx2].score-=50;
				if (herder[idx2].score < TargetScore[idx2]) {
					herder[idx2].score=TargetScore[idx2];
				}
				scoresnd=1;
			}
		}

		// Score
		if ((herder[idx2].type&PLAY_MASK) != PLAY_NONE) {
			DrawScoreText(idx2, colr[idx2], colg[idx2], colb[idx2]);
		}
	}

	// Finally, we can draw the sheep. We'll draw the moving ones
	// first, then we'll drop our static textures on top
	for (idx2=nTotalBonusSheep-1; idx2>=0; idx2--) {
		if (BonusSheep[idx2].is3D != -1) {
			SortSprite(&BonusSheep[idx2]);
		}
	}
	// Now the textures themselves
	for (idx2=0; idx2<4; idx2++) {
		addNonTwiddledPage(disc_txr, StartX[idx2]+1, 1, idx2*128, 0, idx2*128+127, 440, DEFAULT_COLOR);
	}

	// Only play the score sound once, even for all four herders
	if (scoresnd) {
		sound_effect(snd_score, 64);
	}

	// Timer doesn't grow, but we sort a dark rect under it
	DrawTimer(nFinalTime);

	// Draw the herders and their golden sheep
	for (idx2=0; idx2<4; idx2++) {
		if ((herder[idx2].type&PLAY_MASK) == PLAY_NONE) continue;
		SortSprite(&herder[idx2].spr);
		if (herder[idx2].wins) {
			int x,idx3;
			x=herder[idx2].spr.x-(herder[idx2].wins-1)*16;
			for (idx3=0; idx3<herder[idx2].wins; idx3++) {
				// it's set up to be a sprite, but rather than do a whole structure for it, we'll just do this raw
				addPage2(txr_sprites, x, herder[idx2].spr.y+GOLDSHEEPY, 198, 104, 230, 140, DEFAULT_COLOR, 1025.0f);
				x+=32;
			}
		}
	}
}

#define ANIMSPEED 3

/* Text, points */
/* Legacy function, just saves the string now and plays the sound */
void ScaleIn(char *In1, int InScore) {
	if (InScore) {
		sprintf(szTextBanner, "%s - %d pts", In1, InScore);
	} else {
		strcpy(szTextBanner, In1);
	}
	debug("%s\n", szTextBanner);
	if (!inDemo) {
		sound_effect(snd_boom, 225);
	}
}

/* Call after ScaleIn, forwards the points to each player */
/* nIcon is ignored if positive. If negative, points are not added */
/* to score, only used to draw sheep */
/* Negative *scores* do not draw sheep any longer */
void ProcessBonus(int nIcon, int p1, int p2, int p3, int p4) {
	int idx, idx2, n, x1, y1, nCnt;
	int p[4];
	int isSlow;
	SPRITE *pSpr;

#ifdef USE_PC
	debug("ProcessBonus: %d: %d %d %d %d\n", nIcon, p1, p2, p3, p4);
#endif

	p[0]=p1;
	p[1]=p2;
	p[2]=p3;
	p[3]=p4;

	for (idx=0; idx<4; idx++) {
		if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;
		if (0 == (herder[idx].flags & FLAG_MOVED)) continue;	// no points if you never moved

		if (p[idx]) {
			if (nIcon >= 0) {
				TargetScore[idx]+=p[idx];
				BonusTotal[idx]+=p[idx];
			}
			Flash[idx]=FLASH;
			// Add new sheep to the pile!
			nCnt=p[idx]/50;
			for (n=0; n<nCnt; n++) {	// so if negative, nothing will be added
				pSpr=&BonusSheep[nTotalBonusSheep];
				pSpr->txr_addr=txr_sprites;
				pSpr->tilenumber=4;		// white sheep
				pSpr->alpha=255;
				pSpr->z=1000.0-(nTotalBonusSheep/10.0);
				// Calculate staggered position
				// There are 13 sheep in the pattern, stacked 6 on top of 7
				// Builds from the bottom up!
				// Note this *exactly* fits in our memory tile buffer, we can't
				// make it any wider than 128 pixels!
				x1=nBonusSheep[idx]%13;
				y1=nBonusSheep[idx]/13;
				pSpr->x=StartX[idx] + 4 + (((x1>6) ? /* top */ (x1-7)*16+8 : x1*16));
				pSpr->y=(380-((x1>6) ? /* top */ y1*BONUSSHEEP_Y_SPACING2+BONUSSHEEP_Y_SPACING : y1*BONUSSHEEP_Y_SPACING2))+32;
				pSpr->xd=pSpr->x;
				pSpr->yd=pSpr->y-32;
				pSpr->is3D=idx;		// hijack the is3D flag to track which herder this is - no harm done ;)
				if (nBonusSheep[idx] < 13) {
					// The first 7 don't move, so we need to fix them up here. We'll add ONE pixel
					// just so the redraw code can be triggered
					pSpr->y-=31;
					pSpr->yd=pSpr->y-1;
				}
				(nBonusSheep[idx])++;
				nTotalBonusSheep++;
			}
		}
	}

	/* Some delay so you can see the bonus */
	/* left trigger speeds it up, right trigger slows it down */
	isSlow=1;
	for (idx=0; idx<30; idx++) {	
		isSlow--;
		if (isSlow < 0) {
			isSlow=1;
			MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
				if (NULL == st) {
					continue;
				}
				// Check for left trigger to slow down bonus screen
				if (st->ltrig>=127) {
					isSlow=5;
					goto exitscan2;
				}
				// right trigger to speed it up (used to be start)
				if (st->rtrig>=127) {
					isSlow=-1;
				}
			MAPLE_FOREACH_END();
exitscan2: ;
		}

		DrawBonusScreen(isSlow);
		CenterDrawFont(0, 440, DEFAULT_COLOR, szTextBanner);
		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			break;
		}

		if ((inDemo)||(isSlow<0)) {
			// Quick out - break loop and move all sheep to final position
			for (idx2=nTotalBonusSheep-1; idx2>=0; idx2--) {
				if (BonusSheep[idx2].is3D != -1) {
					BonusSheep[idx2].y=BonusSheep[idx2].yd+1;
				}
			}
			break;
		}
	}
}

/* Same as ProcessBonus, but only for the pity goat ;) */
void ProcessGoat(int nIcon, int p1, int p2, int p3, int p4) {
	int idx, idx2;
	int p[4];
	SPRITE *pSpr;
	int isSlow;

	sound_effect(snd_bleat, 225);

#ifdef USE_PC
	debug("Pity Goat\n");
	debug("ProcessBonus: %d: %d %d %d %d\n", nIcon, p1, p2, p3, p4);
#endif

	p[0]=p1;
	p[1]=p2;
	p[2]=p3;
	p[3]=p4;

	for (idx=0; idx<4; idx++) {
		if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;

		if (p[idx]) {
			TargetScore[idx]+=p[idx];
			BonusTotal[idx]=GOATBONUS;
			Flash[idx]=FLASH;
			// This guy gets a goat!
			pSpr=&BonusSheep[nTotalBonusSheep];
			pSpr->txr_addr=txr_sprites;
			pSpr->tilenumber=19;
			pSpr->alpha=255;
			pSpr->z=1000.0-(nTotalBonusSheep/10.0);
			pSpr->is3D=idx|0x80;		// flag goat
			// Goats drop from the top, and land on top of the sheep
			pSpr->x=DestX[idx];
			pSpr->y=0;
			pSpr->xd=pSpr->x;
			pSpr->yd=(nBonusSheep[idx]/13)*BONUSSHEEP_Y_SPACING2+18;
			if ((nBonusSheep[idx]%13) > 4) pSpr->yd+=BONUSSHEEP_Y_SPACING;
			if ((nBonusSheep[idx]%13) > 10) pSpr->yd+=BONUSSHEEP_Y_SPACING;
			pSpr->yd=380-pSpr->yd;	// Invert it
			(nBonusSheep[idx])++;
			nTotalBonusSheep++;
		}
	}

	/* Some delay so you can see the bonus */
	/* Start speeds it up, either trigger slows it down */
	isSlow=1;
	for (idx=0; idx<isSlow; idx++) {	/* .5 - 1.5 second, depending on speed */
		DrawBonusScreen(isSlow);
		CenterDrawFont(0, 440, DEFAULT_COLOR, szTextBanner);
		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			break;
		}

		isSlow--;
		if (isSlow < 0) {
			isSlow=1;
			MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
				if (NULL == st) {
					continue;
				}
				// Check for left trigger to slow down bonus screen
				if (st->ltrig>=127) {
					isSlow=5;
					goto exitscan3;
				}
				// right trigger to speed it up (used to be start)
				if (st->rtrig>=127) {
					isSlow=-1;
				}
			MAPLE_FOREACH_END();
exitscan3: ;
		}
		
		if ((inDemo)||(isSlow<0)) {
			// Quick out - break loop and move all sheep to final position
			for (idx2=nTotalBonusSheep-1; idx2>=0; idx2--) {
				if (BonusSheep[idx2].is3D != -1) {
					BonusSheep[idx2].y=BonusSheep[idx2].yd-1;
				}
			}
			break;
		}
	}
}

void GenerateBonusBanner(char*szBonusBanner) {
	int idx;
	// Update the text banner for all the bonuses (we have to repeat because of the goat)
	// Hacky, manually tweaked code, but should work most of the time
	strcpy(szBonusBanner, "+ ");
	idx=0;
	if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) {
		strcat(szBonusBanner, "     ");
	} else {
		if (BonusTotal[idx]==GOATBONUS) {
			strcat(szBonusBanner, " Pity");
		} else {
			// The Tony Maneuver for formatted strcat ;)
			sprintf(szBonusBanner, "%s%5d", szBonusBanner, (int)herder[idx].score);
		}
	}
	
	idx=1;
	strcat(szBonusBanner, "   ");
	if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) {
		strcat(szBonusBanner, "     ");
	} else {
		if (BonusTotal[idx]==GOATBONUS) {
			strcat(szBonusBanner, " Pity");
		} else {
			// The Tony Maneuver for formatted strcat ;)
			sprintf(szBonusBanner, "%s%5d", szBonusBanner, (int)herder[idx].score);
		}
	}

	idx=2;
	strcat(szBonusBanner, "   TOTAL  ");
	if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) {
		strcat(szBonusBanner, "     ");
	} else {
		if (BonusTotal[idx]==GOATBONUS) {
			strcat(szBonusBanner, " Pity");
		} else {
			// The Tony Maneuver for formatted strcat ;)
			sprintf(szBonusBanner, "%s%5d", szBonusBanner, (int)herder[idx].score);
		}
	}

	idx=3;
	strcat(szBonusBanner, "   +");
	if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) {
		strcat(szBonusBanner, "     ");
	} else {
		if (BonusTotal[idx]==GOATBONUS) {
			strcat(szBonusBanner, " Pity");
		} else {
			// The Tony Maneuver for formatted strcat ;)
			sprintf(szBonusBanner, "%s%5d", szBonusBanner, (int)herder[idx].score);
		}
	}
	debug("%s\n", szBonusBanner);
}

void GenerateSheepBanner(char*szBonusBanner) {
	int idx;
	// Update the text banner for all the bonuses (we have to repeat because of the goat)
	// Hacky, manually tweaked code, but should work most of the time
	strcpy(szBonusBanner, "+ ");
	idx=0;
	if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) {
		strcat(szBonusBanner, "     ");
	} else {
		// The Tony Maneuver for formatted strcat ;)
		if (BonusTotal[idx]<10) {
			sprintf(szBonusBanner, "%s  %d  ", szBonusBanner, BonusTotal[idx]);
		} else {
			sprintf(szBonusBanner, "%s+ %2d +", szBonusBanner, BonusTotal[idx]);
		}
	}
	
	idx=1;
	strcat(szBonusBanner, "    ");
	if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) {
		strcat(szBonusBanner, "     ");
	} else {
		// The Tony Maneuver for formatted strcat ;)
		if (BonusTotal[idx]<10) {
			sprintf(szBonusBanner, "%s  %d  ", szBonusBanner, BonusTotal[idx]);
		} else {
			sprintf(szBonusBanner, "%s+ %2d +", szBonusBanner, BonusTotal[idx]);
		}
	}

	idx=2;
	strcat(szBonusBanner, "  SHEEP   ");
	if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) {
		strcat(szBonusBanner, "     ");
	} else {
		if (BonusTotal[idx]<10) {
			sprintf(szBonusBanner, "%s  %d  ", szBonusBanner, BonusTotal[idx]);
		} else {
			sprintf(szBonusBanner, "%s+ %2d +", szBonusBanner, BonusTotal[idx]);
		}
	}

	idx=3;
	strcat(szBonusBanner, "  +");
	if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) {
		strcat(szBonusBanner, "     ");
	} else {
		if (BonusTotal[idx]<10) {
			sprintf(szBonusBanner, "%s  %d  ", szBonusBanner, BonusTotal[idx]);
		} else {
			sprintf(szBonusBanner, "%s+ %2d +", szBonusBanner, BonusTotal[idx]);
		}
	}
	debug("%s\n", szBonusBanner);
}

// Helper for HandleBonuses and HandleSheep
void ScaleGoldenSheep(int nWinner) {
	int idx, idx2, idx3;
	int nStart;

	// We know who won at this point, so we can update the human player's total score
	// (if not multiplayer, it won't matter anyway)
	if (gHumanPlayer != -1) {
		StoryModeTotalScore+=herder[gHumanPlayer].score;
	}

	// handle accidentally getting here in challenge failure case
	if (nWinner > 3) return;

	// Scale a golden sheep down to the winner(s)
	if ((!isMultiplayer)&&(nWinner != gHumanPlayer)) {
		// all computer players win in story mode!
		for (idx=100; idx>0; idx-=5) {
			DrawBonusScreen(0);
			DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
			
			for (idx2=0; idx2<4; idx2++) {
				if ((herder[idx2].type&PLAY_MASK) == PLAY_NONE) continue;
				if ((herder[idx2].type&PLAY_MASK) == PLAY_HUMAN) continue;
				stretchPage2(txr_sprites, herder[idx2].spr.x-idx, herder[idx2].spr.y+GOLDSHEEPY-idx, herder[idx2].spr.x+32+idx, herder[idx2].spr.y+GOLDSHEEPY+36+idx, 198, 104, 230, 140, 1025.0f, DEFAULT_COLOR);
			}

			pvr_list_finish();
			pvr_scene_end;

			if (gReturnToMenu) {
				sound_stop();
				return;
			}
		}

		// update winners
		for (idx2=0; idx2<4; idx2++) {
			for (idx2=0; idx2<4; idx2++) {
				if ((herder[idx2].type&PLAY_MASK) == PLAY_NONE) continue;
				if ((herder[idx2].type&PLAY_MASK) == PLAY_HUMAN) continue;
				herder[idx2].wins++;
			}
		}
	} else {
		// just one winner
		for (idx=100; idx>0; idx-=5) {
			DrawBonusScreen(0);
			DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);

			stretchPage2(txr_sprites, herder[nWinner].spr.x-idx, herder[nWinner].spr.y+GOLDSHEEPY-idx, herder[nWinner].spr.x+32+idx, herder[nWinner].spr.y+GOLDSHEEPY+36+idx, 198, 104, 230, 140, 1025.0f, DEFAULT_COLOR);

			pvr_list_finish();
			pvr_scene_end;

			if (gReturnToMenu) {
				sound_stop();
				return;
			}
		}

		// update the win count
		herder[nWinner].wins++;
	}


	// Display the Player X Wins!
	// Play the music!
	sound_stop();
	bg_sound_start(SONG_WIN);

	idx2=450;				// frame countdown
	idx3=GetFinalWinner();	// to check if the rounds are up

	// Wait for the music (or delay) to end
	nStart=isStartPressed();
	for (;;) {
		// Wait for any pre-existing Start to be released
		int t=isStartPressed();
		if ((nStart==0)&&(t)) {
			sound_stop();
			break;
		}
		if ((nStart)&(!t)) {
			nStart=0;
		}
		DrawBonusScreen(0);
		DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
		if (idx3 > -1) {
			addPage(txr_winner[nWinner], 0, 0, 255, 255);
		}
		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			break;
		}

		if (idx2>0) idx2--;

		if ((idx2<=0)&&(!musicisplaying())) {
			// end of a cycle, wait for end of music
			break;
		}
		
		Flash[nWinner]=abs((idx2%32)-16)*8;
	}
}

// Helper for HandleBonuses and HandleSheep
// Special version for the timer versions
void ScaleGoldenSheepTime(int nWinner, char *szBest, char *szTime, char *szNeeded, int nRecord) {
	int idx, idx2, idx3;
	int nStart;

	// We know who won at this point, so we can update the human player's total score
	// (if not multiplayer, it won't matter anyway)
	if (gHumanPlayer != -1) {
		StoryModeTotalScore+=herder[gHumanPlayer].score;
	}

	// handle accidentally getting here in challenge failure case
	if (nWinner > 3) return;

	// Scale a golden sheep down to the winner(s)
	// just one winner
	for (idx=100; idx>0; idx-=5) {
		DrawBonusScreen(0);
		CenterDrawFontZ(1024.0f, 0, 130, DEFAULT_COLOR, szNeeded);
		DrawFontZ(1024.0f, 0, 0, -1, DEFAULT_COLOR, "");
		CenterDrawFontZ(1024.0f, 0, -1, DEFAULT_COLOR, szTime);
		DrawFontZ(1024.0f, 0, 0, -1, DEFAULT_COLOR, "");
		CenterDrawFontZ(1024.0f, 0, -1, DEFAULT_COLOR, szBest);
		if (nRecord) {
			DrawFontZ(1024.0f, 1, 0, -1, DEFAULT_COLOR, "");
			CenterDrawFontZ(1024.0f, 1, -1, DEFAULT_COLOR, "New Record!");
		}

		stretchPage2(txr_sprites, herder[nWinner].spr.x-idx, herder[nWinner].spr.y+GOLDSHEEPY-idx, herder[nWinner].spr.x+32+idx, herder[nWinner].spr.y+GOLDSHEEPY+36+idx, 198, 104, 230, 140, 1025.0f, DEFAULT_COLOR);

		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			return;
		}
	}

	// update the win count
	herder[nWinner].wins++;


	// Display the Player X Wins!
	// Play the music!
	sound_stop();
	bg_sound_start(SONG_WIN);

	idx2=450;				// frame countdown
	idx3=GetFinalWinner();	// to check if the rounds are up

	// Wait for the music (or delay) to end
	nStart=isStartPressed();
	for (;;) {
		// Wait for any pre-existing Start to be released
		int t=isStartPressed();
		if ((nStart==0)&&(t)) {
			sound_stop();
			break;
		}
		if ((nStart)&(!t)) {
			nStart=0;
		}
		DrawBonusScreen(0);
		CenterDrawFontZ(1024.0f, 0, 130, DEFAULT_COLOR, szNeeded);
		DrawFontZ(1024.0f, 0, 0, -1, DEFAULT_COLOR, "");
		CenterDrawFontZ(1024.0f, 0, -1, DEFAULT_COLOR, szTime);
		DrawFontZ(1024.0f, 0, 0, -1, DEFAULT_COLOR, "");
		CenterDrawFontZ(1024.0f, 0, -1, DEFAULT_COLOR, szBest);
		if (nRecord) {
			DrawFontZ(1024.0f, 1, 0, -1, DEFAULT_COLOR, "");
			CenterDrawFontZ(1024.0f, 1, -1, DEFAULT_COLOR, "New Record!");
		}
		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			break;
		}

		if (idx2>0) idx2--;

		if ((idx2<=0)&&(!musicisplaying())) {
			// end of a cycle, wait for end of music
			break;
		}
		
		Flash[nWinner]=abs((idx2%32)-16)*8;
	}
}

// nWinner is 1-4, or 0xff
// returns the player number who won, or -1 if aborted
int HandleBonuses(int nWinner, int nTime) {
	int idx, idx2=0, tx1, ty1, tx2, ty2, tmp;
	int notidx;
	int b[4];
	float pxstep[4], pystep[4], px[4], py[4];	
	char buf[80];
	int nPlayerCount=0;		// we'll disable some bonuses if there's only one herder

	// Count actual players
	for (idx=0; idx<4; idx++) {
		if ((herder[idx].type&PLAY_MASK) != PLAY_NONE) nPlayerCount++;
	}

	// nWinner is 1-4, change it into an index
	nWinner--;

	// Cache the time so we don't need to pass it around
	if (gOptions.Timer) {
		nFinalTime=nTime;
	} else {
		nFinalTime=0;
	}

	// Set up scaling text
	tx1=bx[4]+TIMERO-20;
	ty1=by[4]+SCOREH+220;		//10;
	tx2=tx1+80;
	ty2=ty1+80;
	CLEAR_IMAGE(disc_txr, 512, 512);

	// Scale up an appropriate text
	if (gStoryModeSpecialFlags&EFFECT_LOOKING_FOR_HADES) {
		// then we need a max time there...
		if (nTimeLeft > atoi(&szLastStoryCmd[2])) {
			DrawFontTxr(disc_txr, 1, 0, 0, "Time");
			DrawFontTxr(disc_txr, 1, 0, 35,"Over");
		} else {
			DrawFontTxr(disc_txr, 1, 0, 0, "Hades");
			DrawFontTxr(disc_txr, 1, 0, 35,"Found");
		}
	} else if (gStoryModeSpecialFlags&EFFECT_END_IF_ZAPPED) {
		DrawFontTxr(disc_txr, 1, 0, 0, "Zeus");
		DrawFontTxr(disc_txr, 1, 0, 35,"Caught");
	} else if (gStoryModeSpecialFlags&EFFECT_SHEEP_RIDE_CONVEYORS) {
		sound_effect(snd_sheep[ZAPPEDA+ch_rand()%3], SHEEPVOL);
		DrawFontTxr(disc_txr, 1, 0, 0, "Sheep");
		DrawFontTxr(disc_txr, 1, 0, 35,"Lost");
	} else if (nTime == 0) {
		DrawFontTxr(disc_txr, 1, 0, 0, "Time");
		DrawFontTxr(disc_txr, 1, 0, 35,"Over");
	} else {
		DrawFontTxr(disc_txr, 1, 0, 0, "Level");
		DrawFontTxr(disc_txr, 1, 0, 35,"Clear");
	}
	for (idx=0; idx<4; idx++) {
		px[idx]=herder[idx].spr.x;
		py[idx]=herder[idx].spr.y;
		pxstep[idx]=(DestX[idx]-px[idx])/(100.0/ANIMSPEED);
		pystep[idx]=(DestY-py[idx])/(100.0/ANIMSPEED);
		herder[idx].spr.tilenumber=21;	// Front and center!
		herder[idx].spr.z=(float)1024;	// Put completely on top
		TargetScore[idx]=herder[idx].score;
		BonusTotal[idx]=0;
		Flash[idx]=0;
		// Prepare the bonus sheep!
		nBonusSheep[idx]=0;
	}
	nTotalBonusSheep=0;

	// transition
	for (idx=0; idx<100; idx+=ANIMSPEED) {
		if (gOptions.NightMode&0x8000) {
			if (gGfxDarken > 0) {
				gGfxDarken-=2;
				if (gGfxDarken<0) {
					gGfxDarken=0;
				}
			}
		}

		notidx=100-idx;

		// check for escape codes
		if (gReturnToMenu) {
			gGfxDarken=0;
			sound_stop();
			return -1;
		}

		// Draw the background
		if (DrawLevel()) {
			debug("DrawLevel shouldn't return true here!\n");
			continue;	// should never return 1
		}

		// Grow down the score rectangles, making them more transparent at the bottom ;)
		for (idx2=0; idx2<4; idx2++) {
			SortRect((float)(idx2)/10.0f+527.0f, bx[idx2], by[idx2], bx[idx2]+SCOREW, by[idx2]+SCOREH+(int)(idx*3.65), INT_PACK_COLOR(255, colr[idx2], colg[idx2], colb[idx2]), INT_PACK_COLOR(25+idx, colr[idx2], colg[idx2], colb[idx2]));
			if ((herder[idx2].type&PLAY_MASK) == PLAY_NONE) continue;
		
			DrawScoreText(idx2, colr[idx2], colg[idx2], colb[idx2]);
		}

		// Fade the background to black with a dark rectangle
		tmp=idx*2.55;
		SortRect(526.8f, 0, 0, 640, 480, INT_PACK_COLOR(tmp, 0, 0, 0), INT_PACK_COLOR(tmp, 0, 0, 0));

		tmp=gGfxDarken;
		gGfxDarken=0;
		if (gOptions.Timer) {
			sprintf(buf, "%02d", nTime);
		} else {
			strcpy(buf, "@@");
		}
		DrawFont(1, bx[4]+TIMERO, by[4]+TIMERV, DEFAULT_COLOR, buf);
		SortRect(526.9f, bx[4], by[4]+SCOREH, bx[4]+TIMERW, by[4]+SCOREH+(int)(idx*3.65), INT_PACK_COLOR(idx<<1, 0, 0, 0), INT_PACK_COLOR(idx<<1, 0, 0, 0));
		gGfxDarken=tmp;
		
		// Scale out the message
		stretchLarge3(disc_txr, tx1-idx*12, ty1-idx*12, tx2+idx*12, ty2+idx*12, 0, 0, 80, 80, DEFAULT_COLOR);

		// Move the herders into place and draw them 
		for (idx2=0; idx2<4; idx2++) {
			if ((herder[idx2].type&PLAY_MASK) == PLAY_NONE) continue;
	
			px[idx2]+=pxstep[idx2];
			py[idx2]+=pystep[idx2];
			herder[idx2].spr.x=px[idx2];
			herder[idx2].spr.y=py[idx2];
			SortSprite(&herder[idx2].spr);
		}

		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			return -1;
		}
	}
	gGfxDarken=0;

	// Clear the image again so we can reuse it
	CLEAR_IMAGE(disc_txr, 512, 512);

	// break out here
	if (gOptions.Win != 0) {
		return HandleSheep(nWinner, nTime);
	}
	if (gOptions.Timer == -1) {
		return HandleTimer(nWinner, nTime);
	}

	// Wild sheep penalty
	if (!(gStoryModeSpecialFlags&(EFFECT_CREATE_NEW_SHEEP|EFFECT_LOOKING_FOR_HADES|EFFECT_NO_SHEEP))) {
		// don't penalize the player if there are always more sheep coming or they can't be caught!
		if ((nTime == 0) && (herder[0].sheep+herder[1].sheep+herder[2].sheep+herder[3].sheep != MAX_SHEEP)) {
			int x;
			x=-(signed)((MAX_SHEEP - (herder[0].sheep+herder[1].sheep+herder[2].sheep+herder[3].sheep))*50);
			ScaleIn("Wild Sheep Penalty", x);
			ProcessBonus(0, x, x, x, x);
		}
	}

	// Now display the player's level score (the -1 to ProcessBonus prevents adding it to the score again)
	ScaleIn("Level Score", 0);
	ProcessBonus(-1, herder[0].score, herder[1].score, herder[2].score, herder[3].score);

	// Time Bonus
	if (gOptions.Timer != -1) {
		if (nTime > 1) {
			ScaleIn("Time Bonus", nTime*50);
			// TIME
			ProcessBonus(0, nTime*50, nTime*50, nTime*50, nTime*50);
		}
	}

	// Sheep Bonus
	if ((herder[0].sheep)||(herder[1].sheep)||(herder[2].sheep)||(herder[3].sheep)) {
		ScaleIn("Sheep Bonus - each", 100);
		// SHEEP
		ProcessBonus(0, herder[0].sheep*100, herder[1].sheep*100, herder[2].sheep*100, herder[3].sheep*100);
	}

	// Last second bonus
	if (gOptions.Timer != -1) {
		if (nTime == 1) {
			ScaleIn("Last Second", 1000);
			// L.SEC
			ProcessBonus(1, 1000, 1000, 1000, 1000);
		}
	}

	// First place can only have Master Herder, No Contest or First Place bonus
	// Master Herder
	if (nPlayerCount > 1) {
		// If there was no tie, only
		if (nWinner >= 0) {
			if ((!(herder[nWinner].flags & FLAG_ATTACKED)) && (herder[nWinner].flags & FLAG_MOVED)) {
				// MASTR
				ScaleIn("Master Herder", 3000);
				switch (nWinner) {
				case 0: ProcessBonus(2, 3000, 0, 0, 0); break;
				case 1: ProcessBonus(2, 0, 3000, 0, 0); break;
				case 2: ProcessBonus(2, 0, 0, 3000, 0); break;
				case 3: ProcessBonus(2, 0, 0, 0, 3000); break;
				}
			} else {
				// No Contest
				// NOCON
				if (!(herder[nWinner].flags & FLAG_NOTWINNING)) {
					ScaleIn("No Contest", 2000);
					switch (nWinner) {
					case 0: ProcessBonus(3, 2000, 0, 0, 0); break;
					case 1: ProcessBonus(3, 0, 2000, 0, 0); break;
					case 2: ProcessBonus(3, 0, 0, 2000, 0); break;
					case 3: ProcessBonus(3, 0, 0, 0, 2000); break;
					}
				} else {
					// Boring old First Place
					if (nWinner < 4) {
						// 1ST
						ScaleIn("Most Sheep", 1000);
						switch (nWinner) {
						case 0: ProcessBonus(4, 1000, 0, 0, 0); break;
						case 1: ProcessBonus(4, 0, 1000, 0, 0); break;
						case 2: ProcessBonus(4, 0, 0, 1000, 0); break;
						case 3: ProcessBonus(4, 0, 0, 0, 1000); break;
						}
					}
				}
			}
		}
	}

	// Feuding
	if (nPlayerCount > 1) {
		for (idx=0; idx<4; idx++) {
			b[idx]=0;
			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;

			if ((herder[idx].flags & FLAG_STALKER) && (herder[idx].h_attacks > 5)) {
				// If we are reciprocal stalkers, then we're feuding instead
				if (herder[idx].lasthit < 4) {
					if (herder[herder[idx].lasthit].lasthit == idx) {
						b[idx]=50;
					}
				}
			}
		}
		if ((b[0])||(b[1])||(b[2])||(b[3])) {
			// FEUD
			ScaleIn("Feuding", 50);
			ProcessBonus(5, b[0], b[1], b[2], b[3]);
		}
	}


	// Punching Bag
	if (nPlayerCount > 1) {
		// P.BAG
		DOBONUS(23, "Punching Bag", (herder[idx].punchingbag >= 3), 100);
	}

	if (gOptions.Powers) {
		// Materialist
		// MATER
		DOMOST(24, "Materialist", powerups, 100);

		// Slowpoke
		// SLOWP
		DOBONUS(25, "Slowpoke", ((herder[idx].speed == HERDERSPEED) && (herder[idx].flags & FLAG_MOVED) && (herder[idx].idletime < 600)), 100);

		// Conservationalist
		// CONSER
		DOBONUS(26, "Conservationalist", ((herder[idx].range == 2) && (herder[idx].flags & FLAG_MOVED) && (herder[idx].idletime < 600)), 100);
	}

	// Most masochistic
	// MASOC
	if (nPlayerCount > 1) {
		DOMOST(19, "Most Masochistic", zapped, 500);
	}

	// Statue
	// STATU
	DOBONUS(20, "Statue", (herder[idx].idletime > 600), 500);

	// Avenger
	if (nPlayerCount > 1) {
		// AVENG
		DOBONUS(21, "Avenger", (herder[idx].flags & FLAG_REVENGE), 500);
	}

	// Scavenger Hunt
	// HUNT
	DOBONUS(18, "Scavenger Hunt", (herder[idx].c_attacks > 10), 800);

	// First Strike
	if (nPlayerCount > 1) {
		// 1STRK
		DOBONUS(10, "First Strike", (herder[idx].flags & FLAG_FIRSTSTRIKE), 1000);
	}

	// Fastest Reflexes
	if (nPlayerCount > 1) {
		tmp=-1;
		// Find a baseline - any herder with a valid count
		for (idx=0; idx<4; idx++) {
			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;
			if ((herder[idx].sheep == herder[idx].maxsheep) && (herder[idx].lostcount > 0)) {	// discount those without any lost sheep or not at max
				tmp=idx;
			}
		}
		if (tmp != -1) {
			// Now compare against this base to find the best
			for (idx=0; idx<4; idx++) {	
				if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;
				if ((herder[idx].sheep == herder[idx].maxsheep) && (herder[idx].lostcount > 0)) {	
					if (((float)(herder[idx].losttime)/(float)(herder[idx].lostcount)) < ((float)(herder[tmp].losttime)/(float)(herder[tmp].lostcount))) {
						tmp=idx;								
					}
				}
			}
			// Now check the best time for a tie
			for (idx=0; idx<4; idx++) {						
				if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;
				if (idx == tmp) continue;		
				if (herder[idx].sheep == herder[idx].maxsheep) {
					if (((float)(herder[idx].losttime)/(float)(herder[idx].lostcount)) == ((float)(herder[tmp].losttime)/(float)(herder[tmp].lostcount))) {
						tmp=-1;									
						break;									
					}
				}
			}
		}
		// If we still have a winner, he's the sole winner. Award him
		if (tmp != -1) {		
			// REFLX
			ScaleIn("Fastest Reflexes", 1000);
			switch (tmp) {									
			case 0: ProcessBonus(11, 1000, 0, 0, 0); break;	
			case 1: ProcessBonus(11, 0, 1000, 0, 0); break;	
			case 2: ProcessBonus(11, 0, 0, 1000, 0); break;	
			case 3: ProcessBonus(11, 0, 0, 0, 1000); break;	
			}												
		}												
	}

	// Never looks back
	// NO.U
	DOBONUS(13, "Never Looks Back", ((!(herder[idx].flags & FLAG_UTURN)) && (herder[idx].flags & FLAG_MOVED) && (herder[idx].idletime < 600)), 1000);

	// Evasive
	// EVADE
	if (nPlayerCount > 1) {
		DOBONUS(14, "Evasive", ((!(herder[idx].flags & FLAG_VICTIM)) && (herder[idx].flags & FLAG_MOVED) && (herder[idx].idletime < 600)), 1000);
	}

	if (gOptions.Powers) {
		// Minimalist
		// MINML
		DOLEAST(15, "Minimalist", powerups, 1000);

		// Track Star
		// TRACK
		DOBONUS(16, "Track Star", (((herder[idx].type&PLAY_SPECIAL_POWER)==0)&&(herder[idx].speed == HERDERMAXSPEED)), 1000);

		// Lightning God
		// LIGHT
		DOBONUS(17, "Lightning God", (((herder[idx].type&PLAY_SPECIAL_POWER)==0)&&(herder[idx].range == MAXLIGHTNING)), 1000);
	}

	// Berzerker
	// BEZRK
	DOBONUS(7, "Berzerker", (herder[idx].berserker > 0), 500);

	if (gOptions.Powers) {
		// Back to Basics
		// BASIC
		DOBONUS(8, "Back to Basics", ((herder[idx].powerups == 0) && (herder[idx].flags & FLAG_MOVED)), 1500);
	}
	
	// Barbeque
	// BBQ
	DOBONUS(9, "Barbeque", (herder[idx].s_attacks > 50), 1500);

	// Pacifist
	if (nPlayerCount > 1) {
		// PACIF
		DOBONUS(6, "Pacifist", ((!(herder[idx].flags & FLAG_ATTACKED)) && (herder[idx].flags & FLAG_MOVED) && (idx != nWinner) && (herder[idx].idletime < 600)), 2000);
	}

	// Update the text banner for all the bonuses 
	GenerateBonusBanner(szBonusBanner);

	// Now just wait for all the bonuses to be tallied up
	do {
		int i=0;
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
			if (NULL == st) {
				continue;
			}
			// right trigger to speed it up (used to be start)
			if (st->rtrig>=127) {
				i=1;
			}
		MAPLE_FOREACH_END();

		DrawBonusScreen(0);
		DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			return -1;
		}

		// This for provides the exit condition, leave it right before the while!
		for (idx=0; idx<4; idx++) {
			if (herder[idx].score < 0) herder[idx].score=0;
			if (TargetScore[idx] < 0) TargetScore[idx]=0;
			if (i) herder[idx].score=TargetScore[idx];
			if (TargetScore[idx] != herder[idx].score) {
				break;
			}
		}
	} while (idx<4);

	// Pity points - If 1500 or less, we give 150 pts and a goat ;)
	for (idx=0; idx<4; idx++) {
 		b[idx]=0;
		if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;

		if (TargetScore[idx]<=0) {
			TargetScore[idx]=0;		/* ensure it's clamped */
			b[idx]=150;
		} else if (TargetScore[idx]<1500) {
			TargetScore[idx]+=150;
			b[idx]=150;
		}
	}
	if ((b[0])||(b[1])||(b[2])||(b[3])) {
		strcpy(szTextBanner, "Pity Goat");
		ProcessGoat(32, b[0], b[1], b[2], b[3]);
	}

	// Update the text banner for all the bonuses (we have to repeat because of the goat)
	GenerateBonusBanner(szBonusBanner);

	// Now just wait (again) for all the bonuses to be tallied up
	do {
		int i=0;
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
			if (NULL == st) {
				continue;
			}
			// right trigger to speed it up (used to be start)
			if (st->rtrig>=127) {
				i=1;
			}
		MAPLE_FOREACH_END();

		DrawBonusScreen(0);
		DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
		pvr_list_finish();
		pvr_scene_end;

		if (gReturnToMenu) {
			sound_stop();
			return -1;
		}

		// This for provides the exit condition, leave it right before the while!
		for (idx=0; idx<4; idx++) {
			if (herder[idx].score < 0) herder[idx].score=0;
			if (TargetScore[idx] < 0) TargetScore[idx]=0;
			if (i) herder[idx].score=TargetScore[idx];
			if (TargetScore[idx] != herder[idx].score) {
				break;
			}
		}
	} while (idx<4);

	// Now work out who won 
	debug("FINAL SCORES:\n");
	idx2=herder[0].score;
	nWinner=0;		// nWinner is a LOCAL variable here!
	// always check human first so he gets priority in ties
	if (!isMultiplayer) {
		idx2=herder[gHumanPlayer].score;
		nWinner=gHumanPlayer;
	}
	for (idx=0; idx<4; idx++) {
		if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;
		debug("Player %d: %ld\n", idx, herder[idx].score);
		if (herder[idx].score > idx2) {
			idx2=herder[idx].score;
			nWinner=idx;
		}
	}

	ScaleGoldenSheep(nWinner);
	if (gReturnToMenu) {
		return -1;
	}

	return nWinner;
}

// nWinner is 0-3, or -1
// returns the player number who won, or -1 if aborted
// this is the version for win-by-sheep mode
int HandleSheep(int nWinner, int nTime) {
	int idx, idx2, idx3, x1, y1;
	SPRITE *pSpr;
	int ySpacing;
	int nDelayTimer=50;

	if ((nWinner < 0)||(nWinner > 3)) {
		// there was a tie by sheep - do it by score
		debug("Breaking sheep tie - FINAL SCORES:\n");
		idx2=herder[0].score;
		idx3=herder[0].sheep;
		nWinner=0;		// nWinner is a LOCAL variable here!
		// always check human first so he gets priority in ties
		if (!isMultiplayer) {
			idx2=herder[gHumanPlayer].score;
			nWinner=gHumanPlayer;
		}
		for (idx=0; idx<4; idx++) {
			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;
			debug("Player %d: %ld\n", idx, herder[idx].score);
			if ((herder[idx].sheep > idx3) || ((herder[idx].sheep == idx3) && (herder[idx].score > idx2))) {
				idx2=herder[idx].score;
				idx3=herder[idx].sheep;
				nWinner=idx;
			}
		}
	}

	// there can be more than 30 in the challenge screens, though, so
	// we'll take that into account. ySpacing will be 30 if there are
	// less than 30, but we'll condense that for every new row to 3 sheep
	// that we need. (so, take 3 off the spacing for every 3 more sheep)
	if (herder[nWinner].sheep <= 30) {
		ySpacing=30;
	} else {
		ySpacing=270/((herder[nWinner].sheep+2)/3);
		if (ySpacing<1) ySpacing=1;
	}

	// in this one we just count off the sheep and stamp them
	// into the final bitmap, since there will rarely be more than about 12.
	for (idx=0; idx<4; idx++) {
		BonusTotal[idx]=0;
	}
	while ((BonusTotal[0]<herder[0].sheep)||(BonusTotal[1]<herder[1].sheep)||(BonusTotal[2]<herder[2].sheep)||(BonusTotal[3]<herder[3].sheep)) {
		GenerateSheepBanner(szBonusBanner);

		DrawBonusScreen(1);	// no scores to process
		DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
		pvr_list_finish();
		pvr_scene_end;

		// delay between sheep
		for (idx=0; idx<4; idx++) {
			int nDelay=nDelayTimer;
			if (gReturnToMenu) {
				sound_stop();
				return -1;
			}
			if (herder[nWinner].sheep > 30) {
				if (nDelayTimer) nDelayTimer--;
			}

			MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
				if (NULL == st) {
					continue;
				}
				// Check for left trigger to slow down bonus screen
				if (st->ltrig>=127) {
					nDelay=100;
				}
				// right trigger to speed it up (used to be start)
				if (st->rtrig>=127) {
					nDelay=0;
				}
			MAPLE_FOREACH_END();
			if (nDelay == 0) break;
			thd_sleep(nDelay);
		}

		if (!inDemo) {
			sound_effect(snd_boom, 225);
		}

		// Don't redraw while the PVR is rendering! (flicker/tear fix)
		waitPVRDone();

		// nasty trick - hold off the next render while we work
		// This isn't *perfect*, but works 99.9% of the time because
		// we SHOULD have a full vsync till the next test		
		pvr_state.render_busy=1;

		for (idx=0; idx<4; idx++) {
			int pos;
			if (BonusTotal[idx]<herder[idx].sheep) {
				pos=BonusTotal[idx];
				BonusTotal[idx]++;

				pSpr=&BonusSheep[0];
				pSpr->txr_addr=txr_sprites;
				pSpr->tilenumber=4;		// white sheep
				pSpr->alpha=255;
				pSpr->z=1000.0-(nTotalBonusSheep/10.0);
				// Calculate grid position
				// We're doing a simple 3 across, max 10 rows from the bottom up
				// Note this *exactly* fits in our memory tile buffer, we can't
				// make it any wider than 128 pixels!
				x1=36*(pos%3);
				y1=ySpacing*(pos/3);

				pSpr->x=(idx*128)+x1+17;
				pSpr->y=(350-y1);
				pSpr->is3D=-1;			// not moving

				DrawSpriteTxr(disc_txr, pSpr);
			}
		}

		// We're done, clear the render flag. The render should begin on the
		// next vsync, unless we raced the PVR code. Even so, should only lose one frame
		pvr_state.render_busy=0;
	}

	// we need to draw one more frame (hacky...)
	GenerateSheepBanner(szBonusBanner);

	DrawBonusScreen(1);	// no scores to process
	DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
	pvr_list_finish();
	pvr_scene_end;

	// delay between sheep
	for (idx=0; idx<4; idx++) {
		int nDelay=50;
		if (gReturnToMenu) {
			sound_stop();
			return -1;
		}
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
			if (NULL == st) {
				continue;
			}
			// Check for left trigger to slow down bonus screen
			if (st->ltrig>=127) {
				nDelay=100;
			}
			// right trigger to speed it up (used to be start)
			if (st->rtrig>=127) {
				nDelay=0;
			}
		MAPLE_FOREACH_END();
		if (nDelay == 0) break;
		thd_sleep(nDelay);
	}

	// check for record if this was a challenge stage
	if (gStoryModeSpecialFlags&EFFECT_WIN_BY_SHEEP) {
		char szBest[64];
		char szTime[64];
		char szNeeded[64];
		int nRecord=0;
		int nPassed=1;

		sprintf(szNeeded, "Required  : %2d", atoi(&szLastStoryCmd[2]));
		sprintf(szTime, "Your score: %2d", (int)herder[gHumanPlayer].sheep);
		sprintf(szBest, "Best score: %2d", gGame.ChallengeScore[level]);

		if (atoi(&szLastStoryCmd[2]) > herder[gHumanPlayer].sheep) {
			nPassed=0;
			nWinner=4;
		}

		if ((herder[gHumanPlayer].sheep > gGame.ChallengeScore[level]) && (nPassed)) {
			nRecord=1;
			gGame.ChallengeScore[level]=herder[gHumanPlayer].sheep;
		}
		
		DoChallengeEffects(nWinner, szBest, szTime, szNeeded, nRecord, nPassed);
	} else {
		ScaleGoldenSheep(nWinner);
	}
	if (gReturnToMenu) {
		return -1;
	}

	return nWinner;
}


// nWinner is 0-3, or -1
// returns the player number who won, or -1 if aborted
// this is the version for challenge timer count-up mode
int HandleTimer(int nWinner, int nTime) {
	int nBest, nRecord, nPassed;
	char szBest[64], szTime[64], szNeeded[64];

	nWinner=gHumanPlayer;
	nRecord=0;
	nBest=0;
	nPassed=1;
	strcpy(szBonusBanner, "");

	if (gStoryModeSpecialFlags&EFFECT_LOOKING_FOR_HADES) {
		sprintf(szTime,   "Your time  : %3d seconds", nTime);
		sprintf(szNeeded, "Time limit : %3d seconds", atoi(&szLastStoryCmd[2]));
		if (atoi(&szLastStoryCmd[2]) < nTimeLeft) {
			nPassed=0;
			nWinner=4;
		}
		nBest=gGame.ChallengeScore[2];
		if (nBest==0) nBest=99;
		sprintf(szBest,   "Best time  : %3d seconds", nBest);
		if ((nTime < nBest)&&(nPassed)) {
			nRecord=1;
			gGame.ChallengeScore[2]=nTime;
		}
	}

	if (gStoryModeSpecialFlags&EFFECT_END_IF_ZAPPED) {
		sprintf(szTime,   "You lasted : %3d seconds", nTime);
		sprintf(szNeeded, "Needed     : %3d seconds", atoi(&szLastStoryCmd[2]));
		if (atoi(&szLastStoryCmd[2]) > nTimeLeft) {
			nPassed=0;
			nWinner=4;
		}
		nBest=gGame.ChallengeScore[4];
		sprintf(szBest, "Best time  : %3d seconds", nBest);
		if ((nTime > nBest)&&(nPassed)) {
			nRecord=1;
			gGame.ChallengeScore[4]=nTime;
		}
	}

	DoChallengeEffects(nWinner, szBest, szTime, szNeeded, nRecord, nPassed);

	if (gReturnToMenu) {
		return -1;
	}

	return nWinner;
}

void DoChallengeEffects(int nWinner, char *szBest, char *szTime, char *szNeeded, int nRecord, int nPassed) {
	int nDelay;

	// Just the bonus banner
	debug("Screen 1...");	
	DrawBonusScreen(1);	// no scores to process
	DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
	pvr_list_finish();
	pvr_scene_end;
	nDelay=1000;
	MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
		if (NULL == st) {
			continue;
		}
		// Check for left trigger to slow down bonus screen
		if (st->ltrig>=127) {
			nDelay=2000;
		}
		// right trigger to speed it up (used to be start)
		if (st->rtrig>=127) {
			nDelay=0;
		}
	MAPLE_FOREACH_END();
	debug(" delay %d.\n", nDelay);
	if (nDelay) thd_sleep(nDelay);

	// Add the required time
	if (!inDemo) {
		sound_effect(snd_boom, 225);
	}
	debug("Screen 1.5...");	
	DrawBonusScreen(1);	// no scores to process
	CenterDrawFontZ(1024.0f, 0, 130, DEFAULT_COLOR, szNeeded);
	DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
	pvr_list_finish();
	pvr_scene_end;
	nDelay=1000;
	MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
		if (NULL == st) {
			continue;
		}
		// Check for left trigger to slow down bonus screen
		if (st->ltrig>=127) {
			nDelay=2000;
		}
		// right trigger to speed it up (used to be start)
		if (st->rtrig>=127) {
			nDelay=0;
		}
	MAPLE_FOREACH_END();
	debug(" delay %d.\n", nDelay);
	if (nDelay) thd_sleep(nDelay);

	// Add the time the user took
	if (!inDemo) {
		sound_effect(snd_boom, 225);
	}
	debug("Screen 2...");
	DrawBonusScreen(1);	// no scores to process
	DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
	CenterDrawFontZ(1024.0f, 0, 130, DEFAULT_COLOR, szNeeded);
	DrawFontZ(1024.0f, 0, 0, -1, DEFAULT_COLOR, "");
	CenterDrawFontZ(1024.0f, 0, -1, DEFAULT_COLOR, szTime);
	pvr_list_finish();
	pvr_scene_end;
	nDelay=1000;
	MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
		if (NULL == st) {
			continue;
		}
		// Check for left trigger to slow down bonus screen
		if (st->ltrig>=127) {
			nDelay=2000;
		}
		// right trigger to speed it up (used to be start)
		if (st->rtrig>=127) {
			nDelay=0;
		}
	MAPLE_FOREACH_END();
	debug(" delay %d.\n", nDelay);
	if (nDelay) thd_sleep(nDelay);

	// Add the best time ever
	if (!inDemo) {
		sound_effect(snd_boom, 225);
	}
	debug("Screen 3...");
	DrawBonusScreen(1);	// no scores to process
	DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
	CenterDrawFontZ(1024.0f, 0, 130, DEFAULT_COLOR, szNeeded);
	DrawFontZ(1024.0f, 0, 0, -1, DEFAULT_COLOR, "");
	CenterDrawFontZ(1024.0f, 0, -1, DEFAULT_COLOR, szTime);
	DrawFontZ(1024.0f, 0, 0, -1, DEFAULT_COLOR, "");
	CenterDrawFontZ(1024.0f, 0, -1, DEFAULT_COLOR, szBest);
	pvr_list_finish();
	pvr_scene_end;
	nDelay=1000;
	MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
		if (NULL == st) {
			continue;
		}
		// Check for left trigger to slow down bonus screen
		if (st->ltrig>=127) {
			nDelay=2000;
		}
		// right trigger to speed it up (used to be start)
		if (st->rtrig>=127) {
			nDelay=0;
		}
	MAPLE_FOREACH_END();
	debug(" delay %d.\n", nDelay);
	if (nDelay) thd_sleep(nDelay);

	// Add 'New Record!' or 'Failed!', if either is true
	if ((nRecord)||(0 == nPassed)) {
		if (!inDemo) {
			sound_effect(snd_boom, 225);
		}
		debug("Screen 4...");
		DrawBonusScreen(1);	// no scores to process
		DrawFont(0, 0, 440, DEFAULT_COLOR, szBonusBanner);
		CenterDrawFontZ(1024.0f, 0, 130, DEFAULT_COLOR, szNeeded);
		DrawFontZ(1024.0f, 0, 0, -1, DEFAULT_COLOR, "");
		CenterDrawFontZ(1024.0f, 0, -1, DEFAULT_COLOR, szTime);
		DrawFontZ(1024.0f, 0, 0, -1, DEFAULT_COLOR, "");
		CenterDrawFontZ(1024.0f, 0, -1, DEFAULT_COLOR, szBest);
		DrawFontZ(1024.0f, 1, 0, -1, DEFAULT_COLOR, "");
		if (nRecord) {
			CenterDrawFontZ(1024.0f, 1, -1, DEFAULT_COLOR, "New Record!");
		} else {
			CenterDrawFontZ(1024.0f, 1, -1, DEFAULT_COLOR, "Failed!");
		}

		pvr_list_finish();
		pvr_scene_end;
		nDelay=2000;
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
			if (NULL == st) {
				continue;
			}
			// Check for left trigger to slow down bonus screen
			if (st->ltrig>=127) {
				nDelay=4000;
			}
			// right trigger to speed it up (used to be start)
			if (st->rtrig>=127) {
				nDelay=1000;
			}
		MAPLE_FOREACH_END();
		debug(" delay %d.\n", nDelay);
		if (nDelay) thd_sleep(nDelay);
	}

	if (nPassed) {
		ScaleGoldenSheepTime(nWinner, szBest, szTime, szNeeded, nRecord);
	}

	if (gReturnToMenu) {
		return;
	}

	if ((gGame.AutoSave) && (nRecord)) {
		doVMUSave(1);
	}

	return;
}

#undef ANIMSPEED
