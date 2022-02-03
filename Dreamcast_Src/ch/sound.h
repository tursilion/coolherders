/*
 * sound.h
 *
 * (C)2000 Dan Potter
 *
 * Hacked up by M.Brent :)
 *
 */

#ifndef FILE_COOL_H
#error Must include cool.h before sound.h!
#endif

#ifndef __SOUND_H
#define __SOUND_H

#define VOLUME 240
#define PAN 128

#define SHEEPVOL 128
#define CAUGHT 0
#define START 1
#define FREED 2

#define ZAPPEDA 3
#define ZAPPEDB 4
#define ZAPPEDC 5

#define SNDDELAY 15

#define SONG_RAM    "/ram/title.ogg"		/* not normally used anymore (demo) */
#define SONG_NONE	NULL

// paths to all the musics
#define SONG_TITLE		"music/title.ogg"
#define RAM_TITLE		"/ram/title.ogg"
#define SONG_SELECT		"music/Tenzu_Demo.ogg"
#define SONG_HISCORE	"music/introfinal.ogg"

// Level tunes
#define SONG_DISCO		"music/disco.ogg"
#define SONG_HAUNTED	"music/haunted_house.ogg"
#define SONG_FACTORY	"music/toyfactory.ogg"
#define SONG_CANDYLAND  "music/candyland.ogg"
#define SONG_WATERWORKS	"music/waterworks.ogg"
#define SONG_HEAVEN		"music/heaven_final.ogg"
#define SONG_HELL		"music/hell.ogg"
#define SONG_NEWZEALAND	"music/nz2.ogg"

#define SONG_END		"music/winjingle.ogg"
#define SONG_LOSE       "music/end.ogg"
#define SONG_COPPER		"music/copper.ogg"
#define SONG_LOOP		"music/loop.ogg"

#define SONG_RANDOM		((char*)0xffffffff)
////////////////

#ifdef WIN32
#define SONG_WIN	"music/winjingle.ogg"
#define SONG_STORY	"music/loop.ogg"
#define SONG_GAMEOVER "music/end.ogg"
#else
#define SONG_WIN	"/ram/winjingle.ogg"
#define SONG_STORY	"/ram/loop.ogg"
#define SONG_GAMEOVER "/ram/end.ogg"
#endif

void sound_init();
void sound_shutdown();
void sound_effect(sfxhnd_t idx, int vol);
void sound_effects_stop();
void sound_start(char * data, int repeat);
void sound_restart(char * data, int repeat);
void bg_sound_start(char *data);
void sound_stop();
int  musicisplaying();
void set_sound_volume(int nMusic, int nSound);

#endif	/* __SOUND_H */
