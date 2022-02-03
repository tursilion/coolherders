/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* score.h                              */
/****************************************/

#define FLAG_FIRSTSTRIKE	0x0001
#define FLAG_UTURN			0x0002
#define FLAG_ATTACKED		0x0004
#define FLAG_VICTIM			0x0008
#define FLAG_STALKER		0x0010
#define FLAG_REVENGE		0x0020
#define FLAG_POWERUP		0x0080
#define FLAG_NOTWINNING		0x0100
#define FLAG_NOTLOSING		0x0200
#define FLAG_MOVED			0x0400

#define FLAG_CHALLENGE_COMPLETE 0x0800

void initScoreBar();
void DrawScoreBar(int isPaused, int nTime);
int  HandleBonuses(int nWinner, int nTime);
int  HandleTimer(int nWinner, int nTime);


// Herder bonus column will support 8 characters across

// Description of bonus scoring
//
//		Time Bonus			100 points per second left
//
//		Sheep bonus			100 points per sheep captured (sheep)
//		Wild sheep			-50 pts for each uncaptured sheep (nTime==0 and calculated by subtracting each herders sheep)
//		Last Second			1000 pts if stage cleared with 1 second left (nTime == 1)
//		Master Herder		3000 pts if pacifist and won (same as pacifist, and nWinner) (exclusive)
//		No contest			2000 pts for being the leader from start to finish (!FLAG_NOTWINNING) (exclusive)
//		First Place			1000 pts for having the most sheep (nWinner) (exclusive)
//		Last Place			-2000 pts for having the fewest sheep (sheep) (exclusive)
//		Whipping boy		-2500 pts for being the loser from start to finish (!FLAG_NOTLOSING) (exclusive)
//		Stalker				-1000 pts if always attacked same herder, +1000 points to victim (FLAG_STALKER and 
//		Feudin'				50 pts each if stalker victim stalked us back (stalker, and lasthit victim last hit us)
//		Backstabber			-500 pts for hitting other players in the back more than 10 times (backstab>10)
//		Butterfingers		-500 pts for losing the most sheep (sheeplost)
//		Punching bag		100 pts if got caught between two players zapping rapidly (punchingbag >= 3)
//		Materialist			100 pts for collecting the most powerups (powerups)
//		Slowpoke			100 pts for getting no speedups (must not be statue) (speed==HERDERSPPED and FLAG_MOVED)
//		Conservationalist	100 pts for getting no lightning powerups (must not be statue) (range==2 and FLAG_MOVED)
//		Most masochistic	500 pts for being zapped the most times (zapped)
//		Statue				500 pts for standing still more than 10 seconds (idletime>600 and !nWinner)
//		Avenger				500 pts if you zap the person who zapped you within 1 second (FLAG_REVENGE)
//		Scavenger Hunt		800 pts for zapping more than 10 crates (c_attacks > 10)
//		First Strike		1000 pts for being the first to hit another herder (FLAG_FIRSTSTRIKE)
//		Fastest reflexes	1000 pts for shortest average time between losing sheep and regaining them 
//							(only the line at max_sheep counts - you get one loss for dropping below your
//							max and the timer ticks till you are back at it. You must have all your sheep
//							back to earn this bonus.)
//		Never look back		1000 pts for never turning around 180 degrees (must not be statue) (!FLAG_UTURN and FLAG_MOVED)
//		Quick Dodge			1000 pts if never hit (must not be statue) (!FLAG_VICTIM and FLAG_MOVED)
//		Minimalist			1000 pts for collecting the fewest powerups (powerups > 0 and powerups)
//		Track star			1000 pts for getting max speedup (speed==HERDERMAXSPEED)
//		Zeus				1000 pts for getting max lightning powerup (range==MAXLIGHTNING)
//		Berzerker			1500 pts for never letting the berserker timer count down (takes 1 second, berserker>0)
//		Back to basics		1500 pts for collecting no powerups (powerups==0 and FLAG_MOVED)
//		Bar-B-Que			1500 pts for zapping sheep more than 50 times (s_attacks > 50)
//		Pacifist			2000 pts if never attacked player or sheep (must not be statue) (!FLAG_ATTACKED and FLAG_MOVED)
//		Pity points			50 pts if score less than or equal to 0 (results in 50, not added) (score<=0)
