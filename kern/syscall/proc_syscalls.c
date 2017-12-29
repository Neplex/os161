#include <types.h>
#include <copyinout.h>
#include <current.h>
#include <proc.h>
#include <synch.h>
#include <syscall.h>
#include <kern/errno.h>
#include <mips/trapframe.h>

int init_process(struct thread* t)
{
  pid_t pid = PID_MIN;
  struct process* proc;

  // TODO: lock issue
  // lock_acquire(process_lock);

  // Get PID
  while (process[pid] != NULL) { pid++; }
  if (pid >= PID_MAX) { return EMPROC; }

  proc = kmalloc(sizeof(struct process));

  t->t_pid = pid;
  proc->p_parent = -1;
  proc->p_ecode = 0;
  proc->p_thread = t;

  // Save process
  process[pid] = proc;

  // lock_release(process_lock);

  return 0;
}

void delete_process(pid_t pid)
{
  // lock_acquire(process_lock);

  kfree(process[pid]);
  process[pid] = NULL;

  // lock_release(process_lock);
}

int sys_getpid(int *r)
{
  *r = curthread->t_pid;

  return 0;
}

int sys__exit(int exit_code)
{
  process[curthread->t_pid]->p_ecode = exit_code; // Save exit code
  thread_exit();

  return 0;
}

int sys_fork(pid_t *r, struct trapframe *tf)
{
  int result;

  result = thread_fork(curthread->t_name, NULL, enter_process, tf, curthread->t_pid);
  if (result) { return result; }

  *r = 0; // TODO: Return child PID in parent

  return ENOSYS;
}

void enter_process(void* tf, unsigned long parent)
{
  struct trapframe new_tf;

  // Copy parent
  memcpy(&new_tf, (struct trapframe*)tf, sizeof(struct trapframe));

  process[curthread->t_pid]->p_parent = (pid_t) parent;

  mips_usermode(&new_tf);
}

int sys_waitpid(pid_t *r, pid_t pid, userptr_t status, int options)
{
  int result;

  // Valid arguments
  if(options!=0 || status==NULL) { return EINVAL; }

  // Valid PID
  if (pid<PID_MIN || pid>=PID_MAX || process[pid]==NULL) { return ESRCH; }

  // Child process
  if (process[pid]->p_parent != curthread->t_pid) { return ECHILD; }

  // TODO: The child process may not be completed. In this case, we must wait.
  result = copyout(&process[pid]->p_ecode, status, sizeof(userptr_t));
  if (result) { return result; }

  *r = pid;
  delete_process(pid); // He can die now

  return 0;
}

int sys_execv(int* r, userptr_t name, userptr_t args[])
{
  *r=0;
  name=(userptr_t)name;
  args=(userptr_t *)args;

  // TODO: ececv

  return ENOSYS;
}
