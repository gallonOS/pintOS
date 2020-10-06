#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "list.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
void* valid(const void*);
struct proc_file* list_search(struct list* files, int fd);

extern bool running;

struct proc_file {
	struct file* ptr;
	int fd;
	struct list_elem elem;
};

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * p = f->esp;

	valid(p);



  int system_call = * p;
	switch (system_call)
	{
		// Shuts down computer
		case SYS_HALT:
		shutdown_power_off();
		break;

		// Exiting process
		case SYS_EXIT:
		valid(p+1);
		exit(*(p+1));
		break;

		// Executing process
		case SYS_EXEC:
		valid(p+1);
		valid(*(p+1));
		f->eax = exec_proc(*(p+1));
		break;

		// Making the process wait
		case SYS_WAIT:
		valid(p+1);
		f->eax = process_wait(*(p+1));
		break;

		// Create new process
		case SYS_CREATE:
		valid(p+5);
		valid(*(p+4));
		acquire_filesys_lock();
		f->eax = filesys_create(*(p+4),*(p+5));
		release_filesys_lock();
		break;

		// Remove file system
		case SYS_REMOVE:
		valid(p+1);
		valid(*(p+1));
		acquire_filesys_lock();
		if(filesys_remove(*(p+1))==NULL)
			f->eax = false;
		else
			f->eax = true;
		release_filesys_lock();
		break;

		// Get access to the file system
		case SYS_OPEN:
		valid(p+1);
		valid(*(p+1));

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

		// Gets the file size
		case SYS_FILESIZE:
		valid(p+1);
		acquire_filesys_lock();
		f->eax = file_length (list_search(&thread_current()->files, *(p+1))->ptr);
		release_filesys_lock();
		break;

		// Getting file and reading it
		case SYS_READ:
		valid(p+7);
		valid(*(p+6));
		if(*(p+5)==0)
		{
			int i;
			uint8_t* buffer = *(p+6);
			for(i=0;i<*(p+7);i++)
				buffer[i] = input_getc();
			f->eax = *(p+7);
		}
		else
		{
			struct proc_file* fptr = list_search(&thread_current()->files, *(p+5));
			if(fptr==NULL)
				f->eax=-1;
			else
			{
				acquire_filesys_lock();
				f->eax = file_read (fptr->ptr, *(p+6), *(p+7));
				release_filesys_lock();
			}
		}
		break;

		// Getting file and writing to it
		case SYS_WRITE:
		valid(p+7);
		valid(*(p+6));
		if(*(p+5)==1)
		{
			putbuf(*(p+6),*(p+7));
			f->eax = *(p+7);
		}
		else
		{
			struct proc_file* fptr = list_search(&thread_current()->files, *(p+5));
			if(fptr==NULL)
				f->eax=-1;
			else
			{
				acquire_filesys_lock();
				f->eax = file_write (fptr->ptr, *(p+6), *(p+7));
				release_filesys_lock();
			}
		}
		break;

		//Looking for a file
		case SYS_SEEK:
		valid(p+5);
		acquire_filesys_lock();
		file_seek(list_search(&thread_current()->files, *(p+4))->ptr,*(p+5));
		release_filesys_lock();
		break;

		// Gets current position of the file
		case SYS_TELL:
		valid(p+1);
		acquire_filesys_lock();
		f->eax = file_tell(list_search(&thread_current()->files, *(p+1))->ptr);
		release_filesys_lock();
		break;

		// Close files
		case SYS_CLOSE:
		valid(p+1);
		acquire_filesys_lock();
		close_file(&thread_current()->files,*(p+1));
		release_filesys_lock();
		break;


		default:
		printf("Default %d\n",*p);
	}
}

int exec_proc(char *file_name)
{
	acquire_filesys_lock();
	char * fn_cp = malloc (strlen(file_name)+1);
	  strlcpy(fn_cp, file_name, strlen(file_name)+1);
	  
	  char * save_ptr;
	  fn_cp = strtok_r(fn_cp," ",&save_ptr);

	 struct file* f = filesys_open (fn_cp);

	  if(f==NULL)
	  {
	  	release_filesys_lock();
	  	return -1;
	  }
	  else
	  {
	  	file_close(f);
	  	release_filesys_lock();
	  	return process_execute(file_name);
	  }
}
//terminates current user program
void exit(int status)
{
	//printf("Exit : %s %d %d\n",thread_current()->name, thread_current()->tid, status);
	struct list_elem *e;
      for (e = list_begin (&thread_current()->parent->child_proc); e != list_end (&thread_current()->parent->child_proc); e = list_next (e)){
          struct child *f = list_entry (e, struct child, elem);
          if(f->tid == thread_current()->tid){
          	f->used = true;
          	f->exit_error = status;
          }
        }
	//returns the error status 
	thread_current()->exit_error = status;
	//waiting on parent 
	if(thread_current()->parent->waitingon == thread_current()->tid)
		sema_up(&thread_current()->parent->child_lock);
	// exits thread
	thread_exit();
}
// validates the address
void* valid(const void *vaddr){
	//if invalid address
	if (!is_user_vaddr(vaddr)){
		exit(-1);
		return 0;
	}
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	//if page is invalid
	if (!ptr){
		exit(-1);
		return 0;
	}
	return ptr;
}

struct proc_file* list_search(struct list* files, int fd)
{

	struct list_elem *e;

      for (e = list_begin (files); e != list_end (files);
           e = list_next (e))
        {
          struct proc_file *f = list_entry (e, struct proc_file, elem);
          if(f->fd == fd)
          	return f;
        }
   return NULL;
}

void close_file(struct list* files, int fd)
{

	struct list_elem *e;

	struct proc_file *f;

      for (e = list_begin (files); e != list_end (files);
           e = list_next (e))
        {
          f = list_entry (e, struct proc_file, elem);
          if(f->fd == fd)
          {
          	file_close(f->ptr);
          	list_remove(e);
          }
        }

    free(f);
}

void close_all_files(struct list* files)
{

	struct list_elem *e;

	while(!list_empty(files))
	{
		e = list_pop_front(files);

		struct proc_file *f = list_entry (e, struct proc_file, elem);
          
	      	file_close(f->ptr);
	      	list_remove(e);
	      	free(f);


	}

      
}