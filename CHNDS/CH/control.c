/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* control.c                            */
/****************************************/

#include <string.h>
#include "kosemulation.h"

#include "sprite.h"
#include "cool.h"
#include "control.h"
#include "rand.h"
#include "levels.h"		// necessary for mapscr_manager.h
#include "mapscr_manager.h"
#include "menuscr_manager.h"
#include "wireless\chwireless.h"

static int bControlDisabled=false;

void CheckControlStillDisabled(u16 btns);

// track addresses of detected controllers (only updated in menu)
// note that in a multiplayer game, the local controller may not be index 0, but should
// always be gHumanPlayer.
eControlType ControllerState[4];
extern unsigned int lastbuttons[4];
// track who the human player is
int gHumanPlayer=-1;

/* return true if local controller has Start or A pressed */
/* specifically returns 1 for start and 2 for A, if it matters */
int isStartPressed() {
	int fPressed=0;
	u16 btns;
	
	btns = GetController(gHumanPlayer);		// calls CheckControlStillDisabled
	lastbuttons[gHumanPlayer]=btns;
		
	if ((btns&CONT_START)&&(btns&CONT_B)) {
		return 0;
	}

	if (btns & CONT_START) {
		// early return allows A to be held too without overriding
		return CONT_START;
	}
	if (btns & CONT_A) {
		return CONT_A;
	}
	
	return fPressed;
}

// simple controller wrapper for network play
u16 GetNetController(int nControl) {
	if (eContNetwork == ControllerState[nControl]) {
		return gNetworkControl[nControl];
	} else {
		return GetController(nControl);
	}
}

// wrapper for the maple system to replace PAD_Read
u16 GetController(int nControl) {
	u16 btns;
	
	switch (ControllerState[nControl]) {
		case eContNetwork:
			// handle network player
			if (nControl != gHumanPlayer) {
				return gNetworkControl[nControl];
			}
			// for the human, fall through to below to queue the physical buttons
				
		case eContLocal:
			// NDS button mapping now
			btns=PAD_Read();
			
			if (bControlDisabled) {
				CheckControlStillDisabled(btns);
				return 0;
			}
	
			// Nintendo Lotcheck tests up+down, and left+right, so don't respond
			// to those
			if ((btns & (CONT_DPAD_UP|CONT_DPAD_DOWN)) == (CONT_DPAD_UP|CONT_DPAD_DOWN)) {
				btns &= ~(CONT_DPAD_UP|CONT_DPAD_DOWN);
			}
			if ((btns & (CONT_DPAD_LEFT|CONT_DPAD_RIGHT)) == (CONT_DPAD_LEFT|CONT_DPAD_RIGHT)) {
				btns &= ~(CONT_DPAD_LEFT|CONT_DPAD_RIGHT);
			}

			// this gives us a nice handy software reset - A+B+X+Y+Start
			if (!MB_IsMultiBootChild()) {
				if (btns == RESET_KEYS) {
			        // restart the whole darn thing
			        // We check for the 1 and skip the logos when set
			        // Note you can't reset if you are a multiboot client - see help file!
			        debug("Detected soft-reset keys - resetting!");
			        ShowBlack();
					OS_ResetSystem(1);
				}
			}
			
			if (MenuScr_IsInMenu()) {
				// Also don't allow A+B, as they may cause us to malfunction (X+Y is okay)
				if ((btns & (CONT_A|CONT_B)) == (CONT_A|CONT_B)) {
					btns &= ~(CONT_A|CONT_B);
				}
			}
			
#if !defined(SDK_FINALROM)
			// map the debug button to ABXY for emulation testing
			if (btns & PAD_BUTTON_DEBUG) {
				// hard to get 4 keys on a keyboard, this will also make emulation tricky
				btns |= CONT_A|CONT_B|CONT_X|CONT_Y;
			}
#endif

			if (ControllerState[nControl] == eContNetwork) {
				// we just did all this to get input for the net controls
				QueueCtrl(btns, gLocalFrame);
				return gNetworkControl[nControl];
			}
			
			return btns;
			
		default:
			// nobody
			return 0;
	}
}

void DisableControlsTillReleased() {
	bControlDisabled = true;
}

// times out local input only - each machine does this locally
void CheckControlStillDisabled(u16 btns) {
	TPData Data;
	
	bControlDisabled = false;

	// check controller data for zero
	if (btns) bControlDisabled = true;
	
	if (!bControlDisabled) {
		// check the touch screen too
		if (TP_CheckBusy(TP_REQUEST_COMMAND_FLAG_SAMPLING) == 0) {
			TP_RequestSamplingAsync();
		}
		if (0 == TP_WaitCalibratedResult(&Data)) 
		{
			if (TP_TOUCH_ON == Data.touch) {
				bControlDisabled=true;
			}
		}
	}
}

void InitTouchScreen() {
	TPCalibrateParam CalibData;
	
	TP_Init();
	
	if (!TP_GetUserInfo(&CalibData)) 
	{
		OS_Warning("Unable to obtain calibration data");
	}
	TP_SetCalibrateParam(&CalibData);
}

// wraps touch screen access - returns -1 if controls are
// disabled and waiting for release
u32 GetTouchData(TPData *pData) {
	if (bControlDisabled) {
		// we have to check the pad too
		GetController(gHumanPlayer);	// will call CheckControllerStillDisabled for us
		pData->touch = TP_TOUCH_OFF;
		return -1;
	} else {
		if (TP_CheckBusy(TP_REQUEST_COMMAND_FLAG_SAMPLING) == 0) {
			TP_RequestSamplingAsync();
		}
		return TP_WaitCalibratedResult(pData);
	}
}
