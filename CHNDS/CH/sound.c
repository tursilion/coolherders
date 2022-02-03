/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/*
 * sound.c
 *
 * Ported back again (heh!) to KOS 1.1.x by Dan Potter
 * And rewritten for Nitro by Mike Brent ;)
 *
 */
 
// To create the sound banks, you must manually edit the .mus and .bnk files
// In Nitro SoundMaker, create the .mus file under SEQARC - this is the player definitions for the sound effect
// Create the .BNK file under Bank - this has the wav file definitions for the soundbank
// Together these two files define a set of sound effects (played as sequences) - handy to make a group
// You will have to hand-edit the files after creation.

#include <stdio.h>
#include <nnsys/snd.h>

#include "kosemulation.h"

#include "sprite.h"
#include "cool.h"
#include "sound.h"
#include "rand.h"

NNSSndHeapHandle heap;
char sndHeap[512*1024];	// streams need 2-4k each. also need room for loaded sounds and overhead
NNSSndArc arc;
NNSSndStrmHandle hCurrentStream;
NNSSndHandle seSystemHandle;
NNSSndHandle sePlayerHandle[4];
int sePlayerArc[4];
int SoundHeapGlobalState=-1;
int nNumberLoops=0;
int nEffectCountdown=0;
int nPlayerCountdown[4] = { 0, 0, 0, 0 };

// Manual stream looping
int loop_stream=-1;
 
// This is used as a multipler (128=100%) for sound effect volumes
int nLocalSoundVol=128;

// Used for loading characters
int nSoundGroups[16] = 
{
	SNDGRP_ZEUSVOICE,
	SNDGRP_CLASSICHERDERVOICE,
	SNDGRP_CANDYSTRIPERVOICE,
	SNDGRP_NH5VOICE,
	SNDGRP_BACKUPDANCERVOICE,
	SNDGRP_ZOMBIEVOICE,
	SNDGRP_THALIAVOICE,
	SNDGRP_ISKURVOICE,
	SNDGRP_ANGELVOICE,
	SNDGRP_HADESVOICE,
	SNDGRP_TREYVOLTAVOICE,
	SNDGRP_DEMONVOICE,
	SNDGRP_ANGELVOICE,				// Chrys?? This may not work very well!
	SNDGRP_ZEUSVOICE,				// Afro Zeus
	SNDGRP_ANGELVOICE				// Afro Chrys
};
 
/* Initialize the sound system */
void sound_init() {
	debug("sound init");
	
	// Init the nitro sound engine
	NNS_SndInit();
	heap = NNS_SndHeapCreate(&sndHeap, sizeof(sndHeap));
	NNS_SndArcInit(&arc, "/CoolHerders.sdat", heap, TRUE);	// TODO: this may not work on a NAND app? NITRO.
	NNS_SndArcPlayerSetup(heap);
	NNS_SndArcStrmInit(16, heap);	// first arg is priority
	NNS_SndSetMonoFlag(TRUE);		// sad, but let's be honest, we're mono ;)
	NNS_SndStrmHandleInit(&hCurrentStream);

	loop_stream=-1;		// not looping
	
	// load up the system sound bank
	if (!NNS_SndArcLoadGroup(SNDGRP_SYSTEM, heap)) {
		debug("failed to load system sounds\n");
	}
	// init a handle for playing sound effects
	NNS_SndHandleInit(&seSystemHandle);
	
	// Save state so we can reset later to delete all the dynamic sounds and keep these ones
	SoundHeapGlobalState=NNS_SndHeapSaveState(heap);
	if (-1 == SoundHeapGlobalState) {
		debug("Failed to save heap state, heap too small!\n");
	}
}

/* Loads sound a sound group into a given player */
void sound_loadplayer(int nPlayer, int nSndGrp) 
{
	// make sure player bank isn't already doing any sounds
	NNS_SndPlayerStopSeq(&sePlayerHandle[nPlayer],0);
	sound_stop();

	// load player bank
	debug("Loading sound for player %d with group %d\n", nPlayer, nSndGrp);
	// load up the system sound bank
	sePlayerArc[nPlayer] = nSndGrp;
	if (!NNS_SndArcLoadGroup(nSndGrp, heap)) {
		debug("failed to load player sounds\n");
	}
	// init a handle for playing sound effects
	NNS_SndHandleInit(&sePlayerHandle[nPlayer]);
}

/* Unloads sound data loaded for a level */
void sound_unloadlevel()
{
	debug("sound_unloadlevel");
	NNS_SndHeapLoadState(heap, SoundHeapGlobalState);
	OS_WaitVBlankIntr();
}

/* shutdown */
void sound_shutdown() {
	debug("sound_shutdown");
	if (musicisplaying()) {
		sound_stop();
	}
	
	NNS_SndHeapClear(heap);
}

