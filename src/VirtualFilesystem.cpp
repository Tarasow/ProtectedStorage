#include "VirtualFilesystem.h"
#include "GostCipher.h"

#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif
#include <memory>

const size_t bytesIn64Bits = 8;

static void vfs_fullpath(char fpath[PATH_MAX], const char *path) {
	strcpy(fpath, vfsData->rootdir);
	strncat(fpath, path, PATH_MAX);
}

int vfs_getattr(const char* path, struct stat* stats) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int	ret = lstat(fpath, stats);
	return (ret < 0 ? -errno : ret);
}

int vfs_readlink(const char* path, char* link, size_t size) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = readlink(fpath, link, size - 1);
	if (ret >= 0) {
		link[ret] = '\0';
		ret = 0;
	}

	return (ret < 0 ? -errno : ret);
}

int vfs_mknod(const char* path, mode_t mode, dev_t dev) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret;
	if (S_ISREG(mode)) {
		ret = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (ret >= 0) {
			ret = close(ret);
			if (ret < 0)
				return -errno;
		}
	}
	else if (S_ISFIFO(mode))
		ret = mkfifo(fpath, mode);
	else
		ret = mknod(fpath, mode, dev);

	return (ret < 0 ? -errno : ret);
}

int vfs_mkdir(const char* path, mode_t mode) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = mkdir(fpath, mode);
	return (ret < 0 ? -errno : ret);
}

int vfs_unlink(const char* path) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = unlink(fpath);
	return (ret < 0 ? -errno : ret);
}

int vfs_rmdir(const char* path) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = rmdir(fpath);
	return (ret < 0 ? -errno : ret);
}

int vfs_symlink(const char* path, const char* link) {
	char flink[PATH_MAX];
	vfs_fullpath(flink, link);

	int ret = symlink(path, flink);
	return (ret < 0 ? -errno : ret);
}

int vfs_rename(const char* path, const char* npath) {
	char fpath[PATH_MAX], fnewpath[PATH_MAX];

	vfs_fullpath(fpath, path);
	vfs_fullpath(fnewpath, npath);

	int ret = rename(fpath, fnewpath);
	return (ret < 0 ? -errno : ret);
}

int vfs_link(const char* path, const char* npath) {
	char fpath[PATH_MAX], fnewpath[PATH_MAX];
	vfs_fullpath(fpath, path);
	vfs_fullpath(fnewpath, npath);

	int ret = link(fpath, fnewpath);
	return (ret < 0 ? -errno : ret);
}

int vfs_chmod(const char* path, mode_t mode) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = chmod(fpath, mode);
	return (ret < 0 ? -errno : ret);
}

int vfs_chown(const char* path, uid_t uid, gid_t gid) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = chown(fpath, uid, gid);
	return (ret < 0 ? -errno : ret);
}

void truncateMore(const off_t& off, off_t& oldSize, const int& fileDescriptor, char* text, int rsize) {
	for (size_t i = rsize; i < bytesIn64Bits; ++i)
		text[i] = 0;
	if (off < oldSize) {
		gostcipher::encryptAndWrite(text, off % bytesIn64Bits, vfsData->key,
					fileDescriptor, oldSize - static_cast<off_t>(bytesIn64Bits));
		return;
	}
	gostcipher::encryptAndWrite(text, bytesIn64Bits, vfsData->key,
				fileDescriptor, oldSize - static_cast<off_t>(bytesIn64Bits));

	off_t nSize = off - off % bytesIn64Bits;

	if (oldSize < nSize) {
		for (size_t j = 0; j < bytesIn64Bits; ++j)
			text[j] = 0;
		gostcipher::encrypt(text, bytesIn64Bits, vfsData->key);
	}

	while (oldSize < nSize) {
		pwrite(fileDescriptor, text, bytesIn64Bits, oldSize);
		oldSize+=bytesIn64Bits;
	}

	for (size_t j = 0; j < bytesIn64Bits; ++j)
		text[j] = 0;
	gostcipher::encryptAndWrite(text, off % bytesIn64Bits, vfsData->key,
				fileDescriptor, nSize);

}

int vfs_truncate(const char* path, off_t off) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);
	int ret = 0;
	struct stat buf;
	stat(fpath, &buf);
	auto oldSize = buf.st_size;
	int fd = open(fpath, O_RDWR); // течет?
	auto text = std::make_unique<char[]>(bytesIn64Bits);
	int rsize = gostcipher::readAndDecrypt(text.get(), bytesIn64Bits, vfsData->key,
										   fd, oldSize - static_cast<off_t>(bytesIn64Bits));
	if (off == oldSize - static_cast<off_t>(bytesIn64Bits) + rsize)
		return 0;
	if (off < oldSize - static_cast<off_t>(bytesIn64Bits) + rsize) {
		gostcipher::readAndDecrypt(text.get(), bytesIn64Bits, vfsData->key,
					fd, off - off % bytesIn64Bits);
		ret = truncate(fpath, off - off % bytesIn64Bits);
		gostcipher::encryptAndWrite(text.get(), off%bytesIn64Bits,
					vfsData->key, fd, off - off % bytesIn64Bits);
	}
	else {
		ret = 0;
		truncateMore(off, oldSize, fd, text.get(), rsize);
	}
	return (ret < 0 ? -errno : ret);
}

int vfs_utime(const char* path, struct utimbuf* utbuf) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = utime(fpath, utbuf);
	return (ret < 0 ? -errno : ret);
}

