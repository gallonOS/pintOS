#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
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

