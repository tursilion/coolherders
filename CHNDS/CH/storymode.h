/****************************************/
/* Cool Herders                         */
/* http://harmlesslion.com              */
/* Copyright 2009 HarmlessLion LLC      */
/* storymode.h                          */
/****************************************/

extern int StoryModeTotalScore;

void initStory();
char *doStory(int *nPhase, char cLastCmd);
int FindPhase(int nLevel);
