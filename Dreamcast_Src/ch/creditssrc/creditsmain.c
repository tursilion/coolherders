
/*
*  End Creditz for Tursi's Cool Herders
*  Code by Negroponte J. Rabit >;b
*  
*  Notes:
*  
*  Todos:
*  	1>	Write the fokken thing
*/

// Modified by Tursi - removed ROMdisk, integrated with memory pre-allocated by CH
// Added fades, reduced dynamic memory use, integrated with CH systems, fixed a few minor bugs

// Defines and stuff.  For tweaking, yo.

// Note: position of logo and graphic are now handled in
// SortSheepAndTitle() in disclaimer.c. logo.bmp isn't loaded
// anymore!
// Position of 'Cool Herders' logo
#define logo_xpos 20 	
#define logo_ypos 20
#define logo_xsize 320
#define logo_ysize 150

// Position of sheep graphic
#define sheep_xpos 460
#define sheep_ypos 300
#define sheep_xsize 128
#define sheep_ysize 128

// Scrolltext stuff
#define XOFFSCREEN 32	// how many pixels offscreen to render off left/right edges
#define XEND 640 + XOFFSCREEN
#define YPOS 415
#define XSIZSC 1
#define YSIZESC 1
#define YSIZE 32	

// Slide Creditz (stuff that appears in the center of the screen)
#define SLIDEZ_XPOS 192
#define SLIDEZ_XSIZE 256
#define SLIDEZ_YPOS 142
#define SLIDEZ_YSIZE 256
#define SLIDE_TIME 500		// number of frames each
#define BETWEEN_TIME 50		// number of frames between slides
#define FADE_SPEED 0.02f	// amount alpha changes per frame

// hard to say if this could ever work under my Win32 code (at least not without the DX code)
#ifdef WIN32
#include "..\kosemulation.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <math.h>
#else
#include <kos.h> 
#include <png/png.h>
#include <oggvorbis/sndoggvorbis.h>
#include <math.h>
#include <zlib/zlib.h>	
#endif
#include "../sprite.h"
#include "../cool.h"
#include "../menu.h"
#include "../font.h"

// Tursi - cache area for the credits slides (lucky 13!)
extern pvr_ptr_t txr_levela[4],txr_levelb[4],txr_levelc[4];
extern pvr_ptr_t txr_herder[4];
extern void debug(char *str, ...);
extern void set_sound_volume(int nMusic, int nSound);
extern void ShowLoading();
extern void SortSheepAndTitle();
extern int nFrames;
extern int gReturnToMenu;
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

typedef struct glyph_type {
	int xoffset;
	int yoffset;
	int xsize;
	int ysize;
	int pixelsize;
	int baseline;
	int used;
} glyph_t;

typedef struct scroller_type {
	char *buffer;			// Pointer to scrolltext buffer
	int counter;			// Simple counter for scroller, increment by 1 on each frame
	int length;				// Length of scrolltext buffer
	int	atchar;				// Current character position to start
	int atpixel;			// Current pixel position within that character	
	pvr_ptr_t glyphmap;		// Texture containing glyphs
	glyph_t glyphtable[256];		// Table of character glyphs, offsets and sizes and such
} scroller_t;

struct credz_stat_type {
	int tk;					// timekeeper, incremented at 60hz
	int tk_enabled;			// when 0, timekeeper is disabled
	int scz_speed;			// scroller speed (1 character = scz_speed*(60/pixel))
} credz_stat;

// Define them texture buffers
//pvr_ptr_t tx_logo;			// Cool Herders logo and Sheep pic
pvr_ptr_t tx_bg;			// That zooming in and out backgroundy thing
pvr_ptr_t tx_font;			// Single bitmap that contains all the scroller characters
pvr_ptr_t tx_fg;			// Game creditz stuff in center of screen.

// Tursi - a little trick I use for fades in the draw functions
float gFade=0.0f;
int fBGActive=1;
float gBGSpeed=0.0001f;	// slower default for GOAT :)

// **** Presentation-related stuff

enum credz_event {
	credz_event_null,
		credz_event_debugmsg,
		credz_event_scrollerstart,
		credz_event_scrollerpause,
		credz_event_scrollerclear,
		credz_event_scrollersetspeed,
		credz_event_bgseteffect,
		credz_event_bgstart,
		credz_event_bgstop,
		credz_event_fgloadpng,
		credz_event_fgusecurve,
		credz_event_fgusetrans,
		credz_event_fgsettransspeed,
		credz_event_fgstarttrans
};


#define SECS(x)  (x*60)
#define MINS(x)	 (x*360)

// Sequence Stuff
struct credz_counter {
	int counter;
} credz;

int credz_ptr;

int credz_seq[] = {
	0,credz_event_debugmsg,
		100,credz_event_debugmsg,
};

#define NUM_SLIDES 12

