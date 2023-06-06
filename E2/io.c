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

/* SCROLL IMPLEMENTATION (OPTIONAL PDF)*/
void scroll(){
	Word *screen = (Word *)0xb8000;

	for (int i = 0; i < NUM_ROWS; ++i){
		for (int j = 0; j < NUM_COLUMNS; ++j){
			screen[(i*NUM_COLUMNS+j)] = screen[((i+1)*NUM_COLUMNS+j)];
		}
	}
}

void search_last_char(){
	Word *screen = (Word *)0xb8000;
	int found = 0;
	for (int i = NUM_COLUMNS; i>=0; --i){
		if(screen[((y-1)*NUM_COLUMNS+i)] == 'a'){
			x=i;
			found = 1;
		}
		    
	}
	--y;
	if (found == 0){
		x = 0;
	}
}

/* Read a byte from 'port' */
Byte inb (unsigned short port)
{
  Byte v;

  __asm__ __volatile__ ("inb %w1,%0":"=a" (v):"Nd" (port));
  return v;
}

void printc(char c)
{
     __asm__ __volatile__ ( "movb %0, %%al; outb $0xe9" ::"a"(c)); /* Magic BOCHS debug: writes 'c' to port 0xe9 */
  if (c=='\n')
  {
    x = 0;
    
  	if (y+1 == NUM_ROWS){
              scroll();
              y = y;
      }
	else {
	 y=(y+1)%NUM_ROWS;

	}
  }
  else
  {
    Word ch = (Word) (c & 0x00FF) | 0x0200;
	Word *screen = (Word *)0xb8000;
	screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {
   	x=0;
	if (y+1 == NUM_ROWS){
              scroll();
              y = y;
      }
      else{
        y=(y+1)%NUM_ROWS;

      }
     }
  }}
/* PRINTC_COLOR IMPLEMENTATION (OPTIONAL PDF)*/
void print_color_blue(char c)
{
     __asm__ __volatile__ ( "movb %0, %%al; outb $0xe9" ::"a"(c)); /* Magic BOCHS debug: writes 'c' to port 0xe9 */
  if (c=='\n')
  {
    x = 0;
    //y=(y+1)%NUM_ROWS;
  if (y+1 == NUM_ROWS){
              scroll();
              y = y;
      }   
  else {
         y=(y+1)%NUM_ROWS;

        }

  }
  else if (c == 'S'){
	if (x > 0){
	  	Word ch = (Word) ('\0' & 0x00FF) | 0x0300;
        	Word *screen = (Word *)0xb8000;
        	screen[(y * NUM_COLUMNS + (x-1))] = ch;
		--x;
	}
	if (x == 0){
		Word ch = (Word) ('\0' & 0x00FF) | 0x0300;
                Word *screen = (Word *)0xb8000;
                screen[((y-1) * NUM_COLUMNS + (NUM_COLUMNS))] = ch;
		x = NUM_COLUMNS;
		--y;
	}
  }
  else
  {
    Word ch = (Word) (c & 0x00FF) | 0x0300;
        Word *screen = (Word *)0xb8000;
        screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {

      x = 0;
      if (y+1 == NUM_ROWS){
	      scroll();
	      y = y;
      }
      else{
      	y=(y+1)%NUM_ROWS;

      }
     }
  }
}

void print_color_blue2(char c)
{
     __asm__ __volatile__ ( "movb %0, %%al; outb $0xe9" ::"a"(c)); /* Magic BOCHS debug: writes 'c' to port 0xe9 */
  if (c=='\n')
  {
    x = 0;
    //y=(y+1)%NUM_ROWS;
    if (y+1 == NUM_ROWS){
              scroll();
              y = y;
      }
      else {
         y=(y+1)%NUM_ROWS;
        }

  }
  else
  {
    Word ch = (Word) (c & 0x00FF) | 0x0900;
        Word *screen = (Word *)0xb8000;
        screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {

      x = 0;
      if (y+1 == NUM_ROWS){
              scroll();
              y = y;
      }
      else{
        y=(y+1)%NUM_ROWS;

      }
     }
  }
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

void printkg(char *string)
{
  int i;
  for (i = 0; string[i]; i++)
	  print_color_blue2(string[i]);
}
