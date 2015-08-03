# CSCC69 Project 3
Summer 2013

<h2>Outline</h2>

<ol>
<li> Introduction</a> </li>
<li> Starter Code</a></li>
<li> Setup</a></li>
<li> Project Tasks</a></li>
<li> Design Document</a></li>
<li> Hints and Tips</a></li>
</ol>

<h2><a name="intro">Introduction</a></h2>

<p> Currently, user programs in OS/161 are extremely limited, since
they cannot manipulate files on the disk.  In this Project, you
will add support for system calls that allow user programs to perform
file and file system operations.  You will also augment <tt>SFS</tt>,
the native OS/161 file system, by adding support for reading the entries
in a directory, and by making it more space-efficient.</p>

<h2><a name="starter">Starter Code</a></h2>

<p>By now you have put significant effort into your kernel, so we are
allowing you to choose your starting point for this Project.  You
can work with your P2 or P3 kernel, if you want to keep your fork /
pid management implementation, debug statements that you have added to
the code, virtual memory system and any other modifications that you
have made.  
</p>
<p>
We are providing a set of new files to get you started on the Project.
These include a pseudo shell
(described in a later section) in the <tt>src/user/bin/psh</tt> directory
and a few other changes:
</p><ul>
  <li> <tt>kern/arch/mips/syscall/syscall.c</tt> - main dispatch code for the
        system calls you have to handle</li>
  <li> <tt>kern/include/syscall.h</tt> - now including prototypes for the file system call handler functions</li>
  <li> <tt>kern/syscall/file_syscalls.c</tt> - new version with headers for all the file system related
  system calls you will implement
      </li>
  <li> <tt>kern/syscall/file.c</tt> - barebones scaffolding code related to handling
  open files. See also kern/include/file.h for the definitions of the file table structures.
	</li>
      <li><tt>kern/include/thread.h and kern/thread/thread.c</tt> - added a filetable field to the thread struct and initialized it to NULL on thread startup
      </li>
	<li><tt>kern/fs/sfs/</tt> - implementation of the "simple file system"
</li>
	<li><tt>user/sbin/mksfs</tt> - user-level utility program to initialize
a new SFS file system (the build also generates a version that can be run on 
the host system).</li>
	<li><tt>user/sbin/dumpsfs</tt> - user-level utility to dump the contents of an SFS file system on a disk (also comes with a host version)</li>
	<li><tt>user/sbin/sfsck</tt> - user-level utility to check an sfs file
system for consistency (also comes with a host version)</li>
</ul>

<p>If you choose to work with your own kernel, you will need to integrate the changes in these files with your existing code manually (all relevant code changes are marked with "/* BEGIN ASST3 SETUP */" and "/* END ASST3 SETUP */" comments to make it easy to locate).  We are also providing a full kernel tree, based on the P3 starter code with these additions already incorporated.
</p>
<p>The ASST3 config file is set up to use DUMBVM again,
but you should be able to use your  VM solution from ASST3 if you wish,
and just change the ASST3 config file to not use DUMBVM. </p>


<h2><a name="setup">Setup</a></h2>

<p></p><ol>
    
      <li>Check out P3 starting code </li>
      <li> Configure and build your P3 kernel, as well as the user-level test
programs and utilities.</li>
</ol>

<p> The next step after downloading and building the starter code 
is to set up an SFS file system on one of the disks.
Use the <tt>hostbin/host-mksfs</tt> program in your <tt>~/cscc69/root</tt>
directory (the hostbin programs should be executed on the host machine, 
not on OS/161).  The command is:
</p><pre>host-mksfs DISK1.img VOLNAME
</pre>

<p> The second argument, VOLNAME, is a string that you provide to name
the disk volume that contains the file system.  You can choose anything
you like, but you will be providing the VOLNAME to commands that operate
on the file system, so you might want to keep it simple.

</p><p> Note that if you have decided to use your VM solution instead of DUMBVM,
you should initialize the SFS file system on DISK2.img, since DISK1.img is
used for the swap file.
</p>

<p> After you have the file system set up, you can boot OS/161, mount the
filesystem (using the mount menu command - the "fstype" will be "sfs", and you 
will need to figure out the name of the hard disk device to provide as the 
"device:" argument (probably <tt>lhd0</tt>)), and run the file system performance
test from the kernel menu by specifying <tt>fs1 VOLNAME</tt> (replace VOLNAME with 
the name you gave when creating the disk volume above).


</p><h2><a name="tasks">Project Tasks</a></h2>