char *credz_slide_pics[NUM_SLIDES] = { 
		"gfx/Credits/cBinky.png",
		"gfx/Credits/cTursi.png",
		"gfx/Credits/cFel.png",
		"gfx/Credits/cFoxx.png",
		"gfx/Credits/cMaurizio.png",
		"gfx/Credits/cSylvr.png",
		"gfx/Credits/cTaylor.png",
		"gfx/Credits/cAdam.png",
		"gfx/Credits/cTenzu.png",
		"gfx/Credits/cNetolu.png",
		"gfx/Credits/cRabit.png",
		"gfx/Credits/cThanks.png",
};

enum credz_slidestate {
	credz_slidestate_nothing,
		credz_slidestate_fadingout,
		credz_slidestate_fadingin,
		credz_slidestate_showing
};

typedef struct credz_slide_type {
	int state;
	int current;
	int loaded;
	int shown;
	int timer;
	int xpos;
	int ypos;
	int xsize;
	int ysize;
	float alpha;
	pvr_ptr_t slide[NUM_SLIDES];
} credz_slide_t;
// 112 bytes isn't worth the malloc overhead
credz_slide_t mySlides;

char *credz_text;

typedef struct credz_bgstate_type {
	float x;		// X offset
	float y;		// Y offset
	float r;		// Rotation
	float z;		// Zoom
	float a;		// Alpha
} CREDZ_BGSTATE;

/* romdisk */
//extern uint8 romdisk_boot[];
//KOS_INIT_ROMDISK(romdisk_boot);

// ----  Prototypes
void credzUpdate(void);

// ----  Functions

credz_slide_t *credzSlideInit(void) {
	pvr_ptr_t memptr[NUM_SLIDES]={
		txr_levela[0], txr_levela[1], txr_levela[2], txr_levela[3],
			txr_levelb[0], txr_levelb[1], txr_levelb[2], txr_levelb[3],
			txr_levelc[0], txr_herder[0], txr_herder[1], txr_herder[2]
	};
	
	credz_slide_t *s;
	int c;
	
	//	s = (credz_slide_t *)malloc(sizeof(credz_slide_t));
	s=&mySlides;
	memset(s, 0, sizeof(credz_slide_t));
	
	for(c=0;c<NUM_SLIDES;c++) {
		s->slide[c] = memptr[c];
		if(!s->slide[c]) {
			debug("####### Error allocating memory for slide: %s\n", credz_slide_pics[c]);
		} else {
			load_png_block_mask(credz_slide_pics[c], s->slide[c], PNG_NOCACHE, PNG_FULL_ALPHA);
		}
	}
	
	s->xpos = SLIDEZ_XPOS;
	s->ypos = SLIDEZ_YPOS;
	s->xsize = SLIDEZ_XSIZE;
	s->ysize = SLIDEZ_YSIZE;
	// prepare to load the first slide
	s->current=NUM_SLIDES;
	s->timer=BETWEEN_TIME*2;	// a little longer for the first one, to match the music
	s->state = credz_slidestate_nothing;
	s->alpha=0.0f;
	
	return s;
}

void credzSlideUpdate(credz_slide_t *s) {
	switch(s->state) {
	case credz_slidestate_nothing:
		s->timer--;
		if (s->timer <= 0) {
			s->state = credz_slidestate_fadingin;
			if(s->current<NUM_SLIDES-1) {
				s->current++;
			} else {
				s->current = 0;
			}
			s->loaded = 1;
			s->shown = 0;
			s->alpha = 0.0;
		}
		break;
		
	case credz_slidestate_showing:
		s->timer--;
		if(s->timer <= 0) {
			s->state = credz_slidestate_fadingout;
		}
		//			debug("000000 CREDZ_SLIDESTATE_SHOWING\n");
		break;
		
	case credz_slidestate_fadingin:
		s->alpha+=FADE_SPEED;
		if (s->alpha >= 1.0f) {
			s->alpha=1.0f;
			s->state=credz_slidestate_showing;
			s->timer = SLIDE_TIME;
		}
		//		debug("000000 CREDZ_SLIDESTATE_FADINGIN\n");
		break;
		
	case credz_slidestate_fadingout:
		s->alpha-=FADE_SPEED;
		if (s->alpha <= 0.0f) {
			s->alpha=0.0f;
			s->state=credz_slidestate_nothing;
			s->timer = BETWEEN_TIME;
		}
		//			debug("000000 CREDZ_SLIDESTATE_FADINGOUT\n");
		break;						
	}
}

