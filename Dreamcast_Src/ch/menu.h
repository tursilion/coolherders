/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* menu.h                               */
/****************************************/

#define MENUX 40
#define MENUY 364

int HandleMenus();
int doMenu();
int doOptionsMenu();
void AddHighScore(int nWho);
void ShowHighScores(int nTimeout);

// -1 - Cancel, Return to title
//	0 - Multiplayer
//	1 - Story Mode
//	2 - Options
//	3 - Gallery
//  4 - Music Player
//  5 - Demo mode (timeout)

enum {
	MENU_CANCEL = -1,
	MENU_MULTIPLAYER,
	MENU_STORY,
	MENU_CONTINUE,
	MENU_OMAKE,
	MENU_HOWTOPLAY,
	MENU_HIGHSCORES,
	MENU_OPTIONS,
	MENU_GALLERY,
	MENU_MUSIC,
	MENU_CREDITS,
	MENU_DEMO,
};

enum {
	HERD_ZEUS=0,
	HERD_HERD,
	HERD_CANDY,
	HERD_NH5,
	HERD_DANCER,
	HERD_ZOMBIE,
	HERD_THALIA,
	HERD_ISKUR,
	HERD_ANGEL,
	HERD_WOLF,
	HERD_GODDANCE,
	HERD_DEVIL,
};

#define SHIFT_EARNED_STAGE 6
#define SHIFT_HARD_STAGE 20
#define SHIFT_EARNED_PLAYER 0
#define DISABLE_POWERUPS 0x00001000
#define ENABLE_MUSIC_GAL 0x00002000
#define ENABLE_IMAGE_GAL 0x00004000
#define ENABLE_GHOST_CTL 0x00008000
#define ENABLE_NIGHT_MDE 0x00010000
#define ENABLE_END_CREDS 0x00020000
#define ENABLE_OMAKE	 0x00040000
#define ENABLE_HARD_MODE 0x00080000
#define UNLOCK_UNUSED    0xFC000000

#define PLAYER_CONTINUES 3

struct gameoptions {
	int Timer, Rounds, Win, Powers, NightMode;
	int Skill, GhostMaze, SheepSpeed;
};

struct gamestruct {
	unsigned int nMagic;
	int SVol, MVol, CPU;
	unsigned int AutoSave, NumPlays, UnlockFlags;	// unlock flags should set all unused bits to 1!
	// high score stuff
	char HighName[10][4];	// three initials & termination
	int  HighScore[10];		// Actual score obtained
	unsigned short ChallengeScore[5];		// was character used, now used for challenges
	struct gameoptions Options;
};

// what else would it be? ;)
#define SAVEMAGIC 0xbaaabaab

extern struct gamestruct gGame;
extern struct gameoptions gOptions;
extern maple_device_t *gVMUUsed;
