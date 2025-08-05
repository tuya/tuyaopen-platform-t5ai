#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bluetooth_storage.h"

#if CONFIG_EASY_FLASH
#include "easyflash.h"
#endif

static bt_user_storage_t *s_bt_user_storage;
static const uint8_t s_bt_empty_addr[6] = {0};
static const uint8_t s_bt_invaild_addr[6] = {0xff};

static int32_t bluetooth_storage_alloc_linkkey_info(uint8_t *addr)
{
    int32_t ret = 0;

    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    ret = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    if (ret >= 0)
    {
        return ret;
    }

    for (int i = 0; i < sizeof(s_bt_user_storage->linkkey) / sizeof(s_bt_user_storage->linkkey[0]); ++i)
    {
        if (!memcmp(s_bt_empty_addr, s_bt_user_storage->linkkey[i].addr, sizeof(s_bt_user_storage->linkkey[i].addr)) ||
                !memcmp(s_bt_invaild_addr, s_bt_user_storage->linkkey[i].addr, sizeof(s_bt_user_storage->linkkey[i].addr)))
        {
            memcpy(s_bt_user_storage->linkkey[i].addr, addr, sizeof(s_bt_user_storage->linkkey[i].addr));
            memset(s_bt_user_storage->linkkey[i].link_key, 0, sizeof(s_bt_user_storage->linkkey[i].link_key));

            return i;
        }
    }

    return -1;
}

static int32_t bluetooth_storage_free_linkkey_info_by_index(uint8_t index)
{
    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    if (index >= 0 && index < sizeof(s_bt_user_storage->linkkey) / sizeof(s_bt_user_storage->linkkey[0]))
    {
        memset(&s_bt_user_storage->linkkey[index], 0, sizeof(s_bt_user_storage->linkkey[0]));
        return 0;
    }
    else
    {
        return -1;
    }
}

int32_t bluetooth_storage_find_linkkey_info_index(uint8_t *addr, uint8_t *key)
{
    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    for (int i = 0; i < sizeof(s_bt_user_storage->linkkey) / sizeof(s_bt_user_storage->linkkey[0]); ++i)
    {
        if (!memcmp(addr, s_bt_user_storage->linkkey[i].addr, sizeof(s_bt_user_storage->linkkey[i].addr)))
        {
            if (key)
            {
                memcpy(key, s_bt_user_storage->linkkey[i].link_key, sizeof(s_bt_user_storage->linkkey[i].link_key));
            }

            return i;
        }
    }

    return -1;
}


int32_t bluetooth_storage_save_linkkey_info(uint8_t *addr, uint8_t *key)
{
    bt_user_storage_elem_linkkey_t tmp_key;
    int32_t index = 0;
    int32_t ret = 0;
    int i = 0;

    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    index = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    memset(&tmp_key, 0, sizeof(tmp_key));

    if (index >= 0)
    {

    }
    else
    {
        index = bluetooth_storage_alloc_linkkey_info(addr);

        if (index >= 0)
        {

        }
        else
        {
            os_printf("%s overwrite 0 linkkey info, %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                      s_bt_user_storage->linkkey[0].addr[5],
                      s_bt_user_storage->linkkey[0].addr[4],
                      s_bt_user_storage->linkkey[0].addr[3],
                      s_bt_user_storage->linkkey[0].addr[2],
                      s_bt_user_storage->linkkey[0].addr[1],
                      s_bt_user_storage->linkkey[0].addr[0]);
            index = 0;
        }
    }

    memcpy(tmp_key.addr, addr, sizeof(tmp_key.addr));
    memcpy(tmp_key.link_key, key, sizeof(tmp_key.link_key));

    for (i = index + 1; i < sizeof(s_bt_user_storage->linkkey) / sizeof(s_bt_user_storage->linkkey[0]); ++i)
    {
        memcpy(&s_bt_user_storage->linkkey[i - 1], &s_bt_user_storage->linkkey[i], sizeof(s_bt_user_storage->linkkey[0]));
    }

    memcpy(&s_bt_user_storage->linkkey[i - 1], &tmp_key, sizeof(tmp_key));
#if 0//CONFIG_EASY_FLASH_V4
    ret = ef_set_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage));

    if (ret)
    {
        os_printf("%s ef_set_env_blob err %d\n", __func__, ret);
    }

#endif
    (void)ret;
    return i - 1;
}


