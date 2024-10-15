/*
 * File      : fal_partition.c
 * This file is part of FAL (Flash Abstraction Layer) package
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 */

#include <fal.h>
#include <string.h>
#include <stdlib.h>

/* partition magic word */
#define FAL_PART_MAGIC_WORD         0x45503130
#define FAL_PART_MAGIC_WORD_H       0x4550L
#define FAL_PART_MAGIC_WORD_L       0x3130L
#define FAL_PART_MAGIC_WROD         0x45503130

static struct fal_partition *partition_table = NULL;
static uint8_t init_ok = 0;
static size_t partition_table_len = 0;
/**
 * print the partition table
 */
void fal_show_part_table(void)
{
        char *item1 = "name", *item2 = "flash_dev";
        size_t i, part_name_max = strlen(item1), flash_dev_name_max = strlen(item2);
        const struct fal_partition *part;

        if (partition_table_len)
        {
                for (i = 0; i < partition_table_len; i++)
                {
                        part = &partition_table[i];
                        if (strlen(part->name) > part_name_max)
                        {
                            part_name_max = strlen(part->name);
                        }
                        if (strlen(part->flash_name) > flash_dev_name_max)
                        {
                            flash_dev_name_max = strlen(part->flash_name);
                        }
                }
        }
        log_i("== FAL partition table ==");
        log_i("| %-*.*s | %-*.*s |off|len|", part_name_max, FAL_DEV_NAME_MAX, item1, flash_dev_name_max,
                FAL_DEV_NAME_MAX, item2);
        log_i("--");
        for (i = 0; i < partition_table_len; i++)
        {

#ifdef FAL_PART_HAS_TABLE_CFG
            part = &partition_table[i];
#else
            part = &partition_table[partition_table_len - i - 1];
#endif

            log_i("| %-*.*s | %-*.*s | 0x%08lx | 0x%08x |", part_name_max, FAL_DEV_NAME_MAX, part->name, flash_dev_name_max,
                    FAL_DEV_NAME_MAX, part->flash_name, part->offset, part->len);
        }
        log_i("==");
}

    /**
     * Initialize all flash partition on FAL partition table
     *
     * @return partitions total number
     */
int fal_partition_init(void)
{
        size_t i;
        const struct fal_flash_dev *flash_dev = NULL;

        if (init_ok)
        {
            return partition_table_len;
        }

#ifdef FAL_PART_HAS_TABLE_CFG
        partition_table = &partition_table_def[0];
        partition_table_len = sizeof(partition_table_def) / sizeof(partition_table_def[0]);
#else
        /* load partition table from the end address FAL_PART_TABLE_END_OFFSET, error return 0 */
        long part_table_offset = FAL_PART_TABLE_END_OFFSET;
        size_t table_num = 0, table_item_size = 0;
        uint8_t part_table_find_ok = 0;
        uint32_t read_magic_word;
        fal_partition_t new_part = NULL;

        flash_dev = fal_flash_device_find(FAL_PART_TABLE_FLASH_DEV_NAME);
        if (flash_dev == NULL)
        {
            log_e("# (%s)", FAL_PART_TABLE_FLASH_DEV_NAME);
            goto _exit;
        }

        /* check partition table offset address */
        if (part_table_offset < 0 || part_table_offset >= (long) flash_dev->len)
        {
            log_e("#  (%ld) (<%d).", part_table_offset, flash_dev->len);
            goto _exit;
        }

        table_item_size = sizeof(struct fal_partition);
        new_part = (fal_partition_t)FAL_MALLOC(table_item_size);
        if (new_part == NULL)
        {
            log_e("#");
            goto _exit;
        }

        /* find partition table location */
        {
            uint8_t read_buf[64];

            part_table_offset -= sizeof(read_buf);
            while (part_table_offset >= 0)
            {
                if (flash_dev->ops.read(part_table_offset, read_buf, sizeof(read_buf)) > 0)
                {
                    /* find magic word in read buf */
                    for (i = 0; i < sizeof(read_buf) - sizeof(read_magic_word) + 1; i++)
                    {
                        read_magic_word = read_buf[0 + i] + (read_buf[1 + i] << 8) + (read_buf[2 + i] << 16) + (read_buf[3 + i] << 24);
                        if (read_magic_word == ((FAL_PART_MAGIC_WORD_H << 16) + FAL_PART_MAGIC_WORD_L))
                        {
                            part_table_find_ok = 1;
                            part_table_offset += i;
                            log_d("#  '%s'  @0x%08lx.", FAL_PART_TABLE_FLASH_DEV_NAME,
                                    part_table_offset);
                            break;
                        }
                    }
                }
                else
                {
                    /* read failed */
                    break;
                }

                if (part_table_find_ok)
                {
                    break;
                }
                else
                {
                    /* calculate next read buf position */
                    if (part_table_offset >= (long)sizeof(read_buf))
                    {
                        part_table_offset -= sizeof(read_buf);
                        part_table_offset += (sizeof(read_magic_word) - 1);
                    }
                    else if (part_table_offset != 0)
                    {
                        part_table_offset = 0;
                    }
                    else
                    {
                        /* find failed */
                        break;
                    }
                }
            }
        }

        /* load partition table */
        while (part_table_find_ok)
        {
            memset(new_part, 0x00, table_num);
            if (flash_dev->ops.read(part_table_offset - table_item_size * (table_num), (uint8_t *) new_part,
                    table_item_size) < 0)
            {
                log_e("# (%s) ", flash_dev->name);
                table_num = 0;
                break;
            }

            if (new_part->magic_word != ((FAL_PART_MAGIC_WORD_H << 16) + FAL_PART_MAGIC_WORD_L))
            {
                break;
            }

            partition_table = (fal_partition_t) FAL_REALLOC(partition_table, table_item_size * (table_num + 1));
            if (partition_table == NULL)
            {
                log_e("#");
                table_num = 0;
                break;
            }

            memcpy(partition_table + table_num, new_part, table_item_size);

            table_num++;
        };

        if (table_num == 0)
        {
            log_e("# %s (len: %d)  0x%08x.", FAL_PART_TABLE_FLASH_DEV_NAME,
                    FAL_DEV_NAME_MAX, FAL_PART_TABLE_END_OFFSET);
            goto _exit;
        }
        else
        {
            partition_table_len = table_num;
        }
#endif /* FAL_PART_HAS_TABLE_CFG */

        /* check the partition table device exists */

        for (i = 0; i < partition_table_len; i++)
        {
            flash_dev = fal_flash_device_find(partition_table[i].flash_name);
            if (flash_dev == NULL)
            {
                log_d("# (%s).", partition_table[i].flash_name);
                continue;
            }

            if (partition_table[i].offset >= (long)flash_dev->len)
            {
                log_e("# (%s) (%ld) (<%d).",
                        partition_table[i].name, partition_table[i].offset, flash_dev->len);
                partition_table_len = 0;
                goto _exit;
            }
        }

        init_ok = 1;

    _exit:

#if FAL_DEBUG
        fal_show_part_table();
#endif

#ifndef FAL_PART_HAS_TABLE_CFG
        if (new_part)
        {
                FAL_FREE(new_part);
        }
#endif /* !FAL_PART_HAS_TABLE_CFG */

        return partition_table_len;
}