// call every frame to process sound
void sound_frame_update() {
	if (-1 != loop_stream) {
		if (!musicisplaying()) {
			sound_restart(loop_stream, 1);
			nNumberLoops++;
		}
	}
	NNS_SndMain();

	// countdown throttles
	if (nEffectCountdown > 0) nEffectCountdown--;
	if (nPlayerCountdown[0] > 0) nPlayerCountdown[0]--;
	if (nPlayerCountdown[1] > 0) nPlayerCountdown[1]--;
	if (nPlayerCountdown[2] > 0) nPlayerCountdown[2]--;
	if (nPlayerCountdown[3] > 0) nPlayerCountdown[3]--;
}

// Stream functions

/* Starts playing a music file */
void sound_start(int nWhich, int repeat) {
	sound_stop();

	// Regular code begins here
	if (-1 != nWhich) {
		debug("Starting music %d\n", nWhich);
		nNumberLoops=0;
		NNS_SndArcStrmStart(&hCurrentStream, nWhich, 0);
		if (repeat) {
			loop_stream=nWhich;
		}
	}
}
  
/* Starts playing a music file without the stop */
void sound_restart(int nWhich, int repeat) {
	NNS_SndArcStrmStart(&hCurrentStream, nWhich, 0);
	if (repeat) {
		loop_stream=nWhich;
	}
}
  
/* stop playing music */
void sound_stop() {
	debug("sound_stop");
	NNS_SndArcStrmStop(&hCurrentStream, 0);
	loop_stream=-1;
	nNumberLoops = 0;
	OS_WaitVBlankIntr();
	NNS_SndMain();
}
 
/* Test if playing */
unsigned long musicisplaying() {
	// this returns a position, too, but if we want that, rename this function
	return NNS_SndArcStrmGetCurrentPlayingPos(&hCurrentStream);
}

// SFX functions

/* Play a single sound effect */
void sound_effect_system(int idx, int vol) {
	vol=((vol*nLocalSoundVol)>>7);		// we need 0-127
//	debug("Playing sound for system %d, from archive %d, index %d, vol %d\n", seSystemHandle, SEQARC_SYSTEM, idx, vol);
	NNS_SndArcPlayerStartSeqArc(&seSystemHandle, SEQARC_SYSTEM, idx);
	NNS_SndPlayerSetVolume(&seSystemHandle, vol);
}

/* Plays a sound effect in a player's voice*/
/* Caution: Calling this without loading player sound effects is a 'bad thing' */
void sound_effect_player(int nPlayer, int idx, int vol) {
	if ((nPlayer < 0) || (nPlayer > 3)) 
	{
		// If this isn't a legal dereference, we'll crash
		return;
	}
	vol=((vol*nLocalSoundVol)>>7);		// we need 0-127
	debug("Forcing sound for player %d:%d, from archive %d, index %d, vol %d\n", nPlayer, sePlayerHandle[nPlayer], sePlayerArc[nPlayer], idx, vol);
	NNS_SndPlayerStopSeq(&sePlayerHandle[nPlayer],0);	// stop whatever we might now be playing
	NNS_SndArcPlayerStartSeqArc(&sePlayerHandle[nPlayer], sePlayerArc[nPlayer], idx);
	NNS_SndPlayerSetVolume(&sePlayerHandle[nPlayer], vol);
	// don't let a throttled sound override it
	nPlayerCountdown[nPlayer] = SNDDELAY*2;
}

// player and system effects with a throttle
void sound_effect_system_throttled(int idx, int vol) {
	if (0 == nEffectCountdown) {
		nEffectCountdown = SNDDELAY;
		sound_effect_system(idx, vol);
	}
}

void sound_effect_player_throttled(int nPlayer, int idx, int vol) {
	if ((nPlayer < 0) || (nPlayer > 3)) {
		return;
	}
	if (0 == nPlayerCountdown[nPlayer]) {
		nPlayerCountdown[nPlayer] = SNDDELAY*2;
		vol=((vol*nLocalSoundVol)>>7);		// we need 0-127
//		debug("Playing sound for player %d:%d, from archive %d, index %d, vol %d\n", nPlayer, sePlayerHandle[nPlayer], sePlayerArc[nPlayer], idx, vol);
		NNS_SndArcPlayerStartSeqArc(&sePlayerHandle[nPlayer], sePlayerArc[nPlayer], idx);
		NNS_SndPlayerSetVolume(&sePlayerHandle[nPlayer], vol);
	}
}

// volumes are from 0-8, either may be -1 to leave unchanged
void set_sound_volume(int nMusic, int nSound) {
	if (nMusic != -1) {
		debug("Setting music volume to %d\n", nMusic);
		if (nMusic == 0) {
			NNS_SndArcStrmSetChannelVolume(&hCurrentStream, 0, 0);
		} else {
			NNS_SndArcStrmSetChannelVolume(&hCurrentStream, 0, (nMusic<<4)-1);
		}
	}
	if (nSound != -1) {
		debug("Setting sound volume to %d\n", nSound);
		nLocalSoundVol=(nSound<<4)-1;	// this is a scaling factor! Inputs to the sound effect files will take 0-128
		if (nLocalSoundVol < 0) nLocalSoundVol = 0;
	}
}
