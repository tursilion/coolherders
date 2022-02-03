/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://tursi.yiffco.com              */
/* temp.c (vmu util)                    */
/****************************************/

#include <stdio.h>
#include "vmu_icon.h"

void main() {
  int idx, idx2;
  int out;
  char *ptr;
  FILE *f;

  ptr=vmu_icon+48*32;
  f=fopen("vmu_logo.inc", "w");

  // it's upside down?

  for (idx=31; idx>=0; idx--) {
    for (idx2=0; idx2<48; idx2+=8) {
      out=(*(ptr--)=='.' ? 1<<7 : 0);
      out=out|(*(ptr--)=='.' ? 1<<6 : 0);
      out=out|(*(ptr--)=='.' ? 1<<5 : 0);
      out=out|(*(ptr--)=='.' ? 1<<4 : 0);
      out=out|(*(ptr--)=='.' ? 1<<3 : 0);
      out=out|(*(ptr--)=='.' ? 1<<2 : 0);
      out=out|(*(ptr--)=='.' ? 1<<1 : 0);
      out=out|(*(ptr--)=='.' ? 1 : 0);
      fprintf(f, "0x%02x, ", out);
    }
    fprintf(f, "\n");
  }
  fclose(f);
}
