/*
 * libc.c 
 */

#include <libc.h>
#include <errno.h>
#include <types.h>

int errno; 

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

char bufferror[3];

void perror(){
/*	
	char b[20];
	itoa(errno,b);
	write(1,b,20);	
*/
	if (errno == EFAULT){
		write(1, "ERROR! Buffer is NULL", 22);
	}
	else if (errno == EINVAL){
		write(1, "ERROR! Size less than 0",23);
	}
	else {
		write(1, "ERROR!", 6);
		itoa(errno,bufferror);
		write(1, "Errno code: ",12);
		write(1,bufferror,strlen(bufferror));
	}

}
/*
int getpid(void){
  
}*/
