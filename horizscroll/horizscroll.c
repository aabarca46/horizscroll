/*
to see what we attempted 
  make changes in new_segment()
  comment seg_width=6;
  uncomment seg_width=3;
  make changes in set_attr_entry(byte x, byte y, byte pal)
  uncomment code with label
  " comment the below code to test "

to add collision
  make changes in scroll_left()
  uncomment code with label 
  " uncomment to test collission "
  cry when you see how bad it is.
*/
#include <stdlib.h>
#include "neslib.h"
#include <string.h>
#include <joystick.h>
#include <nes.h>
#include "horizscroll.h"
#include "vrambuf.h"
//#link "vrambuf.c"
// link the pattern table into CHR ROM
//#link "chr_generic.s"

// 0 = horizontal mirroring
// 1 = vertical mirroring
#define NES_MIRRORING 1
#define PLAYROWS 24	// max tile height / 2
#define TILE 0xd8	// playable character attributes
#define TILE1 0xCC	// heart attributes
#define ATTR1 01	// heart attributes
#define ATTR 02		// character attributes
#define seg_char 0xf4
/// GLOBAL VARIABLES
word x_scroll;		// X scroll amount in pixels
byte seg_height;	// segment height in metatiles
byte seg_width;		// segment width in metatiles
byte seg_palette;	// attribute table value
byte seg_gap;		// segment gap in metatile tower
int frm_cnt;		// frame counter
unsigned char pad1;	// joystick
unsigned char pad1_new; // joystick
byte death_count;
// buffers that hold vertical slices of nametable data
char ntbuf1[PLAYROWS];	// left side
char ntbuf2[PLAYROWS];	// right side


// initialize block values to check later
char block1 = 0xf4;
char block2 = 0xf5;
char block3 = 0xf6;
char block4 = 0xf7;

//Direction array affects movement and gravity
typedef enum { D_RIGHT, D_DOWN, D_LEFT, D_UP } dir_t;
const char DIR_X[4] = { 1, 0, -1, 0 };
const char DIR_Y[4] = { 0, 2, 0, -2 };

// a vertical slice of attribute table entries
char attrbuf[PLAYROWS/4];

//#link "famitone2.s"
void __fastcall__ famitone_update(void);

//#link "music_aftertherain.s"
extern char after_the_rain_music_data[];
//#link "demosounds.s"
extern char demo_sounds[];

// Hero Metasprite info
const unsigned char metasprite[]={
        0,      0,      TILE,     ATTR, 
        0,      8,      TILE+1,   ATTR, 
        8,      0,      TILE+2,   ATTR, 
        8,      8,      TILE+3,   ATTR, 
        128};

// Heart Metasprite info
const unsigned char metasprite1[]={
        0,      0,      TILE1,     ATTR1, 
        0,      8,      TILE1+1,   ATTR1, 
        8,      0,      TILE1+2,   ATTR1, 
        8,      8,      TILE1+3,   ATTR1, 
        128};

// initializes heart and hero 
Hero heros;
Heart hearts;

/*{pal:"nes",layout:"nes"}*/
const char PALETTE[32] = 
{ 
  0x0c,			// background color

  0x2C,0x30,0x27,0x00,	// ladders and pickups
  0x1C,0x20,0x2C,0x00,	// floor blocks
  0x00,0x10,0x20,0x00,
  0x06,0x16,0x26,0x00,

  0x16,0x35,0x24,0x00,	// enemy sprites
  0x00,0x37,0x25,0x00,	// rescue person
  0x0D,0x2D,0x1A,0x00,
  0x0D,0x27,0x2A	// player sprites
};

// function to write a string into the name table
//   adr = start address in name table
//   str = pointer to string
void put_str(unsigned int adr, const char *str) 
{
  vram_adr(adr);        // set PPU read/write address
  vram_write(str, strlen(str)); // write bytes to PPU
}

