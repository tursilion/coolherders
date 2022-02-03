/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* storymode.c                          */
/****************************************/

#include <stdio.h>
#include <malloc.h>

#ifdef WIN32
#include "kosemulation.h"
#else
#include <kos.h>
#include <png/png.h>
#endif

#include "sprite.h"
#include "cool.h"
#include "score.h"
#include "font.h"
#include "rand.h"
#include "levels.h"
#include "sound.h"
#include "control.h"
#include "sheep.h"
#include "menu.h"		// for globals
#include "storymode.h"
#include "cheat.h"

extern pvr_ptr_t disc_txr;
extern pvr_ptr_t txr_misc[4];
extern int nFrames;
extern int gReturnToMenu;
extern int StoryModeTotalScore;
extern int level, stage;
extern int nContinues;
extern int lastbuttons[4];
extern int gAlwaysWinStoryMode;
extern int gIsHardStory;
extern void flush_png_level_cache();
extern void credzMain(void);

#define SCORE_Y 42

int gStoryModeSpecialFlags;

// Intros for the scroll out
char StoryIntroText[MAX_LEVELS][25] = {
	"New Zealand\nTraining",
	"CandyLand",
	"Hades' Haunted House",
	"Thalia's Toy Factory",
	"Disco Fever!",
	"Styx Waterworks"
};

// track who the human player is
int gHumanPlayer=-1;
int gAYBText=0;

// to save memory lost to padding, this is a 1d array
// of nul-delimited strings. 
// Text to first NUL - text displayed on one screen (may start with ~X to indicate *LEVEL* X)
// Repeat above till first empty string
// Next text till NUL - instructional text for level (may be empty)
// Next text till NUL - command (may be empty to do next phase)

