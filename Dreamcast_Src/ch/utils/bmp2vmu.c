/****************************************/
/* Cool Herders                         */
/* A demented muse by Brendan Wiese     */
/* http://starbow.org                   */
/* Programmed by Mike Brent             */
/* http://tursi.yiffco.com              */
/* bmp2vmu.c (dos util)                 */
/****************************************/

// this code crashes after writing the file in win98
// I haven't time to work out why right now - the file it
// writes *is* correct and *is* complete.
// This note is just warning to me later after I forget

#include <stdio.h>
#include <allegro.h>

void main(int argc, char *argv[]) {
 FILE *fp;
 BITMAP *bmp;
 RGB pal;
 int x, y;
 unsigned int out;

 allegro_init();
 set_color_depth(8);
 set_gfx_mode(GFX_VGA, 320, 200, 320, 200);
 bmp=load_bitmap(argv[1], &pal);
 if (NULL != bmp) {
   blit(bmp, screen, 0, 0, 0, 0, 48, 32);
   fp=fopen("vmu.inc", "w");
   for (y=31; y>=0; y--) {
     for (x=47; x>=0; x-=8) {
       out=(getpixel(screen, x, y)?1:0)<<7;
       out=out|((getpixel(screen,x-1,y)?1:0)<<6);
       out=out|((getpixel(screen,x-2,y)?1:0)<<5);
       out=out|((getpixel(screen,x-3,y)?1:0)<<4);
       out=out|((getpixel(screen,x-4,y)?1:0)<<3);
       out=out|((getpixel(screen,x-5,y)?1:0)<<2);
       out=out|((getpixel(screen,x-6,y)?1:0)<<1);
       out=out|(getpixel(screen,x-7,y)?1:0);
       fprintf(fp, "0x%02x, ", out);
       putpixel(screen, x, y, 255);
     }
     fprintf(fp, "\n");
   }
   fclose(fp);
   destroy_bitmap(bmp);
 } else {
   allegro_exit();
   printf("can't open file!\n");
 }
}

