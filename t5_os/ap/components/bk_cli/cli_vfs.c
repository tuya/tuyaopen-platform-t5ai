#include "cli.h"

#if CONFIG_VFS

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <cli.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "bk_posix.h"

#include "driver/flash_partition.h"

#if defined(BK_PARTITION_LITTLEFS)
#define BK_PARTITION_FS_ID BK_PARTITION_LITTLEFS
#else
#define BK_PARTITION_FS_ID BK_PARTITION_USR_CONFIG
#endif

static int test_format_lfs(void) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	bk_logic_partition_t *fs_partition = bk_flash_partition_get_info(BK_PARTITION_FS_ID);
	uint32_t flash_start = fs_partition->partition_start_addr;
	uint32_t flash_size = fs_partition->partition_length;
	fs_name = "littlefs";
	partition.part_type = LFS_FLASH;
	partition.part_flash.start_addr = flash_start;
	partition.part_flash.size = flash_size;

	ret = mkfs("PART_NONE", fs_name, &partition);
	
	return ret;
}

static int test_mount_lfs(char *mount_point) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;
	bk_logic_partition_t *fs_partition = bk_flash_partition_get_info(BK_PARTITION_FS_ID);
	uint32_t flash_start = fs_partition->partition_start_addr;
	uint32_t flash_size = fs_partition->partition_length;
	fs_name = "littlefs";
	partition.part_type = LFS_FLASH;
	partition.part_flash.start_addr = flash_start;
	partition.part_flash.size = flash_size;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

#define spi_flash_start 0x1000
#define spi_flash_size	0x100000		// 1 MB

static int test_format_spi_lfs(void) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_SPI_FLASH;
	partition.part_flash.start_addr = spi_flash_start;
	partition.part_flash.size = spi_flash_size;

	ret = mkfs("PART_NONE", fs_name, &partition);
	
	return ret;
}

static int test_mount_spi_lfs(char *mount_point) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_SPI_FLASH;
	partition.part_flash.start_addr = spi_flash_start;
	partition.part_flash.size = spi_flash_size;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_format_qspi_lfs(void) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_QSPI_FLASH;
	partition.part_flash.start_addr = 0;
	partition.part_flash.size = 0x40000;

	ret = mkfs("PART_NONE", fs_name, &partition);

	return ret;
}

static int test_mount_qspi_lfs(char *mount_point) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_QSPI_FLASH;
	partition.part_flash.start_addr = 0;
	partition.part_flash.size = 0x40000;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_format_qspi0_part1_lfs(void) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_QSPI_FLASH;
	partition.part_flash.start_addr = 0x80000;
	partition.part_flash.size = 0x40000;

	ret = mkfs("PART_NONE", fs_name, &partition);

	return ret;
}

static int test_mount_qspi0_part1_lfs(char *mount_point) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_QSPI_FLASH;
	partition.part_flash.start_addr = 0x80000;
	partition.part_flash.size = 0x40000;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_format_fatfs(void) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_SDCARD;

	ret = mkfs("PART_NONE", fs_name, &partition);
	
	return ret;
}

static int test_mount_fatfs(char *mount_point) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_SDCARD;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_format_flash_fatfs(void) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_FLASH;

	ret = mkfs("PART_NONE", fs_name, &partition);
	
	return ret;
}

static int test_mount_flash_fatfs(char *mount_point) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_FLASH;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_format_qspi0_fatfs(void) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_QSPI0_FLASH;

	ret = mkfs("PART_NONE", fs_name, &partition);
	
	return ret;
}

static int test_mount_qspi0_fatfs(char *mount_point) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_QSPI0_FLASH;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_umount_vfs(char *mount_point) {
	int ret;
	
	ret = umount(mount_point);
	return ret;
}

static int show_file(int fd) {
	#define CHUNK 18
	char buffer[CHUNK+1];
	int len;
	int total = 0;
	char *ptr;
	char *ptr2;

	buffer[CHUNK] = '\0';
	while(1) {
		len = read(fd, buffer, CHUNK);
		if (len <= 0)
			break;

		if (len == 0)
			break;

		total += len;

		ptr = buffer;
		while(ptr < buffer + len) {
			ptr2 = strchr(ptr, '\0');
			if (!ptr2)	//impossible
				break;
			if (ptr2 < buffer + len) {
				BK_LOGD(NULL, "%s\n", ptr);
			} else {
				BK_LOGD(NULL, "%s", ptr);
			}
			ptr = ptr2 + 1;
		}
	}

	return total;
	#undef CHUNK
}