Your primary goals are to complete a set of file-system related system
calls and to augment <tt>SFS</tt> with the ability to read directory
entries.  You will also make SFS use disk space more efficiently.  

    <ol>
      <li><b>Open file table</b> (15 pts): A major part of handling
      file system calls is the design and implementation of the
      per-process open file table.  You must support fork() semantics
      for the open file table, which may involve changes to the fork()
      implementation that you were provided.  Make sure you read the
      OS/161 man page for fork().
      </li>
      <li><b>System Calls</b> (30 pts): Implement 10 new system calls,
      detailed in the System Calls
      section</a>.  It's not as bad as it sounds -- many of the system
      calls are paired.  <tt>open</tt> and <tt>close</tt>, for
      example.
      </li>
      <li><b>Reading directory entries(OPTIONAL)</b> (5 pts): The current SFS
      code does not support the VFS "getdirentry" operation, so you
      cannot list the files on SFS.  You need to implement this
      operation for SFS.
      </li>
      <li><b>Using wasted space in SFS inode blocks(OPTIONAL)</b> (5 pts): The
      current SFS code uses an entire disk block (512 bytes) for an
      inode, but most of that space is unused.  You will modify the
      SFS code to allow the first part of file data to be stored in
      the unused space in the file's inodes.
      </li>
      <li><b>Design Document</b> (5 pts): Make sure to discuss how
      your design for the open file table supports fork() semantics (a
      child inherits the open file descriptors of the parent, but has
      its own file table so that files opened after the fork are not
      shared). In addition, if you are unable to complete the
      Project, include a list of problems remaining in the
      submission.
      </li>
</ol>

<h2>Open File Table (15 pts)</h2>
<p>
Although these system calls may seem to be tied to the filesystem, in
fact, many of them are really about manipulation of file
descriptors, or process-specific filesystem state.  We have designed
part of this system for you, in <tt>syscall/file.c</tt>.  Some of
this information (such as the current working directory) is specific
only to the process, but others (such as the file offset) is specific
to the process and file descriptor.  Think carefully about the state
you need to maintain, how to organize it, and when and how it has to
change.  Think carefully also about what is shared when a process forks,
and what happens when a file is closed in the parent or the child (or both).

</p><p>
For any given process, the first file descriptors (0, 1, and 2) are
considered to be standard input (stdin), standard output (stdout),
and standard error (stderr). These file descriptors should start out
attached to the console device ("con:"), but your implementation must
allow programs to use <tt>dup2()</tt> to change them to point elsewhere.
You need to ensure that these are initialized correctly as part of starting
up a user process (refer to <tt>kern/syscall/runprogram.c</tt>).

</p><h2><a name="syscalls">New System Calls (30 pts)</a></h2>


<p>Add support for the system calls listed below. 
Some of these deal only with managing file system state, while others
operate on the file system itself.  <b>Read the OS/161 man pages
for these system calls for details on their arguments and operation before 
starting to implement them.</b>

</p><ul>
  <li> <tt>open</tt>, <tt>close</tt>, <tt>dup2</tt></li>
  <li> <tt>read</tt>, <tt>write</tt>, <tt>lseek</tt></li>
  <li> <tt>chdir</tt>, <tt>getcwd</tt>
  </li><li> <tt>fstat</tt>, <tt>getdirentry</tt>,  
</li></ul>

<h3>Managing per-process file system state</h3>

<p><b><tt>open()</tt>, <tt>close()</tt>, <tt>dup2</tt>, <tt>read()</tt>, <tt>write()</tt>, <tt>lseek()</tt>, <tt>chdir()</tt>, and <tt>getcwd()</tt></b>

</p><p> 
You will likely find it useful to look at the console I/O setup provided for P3 and the simple versions of sys_read() and sys_write() to help you get started with implementing these system calls. (refer to <tt>kern/syscall/file_syscalls.c</tt>)

</p><h3> Managing file systems</h3>

<p><b><tt>getdirentry()</tt>, <tt>fstat()</tt></b>

</p><p> The remaining system calls read the state of the file system itself. 
Mainly, these calls can be handled by calling the appropriate VFS functions (after checking for errors with arguments
provided by users!).  The <tt>getdirentry</tt> call, however, will
get an error code returned by the SFS layer until the implementation of 
this function is complete.  

</p><p>For this part of the project,
the system calls should be considered complete when they handle checks 
for user errors, call the correct lower-level function, and return suitable
results or error codes to the user level.  If the lower file system layers
do not implement the functionality that the system call requires to 
perform correctly, that is okay.  Extensions to the SFS file system will be 
dealt with in the next part of the project.

</p><h3>Error Code Guidelines</h3>

<p> The general requirements for error codes are detailed in the
OS/161 man pages. Specific requirements:

</p><ul>
  <li> If a file or directory is expected to exist by the semantics of a 
call, and it does not, the resulting error code should be ENOENT.

  </li><li> If a file is encountered where a directory was expected, the
resulting error code should be ENOTDIR.

  </li><li> If a directory is encountered where a file was expected, the 
resulting error code should be EISDIR.

  </li><li> If an operation cannot be completed because the disk is full, 
