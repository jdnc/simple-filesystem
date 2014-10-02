#define FUSE_USE_VERSION 26
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
#include <libssh/sftp.h>
#include <libssh/libssh.h>


#define BUFSIZE 16384

typedef struct jdfs_state {
    char* base_path;
    ssh_session* session;
} jdfs_state;

#define JDFS_DATA ((jdfs_state *) fuse_get_context()->private_data)

/*int jdfs_getattr(const char *path, struct stat *statbuf)
{
    ssh_channel channel = ssh_channel_new(*JDFS_DATA->session);
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
}*/


/** Create a directory */
int jdfs_mkdir(const char *path, mode_t mode)
{
    sftp_session sftp = sftp_new(*(JDFS_DATA->session));
    if (sftp == NULL) {
	perror("mkdir: could not create sftp session");
	exit(SSH_ERROR);
    }
    int rc = sftp_init(sftp);
    if (rc != SSH_OK) {
    	perror("mkdir : sftp init failed");
	sftp_free(sftp);
	exit(rc);
    }
    rc = sftp_mkdir(sftp, path, mode);
    if (rc != SSH_OK) {
	fprintf(stderr, "can't create directory %s\n", ssh_get_error(*(JDFS_DATA->session)));
	exit(rc);
    }
    sftp_free(sftp);
    return 0;
}

/** Remove a file */
int jdfs_unlink(const char *path)
{
    sftp_session sftp = sftp_new(*(JDFS_DATA->session));
    if (sftp == NULL) {
	perror("unlink: could not create sftp session");
	exit(SSH_ERROR);
    }
    int rc = sftp_init(sftp);
    if (rc != SSH_OK) {
    	perror("unlink: sftp init failed");
	sftp_free(sftp);
	exit(rc);
    }
    rc = sftp_unlink(sftp, path);
    if (rc != SSH_OK) {
	fprintf(stderr, "can't unlink file %s\n", ssh_get_error(*(JDFS_DATA->session)));
	exit(rc);
    }
    sftp_free(sftp);
    return 0;
}

/** Remove a directory */
int jdfs_rmdir(const char *path)
{
    sftp_session sftp = sftp_new(*(JDFS_DATA->session));
    if (sftp == NULL) {
	perror("rmdir: could not create sftp session");
	exit(SSH_ERROR);
    }
    int rc = sftp_init(sftp);
    if (rc != SSH_OK) {
    	perror("rmdir: sftp init failed");
	sftp_free(sftp);
	exit(rc);
    }
    rc = sftp_rmdir(sftp, path);
    if (rc != SSH_OK) {
	fprintf(stderr, "can't remove dir %s\n", ssh_get_error(*(JDFS_DATA->session)));
	exit(rc);
    }
    sftp_free(sftp);
    return 0;
}


/** Rename a file */
// both path and newpath are fs-relative
int jdfs_rename(const char *path, const char *newpath)
{
    sftp_session sftp = sftp_new(*(JDFS_DATA->session));
    if (sftp == NULL) {
	perror("rename: could not create sftp session");
	exit(SSH_ERROR);
    }
    int rc = sftp_init(sftp);
    if (rc != SSH_OK) {
    	perror("rename: sftp init failed");
	sftp_free(sftp);
	exit(rc);
    }
    rc = sftp_rename(sftp, path, newpath);
    if (rc != SSH_OK) {
	fprintf(stderr, "can't rename dir %s\n", ssh_get_error(*(JDFS_DATA->session)));
	exit(rc);
    }
    sftp_free(sftp);
    return 0;
}

/** Change the permission bits of a file */
int jdfs_chmod(const char *path, mode_t mode)
{
    sftp_session sftp = sftp_new(*(JDFS_DATA->session));
    if (sftp == NULL) {
	perror("chmod: could not create sftp session");
	exit(SSH_ERROR);
    }
    int rc = sftp_init(sftp);
    if (rc != SSH_OK) {
    	perror("chmod: sftp init failed");
	sftp_free(sftp);
	exit(rc);
    }
    rc = sftp_chmod(sftp, path, mode);
    if (rc != SSH_OK) {
	fprintf(stderr, "chmod failed %s\n", ssh_get_error(*(JDFS_DATA->session)));
	exit(rc);
    }
    sftp_free(sftp);
    return 0;
}

/** Change the owner and group of a file */
int jdfs_chown(const char *path, uid_t uid, gid_t gid)
{
    sftp_session sftp = sftp_new(*(JDFS_DATA->session));
    if (sftp == NULL) {
	perror("chown: could not create sftp session");
	exit(SSH_ERROR);
    }
    int rc = sftp_init(sftp);
    if (rc != SSH_OK) {
    	perror("chown: sftp init failed");
	sftp_free(sftp);
	exit(rc);
    }
    rc = sftp_chown(sftp, path, uid, gid);
    if (rc != SSH_OK) {
	fprintf(stderr, "chown failed %s\n", ssh_get_error(*(JDFS_DATA->session)));
	exit(rc);
    }
    sftp_free(sftp);
    return 0;
}