// You'll have to seek through it for the one you want.
// No commas in this list! ;) Must manually NUL terminate!
char sStory[] = {
	// 0
//	 12345678901234567890123456789012
	"~0"
	"Iskur: Ha ha ha!\0"
	"Iskur: All the sheep in the\nworld belong to me!\0"
	"Iskur: Tell Zeus that if he\nwants them back...\0"
	"Iskur: He will have to find me\nand TAKE them!\0"
	"\0"
	"\0"	// no instructional text
	"\0"	// no level to be played

	// 1
//	 12345678901234567890123456789012
	"Zeus: What's that, Chrys?\nIskur took them ALL??\0"
	"Zeus: I see.\0"
	"Zeus: It's okay, little one.\nI'll get them back!\0"
	"Old Herder: It looks like he\nmissed a few, Zeus.\0"
	"Old Herder: We will train you\nfor your journey.\0"
	"Old Herder: Let's see how well\nyou can herd these sheep.\0"
	"\0"
	"CHALLENGE STAGE 1\nCollect as many sheep as\npossible in 30 seconds.\nThe herders have given you full\npowers, so impress them!\nYou must collect at least 40\nsheep to pass.\0"
	"C140\0"	// challenge stage 1 for 40 sheep

	// 2 
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
	"Old Herder: You did pretty well!\0"
	"Old Herder: We will compete with\nyou now to make sure you have\nthe right idea!\0"
	"Old Herder: Let's see you get\nmore sheep than any of us!\0"
	"\0"
	"Collect the most sheep to win\nthis round!\nYou will have full power\nand speed!\0"
	"S11\0"	// play for sheep, level 1, out of 1 rounds

	// 3
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
	"Old Herder: Good work, Zeus!\0"
	"Old Herder: Outside of the\ntraining field, you won't have\nthe advantage of full powerups.\0"
	"Old Herder: Let's do it again,\nthis time you will have to\ncollect powerups to beat us!\0"
	"\0"
	"Collect the most sheep to win\nthis round!\nThe free ride is over -\nyou will have to collect\npowerups to improve your\nspeed and lightning!\0"
	"S112\0"	// play for sheep, level 1, out of 1 rounds, on stage 2

	// 4
//	 12345678901234567890123456789012
	"Old Herder: Great work, Zeus!\0"
	"Old Herder: You have learned all\nthat we can teach you.\0"
	"Old Herder: Go now, and bring\nback the sheep!\0"
	"Zeus: I won't let you down.\nCome on, Chrys!\0"
	"\0"
	"\0"	// no instructional text
	"E\0"	// end of level

	// 5
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"~1"
	"Zeus: Look at all the sheep\ntrapped in with the candy!\0"
	"Zeus: We've got to help them!\0"
	"\0"
	"CHALLENGE STAGE 2\nYou have 15 seconds to collect\nas many trapped sheep as\npossible. You will have full\npowers one last time.\nYou must collect at least 25\nsheep to pass.\0"
 	"C225\0" // get 25 sheep

	// 6
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Candi1: What are you doing??\0"
	"Candi2: He's trying to ruin our\ncandy!\0"
	"Candi2: He's let his sheep run\nall over our village!\0"
	"Zeus: No! I'm just trying to\nround them up!\0"
	"Candi1: They've ruined our candy!\nFor payment, they will\nbelong to us now!\0"
	"Candi2: If you want them back\n then meet our challenge.\0"
	"Candi2: Earn more points than us\nherding, and we'll give them\nback.\0"
	"\0"
	"Earn the most points to win\neach round!\nFirst to reach 2 wins\ntakes the match.\nBeware - the candy\nstripers play as a team\nand pool their wins.\0"
	"P23\0"	// play for points, out of 3 rounds

	// 7
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Zeus: ...so Iskur stole all the\nsheep, and I have to get\nthem back.\0"
	"Candi1: We understand. We're\nsorry we gave you such a\nhard time.\0"
	"Candi1: You really ARE a\ngreat Herder.\0"
	"Candi2: Take your sheep, and\ngood luck on your journey!\0"
	"\0"
	"\0"	// no instructional text
	"E\0"	// end of level
	
	// 8
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"~2"
	"Zeus: The tracks just... end?\0"
	"Zeus: That's the old Hades\nhouse... maybe I could\nask there.\0"
	"\0"
	"\0"	// no instructional text
	"\0"	// just do the next story phase

	// 9
//   12345678901234567890123456789012 2345678901234567890123456789012
	"Zeus: AH!! A ghost! ..sheep?\0"
	"Zeus: There are so many of them!\nHow will I find Hades?\0"
	"Zeus: I'll have to try to find\nhim inside the house!\0"
	"\0"
	"\0"	// no instructional text
	"B\0"	// fade to black and do next story

	// 10 (no picture)
	"Zeus: Wow... it's sure dark\nin here!\0"
	"\0" 
	"CHALLENGE STAGE 3\nFind Hades inside one of the\npumpkins. Ghost sheep are\nnot affected by your lightning\nand their touch will stun you!\nYou must find Hades within 25\nseconds to pass!\0"
	"C325\0"	// challenge stage 3 - within 25 seconds

	// 10
//   12345678901234567890123456789012 2345678901234567890123456789012
	"Zeus: I've found you! Er...\0"
	"Zeus: Why are you hiding\nin a pumpkin?\0"
	"Hades: I'm just trying to get\na few moments peace from\nthese annoying sheep!\0"
	"Zeus: I wish I could help, but\nmy staff doesn't\nwork on them!\0"
	"Hades: Oh, you're some kind of\na Herder, are you?\0"
	"Zeus: Yes! I'm trying to find\nIskur, who stole all\nmy sheep!\0"
	"Hades: I can give you the power\nto capture ghost sheep.\0"
	"Hades: I'll make you a little\nwager, since you're a pro...\0"
	"Hades: If you can get more sheep\nfrom my rooms than my\nzombies and I...\0"
	"Hades: I can tell you where\nIskur was headed.\0"
	"\0"
	"Collect the most sheep to win\neach round!\nYou can now zap and capture\nthe ghost sheep normally!\nFirst to reach 2 wins\ntakes the match.\nBeware - Hades and\nhis zombies play as a team\nand pool their wins.\0"
	"S33\0"	// play for sheep, best out of 3

	// 12
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Hades: You did a pretty good\njob after all.\0"
	"Hades: Those ghosts are such a\nnuisance, and my zombies are\npretty useless.\0"
	"Zeus: You're welcome.\0"
	"Zeus: I was wondering if you\ncould help me?\0"
	"Hades: You want to know where\nIskur was going?\0"
	"Zeus: Any clues you can give me\nwould be helpful.\0"
	"Hades: I heard a rumor that\nIskur has been seen with the\nGod of Dance lately.\0"
	"Hades: Try the Disco.\0"
	"\0"
	"\0"	// no instructional text
	"E\0"	// end of level

	// 13
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"~3"
	"Zeus: Hey, there's Thalia!\nI wonder how she's\ndoing!\0"
	"Thalia: Zeus! Zeus! Wait!\0"
	"Zeus: Thalia? What is it?\0"
	"Thalia: Zeus, you have to\nhelp me!\0"
	"Thalia: Iskur passed by, with\nhordes of sheep trailing\nbehind him.\0"
	"Thalia: But some stragglers\nstayed here! They're all\nover my factory!\0"
	"Zeus: I can help.\nLet's start in shipping.\0"
	"\0"
	"CHALLENGE STAGE 4\nThe sheep in the factory's\nshipping department are riding\nthe conveyor belts for fun!\nSave as many as you can from\nfalling into the delivery tubes!\nThalia has powered you up so\nyou are at full speed and power.\nThe round will end when a\nsheep falls into a delivery tube!\nYou must save at least 14\nsheep to pass.\0"
	"C414\0"	// challenge stage 4 - 14 sheep

	// 14
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Thalia: Shipping was the easy\npart, they're all over the\nmain factory, too!\0"
	"Thalia: You have to help me get\nthem out of here so we can\nget back to work!\0"
	"Thalia: Tell you what,\nI'll give you a kiss if you\nget more points than we do!\0"
	"\0"
	"Zeus wants that kiss!\nEarn the most points to win\neach round!\nFirst to reach 2 wins\ntakes the match.\nBeware - Thalia and\nher model NH-5 robots play as a\nteam and pool their wins.\0"
	"P43\0"	// play for points, best out of 3

	// 15
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Zeus: That's the last of them!\0"
	"Thalia: Ah, Zeus! Thank you so\nmuch! \"KISS!\"\0"
	"Zeus: \"Bliss!\"\0"
	"Thalia: You know, I could really\nuse your help around here\nmore often.\0"
	"Zeus: I know, Thalia, but you\nknow I have to stop Iskur.\0"
	"Thalia: I know. Come back and\nsee me again when you're\ndone saving the world.\0"
	"\0"
	"\0"	// no instructional text
	"E\0"	// end of level

	// 16
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"~4"
	"Zeus: There it is, Chrys. And\nit looks like Iskur has\nbeen here, too.\0"
	"Zeus: We're going to have to get\ninside to find him and\nthe sheep!\0"
	"\0"
	"\0"	// no instructional text
	"B\0"	// fade to black and continue

	// 17 (no picture)
	"Zeus: Wow... it's sure dark\nin here!\0"
	"Dancer: Yo! Did you hear that?\0"
	"\0"
	"CHALLENGE STAGE 5\nSneak around the darkened disco\nand avoid the backup dancers as\nlong as you can! You will start\nat normal speed and power but\nmay power up normally.\nThe round will end when any one\nof the backup dancers zaps you.\nYou must evade the dancers for\nat least 10 seconds to pass.\0"
	"C510\0"	// evade for at least 10 seconds

	// 18
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"God of Dance: Yo man, be still!\nDon't be such a spaz!\0"
	"Zeus: Didn't anyone tell you\nthat Disco is dead?\0"
	"God of Dance: Don't diss my\nstyle, you're the zappy bone\njacker!\0"
	"God of Dance: My man Iskur ain't\ndown with you bogarting all\nthe sheep.\0"
	"God of Dance: So you just book\nit back to your crib, square!\0"
	"Zeus: Looks like the only way\nto move on is to beat you at\nyour own boogie.\0"
	"God of Dance: Boo-yah!\nLet's dance!\0"
	"\0"
	"Collect the most sheep to win\neach round!\nFirst to reach 2 wins\ntakes the match.\nBeware - the God of Dance and\nhis backup dancers play as a\nteam and pool their wins.\0"
	"S53\0"	// play for sheep, best out of 3

	// 19
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Zeus: Looks like they danced\ntill they dropped.\nWhat's that, Chrys?\0"
	"Zeus: One of the sheep we saved\nknows where Iskur was headed?\0"
	"Zeus: Iskur's in the well??\nNo... the Waterworks!\0"
	"Zeus: I should have guessed!\0"
	"\0"
	"\0"	// no instructional text
	"E\0"	// end of level

	// 20
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"~5"
	"Zeus: We have to be careful,\nChrys. With all this water,\0"
	"Zeus: ...this is the perfect\nplace for Iskur.\0"
	"Zeus: But... where are all the\nsheep? He must have them\nlocked up inside.\0"
	"\0"
	"\0"	// no instructional text
	"\0"	// go to next story panel

	// 21
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Zeus: Chrys!!\0"
	"Iskur: I told her... ALL the\nsheep in the world\nbelong to me.\0"
	"Zeus: Iskur! Release her!\0"
	"Iskur: Oh no, Zeus.\0"
	"Iskur: I have taken your sheep...\nALL your sheep...\0"
	"Iskur: to prove to you that you\nare not the ultimate herder\nin all of creation.\0"
	"Iskur: Clearly I have outclassed\nyou.\0"
	"Iskur: But if you really want\nher back, Zeus... in fact, if\nyou want ANY of them back...\0"
	"Iskur: You only have to do one\nthing: beat me at herding.\0"
	"Zeus: It is SO on!\0"
	"\0"
	"This is it! Defeat Iskur by\ncollecting more sheep than he\ndoes! One-on-one battle time!\nFirst to reach 2 wins\ntakes the match.\0"
	"S63\0"	// Play for sheep, best out of 3

	// 22 - new art coming
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Iskur: You beat me?!\nHow could that be??\0"
	"Iskur: Just counting sheep -\nthat's no evidence of skill!\0"
	"Iskur: Let's try it again, this\ntime based on points!\0"
	"Zeus: Change whatever rules you\nwant, I'll still beat you!\0"
	"\0"
	"Show Iskur you've got strength\nAND style! Defeat Iskur by\nearning more points than he does!\nFirst to reach 2 wins\ntakes the match.\0"
	"P63\0"	// Play for points, best out of 3

	// 23 - new art coming
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Iskur: No!! You will not defeat\nme so easily, Zeus!\0"
	"Zeus: Whoa! Hey - three of you!\nAnd powered up!!\nThat's not fair!\0"
	"Iskur: Ha ha ha! I thought you\nsaid you could defeat me,\nno matter the rules.\0"
	"Iskur: Don't you want your\nlittle sheep friend back?\0"
	"Zeus: Ooh! That's it! All right,\nbring it on!\0"
	"\0"
	"Iskur's going all-out now!\nThis is the final battle!\nEarn the most points to win\neach round!\nFirst to reach 2 wins\ntakes the match.\nBeware - Iskur and his shadows\nplay as a team\nand pool their wins.\0"
	"P631+\0"	// play for points, best out of 3, stage 1, use clones

	// 24
//   12345678901234567890123456789012 2345678901234567890123456789012
	"Iskur: Defeated. How can that\nbe? You are a mere mortal.\0"
	"Zeus: When you have the passion\ninside you, nothing will\never stop you.\0"
	"Iskur: What a typical thing\nto say.\0"
	"Zeus: All right, I think they're\nall here.\0"
	"Zeus: Are you going to keep\nyour end of the deal?\0"
	"Iskur: Begone, already. I've had\nenough of this game.\0"
	"\0"
	"\0"	// no instructional text
	"\0"	// go to the next story pane

	// 25
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Zeus: Finally!! We're home!\0"
	"Zeus: We're ALL home, and I\ndon't think Iskur will be\0"
	"Zeus: bothering us again for a\nlong time!\0"
	"\0"
	"\0"	// no instructional text
	"Q\0"	// end of game

	// end of data marker (actually, only one works)
	"```"
};

