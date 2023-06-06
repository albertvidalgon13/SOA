/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));


struct task_struct *list_head_to_task_struct(struct list_head *l)
{
	return (struct task_struct*)((unsigned int)l&0xfffff000);
	//posar lo primer
}

int DEFAULT_QUANTUM = 10000;

struct list_head blocked;

struct list_head freequeue;
struct list_head readyqueue;

struct task_struct * idle_task;
struct task_struct * init_task;

int ticks_quantum = 10;

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);
	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");
	printk("entering cpu idle");

	while(1)
	{
	}
}


void update_sched_data_rr(void){
	--ticks_quantum;
}

int needs_sched_rr (void){

	if (list_empty(&readyqueue) && ticks_quantum == 0) {
		ticks_quantum = DEFAULT_QUANTUM;
		return 0;
	}
	if (ticks_quantum == 0) return 1;
	return 0;
	
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue){
	struct list_head *list = &t->list;	
	if (list->prev && list->next){
		list_del(list);
	}
	if (dst_queue != NULL) list_add_tail(list,dst_queue);

}

void sched_next_rr(void){

	struct task_struct * new_ready;
	
	if (list_empty(&readyqueue)){
		new_ready = idle_task;
	}
	else {
		struct list_head *l_first = list_first(&readyqueue);
		list_del(l_first);
		new_ready = list_head_to_task_struct(l_first);
	}

	ticks_quantum = new_ready->quantum_process;
	
	task_switch(new_ready);


}

void schedule(){

	update_sched_data_rr();
	if (needs_sched_rr()){
		update_process_state_rr(current(),&readyqueue);
		sched_next_rr();
	}
}

void inner_task_switch_assembler(unsigned long *x, unsigned long y);

void ready_to_system(struct task_struct * tsk){
	tsk->estadistiques.ready_ticks += get_ticks() - tsk->estadistiques.elapsed_total_ticks; 
	tsk->estadistiques.elapsed_total_ticks = get_ticks(); 
}

void system_to_ready(struct task_struct * tsk){
	tsk->estadistiques.system_ticks += get_ticks() - tsk->estadistiques.elapsed_total_ticks; 
	tsk->estadistiques.elapsed_total_ticks = get_ticks(); 
}

void user_to_system(struct task_struct * tsk){
	tsk->estadistiques.user_ticks += get_ticks() - tsk->estadistiques.elapsed_total_ticks; 
	tsk->estadistiques.elapsed_total_ticks = get_ticks();
}
void system_to_user(struct task_struct * tsk){
	tsk->estadistiques.system_ticks += get_ticks() - tsk->estadistiques.elapsed_total_ticks; 
	tsk->estadistiques.elapsed_total_ticks = get_ticks(); 
}

void inner_task_switch(union task_union *new){

	tss.esp0 = (long unsigned int)(&(new->stack[KERNEL_STACK_SIZE]));
	
	
	wrmsr(0x175,0,(long unsigned int)(&(new->stack[KERNEL_STACK_SIZE])));
	
	set_cr3(get_DIR(&(new->task)));
	//printk("0");
	// tssesp0 <- &new->stack[1024]
	printk("\nentrando en inner task switch: \n");
	// store current ebp in pcb
	//current()->KERNEL_ESP = tss.ebp; //ebp de la cpu no de la tss -> ensamblador
	//tss.esp = new->task.KERNEL_ESP; //funcio en ensablador que rebi kernel_esp,
					//posar ebp en &kernel_esp
					//task.kernel_esp segon parametre
					//posarho a esp
	int x = current()->PID;
	char buff[20];
	itoa(x,buff);
	int x2 = new->task.PID;
	char buff2[20];
	itoa(x2,buff2);
	printk("pasando de PID: ");
	printk(buff);
	printk(" a PID: ");
	printk(buff2);
	printk("\n");

	ready_to_system(&new->task);
	system_to_ready(current());

	inner_task_switch_assembler(&(current()->KERNEL_ESP),new->task.KERNEL_ESP);
	
	
}

void init_stats(struct task_struct * tsk){
	tsk->estadistiques.user_ticks = 0;
  	tsk->estadistiques.system_ticks = 0;
  	tsk->estadistiques.blocked_ticks = 0;
  	tsk->estadistiques.ready_ticks = 0;
  	tsk->estadistiques.elapsed_total_ticks = 0;
  	tsk->estadistiques.total_trans = 0; 
  	tsk->estadistiques.remaining_ticks = 0;
}

void init_idle (void)
{
	//get first value of freequeue
	struct list_head *l_first = list_first(&freequeue);
	//eliminem de la freequeue ja que a partir d'ara no estara free
	list_del(l_first);
	//creem task_struct a partir del list head
	struct task_struct *tsk = list_head_to_task_struct(l_first);
	
	//PID = 0
	tsk->PID = 0;
	tsk->quantum_process = DEFAULT_QUANTUM;

	allocate_DIR(tsk);
	
	((union task_union *)tsk)->stack[KERNEL_STACK_SIZE-1] = (unsigned long)(cpu_idle); //parametrizar -> warnings
	((union task_union *)tsk)->stack[KERNEL_STACK_SIZE-2] = (unsigned long)0;
	tsk->KERNEL_ESP = &((union task_union *)tsk)->stack[1022];
	
	init_stats(tsk);

	idle_task = tsk;

}

void init_task1(void)
{
	//get first value of freequeue **NOT SURE**
	struct list_head *l_first = list_first(&freequeue);
	//eliminem de la freequeue ja que a partir d'ara no estara free
	list_del(l_first);
	//creem task_struct a partir del list head
	struct task_struct *tsk = list_head_to_task_struct(l_first);
	
	//PID = 1
	tsk->PID = 1;
	tsk->quantum_process = DEFAULT_QUANTUM;

	allocate_DIR(tsk);
	set_user_pages(tsk);

	tss.esp0 = &((union task_union * )tsk)->stack[KERNEL_STACK_SIZE];
	
	wrmsr(0x175, 0, &((union task_union * )tsk)->stack[KERNEL_STACK_SIZE]);

	set_cr3(((union task_union * )tsk)->task.dir_pages_baseAddr);

	init_stats(tsk);

	init_task = tsk;
}


void init_sched()
{
	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);
	//ini cola free
	for (int i = 0; i < NR_TASKS; ++i ){
		list_add(&(task[i].task.list), &freequeue);
	}

//ready queue empty at beggining
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