byte getchar(byte x, byte y) 
{
  // compute VRAM read address
  word addr = NTADR_A(x,y);
  // result goes into rd
  byte rd;
  // wait for VBLANK to start
  ppu_wait_nmi();
  // set vram address and read byte into rd
  vram_adr(addr);
  vram_read(&rd, 1);
  // scroll registers are corrupt
  // fix by setting vram address
  vram_adr(0x0);
  return rd;
}

//function displayes text
void cputcxy(byte x, byte y, char ch) 
{
  vrambuf_put(NTADR_A(x,y), &ch, 1);
}

//function displayes text
void cputsxy(byte x, byte y, const char* str) 
{
  vrambuf_put(NTADR_A(x,y), str, strlen(str));
}

// convert from nametable address to attribute table address
word nt2attraddr(word a) {
  return (a & 0x2c00) | 0x3c0 |
    ((a >> 4) & 0x38) | ((a >> 2) & 0x07);
}

// generate new segment
void new_segment() 
{
  seg_height = PLAYROWS;
  seg_width = 6;
  //seg_width = 3;
  seg_palette = 3;
  seg_gap = (rand8() & 3) + 2;
}

// draw metatile into nametable buffers
// y is the metatile coordinate (row * 2)
// ch is the starting tile index in the pattern table
void set_metatile(byte y, byte ch) 
{
  ntbuf1[y*2] = ch;
  ntbuf1[y*2+1] = ch+1;
  ntbuf2[y*2] = ch+2;
  ntbuf2[y*2+1] = ch+3;
}

// set attribute table entry in attrbuf
// x and y are metatile coordinates
// pal is the index to set
void set_attr_entry(byte x, byte y, byte pal) 
{
  if (y&1) pal <<= 4;
  if (x&1) pal <<= 2;
  attrbuf[y/2] |= pal;
}

// fill ntbuf with tile data
// x = metatile coordinate
void fill_buffer(byte x) {
byte i,y;

  // clear nametable buffers
  memset(ntbuf1, 0, sizeof(ntbuf1));
  memset(ntbuf2, 0, sizeof(ntbuf2));
  
  // draw a random star
  ntbuf1[rand8() & 15] = '^';
  // draw segment slice to both nametable buffers
  for (i=0; i<seg_height; i++) 
  {
    if( i == seg_gap +1 )
    {

    }
    else if(i == seg_gap  || i == seg_gap + 2 || i == seg_gap + 3)
    {
    	/* Comment the below code to test */
    }else if
    (seg_width == 3 || seg_width == 2 || seg_width == 1)
    {
    }
    else
    {
    y = PLAYROWS/2-1-i;
     
    set_metatile(y, seg_char);
    set_attr_entry(x, y, seg_palette);
    }
  }
}

// write attribute table buffer to vram buffer
void put_attr_entries(word addr) {
  byte i;
  for (i=0; i<PLAYROWS/4; i++) {
    VRAMBUF_PUT(addr, attrbuf[i], 0);
    addr += 8;
  }
  vrambuf_end();
}

// update the nametable offscreen
// called every 8 horizontal pixels
void update_offscreen() 
{
  register word addr;
  byte x;
  // divide x_scroll by 8
  // to get nametable X position
  x = (x_scroll/8 + 32) & 63;
  // fill the ntbuf arrays with tiles
  fill_buffer(x/2);
  // get address in either nametable A or B
  if (x < 32){
    addr = NTADR_A(x, 4);
  }else
    addr = NTADR_B(x&31, 4);
  // draw vertical slice from ntbuf arrays to name table
  // starting with leftmost slice
  vrambuf_put(addr | VRAMBUF_VERT, ntbuf1, PLAYROWS);
  // then the rightmost slice
  vrambuf_put((addr+1) | VRAMBUF_VERT, ntbuf2, PLAYROWS);
  // compute attribute table address
  // then set attribute table entries
  // we update these twice to prevent right-side artifacts
  put_attr_entries(nt2attraddr(addr));
  // every 4 columns, clear attribute table buffer
  if ((x & 2) == 2) 
  {
    memset(attrbuf, 0, sizeof(attrbuf));
  }
  // decrement segment width, create new segment when it hits zero
  if (--seg_width == 0) 
  {
    new_segment();
  }
}