/** File open operation */
int jdfs_open(const char *path, struct fuse_file_info *fi)
{
    char full_path[2048];
    char prefix[1600];
    int access_type;
    sftp_file file;
    char buffer[BUFSIZE];
    int nbytes, nwritten;
    int fd;
    int rc;
    strcpy(full_path, JDFS_DATA->base_path);
    strncat(full_path, path, 1600);
    rc = access(full_path, F_OK);
    if (rc < 0) {
            char * pos = strrchr(path, '/');
            if (pos != NULL) {
		*pos = '\0';
		strcpy(prefix, path);
		*pos = '/';
                char tmp_path[2048];
                char cmdstring[2048] = "mkdir -p ";
		strcpy(tmp_path, JDFS_DATA->base_path);
                strcat(tmp_path, prefix);
                strcat(cmdstring, tmp_path);
                rc = system(cmdstring);
                if (rc < 0) {
		    fprintf(stderr, "open: cannot cache file locally");
                    return -1;
                }
            }
	    sftp_session sftp = sftp_new(*(JDFS_DATA->session));
	    if (sftp == NULL) {
		perror("open: could not create sftp session");
		exit(SSH_ERROR);
	    }
	    rc = sftp_init(sftp);
	    if (rc != SSH_OK) {
	    	perror("open: sftp init failed");
		sftp_free(sftp);
		exit(rc);
	    }
	    access_type = O_RDONLY;
	    file = sftp_open(sftp, path,
		           access_type, 0);
	    if (file == NULL) {
		fprintf(stderr, "open: Can't open file for reading: %s\n",
		      ssh_get_error(*(JDFS_DATA->session)));
		return SSH_ERROR;
	    }
	    fd = open(full_path, O_CREAT);
	    if (fd < 0) {
		fprintf(stderr, "Can't open file for writing: %s\n",
		      strerror(errno));
		return SSH_ERROR;
	     }
	     for (;;) {
		  nbytes = sftp_read(file, buffer, sizeof(buffer));
	      	  if (nbytes == 0) {
		  break; // EOF
		  } else if (nbytes < 0) {
		  fprintf(stderr, "Error while reading file: %s\n",
		          ssh_get_error(*(JDFS_DATA->session)));
		  sftp_close(file);
		  return SSH_ERROR;
		  }
		  nwritten = write(fd, buffer, nbytes);
		  if (nwritten != nbytes) {
		      fprintf(stderr, "Error writing: %s\n",
		          strerror(errno));
		      sftp_close(file);
		      return SSH_ERROR;
		  }
	     }
             rc = sftp_close(file);
             if (rc != SSH_OK) {
                 fprintf(stderr, "Can't close the read file: %s\n",
                 ssh_get_error(*(JDFS_DATA->session)));
                 return rc;
             }
	     close(fd);
	     sftp_free(sftp);
     }
     fd = open(full_path, fi->flags); 
     if (fd < 0) {
          fprintf(stderr, "open: Error can't open file");
          return -1;
     }
     fi->fh = fd;
     return 0;
}

/** Read data from an open file */
int jdfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int rc = pread(fi->fh, buf, size, offset);
    if (rc < 0)
	fprintf(stderr, "read: Error can't read file");
    return rc;
}

/** Write data to an open file */
int jdfs_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int rc;
    rc = pwrite(fi->fh, buf, size, offset);
    if (rc < 0)
	fprintf(stderr, "write: Error can't write to file");
    return rc;
}

int jdfs_flush(const char *path, struct fuse_file_info *fi)
{
        int access_type;
        sftp_file file;
        char full_path[2048];
        char buffer[BUFSIZE];
        int nbytes, nwritten;
        int fd;
        int rc;
	close(fi->fh);
        strcpy(full_path, JDFS_DATA->base_path);
        strcat(full_path, path);
	sftp_session sftp = sftp_new(*(JDFS_DATA->session));
	    if (sftp == NULL) {
		perror("flush: could not create sftp session");
		exit(SSH_ERROR);
	    }
	    rc = sftp_init(sftp);
	    if (rc != SSH_OK) {
	    	perror("flush: sftp init failed");
		sftp_free(sftp);
		exit(rc);
	    }
	    access_type = O_WRONLY | O_CREAT | O_TRUNC;
	    file = sftp_open(sftp, path,
			   access_type, S_IRWXU);
	    if (file == NULL) {
		fprintf(stderr, "flush: Can't open file for writing: %s\n",
		      ssh_get_error(*(JDFS_DATA->session)));
		return SSH_ERROR;
	    }
	    fd = open(full_path, O_RDONLY);
	    if (fd < 0) {
		fprintf(stderr, "flush: Can't open file for reading: %s\n",
		      strerror(errno));
		return SSH_ERROR;
	     }
	     for (;;) {
		  nbytes = read(fd, buffer, sizeof(buffer));
	      	  if (nbytes == 0) {
		      break; // EOF
		  } else if (nbytes < 0) {
		      fprintf(stderr, "flush: Error while reading file\n");
		      close(fd);
		      return nbytes;
		  }
		  nwritten = sftp_write(file, buffer, nbytes);
		  if (nwritten != nbytes) {
		      fprintf(stderr, "flush: Error writing to server\n");
		      sftp_close(file);
		      return SSH_ERROR;
		  }
	     }
	     rc = sftp_close(file);
             if (rc != SSH_OK) {
                 fprintf(stderr, "Can't close the read file: %s\n",
                 ssh_get_error(*(JDFS_DATA->session)));
                 return rc;
             }
	     close(fd);
	     sftp_free(sftp);
             rc = unlink(full_path);
             if (rc < 0)
                 fprintf(stderr, "flush: Can't unlink the file:\n");

	     return rc;
}
 
