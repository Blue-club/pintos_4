#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/palloc.h"


void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f) {
	// TODO: Your implementation goes here.
	check_address (f->rsp);
	int syscall_number = f->R.rax;

	switch (syscall_number) {
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			exit(f->R.rdi);
			break;
		case SYS_FORK:
			fork(f->R.rdi);
			break;
		case SYS_EXEC:
			exec(f->R.rdi);
			break;
		case SYS_WAIT:
			wait(f->R.rdi);
			break;
		case SYS_CREATE:
			create(f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			remove(f->R.rdi);
			break;	
		case SYS_OPEN:
			open(f->R.rdi);		
			break;
		case SYS_FILESIZE:
			filesize(f->R.rdi);
			break;
		case SYS_READ:
			read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			seek(f->R.rdi, f->R.rsi);
			break;	
		case SYS_TELL:
			tell(f->R.rdi);
			break;
		case SYS_CLOSE:
			close(f->R.rdi);
			break;
	}

	// printf ("system call!\n");
	// thread_exit ();
}

void
check_address(void *addr) {
	// TODO: If you encounter an invalid user pointer afterward, you must still be sure to release the lock or free the page of memory.
	struct thread *t = thread_current();

	if (is_kernel_vaddr(addr) || addr == NULL || pml4_get_page(t->pml4, addr) == NULL) {
		exit(-1);
	}
}

void
halt (void) {
	power_off();
}

void
exit (int status) {
	struct thread *t = thread_current();
	printf("%s: exit(%d)\n", t->name, status);
	thread_exit();
}

pid_t
fork (const char *thread_name) {

}

int exec(const char *cmd_line)
{
	check_address(cmd_line);

	// process.c 파일의 process_create_initd 함수와 유사하다.
	// 단, 스레드를 새로 생성하는 건 fork에서 수행하므로
	// 이 함수에서는 새 스레드를 생성하지 않고 process_exec을 호출한다.

	// process_exec 함수 안에서 filename을 변경해야 하므로
	// 커널 메모리 공간에 cmd_line의 복사본을 만든다.
	// (현재는 const char* 형식이기 때문에 수정할 수 없다.)
	char *cmd_line_copy;
	cmd_line_copy = palloc_get_page(PAL_ZERO);
	if (cmd_line_copy == NULL)
		exit(-1);							  // 메모리 할당 실패 시 status -1로 종료한다.
	strlcpy(cmd_line_copy, cmd_line, PGSIZE); // cmd_line을 복사한다.

	// 스레드의 이름을 변경하지 않고 바로 실행한다.
	if (process_exec(cmd_line_copy) == -1)
		exit(-1); // 실패 시 status -1로 종료한다.
}

int
wait (pid_t pid) {
	process_wait ();
}

bool
create (const char *file, unsigned initial_size) {
	check_address (file);
	return filesys_create (file, initial_size);
}

bool
remove (const char *file) {
	check_address (file);
	return filesys_remove (file);
}

int
open (const char *file) {
	check_address (file);
	return filesys_open (file);
}

int
filesize (int fd) {

}

int
read (int fd, void *buffer, unsigned size) {

}

int
write (int fd, const void *buffer, unsigned size) {
	if (fd == 1) {
		putbuf (buffer, size);
	}
	return size;
}

void
seek (int fd, unsigned position) {

}

unsigned
tell (int fd) {

}

void
close (int fd) {

}