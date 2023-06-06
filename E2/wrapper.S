/*
 * wrapper.S - Entry point to system mode from user mode
 */

#include <asm.h>
#include <segment.h>
#include <errno.h>

/**************************************************/
/**** Save & Restore ******************************/
/**                                              **/
/** When we change to privilege level 0 (kernel) **/
/** (through an interrupt, a system call, an     **/
/** exception ...) we must save the state of the **/
/** currently running task (save).               **/
/**                                              **/
/** Stack layout in 'systemCall':                **/
/**                                              **/
/**   0(%esp) - %ebx    \                        **/
/**   4(%esp) - %ecx     |                       **/
/**   8(%esp) - %edx     |                       **/
/**   C(%esp) - %esi     | Register saved        **/
/**  10(%esp) - %edi     |  by 'save'            **/
/**  14(%esp) - %ebp     |                       **/
/**  18(%esp) - %eax     |                       **/
/**  1C(%esp) - %ds      |                       **/
/**  20(%esp) - %es      |                       **/
/**  24(%esp) - %fs      |                       **/
/**  28(%esp) - %gs     /                        **/
/**  2C(%esp) - %eip    \                        **/
/**  30(%esp) - %cs      |                       **/
/**  34(%esp) - %eflags  |  Return context saved **/
/**  38(%esp) - %oldesp  |   by the processor.   **/
/**  3C(%esp) - %oldss  /                        **/
/**                                              **/
/**************************************************/

ENTRY(write)
    push %ebp;
    movl %esp,%ebp;
	
	movl 8(%ebp),%ebx; 	//fd -> ebx
    movl 12(%ebp),%ecx;	//buffer -> ecx
    movl 16(%ebp),%edx;	//size -> edx
	movl $4,%eax;		//index syscall_table.S
	
	pushl %edx;			//user stack
	pushl %ecx;
	pushl $if

	push %ebp			//fake dynamic link
	movl %esp,%ebp		//ebp point to the top of user stack. point 5 of pdf
	
	sysenter;

if:	
	popl %ebp;			//remove ebp, edx, ecx (temporal data) point 7 of pdf
	addl $4,%esp		//esp + 4
	popl %edx;			//
	popl %ecx;			//

	cmpl $0,%eax;		//if eax < 0
	jge else;
	neg %eax			//-eax
	movl %eax, errno;	//errno = -eax
	movl $-1,%eax;		//eax = -1

else:	
	movl %ebp,%esp;
    popl %ebp;
	ret;


ENTRY(gettime)
	push %ebp;
	movl %esp,%ebp;
	
	movl $5,%eax;

	pushl %edx;
	pushl %ecx;
	pushl $function;
	
	push %ebp;
	movl %esp,%ebp
	
	sysenter;

function:
	
	popl %ebp;
	addl $4,%esp;
	popl %edx;
	popl %ecx;
	
	movl %ebp,%esp;
	popl %ebp;
	ret;	


ENTRY(getpid)
	push %ebp;
	movl %esp,%ebp;

	movl $6,%eax;

	pushl %edx;		//user stack
	pushl %ecx;
	pushl $pidsys

	push %ebp		//fake dynamic link
	movl %esp,%ebp		//ebp point to the top of user stack. point 5 of pdf

	sysenter;
pidsys:

	popl %ebp;
	addl $4,%esp;
	popl %edx;
	popl %ecx;
	
	movl %ebp,%esp;
	popl %ebp;
	ret;


ENTRY(fork)
	push %ebp;
	movl %esp,%ebp;

	movl $7,%eax;

	pushl %edx;		//user stack
	pushl %ecx;
	pushl $forksys

	push %ebp		//fake dynamic link
	movl %esp,%ebp		//ebp point to the top of user stack. point 5 of pdf

	sysenter;
forksys:
	popl %ebp;
	addl $4,%esp;
	popl %edx;
	popl %ecx;
	
	movl %ebp,%esp;
	popl %ebp;
	ret;


ENTRY(exit)
	push %ebp;
	movl %esp,%ebp;

	movl $8,%eax;

	pushl %edx;		//user stack
	pushl %ecx;
	pushl $exit_func

	push %ebp		//fake dynamic link
	movl %esp,%ebp		//ebp point to the top of user stack. point 5 of pdf

	sysenter;

exit_func:

	popl %ebp;
	addl $4,%esp;
	popl %edx;
	popl %ecx;
	
	movl %ebp,%esp;
	popl %ebp;
	ret;



ENTRY(get_stats)
	push %ebp;
	movl %esp,%ebp;


	movl 8(%ebp),%ebx; 	//PID -> ebx
    movl 12(%ebp),%ecx;	//struct stats -> ecx
	movl $9,%eax;

	pushl %edx;		//user stack
	pushl %ecx;
	pushl $get_stats_func

	push %ebp		//fake dynamic link
	movl %esp,%ebp		//ebp point to the top of user stack. point 5 of pdf

	sysenter;

get_stats_func:
	popl %ebp;
	addl $4,%esp;
	popl %edx;
	popl %ecx;
	
	movl %ebp,%esp;
	popl %ebp;
	ret;