void credzSlideRender(credz_slide_t *s) {
	
	if(s->loaded) {
		
		pvr_poly_cxt_t cxt;
		pvr_poly_hdr_t hdr;
		pvr_vertex_t vert;
		
		pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, 256, 256, s->slide[s->current], PVR_FILTER_BILINEAR);
		pvr_poly_compile(&hdr, &cxt);
		pvr_prim(&hdr, sizeof(hdr));
		
		vert.argb = PVR_PACK_COLOR(s->alpha, max(1.0f-gFade, 0.0f), max(1.0f-gFade, 0.0f), max(1.0f-gFade, 0.0f));    
		vert.oargb = 0;
		vert.flags = PVR_CMD_VERTEX;
		
		vert.x = s->xpos;
		vert.y = s->ypos;
		vert.z = 2;
		vert.u = 0.0;
		vert.v = 0.0;
		pvr_prim(&vert, sizeof(vert));
		
		//debug("values %f\t%f\n", r[6]-r[3], r[2]+r[7]);
		//debug("values %f\t%f\n", st, ct);
		
		vert.x = s->xpos + s->xsize;
		vert.y = s->ypos;	
		vert.z = 2;
		vert.u = 1.0;
		vert.v = 0.0;
		pvr_prim(&vert, sizeof(vert));
		
		vert.x = s->xpos;
		vert.y = s->ypos + s->ysize;
		vert.z = 2;
		vert.u = 0.0;
		vert.v = 1.0;
		pvr_prim(&vert, sizeof(vert));
		
		vert.x = s->xpos + s->xsize;
		vert.y = s->ypos + s->ysize;
		vert.z = 2;
		vert.u = 1.0;
		vert.v = 1.0;
		vert.flags = PVR_CMD_VERTEX_EOL;
		pvr_prim(&vert, sizeof(vert));    	
		
		//debug("%d\y%d\y%d\y%d\n", s->xpos, s->xsize, s->ypos, s->ysize);
		
	}
    
}	

void credzSlideUnload(credz_slide_t *s) {
	// Tursi - it's all static now
	//	int c;
	//	for(c=0;c<NUM_SLIDES;c++) {
	//	    pvr_mem_free(s->slide[c]);
	//	}
	//	free(s);
}

// Allocate and return a structure for scroller data
scroller_t *credzScrollerInit(void) {
	scroller_t *scroller;
	file_t file;
	
	scroller = (scroller_t *)malloc(sizeof(scroller_t));
	memset(scroller,0,sizeof(scroller_t));
	
	file = fs_open("gfx/Credits/scroller.txt", O_RDONLY);
	
	if(file) {
		int nLen;
		nLen=fs_total(file);
		debug("Allocating %d (+240) chars for scroller text\n", nLen);
		credz_text = (char *)malloc(nLen+240);
		memset(credz_text, ' ', nLen+240);
		fs_read(file, credz_text+120, nLen);
		fs_close(file);
		// now nul terminate
		credz_text[nLen+239]='\0';
	} else {
		credz_text = (char *)malloc(1000);
		strcpy(credz_text, "There was an error in reading the scroller text...");
	}
	
	scroller->buffer = credz_text;
	scroller->length = strlen(credz_text);
	
	return scroller;
}

// Shift scroller by one pixel - 
//		Speed can be controlled by how many times this function is called (or not called)
void credzScrollerShiftLeft(scroller_t *s) {
	
	s->atpixel++;
	
	if(s->atpixel>s->glyphtable[(int)s->buffer[s->atchar]].xsize) {
		s->atpixel = 0;
		s->atchar++;
		if(s->atchar>s->length) {
			s->atchar = 0;		// don't need such a long intro of blanks at wrap
		}
	}		
}	



// Render scroller
void credzScrollerRender(scroller_t *s)
{
	
	int c_cursor, p_cursor, done, c;
	glyph_t	*g;
	
	float tx1, tx2, ty1, ty2;
	
	int	x1, y1, x2, y2;
	
	pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;
    
    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, 256, 256, tx_font, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));
	
    vert.argb = PVR_PACK_COLOR(1.0f, max(1.0f-gFade, 0.0f), max(1.0f-gFade, 0.0f), max(1.0f-gFade, 0.0f));    
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;
    
    c_cursor = done = 0;
    
    // "cursor" that sweeps across screen on redraw, 
    //		starts at negative since PVR is good with clipping and I'm lazy
    p_cursor = -XOFFSCREEN;
    
    while(!done) {
		g = &(s->glyphtable[(int)s->buffer[s->atchar+c_cursor]]);
		c = (int)s->buffer[s->atchar+c_cursor];
		// just in case, make sure we never pass the NUL
		if ('\0' == c) {
			break;
		}
		
		if(g->used) {
			
			tx1 = (g->xoffset)/256.0;
			tx2 = (g->xoffset+g->xsize)/256.0;
			ty1 = (g->yoffset)/256.0;
			ty2 = (g->yoffset+g->ysize)/256.0;
			
			x1 = p_cursor - s->atpixel;
			y1 = YPOS;
			
			x2 = p_cursor - s->atpixel + g->xsize;
			y2 = YPOS + YSIZE;
			
			// Font Fixups
			
			// Dash - make more slender and sexier
			if(c==45) {
				y1 = y1 + 8;
				y2 = y2 - 8;
			}
			
			// Raise the " and ' a bit	    	
			if(c==39 || c==34) {
				y1 = y1 - 15;
				y2 = y2 - 15;
			}		   
			
			// Lower them , and .
			if(c==44 || c==46) {
				y1 = y1 + 12;
				y2 = y2 + 12;
			} 	
			
			
			vert.argb = PVR_PACK_COLOR(1.0f, max(1.0f-gFade, 0.0f), max(1.0f-gFade, 0.0f), max(1.0f-gFade, 0.0f));    
			vert.oargb = 0;
			vert.flags = PVR_CMD_VERTEX;
			
			vert.x = x1;
			vert.y = y1;
			vert.z = 1;
			vert.u = tx1;
			vert.v = ty1;
			pvr_prim(&vert, sizeof(vert));
			
			vert.x = x2;
			vert.y = y1;	
			vert.z = 1;
			vert.u = tx2;
			vert.v = ty1;
			pvr_prim(&vert, sizeof(vert));
			
			vert.x = x1;
			vert.y = y2;
			vert.z = 1;
			vert.u = tx1;
			vert.v = ty2;
			pvr_prim(&vert, sizeof(vert));
			
			vert.x = x2;
			vert.y = y2;
			vert.z = 1;
			vert.u = tx2;
			vert.v = ty2;
			vert.flags = PVR_CMD_VERTEX_EOL;
			pvr_prim(&vert, sizeof(vert));
			
		}
		
		p_cursor += g->xsize;	        	
		c_cursor++;
		
		if(p_cursor>XEND) done = 1;
		
    }
}


