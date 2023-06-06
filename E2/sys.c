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

int PID_NEXT = 2;

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
	user_to_system(current());
	return current()->PID;
	system_to_user(current());
}

int ret_from_fork() {
	printk("hola soy el hijo");
	
}
/*
void init_stats(struct task_struct * tsk){
	tsk->estadistiques.user_ticks = 0;
  	tsk->estadistiques.system_ticks = 0;
  	tsk->estadistiques.blocked_ticks = 0;
  	tsk->estadistiques.ready_ticks = 0;
  	tsk->estadistiques.elapsed_total_ticks = 0;
  	tsk->estadistiques.total_trans = 0; 
  	tsk->estadistiques.remaining_ticks = 0;
}*/
int sys_fork()
{
	user_to_system(current());
	int PID=-1;
	if (list_empty(&freequeue)) return PID;
	/*  a) get free task_struct process*/
	struct list_head *l_first = list_first(&freequeue);
	list_del(l_first);
	struct task_struct *hijo_struct = list_head_to_task_struct(l_first);
	union task_union *hijo = ((union task_union*) hijo_struct);

	/* b) */
	//hijo = current(); //MAYBE COPY_DATA 2-b)
	copy_data(current(), hijo, sizeof(union task_union));
	/* c) */
	allocate_DIR(hijo);

	/* d) inherit system data*/
	int free_frames [NUM_PAG_DATA]; //frames disponibles = 0
	for (int i = 0; i < NUM_PAG_DATA; ++i) {
		int freeframe = alloc_frame();
		if (freeframe == -1) { //memoria insuficient
			for(int j = 0; j <= i; ++j) {
				free_frame(free_frames[j]);
			}
			list_add_tail(&(hijo->task.list),&freequeue);
			return PID;
		} 
		else free_frames[i] = freeframe;
	}

	/* e) i -> inherit user data*/
	page_table_entry *pt_hijo = get_PT(&hijo->task);

	page_table_entry *pt_parent = get_PT(current());

	/* asociate dad pages to child ones*/
	for (int i = 0; i < NUM_PAG_KERNEL; ++i){
		set_ss_pag(pt_hijo, i, get_frame(pt_parent,i));
	}

	for (int i = 0; i < NUM_PAG_CODE; ++i){
		set_ss_pag(pt_hijo, i+PAG_LOG_INIT_CODE, get_frame(pt_parent, i+PAG_LOG_INIT_CODE));
	}

	for (int i = 0; i < NUM_PAG_DATA; ++i){
		set_ss_pag(pt_hijo, i+PAG_LOG_INIT_DATA, free_frames[i]);
	}
	
	/* e) ii -> SHARED MEM BETWEEN PARENT AND CHILD*/
	int shared_init = NUM_PAG_KERNEL+NUM_PAG_CODE;	
	int shared_end = NUM_PAG_DATA+NUM_PAG_CODE+NUM_PAG_KERNEL;
	// bucle com el fet a classe de teoria, exercici examen

	int pos = PAG_LOG_INIT_DATA+NUM_PAG_DATA;
	
	for (int x = shared_init; x < shared_end; ++x){
		set_ss_pag(pt_parent, x+shared_init, get_frame(pt_hijo,x));		
		copy_data((void *)(x<<12), (void *)((shared_init+x)<<12), PAGE_SIZE); // <<12
		del_ss_pag(pt_parent, x+shared_init);

	}

	/* FLUSH TLB*/
	set_cr3(get_DIR(current()));

	/* f) asignar PID al fill*/
	hijo->task.PID = PID_NEXT;
	PID = PID_NEXT;
	++PID_NEXT;
	/* g) h) inituialize not common fields*/

	//ctxhw -> 5 , ctxsw -> 11 , @rethand -> 1 == 17 
	hijo->stack[KERNEL_STACK_SIZE-18] = (unsigned long)(ret_from_fork); 
	hijo->stack[KERNEL_STACK_SIZE-19] = (unsigned long)0;
	hijo->task.KERNEL_ESP = &(hijo)->stack[KERNEL_STACK_SIZE-19];
	hijo->task.quantum_process = DEFAULT_QUANTUM;

	init_stats(hijo->task);

	/* insert child to the ready queue*/
	list_add_tail(&(hijo->task.list), &readyqueue);
	system_to_user(current());
  	return PID;
}

void sys_exit()
{  
	
	page_table_entry * pt_current = get_PT(current());

	for (int i = 0; i < NUM_PAG_DATA; ++i){
		free_frame(get_frame(pt_current,PAG_LOG_INIT_DATA+i));
		del_ss_pag(pt_current, PAG_LOG_INIT_DATA+i);
	}

	printk("\nmuero con PID: ");
	int x = current()->PID;
	char buff[20];
	itoa(x,buff);
	printk(buff);
	printk("\n");


	update_process_state_rr(current(),&freequeue);
	current()->PID = -1;

	sched_next_rr();
	
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
	
	user_to_system(current());

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
	system_to_user(current());
	return written_bytes;
}

int sys_gettime(){
	user_to_system(current());
	return zeos_ticks;
	system_to_user(current());
}


int sys_get_stats(int pid, struct stats *st){
	
	if (pid < 0) return -1;

	if(!access_ok(VERIFY_WRITE,st,sizeof(struct stats))) return -1;

	for (int i = 0; i < NR_TASKS; ++i ){
		if(task[i].task.PID == pid){
			int x = copy_to_user(&task[i].task.estadistiques, st, sizeof(struct stats));
			if (x == -1) return -1;
			return 0;
		}
	}

	return -1;
}