char szHardStory[] = {
    // 0
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "~0"
    "Iskur: Ha ha ha!\0"
    "Iskur: You thought that I would\ngive up so easily?\0"
    "Iskur: I have taken all of the\nsheep again...\0"
    "Iskur: And this time Zeus won't\nget them back!\0"
    "\0"
    "\0"     // no instructional text
    "\0"     // no level to be played

    // 1
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "Old Herder: He missed a few\nagain, didn't he, Zeus?\0"
    "Zeus: Yes he did.\0"
    "Zeus: But this is getting a\nlittle old.\0"
    "Zeus: My sheep just want to\nlive in peace.\0"
    "Old Herder: I think the only\nway that will happen is if you\ndefeat Iskur once and for all.\0"
    "Old Herder: You can do it, but\nyou'll need more practice!\0"
    "Old Herder: Let's start by\ncollecting more sheep!\0"
    "\0"
    "CHALLENGE STAGE 1\nCollect as many sheep as\npossible in 30 seconds.\nThe herders have given you full\npowers, so impress them!\nYou must collect at least 95\nsheep to pass.\0"
    "C195\0"     // challenge stage 1 for 95 sheep

    // 2
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "Old Herder: Your skills really\nhave improved!\0"
    "Old Herder: We will compete with\nyou again to make sure you have\nthe right idea!\0"
    "Old Herder: We have improved\nour skills too!\0"
    "Old Herder: Let's see you get\nmore sheep than any of us!\0"
    "\0"
    "Collect the most sheep to win\nthis round!\nYou will have full power\nand speed!\0"
    "S11\0"     // play for sheep, level 1, out of 1 rounds

    // 3
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "Zeus: I did it!\0"
    "Old Herder: Yes, but you had\nan advantage, Zeus.\0"
    "Old Herder: We allowed you to\nhave full powerups that round.\0"
    "Old Herder: To prove you are\nreally ready...\0"
    "Old Herder: you must defeat us\nagain with no advantage!\0"
    "\0"
    "Collect the most sheep to win\nthis round!\nThe free ride is over -\nyou will have to collect\npowerups to improve your\nspeed and lightning!\0"
    "S112\0"     // play for sheep, level 1, out of 1 rounds, on stage 2

    // 4
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "Old Herder 1: You did it again,\nZeus! Great work!\0"
    "Zeus: I just have one\nquestion...\0"
    "Old Herder 3: Sure Zeus.\nWhat is it?\0"
    "Zeus: Why was it that you three\nsometimes attack each other\nwhile herding?\0"
    "Old Herder 2: Even though we are\na team, we all want to win.\0"
    "Old Herder 3: No matter who your\nenemy is, they will often\nfight each other.\0"
    "Old Herder 1: Use this knowledge\nto your advantage, Zeus!\0"
    "All Herders: Good luck!\0"
    "Zeus: We won't let you down,\nright Chrys?\0"
    "Chrys: Baaaaah!\n\"We sure won't!\"\0"
    "\0"
    "\0"     // no instructional text
    "E\0"     // end of level

    // 5
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "~1"
    "Zeus: This is a very sticky\nsituation again!\0"
    "\0"
    "CHALLENGE STAGE 2\nYou have 15 seconds to collect\nas many trapped sheep as\npossible. You will have full\npowers one last time.\nYou must collect at least 35\nsheep to pass.\0"
    "C235\0"

    // 6
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "Candi1: You're back again??\0"
    "Candi2: And ruining our candy,\nno doubt!\0"
    "Candi2: Look at all these\nstray sheep!\0"
    "Candi1: Why don't you learn to\ntake care of them?\0"
    "Zeus: I'm so sorry.\0"
    "Zeus: Iskur stole all the\nsheep again.\0"
    "Candi2: Again?!? You must\nbe kidding.\0"
    "Candi1: Lying just to get\nour candy?\0"
    "Candi2: Let's make him pay\nthe price!\0"
    "Zeus: Here we go again...\0"
    "\0"
    "Earn the most points to win\neach round!\nFirst to reach 2 wins\ntakes the match.\nBeware - the candy\nstripers play as a team\nand pool their wins.\0"
    "P23\0"     // play for points, out of 3 rounds

    // 7
//      12345678901234567890123456789012 2345678901234567890123456789012
    "Zeus: I'm serious. Iskur stole\nthe sheep again.\0"
    "Zeus: After I get them back, Chrys\nand I will help make new candy.\0"
    "Candi1: Really?\0"
    "Zeus: Sure. Isn't that\nright, Chrys?\0"
    "Chrys: Baaaaah!\0"
    "Candi2: Well, okay...\0"
    "Candi2: Chrys is too cute\nto resist.\0"
    "Candi1: And so are you!\0"
    "Zeus: ...!\0"
    "Candi2: Erm... good luck on the\nrest of your trip!\0"
    "Candi1: And we hope to see you\nboth VERY soon!\0"
    "\0"
    "\0"     // no instructional text
    "E\0"     // end of level

    // 8
//      12345678901234567890123456789012 2345678901234567890123456789012
    "~2"
    "Zeus: After those Candi girls this\nold house is certainly not\na pretty sight!\0"
    "Zeus: Well, looks like I need to\ntalk with Hades again.\0"
    "Zeus: Here goes nothing!\0"
    "\0"
    "\0"     // no instructional text
    "\0"     // just do the next story phase

    // 9
//   12345678901234567890123456789012 2345678901234567890123456789012
    "Zeus: Ack! Er...\nDon't be scared, Chrys.\0"
    "Zeus: These are just those\nnice ghost sheep again.\0"
    "Zeus: Right?\0"
    "\0"
    "\0"     // no instructional text
    "B\0"     // fade to black and do next story

    // 10 (no picture)
    "Zeus: Uh oh!\nHere we go again!\0"
    "\0"
    "CHALLENGE STAGE 3\nFind Hades inside one of the\npumpkins. Ghost sheep are\nnot affected by your lightning\nand their touch will stun you!\nYou must find Hades within 15\nseconds to pass!\0"
    "C315\0"     // challenge stage 3

    // 10
//   12345678901234567890123456789012 2345678901234567890123456789012
    "Hades: Zeus, you're back!\0"
    "Zeus: Yes. Iskur stole the\nsheep again.\0"
    "Zeus: Would you get out\nof that pumpkin?\0"
    "Hades: Only if you prove to\nme you are ready for Iskur again!\0"
    "Hades: If you can get more sheep\nfrom my rooms than my\nzombies and I...\0"
    "Hades: I'll tell you where\nIskur was headed.\0"
    "Zeus: Isn't it the Disco like\nlast time?\0"
    "Hades: Battle me and my zombies\nto find out!\0"
    "Hades: Your staff will again\nwork on the ghost sheep.\0"
    "\0"
    "Collect the most sheep to win\neach round!\nYou can now zap and capture\nthe ghost sheep normally!\nFirst to reach 2 wins\ntakes the match.\nBeware - Hades and\nhis zombies play as a team\nand pool their wins.\0"
    "S33\0"     // play for sheep, best out of 3

    // 12
//   12345678901234567890123456789012 2345678901234567890123456789012
    "Hades: Whoa, whoa, whoa!\nNice shootin', Tex!\0"
    "Zeus: Thanks... I think.\0"
    "Zeus: Now where do you\nthink Iskur is?\0"
    "Hades: Well, the Disco,\nof course.\0"
    "Zeus: Big surprise.\0"
    "Hades: I can see that Disco\nFever in your eyes.\0"
	"Hades: Go put on a good show!\0"
    "\0"
    "\0"     // no instructional text
    "E\0"     // end of level

    // 13
//   12345678901234567890123456789012 2345678901234567890123456789012
    "~3"
    "Zeus: Thalia!\nWant to go to the Disco?\0"
    "Thalia: I can't!\0"
    "Thalia: The factory is\noverrun again.\0"
    "Thalia: You have to help save\nthe sheep before it's\ntoo late!\0"
    "Zeus: Well Chrys, we better save\nyour friends again, huh?\0"
    "Chrys: Baaaah!\n\"Let's go!\"\0"
    "\0"
    "CHALLENGE STAGE 4\nThe sheep in the factory's\nshipping department are riding\nthe conveyor belts for fun!\nSave as many as you can from\nfalling into the delivery tubes!\nThalia has powered you up so\nyou are at full speed and power.\nThe round will end when a\nsheep falls into a delivery tube!\nYou must save at least 25\nsheep to pass.\0"
    "C425\0"     // challenge stage 4

    // 14
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "Thalia: Well... ready for\nthe rest?\0"
    "Thalia: I'm going to play\nharder to get!\0"
    "Thalia: Same rules as\nlast time...\0"
    "Thalia: If you win...\nyou get a kiss!\0"
    "Zeus: You're on!\0"
    "Chrys: Baaaah!\n\"He's falling for her? Ewe!\"\0"
    "\0"
    "Zeus wants that kiss!\nEarn the most points to win\neach round!\nFirst to reach 2 wins\ntakes the match.\nBeware - Thalia and\nher model NH-5 robots play as a\nteam and pool their wins.\0"
    "P43\0"     // play for points, best out of 3

    // 15
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "Zeus: We saved them again!\0"
    "Thalia: Ah, Zeus! Thank you!\n\"KISS!\"\0"
    "Zeus: \"Bliss!\"\0"
    "Thalia: Come back when you are\ndone with Iskur!\0"
    "Zeus: I will!\0"
    "\0"
    "\0"     // no instructional text
    "E\0"     // end of level

    // 16
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "~4"
    "Zeus: Chrys, do you remember\nthe password?\0"
    "Chrys: Baaaah!?\n\"There was a password!?\"\0"
    "Chrys: Baaaah?\n\"Walt sent me?\"\0"
    "Bouncer: Wrong club, buddy.\0"
    "Zeus: I think we'll have to\nsneak in again, Chrys.\0"
    "\0"
    "\0"     // no instructional text
    "B\0"     // fade to black and continue

    // 17 (no picture)
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "Zeus: Uh oh!\nHere come the dancers...\0"
    "Zeus: ...and I don't think\nthey're offering drinks!\0"
    "\0"
    "CHALLENGE STAGE 5\nSneak around the darkened disco\nand avoid the backup dancers as\nlong as you can! You will start\nat normal speed and power but\nmay power up normally.\nThe round will end when any one\nof the backup dancers zaps you.\nYou must evade the dancers for\nat least 15 seconds to pass.\0"
    "C515\0"

    // 18
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "God of Dance: Oh! It's\nyou again!\0"
    "God of Dance: I need to repay\nyou for last time.\0"
    "God of Dance: Get ready!\nLet's see you dance!\0"
    "\0"
    "Collect the most sheep to win\neach round!\nFirst to reach 2 wins\ntakes the match.\nBeware - the God of Dance and\nhis backup dancers play as a\nteam and pool their wins.\0"
    "S53\0"     // play for sheep, best out of 3

    // 19
//      12345678901234567890123456789012 2345678901234567890123456789012
    "Zeus: He sure didn't have\nFeet of Fury, did he Chrys?\0"
    "Chrys: Baaaah!\n\"No way!\"\0"
    "Zeus: Well, now that the God of\nDance is defeated,\nwe must go back...\0"
    "Zeus: Back...\0"
    "Zeus: to the Waterworks!\0"
    "Chrys: Baaah!\n\"Great Scott!\"\0"
    "\0"
    "\0"     // no instructional text
    "E\0"     // end of level

    // 20
//   12345678901234567890123456789012 2345678901234567890123456789012
    "~5"
    "Zeus: We made it!\0"
    "Zeus: Iskur will be defeated\nonce and for all.\0"
    "Zeus: Let's go save\nyour friends!\0"
    "\0"
    "\0"     // no instructional text
    "\0"     // go to next story panel

    // 21
//   12345678901234567890123456789012 2345678901234567890123456789012
    "Zeus: Chrys!! Why didn't I\nremember to watch for that?!\0"
    "Iskur: Chrys is mine now!\0"
    "Zeus: Iskur! Release her!\0"
    "Iskur: Oh no, Zeus.\0"
    "Iskur: This is my first step in\na plot to rule the WORLD!\0"
    "Iskur: Your sheep will never\nbe at peace!\0"
    "Zeus: No! Leave them be!\0"
    "Iskur: NEVER!\0"
    "\0"
    "This is it! Defeat Iskur by\ncollecting more sheep than he\ndoes! One-on-one battle time!\nFirst to reach 2 wins\ntakes the match.\0"
    "S63\0"     // Play for sheep, best out of 3

    // 22 - new art coming
//   12345678901234567890123456789012 2345678901234567890123456789012
    "Iskur: How could I fail?!\nI am much more powerful!\0"
    "Iskur: You must have cheated!\nLet's try that again!\0"
    "Iskur: And this time you must\nbeat me based on points!\0"
    "Zeus: I'll do it again! Chrys\nand her friends need me!\0"
    "\0"
    "Show Iskur you've got strength\nAND style! Defeat Iskur by\nearning more points than he does!\nFirst to reach 2 wins\ntakes the match.\0"
    "P63\0"     // Play for points, best out of 3

    // 23 - new art coming
//   12345678901234567890123456789012 2345678901234567890123456789012
    "Zeus: Wait, you split in three\nagain, this isn't fair!\0"
    "Iskur: Us bad guys always have\nnifty special powers.\0"
    "Zeus: Who do you think you are?!\0"
	"Iskur: Zeus... I am your father\nIskur2: ...your uncle          \nIskur3: ...and your brother!   \0"
	"Zues: What?!?!\0"
	"Iskur: Okay, maybe not.\0"
    "Zeus: That's it! I'll defeat you\nwith just my heart and skill!\0"
    "Zeus: Let's get it on!\0"
    "\0"
    "Iskur's going all-out now!\nThis is the final battle!\nEarn the most points to win\neach round!\nFirst to reach 2 wins\ntakes the match.\nBeware - Iskur and his shadows\nplay as a team\nand pool their wins.\0"
    "P631+\0"     // play for points, best out of 3, stage 1, use clones

    // 24
//   12345678901234567890123456789012 2345678901234567890123456789012
    "Iskur: Defeated.\0"
    "Iskur: Again.\0"
    "Iskur: You must be the chosen\none, or something\nsimilarly trite.\0"
    "Zeus: Like I told you\nlast time...\0"
    "Zeus: When you have the passion\ninside you, nothing will\never stop you.\0"
    "Iskur: Well, I guess\nyou're right.\0"
    "Iskur: I've done all that I can.\0"
    "\0"
    "\0"     // no instructional text
    "\0"     // go to the next story pane

    // 25
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
    "Zeus: We're home again!\0"
    "Zeus: Chrys, do you think Iskur\nlearned his lesson?\0"
    "Chrys: Baaah.\n\"I think so.\"\0"
    "Chrys: Baaah!\n\"You defeated him twice,\nafter all.\"\0"
    "Chrys: Baaah!\n\"And it seems he took your\nlesson to heart.\"\0"
    "Zeus: You're right, Chrys.\0"
    "Zeus: We can all finally\nbe at peace!\0"
    "The End.\0"
    "Really!\0"
    "\0"
    "\0"     // no instructional text
    "Q\0"     // end of game

    // end of data marker (actually, only one works)
    "```"
};

