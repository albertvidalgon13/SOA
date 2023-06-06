/*
 * assembler.S - Entry point to system mode from user mode
 */

#include <asm.h>
#include <segment.h>


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


ENTRY(wrmsr)
	push %ebp;
	movl %esp,%ebp;
    	movl 8(%ebp),%ecx;
	movl 12(%ebp),%edx;
	movl 16(%ebp),%eax;
	wrmsr;
	movl %ebp,%esp;
	pop %ebp;
	ret;

ENTRY(task_switch)
	push %ebp;
	movl %esp,%ebp;
	
	push %ebx;
	push %esi;
	push %edi;
	
	pushl 8(%ebp)
	call inner_task_switch;
	
	addl $4,%esp	

	pop %edi;
	pop %esi;
	pop %ebx;
	movl %ebp, %esp;
	pop %ebp;
	ret;

ENTRY(inner_task_switch_assembler)
	movl 4(%esp),%eax;
	movl %ebp,(%eax);
	movl 8(%esp),%esp;
	popl %ebp;
	ret;

ENTRY(inner_task_switch_assembler1)
	movl 4(%esp),%eax;
	movl %ebp,(%eax);
	popl %ebp;
	ret;
	
ENTRY(inner_task_switch_assembler2)
	movl 4(%esp),%esp;
	ret;


