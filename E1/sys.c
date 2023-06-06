/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>
#include <errno.h>
#include <utils.h>
#include <io.h>

#include <mm.h>

#include <mm_address.h>
#include <system.h>
#include <sched.h>

#define LECTURA 0
#define ESCRIPTURA 1


int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  int PID=-1;

  // creates the child process
  
  return PID;
}

void sys_exit()
{  
}

void perror(void);

//char so_buffer[512];
char so_buffer[4096];

int sys_write(int fd, char * buffer, int size) {
	//fd: file descriptor. In this delivery it must always be 1.
	//buffer: pointer to the bytes.
	//size: number of bytes.
	//return ’ Negative number in case of error (specifying the kind of error) and
	//the number of bytes written if OK.
	

	int num_error = check_fd(fd,1);
	if (num_error < 0) return num_error;
	if (buffer == NULL) return -EFAULT;
	if (size < 0) return -EINVAL;
	
	//Copiar el buffer del user al so amb copy_from_user
	
	int remaining_bytes = size;
	int written_bytes = 0;
	while (remaining_bytes > 4096) {
		int error = copy_from_user(buffer+written_bytes,so_buffer,4096); //res = 0 ok, res = -1 error
		if (error == -1) return error;

		written_bytes +=  sys_write_console(so_buffer,4096);
		remaining_bytes -= written_bytes;
	}
	
	int error2 = copy_from_user(buffer+written_bytes,so_buffer,remaining_bytes);
	if (error2 == -1) return error2;

	written_bytes += sys_write_console(so_buffer,remaining_bytes);

	return written_bytes;
}

int sys_gettime(){
	return zeos_ticks;
}