char sAltStory[] = {
	// 0
//	 12345678901234567890123456789012
	"~0"
	"Iskur: All your sheep are\nbelong to me!\0"
	"Iskur: Zeus have no chance to\nsurvive! Make your time!\0"
	"Iskur: Ha ha ha ha!\0"
	"\0"
	"\0"	// no instructional text
	"\0"	// go to the next story pane

	// 1
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Old Herder: What happen!\0"
	"Zeus: Someone steal up us\nthe sheep!!\0"
	"Old Herder: What you say!!\0"
	"Old Herder: We train for your\ntravelling.\0"
	"Old Herder: Just which it can\ngather these sheep well,\nyou will see.\0"
	"\0"
	"CHALLENGE STAGE 1\nGather many sheep as much\nas possible in 30 seconds.\nHerders gave all power,\ntherefore impress those!\0"
	"C1\0"	// challenge stage 1

	// 2
//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
	"Old Herder: It is good rather!\0"
	"Old Herder: We you now rival\nbecause it has the thought the\nright in order to verify!\0"
	"Old Herder: You will observe at\nthat you obtain us of many sheep\nat times!\0"
	"\0"
	"Gather most sheep in order circle\nrespectively to win! Match it\nwins acquisition in order first\nto reach to 2. Be careful -\nas for old herders with everyone\nto play, their victories\nare shared.\0"
	"S13\0"	// play for sheep, out of 3 rounds

//   12345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
	"Old Herder: Good work and Zeus!\0"
	"Old Herder: Outside training\nfield, it does not have the\nadvantage of complete powerups.\0"
	"Old Herder: You must gather\npowerups in order to strike us\nthis time!\0"
	"\0"
//   12345678901234567890123456789012 2345678901234567890123456789012 234567890123456789012345678901212345678901234567890123456789012 2345678901234567890123456789012 2345678901234567890123456789012
	"Gather most sheep in order to\nwin this circle! Merely, riding\nis excessive - powerups must be\ngathered your speed and in\norder to improve the\nelectric light!\0"
	"S112\0"	// play for sheep, level 1, out of 1 rounds, on stage 2

	// 4
//	 12345678901234567890123456789012
	"Old Herder: You know what you\ndoing.\0"
	"Zeus: For great justice!\0"
	"Old Herder: Bring back\nevery sheep.\0"
	"\0"
	"\0"	// no instructional text
	"E\0"	// end of level
};

