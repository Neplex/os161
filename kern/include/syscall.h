/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_


#include <cdefs.h> /* for __DEAD */
#include <vnode.h>
#include <limits.h>
struct trapframe; /* from <machine/trapframe.h> */

/*
 * The system call dispatcher.
 */

void syscall(struct trapframe *tf);

/*
 * Support functions.
 */

/* Helper for fork(). You write this. */
void enter_forked_process(struct trapframe *tf);

/* Enter user mode. Does not return. */
__DEAD void enter_new_process(int argc, userptr_t argv, userptr_t env,
		       vaddr_t stackptr, vaddr_t entrypoint);


/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys___time(userptr_t user_seconds, userptr_t user_nanoseconds);

// FILE SYSCALL
#define CONSOLE_NAME "con:"

struct fd {
  char* fd_name;       // Name
  int fd_flags;        // Flags (read/write)
  off_t fd_offset;     // Content pointer
  struct vnode* fd_vn; // VFS node
};

int init_std_file(struct fd* files[], int index);
int init_file_array(struct fd* files[]);

int sys___getcwd(int* r, userptr_t buf, size_t size);
int sys_chdir(userptr_t path);
int sys_open(int* r, userptr_t name, int flags, mode_t mode);
int sys_close(int fd);
int sys_read(int* r, int fd, userptr_t buf, size_t size);
int sys_write(int* r, int fd, userptr_t buf, size_t size);
int sys_lseek(int* r, int fd, off_t offset);
int sys_dup2(int* r, int oldfd, int newfd);

// PROC SYSCALL
struct process {
  pid_t p_parent;
  int p_ecode;
  struct thread* p_thread;
};

struct process* process[PID_MAX];
struct lock* process_lock; // Init in thread bootstrap

int init_process(struct thread* t); // Init PID (call in thread_create)
void delete_process(pid_t pid); // Delete PID (call in waitpid)
void enter_process(void* tf, unsigned long parent);

int sys_getpid(pid_t *r);
int sys__exit(int exit_code);
int sys_fork(pid_t *r, struct trapframe *tf);
int sys_waitpid(pid_t *r, pid_t pid, userptr_t status, int options);
int sys_execv(int* r, userptr_t name, userptr_t args[]);

#endif /* _SYSCALL_H_ */