// Unallocate the character structure
void credzScrollerUnload(scroller_t *s) {
	free(s);
}


#define sinf(x) (float)sin((double)x)
#define cosf(x) (float)cos((double)x)

// Background effect
void credzBackgroundUpdate(CREDZ_BGSTATE *bg)
{	
	float minpoint, maxpoint, st, ct, size, r[8];
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;
    
    size = (float)sqrt((double)bg->z);
    st=sinf(bg->r);
    ct=cosf(bg->r);
	
	//    minpoint=.5-bg->z;
	//    maxpoint=.5+bg->z;
	minpoint = 0.5f-(bg->z*0.5f);
    maxpoint = 0.5f+(bg->z*0.5f);
    
	//    minpoint = 0;
	//    maxpoint = 1;
    
    r[0]=st*(maxpoint+bg->x);
    r[1]=st*(maxpoint+bg->y);
    r[2]=st*(minpoint+bg->x);
    r[3]=st*(minpoint+bg->y);
    r[4]=ct*(maxpoint+bg->x);
    r[5]=ct*(maxpoint+bg->y);
    r[6]=ct*(minpoint+bg->x);
    r[7]=ct*(minpoint+bg->y);
	
	
	/* draw background */ 	
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, 256, 256, tx_bg, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));
	
    vert.argb = PVR_PACK_COLOR(1.0f, max(0.5f-gFade, 0.0f), max(1.0f-gFade, 0.0f), max(0.5f-gFade, 0.0f));    
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;
    
    vert.x = 1;
    vert.y = 1;
    vert.z = 1;
    vert.u = r[6]-r[3];
    vert.v = r[2]+r[7];
    pvr_prim(&vert, sizeof(vert));
    
    //debug("values %f\t%f\n", r[6]-r[3], r[2]+r[7]);
    //debug("values %f\t%f\n", st, ct);
    
    vert.x = 640;
    vert.y = 1;	
    vert.z = 1;
    vert.u = r[4]-r[3];
    vert.v = r[0]+r[7];
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = 1;
    vert.y = 480;
    vert.z = 1;
    vert.u = r[6]-r[1];
    vert.v = r[2]+r[5];
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = 640;
    vert.y = 480;
    vert.z = 1;
    vert.u = r[4]-r[1];
    vert.v = r[0]+r[5];
    vert.flags = PVR_CMD_VERTEX_EOL;
    pvr_prim(&vert, sizeof(vert));
}

#if 0
void credzLogoRender()
{	
	
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;
    
    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, 256, 256, tx_logo, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));
	
    vert.argb = PVR_PACK_COLOR(1.0f, max(1.0f-gFade, 0.0f), max(1.0f-gFade, 0.0f), max(1.0f-gFade, 0.0f));    
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;
    
    vert.x = logo_xpos;
    vert.y = logo_ypos;
    vert.z = 5;
    vert.u = 0.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));
    
    //debug("values %f\t%f\n", r[6]-r[3], r[2]+r[7]);
    //debug("values %f\t%f\n", st, ct);
	
    vert.x = logo_xpos + logo_xsize;
    vert.y = logo_ypos;	
    vert.z = 5;
    vert.u = 1.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = logo_xpos;
    vert.y = logo_ypos + logo_ysize;
    vert.z = 5;
    vert.u = 0.0;
    vert.v = 0.45f;
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = logo_xpos + logo_xsize;
    vert.y = logo_ypos + logo_ysize;
    vert.z = 5;
    vert.u = 1.0;
    vert.v = 0.45f;
    vert.flags = PVR_CMD_VERTEX_EOL;
    pvr_prim(&vert, sizeof(vert));    	
    
    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, 256, 256, tx_logo, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));
	
    vert.argb = PVR_PACK_COLOR(1.0f, max(1.0f-gFade, 0.0f), max(1.0f-gFade, 0.0f), max(1.0f-gFade, 0.0f));    
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;
	
    vert.x = sheep_xpos;
    vert.y = sheep_ypos;
    vert.z = 4;
    vert.u = 0.5;
    vert.v = 0.45f;
    pvr_prim(&vert, sizeof(vert));
    
    //debug("values %f\t%f\n", r[6]-r[3], r[2]+r[7]);
    //debug("values %f\t%f\n", st, ct);
    
    vert.x = sheep_xpos + sheep_xsize;
    vert.y = sheep_ypos;	
    vert.z = 4;
    vert.u = 1.0;
    vert.v = 0.45f;
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = sheep_xpos;
    vert.y = sheep_ypos + sheep_ysize;
    vert.z = 4;
    vert.u = 0.5;
    vert.v = 1.0;
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = sheep_xpos + sheep_xsize;
    vert.y = sheep_ypos + sheep_ysize;
    vert.z = 4;
    vert.u = 1.0;
    vert.v = 1.0;
    vert.flags = PVR_CMD_VERTEX_EOL;
    pvr_prim(&vert, sizeof(vert));    
	
}
#endif

