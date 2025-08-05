/**
* @file tkl_fs.h
* @brief Common process - adapter the file operation api provide by OS
* @version 0.1
* @date 2020-11-09
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/

#ifndef __TKL_FS_H__
#define __TKL_FS_H__


#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DEV_INNER_FLASH,
    DEV_EXT_FLASH,
    DEV_SDCARD,
} FS_DEV_TYPE_T;

/********************************************************************************
 *********************************tuya_os_fs_intf********************************
 ********************************************************************************/

/**
* @brief Make directory
*
* @param[in] path: path of directory
*
* @note This API is used for making a directory
*
* @return 0 on success. Others on failed
*/
INT_T tkl_fs_mkdir(CONST CHAR_T* path);

/**
* @brief Make directory recursively
*
* @param[in] path: path of directory
*
* @note This API is used for making a directory
*
* @return 0 on success. Others on failed
*/
INT_T tkl_fs_mkdir_r(CONST CHAR_T* path);

/**
* @brief Remove directory
*
* @param[in] path: path of directory
*
* @note This API is used for removing a directory
*
* @return 0 on success. Others on failed
*/
INT_T tkl_fs_remove(CONST CHAR_T* path);

/**
* @brief Remove directory recursively
*
* @param[in] path: path of directory
*
* @note This API is used for removing a directory
*
* @return 0 on success. Others on failed
*/
INT_T tkl_fs_remove_r(CONST CHAR_T* path);


/**
* @brief Get file mode
*
* @param[in] path: path of directory
* @param[out] mode: bit attibute of directory
*
* @note This API is used for getting file mode.
*
* @return 0 on success. Others on failed
*/
INT_T tkl_fs_mode(CONST CHAR_T* path, UINT_T* mode);

/**
* @brief Check whether the file or directory exists
*
* @param[in] path: path of directory
* @param[out] is_exist: the file or directory exists or not
*
* @note This API is used to check whether the file or directory exists.
*
* @return 0 on success. Others on failed
*/
INT_T tkl_fs_is_exist(CONST CHAR_T* path, BOOL_T* is_exist);

/**
* @brief File rename
*
* @param[in] path_old: old path of directory
* @param[in] path_new: new path of directory
*
* @note This API is used to rename the file.
*
* @return 0 on success. Others on failed
*/
INT_T tkl_fs_rename(CONST CHAR_T* path_old, CONST CHAR_T* path_new);

/**
* @brief Open directory
*
* @param[in] path: path of directory
* @param[out] dir: handle of directory
*
* @note This API is used to open a directory
*
* @return 0 on success. Others on failed
*/
INT_T tkl_dir_open(CONST CHAR_T* path, TUYA_DIR* dir);

/**
* @brief Close directory
*
* @param[in] dir: handle of directory
*
* @note This API is used to close a directory
*
* @return 0 on success. Others on failed
*/
INT_T tkl_dir_close(TUYA_DIR dir);


/**
* @brief Read directory
*
* @param[in] dir: handle of directory
* @param[out] info: file information
*
* @note This API is used to read a directory.
* Read the file information of the current node, and the internal pointer points to the next node.
*
* @return 0 on success. Others on failed
*/
INT_T tkl_dir_read(TUYA_DIR dir, TUYA_FILEINFO* info);

/**
* @brief Get the name of the file node
*
* @param[in] info: file information
* @param[out] name: file name
*
* @note This API is used to get the name of the file node.
*
* @return 0 on success. Others on failed
*/
INT_T tkl_dir_name(TUYA_FILEINFO info, CONST CHAR_T** name);

/**
* @brief Check whether the node is a directory
*
* @param[in] info: file information
* @param[out] is_dir: is directory or not
*
* @note This API is used to check whether the node is a directory.
*
* @return 0 on success. Others on failed
*/
INT_T tkl_dir_is_directory(TUYA_FILEINFO info, BOOL_T* is_dir);

/**
* @brief Check whether the node is a normal file
*
* @param[in] info: file information
* @param[out] is_regular: is normal file or not
*
* @note This API is used to check whether the node is a normal file.
*
* @return 0 on success. Others on failed
*/
INT_T tkl_dir_is_regular(TUYA_FILEINFO info, BOOL_T* is_regular);

/**
* @brief Open file
*
* @param[in] path: path of file
* @param[in] mode: file open mode: "r","w"...
*
* @note This API is used to open a file
*
* @return the file handle, NULL means failed
*/
TUYA_FILE tkl_fopen(CONST CHAR_T* path, CONST CHAR_T* mode);