// checks if hero y is out of bounds
// if true set collision to 1
void check_for_collision(Hero* h)
{
  if (h->y == 238 || h->y == 26)
    h->collided = 1 ;
}

// get char of tile hero is on. 
// if tile matches char value of block1 - block4 
// set collision to 1
void check_for_wall(Hero* h){
  char i;
  i = getchar(h->x, h->y);
  if( i == block1 || i == block2 || i == block3 || i == block4)
  {
    h->collided = 1 ;
  }
}

// take direction of movement and move player x and y by that direction value
void move_player(Hero* h)
{
  h->x += DIR_X[h->dir];
  h->y += DIR_Y[h->dir];
}
// read user input and set hero direction to that value
void movement(Hero* h)
{
  byte dir;
  pad1_new = pad_trigger(0); // read the first controller
  pad1 = pad_state(0);
  
  if (pad1 & JOY_BTN_A_MASK) dir = D_UP;
  else dir = D_DOWN;
  
  h->dir = dir;
}

//bit1 every 10 increments set bit1 to zero increment bit2
//bit2 every 10 increments set bit1 and bit2 to zero increment bit3
//bit3 every 10 increments set bit1 bit2 and bit3 to zero increment bit4
void add_point(Hero* h)
{
  h->bit1++;
  
  if(h->bit3 == 10)
  {
    h-> bit1 = 0;
    h-> bit2 = 0;
    h-> bit3 = 0;
    h-> bit4++;
    cputcxy(14,1,h-> bit4+'0');
    cputcxy(15,1,h-> bit3+'0');
    cputcxy(16,1,h-> bit2+'0');
    cputcxy(17,1,h-> bit1+'0');
  }
  else if(h->bit2 == 10)
  {
    h-> bit1 = 0; 
    h-> bit2 = 0;
    h-> bit3++;
    cputcxy(15,1,h-> bit3+'0');
    cputcxy(16,1,h-> bit2+'0');
    cputcxy(17,1,h-> bit1+'0'); 
  }
  else if(h->bit1 == 10)
  {
    h-> bit1 = 0;
    h-> bit2++;
    cputcxy(16,1,h-> bit2+'0');
    cputcxy(17,1,h-> bit1+'0'); 
  }
  else
  {
    cputcxy(17,1,h-> bit1+'0');
  }
}

//spawn a new heart at 240 x and y is set to the segment gap +1
void spawn_item(Heart* h)
{
  sfx_play(1,0);
  h->x = 240;
  h->y = (8 * seg_height) - (seg_gap * 16);
  oam_meta_spr( h->x , h->y, 24, metasprite1);
  
}

// scrolls the screen left one pixel
// updates off screen tiles every 15 frames
// checks input from user ever 2 frames
// checks for collision with top and bottom barriers restarts game if true
// moves player up and down if false
// checks for collission with tiles every 120 frames(buggy) 
// if true restart else reset counter and continue
// checks for hero and heart coordinates. if coordinates match +1 spawn new item
// if false move heart and player
// increment frm count and scroll count similar but different functionality
void scroll_left() 
{
  if ((x_scroll & 15) == 0) 
  {
    update_offscreen();
  }
  if((x_scroll & 2) == 0)
  {
    movement(&heros);
  }
  check_for_collision(&heros);
  move_player(&heros);  
  if(frm_cnt == 120)
  {
    /* uncomment to test collission */
    //check_for_wall(&heros);
    frm_cnt=0;
  }
    if(heros.x == hearts.x)
    {
      if(heros.y == hearts.y)
      {
      	hearts.x = NULL;
        hearts.y = NULL;
        oam_meta_spr(hearts.x, hearts.y, 24, metasprite1);
        add_point(&heros);
      	spawn_item(&hearts);
      }
    }else
    {
    hearts.x = hearts.x -1;
    oam_meta_spr(hearts.x , hearts.y, 24, metasprite1);
    }

  oam_meta_spr(heros.x, heros.y, 4, metasprite);  
  ++x_scroll;
  frm_cnt++;
}
  
