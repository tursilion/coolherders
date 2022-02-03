/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* vmu_logo.c                           */
/****************************************/
/* Adapted from code by Dan Potter, FoF */
/* and the GOAT libmenu                 */
/****************************************/

// Nintendo DS save scheme created by Keith Henrickson
// We have 256 pages of 32 bytes each.  Taking 8 pages together gives us
// convenient sectors of 256 bytes, 32 of them to be exact.  They are used
// as follows:
// 0-7: System save state
// 8-15: Save block 1
// 16-23: Save block 2
// 24-31: Save block 3

// Each save block sector has the following format:
// 0-3: "COOL"
// 4-7: Serial number (increment each time you save a game)
// 8-23: Reserved
// 24-27: "HERD"
// 28-31: CRC32 of bytes 0-27
// 32-223: Game data as specificied in struct gamestruct
// 224-227: "LOOC"
// 228-247: Reserved
// 248-251: "DREH"
// 252-255: CRC32 of bytes 0-251

// The read algorithm is as follows.  Read in the first page, check for the two strings.  If the
// data is not intact, the block is unitialized, which is bad.  Proceed to 'initialize save block'
// section.  If you find them, check the CRC.  If it matches, look at the serial number.  If it is
// 0, then the block is unused, and only good for saving.  If it is >0, then keep track of this
// sector, and continue to the next sector.  After all 8 sectors have been consulted, the one with
// the highest number is the one to load.  If all are 0, then the block is 'empty'.  If the CRC fails,
// proceed to 'initialize save block'.  To read the block, load in the rest of the 224 bytes, check
// the trailing strings and CRC.  If they fail, proceed to 'initialize save block'.  Otherwise, copy
// the game data to gamestruct and start the game.

// "INITIALIZE SAVE BLOCK"
// To initialize a save block, consider how we got here.  If we are doing this because a corrupted
// CRC was located, or a magic string was not found where expected, then a message should be displayed
// to the user to the effect of 'save block 3 is corrupt and was deleted'.  There is, however, a special
// case.  If the system save block has NO uninitialized sectors, then the game should be assumed to
// be 'new', and all blocks are initialized with no error message to the user.  To do this, simply write
// the first 32 bytes of each sector, with the serial number set to 0.  As this will take some time
// the best point to do this is at the first game save.  It will take longer than normal, but not by
// much.

// There is a bit of paranoia here, as well as an ease of use situation.  While we could attempt to
// try and recover the last good game save....users do NOT want to be  running fsck on their DS.


#include <string.h>
#include "vmu_logo.h"
#include "kosemulation.h"
#include "sprite.h"
#include "menu.h"
#include "sound.h"
#include "menuscr_manager.h"

/* in cool.c - don't want to include all of cool.h here */
void debug(char *str, ...);
extern unsigned int myJiffies;
extern int nFrames;
extern int gDontCheckLid;
extern void SortSheepAndTitle();
extern char *szLoadText;

/* sound.h */
void set_sound_volume(int nMusic, int nSound);

#define SAVE_BLOCK_SIZE 256

struct gamesavesector 
{
	u32 uCOOLMagic;
	u32 uSerialNumber;
	char szPercentage[4];
	u32 uReserved[3];
	u32 uHERDMagic;
	u32 uMiniCRC;
	u32 uLOOCMagic;
	u32 uGameData[48];
	u32 uReserved2[5];
	u32 uDREHMagic;
	u32 uFullCRC;
};

// local thread helpers
static struct gamestruct LastSavedGame={ 0 };

static u16 nLockID;
int nBlockStatus[4];						// 0 is autosave, 1,2,3 are the savefiles
int nSerialNumber[4];						// serial numbers for same
static u8 SaveBlock[SAVE_BLOCK_SIZE];		// just some work memory for the current file
static u8 lastSave[SAVE_BLOCK_SIZE];

// return the address offset in EEPROM based on the parameters in, SAVE_BLOCK_SIZE byte blocks
inline u32 calcEEAddress(int nBlock, int nSector) {
	return (nBlock*8 + nSector)*SAVE_BLOCK_SIZE;
}

// notify the user of something
void notifyUser(char *str) {
	int nOption=0;

	MenuScr_InitMenu(MENU_STORY_TEXT);
	MenuScr_UpdateMenuString(0, str);
	while (!MenuScr_DrawFrame(&nOption)) {
		pvr_scene_finish_2d();
	}
	MenuScr_CloseMenu(1);
	while (MenuScr_DrawMenuItems()) {
		pvr_scene_finish_2d();
	}
}