int vfs_open(const char* path, struct fuse_file_info* ffinfo) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int fd = open(fpath, ffinfo->flags);
	ffinfo->fh = fd;

	return (fd < 0 ? -errno : 0);
}

int vfs_read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* ffinfo) {
	int ret = gostcipher::readAndDecrypt(buf, size, vfsData->key, ffinfo->fh, off);
	return (ret < 0 ? -errno : ret);
}


int vfs_write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* ffinfo) {
	int ret = gostcipher::encryptAndWrite(const_cast<char*>(buf), size, vfsData->key, ffinfo->fh, off);
	return (ret < 0 ? -errno : size);
}

int vfs_statfs(const char* path, struct statvfs* statv) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = statvfs(fpath, statv); // тут правда возвращается?
	return (ret < 0 ? -errno : ret);
}

int vfs_flush(const char* path, struct fuse_file_info* ffinfo) { // is it needed?
	return 0;
}

int vfs_release(const char* path, struct fuse_file_info* ffinfo) {
	int ret = close(ffinfo->fh);
	return (ret < 0 ? -errno : ret);
}

int vfs_fsync(const char* path, int datasync, struct fuse_file_info* ffinfo) {
	int ret = 0;
	#ifdef HAVE_FDATASYNC
	if (datasync)
		ret = fdatasync(ffinfo->fh);
	else
	#endif
	ret = fsync(ffinfo->fh);
	return (ret < 0 ? -errno : ret);
}

#ifdef HAVE_SYS_XATTR_H

int vfs_setxattr(const char* path, const char* name, const char* value, size_t size, int flags) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = lsetxattr(fpath, name, value, size, flags);
	return (ret < 0 ? -errno : ret);
}

int vfs_getxattr(const char* path, const char* name, char* value, size_t size) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = lgetxattr(fpath, name, value, size);
	return (ret < 0 ? -errno : ret);
}

int vfs_listxattr(const char* path, char* list, size_t size) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = llistxattr(fpath, list, size);
	return (ret < 0 ? -errno : ret);
}

int vfs_removexattr(const char* path, const char* name) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = lremovexattr(fpath, name);
	return (ret < 0 ? -errno : ret);
}

#endif

int vfs_opendir(const char* path, struct fuse_file_info* ffinfo) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	DIR* dir = opendir(fpath);
	ffinfo->fh = (intptr_t)dir;

	return (dir ? 0 : -errno);
}

int vfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t off,
				struct fuse_file_info* ffinfo) {
	DIR* dir = (DIR *) (uintptr_t) ffinfo->fh;
	struct dirent* dnt = readdir(dir);

	if (!dnt)
		return -errno;

	do {
		if ( filler(buf, dnt->d_name, NULL, 0) )
			return -ENOMEM;
	} while ( (dnt = readdir(dir)) );

	return 0;
}

int vfs_releasedir(const char* path, struct fuse_file_info* ffinfo) {
	closedir( (DIR *)(uintptr_t)ffinfo->fh);
	return 0;
}

int vfs_fsyncdir(const char* path, int datasync, struct fuse_file_info* ffinfo) {
	return 0;
}

void* vfs_init(struct fuse_conn_info* conn) {
	return vfsData;
}

void vfs_destroy(void* userData) {
	return;
}

int vfs_access(const char* path, int mask) {
	char fpath[PATH_MAX];
	vfs_fullpath(fpath, path);

	int ret = access(fpath, mask);
	return (ret < 0 ? -errno : ret);
}

int vfs_ftruncate(const char* path, off_t off, struct fuse_file_info* ffinfo) {
	return vfs_truncate(path, off);
}

int vfs_fgetattr(const char* path, struct stat* stats, struct fuse_file_info* ffinfo) {
	if (!strcmp(path, "/"))
		return vfs_getattr(path, stats);

	int ret = fstat(ffinfo->fh, stats);
	return (ret < 0 ? -errno : ret);
}

struct fuse_operations vfsOper = {
	.getattr = vfs_getattr,
	.readlink = vfs_readlink,
	.getdir = NULL,
	.mknod = vfs_mknod,
	.mkdir = vfs_mkdir,
	.unlink = vfs_unlink,
	.rmdir = vfs_rmdir,
	.symlink = vfs_symlink,
	.rename = vfs_rename,
	.link = vfs_link,
	.chmod = vfs_chmod,
	.chown = vfs_chown,
	.truncate = vfs_truncate,
	.utime = vfs_utime,
	.open = vfs_open,
	.read = vfs_read,
	.write = vfs_write,
	.statfs = vfs_statfs,
	.flush = vfs_flush,
	.release = vfs_release,
	.fsync = vfs_fsync,

	#ifdef HAVE_SYS_XATTR_H
	.setxattr = vfs_setxattr,
	.getxattr = vfs_getxattr,
	.listxattr = vfs_listxattr,
	.removexattr = vfs_removexattr,
	#else
	.setxattr = NULL,
	.getxattr = NULL,
	.listxattr = NULL,
	.removexattr = NULL,
	#endif

	.opendir = vfs_opendir,
	.readdir = vfs_readdir,
	.releasedir = vfs_releasedir,
	.fsyncdir = vfs_fsyncdir,
	.init = vfs_init,
	.destroy = vfs_destroy,
	.access = vfs_access,
	.create = NULL,
	.ftruncate = vfs_ftruncate,
	.fgetattr = vfs_fgetattr
};
