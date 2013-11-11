/* BEGIN A3 SETUP */
/*
 * File handles and file tables.
 * New for ASST3
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/unistd.h>
#include <file.h>
#include <syscall.h>
#include <kern/fcntl.h>
#include <vfs.h>
#include <synch.h>
#include <current.h>

/*** openfile functions ***/

/*
 * file_open
 * opens a file, places it in the filetable, sets RETFD to the file
 * descriptor. the pointer arguments must be kernel pointers.
 * NOTE -- the passed in filename must be a mutable string.
 * 
 * A3: As per the OS/161 man page for open(), you do not need 
 * to do anything with the "mode" argument.
 */
int
file_open(char *filename, int flags, int mode, int *retfd)
{
    //DEBUG(DB_SFS, "file_open entry \n");
    struct vnode *vn;
    struct openfile *ofile;
    int result;
    

    // check if the file already open
    result = vfs_open(filename, flags, mode, &vn);
    if (result) {
        return result;
    }
    
    // reserve memory for the file
    ofile = kmalloc(sizeof(struct openfile));
    if (ofile == NULL) {
        vfs_close(vn);
        return ENOMEM;
    }

    // initialize open file
    ofile->of_lock = lock_create("file lock");
    if (ofile->of_lock == NULL) {
        vfs_close(vn);
        kfree(ofile);
        return ENOMEM;
    }
    ofile->of_vn = vn;
    ofile->of_offset = 0;
    ofile->of_flags = flags;
    ofile->of_refcount = 1;

    // add file to file table, getting the file descriptor */
    result = filetable_addfile(ofile, retfd);
    if (result) {
        lock_destroy(ofile->of_lock);
        kfree(ofile);
        vfs_close(vn);
        return result;
    }

    return 0;
}


/* 
 * file_close
 * Called when a process closes a file descriptor.  Think about how you plan
 * to handle fork, and what (if anything) is shared between parent/child after
 * fork.  Your design decisions will affect what you should do for close.
 */
int
file_close(int fd) {
    //DEBUG(DB_SFS, "file_close entry \n");
    struct openfile *ofile;
    int result;
    

    // find the file in the file table
    result = filetable_findfile(fd, &ofile);
    if (result) {  // file not found
        return result;
    }
    
    // lock the file
    lock_acquire(ofile->of_lock);

    // if file has only one reference, can close it and free mem
    if (ofile->of_refcount == 1) {
        
        vfs_close(ofile->of_vn);
        lock_release(ofile->of_lock);
        lock_destroy(ofile->of_lock);
        kfree(ofile);
        
        // clear entry in file table
        curthread->t_filetable->ft_openfiles[fd] = NULL; 
        
    } else if (ofile->of_refcount > 1) {  // else the file has more than one reference and cannot be closed
            
        ofile->of_refcount--;
        lock_release(ofile->of_lock);
        return ENOTEMPTY;
            
    }    

    return 0;
}

/*** filetable functions ***/

/* 
 * filetable_init
 * pretty straightforward -- allocate the space, set up 
 * first 3 file descriptors for stdin, stdout and stderr,
 * and initialize all other entries to NULL.
 * 
 * Should set curthread->t_filetable to point to the
 * newly-initialized filetable.
 * 
 * Should return non-zero error code on failure.  Currently
 * does nothing but returns success so that loading a user
 * program will succeed even if you haven't written the
 * filetable initialization yet.
 */

int
filetable_init(void)
{
    int fd;
    char path[5];
    int result;
    
    if (curthread->t_filetable != NULL) {
        DEBUG(DB_SFS, "File table already exists\n");
        return EBADF;
    }
    
    // allocate space for file table
    curthread->t_filetable = kmalloc(sizeof(struct filetable));
    if (curthread->t_filetable == NULL) {
        DEBUG(DB_SFS, "Failed to allocate space for file table\n");
        return ENOMEM;
    }
    
    // initialize all entries to NULL
    for (fd = 0; fd < __OPEN_MAX; fd++) {
        curthread->t_filetable->ft_openfiles[fd] = NULL;
    }
 
    
    // set up first 3 file descriptors: stdin, stdout, stderr
    strcpy(path, "con:");
    result = file_open(path, O_RDONLY, 0, &fd);
    if (result) {
            return result;
    }
    strcpy(path, "con:");
    result = file_open(path, O_WRONLY, 0, &fd);
    if (result) {
            return result;
    }
    strcpy(path, "con:");
    result = file_open(path, O_WRONLY, 0, &fd);
    if (result) {
            return result;
    }

    return 0;
}	

/*
 * filetable_destroy
 * closes the files in the file table, frees the table.
 * This should be called as part of cleaning up a process (after kill
 * or exit).
 */
void
filetable_destroy(struct filetable *ft)
{
    DEBUG(DB_SFS, "file table_destroy entry \n");
    int fd;
    int result;
    
    if (ft == NULL) {
        DEBUG(DB_SFS, "File table already exists\n");
    
    } else {
        for (fd = 0; fd < __OPEN_MAX; fd++) {
            if (ft->ft_openfiles[fd]) {
                result = file_close(fd);
            }
        }
    }
    kfree(ft);
}	


/* 
 * You should add additional filetable utility functions here as needed
 * to support the system calls.  For example, given a file descriptor
 * you will want some sort of lookup function that will check if the fd is 
 * valid and return the associated vnode (and possibly other information like
 * the current file position) associated with that open file.
 */


/*
* filetable_findfile
* check if file descriptor is valid and references an
* open file, and set file to the file at fd
*/
int
filetable_findfile(int fd, struct openfile **ofile) {
    
    //DEBUG(DB_SFS, "filetable findfile entry \n");
    
    if (fd < 0 || fd >= __OPEN_MAX) {
        DEBUG(DB_SFS, "Invalid fd outside the bounds of the file table: %d\n", fd);
        return EBADF;
    }

    *ofile = curthread->t_filetable->ft_openfiles[fd];
    if (*ofile == NULL) {
        DEBUG(DB_SFS, "No open file pointed to by fd: %d\n", fd);
        return EBADF;
    }

    return 0;
}

/*
 * filetable addfile
 * find the first available space in the file table, 
 * add the file to the file table and set the file descriptor to the index
 */
int
filetable_addfile(struct openfile *ofile, int *retfd) {
    
    DEBUG(DB_SFS, "filetable add file entry \n");

    for (int i = 0; i < __OPEN_MAX; i++) {
        if (curthread->t_filetable->ft_openfiles[i] == NULL) {
            curthread->t_filetable->ft_openfiles[i] = ofile;
            *retfd = i;
            return 0;
        }
    }
    
    DEBUG(DB_SFS, "File table is full\n");
    return ENOSPC;
}

/* END A3 SETUP */