static void dumpsavegame() {
	int idx;

	debug("Magic: 0x%08x\n", gGame.nMagic);
	debug("Timer: %d\n", gGame.Options.Timer);
	debug("Rounds: %d\n", gGame.Options.Rounds);
	debug("Powers: %s\n", gGame.Options.Powers?"On":"Off");
	debug("Sound Vol: %d\n", gGame.SVol);
	debug("Music Vol: %d\n", gGame.MVol);
	debug("CPU Skill: %d\n", gGame.Options.Skill);
	debug("Sheep Speed: %d\n", gGame.Options.SheepSpeed);
	debug("Autosave: %s\n", gGame.AutoSave?"On":"Off");
	debug("Continue: %d\n", gGame.continuelevel);
	debug("High Scores:\n");
	for (idx=0; idx<10; idx++) {
		debug("%s - %d\n", gGame.HighName[idx], gGame.HighScore[idx]);
	}
	for (idx=0; idx<5; idx++) {
		debug("Challenge %d - %s - %d\n", idx, gGame.ChallengeName[idx], (int)gGame.ChallengeScore[idx]);
	}
}

/*
   C implementation of CRC-32 checksums for NAACCR records.  Code is based
   upon and utilizes algorithm published by Ross Williams.

   This file contains:
      CRC lookup table
      function CalcCRC32 for calculating CRC-32 checksum
      function AssignCRC32 for assigning CRC-32 in NAACCR record
      function CheckCRC32 for checking CRC-32 in NAACCR record

   Provided by:
      Eric Durbin
      Kentucky Cancer Registry
      University of Kentucky
      October 14, 1998

   Status:
      Public Domain
*/

#define CRC32_XINIT 0xFFFFFFFFL		/* initial value */
#define CRC32_XOROT 0xFFFFFFFFL		/* final xor value */

/*****************************************************************/
/*                                                               */
/* CRC LOOKUP TABLE                                              */
/* ================                                              */
/* The following CRC lookup table was generated automagically    */
/* by the Rocksoft^tm Model CRC Algorithm Table Generation       */
/* Program V1.0 using the following model parameters:            */
/*                                                               */
/*    Width   : 4 bytes.                                         */
/*    Poly    : 0x04C11DB7L                                      */
/*    Reverse : TRUE.                                            */
/*                                                               */
/* For more information on the Rocksoft^tm Model CRC Algorithm,  */
/* see the document titled "A Painless Guide to CRC Error        */
/* Detection Algorithms" by Ross Williams                        */
/* (ross@guest.adelaide.edu.au.). This document is likely to be  */
/* in the FTP archive "ftp.adelaide.edu.au/pub/rocksoft".        */
/*                                                               */
/*****************************************************************/

unsigned long  crctable[256] =
{
 0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL,
 0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
 0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
 0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
 0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
 0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
 0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL,
 0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
 0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
 0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
 0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L,
 0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
 0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L,
 0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
 0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
 0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,
 0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL,
 0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
 0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L,
 0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
 0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
 0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
 0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL,
 0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
 0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
 0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
 0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
 0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
 0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L,
 0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
 0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L,
 0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,
 0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
 0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
 0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
 0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
 0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL,
 0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
 0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
 0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
 0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L,
 0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
 0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L,
 0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
 0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
 0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
 0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L,
 0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
 0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL,
 0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
 0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
 0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
 0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL,
 0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
 0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL,
 0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
 0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
 0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
 0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L,
 0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
 0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L,
 0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
 0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
 0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
};

/*****************************************************************/
/*                   End of CRC Lookup Table                     */
/*****************************************************************/

/* Calculate CRC-32 Checksum for NAACCR Record,
   skipping area of record containing checksum field.

   Uses reflected table driven method documented by Ross Williams.

   PARAMETERS:
     unsigned char *p           NAACCR Record Buffer
     unsigned long reclen       NAACCR Record Length
     unsigned long checksumpos  Position of CHECKSUM (as in Data Dictionary)
     unsigned long checksumlen  Length of checksum Field

   RETURNS:
	 checksum value

   Author:
     Eric Durbin 1998-10-14

   Status:
     Public Domain
*/
unsigned long CalcCRC32(unsigned char *p, unsigned long checksumpos, unsigned long checksumlen)
{
	unsigned long j;

	/* initialize value */
	unsigned long crc = CRC32_XINIT;

	/* process each byte prior to checksum field */
	for (j = 1; j < checksumpos; j++) {
		crc = crctable[(crc ^ *p++) & 0xFFL] ^ (crc >> 8);
	}

	/* skip checksum position */
	j += checksumlen;
	p += checksumlen;

	/* return XOR out value */
	return crc ^ CRC32_XOROT;
}

