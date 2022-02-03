/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* storymode.c                          */
/****************************************/

#include <stdio.h>
#include <stdlib.h>

#include "kosemulation.h"

#include "sprite.h"
#include "cool.h"
#include "font.h"
#include "rand.h"
#include "levels.h"
#include "sound.h"
#include "control.h"
#include "sheep.h"
#include "menu.h"		// for globals
#include "storymode.h"
#include "mapscr_manager.h"
#include "menuscr_manager.h"

extern pvr_ptr_t txr_level;
extern pvr_ptr_t txr_misc,pal_misc;
extern int myJiffies;
extern int nFrames;
extern int StoryModeTotalScore;
extern int level, stage;
extern int nMainScrollTarget;
extern int nContinues;
extern int gStageSpecialEffect;
extern unsigned int lastbuttons[4];
extern int gIsContinuingStory;

#define SCORE_Y 42

// used for the crossfading story
#define TXR_SCREENA txr_level
#define TXR_SCREENB (txr_level+(256*192*2))
pvr_ptr_t ptrA, ptrB;

// lives in mapscr_manager - we need the sprite table and no sense wasting RAM on two of them
extern GXOamAttr Sprites[128];

// these are at the bottom of the file
extern char sStory[];

// wipes both textures
void FastClearStoryPics() {
	CLEAR_IMAGE(TXR_SCREENA, 256, 192, 16);
	CLEAR_IMAGE(TXR_SCREENB, 256, 192, 16);
}

// Copies texture B over A during VBlanks
// New version just swaps the pointers
void SafeishCopy() {
	pvr_ptr_t ptrC;
	
	ptrC=ptrB;
	ptrB=ptrA;
	ptrA=ptrC;
}

// based on load_bmp_block_mask
void SafeishLoadBmp(char *fn, pvr_ptr_t pTarg) {
	kos_img_t	img;
	unsigned int nOldFrames;
	int nLeft;
	unsigned char *pSrc;

	// do the full load, then
	nOldFrames=myJiffies;
	debug("Loading %s\n", fn);
	if (bmp_to_img(fn, NULL, &img) < 0) {
		debug("Can't load texture '%s' from file\n", fn);
		return;
	}
	debug("Loaded in %d ticks. Now processing...\n", myJiffies-nOldFrames);
	nOldFrames=myJiffies;

	DC_FlushRange(img.data, img.byte_count);
	
	if (pTarg != INVALID_PTR) {
		nLeft=img.byte_count;
		pSrc=(unsigned char*)img.data;
		while (nLeft > 0) {
			// try to keep the sound running
			sound_frame_update();
			OS_WaitVBlankIntr();
			GX_BeginLoadTex();
				if (nLeft > SAFE_COPY_BYTES_PER_VBLANK) {
					GX_LoadTex(pSrc, pTarg, SAFE_COPY_BYTES_PER_VBLANK);
				} else {
					GX_LoadTex(pSrc, pTarg, nLeft);
				}
			GX_EndLoadTex();

			nLeft-=SAFE_COPY_BYTES_PER_VBLANK;
			pSrc+=SAFE_COPY_BYTES_PER_VBLANK;
			pTarg+=SAFE_COPY_BYTES_PER_VBLANK;
		}
	}

	debug("Done in %d ticks.\n", myJiffies-nOldFrames);
	
	kos_img_free(&img);
	
	return;
}

