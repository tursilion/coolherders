#include <stdio.h>
#include "s3mplay.h"

FILE *fp;
int idx;

int main() {
  fp=fopen("s3mplay.txt", "w");
  for (idx=0; idx<sizeof(s3mplay); idx+=4) {
    fprintf(fp,/* "%08x - %02x %02x %02x %02x: */"%02x%02x%02x%02x\n", 
/* idx, s3mplay[idx], s3mplay[idx+1], s3mplay[idx+2], s3mplay[idx+3], */
s3mplay[idx+3], s3mplay[idx+2], s3mplay[idx+1], s3mplay[idx]);
  }  
  fclose(fp);
  return 0;
}


