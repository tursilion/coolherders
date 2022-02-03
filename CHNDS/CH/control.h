/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2011 HarmlessLion LLC      */
/* control.h                            */
/****************************************/

typedef enum {
	eContNone,
	eContLocal,
	eContNetwork
} eControlType;

extern eControlType ControllerState[4];
extern int gHumanPlayer;

// software reset!
#define RESET_KEYS (CONT_A|CONT_B|CONT_X|CONT_Y|CONT_START)

int isStartPressed();
u16 GetNetController(int nControl);
u16 GetController(int nControl);
void DisableControlsTillReleased();

void InitTouchScreen();
u32 GetTouchData(TPData *pData);