// return 0 is failure, first screen # is 0001
int FindPhase(int nLevel) {
	char *pText;
	
	// a 'phase' is a screen number
	// Seeks through the list to find the phase number that marks
	// the beginning of the 1-based level passed (1-8)
	pText=sStory;

	for (;;) {
		// check for end of data
		if ((*pText=='`') && (*(pText+1) == '`')) {
			return 0;
		}
		
		// Check if we have it
		if (*pText == '`') {
			if (*(pText+1) == 'L') {
				if ((*(pText+2))-'1' == nLevel) {
					break;
				}
			}
		}
		
		// skip to next line
		while (*pText != '\0') pText++;
		pText++;
	}
	
	// Next line should be it, first four contain the digits
	// skip to next line
	while (*pText != '\0') pText++;
	pText++;
	
	// if we are continuing, skip to the first title card for that level
	// that will be the first empty text. Hell has no title card
	if ((gIsContinuingStory)&&(level != LEVEL_UNDERWORLD)) {
repeat_search:		
		for (;;) {
			// check for end of data
			if ((*pText=='`') && (*(pText+1) == '`')) {
				return 0;
			}
			
			// Check if we have it
			if (*pText != '`') {
				if (*(pText+4)=='\0') {
					// should be right
					break;
				}
			}
			
			// skip to next line
			while (*pText != '\0') pText++;
			pText++;
		}
		// Toy Factory has two title cards
		if ((level == LEVEL_TOY)&&(gIsContinuingStory)) {
			gIsContinuingStory=0;
			// skip to next line
			while (*pText != '\0') pText++;
			pText++;
			// skip to next line
			while (*pText != '\0') pText++;
			pText++;
			goto repeat_search;
		}
			
		gIsContinuingStory=0;	// only once
	}

	return atoi(pText);
}

// Show the story sequence for nPhase. 
// Also set up the computer herder types for each stage.
// Returns instructions for the game's next level
// nInPhase tracks where we are (updated on return)
// cLastCmd is used to chec for skip ('#' to skip)
// menu must already be open for updating text
// start is disabled during the end story (level 7)

/* OBJ Character 'A' button */
const unsigned int d_64_256_obj_schDT[16 * 16 / 2 / 4] = {
	// sketched in photoshop and converted by hand
	0xfff00000,
	0xf64ff000,
	0xef642f00,
	0xef8532f0,
	0xfef642f0,
	0xcef7421f,
	0xcfef421f,
	0xffef421f,
	
	0x00000fff,
	0x000ff99f,
	0x00f9aafe,
	0x0f9abbfe,
	0x0f9bbfef,
	0xf8abcfec,
	0xf8abfefc,
	0xf79bfeff,
	
	0xeeef321f,
	0xffef211f,
	0x7fef211f,
	0x5fef11f0,
	0x3fef11f0,
	0x11f11f00,
	0x111ff000,
	0xfff00000,
	
	0xf68afeee,
	0xf579feff,
	0xf356fef9,
	0x0f34fef6,
	0x0f22fef3,
	0x00f12f22,
	0x000ff111,
	0x00000fff
};

/* Palette Data for the 'A' */
#define PACKPAL(y,x)	\
	( (((x&0xff00)>>11)<<16) | (((x&0xff0000)>>19)<<21) | (((x&0xff000000)>>27)<<26) | 		\
	  (((y&0xff00)>>11)) | (((y&0xff0000)>>19)<<5) | (((y&0xff000000)>>27)<<10) )

const unsigned int d_64_256_obj_sclDT[16 * 2 / 4] = {
	PACKPAL(0xFF3F3F00,0xFF414100),
	PACKPAL(0xFF454500,0xFF4B4B00),
	PACKPAL(0xFF4E4E00,0xFF515100),
	PACKPAL(0xFF565600,0xFF595900),
	PACKPAL(0xFF5D5D00,0xFF626200),
	PACKPAL(0xFF6B6B00,0xFF777700),
	PACKPAL(0xFF858500,0xFFFFFF00),
	PACKPAL(0xC3C3C300,0x00000000)
};

void initStory() {
	ptrA=TXR_SCREENA;
	ptrB=TXR_SCREENB;
	FastClearStoryPics();
}