char sHowToPlay[] = {
	// 0
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"Your basic goal in Cool Herders\nis to collect sheep.\0"
	"To do this, simply run past the\nsheep, and they will follow you\0"
	"\0"
	"\0"	// no instructional text
	"\0"	// no level to be played

	// 1
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"By pressing A, your staff will\nrelease a blast of lightning\0"
	"Your lightning staff will\nrecharge over time naturally.\0"
	"If it is not recharged, each\nblast will be shorter.\0"
	"Lightning blasts can be used to:\0"
	"Destroy crates and other objects\0"
	"Temporarily stun sheep\0"
	"Stun other players, which will\nknock their captured sheep free.\0"
	"\0"
	"\0"	// no instructional text
	"\0"	// no level to be played

	// 2
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"You can improve your powers by\ndestroying crates and\nother objects\0"
	"The boots will\nimprove your speed\0"
	"The lightning bolts will\nincrease your lightning range\0"
	"Collect these powerups by\nwalking over them.\0"
	"\0"
	"\0"	// no instructional text
	"\0"	// no level to be played

	// 3
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"At the end of each round bonuses\nare awarded\0"
	"The winner of the game will be\nthe herder who earned the\0"
	"most points total during the\nround, including level score\nand bonus points added together.\0"
	"On some stages or in the options\nyou can also set the win mode\nto sheep alone.\0"
	"Watch the screen carefully to\nlearn what tactics\0"
	"will make you a\nCooler Herder!\0"
	"\0"
	"\0"	// no instructional text
	"\0"	// no level to be played

	// 4
//	 12345678901234567890123456789012 2345678901234567890123456789012
	"For more details, please see the\ndocumentation inside\nthe CD cover\0"
	"\0"
	"\0"	// no instructional text
	"\0"	// no level to be played
};