int32_t bluetooth_storage_update_to_newest(uint8_t *addr)
{
    bt_user_storage_elem_linkkey_t tmp_key;
    int i = 0;
    int32_t ret = 0;
    int32_t index = 0;

    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    index = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    if (index < 0)
    {
        return -1;
    }

    if (index >= sizeof(s_bt_user_storage->linkkey) / sizeof(s_bt_user_storage->linkkey[0]))
    {
        os_printf("%s index err %d\n", __func__, index);
        return -1;
    }

    memcpy(&tmp_key, &s_bt_user_storage->linkkey[index], sizeof(s_bt_user_storage->linkkey[index]));

    for (i = index + 1; i < sizeof(s_bt_user_storage->linkkey) / sizeof(s_bt_user_storage->linkkey[0]); ++i)
    {
        memcpy(&s_bt_user_storage->linkkey[i - 1], &s_bt_user_storage->linkkey[i], sizeof(s_bt_user_storage->linkkey[0]));
    }

    memcpy(&s_bt_user_storage->linkkey[i - 1], &tmp_key, sizeof(tmp_key));
#if 0//CONFIG_EASY_FLASH_V4
    ret = ef_set_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage));

    if (ret)
    {
        os_printf("%s ef_set_env_blob err %d\n", __func__, ret);
    }

#endif
    (void)ret;
    return 0;
}

int32_t bluetooth_storage_get_newest_linkkey_info(uint8_t *addr, uint8_t *key)
{
    int i = 0;

    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    for (i = sizeof(s_bt_user_storage->linkkey) / sizeof(s_bt_user_storage->linkkey[0]) - 1; i >= 0; --i)
    {
        if (memcmp(s_bt_empty_addr, s_bt_user_storage->linkkey[i].addr, sizeof(s_bt_user_storage->linkkey[i].addr)) &&
                memcmp(s_bt_invaild_addr, s_bt_user_storage->linkkey[i].addr, sizeof(s_bt_user_storage->linkkey[i].addr)))
        {
            int j = 0;

            for (j = 0; j < sizeof(s_bt_user_storage->linkkey[i].link_key) / sizeof(s_bt_user_storage->linkkey[i].link_key[0]); ++j)
            {
                if (s_bt_user_storage->linkkey[i].link_key[j] != 0 && s_bt_user_storage->linkkey[i].link_key[j] != 0xff)
                {
                    break;
                }
            }

            if (j < sizeof(s_bt_user_storage->linkkey[i].link_key) / sizeof(s_bt_user_storage->linkkey[i].link_key[0]))
            {
                memcpy(addr, s_bt_user_storage->linkkey[i].addr, sizeof(s_bt_user_storage->linkkey[i].addr));

                if (key)
                {
                    memcpy(key, s_bt_user_storage->linkkey[i].link_key, sizeof(s_bt_user_storage->linkkey[i].link_key));
                }

                return i;
            }
        }
    }

    return -1;
}

int32_t bluetooth_storage_find_volume_by_addr(uint8_t *addr, uint8_t *volume)
{
    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    for (int i = 0; i < sizeof(s_bt_user_storage->linkkey) / sizeof(s_bt_user_storage->linkkey[0]); ++i)
    {
        if (!memcmp(addr, s_bt_user_storage->linkkey[i].addr, sizeof(s_bt_user_storage->linkkey[i].addr)))
        {
            if (volume)
            {
                *volume = s_bt_user_storage->linkkey[i].a2dp_volume;
            }

            return i;
        }
    }

    return -1;
}

int32_t bluetooth_storage_save_volume(uint8_t *addr, uint8_t volume)
{
    int32_t index = 0;

    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    index = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    if (index >= 0)
    {
        s_bt_user_storage->linkkey[index].a2dp_volume = volume;
    }
    else
    {
        os_printf("%s, not find the device info, %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                    addr[5],addr[4],addr[3],addr[2],addr[1],addr[0]);
        return -1;
    }

    return 0;
}

int32_t bluetooth_storage_del_linkkey_info(uint8_t *addr)
{
    int32_t ret = 0;
    int32_t index = 0;
    int i = 0;

    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    index = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    if (index >= 0)
    {
        for (i = index + 1; i < sizeof(s_bt_user_storage->linkkey) / sizeof(s_bt_user_storage->linkkey[0]); ++i)
        {
            memcpy(&s_bt_user_storage->linkkey[i - 1], &s_bt_user_storage->linkkey[i], sizeof(s_bt_user_storage->linkkey[0]));
        }

        memset(&s_bt_user_storage->linkkey[i - 1], 0, sizeof(s_bt_user_storage->linkkey[0]));
    }

#if 0//CONFIG_EASY_FLASH_V4
    ret = ef_set_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage));

    if (ret)
    {
        os_printf("%s ef_set_env_blob err %d\n", __func__, ret);
    }

#endif
    return ret;
}

int32_t bluetooth_storage_clean_linkkey_info(void)
{
    int32_t ret = 0;

    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    memset(s_bt_user_storage->linkkey, 0, sizeof(s_bt_user_storage->linkkey));

#if 0//CONFIG_EASY_FLASH_V4
    ret = ef_set_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage));

    if (ret)
    {
        os_printf("%s ef_set_env_blob err %d\n", __func__, ret);
    }

