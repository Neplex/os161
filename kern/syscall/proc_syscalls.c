#include <types.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <limits.h>
#include <kern/errno.h>
#include <copyinout.h>

int sys_getpid(int *r)
{
  *r=0;

  return ENOSYS;
}

int sys__exit(int exit_code)
{
  exit_code=(int)exit_code;

  thread_exit();
  return ENOSYS;
}

int sys_fork(pid_t *r, struct trapframe *tf)
{
  *r=0;
  tf=(struct trapframe*)tf;

  return ENOSYS;
}

int sys_waitpid(pid_t *r, pid_t pid, userptr_t status, int options)
{
  *r=0;
  pid=(pid_t)pid;
  status=(userptr_t)status;
  options=(int)options;

  return ENOSYS;
}

int sys_execv(int* r, userptr_t name, userptr_t args[])
{
  *r=0;
  name=(userptr_t)name;
  args=(userptr_t *)args;

  return ENOSYS;
}
