#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/init.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

struct lock fslock;

void syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  //initialize file system lock
  lock_init(&fslock);
}

void* check_addr(const void *vaddr)
{
	if (!is_user_vaddr(vaddr))
	{
		exit_proc(-1);
		return 0;
	}
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if (!ptr)
	{
		exit_proc(-1);
		return 0;
	}
	return ptr;
}

void halt(void){
  shutdown_power_off();
}

void exit(int status){
  //termintate current user program
  struct thread *curr= thread_current();
  if(active_thread(curr->parent)) cur->cp->status = status;
  printf("%s: exit(%d)\n", curr->name,status);
  thread_exit(); 
}

int wait(pid_t pid){
  return process_wait(pid);
}
bool create(const char*file, unsigned initial_size){
  if(file == NULL) return -1;
  lock_aquire(&fs_lock);
  bool status = filesys_create(file, initial_size);
  lock_release(&fs_lock);
  return status;
}
int filesize (int fd){
  int size;
  lock_aquire(&fslock);
  struct file *file = process_get_file(fd);
  if(!file){
    lock_release(&fslock);
    return -1;
  }
  size = file_length(file);
  lock_release(&fslock);
  return size;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
    //the saved stack
  int *p = f->esp;
  //if the stack pointer is invalid 
  if(!valid(p))kill(); 
  int syscall_number = *p;
  switch(syscall_number)
  {
    case SYS_HALT:
      shutdown_power_off();
      break;
    case SYS_EXIT:
	check_addr(p+1);
	exit_proc(*(p+1));
	break;
    case SYS_EXEC:
	check_addr(p+1);
	check_addr(*(p+1));
	f->eax = exec_proc(*(p+1));
	break;

    case SYS_WAIT:
	check_addr(p+1);
	f->eax = process_wait(*(p+1));
	break;

    case SYS_CREATE:
	check_addr(p+5);
	check_addr(*(p+4));
	acquire_filesys_lock();
	f->eax = filesys_create(*(p+4),*(p+5));
	release_filesys_lock();
	break;
    case SYS_REMOVE:
	check_addr(p+1);
	check_addr(*(p+1));
	acquire_filesys_lock();
	if(filesys_remove(*(p+1))==NULL)
		f->eax = false;
	else
		f->eax = true;
	release_filesys_lock();
	break;

    case SYS_OPEN:
	check_addr(p+1);
	check_addr(*(p+1));

	acquire_filesys_lock();
	struct file* fptr = filesys_open (*(p+1));
	release_filesys_lock();
	if(fptr==NULL)
		f->eax = -1;
	else
	{
		struct proc_file *pfile = malloc(sizeof(*pfile));
		pfile->ptr = fptr;
		pfile->fd = thread_current()->fd_count;
		thread_current()->fd_count++;
		list_push_back (&thread_current()->files, &pfile->elem);
		f->eax = pfile->fd;

	}
	break;
  }
}
bool valid()
void getargs(struct intr_frame *f, int *arg, int n){ 
  int *p;
  for (int i = 0; i < n; i++){
    p=f->esp + i + 1;
    check_vaddr((const void *)p);
  }
}

