#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>



// static int wfs_getattr(const char *path, struct stat *stbuf)
// {
// 	int res;

// 	res = lstat(path, stbuf);
// 	if (res == -1)
// 		return -errno;

// 	return 0;
// }

// static int wfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
// 		       off_t offset, struct fuse_file_info *fi)
// {
// 	DIR *dp;
// 	struct dirent *de;

// 	(void) offset;
// 	(void) fi;

// 	dp = opendir(path);
// 	if (dp == NULL)
// 		return -errno;

// 	while ((de = readdir(dp)) != NULL) {
// 		struct stat st;
// 		memset(&st, 0, sizeof(st));
// 		st.st_ino = de->d_ino;
// 		st.st_mode = de->d_type << 12;
// 		if (filler(buf, de->d_name, &st, 0))
// 			break;
// 	}

// 	closedir(dp);
// 	return 0;
// }

// static int wfs_mknod(const char *path, mode_t mode, dev_t rdev)
// {
// 	int res;

// 	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
// 	   is more portable */
// 	if (S_ISREG(mode)) {
// 		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
// 		if (res >= 0)
// 			res = close(res);
// 	} else if (S_ISFIFO(mode))
// 		res = mkfifo(path, mode);
// 	else
// 		res = mknod(path, mode, rdev);
// 	if (res == -1)
// 		return -errno;

// 	return 0;
// }

static int wfs_mkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

// static int wfs_unlink(const char *path)
// {
// 	int res;

// 	res = unlink(path);
// 	if (res == -1)
// 		return -errno;

// 	return 0;
// }

// static int wfs_rmdir(const char *path)
// {
// 	int res;

// 	res = rmdir(path);
// 	if (res == -1)
// 		return -errno;

// 	return 0;
// }

// static int wfs_read(const char *path, char *buf, size_t size, off_t offset,
// 		    struct fuse_file_info *fi)
// {
// 	int fd;
// 	int res;

// 	(void) fi;
// 	fd = open(path, O_RDONLY);
// 	if (fd == -1)
// 		return -errno;

// 	res = pread(fd, buf, size, offset);
// 	if (res == -1)
// 		res = -errno;

// 	close(fd);
// 	return res;
// }

// static int wfs_write(const char *path, const char *buf, size_t size,
// 		     off_t offset, struct fuse_file_info *fi)
// {
// 	int fd;
// 	int res;

// 	(void) fi;
// 	fd = open(path, O_WRONLY);
// 	if (fd == -1)
// 		return -errno;

// 	res = pwrite(fd, buf, size, offset);
// 	if (res == -1)
// 		res = -errno;

// 	close(fd);
// 	return res;
// }

static struct fuse_operations ops = {
  // .getattr = wfs_getattr,
  // .mknod   = wfs_mknod,
  .mkdir   = wfs_mkdir,
  // .unlink  = wfs_unlink,
  // .rmdir   = wfs_rmdir,
  // .read    = wfs_read,
  // .write   = wfs_write,
  // .readdir = wfs_readdir,
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &ops, NULL);
}

