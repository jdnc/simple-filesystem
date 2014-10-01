#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <libssh/libssh.h>

# define SESSION ((ssh_session *) fuse_get_context()->private_data)

int jdfs_getattr(const char *path, struct stat *statbuf)
{
    ssh_channel channel = ssh_channel_new(SESSION);
    if (ssh_channel == NULL) {
        perror("get_attr: could not open new ssh channel");
   	exit(-1);
    }
    int rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        perror("getattr: ssh_channel_open_session failed");
        ssh_channel_free(channel);
	exit(-1);
    }
    rc = ssh_channel_request_exec()
}


/** Create a directory */
int bb_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
    bb_fullpath(fpath, path);
    
    retstat = mkdir(fpath, mode);
    if (retstat < 0)
	retstat = bb_error("bb_mkdir mkdir");
    
    return retstat;
}

/** Remove a file */
int bb_unlink(const char *path)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("bb_unlink(path=\"%s\")\n",
	    path);
    bb_fullpath(fpath, path);
    
    retstat = unlink(fpath);
    if (retstat < 0)
	retstat = bb_error("bb_unlink unlink");
    
    return retstat;
}

/** Remove a directory */
int bb_rmdir(const char *path)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("bb_rmdir(path=\"%s\")\n",
	    path);
    bb_fullpath(fpath, path);
    
    retstat = rmdir(fpath);
    if (retstat < 0)
	retstat = bb_error("bb_rmdir rmdir");
    
    return retstat;
}

/
/** Rename a file */
// both path and newpath are fs-relative
int bb_rename(const char *path, const char *newpath)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];
    
    log_msg("\nbb_rename(fpath=\"%s\", newpath=\"%s\")\n",
	    path, newpath);
    bb_fullpath(fpath, path);
    bb_fullpath(fnewpath, newpath);
    
    retstat = rename(fpath, fnewpath);
    if (retstat < 0)
	retstat = bb_error("bb_rename rename");
    
    return retstat;
}



/** Change the permission bits of a file */
int bb_chmod(const char *path, mode_t mode)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_chmod(fpath=\"%s\", mode=0%03o)\n",
	    path, mode);
    bb_fullpath(fpath, path);
    
    retstat = chmod(fpath, mode);
    if (retstat < 0)
	retstat = bb_error("bb_chmod chmod");
    
    return retstat;
}

/** Change the owner and group of a file */
int bb_chown(const char *path, uid_t uid, gid_t gid)
  
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_chown(path=\"%s\", uid=%d, gid=%d)\n",
	    path, uid, gid);
    bb_fullpath(fpath, path);
    
    retstat = chown(fpath, uid, gid);
    if (retstat < 0)
	retstat = bb_error("bb_chown chown");
    
    return retstat;
}

/** Change the size of a file */
int bb_truncate(const char *path, off_t newsize)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_truncate(path=\"%s\", newsize=%lld)\n",
	    path, newsize);
    bb_fullpath(fpath, path);
    
    retstat = truncate(fpath, newsize);
    if (retstat < 0)
	bb_error("bb_truncate truncate");
    
    return retstat;
}


/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int bb_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);
    bb_fullpath(fpath, path);
    
    fd = open(fpath, fi->flags);
    if (fd < 0)
	retstat = bb_error("bb_open open");
    
    fi->fh = fd;
    log_fi(fi);
    
    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.
int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);
    
    retstat = pread(fi->fh, buf, size, offset);
    if (retstat < 0)
	retstat = bb_error("bb_read read");
    
    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.
