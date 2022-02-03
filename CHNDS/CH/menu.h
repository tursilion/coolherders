/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* menu.h                               */
/****************************************/

#ifndef _MENU_H_
#define _MENU_H_

#define MENUX 40
#define MENUY 364

int HandleMenus();
void HandleMainMenus();
int doJoinMultiMenu();
int doPlayerSelect(int nRecall, void (*CallBack)());
int doStageSelect(int nLevel, void (*CallBack)());
void doBonusMenu();
int doMenu(int nMenu, void (*CallBack)());
int doOptionsMenu();
int doBattleOptionsMenu(void (*CallBack)());
void AddHighScore(int nWho, int nGame, int nCount);
void ShowHighScores(int nTimeout);
void TweakSubscreenMapToBackground();
void doWirelessError();
void doLostWireless();
void doHostQuit();

// -1 - Cancel, Return to title
// Menu tokens
enum {
	MENU_CANCEL = -1,
	MENU_PRESSSTART,	// 0
	
	MENU_MAIN,
		MENU_STORY,
			MENU_STORY_NEW,
			MENU_STORY_CONTINUE,
		MENU_STORY_TEXT,

		MENU_MULTIPLAYER,
			MENU_QUICKPLAY,
			MENU_HOST_GAME,
			MENU_JOIN_GAME,
				MENU_JOIN_MULTIPLAYER,
				MENU_JOIN_SCAN,
			MENU_WIRELESS_GAME,
				MENU_LEVEL_SELECT,
				MENU_CHARACTER_SELECT,
				MENU_START,
			
		MENU_OPTIONS,
		MENU_BATTLE_OPTIONS,
		
		MENU_EXTRAS,
			MENU_FILE,
				MENU_AUTOSAVE,
				MENU_LOAD,
					MENU_LOAD_A,
					MENU_LOAD_B,
					MENU_LOAD_C,
				MENU_SAVE,
					MENU_SAVE_A,
					MENU_SAVE_B,
					MENU_SAVE_C,
			MENU_BONUS,
				MENU_MINIGAMES,
					MENU_MINI_CANDY,
					MENU_MINI_WATER,
				MENU_HIGHSCORES,
				MENU_MUSICPLAYER,
				MENU_CREDITS,
	
	MENU_PAUSE,
		MENU_CONTINUE,
		MENU_QUIT,
			MENU_QUIT_YES,
			MENU_QUIT_NO,
			
	MENU_SYS_INFO,		// TODO: presently useless, but a convenient place to stick stats during dev, maybe?
	MENU_DEMO,
	MENU_STAGEINTRO_TEXT,
	
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
	
	HERD_CHRYS,
	HERD_AFROZEUS,
	HERD_AFROCHRYS
};

#define PLAYER_CONTINUES 3

// power ups
#define POW_POWERUPS 0x01
#define POW_SPECIALS 0x02

struct gameoptions {
	unsigned char Timer, Rounds, Powers;	// powers: 0x01 = powerups, 0x02 = specials
	unsigned char Skill, CPU;
	signed char SheepSpeed;
};

const int MAX_WIRELESS_GAMES = 5;	// must be 6 or less

struct multigame {
	char GameName[11]; 			// based on user id of DS which is limited to 10 chars (plus NUL)
	unsigned char MAC[6];		// MAC address of the host
};

struct multiplay_options {
	unsigned char fHosting;
	struct multigame MPGames[MAX_WIRELESS_GAMES]; // should be enough
};

struct gamestruct {
	unsigned int nMagic;
	unsigned char SVol, MVol;
	unsigned char AutoSave, continuelevel;
	unsigned int SaveSlot;					// remembers the most recently used save slot
	// high score stuff
	char HighName[10][4];					// three initials & termination
	int  HighScore[10];						// Actual score obtained
	unsigned short ChallengeScore[5];		// maximum score per challenge
	char ChallengeName[5][4];				// three initials & termination
	struct gameoptions Options;				// saved versions of options
};

extern struct gamestruct gGame;
extern struct gameoptions gOptions;			// used during the game, temporary versions
extern struct multiplay_options gMPStuff;	// multiplayer information
extern char CharacterNames[15][11];

#endif
