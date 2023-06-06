/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

void * get_ebp();
void * get_cr2();

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	return -ENOSYS; 
}

int ll = 0;

int sys_getpid()
{
/*  if (ll == 0){
    int* ptr = (int*)0x1388;
    char r = *ptr;
    printc_xy(22,20,r);
  }
  else {
    int* ptr = (int*)0x1388;
    char r = *ptr;
    printc_xy(24,20,r);
  }
  ++ll;
*/
 /* int x = shared_vector[1].ref;
  char buff[2];
  itoa(x,buff);
  printk(buff);*/



	return current()->PID;
}

int global_PID=1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;

  char value = 'a';
 // else value = 'b';
  int* ptr = (int*)0x1388;
  *ptr = value;
  char r = *ptr;
 // printc_xy(20,20,r);
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);
      
      /* Return error */
      return -EAGAIN; 
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */

  /*
  
  compartir paginas entre padre e hijo (igual que ne el kernel y codigo) y ponerlas en read only 
  
  */

  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    set_ss_pag(process_PT, pag+PAG_LOG_INIT_DATA, get_frame(parent_PT, PAG_LOG_INIT_DATA+pag));
    process_PT[pag+PAG_LOG_INIT_DATA].bits.rw=0;
    parent_PT[pag+PAG_LOG_INIT_DATA].bits.rw=0;
    unsigned int frame2 = get_frame(process_PT,pag+PAG_LOG_INIT_DATA);
    ++phys_mem[frame2];
  }
/*
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {

    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }
*/
//  page_table_entry *parent = get_DIR(current());
//  page_table_entry *fill = get_DIR(&uchild->task);

  for (int page = NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA*2; page < TOTAL_PAGES-(NUM_PAG_CODE+NUM_PAG_DATA*2+NUM_PAG_KERNEL); ++page){
    if (parent_PT[page].bits.present){
      set_ss_pag(process_PT, page, get_frame(parent_PT,page));
      for (int x = 0; x < 10; ++x) {
        if (get_frame(parent_PT, page) == shared_vector[x].id_frame){
          ++shared_vector[x].ref;
        }
      }
    }
  }


  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;

  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
}

#define TAM_BUFFER 512

int sys_read(char* b, int maxchars){

  //if (maxchars > 256) return 0;
  //if (b == NULL) return 0;

  /* check access_ok on system buffer */
  /*char buff2[3] = "";
  if (!access_ok(VERIFY_READ, buff2, maxchars)) printk("system addr\n");*/
  if (maxchars < 0 ) return -EINVAL;
  if (!access_ok(VERIFY_READ, b, maxchars)) return -EFAULT;
  int l = buffer_circular.lectura;
  int i = 0;

  if (buffer_circular.buffer[l] != NULL) {

    for (i = 0; i < maxchars; ++i){      
      if (l+i == buffer_circular.escritura) break;

      char buff[0];
      buff[0] = buffer_circular.buffer[(l+i)%256];
      int err = copy_to_user(&buff, b, sizeof(char));
      if (err) break;

      --buffer_circular.items;
      ++b;
    }
    buffer_circular.lectura = (l+i)%256;     
  }

  return i; 
}

void * sys_shmat(int id, void* addr){
  if (id < 0 || id > 9) return -EINVAL;
  if (((unsigned long)addr & 0xfff) == 1) return -EINVAL;
  unsigned long logical_id;
  page_table_entry * process_pt = get_PT(current());

  //int x = acces_ok(CHECK_NOT_DATA, addr, 4096);

  if (!addr == NULL) logical_id = (unsigned long)addr>>12;

  if (addr == NULL || process_pt[logical_id].bits.present) {

    int trobat = 0;
    for (int i = PAG_LOG_INIT_DATA+(2*NUM_PAG_DATA); i < TOTAL_PAGES-1; ++i){ //2pagdata implementacion pocha
      if (process_pt[i].bits.present == 0) {
        logical_id = i;
        trobat = 1;
        break;
      }
    }
    if (!trobat) return -EFAULT;

    set_ss_pag(process_pt, logical_id, shared_vector[id].id_frame);
  }
  else {
    set_ss_pag(process_pt, logical_id, shared_vector[id].id_frame);
  }
  ++shared_vector[id].ref;
  void * ret = (void *)(logical_id<<12);
  
  return ret;
}

