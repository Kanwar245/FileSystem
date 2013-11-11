/* BEGIN A3 SETUP */
/* This file existed for A1 and A2, but has been completely replaced for A3.
 * We have kept the dumb versions of sys_read and sys_write to support early
 * testing, but they should be replaced with proper implementations that 
 * use your open file table to find the correct vnode given a file descriptor
 * number.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>
#include <vfs.h>
#include <vnode.h>
#include <uio.h>
#include <kern/fcntl.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <copyinout.h>
#include <synch.h>
#include <file.h>
#include <kern/seek.h>

/* This special-case for the console vnode should go away with a proper 
 * open file table implementation.
 */

struct vnode *cons_vnode=NULL;

void dumb_consoleIO_bootstrap()
{
  int result;
  char path[5];

  /* The path passed to vfs_open must be mutable.
   * vfs_open may modify it.
   */

  strcpy(path, "con:");
  result = vfs_open(path, O_RDWR, 0, &cons_vnode);

  if (result) {
    /* Tough one... if there's no console, there's not
     * much point printing a warning...
     * but maybe the bootstrap was just called in the wrong place
     */
    kprintf("Warning: could not initialize console vnode\n");
    kprintf("User programs will not be able to read/write\n");
    cons_vnode = NULL;
  }
}

/*
 * mk_useruio
 * sets up the uio for a USERSPACE transfer. 
 */
static
void
mk_useruio(struct iovec *iov, struct uio *u, userptr_t buf, 
	   size_t len, off_t offset, enum uio_rw rw)
{
       // DEBUG(DB_SFS, "mk uio entry \n");    
    
	iov->iov_ubase = buf;
	iov->iov_len = len;
	u->uio_iov = iov;
	u->uio_iovcnt = 1;
	u->uio_offset = offset;
	u->uio_resid = len;
	u->uio_segflg = UIO_USERSPACE;
	u->uio_rw = rw;
	u->uio_space = curthread->t_addrspace;
}

/*
 * sys_open
 * just copies in the filename, then passes work to file_open.
 * You have to write file_open.
 * 
 */
int
sys_open(userptr_t filename, int flags, int mode, int *retval)
{
        //DEBUG(DB_SFS, "open entry \n");
	char *fname;
	int result;
       

	if ( (fname = (char *)kmalloc(__PATH_MAX)) == NULL) {
		return ENOMEM;
	}

	result = copyinstr(filename, fname, __PATH_MAX, NULL);
	if (result) {
		kfree(fname);
		return result;
	}

	result =  file_open(fname, flags, mode, retval);
	kfree(fname);
	return result;
}

/* 
 * sys_close
 * You have to write file_close.
 */
int
sys_close(int fd)
{
        //DEBUG(DB_SFS, "close entry \n");
	return file_close(fd);
}

/* 
 * sys_dup2
 * 
 */
int
sys_dup2(int oldfd, int newfd, int *retval)
{
    DEBUG(DB_SFS, "dup2 entry \n");
    int result;
    struct openfile *of;

    // get the old file
    result = filetable_findfile(oldfd, &of);
    if (result) {
        return result;
    }

    if (newfd < 0 || newfd >= __OPEN_MAX || oldfd < 0 || oldfd >= __OPEN_MAX) {
        return EBADF;
    }

    // if newfd is open, close it
    if (curthread->t_filetable->ft_openfiles[newfd] != NULL) {
        file_close(newfd);
    }

    curthread->t_filetable->ft_openfiles[newfd] = of;

    // increment the ref count
    lock_acquire(of->of_lock);
    of->of_refcount++;
    lock_release(of->of_lock);

    *retval = newfd;
    return 0;
}

/*
 * sys_read
 * calls VOP_READ.
 * 
 * A3: This is the "dumb" implementation of sys_write:
 * it only deals with file descriptors 1 and 2, and 
 * assumes they are permanently associated with the 
 * console vnode (which must have been previously initialized).
 *
 * In your implementation, you should use the file descriptor
 * to find a vnode from your file table, and then read from it.
 *
 * Note that any problems with the address supplied by the
 * user as "buf" will be handled by the VOP_READ / uio code
 * so you do not have to try to verify "buf" yourself.
 *
 * Most of this code should be replaced.
 */
int
sys_read(int fd, userptr_t buf, size_t size, int *retval)
{
        //DEBUG(DB_SFS, "sys_read entry \n");
	struct uio user_uio;
	struct iovec user_iov;
	int result;
	off_t offset;
    struct openfile *ofile;
                
        
	/* Check if fd is within bounds */
	if (fd < 0 || fd >= __OPEN_MAX) {
	  return EBADF;
	}
        
    // Find open file in file table
    if ((result = filetable_findfile(fd, &ofile))) {
        DEBUG(DB_SFS, "Cannot find open file in filetable\n");
        return result;
    }
        
    // Lock open file
    lock_acquire(ofile->of_lock);
     
    // Get the offset 
    offset = ofile->of_offset;

	/* set up a uio with the buffer, its size, and the current offset */
	mk_useruio(&user_iov, &user_uio, buf, size, offset, UIO_READ);

	/* does the read */
	result = VOP_READ(ofile->of_vn, &user_uio);
	if (result) {
        // Release lock for open file
        lock_release(ofile->of_lock);
		return result;
	}

	/*
	 * The amount read is the size of the buffer originally, minus
	 * how much is left in it.
	 */
	*retval = size - user_uio.uio_resid;
        
    // Move offset to new position
    ofile->of_offset += size;
        
    // Release lock for open file
    lock_release(ofile->of_lock);

	return 0;
}