BOOL saveInitializeBlock(int nBlock, int nSector) 
{
	BOOL bReturn;
	struct gamesavesector *pTempSector = (struct gamesavesector *)SaveBlock;
	
	memset(pTempSector, 0, SAVE_BLOCK_SIZE);
	CARD_LockBackup(nLockID);
	pTempSector->uCOOLMagic = 0x434F4F4C;
	pTempSector->uHERDMagic = 0x48455244;
	pTempSector->uSerialNumber = 0;			// not actually a game
	pTempSector->uMiniCRC = CalcCRC32(SaveBlock, 28, 4);
	debug("Calculated CRC was 0x%08X\n", pTempSector->uMiniCRC);
	bReturn = CARD_WriteAndVerifyEeprom(calcEEAddress(nBlock, nSector), pTempSector, SAVE_BLOCK_SIZE);
	CARD_UnlockBackup(nLockID);
	return bReturn;
}

BOOL saveFormatMemory(void) 
{
	debug("Starting format of memory\n");
	int nBlock, nSector;
	
	for (nBlock = 0; nBlock < 4; nBlock++) {
		for (nSector = 0; nSector < 8; nSector++) {
			if (!saveInitializeBlock(nBlock, nSector)) {
				debug("Memory failed to format %d, %d\n", nBlock, nSector);
				return FALSE;
			}
		}
	}
	debug("Memory formatted\n");
	nBlockStatus[0]=0;
	nBlockStatus[1]=0;
	nBlockStatus[2]=0;
	nBlockStatus[3]=0;
	nSerialNumber[0]=0;
	nSerialNumber[1]=0;
	nSerialNumber[2]=0;
	nSerialNumber[3]=0;
	return TRUE;
}

// Identifies the state of the block header
// 1-MAXINT = save serial number
// 0 = empty block
// -1 = uninitialized block
// -2 = corrupted block
int saveIdentifyBlockHeader(int nBlock, int nSector) {
	struct gamesavesector *pTempSector = (struct gamesavesector *)SaveBlock;
	
	CARD_LockBackup(nLockID);
	if (CARD_ReadEeprom(calcEEAddress(nBlock, nSector), pTempSector, 256)) 
	{
		int nNumMagics = 0;
		if (0x434F4F4C == pTempSector->uCOOLMagic) 
		{
			nNumMagics++;
		}
		if (0x48455244 == pTempSector->uHERDMagic) 
		{
			nNumMagics++;
		}
		if (0 == nNumMagics) 
		{
			CARD_UnlockBackup(nLockID);
			return -1; 
		}
		if (1 == nNumMagics)
		{
			debug("Could only retreive one magic, bad memory\n");
			CARD_UnlockBackup(nLockID);
			return -2; 
		}
		if (pTempSector->uMiniCRC != CalcCRC32(SaveBlock, 28, 4)) 
		{
			debug("CRC was 0x%08X vs 0x%08X in memory, bad memory\n", CalcCRC32(SaveBlock, 28, 4), pTempSector->uMiniCRC);
			CARD_UnlockBackup(nLockID);
			return -2; 
		}
	} else {
		CARDResult n = CARD_GetResultCode();
		debug("Failed ReadEeprom with result code %d\n", n);
		CARD_UnlockBackup(nLockID);
		return -2; 
	}
	CARD_UnlockBackup(nLockID);
	return pTempSector->uSerialNumber;
}

void saveUpdateSaveBlocks(void) 
{
	u8 nBlock;
	u8 nSector;
	u8 bHasUninitialized = 0;
	
	// nBlockStatus[0] is just for autosave
	for (nBlock = 0; nBlock < 4; nBlock++) 
	{
		int nSerial = -1;
		nBlockStatus[nBlock] = -1;
		bHasUninitialized = 0;
		for (nSector = 0; nSector < 8; nSector++) 
		{
			int nTempStatus = saveIdentifyBlockHeader(nBlock, nSector);
			
			debug("Block %d, sector %d, status = %d\n", nBlock, nSector, nTempStatus);
			
			if (-2 == nTempStatus) 
			{
				// In this case, a sector is verifiably corrupt.  We're done here.
				nBlockStatus[nBlock] = -2;
				break;
			}
			
			if (-1 == nTempStatus) 
			{
				// The block doesn't seem to contain data yet...
				bHasUninitialized = 1;
				continue;
			}
			
			if (nTempStatus > nSerial) 
			{
				// If the sector is empty, don't record it by number.  We
				// want to know that the block is empty if they're all that way
				nBlockStatus[nBlock] = (nTempStatus != 0) ? nSector+1 : 0;
				nSerialNumber[nBlock] = nTempStatus;
				nSerial = nTempStatus;
			}
		}
		if (8 == nSector) 
		{
			// We made it to the end...did we find any empty blocks
			if (bHasUninitialized) 
			{
				// There are some...now...
				if (-1 != nBlockStatus[nBlock])
				{
					// Unfortunately, some were initialized...that case is an error
					nBlockStatus[nBlock] = -2;
				}
			}
		}
		debug("Save block %d has status %d\n", nBlock, nBlockStatus[nBlock]);
	}
}