// Set the x/y offsets and sizes of a character from the loaded bitmap
void credzScrollerSetupGlyph(scroller_t *s, int c, int pixelsize, int baseline, int xoffset, int yoffset, int xsize, int ysize) 
{
	if(c<256) {
		s->glyphtable[c].baseline = baseline;
		s->glyphtable[c].pixelsize = pixelsize;
		s->glyphtable[c].xoffset = xoffset;
		s->glyphtable[c].yoffset = yoffset;
		s->glyphtable[c].xsize = xsize;
		s->glyphtable[c].ysize = ysize;
		s->glyphtable[c].used = 1;
	}
}

int credzMainInit(void)
{
	int error=0; 
	
	// Tursi, again, use the memory CH already allocated
	ShowLoading();
	
	//     tx_bg = pvr_mem_malloc(256*256*4);
	tx_bg=txr_levelc[1];
	if(!tx_bg) {
        debug("####### ERROR: tx_bg");                                           
	}
	
	//     tx_logo = pvr_mem_malloc(256*256*4);
//	tx_logo=txr_levelc[2];
//	if(!tx_logo) {
//		debug("####### ERROR: tx_logo");
//	}
	
	//     tx_font = pvr_mem_malloc(256*256*4);
	tx_font = txr_levelc[3];
	if(!tx_font) {
		debug("####### ERROR: tx_font");
	}
	
	load_png_block_mask("gfx/Credits/tile.png", tx_bg, PNG_NOCACHE, PNG_NO_ALPHA);
//	load_png_block_mask("gfx/Credits/logo.png", tx_logo, PNG_NOCACHE, PNG_FULL_ALPHA);	
	load_png_block_mask("gfx/Credits/font.png", tx_font, PNG_NOCACHE, PNG_FULL_ALPHA);

    return error;
}

void credzMainCleanup(void)
{
	// Tursi - don't free these PVR buffers, I still need them :)
	//	pvr_mem_free(tx_bg);
	//	pvr_mem_free(tx_logo);
	//	pvr_mem_free(tx_font);
	//	pvr_mem_reset();
	free(credz_text);
}

void credzFinish(void)
{
}


int credzReadControllers(void)
{
	int button = 0;
	MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)
		if (NULL == st) {
			continue;
		}
		button|=st->buttons;
		// map triggers to D and Z buttons (helps for debug)
		if (st->ltrig>=127) {
			button|=CONT_D;
		}
		if (st->rtrig>=127) {
			button|=CONT_Z;
		}
	MAPLE_FOREACH_END()	

	return button;
}

// TODO: move hdr creation outside frame loop

