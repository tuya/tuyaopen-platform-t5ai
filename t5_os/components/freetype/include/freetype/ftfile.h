#ifndef FT_FILE_H__
#define FT_FILE_H__

#include <os/os.h>
#include "ff.h"

//打开文件
FIL *ft_fs_open (const char *filename,const char *mode);

//关闭文件
int ft_fs_close (FIL *file);

//读取文件
//缓冲指针，缓冲元素大小，元素个数
unsigned int ft_fs_read (void *ptr,unsigned int size,unsigned int nmemb, FIL *file);

//文件偏移
int ft_fs_seek (FIL *file,long int offset,int whence);

//返回相对与文件开头的偏移
long int ft_fs_tell (FIL *file);











#endif