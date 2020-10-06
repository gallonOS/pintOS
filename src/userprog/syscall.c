#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "list.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
void* check_addr(const void*);
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

	check_addr(p);



  int system_call = * p;
	switch (system_call)
	{
		// Shuts down computer
		case SYS_HALT:
		shutdown_power_off();
		break;

		// Exiting process
		case SYS_EXIT:
		check_addr(p+1);
		exit_proc(*(p+1));
		break;

		// Executing process
		case SYS_EXEC:
		check_addr(p+1);
		check_addr(*(p+1));
		f->eax = exec_proc(*(p+1));
		break;

		// Waits for a child process pid and retrieves the child's exit status.
		case SYS_WAIT:
		check_addr(p+1);
		f->eax = process_wait(*(p+1));
		break;

		// Creates a new file called file initially initial_size bytes in size.
		//Returns true if successful, false otherwise. Creating a new file does not open it:
		//opening the new file is a separate operation which would require a open system call.
		case SYS_CREATE:
		check_addr(p+5);
		check_addr(*(p+4));
		acquire_filesys_lock();
		f->eax = filesys_create(*(p+4),*(p+5));
		release_filesys_lock();
		break;

		//Deletes the file called file. Returns true if successful, false otherwise.
	//A file may be removed regardless of whether it is open or closed, and removing an open file does not close it.
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

		// Get access to the file system
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

		// Gets the file size
		case SYS_FILESIZE:
		check_addr(p+1);
		acquire_filesys_lock();
		f->eax = file_length (list_search(&thread_current()->files, *(p+1))->ptr);
		release_filesys_lock();
		break;

		//Reads size bytes from the file open as fd into buffer.
		//Returns the number of bytes actually read (0 at end of file),
		//or -1 if the file could not be read (due to a condition other than end of file).
		//Fd 0 reads from the keyboard using input_getc().
		case SYS_READ:
		check_addr(p+7);
		check_addr(*(p+6));
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

		//Writes size bytes from buffer to the open file fd.
		//Returns the number of bytes actually written,
		//which may be less than size if some bytes could not be written.
		case SYS_WRITE:
		check_addr(p+7);
		check_addr(*(p+6));
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

		//Changes the next byte to be read or written in open file fd to position,
		//expressed in bytes from the beginning of the file. (Thus, a position of 0 is the file's start.)
		case SYS_SEEK:
		check_addr(p+5);
		acquire_filesys_lock();
		file_seek(list_search(&thread_current()->files, *(p+4))->ptr,*(p+5));
		release_filesys_lock();
		break;

		//Returns the position of the next byte to be read or written in open file fd,
		//expressed in bytes from the beginning of the file.
		case SYS_TELL:
		check_addr(p+1);
		acquire_filesys_lock();
		f->eax = file_tell(list_search(&thread_current()->files, *(p+1))->ptr);
		release_filesys_lock();
		break;

		//Closes file descriptor fd. Exiting or terminating a process implicitly
		case SYS_CLOSE:
		check_addr(p+1);
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

void exit_proc(int status)
{
	//printf("Exit : %s %d %d\n",thread_current()->name, thread_current()->tid, status);
	struct list_elem *e;

      for (e = list_begin (&thread_current()->parent->child_proc); e != list_end (&thread_current()->parent->child_proc);
           e = list_next (e))
        {
          struct child *f = list_entry (e, struct child, elem);
          if(f->tid == thread_current()->tid)
          {
          	f->used = true;
          	f->exit_error = status;
          }
        }


	thread_current()->exit_error = status;

	if(thread_current()->parent->waitingon == thread_current()->tid)
		sema_up(&thread_current()->parent->child_lock);

	thread_exit();
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

//Iterates through list to find match
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