// scrolls the screen left one pixel
// updates off screen tiles every 15 frames
// move heart sprite
void title_screen_scroll() 
{
  if((x_scroll & 15) == 0) 
  {
    update_offscreen();
  }
  if(hearts.x == NULL)
  {
   spawn_item(&hearts); 
  }
  hearts.x = hearts.x -1;
  oam_meta_spr(hearts.x , hearts.y, 24, metasprite1);
  ++x_scroll;
}

// start new segment of tiles
// set x_scroll to 0
// spawn heart
// call scroll left 
// loop runs until collision is detected
// break and call initialize game
void main_scroll() 
{

    
  new_segment();
  x_scroll = 0;
  spawn_item(&hearts);
  // infinite loop
  while (1) 
  {
    
    // ensure VRAM buffer is cleared
    ppu_wait_nmi();
    vrambuf_clear();
    // split at sprite zero and set X scroll
    split(x_scroll, 0); 
    // scroll to the left
    scroll_left();
    if(heros.collided == 1)
    {
    game_over();
    break;
    }
  } 
  init_game();
}

// copy of main_scroll for title screen
void scroll_title_screen()
{
  
  byte joy;
  
  new_segment();
  x_scroll = 0;
  spawn_item(&hearts);
  // infinite loop
  while (1) 
  {
    
    // ensure VRAM buffer is cleared
    ppu_wait_nmi();
    vrambuf_clear();
    // split at sprite zero and set X scroll
    split(x_scroll, 0); 
    // scroll to the left
    title_screen_scroll();
    heros.x = NULL;
    heros.y = 230;
    if(frm_cnt == 45)
    {
      cputsxy(5,10,"Press Enter to Start");
      cputsxy(5,15,"Press Enter to Start");
      cputsxy(5,20,"Press Enter to Start");
      frm_cnt= 0;
    }
    joy = joy_read (JOY_1);   
      if(joy){
        clrscrn();
        break;
      }
  frm_cnt++;
  }
}

//game over screen
void game_over()
{
  death_count++;
  clrscrn();
  if(death_count == 5)
  {
    death_count = 0;
    title_screen();
    
  }
    frm_cnt = 0;
}


//reset game
void clrscrn()
{
  vrambuf_clear();
  ppu_off();
  vram_adr(0x2000);
  vram_fill(0, 32*28);
  vram_adr(0x0);
}

void title_screen()
{
  
  vrambuf_clear();
  oam_spr(0, 0, 0xa4, 0, 0);
  set_vram_update(updbuf);
  ppu_on_all();
  cputsxy(6,1,"Infiniscroll");
  cputsxy(12,2,"Anthony Moreno ");
  cputsxy(15,3,"Alessandro Abarca");
  scroll_title_screen();
}


//sets values at top of screen 
// call scroll title screen
void init_game()
{
  vrambuf_clear();
  
  heros.bit1 = 0;
  heros.bit2 = 0;
  heros.bit3 = 0;
  heros.bit4 = 0;

  vram_adr(NTADR_A(0,3));
  vram_fill(5, 32);
  vram_adr(0x23c0);
  vram_fill(0x55, 8);

  oam_clear();
  heros.x = 48;
  heros.y = 120;
  vrambuf_clear();
  oam_spr(0, 30, 0xa5, 0, 0);
  vrambuf_clear();
  set_vram_update(updbuf);
  ppu_on_all();
  
  heros.collided = 0;
  
  cputsxy(5,1,"Score:");
  cputcxy(14,1,'0');
  cputcxy(15,1,'0');
  cputcxy(16,1,'0');
  cputcxy(17,1,'0');
  
  main_scroll();

}

void main(void) 
{
  
  /* Uncomment below code to hear sound. Headphone warning*/ 
  
  //famitone_init(after_the_rain_music_data);
  //sfx_init(demo_sounds);
  // set music callback function for NMI
  //nmi_set_callback(famitone_update);
  // play music
  //music_play(4);
  //while(1){}
  //music_stop();
  
  pal_all(PALETTE);
  joy_install (joy_static_stddrv);
  oam_clear();
  title_screen();
  
    init_game();    
}
