#include <libc.h>
#include <stddef.h>
char buff[24];
char buff2[24];

int pid;

int y= 1;
int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

//set_color(5,11);
  //gotoxy(0,0);
  //write(1,"FPS:",4);

  //void* ptr1 = shmat(1,4000);

 // void * addr = shmat(1,1249280);
 /* int x = fork();

  if (x == 0){
    y = 20;
    itoa(y,buff);
    write(1,buff,24);
  }
  else {
    itoa(y,buff);
    write(1,buff,24);
  }
*/

  gotoxy(0,0);

  int x = fork();
  if (x == 0){
    the_game();
  }
  else {
    test();
  }


 /* if (x == 0){
    getpid();
  }
  shmrm(1);
  shmdt(addr);
  */
  /*
  int x = fork();
  if (x == 0){
    shmdt(1249280);
    getpid();
    int a = fork();
    if (a == 0){
      getpid();
    }
  }

*/
  //write a ptr1
  //read de ptr2
  //igual
  
  //int x = 2;
  while(1) { 
    /*++x;
    if (x % 100000000 == 1){
      write(1,"read!",5);
      char buff[3] = "";
      read(buff,3);
      write(1,buff,3);
    }
    */

  }
}