void credzScrollerSetGlyphs(scroller_t *fontstructure) {
	//	int c;
	//	
	//	for(c=0;c<255;c++) {
	//		credzScrollerSetupGlyph(fontstructure, c, 0, 3, 220, 230, 1, 1);
	//	}
	
	credzScrollerSetupGlyph(fontstructure, 32, 0, 3, 220, 230, 8, 1);
	credzScrollerSetupGlyph(fontstructure, 33, 32, 24, 13, 100, 16, 32);
	credzScrollerSetupGlyph(fontstructure, 34, 32, 25, 183, 144, 24, 16);
	credzScrollerSetupGlyph(fontstructure, 35, 32, 23, 181, 216, 19, 23);
	credzScrollerSetupGlyph(fontstructure, 36, 32, 26, 13, 164, 17, 32);
	credzScrollerSetupGlyph(fontstructure, 37, 32, 25, 95, 29, 30, 31);
	credzScrollerSetupGlyph(fontstructure, 38, 32, 25, 169, 29, 24, 30);
	credzScrollerSetupGlyph(fontstructure, 39, 32, 25, 226, 63, 14, 16);
	credzScrollerSetupGlyph(fontstructure, 40, 32, 27, 0, 0, 13, 38);
	credzScrollerSetupGlyph(fontstructure, 41, 32, 27, 0, 38, 13, 38);
	credzScrollerSetupGlyph(fontstructure, 42, 32, 24, 201, 215, 20, 20);
	credzScrollerSetupGlyph(fontstructure, 43, 32, 19, 119, 219, 21, 19);
	credzScrollerSetupGlyph(fontstructure, 44, 32, 7, 224, 192, 14, 16);
	credzScrollerSetupGlyph(fontstructure, 45, 32, 13, 142, 101, 25, 8);
	credzScrollerSetupGlyph(fontstructure, 46, 32, 7, 221, 215, 15, 14);
	credzScrollerSetupGlyph(fontstructure, 47, 32, 28, 13, 29, 25, 36);
	credzScrollerSetupGlyph(fontstructure, 48, 32, 25, 193, 29, 26, 30);
	credzScrollerSetupGlyph(fontstructure, 49, 32, 24, 93, 180, 16, 29);
	credzScrollerSetupGlyph(fontstructure, 50, 32, 25, 94, 63, 20, 30);
	credzScrollerSetupGlyph(fontstructure, 51, 32, 25, 114, 63, 23, 30);
	credzScrollerSetupGlyph(fontstructure, 52, 32, 24, 93, 151, 25, 29);
	credzScrollerSetupGlyph(fontstructure, 53, 32, 25, 159, 63, 21, 30);
	credzScrollerSetupGlyph(fontstructure, 54, 32, 26, 13, 196, 23, 31);
	credzScrollerSetupGlyph(fontstructure, 55, 32, 24, 66, 122, 23, 29);
	credzScrollerSetupGlyph(fontstructure, 56, 32, 25, 137, 63, 22, 30);
	credzScrollerSetupGlyph(fontstructure, 57, 32, 26, 13, 132, 22, 32);
	credzScrollerSetupGlyph(fontstructure, 58, 32, 18, 213, 93, 15, 25);
	credzScrollerSetupGlyph(fontstructure, 59, 32, 18, 93, 209, 15, 27);
	credzScrollerSetupGlyph(fontstructure, 60, 32, 0, 0, 143, 4, 4);
	credzScrollerSetupGlyph(fontstructure, 61, 32, 15, 13, 227, 25, 12);
	credzScrollerSetupGlyph(fontstructure, 62, 32, 0, 4, 143, 4, 4);
	credzScrollerSetupGlyph(fontstructure, 63, 32, 25, 13, 65, 21, 35);
	credzScrollerSetupGlyph(fontstructure, 64, 32, 21, 160, 168, 21, 24);
	credzScrollerSetupGlyph(fontstructure, 65, 32, 24, 66, 93, 27, 29);
	credzScrollerSetupGlyph(fontstructure, 66, 32, 25, 66, 63, 28, 30);
	credzScrollerSetupGlyph(fontstructure, 67, 0, 1, 0, 137, 5, 6);
	credzScrollerSetupGlyph(fontstructure, 67, 32, 25, 38, 183, 26, 30);
	credzScrollerSetupGlyph(fontstructure, 68, 32, 25, 38, 153, 24, 30);
	credzScrollerSetupGlyph(fontstructure, 69, 32, 24, 203, 63, 23, 29);
	credzScrollerSetupGlyph(fontstructure, 70, 32, 24, 180, 63, 23, 29);
	credzScrollerSetupGlyph(fontstructure, 71, 32, 25, 38, 123, 28, 30);
	credzScrollerSetupGlyph(fontstructure, 72, 32, 24, 50, 0, 30, 29);
	credzScrollerSetupGlyph(fontstructure, 73, 32, 24, 162, 0, 16, 29);
	credzScrollerSetupGlyph(fontstructure, 74, 32, 24, 110, 0, 24, 29);
	credzScrollerSetupGlyph(fontstructure, 75, 32, 24, 134, 0, 28, 29);
	credzScrollerSetupGlyph(fontstructure, 76, 32, 25, 219, 29, 21, 30);
	credzScrollerSetupGlyph(fontstructure, 77, 32, 24, 178, 0, 28, 29);
	credzScrollerSetupGlyph(fontstructure, 78, 32, 24, 206, 0, 20, 29);
	credzScrollerSetupGlyph(fontstructure, 79, 32, 25, 38, 93, 27, 30);
	credzScrollerSetupGlyph(fontstructure, 80, 32, 25, 38, 63, 28, 30);
	credzScrollerSetupGlyph(fontstructure, 81, 32, 25, 38, 29, 29, 34);
	credzScrollerSetupGlyph(fontstructure, 82, 32, 25, 67, 29, 28, 32);
	credzScrollerSetupGlyph(fontstructure, 83, 32, 25, 149, 29, 20, 30);
	credzScrollerSetupGlyph(fontstructure, 84, 32, 25, 125, 29, 24, 30);
	credzScrollerSetupGlyph(fontstructure, 85, 32, 24, 66, 151, 24, 29);
	credzScrollerSetupGlyph(fontstructure, 86, 32, 24, 66, 180, 23, 29);
	credzScrollerSetupGlyph(fontstructure, 87, 32, 24, 13, 0, 37, 29);
	credzScrollerSetupGlyph(fontstructure, 88, 32, 24, 66, 209, 26, 29);
	credzScrollerSetupGlyph(fontstructure, 89, 32, 24, 93, 93, 26, 29);
	credzScrollerSetupGlyph(fontstructure, 90, 32, 24, 93, 122, 23, 29);
	credzScrollerSetupGlyph(fontstructure, 92, 32, 0, 8, 143, 4, 4);
	credzScrollerSetupGlyph(fontstructure, 94, 32, 0, 0, 147, 4, 4);
	credzScrollerSetupGlyph(fontstructure, 95, 32, -1, 142, 93, 26, 8);
	credzScrollerSetupGlyph(fontstructure, 96, 32, 29, 0, 110, 13, 9);
	credzScrollerSetupGlyph(fontstructure, 97, 0, 1, 5, 131, 5, 6);
	credzScrollerSetupGlyph(fontstructure, 97, 32, 19, 204, 119, 23, 24);
	credzScrollerSetupGlyph(fontstructure, 98, 32, 20, 140, 119, 23, 25);
	credzScrollerSetupGlyph(fontstructure, 99, 0, 1, 0, 131, 5, 6);
	credzScrollerSetupGlyph(fontstructure, 99, 32, 20, 119, 144, 21, 25);
	credzScrollerSetupGlyph(fontstructure, 100, 32, 20, 119, 119, 21, 25);
	credzScrollerSetupGlyph(fontstructure, 101, 0, 1, 5, 125, 5, 6);
	credzScrollerSetupGlyph(fontstructure, 101, 32, 19, 160, 192, 19, 24);
	credzScrollerSetupGlyph(fontstructure, 102, 32, 19, 160, 216, 19, 24);
	credzScrollerSetupGlyph(fontstructure, 103, 32, 20, 181, 119, 23, 25);
	credzScrollerSetupGlyph(fontstructure, 104, 0, 1, 0, 125, 5, 6);
	credzScrollerSetupGlyph(fontstructure, 104, 32, 19, 207, 144, 24, 24);
	credzScrollerSetupGlyph(fontstructure, 105, 32, 19, 226, 0, 14, 24);
	credzScrollerSetupGlyph(fontstructure, 106, 32, 19, 181, 192, 20, 24);
	credzScrollerSetupGlyph(fontstructure, 107, 32, 19, 204, 168, 24, 24);
	credzScrollerSetupGlyph(fontstructure, 108, 32, 20, 163, 119, 18, 25);
	credzScrollerSetupGlyph(fontstructure, 109, 32, 19, 181, 168, 23, 24);
	credzScrollerSetupGlyph(fontstructure, 110, 32, 19, 140, 216, 17, 24);
	credzScrollerSetupGlyph(fontstructure, 111, 32, 20, 191, 93, 22, 25);
	credzScrollerSetupGlyph(fontstructure, 112, 32, 20, 168, 93, 23, 25);
	credzScrollerSetupGlyph(fontstructure, 113, 32, 20, 38, 213, 24, 27);
	credzScrollerSetupGlyph(fontstructure, 114, 0, 1, 0, 119, 5, 6);
	credzScrollerSetupGlyph(fontstructure, 114, 32, 20, 119, 93, 23, 26);
	credzScrollerSetupGlyph(fontstructure, 115, 0, 1, 5, 119, 5, 6);
	credzScrollerSetupGlyph(fontstructure, 115, 32, 20, 119, 169, 18, 25);
	credzScrollerSetupGlyph(fontstructure, 116, 0, 1, 5, 137, 5, 6);
	credzScrollerSetupGlyph(fontstructure, 116, 32, 20, 119, 194, 21, 25);
	credzScrollerSetupGlyph(fontstructure, 117, 32, 19, 140, 144, 20, 24);
	credzScrollerSetupGlyph(fontstructure, 118, 32, 19, 140, 168, 19, 24);
	credzScrollerSetupGlyph(fontstructure, 119, 32, 19, 80, 0, 30, 24);
	credzScrollerSetupGlyph(fontstructure, 120, 32, 19, 160, 144, 23, 24);
	credzScrollerSetupGlyph(fontstructure, 121, 32, 19, 201, 192, 23, 23);
	credzScrollerSetupGlyph(fontstructure, 122, 32, 19, 140, 192, 19, 24);
	credzScrollerSetupGlyph(fontstructure, 124, 32, 24, 0, 76, 11, 34);
	credzScrollerSetupGlyph(fontstructure, 126, 32, 0, 4, 147, 4, 4);	
}

