/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* control.c                            */
/****************************************/


#ifdef WIN32
#include "kosemulation.h"
#else
#include <kos.h>
#endif

#include "sprite.h"
#include "cool.h"
#include "control.h"
#include "rand.h"
/* from KOS itself... so we can tell when the PVR is busy rendering */
#ifndef WIN32
#include "../kernel/arch/dreamcast/hardware/pvr/pvr_internal.h"
#endif
extern volatile pvr_state_t pvr_state;
extern int gReturned;
extern int gAtMain;

// track addresses of detected controllers (only updated in menu)
maple_device_t *ControllerState[4]={ NULL, NULL, NULL, NULL };
extern char cheatstring[];
extern int lastbuttons[4];

/* return true if any controller has Start or A pressed */
/* specifically returns 1 for start and 2 for A, if it matters */
int isStartPressed() {
	int fPressed=0;
	uint32 allbtns=0;
	char c;
	
	MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
		if (NULL == st) {
			continue;
		}

		allbtns|=st->buttons;
		lastbuttons[__i]=st->buttons;

		if ((st->buttons&CONT_START)&&(st->buttons&CONT_A)) {
			continue;
		}

		if (st->buttons & CONT_START) {
			if (gReturned) {
				fPressed=CONT_START;
			}
			goto exitscan;
		}
		if (st->buttons&CONT_A) {
			if (gReturned) {
				fPressed=CONT_A;
			}
			goto exitscan;
		}

		// scramble random number generator
		ch_rand();

	MAPLE_FOREACH_END();

exitscan:
	// handle cheat string for characters that matter
	c='\0';
	if (allbtns&CONT_A) {
		c='A';
	}
	if (allbtns&CONT_B) {
		c='B';
	}
	if (allbtns&CONT_X) {
		c='X';
	}
	if (allbtns&CONT_Y) {
		c='Y';
	}
	if (allbtns&CONT_DPAD_UP) {
		c='U';
	}
	if (allbtns&CONT_DPAD_DOWN) {
		c='D';
	}
	if (allbtns&CONT_DPAD_LEFT) {
		c='L';
	}
	if (allbtns&CONT_DPAD_RIGHT) {
		c='R';
	}
	if (c!=cheatstring[9]) {
		if (cheatstring[9]!='\0') {
			memmove(cheatstring, &cheatstring[1], 10);
		}
		cheatstring[9]=c;
		cheatstring[10]='\0';	// just to be safe
	}
//	debug("Cheatstring: %s\n", cheatstring);

	// Just to be nice, we'll set the gReturned flag for the main loop
	if (0 == (allbtns & (CONT_X|CONT_Y))) {
		if (gAtMain) {
			if (!gReturned) {
				debug("gReturned clear - btns %08x\n", allbtns);
			}
			gReturned++;
		}
	}

	return fPressed;
}

void waitPVRDone() {
	// Don't redraw while the PVR is rendering!
	while (pvr_state.render_busy);
}

