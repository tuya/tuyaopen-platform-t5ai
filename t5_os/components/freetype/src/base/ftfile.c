
#include <stdio.h>
#include <os/os.h>
#include <os/mem.h>

#include "freetype/config/ftstdlib.h"

#include "freetype/ftfile.h"
#include "freetype/ftmem.h"

//int fclose(FILE * /*stream*/)

//FILE *fopen(const char * __restrict /*filename*/,
//                           const char * __restrict /*mode*/)


//size_t fread(void * __restrict /*ptr*/,
//                    size_t /*size*/, size_t /*nmemb*/, FILE * __restrict /*stream*/)

//int fseek(FILE * /*stream*/, long int /*offset*/, int /*whence*/)


//long int ftell(FILE * /*stream*/)


//打开文件
FIL *ft_fs_open (const char *filename,const char *mode)
{
	uint8_t flags = 0;
#if 0
    if(mode == "os_strstr") flags = FA_WRITE | FA_OPEN_ALWAYS;
    else if(mode == LV_FS_MODE_RD) flags = FA_READ;
    else if(mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;
#else
	flags = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;
#endif
    FIL * file = os_malloc(sizeof(FIL));
    if(file == NULL) return NULL;

    FRESULT res = f_open(file, filename, flags);
    if(res == FR_OK) {
		bk_printf("open file %s ok\r\n", filename);
        return file;
    }
    else {
		os_free(file);
		file = NULL;
        return NULL;
    }
	bk_printf("open file %s fail\r\n", filename);
	return NULL;
}


//关闭文件
int ft_fs_close (FIL *file)
{
    if (file)
    {
        f_close(file);
		os_free(file);
		file = NULL;
        return 0;
    }else{
		bk_printf("close file fail\r\n");
	}
    return -1;
}


//读取文件
//缓冲指针，缓冲元素大小，元素个数
unsigned int ft_fs_read (void *ptr,unsigned int size, unsigned int nmemb, FIL *file)
{
	UINT rb = 0;
	if(!file){
		bk_printf("read file fail\r\n");
		return -1;
	}
	//bk_printf("wand read file len:%u, %u\r\n", size, nmemb);
	FRESULT res = f_read(file, ptr, size*nmemb, &rb);
	//bk_printf("real read file len:%u\r\n", rb);
    if(res == FR_OK) return rb;
    else return -1;

}


//文件偏移
int ft_fs_seek (FIL *file,long int offset,int whence)
{
	if (file){
		switch(whence) {
			case SEEK_SET:
				f_lseek(file, offset);
				break;
			case SEEK_CUR:
				f_lseek(file, f_tell(file) + offset);
				break;
			case SEEK_END:
				f_lseek(file, f_size(file) + offset);
				break;
			default:
				break;
		}
		return 0;
	}

    return -1;
}


//返回相对与文件开头的偏移
long int ft_fs_tell (FIL *file)
{
    if (file)
    {
		return f_tell(file);
    }
    return -1;
}