int bb_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi
	    );
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);
	
    retstat = pwrite(fi->fh, buf, size, offset);
    if (retstat < 0)
	retstat = bb_error("bb_write pwrite");
    
    return retstat;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int bb_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_flush(path=\"%s\", fi=0x%08x)\n", path, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);
	
    return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int bb_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    log_fi(fi);

    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    retstat = close(fi->fh);
    
    return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int bb_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_opendir(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    bb_fullpath(fpath, path);
    
    dp = opendir(fpath);
    if (dp == NULL)
	retstat = bb_error("bb_opendir opendir");
    
    fi->fh = (intptr_t) dp;
    
    log_fi(fi);
    
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int bb_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    int retstat = 0;
    DIR *dp;
    struct dirent *de;
    
    log_msg("\nbb_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
	    path, buf, filler, offset, fi);
    // once again, no need for fullpath -- but note that I need to cast fi->fh
    dp = (DIR *) (uintptr_t) fi->fh;

    // Every directory contains at least two entries: . and ..  If my
    // first call to the system readdir() returns NULL I've got an
    // error; near as I can tell, that's the only condition under
    // which I can get an error from readdir()
    de = readdir(dp);
    if (de == 0) {
	retstat = bb_error("bb_readdir readdir");
	return retstat;
    }

    // This will copy the entire directory into the buffer.  The loop exits
    // when either the system readdir() returns NULL, or filler()
    // returns something non-zero.  The first case just means I've
    // read the whole directory; the second means the buffer is full.
    do {
	log_msg("calling filler with name %s\n", de->d_name);
	if (filler(buf, de->d_name, NULL, 0) != 0) {
	    log_msg("    ERROR bb_readdir filler:  buffer full");
	    return -ENOMEM;
	}
    } while ((de = readdir(dp)) != NULL);
    
    log_fi(fi);
    
    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int bb_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_releasedir(path=\"%s\", fi=0x%08x)\n",
	    path, fi);
    log_fi(fi);
    
    closedir((DIR *) (uintptr_t) fi->fh);
    
    return retstat;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
// Undocumented but extraordinarily useful fact:  the fuse_context is
// set up before this function is called, and
// fuse_get_context()->private_data returns the user_data passed to
// fuse_main().  Really seems like either it should be a third
// parameter coming in here, or else the fact should be documented
// (and this might as well return void, as it did in older versions of
// FUSE).
void *jdfs_init(struct fuse_conn_info *conn)
{
    return SESSION;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void bb_destroy(void *userdata)
{
    log_msg("\nbb_destroy(userdata=0x%08x)\n", userdata);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int bb_access(const char *path, int mask)
{
    int retstat = 0;
    char fpath[PATH_MAX];
   
    log_msg("\nbb_access(path=\"%s\", mask=0%o)\n",
	    path, mask);
    bb_fullpath(fpath, path);
    
    retstat = access(fpath, mask);
    
    if (retstat < 0)
	retstat = bb_error("bb_access access");
    
    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int bb_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    int fd;
    
    log_msg("\nbb_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
	    path, mode, fi);
    bb_fullpath(fpath, path);
    
    fd = creat(fpath, mode);
    if (fd < 0)
	retstat = bb_error("bb_create creat");
    
    fi->fh = fd;
    
    log_fi(fi);
    
    return retstat;
}

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int bb_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_ftruncate(path=\"%s\", offset=%lld, fi=0x%08x)\n",
	    path, offset, fi);
    log_fi(fi);
    
    retstat = ftruncate(fi->fh, offset);
    if (retstat < 0)
	retstat = bb_error("bb_ftruncate ftruncate");
    
    return retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int bb_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_fgetattr(path=\"%s\", statbuf=0x%08x, fi=0x%08x)\n",
	    path, statbuf, fi);
    log_fi(fi);

    // On FreeBSD, trying to do anything with the mountpoint ends up
    // opening it, and then using the FD for an fgetattr.  So in the
    // special case of a path of "/", I need to do a getattr on the
    // underlying root directory instead of doing the fgetattr().
    if (!strcmp(path, "/"))
	return bb_getattr(path, statbuf);
    
    retstat = fstat(fi->fh, statbuf);
    if (retstat < 0)
	retstat = bb_error("bb_fgetattr fstat");
    
    log_stat(statbuf);
    
    return retstat;
}

struct fuse_operations bb_oper = {
  .getattr = bb_getattr,
  .readlink = bb_readlink,
  // no .getdir -- that's deprecated
  .getdir = NULL,
  .mknod = bb_mknod,
  .mkdir = bb_mkdir,
  .unlink = bb_unlink,
  .rmdir = bb_rmdir,
  .symlink = bb_symlink,
  .rename = bb_rename,
  .link = bb_link,
  .chmod = bb_chmod,
  .chown = bb_chown,
  .truncate = bb_truncate,
  .utime = bb_utime,
  .open = bb_open,
  .read = bb_read,
  .write = bb_write,
  /** Just a placeholder, don't set */ // huh???
  .statfs = bb_statfs,
  .flush = bb_flush,
  .release = bb_release,
  .fsync = bb_fsync,
  
#ifdef HAVE_SYS_XATTR_H
  .setxattr = bb_setxattr,
  .getxattr = bb_getxattr,
  .listxattr = bb_listxattr,
  .removexattr = bb_removexattr,
#endif
  
  .opendir = bb_opendir,
  .readdir = bb_readdir,
  .releasedir = bb_releasedir,
  .fsyncdir = bb_fsyncdir,
  .init = bb_init,
  .destroy = bb_destroy,
  .access = bb_access,
  .create = bb_create,
  .ftruncate = bb_ftruncate,
  .fgetattr = bb_fgetattr
};

void bb_usage()
{
    fprintf(stderr, "usage:  bbfs [FUSE and mount options] rootDir mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    char serverAddr[64]; 
    char userName[15];
    strcpy(serverAddr, argv[1]);
    strcpy(userName, argv[2]);
    ssh_session jdfs_session = ssh_new();
    if (jdfs_session == NULL) {
        perror("failed to launch ssh session");
	exit(-1);
    }
    ssh_options_set(jdfs_session, SSH_OPTIONS_HOST, serverAddr);
    ssh_options_set(jdfs_session, SSH_OPTIONS_USER, userName);
    int rc = ssh_userauth_publickey_auto(jdfs_session, NULL);
    if (rc == SSH_AUTH_ERROR) {
        perror("failed to authenticate user on server");
	exit(rc);
    }
    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &bb_oper, jdfs_session);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    ssh_free(jdfs_session);
    return fuse_stat;
}