int jdfs_access(const char *path, int mask) 
{
  return 0;

}

int jdfs_getattr(const char* path, struct stat* statbuf)
{
   char cmdstring[4096] = "stat --printf \'%a\n%h\n%u\n%g\n%t\n%s\n%o\n%b\n%X\n%Y\n%Z\n\' ";
   strcat(cmdstring, path);
   char buffer[4096];
   ssh_channel channel;
   int rc;
   channel = ssh_channel_new(*(JDFS_DATA->session));
   if (channel == NULL) return SSH_ERROR;
   rc = ssh_channel_open_session(channel);
   if (rc != SSH_OK)
   {
        ssh_channel_free(channel);
        return rc;
   }
   rc = ssh_channel_request_exec(channel, cmdstring);
   if (rc != SSH_OK)
   {
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      return rc;
   }
   unsigned int nbytes;
   nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
   while (nbytes > 0)
   {
      if (fwrite(buffer, 1, nbytes, stdout) != nbytes)
      {
           ssh_channel_close(channel);
    	   ssh_channel_free(channel);
           return SSH_ERROR;
      }
      nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    }
    if (nbytes < 0)
    {
         ssh_channel_close(channel);
         ssh_channel_free(channel);
         return SSH_ERROR;
    }
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    fprintf(stderr, "stat: %s", buffer);
    return 0;
}
/**
 * Initialize qfilesystem
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
void* jdfs_init(struct fuse_conn_info *conn)
{
    return JDFS_DATA;
}

struct fuse_operations jdfs_oper = {
  .getattr = jdfs_getattr,
//  .readlink = bb_readlink,
//  .getdir = NULL,
//  .mknod = bb_mknod,
  .mkdir = jdfs_mkdir,
  .unlink = jdfs_unlink,
  .rmdir = jdfs_rmdir,
//  .symlink = bb_symlink,
  .rename = jdfs_rename,
//  .link = bb_link,
  .chmod = jdfs_chmod,
  .chown = jdfs_chown,
//  .truncate = bb_truncate,
//  .utime = bb_utime,
  .open = jdfs_open,
  .read = jdfs_read,
  .write = jdfs_write,
  /** Just a placeholder, don't set */ // huh???
//  .statfs = bb_statfs,
  .flush = jdfs_flush,
//  .release = bb_release,
//  .fsync = bb_fsync,  
//  .opendir = bb_opendir,
//  .readdir = bb_readdir,
//  .releasedir = bb_releasedir,
//  .fsyncdir = bb_fsyncdir,
     .init = jdfs_init,
//  .destroy = bb_destroy,
   .access = jdfs_access,
//  .create = bb_create,
//  .ftruncate = bb_ftruncate,
//  .fgetattr = bb_fgetattr
};


int main(int argc, char *argv[])
{
    int rc = 0; 
    char serverAddr[64]; 
    char userName[15];
    strcpy(serverAddr, argv[argc - 2]);
    strcpy(userName, argv[argc - 1]);
    argv[argc -2] = NULL;
    argv[argc - 1] = NULL;
    argc--;
    argc--;
    ssh_session jdfs_session = ssh_new();
    if (jdfs_session == NULL) {
        perror("failed to launch ssh session");
	exit(-1);
    }
    printf("serverAddr : %s\n", serverAddr);
    printf("userName : %s\n", userName);
    ssh_options_set(jdfs_session, SSH_OPTIONS_HOST, serverAddr);
    ssh_options_set(jdfs_session, SSH_OPTIONS_USER, userName);
    rc = ssh_connect(jdfs_session);
    if (rc != SSH_OK)
    {
        fprintf(stderr, "Error connecting to localhost: %s\n",
            ssh_get_error(jdfs_session));
        exit(-1);
    }
    rc = ssh_userauth_autopubkey(jdfs_session, NULL);
    if (rc == SSH_AUTH_ERROR) {
        fprintf(stderr, "main: failed to authenticate user on server: %s\n", ssh_get_error(jdfs_session));
	exit(rc);
    }
    jdfs_state* jdfs_data = malloc(sizeof(struct jdfs_state));
    if (jdfs_data == NULL ) {
        perror("malloc failed");
        exit(-1);
    } 
    jdfs_data->base_path = realpath(argv[3], NULL);
    jdfs_data->session = &jdfs_session;
    fprintf(stderr, "about to call fuse_main\n");
    int fuse_stat = fuse_main(argc, argv, &jdfs_oper, jdfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    ssh_free(jdfs_session);
    return fuse_stat;
}
