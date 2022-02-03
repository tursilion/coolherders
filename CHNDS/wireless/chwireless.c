// Modified for Cool Herders interface. 
// Be really careful not to call debug() in functions that are called via interrupt
// contexts (I think there's only one, and we should just move it to chwh.c).
// debug() doesn't get along with that state at all, and causes random crashes.

#ifdef SDK_TWL
#include    <twl.h>
#else
#include    <nitro.h>
#endif
#include    <nitro/wm.h>
#include <string.h>

#include "kosemulation.h"
#include "sprite.h"
#include "cool.h"
#include "menu.h"
#include "chwh.h"
#include "chwireless.h"
#include "menuscr_manager.h"
#include "rand.h"
#include "control.h"
#include "sound.h"
#include "levels.h"
#include "font.h"
#include "menu.h"
#include "sheep.h"

extern OSOwnerInfo OwnerInformation;
extern struct multiplay_options gMPStuff;
extern int level, stage;
extern pvr_ptr_t txr_level;
extern pvr_ptr_t txr_misc,pal_misc;
extern pvr_ptr_t txr_herder[6];
extern pvr_ptr_t pal_level, pal_herder[6];
extern char szNames[13][8];
extern unsigned int lastbuttons[4];
extern int sSysState;

unsigned int gGameSeed = 0;

// from chwh.c
extern "C" {
	void WH_ChangeSysState(int state);
	BOOL WH_StateInStartScan(void);
	BOOL WH_StateInEndScan(void);
}

/*******************
Sorry - some proprietary code was removed
********************/

// returns 0 on failure or 1 on success
int EndScanning() {
    WaitForNotBusy();

    if (!WH_StateInEndScan())
    {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return 0;
    }
    
    return WaitForNotBusy();
}

// actually connect to a parent in question
// 0 on failure, 1 on success
int ConnectChild(u8 MAC[6]) {
    if (!WH_ChildConnectAuto(WH_CONNECTMODE_DS_CHILD, MAC, 0)) {
    	debug("WH_ChildConnectAuto failed.");
    	return 0;
    }	
    return 1;
}

// loads a command and up to 6 bytes of data into the output array
// pData must be AT LEAST 6 bytes long if not NULL!!
void SetSharedData(int cmd, char *pData) {
    // Edit send data - always loads the cmd last!
    
    // gSendBuf is an array of u16s
    if (NULL == pData) {
    	gSendBuf[1] = 0;
    	gSendBuf[2] = 0;
    	gSendBuf[3] = 0;
    } else {
    	// note the data block is memcpy'd! Endianess will matter.
    	memcpy(&gSendBuf[1], pData, 6);
    }
    // setting the cmd byte last prevents a race condition where a
    // command is recognized before the data is loaded
    gSendBuf[0] = (unsigned short)((cmd<<8)|WH_GetCurrentAid());
}

// Controller inputs actually don't queue, they just go out directly
// but the name is preserved for consistency (actually nothing queues now)
void QueueCtrl(int btns, int nFrame) {
	// btns are a 16-bit value, and we strip out the unnecessary stuff
	static int nLastFrame = 0;
	char data[6];
	
	if ((nFrame == 0) || (nFrame != nLastFrame)) {
		memset(data,0,sizeof(data));
		btns&=PAD_ALL_MASK;
		data[0]=(char)((btns>>8)&0xff);
		data[1]=(char)((btns)&0xff);
		data[2]=(char)((nFrame>>8)&0xff);	// 16-bits gives over 1000 seconds before loop, our max is 90.
		data[3]=(char)(nFrame&0xff);
	
		SetSharedData(NET_CMD_CTRL, data);
		nLastFrame = nFrame;
	}
}

void QueueSeed(unsigned int seed) {
	char data[6];
	
	memset(data,0,sizeof(data));
	memcpy(data, &seed, sizeof(seed));
	
	SetSharedData(NET_CMD_SEED, data);
}

// Name may queue two messages if needed.
void QueueName(char *pszName) {
	char data[6];
	
	memset(data,0,sizeof(data));
	
	// 5 chars or less
	strncpy(data, pszName, 5);
	SetSharedData(NET_CMD_NAME, data);
}

void QueueName2(char *pszName) {
	char data[6];
	
	memset(data,0,sizeof(data));
	
	strncpy(data, pszName+5, 5);
	SetSharedData(NET_CMD_NAME2, data);
}

