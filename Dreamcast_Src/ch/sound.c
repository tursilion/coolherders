/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/*
 * sound.c
 *
 * Ported back again (heh!) to KOS 1.1.x by Dan Potter
 *
 */

#include <stdio.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#else
#include <kos.h>
#include <oggvorbis/sndoggvorbis.h>
#endif

#include "sprite.h"
#include "cool.h"
#include "sound.h"
#include "rand.h"

// Most of this module has been superceded, functionality-wise,
// by the snd_sfx_* and sndoggvorbis stuff.

sfxhnd_t snd_sheep[6] = { 0,0,0,0,0,0 };
sfxhnd_t snd_score = 0;
sfxhnd_t snd_boom = 0;
sfxhnd_t snd_bleat = 0;

// This is used as a multipler (255=100%) for sound effect volumes
int nLocalSoundVol=255;

/* Starts playing a music file */
void sound_start(char * data, int repeat) {
	sndoggvorbis_stop();

	// temporary code to play a RANDOM background music - till we have more levels! ;)
	if (data == SONG_RANDOM) {
		switch (ch_rand()%8) {
		case 0:	data=SONG_NEWZEALAND; break;
		case 1:	data=SONG_HAUNTED; break;
		case 2: data=SONG_FACTORY; break;
		case 3: data=SONG_CANDYLAND; break;
		case 4: data=SONG_WATERWORKS; break;
		case 5: data=SONG_HEAVEN; break;
		case 6: data=SONG_DISCO; break;
		case 7: data=SONG_HELL; break;
		}
	}

	// Regular code begins here
	if (data != NULL) {
		debug("Starting music '%s'\n", data);
		sndoggvorbis_start(data, repeat);
	}
}

void bg_sound_start_thread(void *pDat) {
	sound_start((char*)pDat, 0);
}

/* Wrapper to start a new tune in a background thread to keep foreground game smooth */
void bg_sound_start(char *data) {
	thd_create(bg_sound_start_thread, data);
}

/* Starts playing a music file without the stop */
void sound_restart(char * data, int repeat) {
	sndoggvorbis_start(data, repeat);
}

/* stop playing music */
void sound_stop() {
	sndoggvorbis_stop();
	thd_sleep(100);
}

/* Play a single sound effect */
void sound_effect(sfxhnd_t idx, int vol) {
	vol=(vol*nLocalSoundVol)/255;
	snd_sfx_play(idx, vol, 0x80);
}

/* sound all sound effects (not music) */
void sound_effects_stop() {
	snd_sfx_stop_all();
}

/* Initialize the sound system */
void sound_init() {
	int idx;
	char buf[256];
	
	snd_stream_init(NULL);
	sndoggvorbis_init();

	for (idx=0; idx<6; idx++) {
		sprintf(buf, "sfx/flossie0%d.wav", idx);
		snd_sheep[idx] = snd_sfx_load(buf);
	}
	snd_score=snd_sfx_load("sfx/click.wav");
	snd_boom=snd_sfx_load("sfx/bang.wav");
	snd_bleat=snd_sfx_load("sfx/bleat_ad.wav");
}

/* shutdown */
void sound_shutdown() {
	if (musicisplaying()) {
		sndoggvorbis_stop();	
	}
	snd_sfx_unload_all();
	sndoggvorbis_shutdown();
}

/* Test if playing */
int musicisplaying() {
	/* BUG: this returns false before the tune is over! */
	/* This is why sounds sometimes sound like they cut off.  */
	/* (It seems that the ogg library is stopping the audio   */
	/* as well... delays on this end have no effect on the sound */
	return sndoggvorbis_isplaying();
}

// volumes are from 0-255, either may be -1 to leave unchanged
void set_sound_volume(int nMusic, int nSound) {
	if (nMusic != -1) {
		debug("Setting music volume to %d\n", nMusic);
		sndoggvorbis_volume(nMusic);
	}
	if (nSound != -1) {
		debug("Setting sound volume to %d\n", nSound);
		nLocalSoundVol=nSound;
	}
}