/*
 * sys_write
 * calls VOP_WRITE.
 *
 * A3: This is the "dumb" implementation of sys_write:
 * it only deals with file descriptors 1 and 2, and 
 * assumes they are permanently associated with the 
 * console vnode (which must have been previously initialized).
 *
 * In your implementation, you should use the file descriptor
 * to find a vnode from your file table, and then read from it.
 *
 * Note that any problems with the address supplied by the
 * user as "buf" will be handled by the VOP_READ / uio code
 * so you do not have to try to verify "buf" yourself.
 *
 * Most of this code should be replaced.
 */

int
sys_write(int fd, userptr_t buf, size_t len, int *retval) 
{
    //DEBUG(DB_SFS, "write entry \n");
    struct uio user_uio;
    struct iovec user_iov;
    int result;
	off_t offset;
    struct openfile *ofile;
  
    /* Check if fd is within bounds */
    if (fd < 0 || fd >= __OPEN_MAX) {
        return EBADF;
    }
        
    // Find open file in file table
     if ((result = filetable_findfile(fd, &ofile))) {
        DEBUG(DB_SFS, "Cannot find open file in filetable\n");
        return result;
    }
        
        
    // Lock open file
    lock_acquire(ofile->of_lock);
    
    // Get the offset 
    offset = ofile->of_offset;

    /* set up a uio with the buffer, its size, and the current offset */
    mk_useruio(&user_iov, &user_uio, buf, len, offset, UIO_WRITE);

    /* does the write */
    result = VOP_WRITE(ofile->of_vn, &user_uio);
    if (result) {
        // Release lock for open file
        lock_release(ofile->of_lock);
        return result;
    }
    
    // Move offset to new position
    ofile->of_offset += len;
    
    /*
     * the amount written is the size of the buffer originally,
     * minus how much is left in it.
     */
    *retval = len - user_uio.uio_resid;
    
    // Release lock for open file
    lock_release(ofile->of_lock);

    return 0;
}

/*
 * sys_lseek
 * 
 */
int
sys_lseek(int fd, off_t offset, int whence, off_t *retval)
{     
    DEBUG(DB_SFS, "lseek entry \n");
    
    /*int result;
    struct openfile *ofile;
    struct stat *stat_info;
    off_t retoffset;
    
    // Check if fd is valid
    if (fd < 0 || fd >= __OPEN_MAX) {
      return EBADF;
    }
    
    // Find open file in file table
    if ((result = filetable_findfile(fd, &ofile))) {
        DEBUG(DB_SFS, "Cannot find open file in filetable\n");
        return result;
    }

     // Lock open file
    lock_acquire(ofile->of_lock);
    
    switch(whence) {
        // the new position is pos
        case SEEK_SET:
            retoffset = offset;
        break;
        
        // the new position is the current position plus pos
        case SEEK_CUR:
            retoffset = ofile->of_offset + offset;
        break;
        
        // the new position is the position of end-of-file plus pos
        case SEEK_END:
            // get vnode stat info to get size of file
            if ((result = VOP_STAT(ofile->of_vn, stat_info))) {
                // Release open file
                lock_release(ofile->of_lock);
                DEBUG(DB_SFS, "VOP_STAT failure\n");
                return result;
            }                
            retoffset = offset + stat_info->st_size;
    }
    
    // Check if offset is valid
    if ((result = VOP_TRYSEEK(ofile->of_vn, retoffset))) {
        // Release open file
        lock_release(ofile->of_lock);
        return result;
    }
    
    // Set open file's offset and retval to the new offset
    ofile->of_offset = retoffset;
    *retval = retoffset;

    // Release open file
    lock_release(ofile->of_lock);
    
    return 0;*/
    int result;
    struct openfile *of;
    off_t newpos;
    struct stat s;

    // get the file from file table
    result = filetable_findfile(fd, &of);
    if (result)
        return result;

    // lock the file
    lock_acquire(of->of_lock);

    // check the type
    switch (whence) {
        // new position = offset
        case SEEK_SET:
            newpos = offset;
            break;
        // new position = current position + offset
        case SEEK_CUR:
            newpos = of->of_offset + offset;
            break;
        // new position = the position of the end of the file + offset
        case SEEK_END:
            result = VOP_STAT(of->of_vn, &s);
            if (result) {
                lock_release(of->of_lock);
                return result;
            }
            break;
        default:
            // invalid seek type
            lock_release(of->of_lock);
            return EINVAL;
    }

    if (newpos < 0) {
        lock_release(of->of_lock);
        return EINVAL;
    }

    // make the file use newpos
    result = VOP_TRYSEEK(of->of_vn, newpos);
    if (result) {
        lock_release(of->of_lock);
        return result;
    }

    of->of_offset = newpos;
    *retval = newpos;

    lock_release(of->of_lock);

    return 0;
}


