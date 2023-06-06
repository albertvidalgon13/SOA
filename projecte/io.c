/*
 * io.c - 
 */

#include <io.h>

#include <types.h>

/**************/
/** Screen  ***/
/**************/

#define NUM_COLUMNS 80
#define NUM_ROWS    25

Byte x, y=19;

Word background = 0x0000;
Word foreground = 0x0200;

/* Read a byte from 'port' */
Byte inb (unsigned short port)
{
  Byte v;

  __asm__ __volatile__ ("inb %w1,%0":"=a" (v):"Nd" (port));
  return v;
}

Word word_convert(int number){
  if (number == 0) return 0x0000;
  if (number == 1) return 0x0100;
  if (number == 2) return 0x0200;
  if (number == 3) return 0x0300;
  if (number == 4) return 0x0400;
  if (number == 5) return 0x0500;
  if (number == 6) return 0x0600;
  if (number == 7) return 0x0700;
  if (number == 8) return 0x0800;
  if (number == 9) return 0x0900;
  if (number == 10) return 0x0a00;
  if (number == 11) return 0x0b00;
  if (number == 12) return 0x0c00;
  if (number == 13) return 0x0d00;
  if (number == 14) return 0x0e00;
  if (number == 15) return 0x0f00;
}

void init_pantalla(){

  Word ch = (Word) (' ' & 0x00FF) | 0x00000;
	Word *screen = (Word *)0xb8000;

  for (int i = 0; i < NUM_ROWS; ++i){
		for (int j = 0; j < NUM_COLUMNS; ++j){
			screen[(i*NUM_COLUMNS+j)] = ch;
		}
	}
}

void printc(char c)
{
     __asm__ __volatile__ ( "movb %0, %%al; outb $0xe9" ::"a"(c)); /* Magic BOCHS debug: writes 'c' to port 0xe9 */
  if (c=='\n')
  {
    x = 0;
    y=(y+1)%NUM_ROWS;
  }
  else
  {
    Word ch = (Word) (c & 0x00FF) | (foreground ^ background);
	Word *screen = (Word *)0xb8000;
	screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {
      x = 0;
      y=(y+1)%NUM_ROWS;
    }
  }
}

void set_color(int fg, int bg){
  foreground = word_convert(fg);
  background = word_convert(bg)<<1;
}

void set_xy(int auxx, int auxy){
  x = auxx;
  y = auxy;
}


void printc_xy(Byte mx, Byte my, char c)
{
  Byte cx, cy;
  cx=x;
  cy=y;
  x=mx;
  y=my;
  printc(c);
  x=cx;
  y=cy;
}

void printk(char *string)
{
  int i;
  for (i = 0; string[i]; i++)
    printc(string[i]);
}