BOOL saveLoadBlock(u8 nBlock) 
{
	u8 nSector;
	
	if (nBlockStatus[nBlock] <= 0) 
	{
		debug("Block status of %d indicates not ready to load\n", nBlockStatus[nBlock]);
		return FALSE;
	}
	// Sectors are 1 based, but memory is zero based
	nSector = nBlockStatus[nBlock] - 1;

	struct gamesavesector *pTempSector = (struct gamesavesector *)SaveBlock;

	CARD_LockBackup(nLockID);
	if (CARD_ReadEeprom(calcEEAddress(nBlock, nSector), pTempSector, 256)) 
	{
		if (0x4C4F4F43 != pTempSector->uLOOCMagic) 
		{
			debug("LOOC magic not present\n");
			CARD_UnlockBackup(nLockID);
			return FALSE; 
		}
		if (0x44524548 != pTempSector->uDREHMagic) 
		{
			debug("DREH magic not present\n");
			CARD_UnlockBackup(nLockID);
			return FALSE; 
		}
		if (pTempSector->uFullCRC != CalcCRC32(SaveBlock, 252, 4)) 
		{
			debug("CRC was 0x%08X vs 0x%08X in memory, bad memory\n", CalcCRC32(SaveBlock, 252, 4), pTempSector->uFullCRC);
			CARD_UnlockBackup(nLockID);
			return FALSE; 
		}
		
		CARD_UnlockBackup(nLockID);		
		return TRUE;
	}
	
	CARDResult n = CARD_GetResultCode();
	debug("Failed ReadEeprom with result code %d\n", n);
	CARD_UnlockBackup(nLockID);
	
	return FALSE;
}

BOOL saveSaveBlock(u8 nBlock) 
{
	u8 nSector;
	
	if (nBlockStatus[nBlock] < 0) 
	{
		debug("Block status of %d indicates not ready to save, initialize first\n", nBlockStatus[nBlock]);
		return FALSE;
	}
	
	if (0 == nBlockStatus[nBlock]) 
	{
		// In this case, we want to start with sector 1
		nBlockStatus[nBlock] = 1;
		nSerialNumber[nBlock] = 0;
	}
	else 
	{
		nBlockStatus[nBlock] = (nBlockStatus[nBlock] + 1) % 8 + 1;
	}
	
	nSector = nBlockStatus[nBlock] - 1;

	struct gamesavesector *pTempSector = (struct gamesavesector *)SaveBlock;
	pTempSector->uCOOLMagic = 0x434F4F4C;
	pTempSector->uHERDMagic = 0x48455244;
	pTempSector->uLOOCMagic = 0x4C4F4F43;
	pTempSector->uDREHMagic = 0x44524548;
	pTempSector->uSerialNumber = ++nSerialNumber[nBlock];
	pTempSector->uMiniCRC = CalcCRC32(SaveBlock, 28, 4);
	pTempSector->uFullCRC = CalcCRC32(SaveBlock, 252, 4);
	
	if (0 == memcmp(lastSave, SaveBlock, SAVE_BLOCK_SIZE)) {
		debug("No changes to save file, skipping write...\n");
		return TRUE;
	}
	
	CARD_LockBackup(nLockID);
	if (CARD_WriteAndVerifyEeprom(calcEEAddress(nBlock, nSector), pTempSector, 256)) 
	{
		memcpy(lastSave, SaveBlock, SAVE_BLOCK_SIZE);
		CARD_UnlockBackup(nLockID);
		return TRUE;
	} else {
		CARDResult n = CARD_GetResultCode();
		debug("Failed Write with result code %d\n", n);
		
		memset(lastSave, 0, SAVE_BLOCK_SIZE);
		nBlockStatus[nBlock] = 0;	// save failed
		CARD_UnlockBackup(nLockID);
		return FALSE;
	}
}