#endif
    return ret;
}

int32_t bluetooth_storage_sync_to_flash(void)
{
    int32_t ret = 0;

    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

#if CONFIG_EASY_FLASH_V4

    ret = ef_set_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage));

    if (ret)
    {
        os_printf("%s ef_set_env_blob err %d\n", __func__, ret);
    }

    ret = ef_save_env();

    if (ret)
    {
        os_printf("%s ef_save_env err %d\n", __func__, ret);
    }

#endif
    return ret;
}

int32_t bluetooth_storage_linkkey_debug(void)
{
    char tmp_buff[128] = {0};
    int index = 0;

    for (int i = 0; i < sizeof(s_bt_user_storage->linkkey) / sizeof(s_bt_user_storage->linkkey[0]); ++i)
    {
        memset(tmp_buff, 0, sizeof(tmp_buff));

        index = sprintf(tmp_buff, "%02X:%02X:%02X:%02X:%02X:%02X ", s_bt_user_storage->linkkey[i].addr[5],
                        s_bt_user_storage->linkkey[i].addr[4],
                        s_bt_user_storage->linkkey[i].addr[3],
                        s_bt_user_storage->linkkey[i].addr[2],
                        s_bt_user_storage->linkkey[i].addr[1],
                        s_bt_user_storage->linkkey[i].addr[0]);

        for (int j = 0; j < sizeof(s_bt_user_storage->linkkey[i].link_key); ++j)
        {
            index += sprintf(tmp_buff + index, "%02X", s_bt_user_storage->linkkey[i].link_key[j]);
        }

        os_printf("%s %s\n", __func__, tmp_buff);
    }

    return 0;
}

#if CONFIG_BLE

int32_t bluetooth_storage_save_ble_key_info(bk_ble_bond_dev_t *list, uint32_t count)
{
    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    for (uint32_t i = 0; i < count && i < sizeof(s_bt_user_storage->ble_key) / sizeof(s_bt_user_storage->ble_key[0]); ++i)
    {
        os_memcpy(s_bt_user_storage->ble_key + i, list + i, sizeof(*list));
    }

    return 0;
}

int32_t bluetooth_storage_clean_ble_key_info(void)
{
    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    os_memset(s_bt_user_storage->ble_key, 0, sizeof(s_bt_user_storage->ble_key));

    return 0;
}

int32_t bluetooth_storage_read_ble_key_info(bk_ble_bond_dev_t *list, uint32_t *count)
{
    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    uint32_t final_count = ((*count < sizeof(s_bt_user_storage->ble_key) / sizeof(s_bt_user_storage->ble_key[0])) ?
                    *count: sizeof(s_bt_user_storage->ble_key) / sizeof(s_bt_user_storage->ble_key[0]));

    os_memcpy(list, s_bt_user_storage->ble_key, sizeof(s_bt_user_storage->ble_key[0]) * final_count);

    *count = final_count;

    return 0;
}

int32_t bluetooth_storage_save_local_key(bk_ble_local_keys_t *key)
{
    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    os_memcpy(&s_bt_user_storage->local_keys, key, sizeof(*key));

    return 0;
}

int32_t bluetooth_storage_clean_local_key(void)
{
    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    os_memset(&s_bt_user_storage->local_keys, 0, sizeof(s_bt_user_storage->local_keys));

    return 0;
}

int32_t bluetooth_storage_read_local_key(bk_ble_local_keys_t *key)
{
    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    os_memcpy(key, &s_bt_user_storage->local_keys, sizeof(*key));

    return 0;
}

#endif

int32_t bluetooth_storage_init(void)
{
    int32_t ret = 0;
    size_t saved_len = 0;

    if (!s_bt_user_storage)
    {
        s_bt_user_storage = os_malloc(sizeof(*s_bt_user_storage));

        if (!s_bt_user_storage)
        {
            os_printf("%s alloc fail\n", __func__);
            return -1;
        }

        os_memset(s_bt_user_storage, 0, sizeof(*s_bt_user_storage));

#if CONFIG_EASY_FLASH_V4
        ret = ef_get_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage), &saved_len);

        if (ret != sizeof(*s_bt_user_storage) || saved_len != sizeof(*s_bt_user_storage))
        {
            os_printf("%s ef_get_env_blob err %d %d\n", __func__, ret, saved_len);

            if(saved_len < sizeof(*s_bt_user_storage))
            {
                os_memset(((uint8_t *)s_bt_user_storage) + saved_len, 0, sizeof(*s_bt_user_storage) - saved_len);
            }
        }

#endif
    }
    (void)saved_len;
    (void)ret;
    return 0;
}

int32_t bluetooth_storage_deinit(void)
{
    if (s_bt_user_storage)
    {
        os_free(s_bt_user_storage);
        s_bt_user_storage = NULL;
    }

    return 0;
}