char *doStory(int *nInPhase, char cLastCmd) {
	char *pText;
	int idx;
	int isGame;
	int nPhase=*nInPhase;
	char buf[128];
	int nDarken;
	char cLast='.';
	int nOption=0;
	int fAutoAdvance;
	int xStart=0;		// once set to CONT_START, we skip through the rest
	char bReopenMenu = 0;
	char bDisableStart = 0;
	
	// hack to disable start skipping the story on the end of game story
	if (level == MAX_LEVELS) {
		bDisableStart = 1;
	}

	if (cLastCmd == '#') 
	{
		xStart = 1;		// skip all
	}
	else 
	{
		// Ensure the story mode hardware is ready
		nMainScrollTarget = SCROLL_TARG_NONE;		// disable menu scroll system
	}

	isGame=1;
	pText = sStory;

	// Find start of text for this screen
	for (;;) {
		if (*pText!='`') {
			int nval=atoi(pText);
			if (nval == *nInPhase) {
				break;
			}
			// account for skipped boards
			if (nval-1 == *nInPhase) {
				*nInPhase=nval;
				nPhase=nval;
				break;
			}
		} else {
			if (*(pText+1)=='`') {
				debug("Bad phase - not found\n");
				return "Q";
			}
		}
		while (*pText != '\0') {
			pText++;
		}
		pText++;
	}

	if (!xStart) 
	{
		// prepare the 'A' sprite
	    GXS_LoadOBJ(d_64_256_obj_schDT, 0, sizeof(d_64_256_obj_schDT));
	    GXS_LoadOBJPltt(d_64_256_obj_sclDT, 0, sizeof(d_64_256_obj_sclDT));
        G2_SetOBJAttr(&Sprites[0],  // Pointer to the attributes
              228,             		// X
              164,             		// Y
              0,               		// Priority
              GX_OAM_MODE_NORMAL,   // OBJ mode
              FALSE,           		// Mosaic 
              GX_OAM_EFFECT_NONE,   // Flip/affine/no display/affine(double)
              GX_OAM_SHAPE_16x16, 
              GX_OAM_COLORMODE_16,
              0,               		// Character name
              0,               		// Color param
              0                		// Affine param
        );    
        DC_FlushRange(Sprites, sizeof(Sprites));
        GXS_LoadOAM(Sprites, 0, sizeof(Sprites));

		// level is 512x512x8 - we load two 15-bit truecolors for
		// the purpose of cross fade. We always load to SCREENB then copy to SCREENA
		sprintf(buf, "gfx/Story/%04d.bmp", nPhase);
		SafeishLoadBmp(buf, ptrB);

		if (*(pText+4) != '\0') {
			// remember there is a second copy of this text parser after the keypress handling code
			char *pOffset = pText+4;

			if ('+' == *(pOffset)) {
				fAutoAdvance=1;
				pOffset++;
			} else {
				fAutoAdvance=0;
			}
			if ('*' == *(pOffset)) {
				// resize to larger window for stage description
				MenuScr_UpdateBubbleSize(0, 14, 10);
				pOffset++;
			}
			if ('$' == *(pOffset)) {
				int r,g,b;
				u16 val;
				r=*(pOffset+1)-'0';
				g=*(pOffset+2)-'0';
				b=*(pOffset+3)-'0';
				if (r>9) r-=7;
				if (g>9) g-=7;
				if (b>9) b-=7;
				val = (b<<10)|(g<<5)|(r);
				*(u16*)(HW_DB_BG_PLTT+258) = val;	// set color
				*(u16*)(HW_DB_BG_PLTT+264) = val;	// set color
				pOffset+=4;
			}
			MenuScr_UpdateMenuString(0, pOffset);
		} else {
			MenuScr_UpdateMenuString(0, "");
			MenuScr_CloseMenu(0);
			bReopenMenu = 1;
			fAutoAdvance=1;
		}
			
		// fade gfx in by crossfading from TXR_SCREENA to TXR_SCREENB
		for (nDarken=1; nDarken<32; nDarken+=4) {
			pvr_scene_begin();
			
			// fading out
			addPageStory(ptrA, (fx16)96, 32-nDarken);
			// fading in
			addPageStory(ptrB, (fx16)95, nDarken);
			
			// Draw the text cloud
			MenuScr_DrawFrame(&nOption);
			
			// draw it
			pvr_scene_finish();
		}

		SafeishCopy();
	}
	
	// track the current tile - the last digit always changes so that's enough
	cLast=*(pText+3);
	
	// loop for whole speech
continueStoryMode:
	for (;;) {
		idx=10;

		if (!xStart) 
		{
			if (!fAutoAdvance) {
				// display the little 'A' character as a sprite (it's already onscreen, we just enable the layer)
				GXS_SetVisiblePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ);
			}
		
			// wait for user input
			for (;;) {
				// draw scene
				pvr_scene_begin();
				
				addPageStory(ptrA, 96, 31);
				
				// Draw the text cloud
				xStart=MenuScr_DrawFrame(&nOption);
			
				// draw it
				pvr_scene_finish();

				if (!xStart) {
					xStart=isStartPressed();
				}
				if (xStart) {
					break;
				}
				if (fAutoAdvance) {
					idx--;
					if (idx < 1) break;
				}
			}
			
			GXS_SetVisiblePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3);

			// wait for it to be released
			DisableControlsTillReleased();
			
			// only reset it if Start wasn't the key pressed
			if ((xStart != CONT_START)||(bDisableStart)) xStart = 0;
		}

		// Find next text for this screen
		while (*pText != '\0') {
			pText++;
		}
		pText++;
		
		if ((*pText == '`') || (*(pText+3)!=cLast)) {
			// done this one
			break;
		}
		
		if (*(pText+4) != '\0') {
			char *pOffset = pText+4;
	
			if ('+' == *(pOffset)) {
				fAutoAdvance=1;
				pOffset++;
			} else {
				fAutoAdvance=0;
			}
			if ('*' == *(pOffset)) {
				// resize to larger window for stage description
				MenuScr_UpdateBubbleSize(0, 14, 10);
				pOffset++;
			}
			if ('$' == *(pOffset)) {
				int r,g,b;
				u16 val;
				r=*(pOffset+1)-'0';
				g=*(pOffset+2)-'0';
				b=*(pOffset+3)-'0';
				if (r>9) r-=7;
				if (g>9) g-=7;
				if (b>9) b-=7;
				val = (b<<10)|(g<<5)|(r);
				*(u16*)(HW_DB_BG_PLTT+258) = val;	// set color
				*(u16*)(HW_DB_BG_PLTT+264) = val;	// set color
				pOffset+=4;
			}
			MenuScr_UpdateMenuString(0, pOffset);
		} else {
			if (!xStart) MenuScr_UpdateMenuString(0, "");
			fAutoAdvance=1;
		}
	}
	
	// reopen the menu as needed
	if (bReopenMenu) {
		MenuScr_InitMenu(MENU_STORY_TEXT);
	}

	// increment the phase for the user
	(*nInPhase)++;

	if (*pText != '`') {
		if (xStart) 
		{
			return "#";		// skip story
		}
		else 
		{
			return "";		// continue normally
		}
	}

	pText++;
	
	// Now we're pointing at the command 
	// Set up the computer herders for this level
	// defaults - just in case of an error
	for (idx=0; idx<4; idx++) {
		if (idx == gHumanPlayer) {
			continue;
		}
		// type will be merged in below
		herder[idx].type=PLAY_COMPUTER;
		if ((idx==0)||((idx==1)&&(gHumanPlayer==0))) {
			herder[idx].color = rand()%16;
		} else {
			herder[idx].color = (herder[idx-1].color+1)&0x0f;
		}
	}

	// First index to set
	idx=0;
	if (idx==gHumanPlayer) idx++;

	// set default game settings (options don't override)
	gOptions.Timer=60;
	gOptions.Rounds=1;
	gOptions.Powers=POW_POWERUPS | POW_SPECIALS;
	gOptions.Skill=1;						// normal
	gOptions.SheepSpeed=0;					// normal 

	herder[gHumanPlayer].type=PLAY_HUMAN|HERD_ZEUS;
	
	if (*pText == 'H') {
		idx=0;
		if (idx==gHumanPlayer) idx++;

		herder[idx].type|=HERD_HERD;
		idx++; if (idx==gHumanPlayer) idx++;
		herder[idx].type|=HERD_HERD;
		idx++; if (idx==gHumanPlayer) idx++;
		herder[idx].type|=HERD_HERD;
	} else if (*pText == 'S') {
		idx=0;
		if (idx==gHumanPlayer) idx++;

		switch (*(pText+1)-'1') {
		case LEVEL_NZ:
			herder[idx].type|=HERD_HERD;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_HERD;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_HERD;
			break;

		case LEVEL_CANDY:
			herder[idx].type|=HERD_CANDY;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_CANDY;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_CANDY;
			break;

		case LEVEL_HAUNTED:
			herder[idx].type|=HERD_ZOMBIE;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_ZOMBIE;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_WOLF;
			break;

		case LEVEL_TOY:
			herder[idx].type|=HERD_NH5;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_NH5;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_THALIA;
			break;

		case LEVEL_DISCO:
			herder[idx].type|=HERD_DANCER;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_DANCER;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_GODDANCE;
			gStageSpecialEffect |= STAGE_EFFECT_NO_PLAYER_SPECIAL;
			MapScr_SetSpecialText("Special Disabled");
			break;

		case LEVEL_WATER:
			// Deliberate overwrite with PLAY_NONE here - only you vs Iskur first round
			idx=0;
			if (idx==gHumanPlayer) idx++;
			if (*(pText+4) == '+') {	// note: may overrun into next text if command string is short. Not a problem but be aware.
				// 3 way Iskur ;)
				herder[idx].type|=HERD_ISKUR|PLAY_SPECIAL_CLONE;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type|=HERD_ISKUR|PLAY_SPECIAL_CLONE;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type|=HERD_ISKUR|PLAY_SPECIAL_POWER;
				gStageSpecialEffect |= STAGE_EFFECT_SPECIAL_TO_WIN;
				MapScr_SetSpecialText("Shock REAL Iskur");
			} else {
				herder[idx].type=PLAY_NONE;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=PLAY_NONE;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type|=HERD_ISKUR;
				MapScr_SetSpecialText("  Defeat Iskur  ");
			}
			break;
			
		case LEVEL_UNDERWORLD:
			// you versus the demon, but he's powered up
			// Deliberate overwrite with PLAY_NONE here - only you vs the Demon
			idx=0;
			if (idx==gHumanPlayer) idx++;
			herder[idx].type=PLAY_NONE;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type=PLAY_NONE;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type|=HERD_DEVIL|PLAY_SPECIAL_POWER;
			gStageSpecialEffect |= STAGE_EFFECT_CLEAR_SHEEP_TO_WIN;
			gOptions.Skill++;		// smarter enemy
			gOptions.Timer = 92;	// full length of the music ;)
			MapScr_SetSpecialText("Catch ALL sheep!");
			break;
			
		default: debug("Story mode error - bad level %d (unless ending!)\n", nPhase/4);
			break;
		}
	}

	return pText;
}

