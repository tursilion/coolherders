/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://harmlesslion.com              */
/* Copyright 2000-2005 Brendan Wiese &  */
/* Mike Brent.                          */
/* control.h                            */
/****************************************/

extern maple_device_t *ControllerState[4];

int isStartPressed();

void waitPVRDone();		// not control based, but as good a place as any