void QueueWorld(int level, int stage) {
	char data[6];
	
	memset(data,0,sizeof(data));
	data[0]=(char)(level&0xff);
	data[1]=(char)(stage&0xff);
	
	SetSharedData(NET_CMD_WORLD, data);
}

void QueueCfg(int powers, int specials, int cpu, int sheep, int skill, int timer) {
	char data[6];
	
	memset(data,0,sizeof(data));
	data[0]=(char)(powers?1:0);
	data[1]=(char)(specials?1:0);
	data[2]=(char)(cpu?1:0);
	data[3]=(char)(sheep&0xff);
	data[4]=(char)(skill&0xff);
	data[5]=(char)(timer&0xff);
	
	SetSharedData(NET_CMD_CFG, data);
}

void QueueChar(int who, int color) {
	char data[6];
	
	memset(data,0,sizeof(data));
	data[0]=(char)(who&0xff);
	data[1]=(char)(color&0xff);
	
	SetSharedData(NET_CMD_CHAR, data);
}

void QueueReady() {
	char data[6];
	
	memset(data,0,sizeof(data));
	
	SetSharedData(NET_CMD_READY, data);
}

void QueueNotReady() {
	char data[6];
	
	memset(data,0,sizeof(data));
	
	SetSharedData(NET_CMD_NOT_READY, data);
}

void QueueHostQuit() {
	char data[6];
	
	memset(data,0,sizeof(data));
	
	SetSharedData(NET_CMD_HOST_QUIT, data);
}

void QueueGameReady() {
	char data[6];
	
	memset(data,0,sizeof(data));
	
	SetSharedData(NET_CMD_GAME_READY, data);
}

// parse a received command for player idx (except for control)
int ProcessCommand(int idx) {
	u16 *p;
	int cmd, tmp;
	unsigned char data[6];
	
    // Get data from terminal with ID of 'idx'
    p = WH_GetSharedDataAdr((unsigned short)idx);

    if (p == NULL) {
    	// we should have seen data!
    	debug("!!** Error, could not get data but the bitmask said we should, index %d", idx);
    	return -1;
    }

    // Copy buffer
    MI_CpuCopy8(&p[1], data, 6);
	cmd = (p[0]>>8)&0xff;
	
	// these commands are processed even if we sent them to ourselves
	switch (cmd) {
		case NET_CMD_CTRL:
			gNetworkControlFrame[idx] = (data[2]<<8)|data[3];
			if (gNetworkControlFrame[idx] == gLocalFrame) {
				// don't assign the control until we're on the right frame
				gNetworkControl[idx]=(data[0]<<8)|data[1];
			}
//			debug("Machine %d sent CTRL 0x%04X, frame %5d", idx, gNetworkControl[idx], gNetworkControlFrame[idx]);
			return cmd;
			
		case NET_CMD_SEED:	
			// sent at the beginning of a game by the host only
			// should indicate that the game is starting
			memcpy(&tmp, data, sizeof(int));
			gGameSeed = tmp;
			debug("Setting game seed %d", tmp);
			if (gEndMenu == 0) gEndMenu = 10;		// countdown
			return cmd;
	}

//	debug("Machine %d sent cmd %d", idx, cmd);
	
	// nothing else does, though
	if (idx == gHumanPlayer) {
		return cmd;
	}

	switch (cmd) {
		case NET_CMD_HOST_QUIT:
			debug("Got command HOST QUIT");
			gEndMenu = -1;
			break;
			
		case NET_CMD_NAME:
			// first half of the name
			if (0 != memcmp(herder[idx].name, data, 5)) {
				memset(herder[idx].name, 0, sizeof(herder[idx].name));
				memcpy(herder[idx].name, data, 5);
			}
			break;
			
		case NET_CMD_NAME2:
			// second half of the name (if relevant)
			// NET_CMD_NAME should always come first to zero the buffer
			data[5]='\0';	// make SURE it's terminated
			if (0 != strcmp((char*)data, (char*)&herder[idx].name[5])) {
				herder[idx].name[10]='\0';
				memcpy(&herder[idx].name[5], data, 5);
			}
			break;
			
		case NET_CMD_WORLD:
			// world and stage updated - should come from the host only
			level=data[0];
			stage=data[1];
			break;
			
		case NET_CMD_CFG:
			// game configuration - should come from the host only
			gGame.Options.Powers = (unsigned char)((data[0]?POW_POWERUPS:0) | (data[1]?POW_SPECIALS:0));
			gGame.Options.CPU = data[2];
			gGame.Options.SheepSpeed=(signed char)data[3];
			gGame.Options.Skill=data[4];
			gGame.Options.Timer=data[5];
			break;
			
		case NET_CMD_CHAR:
			// herder character and color
			if (data[0] > HERD_AFROCHRYS) data[0]=0;
			herder[idx].type = PLAY_HUMAN | data[0];
			herder[idx].color = data[1]&0x0f;
			break;
			
		case NET_CMD_READY:
			gHerderReady[idx]=1;
			break;
			
		case NET_CMD_NOT_READY:
			gHerderReady[idx]=0;
			break;
			
		case NET_CMD_GAME_READY:
			// nothing here
			break;
			
		default:
			debug("Got unexpected command %d from machine %d", cmd, idx);
			break;
	}
	
	return cmd;
}

