/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>
#include <system.h>
#include <zeos_interrupt.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

int coordx = 0;
int coordy = 0;

char char_map[] =
{
  '\0','C','1','2','3','4','5','6',
  '7','8','9','0','\'','¡','S','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\n','\0','a','s',
  'd','f','g','h','j','k','l','ñ',
  '\0','º','\0','ç','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0',' ','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void clock_routine(){
	zeos_show_clock();
	++zeos_ticks;
}
void keyboard_routine(){
	char x = inb(0x60);
	if ((x & 0x80) != 0x80){
	//	printc_xy(0+coordx,20+coordy,char_map[x & 0x7f]);
		char llet = char_map[x & 0x7f];
		if (llet == '\0') print_color_blue(char_map[1]);
		else print_color_blue(char_map[x & 0x7f]);
	//	printc(x);
	}
}

void pf_routine(unsigned long FLAGS, unsigned long EIPreg) {
	//In case the conversion does not function use:
	//char buffer[8];
	//itoa(EIPreg,buffer); //convert numeric value into char array (String)
	//printk("\nProcess generates a PAGE FAULT exception at EIP: ");
	//printk(buffer);
	//This number won't be in base16 so you must convert
	
	printk("\nProcess generates a PAGE FAULT exception at EIP: 0x");
	char reversedDigits[20];
	int i = 0;
	unsigned long  decimal = EIPreg;
	while (decimal > 0) {
		unsigned long remain = decimal % 16;

		if (remain < 10)
			reversedDigits[i] = '0' + remain;
		else
			reversedDigits[i] = 'A' + (remain - 10);

		decimal = decimal/16;
		i++;
	}

	while (i >= 0) {
		printc(reversedDigits[i]);
		i--;
	}
	printk("\n");
	while(1) {}
}

void clock_handler();
void keyboard_handler();
void pf_handler();

void wrmsr(unsigned long ecx, unsigned long edx, unsigned long eax); 
void syscall_handler_sysenter();
int gettime();
int write(int fd, char *buffer, int size);

void setIdt()
{
	/* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  
  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  setInterruptHandler(32, clock_handler, 0);
  setInterruptHandler(33, keyboard_handler, 0);
  setInterruptHandler(14, pf_handler, 0);
  //setTrapHandler(0x80, syscall_handler_sysenter, 3);
  
  wrmsr(0x174, 0, __KERNEL_CS);
  wrmsr(0x175, 0, INITIAL_ESP);
  wrmsr(0x176, 0, syscall_handler_sysenter);

  set_idt_reg(&idtR);
}