int FindPhase(int nLevel) {
	int nPhase;
	char *pText;
	// Seeks through the list to find the phase number that marks
	// the beginning of the 0-based level passed (0-5)

	nPhase=0;
	pText=gIsHardStory?szHardStory:sStory;

	if (nLevel == 0) {
		return 0;
	}

	for (;;) {
		// Find start of text for this screen
		for (;;) {
			while (*pText != '\0') {
				pText++;
			}
			pText++;
			if (*pText=='\0') {
				pText++;
				// Find end of instruction
				while (*pText != '\0') {
					pText++;
				}
				pText++;
				// Find end of game command
				while (*pText != '\0') {
					pText++;
				}
				// and move to next
				pText++;
				nPhase++;
				break;
			}
		}

		if (*pText=='~') {
			if ((*(pText+1))-'0' == nLevel) {
				break;
			}
		}
		
		if (*pText=='`') {
			nPhase=0;
			break;
		}
	}

	return nPhase;
}

// Show the story sequence for nPhase. 
// Also set up the computer herder types for each stage.
// Returns instructions for the game's next level
char *doStory(int *nInPhase) {
	char *pText, *pTmp;
	int idx;
	int isGame;
	int nPhase=*nInPhase;
	char buf[128];

	isGame=1;
	if (nPhase < 0) {
		isGame=0;
		// how to play mode runs negative
		nPhase*=-1;
		nPhase--;
		pText=sHowToPlay;
	} else {
		// check for alt text (first 4 phases only)
		pText=gIsHardStory?szHardStory:sStory;
		if (nPhase < 4) {
			MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st);
				if (NULL == st) {
					continue;
				}
				if ((st->ltrig >= 127) || (st->rtrig >= 127) || (gAYBText)) {
					pText=sAltStory;
					goto exitscan4;
					break;
				}
			MAPLE_FOREACH_END();
exitscan4: ;
		}
	}

	// Find start of text for this screen
	idx=nPhase;
	while (idx) {
		while (*pText != '\0') {
			pText++;
		}
		pText++;
		if (*pText=='\0') {
			pText++;
			// Find end of instruction
			while (*pText != '\0') {
				pText++;
			}
			pText++;
			// Find end of game command
			while (*pText != '\0') {
				pText++;
			}
			// and move to next
			pText++;
			idx--;
		}
	}

	// Load appropriate image into disc_txr (note: 512x256)
	if (isGame) {
		sprintf(buf, "gfx/Story/story%d.png", nPhase);
	} else {
		sprintf(buf, "gfx/HowToPlay/howtoplay%d.png", nPhase);
	}

	load_png_block(buf, disc_txr, PNG_NOCACHE);	// load texture

	// fade gfx in
	for (gGfxDarken=255; gGfxDarken>0; gGfxDarken-=5) {
		BeginScene();
		
		// Opaque polygons
		pvr_list_begin(PVR_LIST_OP_POLY);
		addPageStory(disc_txr, 1000.0f);
		pvr_list_finish();
		
		// transparent polygons
		pvr_list_begin(PVR_LIST_TR_POLY);
		if (isGame) {
			sprintf(buf, "Total Score: %d", StoryModeTotalScore);
			CenterDrawFontZ(1001.0f, 0, SCORE_Y, DEFAULT_COLOR, buf);
		}
		pvr_list_finish();

		// draw it
		pvr_scene_end;

		if (gReturnToMenu) {
			return "";
		}
	}
	gGfxDarken=0;

	// loop for whole speech