// to save memory lost to padding, this is a 1d array
// of nul-delimited strings. 
// You'll have to seek through it for the one you want.
// No commas in this list! ;) Must manually NUL terminate!
char sStory[] = {
// Codes - start with `, anything else is text
// Lx 	- start point for level x
// H	- How to play (special wrapper)
// Mx   - play minigame 'x' (replaces 'C')
// Sxyz+ - play normal level x, out of y rounds, on stage z, for sheep (plus uses Iskur's clones) (z & + are optional)
// E - end of level
// Q - end game (play end credits)
// ` - end of data
// All text starts with four digits that indicate the slide to be displayed, ignore whitespace
// after the digits.
// Text starting with a '+' will autoadvance, as will frames with no text
// Text starting with a '*' will enlarge the text window
// Text starting with a '$xxx' will set the text color before display (beginning only!)

"`L1\0"
"0000$066Press A to advance story\nor START to skip\0"
"0001$000In New Zealand...\0"
"0002...before it was new...\0"
"0003...it was a typical\nday...\0"
"0004...for most of it.\0"
"0005But,\0"
"0006things would become far\nless typical,\0"
"0007for one boy in\nparticular.\0"
"0008\0"
"0009\0"
"0010$0FF\"Dear Zeus:\nI've taken most of the\nsheep from your pathetic\nlittle village.\"\0"
"0010\"If you want them\nreturned, seek them out\nand take them back\nyourself.\"\0"
"0010\"Your nemesis,\nIskur\"\0"
"0011$00FIskur?\nWhat could he want?\0"
"0012He's trying to throw me\noff my game!\0"
"0013I'll practice harder!\nThat'll show him!\0"
"0014$F0F...\0"
"`H\0"			// how to play
"`E\0"
"`L2\0"
"0015$00FDo you really think Iskur\nwas serious, Chrys?\0"
"0015Could he sheepnap a whole\nherd?\0"
"0016+$F0F!!!\0"
"0017$00FWhat?\0"
"0017You think so?\0"
"0018I guess you have a point\nthere.\0"
"0018No harm in checking it\nout, right?\0"
"0019\0"
"0020\0"
"0021$00FHoly Doodle!\0"
"0021These tracks go on\nforever!\0"
"0022Well, almost forever...\0"
"0023They just lead up to the\nbanks of this pink river.\0"
"0024Wait a minute...\0"
"0024...pink river?\0"
"0024That means we've made it\nto...\0"
"0025The Candy Shoppe!\0"
"0026Maybe they know something\nabout the sheepnapping.\0"
"0027Uh...\nWhat's that?\0"
"0028+$F0F-burble-\0"
"0028+-BLORP-\0"
"0029+-beh-\0"
"0029$00F+Oh my!\0"
"0030+$F0F-beh!-\0"
"0031+-beh-\0"
"0032+-BEH!-\0"
"0033$00FYecch!\nCandy-coated sheep!\0"
"0033The candy stripers won't\nbe happy about this.\0"
"0034Oh no.\0"
"0034They REALLY don't look\nhappy.\0"
"0035+eep!\0"
"0036+ack!\0"
"0037+My arm!!\0"
"0038Wait! WAIT!!\0"
"0038I can explain!\0" 
"0038*$F00Show the candy stripers\nthat you are the best\nherder for the job! Be\ncareful! The sheep are on\na sugar rush and can not\nbe stunned!\0"
"`S21\0"
"`E\0"
"`L3\0" 
// 0039 and 0040 have the same image to let us
// do the bubble and show the change in speaker
"0039$00F... and that's how it is.\nYou believe me, right?\0"
"0040$0F0Absolutely!\0"
"0040No one as good as you at\nherding would ever leave\nso many sheep behind.\0"
"0041There was a guy hanging\naround last night.\0"
"0041A big guy, though...\n...with a sheep for a\nhead.\0"
"0042$00FBig guy...\0"
"0043..with a sheep for a\nhead?\0"
"0043That's not something\nIskur would do.\0"
// Panel 0044 lost between ink and color
//"0044I don't know what he was\ndoing...\0"
//"0044...but I may know where\nhe was going.\0"
"0045$0F0He was heading towards\nold man Hades' place.\0"
"0046$00FO-Old Man Hades?!\0"
"0046That guy's name is\nsynonymous with death!\0"
"0047I guess we don't have\nmuch choice, Chrys.\0"
"0047On to our doom!\0"
"0048\0"
"0049\0"
"0050$00FBoy, it sure got dark\nquick!\0"
"0050But no need to worry...\0"
"0051...right, Chrys?\0"
"0052...Chrys?\0"
"0053CHRYSOMALLOS!!\0"
"0053WAIT UP!!\0"
"0054I'm right behind you,\nbuddy!\0"
"0055-huff-\n... I ... finally ...\0"
"0055-gasp-\n... caught up ... with\n...\0"
"0056EEP!\0"
"0057Old Man Hades' place!\0"
"0058Aww, man...\0"
"0058I'm too pretty to die!\0"
"0058*$F00These haunting sheep can\nbe captured only when\nstunned. A captured ghost\nsheep can only be freed\nfrom another herder by\na special attack.\0"
"`S31\0"
"`E\0"
"`L4\0"
"0059$665Are they still calling me\n\"Death\"?\0"
"0059That's so sad.\0"
"0060You see, I used to be a\nsheep addict.\0"
"0060I ate a lot of mutton. A\nlot.\0"
"0061And now, I'm haunted by\nthe ghosts of the sheep I\nate.\0"
"0062But that's not why you're\nhere, is it?\0"
"0062No relief for a bad old\nwolf.\0"
"0063Down in the city is a\nDisco.\0"
"0063It's run by Trey Volta,\nthe self-proclaimed God of\nDance.\0"
"0064The only problem is, he\ndoesn't like herders.\0"
"0064So.. you're going to have\nto sneak in.\0"
"0065$00F... Sneak in?\0"
"0066$F0F...\0"
"0067\0"
"0068\0"
"0069$00FCool!\nYou can see the whole\ncity from here!\0"
"0069$880-WAIT!-\0"
"0069$00FWha-?\0"
"0070$880WAIT!\0"
"0070ZEUS!!\0"
"0071\0"
"0072\0"
"0073\0"
"0074$00FThalia!\0"
"0074What's up?\0"
"0075$880Sheep, Zeus!\0"
"0075Sheep EVERYWHERE!\0"
"0076They're all over my\nbeautiful factory!\0"
"0076My robots weren't\ndesigned to handle that\nmuch woolly doom!\0"
"0077But you're a herder...\0"
"0077... you can handle this,\ncan't you?\0"
"0078$00Fuh...\0"
"0079$F0F+-shake-\0"
"0080$00F...sure?\0"
"0081$880Oh, thank you!\0"
"0081I'll bet you're better\nthan ever!\0"
"0081*$F00Some of the sheep are\nhiding in boxes,\nmake sure you get them all!\0"
"`S41\0"
"`E\0"
"`L5\0"
// Panel 0082 lost between sketch and ink
//"0082WOW!\0"
"0083$880You've got some crazy\ntalent there, Zeus!\0"
"0083Simply amazing.\0"
"0083Thank you!\0"
"0084$000+-KISS-\0"
"0085$000+* * *\0"
"0086$880Zeus?\0"
"0087Are you OK?\0"
"0087You look a little\nflushed...\0"
"0088$00FNo no no!\0"
"0088I'm Fine!\nI'm GREAT!\0"
"0088Good good good good good!\0"
"0089We do have to get going,\nthough.\0"
"0089There's a sheepnapper on\nthe loose.\0"
"0090\0"
"0091\0"
"0092...\0"
"0092$00FHaven't we been this way\nalready?\0"
"0092Well, while we're here...\0"
"0092...we may as well try on\nthe disguises Thalia gave\nus.\0"
"0093OOH!\nThese are PERFECT!\0"
"0093No one will ever\nrecognise us!\0"
"0094$000+-POM-\0"
"0095$00FOk, Chrys.\0"
"0095Let's dance!\0" 
"0096\0"
"0097+$000-gasp-\0"
"0098+$008-step-step-\0"
"0099+$030What the...?\0"
"0100+$808-patter-patter-\0"
// Panel 0101 lost between sketch and ink
//"0101Here we are...\0"
"0102$00FAre you ready, Chrys?\0"
"0103$088Dudes!\nCool 'fros!\0"
"0104Too bad one of my friends\nturned out to be a herder.\0"
"0104There are sheep all over\nthe place!\0"
"0105I had to close the Disco,\ndudes.\0"
"0105We have to clean up the\nmess.\0"
"0106$00FClosed!?\0"
"0106Can we help?\0"
"0107$088Dude, if your moves are\nanywhere near as cool as\nyour looks...\0"
"0108...we're saved!!\0"
"0108*$F00Using your distinctive\nspecial would give you\naway, so you will have\nto do without specials\nfor this round.\0"
"`S51\0"
"`E\0"
"`L6\0"
"0109$00FOh...\0"
"0109Oh no.\0"
"0110$088AAAAAAH!\0"
"0110You're a HERDER?!\0"
"0111Such grace...\n...such syle...\0"
"0111...there's just no way!\0"
"0112$00FIs THAT why you hate\nherders?\0"
"0113$088Of course!\0"
"0113The God of Dance can't\nafford to be associated\nwith such ruffians!\0"
"0114$00FWhat about Iskur, then?\0"
"0115$088Wha-?\nIskur...?\0"
"0116$00F...guy with a sheep for a\nhead?\0"
"0117$088Oh! That guy!\0"
"0118His head really was a\nsheep, huh?\0"
"0118I thought he was trying\nto be avante-garde.\0"
"0119$00FAvante-garde?\0"
"0119How?\0"
"0120+$088...\0"
"0121Anyway, I sent him\npacking.\0"
"0121He's gone to the\nwaterworks.\0"
"0122$00FThe Styx?\0"
"0123\0"
"0124\0"
"0125$088The Styx is that way,\ndudes.\0"
"0125Shouldn't take you too\nlong to get there.\0"
"0126$00FThanks, man.\0"
"0126I'll shock Iskur good for\nyou.\0"
"0127+$088...\0"
"0128Oh yeah,\nthey're doomed.\0"
"0129$00FI am so glad to be out of\nthat city...\0"
"0129...it gives me the\nheebie-jeebies!\0"
"0130With that behind us,\0"
"0130we can finally make some\nprogress!\0"
"0131Hmm...\0"
"0132It should be down there.\0"
"0132...somewhere.\0"
"0133All we have to do is find\na safe way...\0"
"0134DOOOWN!\0"
"0135AAAAAGH!\0"
"0136OH NO!\0"
"0136NO NO NO NO NO!!!\0"
"0137What are all these pipes\ndoing here?\0"
"0138$000+-pok-\0"
"0139$00F+Geh.\0"
"0140\0"
"0141+$000-BLOOSH-\0"
"0142$F0FBaa!\0"
"0143\0"
"0144\0"
"0145$F0FBaa!\0"
"0146$00FChrys?\0"
// Panel 0147 lost between sketch and ink
//"0147Chrys!\0"
"0148$F0FBAAAAA!!\0"
"0149$00FHuh?\0"
"0149Behind me?\0"
"0150...Iskur?\0"
"0150*$F00It's one on one with\nIskur to prove who is\nthe best herder after\nall, so go all out!\0"
"`S611\0"
"0151$00F...\0"
"0152-huff-\n-huff-\0"
"0153Iskur...\0"
"0153You've really bulked up,\nman.\0"
"0154It's not as easy to beat\nyou as it used to be.\0"
"0155$0FFOh!\0"
"0155I see!\0"
"0156It's a challenge you\nwant...\0"
"0157...so be it.\0"
"0158$00FHmpf!\0"
"0158Bring it on!\0"
"0158*$F00Use your special on\nthe REAL Iskur BEFORE\nall sheep are collected\nto put an end to this\nnonsense!\0"
"`S613+\0"		// stage '3' is important, it fakes an additional increment into the level display
"`E\0"
"`L7\0"
"0159$0FFGyaaaa!\0"
"0160$000-THUD-\0"
"0161$00FHeh!\0"
"0161That wasn't so hard.\0"
"0162$A11GRAAARGH!\0"
"0163You JERK!\0"
"0163You'll pay for this!\0"
"0164$00Fa...\0"
"0164...a talking sheep?\0"
"0165$A11No!\0"
"0165Not a sheep, you\nnumbskull!\0"
"0166+I...\0"
"0167+...am...\0"
"0168+...your...\0"
"0169DOOM!\0"
"0169*$F00Looks like something was\nbehind Iskur's strange\nbehaviour after all! You\nmust use your special and\ncapture ALL the sheep\nbefore time runs out to\ndefeat him!\0"
"`S713\0"		// stage '3' is important, it fakes an additional increment into the level display
"`E\0"
"`L8\0"
"0170+$A11-cough-\n-hack-\0"
// Panel 0171 lost between sketch and ink
//"0171+EEP!\0"
"0172-GULP!-\0"
"0173$00FWhy did you try to kidnap\nChrys?\0"
"0174$A11It... it's starting to\nget cold on the other\nside...\0"
"0174I just wanted to keep\nwarm!\0"
"0175And everyone knows that\nChrys' wool is the best.\0"
"0175\"Worth its weight in\ngold,\" they say.\0"
"0176So I says to myself:\0"
"0176\"Wouldn't she make a nice\ncardigan sweater?\"\0"
"0177$00FAll that trouble, the\ntrickery...\0"
"0177...the masses of stolen\nsheep...\0"
"0177...just so you can\nhave...\0"
"0178A SWEATER?!\0"
"0179$A11A NICE sweater.\0"
"0180$000-FZAKK-\0"
"0181$00FSilly demon.\0"
"0181Ok, Chrys.\nLet's get you out of\nthere.\0"
"0182Thank goodness you're ok!\0"
"0182$F0FBaa!\0"
"0183$00F...\0"
"0184Oh yeah!\nIskur!\0"
"0185+$0FF-unnh-\0"
"0186-groan-\0"
"0187$00FNow that's the skinny guy\nI remember...\0"
"0188$0FFHuh?\0"
"0188Where am I?\0"
"0189Zeus?\nChrys?\0"
"0189What's going on?\0"
"0190$00FJust a slight case of\ndemonic possession, chum.\0"
"0190Nothing to worry about.\0"
"0191At any rate, there're\nstill some sheep on the\nloose.\0"
"0192What do you say, chum...\0"
"0192...how about a little\npost-traumatic herding?\0"
"0193$0FFWell...\0"
"0193It's not like I have\nanything better to do...\0"
"0194+$F0FBaa!\0"
"0195$000Cool Herders!\0"
"`Q\0"
"```\0"
};

char szJustHereForTheRandomROMReader[] = "All your sheep are belong to me!";
