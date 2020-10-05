#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

struct lock fslock;

void syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  //initialize file system lock
  lock_init(&fslock);
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
      halt();
      break;
    case SYS_EXIT:
      if(!valid(p+1)) kill();
      exit(*(p+1));
      break
    case SYS_EXEC:
      if(!valid(p+1) || !valid(*(p+1))) kill();

    case SYS_WAIT:
    case SYS_CREATE:
    case 
  }
}


syscall_handler(struct intr_frame *f){

}