continueStoryMode:
	for (;;) {
		int xStart=0;

		// check for level tag and skip it
		if (*pText == '~') {
			pText++;
			if (*pText) {
				pText++;
			}
		}

		// wait for user input
		for (;;) {
			// draw scene
			BeginScene();
			
			// Opaque polygons
			pvr_list_begin(PVR_LIST_OP_POLY);
			addPageStory(disc_txr, 1000.0f);
			pvr_list_finish();
			
			// transparent polygons
			pvr_list_begin(PVR_LIST_TR_POLY);
			if (isGame) {
				sprintf(buf, "Total Score: %d", StoryModeTotalScore);
				CenterDrawFontZ(1001.0f, 0, SCORE_Y, DEFAULT_COLOR, buf);
			}

			CenterDrawFontBackgroundBreaksZ(1001.0f, 32, 336, DEFAULT_COLOR, INT_PACK_COLOR(255,64,64,255), pText);
			addPage2(txr_sprites, 528, 376, 144, 192, 191, 239, INT_PACK_ALPHA(255-((nFrames%63)*4)), 1002.0f);
			pvr_list_finish();

			// draw it
			pvr_scene_end;

			if (gReturnToMenu) {
				return "";
			}

			xStart=isStartPressed();
			if (xStart) {
				break;
			}
		}

		// wait for it to be released
		while (isStartPressed());

		// check on full exit or not
		if (xStart==CONT_START) {
			// START, not A, was pressed to exit the above loop.
			// Skip ALL text.
			// Find next text for this screen
			while ('\0' != *pText) {
				while (*pText != '\0') {
					pText++;
				}
				pText++;
			}
			break;
		}
	
		// Find next text for this screen
		while (*pText != '\0') {
			pText++;
		}
		pText++;

		if ('\0' == *pText) {
			// done this one
			break;
		}
	}

	pText++;
	// Now we're pointing at the instructional text. If there is any, we fade out the screen
	// and display the instructions overtop
	if (*pText) {
		// count how many linefeeds there are
		idx=2;	// accounts for the border drawn by the font function
		pTmp=pText;
		while (*pTmp) {
			if (*pTmp == '\n') idx++;
			pTmp++;
		}

		// wait for user input
		for (;;) {
			int xStart;

			// draw scene
			BeginScene();
			
			gGfxDarken=160;

			// Opaque polygons
			pvr_list_begin(PVR_LIST_OP_POLY);
			addPageStory(disc_txr, 1000.0f);
			pvr_list_finish();
			
			// transparent polygons
			pvr_list_begin(PVR_LIST_TR_POLY);
			if (isGame) {
				sprintf(buf, "Total Score: %d", StoryModeTotalScore);
				CenterDrawFontZ(1001.0f, 0, SCORE_Y, DEFAULT_COLOR, buf);
			}

			gGfxDarken=0;
			addPage2(txr_sprites, 528, 376, 144, 192, 191, 239, INT_PACK_ALPHA(255-((nFrames%63)*4)), 1002.0f);
			CenterDrawFontBackgroundBreaksZ(1001.0f, 32, 240-((22*idx)/2), DEFAULT_COLOR, INT_PACK_COLOR(192,64,64,255), pText);

			pvr_list_finish();

			// draw it
			pvr_scene_end;

			if (gReturnToMenu) {
				return "";
			}

			xStart=isStartPressed();
			if (xStart) {
				break;
			}
		}

		// wait for it to be released
		while (isStartPressed());

		// Find next text for this screen
		while (*pText != '\0') {
			pText++;
		}
		
		// reset the darken so the fade out starts at the background level
		gGfxDarken=160;
	}
	pText++;
	
	// Set up the computer herders for this level
	// defaults - just in case of an error
	for (idx=0; idx<4; idx++) {
		if (idx == gHumanPlayer) {
			continue;
		}

		// type will be merged in below
		herder[idx].type=PLAY_COMPUTER;

		// Secret: if trigger is being held on other controllers,
		// that player can control the computer
		if (NULL != ControllerState[idx]) {
			cont_state_t * st;
			st = maple_dev_status(ControllerState[idx]);
			if (NULL != st) {
				if ((st->ltrig >= 127) || (st->rtrig >= 127)) {
					// type will be merged in below
					herder[idx].type=PLAY_HUMAN;
					debug("Player %d will be human controlled!\n", idx);
				}
			}
		}
	}

	// First index to set
	idx=0;
	if (idx==gHumanPlayer) idx++;

	// set default game settings
	gOptions.Timer=60;
	gOptions.Rounds=2;
	gOptions.Win=0;	// score
	gOptions.Powers=1;
	gOptions.NightMode&=0xff;// night mode off (but remember the darkness value)
	if (gOptions.NightMode == 0) {
		gOptions.NightMode=NIGHT_DARKEN;
	}
	gOptions.Skill=gIsHardStory?2:1;		// normal - hard mode should use 2
	gOptions.GhostMaze=0;					// normal (haunted only)
	gOptions.SheepSpeed=0;					// normal 

	gStoryModeSpecialFlags=0;
	herder[gHumanPlayer].type=PLAY_HUMAN|HERD_ZEUS;

	// increment the phase for the user
	(*nInPhase)++;

	if (*pText=='C') {
		// challenge mode - just the human in most cases
		herder[idx].type=PLAY_NONE;
		idx++; if (idx==gHumanPlayer) idx++;
		herder[idx].type=PLAY_NONE;
		idx++; if (idx==gHumanPlayer) idx++;
		herder[idx].type=PLAY_NONE;

		switch (*(pText+1)) {
		case '1':	// new zealand herding challenge
			// override some game settings
			gOptions.Timer=30;		// 30 seconds on the clock
			gOptions.Powers=0;		// fully powered up
			gOptions.Win=1;			// win by sheep
			gStoryModeSpecialFlags=EFFECT_CREATE_NEW_SHEEP|EFFECT_WIN_BY_SHEEP;
			break;

		case '2':	// candyland herding challenge
			gOptions.Timer=15;		// only 15 seconds here
			gOptions.Powers=0;		// fully powered up;
			gOptions.Win=1;			// win by sheep
			gStoryModeSpecialFlags=EFFECT_CREATE_NEW_SHEEP|EFFECT_WIN_BY_SHEEP;
			break;

		case '3':	// Haunted house find Hades challenge
			// this has to be mostly handled in the engine too
			gOptions.Timer=-1;		// counting up!
			gOptions.NightMode|=0x8000;
			gStoryModeSpecialFlags=EFFECT_EVERY_CRATE_GIVES_BOOT|EFFECT_LOOKING_FOR_HADES|EFFECT_SHEEP_STUN_PLAYER|EFFECT_SHEEP_IGNORE_LIGHTNING;
			break;

		case '4':	// Toy factory conveyor challenge
			// this has to be mostly handled in the engine
			gOptions.Timer=0;		// infinite time - save the sheepies!
			gOptions.Powers=0;		// fully powered up;
			gOptions.Win=1;			// win by sheep
			gStoryModeSpecialFlags=EFFECT_EVERY_CRATE_GIVES_BOOT|EFFECT_CREATE_NEW_SHEEP|EFFECT_SHEEP_RIDE_CONVEYORS|EFFECT_SHEEP_IGNORE_LIGHTNING|EFFECT_NO_SHEEP|EFFECT_WIN_BY_SHEEP;
			break;

		case '5':	// Disco avoidance challenge
			gOptions.Timer=-1;		// counting up!
			gOptions.NightMode|=0x8000;
			// three backup dancers!
			idx=0;
			if (idx==gHumanPlayer) idx++;
			herder[idx].type=PLAY_COMPUTER|HERD_DANCER;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type=PLAY_COMPUTER|HERD_DANCER;
			idx++; if (idx==gHumanPlayer) idx++;
			herder[idx].type=PLAY_COMPUTER|HERD_DANCER;
			gStoryModeSpecialFlags=EFFECT_GRADUAL_SPEEDUP|EFFECT_END_IF_ZAPPED|EFFECT_NO_SHEEP;
			herder[gHumanPlayer].type|=PLAY_SPECIAL_POWER;
			break;
		}
	} else {
		if ((*pText == 'P')||(*pText == 'S')) {
			idx=0;
			if (idx==gHumanPlayer) idx++;

			switch (*(pText+1)-'1') {
			case LEVEL_NZ:
				herder[idx].type|=HERD_HERD;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type|=HERD_HERD;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type|=HERD_HERD;
				if (*(pText+3)=='\0') {
					herder[gHumanPlayer].type|=PLAY_SPECIAL_POWER;
				}
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
				break;

			case LEVEL_WATER:
				// Deliberate overwrite with PLAY_NONE here - only you vs Iskur most of the rounds
				idx=0;
				if (idx==gHumanPlayer) idx++;
				if (*(pText+4) == '+') {	// note: may overrun into next text if command string is short. Not a problem but be aware.
					// 3 way Iskur ;)
					herder[idx].type|=HERD_ISKUR|PLAY_SPECIAL_CLONE;
					idx++; if (idx==gHumanPlayer) idx++;
					herder[idx].type|=HERD_ISKUR|PLAY_SPECIAL_CLONE;
					idx++; if (idx==gHumanPlayer) idx++;
					herder[idx].type|=HERD_ISKUR|PLAY_SPECIAL_POWER;
				} else {
					herder[idx].type=PLAY_NONE;
					idx++; if (idx==gHumanPlayer) idx++;
					herder[idx].type=PLAY_NONE;
					idx++; if (idx==gHumanPlayer) idx++;
					herder[idx].type|=HERD_ISKUR;
				}
				break;

			default: debug("Story mode error - bad level %d (unless ending!)\n", nPhase/4);
				break;
			}
		}
	}

	// fade out (from whatever gGfxDarken is at now)
	for (; gGfxDarken<255; gGfxDarken+=5) {
		BeginScene();
		
		// Opaque polygons
		pvr_list_begin(PVR_LIST_OP_POLY);
		addPageStory(disc_txr, 1000.0f);
		pvr_list_finish();
		
		// transparent polygons
		pvr_list_begin(PVR_LIST_TR_POLY);
		if (isGame) {
			sprintf(buf, "Total Score: %d", StoryModeTotalScore);
			CenterDrawFontZ(1001.0f, 0, SCORE_Y, DEFAULT_COLOR, buf);
		}
		pvr_list_finish();

		// draw it
		pvr_scene_end;

		if (gReturnToMenu) {
			return "";
		}
	}
	gGfxDarken=0;

	// That leaves pText pointing at the instructions block
	// the only one we bother with here is the fade-to-black one
	if ('B' == *pText) {
		while (*pText != '\0') {
			pText++;
		}
		pText++;
		CLEAR_IMAGE(disc_txr, 512, 512);
		goto continueStoryMode;
	}

	return pText;
}