/* really not "file" calls, per se, but might as well put it here */

/*
 * sys_chdir
 * 
 */
int
sys_chdir(userptr_t path)
{
    
    DEBUG(DB_SFS, "chdir entry \n");
    
    char *pathname;
    int result;
    
    if (((pathname = (char *)kmalloc(__PATH_MAX)) == NULL)) {
        return ENOMEM;
    }
        
	if ((result = copyinstr(path, pathname, __PATH_MAX, NULL))) {
		kfree(pathname);
		return result;
	}
       
    if ((result = vfs_chdir(pathname))) {
        return result;
    }
     
    return 0;
}

/*
 * sys___getcwd
 * 
 */
int
sys___getcwd(userptr_t buf, size_t buflen, int *retval)
{
    DEBUG(DB_SFS, "getcwd entry \n");

    struct uio user_uio;
    struct iovec user_iov;
    int result;

    // set up the uio with buf
    mk_useruio(&user_iov, &user_uio, buf, buflen, 0, UIO_READ);

    result = vfs_getcwd(&user_uio);
    if (result) {
        return result;
    }

    // get the size of the remaining buf data
    *retval = buflen - user_uio.uio_resid;

    return 0;
}

/*
 * sys_fstat
 */
int
sys_fstat(int fd, userptr_t statptr)
{
    DEBUG(DB_SFS, "fstat entry \n");
    int result;
    struct iovec user_iov;
    struct openfile *ofile;
    struct stat stat_info;
    struct uio user_uio;

    /// Check if fd is valid
    if (fd < 0 || fd >= __OPEN_MAX) {
      return EBADF;
    }
    
    // Find open file in file table
    if ((result = filetable_findfile(fd, &ofile))) {
        DEBUG(DB_SFS, "Cannot find open file in filetable\n");
        return result;
    }
    
     // Lock open file
    lock_acquire(ofile->of_lock);
    
    DEBUG(DB_SFS, "fstat return info about file \n");
    
    // Return info about a file
    if((result = VOP_STAT(ofile->of_vn, &stat_info))){
        // Release the lock
        lock_release(ofile->of_lock);
        DEBUG(DB_SFS, "VOP_STAT failure\n");
        return result;
    }
    
    DEBUG(DB_SFS, "fstat mk_useruio \n");
    
    /* set up a uio with the buffer, its size, and the current offset */
    mk_useruio(&user_iov, &user_uio, statptr, sizeof(struct stat), 0, UIO_READ);
    
    DEBUG(DB_SFS, "uio move \n");
    
    /* Move from kernel to userspace */
    if ((result = uiomove(&stat_info, sizeof(struct stat), &user_uio))) {
        // Release the lock
        lock_release(ofile->of_lock);
        return result;
    }
    
    // Release the lock
    lock_release(ofile->of_lock);
    
    return 0;
}

/*
 * sys_getdirentry
 */
int
sys_getdirentry(int fd, userptr_t buf, size_t buflen, int *retval)
{
    DEBUG(DB_SFS, "getdirentry entry \n");

	struct uio user_uio;
	struct iovec user_iov;
    struct openfile *ofile;
    int result;
    int offset;
    struct vnode* vnode;
    
    // Check if fd is valid
    if (fd < 0 || fd >= __OPEN_MAX) {
      return EBADF;
    }
    
    // Find open file in file table
    if ((result = filetable_findfile(fd, &ofile))) {
        DEBUG(DB_SFS, "Cannot find open file in filetable\n");
        return result;
    }
    
    // Lock open file
    lock_acquire(ofile->of_lock);
    
    // Get the offset 
    offset = ofile->of_offset;
    
    // Get vnode of open file
    vnode = ofile->of_vn;
    
    /* set up a uio with the buffer, its size, and the current offset */
	mk_useruio(&user_iov, &user_uio, buf, buflen, offset, UIO_READ);
        
    if ((result = VOP_GETDIRENTRY(vnode, &user_uio))) {
        DEBUG(DB_SFS, "VOP_GETDIRENTRY FAIL\n");
        lock_release(ofile->of_lock);
        return result;
    }
    
    // Set retval to length of the filename
    *retval = buflen - user_uio.uio_resid;
    
    // Set open file offset to new offset
    ofile->of_offset = user_uio.uio_offset;
    
    // Release lock
    lock_release(ofile->of_lock);
    
    return 0;
        
}

/* END A3 SETUP */