void vmuInit() {
	// Get a lock ID to use for all future backup operations
	debug("Attempting to get a lock for game save operations\n");
	s32 nTempLockID = OS_GetLockID();	
	
	if (nTempLockID < 0) 
	{
		debug("Can't get a lock for backup memory.\n");
		return;
	} 

	nLockID = (u16)nTempLockID;
	debug("Lock Id is %d\n", nLockID);
	
	CARD_LockBackup(nLockID);
	if (FALSE == CARD_IdentifyBackup(CARD_BACKUP_TYPE_EEPROM_64KBITS)) 
	{
		CARDResult n = CARD_GetResultCode();
		debug("Failed with result code %d\n", n);
		
		CARD_UnlockBackup(nLockID);
		debug("Something went wrong with the backup memory.\n");
		OS_ReleaseLockID(nLockID);
		nLockID = 0;
		return;
	}
	CARD_UnlockBackup(nLockID);
	
	saveUpdateSaveBlocks();	
	if ((nBlockStatus[1] < 0) || (nBlockStatus[2] < 0) || (nBlockStatus[3] < 0)) {
		// please reformat
		nBlockStatus[0] = -1;
		debug("Save memory needs formatting...\n");
	} else {
		// all good
		nBlockStatus[0] = 0;
	}
}

// Load the save game from the VMU
void doVMULoad(int nBlockNumber) {
	uint8 *pBuf=NULL;
	int bufsize=-1;

	debug("Loading: Block %d\n", nBlockNumber);
	
	szLoadText = "Load";

	if (0 == nBlockNumber) 
	{
		MenuScr_Init();
		switch (doMenu(MENU_LOAD,NULL)) 
		{
			case MENU_LOAD_A:
				nBlockNumber = 1;
				break;
			case MENU_LOAD_B:
				nBlockNumber = 2;
				break;
			case MENU_LOAD_C:
				nBlockNumber = 3;
				break;
				
			default:
			return;
		}
	}
	
	sound_stop();
	
	if (!saveLoadBlock(nBlockNumber)) 
	{
		memset(lastSave, 0, SAVE_BLOCK_SIZE);
		notifyUser("Load Failed\nPress A");
		return;
	} else {
		struct gamesavesector *pTempSector = (struct gamesavesector *)SaveBlock;
		memcpy(&gGame, pTempSector->uGameData, sizeof(gGame));
		gGame.SaveSlot = nBlockNumber;
		
		// slot shouldn't have changed....
		memcpy(lastSave, SaveBlock, SAVE_BLOCK_SIZE);
		
		dumpsavegame();
		
		notifyUser("Load Successful\nPress A");
	}
	
}

// Save the save game to the VMU
// returns 1 if we paused to ask the user anything
int doVMUSave(int isAuto, int nBlockNumber) {
	int retval = 0;
	
	if (nBlockStatus[0] == -1) {
		if (!isAuto) {
			sound_stop();
			saveFormatMemory();
			notifyUser("Save formatted");
		} else {
			// don't auto-erase memory
			return 0;
		}
	}
	
	// The short version is that we need to put ggame into save ram.
	debug("Saving: Auto %d Block %d\n", isAuto, nBlockNumber);
	
	if (isAuto) {
		szLoadText = "AutoSave";
	} else {
		szLoadText = "Save";
	}
	
	if (0 == nBlockNumber) {
		MenuScr_Init();
		switch (doMenu(MENU_SAVE,NULL)) 
		{
			case MENU_SAVE_A:
				nBlockNumber = 1;
				break;
			case MENU_SAVE_B:
				nBlockNumber = 2;
				break;
			case MENU_SAVE_C:
				nBlockNumber = 3;
				break;
				
			default:
			return 1;
		}
		retval = 1;
	}
	
	gGame.SaveSlot = nBlockNumber;

	dumpsavegame();

	memset(SaveBlock, 0, SAVE_BLOCK_SIZE);
	struct gamesavesector *pTempSector = (struct gamesavesector *)SaveBlock;
	debug("Saving to block %d: Address %08X, bytes %d\n", nBlockNumber, pTempSector, sizeof(gGame));
	memcpy(pTempSector->uGameData, &gGame, sizeof(gGame));
	
	strncpy(pTempSector->szPercentage, "000", 4);
	pTempSector->szPercentage[3] = 0;
	
	sound_stop();

	if (!saveSaveBlock(nBlockNumber)) {
		if (!isAuto) {
			// TODO: we really should warn the user in the auto case,
			// but sometimes the screen is turned off! Data may not be loaded.
			notifyUser("Save Failed\nPress A");
		}
	} else if (!isAuto) {
		notifyUser("Save Successful\nPress A");
	}

	return retval;
}
