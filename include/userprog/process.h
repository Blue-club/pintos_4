#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd (const char *);
tid_t process_fork (const char *, struct intr_frame *);
int process_exec (void *);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *);
void argument_stack(char **, int, struct intr_frame *);
int process_add_file(struct file *f);
struct file *process_get_file(int fd);
void process_close_file(int fd);
struct thread *get_child_process(int);

#endif /* userprog/process.h */