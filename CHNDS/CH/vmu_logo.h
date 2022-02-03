/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* vmu_logo.h                           */
/****************************************/

void vmuInit();
void doVMULoad(int nBlockNumber);
int doVMUSave(int isAuto, int nBlockNumber);
void saveUpdateSaveBlocks(void);
BOOL saveFormatMemory(void);

// extern for the menu system
extern int nBlockStatus[4];