int sys_shmdt(void* addr){
  if (((unsigned long)addr & 0xfff) == 1) return -EINVAL;
  if (addr == NULL) return -EINVAL;

  if (access_ok(VERIFY_WRITE,addr,4096)) return -EINVAL;

  unsigned long logical_id = (unsigned long)addr>>12;
  page_table_entry * process_pt = get_PT(current());
  int i;

  for (i = 0; i < 10; ++i){
    if (get_frame(process_pt, logical_id) == shared_vector[i].id_frame){
      --shared_vector[i].ref;
      break;
    }
  }

  if (shared_vector[i].delete && shared_vector[i].ref == 0){
    for (int pos = 0; pos < 4096; ++pos){
      //printk("adsf\n");
      ((char*)addr)[pos] = 0;
    }
  }

  del_ss_pag(process_pt, logical_id);
  set_cr3(get_DIR(current()));

  return 0;
}

int sys_shmrm (int id){

  if (id > 9 || id < 0) return -EINVAL;
  shared_vector[id].delete = 1;
  return 0;

}

int sys_gotoxy(int x, int y){
  if (x > 80 || x < 0) return -1;
  if (y > 25 || y < 0) return -1;

  set_xy(x,y);
}
int sys_set_color(int fg, int bg){
  if (fg > 15 || fg < 0) return -1;
  if (bg > 15 || bg < 0) return -1;

  set_color(fg,bg);
}

int sys_write(int fd, char *buffer, int nbytes) {
char localbuffer [TAM_BUFFER];
int bytes_left;
int ret;

	if ((ret = check_fd(fd, ESCRIPTURA)))
		return ret;
	if (nbytes < 0)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buffer, nbytes))
		return -EFAULT;
	
	bytes_left = nbytes;
	while (bytes_left > TAM_BUFFER) {
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left-=ret;
		buffer+=ret;
	}
	if (bytes_left > 0) {
		copy_from_user(buffer, localbuffer,bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left-=ret;
	}
	return (nbytes-bytes_left);
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{  
  int i;

  page_table_entry *process_PT = get_PT(current());
  // Deallocate all the propietary physical pages

  /* D+S */
  for (i = 0; i < NUM_PAG_DATA; i++) {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }

  /* AFTER D+S */
  for (i=0; i<TOTAL_PAGES; i++)
  {
    int is_shared = 0;
    for (int x = 0; x < 10; ++x) {
      if (get_frame(process_PT, PAG_LOG_INIT_DATA+2*NUM_PAG_DATA+i) == shared_vector[x].id_frame){
        --shared_vector[x].ref;
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+2*NUM_PAG_DATA+i);
      }
    }

    //if (!is_shared) free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+NUM_PAG_DATA+i));
    
  }
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID=-1;
  
  /* Restarts execution of the next process */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}

/*
  SNAKE GAME STARTS HERE
*/

int fruitx = 5;
int fruity = 5;
int snakex = 2;
int snakey = 2;
int altura = 25;
int ancho = 80;
int game_over = 0;
int score = 0;
char scorebuff[10];

void setup(){
  printk("score: ");

  for (int i = 0; i < 80; ++i){
    printc_xy(i,1,'#');
    printc_xy(i,24,'#');
  }  
  for (int i = 1; i < 24; ++i){
    printc_xy(0,i,'#');
    printc_xy(79,i,'#');
  }  
}

void draw(){
  sys_gotoxy(8,0);

  itoa(score,scorebuff);
  printk(scorebuff);

  for (int i = 0; i < altura; ++i){
    for (int j = 0; j < ancho; ++j){
      if (i == snakex && j == snakey){
        printc_xy(i,j,'0');
      }
      else if (i == fruitx && j == fruity){
        printc_xy(i,j,'$');
      }
    }
  }
}

//mirar si se ha pulsado alguna tecla para mover la snake
void take_input(){
  sys_gotoxy(20,0);
  
  char * pos;
  sys_read(pos,1);

  printk(pos);

  if (pos == 's'){
    ++snakex;
  }
  else if (pos == 'd'){
    ++snakey;
  }
  else if (pos == 'w'){
    --snakex;
  }
  else if (pos == 'a'){
    --snakey;
  }

}

//FUNCION PA METER UNA FRUTA EN UNA POSICION (X,Y) RANDOM DEL MAPA (NS COMO)
void spawn_new_fruit(){

}

void movement(){
  
  if (snakex < 0 || snakex > 24 || snakey < 0 || snakey > 79){
    game_over = 1;
  }

  if (snakex == fruitx && snakey == fruity){
    ++score;
    spawn_new_fruit();
  }

}



void sys_the_game(){

  setup();

  while(!game_over){
    draw();
    take_input();
    movement();

  }
}


void sys_test(){}