the resulting error code should be ENOSPC.

  </li><li> If removal of a non-empty directory is attempted, the resulting 
error code should be ENOTEMPTY.

  </li><li> When an invalid file handle is used, the 
resulting error code should be EBADF.

  </li><li> If an attempt is made to operate in a prohibited manner upon 
"." or "..", the resulting error code should be EINVAL.

</li></ul>

<p> Note that many of these errors cannot be detected within the
system call handlers themselves.  You should simply check if the
corresponding VFS operation succeeds or fails, and return the error
from the VFS function on failure.  You will need to consider the list
of possible errors from the OS161 man pages, and decide which ones can
be checked by the system call handler and which ones are the
responsibility of the lower-level code.

</p><h2>SFS getdirentry implementation OPTIONAL (5 pts)</h2>

<p> For this part of the project, you must work below the VFS layer and 
augment the SFS file system.  The main file to look at is <tt>kern/fs/sfs/sfs_vnops.c</tt>, where the interface functions for the VFS layer are implemented.
You will find that the <tt>vnode_ops</tt> table for directories (<tt>sfs_dirops</tt>) has <tt>UNIMP</tt> as the function for <tt>getdirentry</tt>.  
</p>
<p>
Implement a function named <tt>sfs_getdirentry</tt> and modify the <tt>sfs_dirops</tt> table to point to it instead of <tt>UNIMP</tt>.  You should review the 
comments for vop_getdirentry in <tt>kern/include/vnode.h</tt>, the structure of sfs directory entries in <tt>kern/include/kern/sfs.h</tt>, the existing <tt>sfs_readdir</tt> function in <tt>kern/fs/sfs/sfs_vnops.c</tt>, and the uio functions in <tt>kern/include/uio.h</tt> and <tt>kern/lib/uio.c</tt>.

</p><h2>Using space in SFS inodes OPTIONAL(5 pts)</h2>
<p>Read the definition of a "struct sfs_inode" in <tt>kern/include/kern/sfs.h</tt>.  It is designed to occupy exactly one disk block.  This design has some 
advantages over the Unix File System design -- an inode number can simply be its disk block number, and there is no fixed limit on the number of inodes in the file system.  The downside is that it is very wasteful of space.  There are
only 60 bytes of information in the inode (12 direct block pointers = 48 bytes, 1 indirect block pointer = 4 bytes, 4 bytes for size, 2 bytes for type and 2 bytes for linkcount), with the rest allocated to the "sfi_waste" field.</p>
<p>This wasted space can be used to store the first part of the file data.
Modify the sfs_inode structure to include a field "char sfi_inlinedata[SFS_INLINED_BYTES]" and modify the code in <tt>sfs_vnops.c</tt> to store the first
SFS_INLINED_BYTES of file data in the inode itself.  Note that we have 
defined SFS_INLINED_BYTES for you in <tt>kern/include/kern/sfs.h</tt> to be 
a multiple of the directory entry size, so the same changes should work for
storing the first few directory entries in the inode.  There may still be 
some wasted space in the inode -- make sure that the sizeof(struct sfs_inode) is still 512 bytes when you have made your changes!</p>
<p><b>Tips</b>:
</p><ul>
<li> This change really only affects the way that an offset in the file is translated to an address on the disk.  For example, currently offset 0 in the file 
corresponds to offset 0 in the disk block whose address is stored in <tt>sfi_direct[0]</tt>. That is, it is the first byte in the first data block.  When we inline some data into the inode, offset 0 in the file will be the first byte of the new sfi_inlinedata field in the inode.  Similarly, the first byte of the first data block will now be at offset SFS_INLINE_DATA in the file.  Read the <tt>sfs_io()</tt>
function in <tt>sfs_vnops.c</tt> to see how file offsets are currently mapped
to offsets in disk blocks, and make the necessary changes there. (check for 
comments in <tt>sfs_vnops.c</tt> that indicate where changes are likely to be needed)</li>
<li> Modified inodes must be marked dirty so that they are written back to 
disk during a sync operation.  Make sure that you mark the inode dirty if there are any writes to the new data
region in an inode.
</li>
<li> You will also need to consider how this change affects truncating a 
file to a new length.  Read and modify <tt>sfs_truncate()</tt> in <tt>sfs_vnops.c</tt>
</li>
</ul>

<h3>Testing Your Implementation</h3>

