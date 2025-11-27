#ifndef __BK_FILESYSTEM_H_
#define __BK_FILESYSTEM_H_

#include <stddef.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
//#include <sys/statfs.h>

#include "bk_std_header.h"	//for struct statfs, DIR, dirent

#define VFS_DIFFERENT_MOUNT 0
#define VFS_REPEAT_MOUNT    1
#define VFS_EXCEPTION_MOUNT 2

struct bk_filesystem;
struct bk_file;

typedef struct {
	struct bk_filesystem *filesystem;

	struct dirent dir_entry;

	void *dir_data;
} bk_dir;

struct bk_filesystem_ops {
	int (*mount)	(struct bk_filesystem *fs, unsigned long mount_flags, const void *data);
	int (*unmount)	(struct bk_filesystem *fs);
	int (*unmount2)	(struct bk_filesystem *fs, int flag);
	int (*check_repeat_mount)(struct bk_filesystem *fs, const void *data);

	int (*mkfs)		(const char *partition_name, const void *data);
	int (*statfs)	(struct bk_filesystem *fs, struct statfs *buf);
};

struct bk_filesystem {
	char *mount_point;
	const struct bk_filesystem_ops *fs_ops;
	struct bk_file_ops *f_ops;
	uint32_t extra_ref_count;
	void *fs_data;
};

struct bk_file_ops {
	int (*open)		(struct bk_file *file, const char *path, int oflag);
	int (*close)	(struct bk_file *file);
	ssize_t (*read)	(struct bk_file *file, void *buf, size_t count);
	ssize_t (*write)(struct bk_file *file, const void *buf, size_t count);
	off_t (*lseek)	(struct bk_file *file, off_t offset, int whence);

	int (*unlink)	(struct bk_filesystem *fs, const char *pathname);
	int (*stat)		(struct bk_filesystem *fs, const char *pathname, struct stat *statbuf);
	int (*rename)	(struct bk_filesystem *fs, const char *oldpath, const char *newpath);

	int (*fsync)	(struct bk_file *file);
	int (*ftruncate)(struct bk_file *file, off_t offset);
	int (*fcntl)	(struct bk_file *file, int cmd, void *args);	//cmd : F_GETFL, F_SETFL, fs special

	int (*opendir)	(bk_dir *dir, const char *pathname);
	int (*readdir)	(bk_dir *dir, struct dirent *entry);
	int (*closedir)	(bk_dir *dir);

	int (*mkdir)	(struct bk_filesystem *fs, const char *pathname);
	int (*rmdir)	(struct bk_filesystem *fs, const char *pathname);
	off_t (*ftell)	(struct bk_file *file);
	int (*feof)	(struct bk_file *file);
};

#define BK_FILE_MAGIC 0xADCB

struct bk_file {
	unsigned short magic;

	char *path;

	struct bk_filesystem *filesystem;
	struct bk_file_ops *f_ops;

	uint32_t flags;
	size_t size;
	off_t pos;

	void *f_data;
};

struct filesystem_proto {
	const char *fs_type;
	struct bk_filesystem_ops *fs_ops;
	struct bk_file_ops *f_ops;
};

int bk_filesystem_init(void);
int bk_register_filesystem(const char *fs_type, struct bk_filesystem_ops *fs_ops, struct bk_file_ops *f_ops);

/**
 * @brief Mount a file system.
 *
 * This API mount a file system:
 *   - 
 *
 * @attention 1. This API should be called before any other file operations(read/write) APIs.
 *
 * @param source:The mount file system path, I.E:INTERNAL_FLASH_PATITION_0,SPI_FLASH_0_PATITION_0...
 *        target:The target mount file system path, which is used by APP read,write file.
 *               BK72xx MCU file system can customize the path as the  APP requirements.
 *               I.E:/0/, /1/, C:/, D:/
 *        fs_type:FS_TYPE_LFS or FS_TYPE_FATFS
 *        mount_flags:MS_RW/MS_RDONLY/MS_NOEXEC/...
 *        data:The special file system(lfs, fatfs...) private data
 * @return
 *    - 0: succeed
 *    - others: other errors.
 */
int bk_vfs_mount(const char *source, const char *target,
                 const char *fs_type, unsigned long mount_flags,
                 const void *data);
int bk_vfs_umount(const char *target);
int bk_vfs_umount2(const char *target, int flags);

struct bk_filesystem *bk_vfs_lookup(const char *path);

int bk_vfs_mkfs(const char *partition_name, const char *fs_type, const void *data);
int bk_vfs_statfs(const char *path, struct statfs *buf);

#endif