// the actual data sync wrapper
// returns 1 if data was received, else 0
int SyncNetwork() {
	int ret = 0;
	int idx;
	
	// do the network sync stuff.
	ret = WirelessFrame();
	if ((ret) && (gRxBitmap & 0x80000000U)) {
		gRxBitmap &= 0x7fffffff;
	
		// we got data, run through all four nodes and update our inputs
		for (int idx=0; idx<4; idx++) {
			if (gRxBitmap&(1<<idx)) {
				// received data from this node. We don't even have to worry if it's new, since the fill-in info will create it!
				ProcessCommand(idx);
			} else {
				// TODO: it shouldn't be possible to lose ourselves, is it??
				if (idx != gHumanPlayer) {
					if ((herder[idx].type & PLAY_MASK) == PLAY_HUMAN) {
						// no data, this client is gone, so end the stage
						herder[idx].type = PLAY_NETWORKDROP;
					}
				}
			} 
		}

//		debug("Frames(%4d): %4d %4d %4d %4d Lost:%d %04X %04X %04X %04X", gLocalFrame, gNetworkControlFrame[0], gNetworkControlFrame[1], gNetworkControlFrame[2], gNetworkControlFrame[3], gLostFrames,
//			gNetworkControl[0], gNetworkControl[1], gNetworkControl[2], gNetworkControl[3]);
		gLostFrames = 0;
	} else {
		gLostFrames++;
	}
	
	return ret;
}

// returns true if all nodes are in sync frame-wise
int CheckNetworkSync() {
	int ret = 1;
	// we got data, run through all four nodes and update our inputs
	for (int idx=0; idx<4; idx++) {
		if ((herder[idx].type & PLAY_MASK) != PLAY_HUMAN) continue;

		// make sure all the received values are the same
		// to prevent race, we have to consider it okay if someone
		// else is 1 frame ahead of us, too. But not behind us!
		if (gNetworkControlFrame[idx] != gLocalFrame) {
			if (gNetworkControlFrame[idx]-1 != gLocalFrame) {
				ret = 0;
				// tell me why it's off
				//debug("Frames(%4d): %4d %4d %4d %4d", gLocalFrame, gNetworkControlFrame[0], gNetworkControlFrame[1], gNetworkControlFrame[2], gNetworkControlFrame[3]);
			}
		}
	}
	
	return ret;
}

// handles the network protocol so the network flows nicely
// regardless of what menu people are in. Does not do any
// screen updates here! Call as close to vblank as possible
void HandleNetworkStuff() {
	// fill in our data for this frame
	static int nXmitFrame = 0;

	// fill in our own output data
	switch (nXmitFrame) {
		case 0:		// name
			QueueName((char*)OwnerInformation.nickName);
			if (strlen((char*)OwnerInformation.nickName) <= 5) {
				nXmitFrame++;	// skip name2 if short enough
			}
			break;
			
		case 1:		// name2
			QueueName2(((char*)OwnerInformation.nickName));
			break;
			
		case 2:		// char
			QueueChar((int)(herder[gHumanPlayer].type&PLAY_CHAR_MASK), herder[gHumanPlayer].color);
			break;
			
		case 3:		// Ready or Not
			if (gHerderReady[gHumanPlayer]) {
				QueueReady();
			} else {
				QueueNotReady();
			}
			break;
			
		case 4:		// World
			if (!gMPStuff.fHosting) {
				// not our business to send this or the cfg
				nXmitFrame=-1;
			} else {
				QueueWorld(level, stage);
			}
			break;
			
		case 5:		// cfg
			if (!gMPStuff.fHosting) {
				// not our business to send this or the cfg
				nXmitFrame=-1;
			} else {
				QueueCfg(gGame.Options.Powers&POW_POWERUPS, gGame.Options.Powers&POW_SPECIALS, gGame.Options.CPU, gGame.Options.SheepSpeed, gGame.Options.Skill, gGame.Options.Timer);
			}
			break;
			
		default:
			nXmitFrame=-1;
	}
	nXmitFrame++;
	if (gMPStuff.fHosting) {
		if (nXmitFrame >= 6) nXmitFrame = 0;
	} else {
		if (nXmitFrame >= 4) nXmitFrame = 0;
	}
}