// Omake is Japanese for Extra (or such). It's a cool word.
void doOmake() {
	int flag;
	int nWorld;
	int idx;
	cont_state_t * st=NULL;
	int nRetries=5;

	sound_stop();

	// get the background picture
	load_png_block("gfx/Menu/World1.png", disc_txr, 0);
	load_png_block("gfx/Menu/World2.png", txr_misc[0], 0);

	// Make sure the lights are on
	gGfxDarken=0;
	
	flag=1;
	while (flag) {
		BeginScene();
		
		// Opaque polygons
		pvr_list_begin(PVR_LIST_OP_POLY);
		
		gGfxDarken=128;
		SortFullPicture(disc_txr, txr_misc[0], 100.0f);
		gGfxDarken=0;

		pvr_list_finish();
		
		// transparent polygons
		pvr_list_begin(PVR_LIST_TR_POLY);
	
		CenterDrawFontBackgroundBreaksZ(110.0f, 1, 44, DEFAULT_COLOR, INT_PACK_COLOR(128, 0,0, 255), 
//           12345678901234567890123456789012
			"Welcome to Omake Mode!\n"
			"\n"
			"Omake Mode is a final challenge\n"
			"for your herding skills!\n"
			"The computer's skill and power\n"
			"has been increased, and you\n"
			"will run though a rapid sequence\n"
			"of each of the stages.\n"
			"All stages are single round,\n"
			"win by points, and no continues\n"
			"are permitted.\n"
			"That means you get one chance\n"
			"to get through EIGHT stages.\n"
			"\n"
			"Press A to accept or B to cancel"
			);

		pvr_list_finish();

		// draw it
		pvr_scene_end;

		// Handle the player's input
		st = maple_dev_status(ControllerState[gHumanPlayer]);
		if (NULL == st) {
			nRetries--;
			if (nRetries) continue;
			// controller failed? drop out.
			return;
		}

		if (st->joyy < -JOYDEAD) st->buttons |= CONT_DPAD_UP;
		if (st->joyy > JOYDEAD)  st->buttons |= CONT_DPAD_DOWN;
		if (st->joyx < -JOYDEAD) st->buttons |= CONT_DPAD_LEFT;
		if (st->joyx > JOYDEAD)  st->buttons |= CONT_DPAD_RIGHT;
		if ((st->ltrig>=127)||(st->rtrig>=127)) st->buttons|=CONT_Z;

		// wait for old keys to be released
		if (st->buttons&lastbuttons[gHumanPlayer]) continue;
		// save old keys
		lastbuttons[gHumanPlayer]=st->buttons;

		// ignore Start *AND* A (same as the left/right check elsewhere
		if ((st->buttons&CONT_START)&&(st->buttons&CONT_A)) {
			continue;
		}

		if (st->buttons & (CONT_START|CONT_A)) {
			flag=0;
			break;
		}

		if (st->buttons & CONT_B) {
			return;
		}

		if (gReturnToMenu) {
			sound_stop();
			return;
		}
	}

	// The player has accepted the challenge!
	gGame.NumPlays++;
	nContinues=0;

	for (nWorld=0; nWorld<MAX_LEVELS; nWorld++) {
		sound_stop();

		// Now set up the stage
		gOptions.Timer=60;
		gOptions.Rounds=1;
		gOptions.Win=0;
		gOptions.Powers=1;
		gOptions.NightMode=0;
		gOptions.Skill=2;
		gOptions.GhostMaze=0;
		gOptions.SheepSpeed=1;
		gStoryModeSpecialFlags=0;
		herder[gHumanPlayer].type=PLAY_HUMAN|HERD_ZEUS;

		if ((gOptions.Timer != 60) || (gOptions.Rounds != 1) || (gOptions.Win != 0) || (gOptions.Powers != 1) || (gOptions.Skill != 2) || (gOptions.SheepSpeed < 1) || ((herder[gHumanPlayer].type & PLAY_SPECIAL_MASK) == PLAY_SPECIAL_POWER)) {
			debug("** HEY! We check these against a cheat device! Search for the above IF statement and fix it!\n");
		}

		// Another switch to set up opponents
		idx=0;
		if (idx==gHumanPlayer) idx++;

		switch (nWorld) {
			case LEVEL_NZ:
				herder[idx].type=PLAY_COMPUTER|HERD_HERD|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=PLAY_COMPUTER|HERD_HERD|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=PLAY_COMPUTER|HERD_HERD|PLAY_SPECIAL_POWER;
				break;

			case LEVEL_CANDY:
				herder[idx].type=HERD_CANDY|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_CANDY|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_CANDY|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				break;

			case LEVEL_HAUNTED:
				herder[idx].type=HERD_ZOMBIE|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_ZOMBIE|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_WOLF|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				break;

			case LEVEL_TOY:
				herder[idx].type=HERD_NH5|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_NH5|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_THALIA|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				break;

			case LEVEL_DISCO:
				herder[idx].type=HERD_DANCER|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_DANCER|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_GODDANCE|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				break;

			case LEVEL_WATER:
				herder[idx].type=HERD_ISKUR|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_ISKUR|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_ISKUR|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				break;

			case LEVEL_HEAVEN:
				herder[idx].type=HERD_ANGEL|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_ANGEL|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_ANGEL|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				break;

			case LEVEL_HELL:
				herder[idx].type=HERD_DEVIL|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_DEVIL|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				idx++; if (idx==gHumanPlayer) idx++;
				herder[idx].type=HERD_DEVIL|PLAY_COMPUTER|PLAY_SPECIAL_POWER;
				break;

			default: debug("Omake mode error - bad level %d (unless ending!)\n", nWorld);
				break;
		}

		level=nWorld;
		stage=2;

		Game(-1);
		
		// should already be stopped, but just be sure
		sound_stop();

		if (gReturnToMenu) {
			return;
		}

		flag=GetFinalWinner();
		if (flag != gHumanPlayer) {
			doGameOver();
			return;
		}
	}

	if (gAlwaysWinStoryMode) {
		ShowUnlockMessage("Nice, but....\nyou were kinda using the\ncheat code, weren't you?\nI can't reward you for that!", 0, 0);
		return;
	}

	if ((gOptions.Timer != 60) || (gOptions.Rounds != 1) || (gOptions.Win != 0) || (gOptions.Powers != 1) || (gOptions.Skill != 2) || (gOptions.SheepSpeed < 1) || ((herder[gHumanPlayer].type & PLAY_SPECIAL_MASK) == PLAY_SPECIAL_POWER)) {
		ShowUnlockMessage("Nice, but....\nyou seem to be using a\ncheat device?\nWhat, MY codes weren't good enough?\nSorry, no reward!", 0, 0);
		return;
	}

	// Better congrats screen? :)
	// player won! Do credits!
	credzMain();
	if (!gReturnToMenu) {
		Message();
	}
}
