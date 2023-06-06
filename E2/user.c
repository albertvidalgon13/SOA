#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
	
	/* GENERATING PAGE FAUL EXCEPTION */
//	char* p = 0;
//     	*p = 'x';	

	/* ERROR CHECKING*/
//	char buffer[20] = "hola que tal";
//	if(write(1,buffer,-1) == -1) perror();	//should retourn EINVAL error
//	if(write(0,buffer,20) == -1) perror();
/*	char *p = 0;
	if(write(1,p,20) == -1) perror();
*/	
	/* WRITE CHECKING*/
	//char bufwrite[20] = "hola";
	//int num = write(1,bufwrite,strlen(bufwrite));
/*	write(1,"hola",4);
	write(1,"adeu",4);
	write(1,"aaa",3);
*/	/*	if(num == strlen(bufwrite)){
		write(1,"Escrit ok ", 20);
	}
*/
/*	write(1,"hola",4);
	write(1,"adeu",4);

	struct stats x;
	get_stats(getpid(),&x);

	int a = x.elapsed_total_ticks;

	char buff2[10];
	itoa(a,buff2);
	write(1,buff2,10);
	*/
/*	int tks = gettime();
	itoa(tks,buff);
	write(1,buff,20);
*/
	//fork();
	//int message_timer = 2;

	//exit();

	while(1) {
/*
		message_timer++;
			
			if(message_timer%100000000 == 1) {
				exit();
			}
	}
*/
	}	

}