// Tursi
extern void flush_png_level_cache();
void credzMain(void) 
{
	int end;
	int duration;
	int c; 
	float cn;
	scroller_t *scrollerz;
	credz_slide_t *slidez;
	CREDZ_BGSTATE	bg;
	
	// Tursi: flush our RAMdisk cache - this will help ensure we
	// have enough memory to run these (though I'm thinking we don't
	// need to worry about it now. But that's okay.)
	flush_png_level_cache();
	// Tursi: not allowed to init twice! :) Make sure music isn't going
    //pvr_init_defaults();
	sndoggvorbis_stop();
	thd_sleep(100);
	// reset speed
	gBGSpeed=0.0001f;
	
   	credzMainInit();
	
	scrollerz = credzScrollerInit();
	slidez = credzSlideInit();
	credzScrollerSetGlyphs(scrollerz);
	fBGActive=1;
	
	//sndoggvorbis_init();
	// Tursi - Do loop this music
	sndoggvorbis_start("music/credits.ogg",1);
	
	credz.counter = cn = 0;
	
	c=0;
	end = 0;
	duration = 0;
	gFade = 0.0f;
	while((!(end&CONT_START))||(gFade<1.0)) {
		
		pvr_wait_ready();
		
		pvr_scene_begin();	
		
		// Do the backgrounds, 'cause they all opaque and shit
		
		pvr_list_begin(PVR_LIST_OP_POLY);		
		
		if (fBGActive) {
			c++;
			//cn=(float)c*0.0003;	// this sets the speed of movement
			cn=(float)c*gBGSpeed;

			bg.x = 2*cn*cosf(cn*3);
			bg.y = 3*cn*sinf(cn*4);
			bg.z = 1.5+sinf(cn*10);
			bg.r = 1-(cn*10);
		}
		credzBackgroundUpdate(&bg);
		
		if ((end&(CONT_DPAD_RIGHT|CONT_X)) == (CONT_DPAD_RIGHT|CONT_X)) {
			gBGSpeed+=0.00001f;
		}
		if ((end&(CONT_DPAD_LEFT|CONT_X)) == (CONT_DPAD_LEFT|CONT_X)) {
			gBGSpeed-=0.00001f;
		}

		if (end&CONT_Y) {
			fBGActive=0;
		}
		if (end&CONT_A) {
			fBGActive=1;
		}
		
		pvr_list_finish();
		
		pvr_list_begin(PVR_LIST_TR_POLY);

		if (!fBGActive) {
			DrawFontZ(1.0f, 0, sheep_xpos-32, 32, INT_PACK_COLOR(128,255,255,255), "Press A to");
			DrawFontZ(1.0f, 0, sheep_xpos-32, -1, INT_PACK_COLOR(128,255,255,255), "re-enable bg");
		}
		
		// Do the logo 
		//credzLogoRender();
		SortSheepAndTitle();
		
		credzScrollerShiftLeft(scrollerz); // double the speed,
		if (!(end&CONT_D)) {	// left trigger, slower
			credzScrollerShiftLeft(scrollerz); // double the fun!			
		}
		if (end&CONT_Z) {	// right trigger, faster
			credzScrollerShiftLeft(scrollerz); // double the speed,
			credzScrollerShiftLeft(scrollerz); // double the fun!
		}
		if ((end&(CONT_Z|CONT_D))==(CONT_Z|CONT_D)) {
			// both? Even FASTER! x4!! (1 extra to make up for the one skipped above)
			credzScrollerShiftLeft(scrollerz); // double the speed,
			credzScrollerShiftLeft(scrollerz); // double the speed,
			credzScrollerShiftLeft(scrollerz); // double the fun!
			credzScrollerShiftLeft(scrollerz); // double the speed,
			credzScrollerShiftLeft(scrollerz); // double the fun!
		} 
		
		credzScrollerRender(scrollerz);
		
		if (!(end&CONT_D)) {	// left trigger, slower
			credzSlideUpdate(slidez);
		}
		if (end&CONT_Z) {	// faster - update twice
			credzSlideUpdate(slidez);
		}
		if ((end&(CONT_Z|CONT_D))==(CONT_Z|CONT_D)) {
			// both? Even FASTER! x4!! (1 extra to make up for the one skipped above)
			credzSlideUpdate(slidez);
			credzSlideUpdate(slidez);
			credzSlideUpdate(slidez);
		}
		credzSlideRender(slidez);
		
		pvr_list_finish();  
		
		pvr_scene_end;

		if (gReturnToMenu) {
			break;
		}
		
		if (end&CONT_START) {
			gFade+=0.02f;
			if (gFade>=1.0) gFade=1.0;	// this'll end it
			set_sound_volume((int)((gGame.MVol*28)*(1.0-gFade)), -1);
		} else {
			end = credzReadControllers();
			if (end&CONT_START) {
				gFade=0.01f;
			}
		}
		credz.counter++;
	}
	// Tursi - fade out before exitting
	
	sndoggvorbis_stop();
	// reset sound volume
	set_sound_volume(gGame.MVol*28, -1);
	// Tursi - still need the ogg system too ;)
	//	sndoggvorbis_shutdown();	
	
	debug("####### Scroller Position %d:%d\n", scrollerz->atchar, scrollerz->atpixel);
	
	credzScrollerUnload(scrollerz);
	credzSlideUnload(slidez);
	credzMainCleanup();
	
	//debug("Value of bg: x, y, zoom, rotate is %f, %f, %f, %f\n", (float)bg.x, (float)bg.y, (float)bg.z, (float)bg.r);
	
	credzFinish();	
}

// ----   Main Code
// tursi - not used here
#if 0
int main(int argc, char **argv)
{   
	credzMain();
	return 0;
}
#endif
