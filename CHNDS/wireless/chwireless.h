/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2011 HarmlessLion LLC      */
/* chwireless.h                         */
/****************************************/

// defines for the menu communication protocol
enum {
	NET_CMD_CTRL,
	NET_CMD_SEED,
	NET_CMD_NAME,		// name starting at 0
	NET_CMD_NAME2,		// name starting at 5
	NET_CMD_WORLD,
	NET_CMD_CFG,
	NET_CMD_CHAR,
	NET_CMD_READY,
	NET_CMD_NOT_READY,
	NET_CMD_HOST_QUIT,	// host is leaving, children should give up
	NET_CMD_GAME_READY,	// host or client is ready for game, seed received
};

extern unsigned int gRxBitmap;
extern u16 gNetworkControl[4];
extern u16 gNetworkControlFrame[4];
extern u16 gLocalFrame;
extern int gLostFrames;

void InitWireless();
int InitializeParent();
int InitializeChild();
int EndWireless();
int ScanForHostGames();
void AddOrUpdateGame(unsigned char mac[6], char *pszName);
int EndScanning();
int WirelessFrame();
void SetSharedData(int cmd, char *pData);
int HandleWirelessGame(int calledback);
void ShowHostOnlyNotice();
int SyncNetwork();
int CheckNetworkSync();
void HandleNetworkStuff();
int ProcessCommand(int idx);

int WirelessWaitHostMachineReady();		// host sends NET_CMD_GAME_READY
int WirelessWaitAllMachinesReady();		// ALL send NET_CMD_READY

int WaitForNotBusy();
int WaitForStop();
int WaitForDataSharing();

void QueueCtrl(int btns, int nFrame);
void QueueSeed(unsigned int seed);
void QueueName(char *pszName);
void QueueName2(char *pszName);
void QueueWorld(int level, int stage);
void QueueCfg(int powers, int specials, int cpu, int sheep, int skill, int timer);
void QueueChar(int who, int color);
void QueueReady();
void QueueNotReady();
void QueueGameReady();