/**
* @brief Close file
*
* @param[in] file: file handle
*
* @note This API is used to close a file
*
* @return 0 on success. EOF on failed
*/
INT_T tkl_fclose(TUYA_FILE file);

/**
* @brief Read file
*
* @param[in] buf: buffer for reading file
* @param[in] bytes: buffer size
* @param[in] file: file handle
*
* @note This API is used to read a file
*
* @return the bytes read from file
*/
INT_T tkl_fread(VOID_T* buf, INT_T bytes, TUYA_FILE file);

/**
* @brief write file
*
* @param[in] buf: buffer for writing file
* @param[in] bytes: buffer size
* @param[in] file: file handle
*
* @note This API is used to write a file
*
* @return the bytes write to file
*/
INT_T tkl_fwrite(VOID_T* buf, INT_T bytes, TUYA_FILE file);

/**
* @brief write buffer to flash
*
* @param[in] fd: file fd
*
* @note This API is used to write buffer to flash
*
* @return 0 on success. others on failed
*/
INT_T tkl_fsync(INT_T fd);

/**
* @brief Read string from file
*
* @param[in] buf: buffer for reading file
* @param[in] len: buffer size
* @param[in] file: file handle
*
* @note This API is used to read string from file
*
* @return the content get from file, NULL means failed
*/
CHAR_T* tkl_fgets(CHAR_T* buf, INT_T len, TUYA_FILE file);

/**
* @brief Check wheather to reach the end fo the file
*
* @param[in] file: file handle
*
* @note This API is used to check wheather to reach the end fo the file
*
* @return 0 on not eof, others on eof
*/
INT_T tkl_feof(TUYA_FILE file);

/**
* @brief Seek to the offset position of the file
*
* @param[in] file: file handle
* @param[in] offs: offset
* @param[in] whence: seek start point mode
*
* @note This API is used to seek to the offset position of the file.
*
* @return 0 on success, others on failed
*/
INT_T tkl_fseek(TUYA_FILE file, INT64_T offs, INT_T whence);

/**
* @brief Get current position of file
*
* @param[in] file: file handle
*
* @note This API is used to get current position of file.
*
* @return the current offset of the file
*/
INT64_T tkl_ftell(TUYA_FILE file);

/**
* @brief Get file size
*
* @param[in] filepath file path + file name
*
* @note This API is used to get the size of file.
*
* @return the sizeof of file
*/
INT_T tkl_fgetsize(CONST CHAR_T *filepath);

/**
* @brief Judge if the file can be access
*
* @param[in] filepath file path + file name
*
* @param[in] mode access mode
*
* @note This API is used to access one file.
*
* @return 0 success,-1 failed
*/
INT_T tkl_faccess(CONST CHAR_T *filepath, INT_T mode);

/**
* @brief read the next character from stream
*
* @param[in] file char stream
*
* @note This API is used to get one char from stream.
*
* @return as an unsigned char cast to a int ,or EOF on end of file or error
*/
INT_T tkl_fgetc(TUYA_FILE file);

/**
* @brief flush the IO read/write stream
*
* @param[in] file char stream
*
* @note This API is used to flush the IO read/write stream.
*
* @return 0 success,-1 failed
*/
INT_T tkl_fflush(TUYA_FILE file);

/**
* @brief get the file fd
*
* @param[in] file char stream
*
* @note This API is used to get the file fd.
*
* @return the file fd
*/
INT_T tkl_fileno(TUYA_FILE file);


/**
* @brief truncate one file according to the length
*
* @param[in] fd file description
*
* @param[in] length the length want to truncate
*
* @note This API is used to truncate one file.
*
* @return 0 success,-1 failed
*/
INT_T tkl_ftruncate(INT_T fd, UINT64_T length);

/**
* @brief mount file system
*
* @param[in] mount point
*
* @param[in] device type fs based
*
* @note This API is used to mount file system.
*
* @return 0 success,-1 failed
*/
INT_T tkl_fs_mount(CONST CHAR_T *path, FS_DEV_TYPE_T dev_type);

/**
* @brief unmount file system
*
* @param[in] mount point
*
* @note This API is used to unmount file system.
*
* @return 0 success,-1 failed
*/
INT_T tkl_fs_unmount(CONST CHAR_T *path);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif // 

