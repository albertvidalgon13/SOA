/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>
#include <mm.h>
#include <sched.h>

#include <zeos_interrupt.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','�','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','�',
  '\0','�','\0','�','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

int zeos_ticks = 0;

void clock_routine()
{
  zeos_show_clock();
  zeos_ticks ++;
  
  schedule();
}

/*

  comprobacion de direccion logica cr2 (para ver si ha sigo por cow)
    mirar numero de referencias en phys_mem[] (si es == 1 significa que solo la tengo yo) --> ponerla en read write
    si > 1 (alguien mas usando esta pagina) --> hacer cow, buscar una paglogica libre, fer dinamic link, ++ref pf, fer copy_data, delete link, cambiar r/w,--ref pf compartida

*/
void pf_routine(unsigned long FLAGS, unsigned long EIPreg) {
	//In case the conversion does not function use:
	//char buffer[8];
	//itoa(EIPreg,buffer); //convert numeric value into char array (String)
	//printk("\nProcess generates a PAGE FAULT exception at EIP: ");
	//printk(buffer);
	//This number won't be in base16 so you must convert
	
  int register_cr2 = (int) get_cr2();
  int real_page = register_cr2>>12;
  page_table_entry *process_PT = get_PT(current());

  if (real_page >= PAG_LOG_INIT_DATA && real_page < PAG_LOG_INIT_DATA+NUM_PAG_DATA){
    unsigned int frame = get_frame(process_PT,real_page);
    char buff3[23];
    itoa(phys_mem[frame], buff3);
//    printk(buff3);
    if (phys_mem[frame] == 1) process_PT[real_page].bits.rw=1; 

    else if (phys_mem[frame] > 1) {
      int free_frame = alloc_frame();
      set_ss_pag(process_PT,NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA, free_frame);
      copy_data((void*)(real_page<<12), (void*)((NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA)<<12), PAGE_SIZE);
      del_ss_pag(process_PT, NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA);
      del_ss_pag(process_PT,real_page);
      set_ss_pag(process_PT, real_page, free_frame);
      ++phys_mem[free_frame];
      --phys_mem[frame];
    }
  }
  else {
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
}


void keyboard_routine()
{
  unsigned char c = inb(0x60);
  if ((c & 0x80) != 0x80){
    if (buffer_circular.items + 1 < 256 ){
      buffer_circular.buffer[buffer_circular.escritura] = char_map[c&0x7f];

      if (buffer_circular.escritura >= 255) buffer_circular.escritura = 0;
      else ++buffer_circular.escritura;
      ++buffer_circular.items;
    }
  //printc_xy(0,0,buffer_circular.buffer[buffer_circular.lectura ]);
  }

  //for (int i = 0; i < buffer_circular.items; ++i)

  //if (c&0x80) printc_xy(0, 0, char_map[c&0x7f]);
}

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

void clock_handler();
void keyboard_handler();
void system_call_handler();
void pf_handler();
void setMSR(unsigned long msr_number, unsigned long high, unsigned long low);

void setSysenter()
{
  setMSR(0x174, 0, __KERNEL_CS);
  setMSR(0x175, 0, INITIAL_ESP);
  setMSR(0x176, 0, (unsigned long)system_call_handler);
}

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
  setSysenter();

  set_idt_reg(&idtR);
}

