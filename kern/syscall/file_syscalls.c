#include <types.h>
#include <current.h>
#include <proc.h>
#include <uio.h>
#include <vfs.h>
#include <vnode.h>
#include <syscall.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <copyinout.h>

/* Create std file (stdin, stdout, stderr) */
int init_std_file(struct fd* files[], int index)
{
  int result;
  char name[5] = CONSOLE_NAME;
  struct vnode* vn;
  struct fd* fd;

  result = vfs_open(name, index, 0664, &vn);
  if (result) { return result; }

  fd = kmalloc(sizeof(struct fd));
  fd->fd_name = name;
  fd->fd_flags = index;
  fd->fd_offset = 0;
  fd->fd_vn = vn;
  files[index] = fd;

  return 0;
}

/* Init current thread file array */
int init_file_array(struct fd* files[])
{
  int result, i;

  for (i=0; i<OPEN_MAX; i++) {
    files[i] = NULL;
  }

  // STDIN
  result = init_std_file(files, STDIN_FILENO);
  if (result) { return result; }

  // STDOUT
  result = init_std_file(files, STDOUT_FILENO);
  if (result) { return result; }

  // STDERR
  result = init_std_file(files, STDERR_FILENO);
  if (result) { return result; }

  return 0;
}

int sys___getcwd(int *r, userptr_t buf, size_t size)
{
    struct iovec v;
    struct uio u;
    int ret;

    v.iov_ubase=buf;
    v.iov_len=size;
    u.uio_iov=&v;
    u.uio_iovcnt=1;
    u.uio_offset=0;
    u.uio_resid=size;
    u.uio_segflg=UIO_USERSPACE;
    u.uio_space=curthread->t_proc->p_addrspace;
    u.uio_rw=UIO_READ;
    ret=vfs_getcwd(&u);
    *r = ret ? -1 : u.uio_offset;
    return ret;
}

int sys_chdir(userptr_t path)
{
  int result;
  size_t count;
  char kpath[PATH_MAX];

  result=copyinstr((const_userptr_t)path, kpath, PATH_MAX, &count);
  if (result) { return result; }

  return vfs_chdir(kpath);
}

int sys_open(int* r, userptr_t name, int flags, mode_t mode)
{
  int result, i=3;
  struct vnode* vn;
  struct fd* fd;
  *r = -1;

  // Get first free file descriptor
  while (curthread->files[i] != NULL) { i++; }
  if (i >= OPEN_MAX) { return EMFILE; }

  size_t count;
  char kname[NAME_MAX];

  result=copyinstr((const_userptr_t)name, kname, NAME_MAX, &count);
  if (result) { return result; }

  // Open file
  result = vfs_open(kname, flags, mode, &vn);
  if (result) { return result; }

  fd = kmalloc(sizeof(struct fd));
  fd->fd_name = kname;
  fd->fd_flags = flags;
  fd->fd_offset = 0;
  fd->fd_vn = vn;
  curthread->files[i] = fd;
  *r = i;

  return 0;
}

int sys_close(int fd)
{
  // Valid file descriptor
  if (fd<0 || fd>=OPEN_MAX || curthread->files[fd]==NULL) { return EBADF; }

  vfs_close(curthread->files[fd]->fd_vn);
  kfree(curthread->files[fd]);
  curthread->files[fd] = NULL;

  return 0;
}

int sys_read(int* r, int fd, userptr_t buf, size_t size)
{
  int result, flags;
  struct iovec v;
  struct uio u;

  // Valid file descriptor
  if (fd<0 || fd>=OPEN_MAX || curthread->files[fd]==NULL) { return EBADF; }

  // Write only
  flags = curthread->files[fd]->fd_flags;
  if ((flags&O_ACCMODE) != O_RDWR && (flags&O_ACCMODE) != O_RDONLY) { return EROFS; }

  v.iov_ubase = buf;
  v.iov_len = size;
  u.uio_iov = &v;
  u.uio_iovcnt = 1;
  u.uio_offset = curthread->files[fd]->fd_offset;
  u.uio_resid = size;
  u.uio_segflg = UIO_USERSPACE;
  u.uio_space = curthread->t_proc->p_addrspace;
  u.uio_rw = UIO_READ;

  result = VOP_READ(curthread->files[fd]->fd_vn, &u);

  if (result) {
    *r = -1;
  } else {
    *r = u.uio_offset;
    curthread->files[fd]->fd_offset += u.uio_offset;
  }

  return result;
}

int sys_write(int* r, int fd, userptr_t buf, size_t size)
{
  int result, flags;
  struct iovec v;
  struct uio u;

  // Valid file descriptor
  if (fd<0 || fd>=OPEN_MAX || curthread->files[fd]==NULL) { return EBADF; }

  // Read only
  flags = curthread->files[fd]->fd_flags;
  if ((flags&O_ACCMODE) != O_RDWR && (flags&O_ACCMODE) != O_WRONLY) { return EROFS; }

  v.iov_ubase = buf;
  v.iov_len = size;
  u.uio_iov = &v;
  u.uio_iovcnt = 1;
  u.uio_offset = curthread->files[fd]->fd_offset;
  u.uio_resid = size;
  u.uio_segflg = UIO_USERSPACE;
  u.uio_space = curthread->t_proc->p_addrspace;
  u.uio_rw = UIO_WRITE;

  result = VOP_WRITE(curthread->files[fd]->fd_vn, &u);

  if (result) {
    *r = -1;
  } else {
    *r = u.uio_offset;
    curthread->files[fd]->fd_offset += u.uio_offset;
  }

  return result;
}

int sys_lseek(int* r, int fd, off_t offset)
{
  // Valid file descriptor
  if (fd<0 || fd>=OPEN_MAX || curthread->files[fd]==NULL) { return EBADF; }

  if (offset < 0) { offset = 0; }
  // TODO: if offset upper than file length set file length
  *r = curthread->files[fd]->fd_offset = offset;

  return 0;
}

int sys_dup2(int* r, int oldfd, int newfd)
{
  int result;

  // Valid file descriptor
  if (oldfd<0 || oldfd>=OPEN_MAX || curthread->files[oldfd]==NULL) { return EBADF; }
  if (newfd<0 || newfd>=OPEN_MAX) { return EBADF; }

  // If it same do nothing
  if (oldfd != newfd)
  {
    // Close newfd
    if (curthread->files[newfd]) {
      result = sys_close(newfd);
      if (result) { return result; }
    }

    // Copy file descriptor
    curthread->files[newfd] = curthread->files[oldfd];
  }
  *r = newfd;

  return 0;
}
