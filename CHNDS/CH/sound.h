/*
 * sound.h
 *
 * (C)2000 Dan Potter
 *
 * Hacked up by M.Brent :)
 *
 */

#ifndef __SOUND_H
#define __SOUND_H

// include sound bank headers
#include "Audio\CoolHerders.sadl"

#define MAXSND_CHARGE 1
#define MAXSND_DAMAGE 2
#define MAXSND_SPECIAL 1
#define MAXSND_VICTORY 3

#define SHEEPVOL 80
#define CAUGHT 0
#define START 1
#define FREED 2
#define ZAPPEDA 3
#define ZAPPEDB 4
#define ZAPPEDC 5

#define PLAYERVOL 90
#define SPECIALVOL 110

#define SNDDELAY 18

extern int nNumberLoops;

void sound_init();
void sound_loadplayer(int nPlayer, int nSndGrp);
void sound_unloadlevel();
void sound_shutdown();
void sound_frame_update();
void sound_start(int nWhich, int repeat);
void sound_restart(int nWhich, int repeat);
void sound_stop();
unsigned long musicisplaying();
void set_sound_volume(int nMusic, int nSound);

void sound_effect_system(int idx, int vol);
void sound_effect_player(int nPlayer, int idx, int vol);
void sound_effect_system_throttled(int idx, int vol);
void sound_effect_player_throttled(int nPlayer, int idx, int vol);

#endif	/* __SOUND_H */