// quick bubble to say only the host can set the options
void ShowHostOnlyNotice() {
	int nOption=0;
	
	MenuScr_InitMenu(MENU_STORY_TEXT);
	MenuScr_UpdateMenuString(0, "Only the host\ncan change these\nsettings.\nPress A to continue.");
	while (!MenuScr_DrawFrame(&nOption)) {
		pvr_scene_finish_2d();
		HandleNetworkStuff();
	}
	MenuScr_CloseMenu(1);
	while (MenuScr_DrawMenuItems()) {
		pvr_scene_finish_2d();
		HandleNetworkStuff();
	}
}

// returns 1 if okay, 0 if times out. Used by everyone to trigger a start
// Waits for EVERYONE to be sending NET_CMD_GAME_READY - in theory
// everyone already has.
int WirelessWaitHostMachineReady() {
	int ok;

	debug("Waiting for everyone to set NET_CMD_GAME_READY.");
	
	ok = 0;
	while (!ok) {
		ok=1;
		pvr_scene_finish_2d();
		gLocalFrame = 0;
		for (int idx=0; idx<4; idx++) {
			if ((herder[idx].type&PLAY_MASK)!=PLAY_HUMAN) continue;
			if (gRxBitmap&(1<<idx)) {
				// received data from host
				if (NET_CMD_GAME_READY != ProcessCommand(idx)) {
					debug("Player %d is not ready", idx);
					ok=0;
				}
				gLostFrames = 0;
			} else {
				debug("Player %d did not send data", idx);
				ok = 0;
			}
		}
		if (gLostFrames >= MAX_LOST_FRAMES) {
			// have not heard for the host for almost 3 seconds!
			doLostWireless();
			return 0;
		}
	}
	
	debug("Everyone is NET_CMD_GAME_READY.");
	return 1;
}

// returns 1 if okay, 0 if times out. Used by the host at the beginning of a stage
// All active machines must queue NET_CMD_READY!
int WirelessWaitAllMachinesReady() {
	int ok;
	
	debug("Waiting for all machines to set NET_CMD_READY.");
	
	ok = 0;
	while (!ok) {
		ok=1;
		pvr_scene_finish_2d();
		gLocalFrame = 0;
		for (int idx=0; idx<4; idx++) {
			if ((herder[idx].type&PLAY_MASK)!=PLAY_HUMAN) continue;
			if (gRxBitmap&(1<<idx)) {
				// received data from host
				if (NET_CMD_READY != ProcessCommand(idx)) {
					ok=0;
				}
				gLostFrames = 0;
			} else {
				ok = 0;
			}
		}
		if (gLostFrames >= MAX_LOST_FRAMES) {
			// have not heard for the host for almost 3 seconds!
			doLostWireless();
			return 0;
		}
	}
	
	return 1;
}