<p> The parts above the VFS layer, and the parts below can mostly be tested 
independently.  The exception is the getdirentry system call, which requires 
both an implementation of the system call and the underlying sfs_getdirentry
function to work together.
</p>
<p>The SFS design is not "crash consistent", meaning that if the OS crashes (or panics, or is killed with a CTRL-C, or 
any such unclean shutdown), the state of the file system on disk will be 
inconsistent.  This happens because SFS caches modified 
file system state in memory until either a sync is called, or the file system is unmounted -- these modifications are lost when there is a crash.  IThe best thing to do is to re-initialize the file system on
your DISK file (using hostbin/host-mksfs) whenever the system crashes, since 
you cannot trust the state of the file system on that disk otherwise.
</p>
<p>In addition to hostbin/host-mksfs, there are two other utility
programs that might come in handy: hostbin/host-dumpsfs will dump the
file system stored on the specified DISK, showing the state of the
free space bitmaps and the directory entries. To remain useful, however, it needs to be updated to know about directory entries that might be stored in the 
directory inode itself.  The same is true for hostbin/host-sfsck, which
checks a file system on disk for consistency and corrects what problems it can.</p>  
<p><b>System
calls:</b><br> Your code-base does not include <tt>execv</tt>, nor
does the "p" command allow arguments to be passed.  Instead,
<tt>/src/bin</tt> contains a new command named <tt>psh</tt>
(pseudo-shell) that will allow you to run the file system programs
<tt>ls.c, cp.c, cat.c</tt> with arguments.  It is a pseudo shell
because it does not fork and exec a new process when you type in a
command.  Instead, it simply calls the function that implements each
program.
</p>
<p> Note that the pseudo-shell may not work until you have implemented the 
read system call, so that it can read commands that you enter at the console.
<em>We have left the "dumb" console-only versions of sys_read and sys_write
in place, to facilitate early testing, but these should be replaced with
versions that use the file table and work for all file descriptors. Your
submission should remove all use of "dumb_console_IO" including the call to
bootstrap this in kern/startup/main.c</em>
We have also provided a cheap hack to synchronize the menu thread with the thread forked to
run the pseudo-shell process (or other user-level processes), so that characters you type are read by the shell (or other process) and not by the OS161 menu thread.  If you do not have a working pid_join()
implementation from P1, you can use this simple synchronization strategy. (We  create a semaphore with initial value of 0; the menu thread does a P()
on this semaphore and the program thread does a V() when it exits.)
</p>
<p>You may also find the following programs in <tt>testbin</tt> useful:  
<tt>badcall, bigfile, conman, dirseek, filetest, hash, sink, tail</tt>.
You should run these directly from the OS/161 prompt -- not through <tt>psh</tt>.  
Several of these will need some modification: dirseek attempts to create a 
subdirectory on SFS for testing, but you are not implementing mkdir; filetest, 
hash, and tail expect arguments such as a file name but argument passing to 
user programs is only available if you have your P1 solution (you can hardcode 
the arguments for individual tests).

</p><p><b>Using inode for data</b><br>
The built-in file system tests, fs[1-5], can be used from the OS/161 menu
prompt. They work on the starter code for SFS, and should continue to work
when you have made your changes.
</p>

<h3>Requirements</h3>

<p>We will test your implementation by running the file system
commands we have provided to you for <tt>psh</tt>, as well as the fs
tests from the menu.  Make sure your implementation supports as many
of these operations as possible.  If it cannot support all of them,
please note how complete your solution is in your design document.

</p><h2><a name="design">Design Document</a></h2>

<p>
Your design document should address the following topics:

</p><ul>
      <li>Explain how your open file table design supports the <tt>fork</tt>
	semantics.
      </li>
      <li>Explain your implementation of sfs_getdirentry.</li>
<li> Explain what changes you needed to make to use wasted inode space to 
store file data.</li>
<li> Discuss the single biggest challenge you had to address for the project
</li>
</ul>

<h2><a name="strategy">More Hints and Tips</a></h2>
<ul>


  <li>Don't reinvent the wheel.  Look for provided functions that can help
	to implement the project requirements.

  </li><li>Much of this project is easily split apart.  Expect to work
  with your partner(s) while designing the open file table (and the <tt>open()/close()</tt> system calls) -- make sure everyone agrees and understands how to use the open file table -- and then split up the remaining system calls.  Once
  the system calls are done, review what you've learned before
  tackling the sfs_getdirentry part of the project.

  </li><li>The changes to store file data in inodes are 
independent of the rest of the project. </li>

  <li>We say "test early and often".   You can do that
  for much of the project -- build open and close, then build read
  and write, for example. You can test the system call implementations by 
  using the original emulated file system (emufs), however some operations are
  only partially supported (for instance, chdir works, but getcwd only
  works on the root of the file system).  

  </li><li>Don't assign one partner to code and the other to test.  Switch
  responsibilities, making sure to test each other's code.  Review
  each other's designs, as well; comparing ideas and reviewing design
  decisions will keep your code clean and will prepare you for the
  exam. 

  </li><li>Partner relations remain key.  Please be professional -- even as
  you try your hardest to break your partner's code.  Remember that
  it's his or her job to do the same to your code.
</li></ul>