/**
 * find the partition by name
 *
 * @param name partition name
 *
 * @return != NULL: partition
 *            NULL: not found
 */
struct fal_partition *fal_partition_find(char *name)
{
        assert(init_ok);

        size_t i;
        for (i = 0; i < partition_table_len; i++)
        {
                if (!strcmp(name, partition_table[i].name))
                {
                        return &partition_table[i];
                }
        }

        return NULL;
}

/**
 * get the partition table
 *
 * @param len return the partition table length
 *
 * @return partition table
 */
const struct fal_partition *fal_get_partition_table(size_t *len)
{
        assert(init_ok);
        assert(len);

        *len = partition_table_len;
        return partition_table;
}

/**
 * set partition table temporarily
 * This setting will modify the partition table temporarily, the setting will be lost after restart.
 *
 * @param table partition table
 * @param len partition table length
 */
void fal_set_partition_table_temp(struct fal_partition *table, size_t len)
{
        assert(init_ok);
        assert(table);

        partition_table_len = len;
        partition_table = table;
}

/**
 * read data from partition
 *
 * @param part partition
 * @param addr relative address for partition
 * @param buf read buffer
 * @param size read size
 *
 * @return >= 0: successful read data size
 *           -1: error
 */
int fal_partition_read(const struct fal_partition *part, uint32_t addr, uint8_t *buf, size_t size)
{
        int ret = 0;
        const struct fal_flash_dev *flash_dev = NULL;

        assert(part);
        assert(buf);

        if (addr + size > part->len)
        {
                log_e("#");
                return -1;
        }
        flash_dev = fal_flash_device_find(part->flash_name);
        if (flash_dev == NULL)
        {
                log_e("# (%s) (%s).", part->flash_name, part->name);
                return -1;
        }
        ret = flash_dev->ops.read(part->offset + addr, buf, size);
        if (ret < 0)
        {
                log_e("#(%s)", part->flash_name);
        }
        return ret;
}

/**
 * write data to partition
 *
 * @param part partition
 * @param addr relative address for partition
 * @param buf write buffer
 * @param size write size
 *
 * @return >= 0: successful write data size
 *           -1: error
 */
int fal_partition_write(const struct fal_partition *part, uint32_t addr, const uint8_t *buf, size_t size)
{
        int ret = 0;
        const struct fal_flash_dev *flash_dev = NULL;

        assert(part);
        assert(buf);

        if (addr + size > part->len)
        {
                log_e("#");
                return -1;
        }
        flash_dev = fal_flash_device_find(part->flash_name);
        if (flash_dev == NULL)
        {
                log_e("# (%s) (%s).", part->flash_name, part->name);
                return -1;
        }
        ret = flash_dev->ops.write(part->offset + addr, buf, size);
        if (ret < 0)
        {
                log_e("#(%s)", part->flash_name);
        }

        return ret;
}

/**
 * erase partition data
 *
 * @param part partition
 * @param addr relative address for partition
 * @param size erase size
 *
 * @return >= 0: successful erased data size
 *           -1: error
 */
int fal_partition_erase(const struct fal_partition *part, uint32_t addr, size_t size)
{
        int ret = 0;
        const struct fal_flash_dev *flash_dev = NULL;

        assert(part);

        if (addr + size > part->len)
        {
                log_e("# bound.");
                return -1;
        }
        flash_dev = fal_flash_device_find(part->flash_name);
        if (flash_dev == NULL)
        {
                log_e("# erase  device(%s)  partition(%s).", part->flash_name, part->name);
                return -1;
        }
        ret = flash_dev->ops.erase(part->offset + addr, size);
        if (ret < 0)
        {
                log_e("# erase error device(%s)", part->flash_name);
        }

        return ret;
}

/**
 * erase partition all data
 *
 * @param part partition
 *
 * @return >= 0: successful erased data size
 *           -1: error
 */
int fal_partition_erase_all(const struct fal_partition *part)
{
        return fal_partition_erase(part, 0, part->len);
}