// Multiplayer Wireless game (both host and client)
// gMPStuff.fHosting indicates which we are doing.
// Return 1 to loop back again, 0 to exit
// when calledback is true, we are being recalled after a game
int HandleWirelessGame(int calledback) {
	int nLoadedLevel = -1;
	int nLoadedHerder[4] = {	// (color<<8)+character
		-1, -1, -1, -1
	};
	int idx, nOption=0;
	int me = 0;
	static int nLastLevel = -1;
	static int nLastStage = -1;
	static int nLastHerder = -1;
	char buf[33];
	
	// this is used by the child to detect a lost parent
	gLostFrames = 0;
	
	// we reset this a lot, sync is only important in game itself
	gLocalFrame = 0;
	
	// flag used to exit the menu
	gEndMenu=0;
	
	// this is the game seed that all systems will use when the game starts, 0 detects it was never set
	gGameSeed = 0;
	
	ShowBlack();
	sound_stop();
	HandleTopMenuView(SCROLL_TARG_NONE);
	
	MenuScr_Init();
	MenuScr_InitMenu(MENU_WIRELESS_GAME);

	// attempt to start up host networking (client is already set)
	if (gMPStuff.fHosting) {
		// no data received yet
		gRxBitmap=0;
		if (!calledback) {
			// clear all win counters
			herder[0].wins = 0;
			herder[1].wins = 0;
			herder[2].wins = 0;
			herder[3].wins = 0;
			// start up wireless
			InitWireless();
			if (!InitializeParent()) {
				// don't end wireless here, let it end on exit
				doWirelessError();
				return 0;
			}
		}
		// don't leave any left-over messages
		QueueNotReady();
	}
	
	// don't do this till after we initialize the parent, or it's not valid!
	me = WH_GetCurrentAid(); // (0-3, 0 should be host)
	if ((me<0)||(me>3)) {
		debug("** Fatal multiplayer error - AID is not 0-3, it's %d\n", me);
		return 0;
	}
	
	// configure the controls appropriately
	gHumanPlayer = me;
	for (idx=0; idx<4; idx++) {
		herder[idx].type = PLAY_NONE;
		gHerderReady[idx] = 0;
		gNetworkControl[idx]=0;
		ControllerState[idx] = eContNone;
	}
	ControllerState[me] = eContLocal;

	if (nLastLevel == -1) {
		level = LEVEL_NZ;
		stage = 0;
	} else {
		level = nLastLevel;
		stage = nLastStage;
	}
	
	// load the backdrop
	load_bmp_block_mask("gfx/Menu/World.bmp", NULL, txr_misc, pal_misc, -1);
	
	// add myself
	if (nLastHerder == -1) {
		herder[me].type = PLAY_HUMAN | HERD_ZEUS;
	} else {
		herder[me].type = PLAY_HUMAN | nLastHerder;
	}
	strcpy((char*)herder[me].name, (char*)OwnerInformation.nickName);
	
	// set up the herder sprites by position at least
	for (idx=0; idx<4; idx++) {
		herder[idx].spr.x=30;
		herder[idx].spr.y=68+32*idx;
		herder[idx].spr.alpha=31;
		herder[idx].spr.is3D=1;
		herder[idx].spr.z=(fx16)2028;
		herder[idx].spr.nDepth=DEPTH_256x256x4;
		herder[idx].spr.txr_addr=txr_herder[idx];
		herder[idx].spr.pal_addr=pal_herder[idx];
	}
	
	for (;;) {
		// scramble the system rand(), since we set srand now
		idx=rand();
		// quick update of the 'last' cache
		nLastLevel = level;
		nLastStage = stage;
		nLastHerder = (int)(herder[me].type&PLAY_CHAR_MASK);
		
		// keep frame sync loose
		gLocalFrame = 0;
	
		// draw the display
		pvr_scene_begin();
		SetOffset(0,0);
		
		// backdrop first, darkened halfway
		SortFullPictureX(GX_TEXFMT_PLTT256, txr_misc, pal_misc, 16);
		
		// next the herders
		for (idx=0; idx<4; idx++) {
			// in the menu, a network drop just maps back to none
			if ((herder[idx].type & PLAY_MASK) == PLAY_NETWORKDROP) {
				herder[idx].type = PLAY_NONE;
			}
			if ((herder[idx].type & PLAY_MASK) == PLAY_NONE) continue;
			if (nLoadedHerder[idx]==-1) continue;
			if (gHerderReady[idx]) {
				herder[idx].spr.tilenumber = 34;
			} else {
				herder[idx].spr.tilenumber = 41;
			}
			SortSpriteSquashed(&herder[idx].spr, herder[idx].map, POLY_HERD, 75, 75);
		}
		
		// and the world window, if it's loaded
		if (nLoadedLevel != -1) {
			scaleimage(txr_level, pal_level, 184, 68, 184+63, 68+48, 31);
		}
		
		pvr_scene_finish();
		
		// run the network
		HandleNetworkStuff();
		if (gLostFrames >= MAX_LOST_FRAMES) {
			// have not heard for the host for almost 3 seconds!
			doLostWireless();
			return 0;
		}
		if (gEndMenu == -1) {
			// host quit deliberately
			doHostQuit();
			return 0;
		}
		if (!gMPStuff.fHosting) {
			if (gEndMenu > 1) gEndMenu--;		// counting down a few frames for sync
			if (gEndMenu == 1) {
				// host sent a seed and we counted down a few frames
				// the host exits this loop when everyone else shows ready
				break;
			}
		}
		
		// update the text layer during vblank. so it may lag a little, no biggie
		Clear2D();
		SetOffset2D(-4,0);
		
		sprintf(buf, "Sheep Sp:%c CPU Skill:%c Timer:%2.2d",
			gGame.Options.SheepSpeed==0?'0':gGame.Options.SheepSpeed>0?'+':'-',
			gGame.Options.Skill==1?'0':gGame.Options.Skill>1?'+':'-',
			gGame.Options.Timer);
		WriteFont2D(1, 0, buf);
		
		sprintf(buf, "PowerUps:%c Specials:%c CPU:%c",
			gGame.Options.Powers&POW_POWERUPS?'Y':'N',
			gGame.Options.Powers&POW_SPECIALS?'Y':'N',
			gGame.Options.CPU?'Y':'N');
		WriteFont2D(3, 2, buf);
		
		WriteFont2D(6, 0, "PLAYER NAME      WINS   WORLD");
		for (idx=0; idx<4; idx++) {
			if ((herder[idx].type & PLAY_MASK) == PLAY_NETWORKDROP) {
				herder[idx].type = PLAY_NONE;
			}
			if ((herder[idx].type & PLAY_MASK) == PLAY_NONE) continue;
			sprintf(buf, "%c%c    %-10.10s %2.2d",
				me==idx?'M':'P', me==idx?'E':idx+'1',
				herder[idx].name, herder[idx].wins);
			WriteFont2D(9+4*idx, 1, buf);
		}
		
		switch (nLoadedLevel) {
			case LEVEL_NZ:			CenterWriteFontBreak2D(15, 26, "Zealande"); break;
			case LEVEL_CANDY:		CenterWriteFontBreak2D(15, 26, "Candy \nShoppe"); break;
			case LEVEL_HAUNTED:		CenterWriteFontBreak2D(15, 26, "Hades'\nPlace "); break;
			case LEVEL_TOY:			CenterWriteFontBreak2D(15, 26, "Toy\nFactory"); break;
			case LEVEL_DISCO:		CenterWriteFontBreak2D(15, 26, "Disco"); break;
			case LEVEL_WATER:		CenterWriteFontBreak2D(15, 26, "Water\nworks"); break;
			case LEVEL_UNDERWORLD:	CenterWriteFontBreak2D(15, 26, "Under\nworld"); break;
		}
		if (nLoadedLevel != -1) {
			sprintf(buf, "Stage:%d", stage+1);
			WriteFont2D(20, 23, buf);
		}
		
		if (MenuScr_DrawFrame(&nOption)) {
			MenuScr_CloseMenu(1);
			// keep running the network while we close
			while(MenuScr_DrawMenuItems()) 
			{
				pvr_scene_finish_2d();
				HandleNetworkStuff();
			}

			// user selected a menu option
			switch (nOption) {
				case -1:	// back
					if (gHerderReady[me]) {
						gHerderReady[me]=0;
						break;
					}
					// else just drop out
					if (gMPStuff.fHosting) {
						// notify the children so they don't wonder what's going on
						HandleTopMenuView(SCROLL_TARG_NONE);
						QueueHostQuit();
						MenuScr_InitMenu(MENU_STAGEINTRO_TEXT);	
						MenuScr_UpdateMenuString(0, "closing");
						for (int idx=0; idx<32; idx++) {
							MenuScr_DrawFrame(&nOption);
							pvr_scene_finish_2d();
							// don't insert an early quit - it's possible for a child to miss a few
							// frames due to loading graphics, and we might accidentally quit too early
							// This is only 1 second.
						}
						MenuScr_CloseMenu(0);
					}
					return 0;
					
				case MENU_PRESSSTART:
					// this is cancelling the Waiting screen
					gHerderReady[me] = 0;
					break;
					
				case MENU_LEVEL_SELECT:
					if (!gMPStuff.fHosting) {
						ShowHostOnlyNotice();
						break;
					}

					// Do the stage select itself
					if (-1 != doStageSelect(99, HandleNetworkStuff)) {
						// have to reload the level image in case it changed (as we
						// do not know whether it is correct anymore or not)
						nLoadedLevel = -1;
					} else {
						// the level select uses the same arrays we do here, so 
						// shouldn't need to reload anything - but assume the nLoadedLevel
						// is now correct. (assumptions for the win?)
						nLoadedLevel = level;
					}

					MenuScr_Init();
					break;
				
				case MENU_BATTLE_OPTIONS:
					if (!gMPStuff.fHosting) {
						ShowHostOnlyNotice();
						break;
					}
					doBattleOptionsMenu(HandleNetworkStuff);
					// should not have touched any 3D buffers
					break;
				 
				case MENU_CHARACTER_SELECT:
					// do player select (use recall option with a force setting)
					doPlayerSelect(nLastHerder+1, HandleNetworkStuff);
					
					// correct corrupted data	
					for (idx=0; idx<2; idx++) {				// two herders are corrupted
						nLoadedHerder[idx]=-1;
					}
					MenuScr_Init();
					break;
				
				case MENU_START:
					// put this player into 'ready' mode
					gHerderReady[me]=1;
					MenuScr_InitMenu(MENU_STAGEINTRO_TEXT);	
					MenuScr_UpdateMenuString(0, "waiting");
					break;
			}
			if (nOption != MENU_START) {
				// bring back the same menu
				MenuScr_InitMenu(MENU_WIRELESS_GAME);
			}
		} else {
			// check local keys of interest
			u16 btns = GetController(gHumanPlayer);
		
			if ((lastbuttons[gHumanPlayer]&(CONT_R|CONT_L)) != (btns & (CONT_R|CONT_L)))	{
				if (btns & CONT_L) {
					// previous stage
					if (stage > 0) stage--;
				}
				if (btns & CONT_R) {
					// next stage
					if (stage < 2) stage++;
				}
			}
			lastbuttons[gHumanPlayer]=btns;
		}

		// now that everything is onscreen, let's see if we have anything to load.
		// if we do load anything, we loop around after it's done, cause it can take
		// a while. So only one image is loaded per cycle.
		
		// check the level image first
		if (nLoadedLevel != level) {
			sprintf(buf, "gfx/Menu/level%d.bmp", level);
			load_bmp_block_mask(buf, NULL, txr_level, pal_level, -1);
			nLoadedLevel = level;
			continue;
		}
		
		// how about herders, then?
		for (idx=0; idx<4; idx++) {
			if ((herder[idx].type&PLAY_MASK) == PLAY_NETWORKDROP) {
				herder[idx].type = PLAY_NONE;
			}
			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;
			int nTmp = (int)(herder[idx].type&PLAY_CHAR_MASK) | (herder[idx].color<<8);
			if (nTmp != nLoadedHerder[idx]) {
				char *pherd = szNames[herder[idx].type&PLAY_CHAR_MASK];
				char buf2[32];

				// select the image
				sprintf(buf, "gfx/Players/%s_img.bmp", pherd);	
				// select the palette
				sprintf(buf2, "gfx/Players/%s%X.bmp", pherd, herder[idx].color);

				debug("Player %d (%s) loading %s (%s) for menu\n", idx, herder[idx].name, buf, buf2);
				load_bmp_block_mask(buf, buf2, txr_herder[idx], pal_herder[idx], -1);
				buf[strlen(buf)-7]='\0';
				strcat(buf,"map.txt");
				load_map(buf, herder[idx].map);
				nLoadedHerder[idx]=nTmp;
				break;
			}
		}
		if (idx < 4) continue;		// loop around if we had to load anything
		
		// if we're the host, check whether everyone is marked as ready (but only if someone else is active)
		if (gMPStuff.fHosting) {
			int nNumPlayers = 0;
			for (idx=0; idx<4; idx++) {
				if ((herder[idx].type&PLAY_MASK) == PLAY_NETWORKDROP) {
					herder[idx].type = PLAY_NONE;
				}
				if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) continue;
				// this includes ourselves being ready!
				if (!gHerderReady[idx]) break;
				nNumPlayers++;
			}
			// everyone is ready (at there's at least one other player)
			// we are always one of the readys
			if ((idx >= 4) && (nNumPlayers > 1)) break;
		}
	}
	
	debug("Out of main screen loop.");
	
	// we get here when the game is about to begin. If we're the host, we have to
	// send a seed. Everyone else has gotten here because they received the host's seed.
	if (gMPStuff.fHosting) {
		// need to generate and send a seed, then mark ourselves ready
		// not ch_rand on purpose
		gGameSeed = (rand()&0xffff)|((rand()&0xffff)<<16);		// prolly still lose 2 bits, but that's okay
		QueueSeed(gGameSeed);
		debug("Sending seed %d.", gGameSeed);
		
		// and now we just wait for everyone else to get the seed. They are waiting for us to go ready.
		for (;;) {
			int ok = 1;

			// do the sync inline so we can make sure whether everyone else is done
			pvr_scene_finish_2d();
			if (gRxBitmap) {
				// we got data, run through all four nodes and update our display
				for (int idx=0; idx<4; idx++) {
					if (idx == gHumanPlayer) continue;		// don't process ourselves here, we only care about everyone else
					if ((herder[idx].type&PLAY_MASK)==PLAY_NETWORKDROP) {
						debug("Lost host %d to drop - giving up.", idx);
						doLostWireless();
						return 0;
					}
					if ((herder[idx].type&PLAY_MASK)==PLAY_NONE) continue;
					if (gRxBitmap&(1<<idx)) {
						// received data from this node. We don't even have to worry if it's new, since the fill-in info will create it!
						if (NET_CMD_GAME_READY != ProcessCommand(idx)) {
							debug("Host %d not ready...", idx);
							ok=0;
						}
					} else {
						// no data, this client is gone, so remove them
						// this should probably be an error (client lost!) rather than a drop, we're trying to start
						debug("Lost host %d - giving up.", idx);
						doLostWireless();
						return 0;
					}
				}
				gLostFrames = 0;
			} else {
				ok = 0;
				if (gLostFrames >= MAX_LOST_FRAMES) {
					// have not heard for the host for almost 3 seconds! (WE ARE THE HOST... oh well).
					doLostWireless();
					return 0;
				}
			}
			if (ok) {
				debug("Server sending NET_CMD_GAME_READY");
				QueueGameReady();
				break;
			}
		}
	} else {
		// we're just a client, we dropped out of the above loop because we got a seed, so we are ready
		debug("Host has sent a seed to start the game, sending NET_CMD_GAME_READY");
		QueueGameReady();
	}

	// at this point, so everyone including the host is in sync, we wait for the host ready to show up.
	WirelessWaitHostMachineReady();
	
	// set the seed
	if (gGameSeed == 0) {
		debug("### WARNING: Game Seed 0 - was never set??");
		doLostWireless();
		return 0;
	}
	ch_srand(gGameSeed);
	srand(gGameSeed);
	
	// since everyone has the same random number seed, it SHOULD be safe to generate the computer players
	// in this case we DO use ch_rand() to ensure that!
	// set up computer players (everyone but the human)
	if (gGame.Options.CPU) {
		for (idx=0; idx<4; idx++) {
			if ((herder[idx].type&PLAY_MASK) == PLAY_NONE) {
				int o1,o2,o3;
				herder[idx].type = PLAY_COMPUTER|(ch_rand()%12);
				herder[idx].color = (unsigned char)(ch_rand()&0x0f);
				if (idx == 0) o1=3; else o1=0;
				if (idx == 1) o2=3; else o2=1;
				if (idx == 2) o3=3; else o3=2;
				// make sure there's no color match with the human (we don't check character here)
				while ((herder[o1].color == herder[idx].color) || (herder[o2].color == herder[idx].color) || (herder[o3].color == herder[idx].color)) {
					herder[idx].color = (herder[idx].color+1)&0x0f;
				}
			}
		}
	}	
	
	// let the scary stuff begin! At this point everyone is saying NET_CMD_GAME_READY, which we had to do instead of NET_CMD_READY
	// since NET_CMD_READY was already being sent at intervals. 
	
	// In the game itself, when loading begins, everyone queues NET_CMD_NOT_READY. When load is complete, they 
	// queue NET_CMD_READY and then call WirelessWaitAllMachinesReady(). This helps get back into sync after the
	// file access. At that point, the host queues NET_CMD_GAME_READY, and when everyone sees this, the level begins.
	
	// I don't THINK we need to worry about the return value from Game? (0=continue, 1=exit, bbaa=end credits?)
	// passing '2' for multiplayer wireless
	Game(2);
	
	// we want to loop back in
	return 1;
}