static int test_read_vfs(char *file_name)
{
	int fd;
	int ret;

	fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		BK_LOGD(NULL, "can't open %s\n", file_name);
		return -1;
	}
	
	ret = show_file(fd);
	//BK_LOGD(NULL, "read from %s, ret=%d\n", file_name, ret);
	close(fd);
	return ret;
}

static int test_write_vfs(char *file_name, char *content)
{
	int fd;
	int ret;

	fd = open(file_name, O_RDWR | O_CREAT | O_APPEND);
	if (fd < 0) {
		BK_LOGD(NULL, "can't open %s\n", file_name);
		return -1;
	}
	
	ret = write(fd, content, strlen(content) + 1);
	//BK_LOGD(NULL, "write to %s, ret=%d\n", file_name, ret);
	close(fd);
	return ret;
}

static int test_unlink_vfs(char *file_name)
{
	int ret;

	ret = unlink(file_name);
	return ret;
}


void cli_vfs_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret;

	if (argc < 2) {
		BK_LOGD(NULL, "usage : vfs format|mount|umount|read|write|unlink\n");
		return;
	}

	if (os_strcmp(argv[1], "format") == 0) {
		if (argc < 3) {
			BK_LOGD(NULL, "usage : vfs format lfs|fatfs\n");
			return;
		}
		if (os_strcmp(argv[2], "lfs") == 0)
			ret = test_format_lfs();
		else if (os_strcmp(argv[2], "spi_lfs") == 0)
			ret = test_format_spi_lfs();
		else if (os_strcmp(argv[2], "qspi_lfs") == 0)
			ret = test_format_qspi_lfs();
		else if (os_strcmp(argv[2], "qspi0_1_lfs") == 0)
			ret = test_format_qspi0_part1_lfs();
		else if (os_strcmp(argv[2], "fatfs") == 0)
			ret = test_format_fatfs();
		else if (os_strcmp(argv[2], "qspi0_fatfs") == 0)
			ret = test_format_qspi0_fatfs();
		else if (os_strcmp(argv[2], "flash_fatfs") == 0)
			ret = test_format_flash_fatfs();
		else {
			BK_LOGD(NULL, "usage : vfs format lfs|fatfs\n");
			return;
		}
		BK_LOGD(NULL, "format ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "mount") == 0) {
		char *mount_point;
		
		if (argc < 4) {
			BK_LOGD(NULL, "usage : vfs mount lfs|fatfs MOUNT_POINT\n");
			return;
		}
		mount_point = argv[3];

		if (os_strcmp(argv[2], "lfs") == 0)
			ret = test_mount_lfs(mount_point);
		else if (os_strcmp(argv[2], "spi_lfs") == 0)
			ret = test_mount_spi_lfs(mount_point);
		else if (os_strcmp(argv[2], "qspi_lfs") == 0)
			ret = test_mount_qspi_lfs(mount_point);
		else if (os_strcmp(argv[2], "qspi0_1_lfs") == 0)
			ret = test_mount_qspi0_part1_lfs(mount_point);
		else if (os_strcmp(argv[2], "fatfs") == 0)
			ret = test_mount_fatfs(mount_point);
		else if (os_strcmp(argv[2], "qspi0_fatfs") == 0)
			ret = test_mount_qspi0_fatfs(mount_point);
		else if (os_strcmp(argv[2], "flash_fatfs") == 0)
			ret = test_mount_flash_fatfs(mount_point);
		else {
			BK_LOGD(NULL, "usage : vfs mount lfs|fatfs MOUNT_POINT\n");
			return;
		}
		BK_LOGD(NULL, "mount ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "umount") == 0) {
		char *mount_point;

		if (argc < 3) {
			BK_LOGD(NULL, "usage : vfs umount MOUNT_POINT\n");
			return;
		}
		
		mount_point = argv[2];

		ret = test_umount_vfs(mount_point);
		BK_LOGD(NULL, "umount ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "read") == 0) {
		char *file_name;
		
		if (argc < 3) {
			BK_LOGD(NULL, "usage : vfs read FULL_FILE_NAME\n");
			return;
		}
		file_name = argv[2];

		ret = test_read_vfs(file_name);
		BK_LOGD(NULL, "read ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "write") == 0) {
		char *file_name;
		char *content;
		
		if (argc < 4) {
			BK_LOGD(NULL, "usage : vfs write FULL_FILE_NAME CONTENT\n");
			return;
		}
		file_name = argv[2];
		content = argv[3];

		ret = test_write_vfs(file_name, content);
		BK_LOGD(NULL, "write ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "unlink") == 0) {
		char *file_name;
		
		if (argc < 3) {
			BK_LOGD(NULL, "usage : vfs unlink FULL_FILE_NAME\n");
			return;
		}
		file_name = argv[2];

		ret = test_unlink_vfs(file_name);
		BK_LOGD(NULL, "unlink ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "threadsafe_test") == 0) {
		uint32_t task_count = os_strtoul(argv[2], NULL, 10);
		uint32_t file_size = os_strtoul(argv[3], NULL, 10);
		void test_vfs_concurrency(uint32_t task_count, uint32_t file_size);
		test_vfs_concurrency(task_count, file_size);

	} else {
		BK_LOGD(NULL, "vfs unknown sub cmd %s\n", argv[1]);
	}
}

#if 1
#define VFS_TEST_CONCURRENCY_TASK_MAX_CNT (8)
static beken_thread_t s_vfs_test_task_handle[VFS_TEST_CONCURRENCY_TASK_MAX_CNT];

#define VFS_TEST_CONCURRENCY_MEM_LEN (1024)
#define VFS_TEST_CONCURRENCY_FILE_LEN (VFS_TEST_CONCURRENCY_MEM_LEN * 16)	//default:128k bytes
#define VFS_TEST_CONCURRENCY_FILE_LEN_MAX (VFS_TEST_CONCURRENCY_MEM_LEN * 1024 * 128)	//MAX:128M bytes

static uint32_t *s_vfs_test_src_p[VFS_TEST_CONCURRENCY_TASK_MAX_CNT];
static uint32_t *s_vfs_test_tar_p[VFS_TEST_CONCURRENCY_TASK_MAX_CNT];

//arg: ((DISK_NUMBER << 16) | task_id)
static void vfs_test_concurrency_task(beken_thread_arg_t arg)
{
	uint32_t task_id = ((uint32_t)arg & 0xffff)%VFS_TEST_CONCURRENCY_TASK_MAX_CNT;
	char cFileName[256];
	int fd;
	uint64_t left_len = 0;
	unsigned int uiTemp = 0;
	uint32_t i;
	int fr = 0;

	CLI_LOGI("taskid=%d\r\n", task_id);
	sprintf(cFileName, "/vfs_test_task_%u.txt", task_id);
	while (1) {
		//init mem value
		for(i = 0; i < VFS_TEST_CONCURRENCY_MEM_LEN/4; i++)
			*(s_vfs_test_src_p[task_id] + i) = (uint32_t)(s_vfs_test_src_p[task_id] + i);

		//open file
		CLI_LOGI("open \"%s\"\r\n", cFileName);
		fd = open(cFileName, O_RDWR | O_CREAT);
		if (fd < 0) {
			CLI_LOGE("open failed fd = %d\r\n", fd);
			continue;
		}

		//write to VFS
		left_len = VFS_TEST_CONCURRENCY_FILE_LEN;
        do
        {
            uiTemp = write(fd, s_vfs_test_src_p[task_id], VFS_TEST_CONCURRENCY_MEM_LEN);
            if ((uiTemp) /*&& (uiTemp == VFS_TEST_CONCURRENCY_MEM_LEN)*/)
            {
                left_len -= uiTemp;
            }
            else
            {
                CLI_LOGE("write failed uiTemp=%d,left_len=%d\r\n", uiTemp, left_len);
                continue;
           }

//avoid file too large cause watchdog timeout
#if (CONFIG_TASK_WDT)
{
           extern void bk_task_wdt_feed(void);
           bk_task_wdt_feed();
}
#endif
            CLI_LOGI("task_id=%d,left_len=%d,write_len=%d\r\n", task_id, (uint32_t)left_len, uiTemp);

            //let other task runs
            rtos_thread_msleep(100);

        }
        while (left_len);

        fr = close(fd);
		if (fr)
		{
			CLI_LOGE("close fail fr = %d\r\n", fr);
			continue;
		}

#if CONFIG_WDT
		bk_wdt_feed();
#if CONFIG_TASK_WDT
		bk_task_wdt_feed();
#endif
#endif

		//read from SDCARD and compare
		CLI_LOGI("open \"%s\"\r\n", cFileName);

		fd = open(cFileName, O_RDONLY);
		if (fd < 0)
		{
			CLI_LOGE("open failed fd = %d\r\n", fd);
			continue;
		}

		left_len = VFS_TEST_CONCURRENCY_FILE_LEN;
		do
		{
			fr = read(fd, s_vfs_test_tar_p[task_id], VFS_TEST_CONCURRENCY_MEM_LEN);
			if (fr > 0)
			{
				left_len -= fr;
			}
			else
			{
				CLI_LOGE("read failed fr = %d,uiTemp=%d,left_len=%d\r\n", fr, uiTemp, left_len);
				continue;
			}

			//compare
			for(uint32_t i = 0; i < VFS_TEST_CONCURRENCY_MEM_LEN/4; i++)
			{
				if(*(s_vfs_test_src_p[task_id] + i) != *(s_vfs_test_tar_p[task_id] + i))
				{
					CLI_LOGE("compare failed src = 0x%x,tar=0x%x,i=%d\r\n", *(s_vfs_test_src_p[task_id] + i), *(s_vfs_test_tar_p[task_id] + i), i);
				}
			}
		}
		while (left_len);

        fr = close(fd);
        if (fr)
        {
            CLI_LOGI("close failed 1 fr = %d\r\n", fr);
            continue;
        }

#if CONFIG_WDT
		bk_wdt_feed();
#if CONFIG_TASK_WDT
		bk_task_wdt_feed();
#endif
#endif

		rtos_delay_milliseconds(20);
	}

	s_vfs_test_task_handle[task_id] = NULL;
	rtos_delete_thread(NULL);

	if(s_vfs_test_src_p[task_id])
	{
		os_free(s_vfs_test_src_p[task_id]);
		s_vfs_test_src_p[task_id] = NULL;
	}

	if(s_vfs_test_tar_p[task_id])
	{
		os_free(s_vfs_test_tar_p[task_id]);
		s_vfs_test_tar_p[task_id] = NULL;
	}
}

void test_vfs_concurrency(uint32_t task_count, uint32_t file_size)
{
	bk_err_t ret = BK_OK;
	if(task_count > VFS_TEST_CONCURRENCY_TASK_MAX_CNT)
		task_count = VFS_TEST_CONCURRENCY_TASK_MAX_CNT;	//max is 8

	for(uint32_t i = 0; i  < task_count; i++)
	{
		if (!s_vfs_test_task_handle[i])
		{
			ret = rtos_create_thread(&s_vfs_test_task_handle[i],
									 4+i,
									 "vfs_test",
									 (beken_thread_function_t)vfs_test_concurrency_task,
									 2 * 1024,
									 (beken_thread_arg_t)((1 << 16) | i));
			if (ret != BK_OK)
			{
				s_vfs_test_task_handle[i] = NULL;
				CLI_LOGE("Failed to create vfs test task: %d\r\n", ret);
				return;
			}

			if(s_vfs_test_src_p[i] == NULL)
				s_vfs_test_src_p[i] = (uint32_t *)os_malloc(VFS_TEST_CONCURRENCY_MEM_LEN);
			BK_ASSERT(s_vfs_test_src_p[i]);

			if(s_vfs_test_tar_p[i] == NULL)
				s_vfs_test_tar_p[i] = (uint32_t *)os_malloc(VFS_TEST_CONCURRENCY_MEM_LEN);
			BK_ASSERT(s_vfs_test_tar_p[i]);

			CLI_LOGI("s_vfs_test_src_p[%d]=0x%x,tar=0x%x\r\n", i, s_vfs_test_src_p[i], s_vfs_test_tar_p[i]);
		}
	}
}
#endif

#define VFS_CMD_CNT (sizeof(vfs_commands) / sizeof(struct cli_command))
static const struct cli_command vfs_commands[] = {
	{"vfs", "vfs format|mount|umount|read|write|unlink|threadsafe_test", cli_vfs_test},
};

int cli_vfs_init(void)
{
	return cli_register_commands(vfs_commands, VFS_CMD_CNT);
}

#endif
