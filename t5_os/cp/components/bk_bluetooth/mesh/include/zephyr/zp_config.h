#ifndef _ZP_CONFIG_H_
#define _ZP_CONFIG_H_

#include <common/sys_config.h>

#define CONFIG_BT_ID_MAX          1

#define CONFIG_BT_HCI_CMD_COUNT   10
#define CONFIG_BT_RX_BUF_COUNT    10

#define CONFIG_BT_RX_STACK_SIZE       (3 * 1024)
#define CONFIG_BT_HCI_TX_STACK_SIZE   (3 * 1024)

#define CONFIG_BT_HCI_RESERVE         10

#define CONFIG_BT_RPA_TIMEOUT         10

#define CONFIG_BT_HCI_TX_PRIO         5
#define CONFIG_BT_RX_PRIO             5

#define CONFIG_BT_MAX_PAIRED          3

#define NET_BUF_POOL_LIST_SIZE        10 ////_net_buf_pool_list

#define CONFIG_BT_DEVICE_NAME "zephyr-mesh"

#define CONFIG_BK_HCI_RECV_PRIO_TASK_STACK_SIZE   3 * 1024
#define CONFIG_BK_HCI_RECV_TASK_STACK_SIZE        3 * 1024
#define CONFIG_BK_HCI_TASK_STACK_SIZE             3 * 1024
#define CONFIG_BT_MESH_ADV_STACK_SIZE             3 * 1024

#define CONFIG_BK_HCI_RECV_PRIO_WAIT_LIMIT        10
#define CONFIG_BK_HCI_RECV_WAIT_LIMIT             10

//#define CONFIG_BT_RECV_IS_RX_THREAD               1

#define CONFIG_NET_BUF_USER_DATA_SIZE             20

#define __noinit

//set no section
#if 1
#define ___in_section(a, b, c)
#else
#define ___in_section(a, b, c) \
	__attribute__((section("." Z_STRINGIFY(a)			\
				"." Z_STRINGIFY(b)			\
				"." Z_STRINGIFY(c))))
#endif
#define __in_section(a, b, c) ___in_section(a, b, c)

#define Z_QUEUE_SEM_SIZE            10
#define Z_QUEUE_ALLOC_NODE          0


#if CONFIG_ACLSEMI_BT_MESH_DEBUG_LEVEL >= 5
#define ZP_LOG_VER 1
#endif

#if CONFIG_ACLSEMI_BT_MESH_DEBUG_LEVEL >= 4
#define ZP_LOG_DBG 1
#endif

#if CONFIG_ACLSEMI_BT_MESH_DEBUG_LEVEL >= 3
#define ZP_LOG_INF 1
#endif

#if CONFIG_ACLSEMI_BT_MESH_DEBUG_LEVEL >= 2
#define ZP_LOG_WRN 1
#endif

#if CONFIG_ACLSEMI_BT_MESH_DEBUG_LEVEL >= 1
#define ZP_LOG_ERR 1
#endif


#define CONFIG_BT_BROADCASTER                 1
#define CONFIG_BT_PERIPHERAL                  1
#define CONFIG_BT_OBSERVER                    1
//#define CONFIG_BT_CENTRAL                     1

#define CONFIG_BT_ECC                         1
#define CONFIG_BT_EXT_ADV                     1
#define CONFIG_BT_MESH                        1
#define CONFIG_BT_TESTING                     0



#define CONFIG_LOG_STRDUP_MAX_STRING    128

#define CONFIG_BT_BACKGROUND_SCAN_INTERVAL    160
#define CONFIG_BT_BACKGROUND_SCAN_WINDOW      160

#define CONFIG_BT_MAX_CONN                    CONFIG_BLE_CONN_NUM
#define CONFIG_BT_EXT_ADV_MAX_ADV_SET         2
#define CONFIG_BT_L2CAP_TX_BUF_COUNT          10
#define CONFIG_BT_L2CAP_TX_MTU                537
#define CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT   (20 * 1000)
#define CONFIG_BT_CONN_TX_MAX                 (5)
#define CONFIG_BT_L2CAP_RX_MTU                537

#define CONFIG_BT_DEVICE_APPEARANCE     0x5555

#define CONFIG_BT_DEVICE_NAME_DYNAMIC 1
#define CONFIG_BT_DEVICE_NAME_MAX 48 //Z_DEVICE_MAX_NAME_LEN 

#define CONFIG_BT_CREATE_CONN_TIMEOUT         3


// zephyr ble mesh config
#if 0 //config in Kconfig
#define CONFIG_BT_MESH_ADV_LEGACY 0
#define CONFIG_BT_MESH_ADV_EXT 1

#define CONFIG_BT_MESH_FRIEND 1
#define CONFIG_BT_MESH_LOW_POWER 1
#define CONFIG_BT_MESH_SELF_TEST //defined or not defined


#define CONFIG_BT_MESH_PROV 1
#define CONFIG_BT_MESH_PB_ADV 1
#define CONFIG_BT_MESH_PROVISIONER 1
#define CONFIG_BT_MESH_PROV_DEVICE 1
#define CONFIG_BT_MESH_CDB 1
//#define CONFIG_BT_SETTINGS 0
#define CONFIG_BT_MESH_GATT 0
#define CONFIG_BT_MESH_PB_GATT 0
#define CONFIG_BT_MESH_GATT_SERVER 0
#define CONFIG_BT_MESH_CFG_CLI 1
#define CONFIG_BT_MESH_HEALTH_CLI 1
#define CONFIG_BT_MESH_SHELL 1
#endif

//#ifdef CONFIG_BT_MESH_FRIEND
//#undef CONFIG_BT_MESH_FRIEND
//#endif
//#define CONFIG_BT_MESH_FRIEND 0

//Kconfig not care


#define CONFIG_BT_MESH_ACCESS_LAYER_MSG 1

#define CONFIG_BT_MESH_BEACON_ENABLED        1
#define CONFIG_BT_MESH_RELAY_ENABLED 0
#define CONFIG_BT_MESH_GATT_PROXY_ENABLED    1
#define CONFIG_BT_MESH_FRIEND_ENABLED 0

#define CONFIG_BT_MESH_MODEL_KEY_COUNT                      2
#define CONFIG_BT_MESH_MODEL_GROUP_COUNT                    4
#define CONFIG_BT_MESH_DEFAULT_TTL                          5
#define CONFIG_BT_MESH_NETWORK_TRANSMIT_COUNT               3
#define CONFIG_BT_MESH_NETWORK_TRANSMIT_INTERVAL            20
#define CONFIG_BT_MESH_ADV_BUF_COUNT                        6
#define CONFIG_BT_MESH_UNPROV_BEACON_INT                    2
#define CONFIG_BT_MESH_MSG_CACHE_SIZE                       2
#define CONFIG_BT_MESH_LOOPBACK_BUFS                        6
#define CONFIG_BT_MESH_IVU_DIVIDER                          6
#define CONFIG_BT_MESH_SUBNET_COUNT                         2
#define CONFIG_BT_MESH_APP_KEY_COUNT                        1
#define CONFIG_BT_MESH_TX_SEG_MSG_COUNT                     2
#define CONFIG_BT_MESH_RX_SEG_MSG_COUNT                     2
#define CONFIG_BT_MESH_TX_SEG_MAX                           3
#define CONFIG_BT_MESH_RX_SEG_MAX                           3
#define CONFIG_BT_MESH_SEG_BUFS                             6
#define CONFIG_BT_MESH_LABEL_COUNT                          3
#define CONFIG_BT_MESH_TX_SEG_RETRANS_TIMEOUT_UNICAST       400
#define CONFIG_BT_MESH_TX_SEG_RETRANS_TIMEOUT_GROUP         10
#define CONFIG_BT_MESH_TX_SEG_RETRANS_COUNT                 3
#define CONFIG_BT_MESH_CRPL                                 128
#define CONFIG_BT_MESH_PB_ADV_RETRANS_TIMEOUT               500

#define CONFIG_BT_MESH_ADV_PRIO 5
#define CONFIG_BT_MESH_PROXY_FILTER_SIZE 3
//#define CONFIG_BT_MESH_GATT_PROXY //defined or not defined

#define CONFIG_BT_MESH_RELAY 1
#define CONFIG_BT_MESH_RELAY_RETRANSMIT_COUNT 3
#define CONFIG_BT_MESH_RELAY_RETRANSMIT_INTERVAL 20

#define CONFIG_BT_MESH_NODE_ID_TIMEOUT 60
#define CONFIG_BT_MESH_CDB_NODE_COUNT 21 //3
#define CONFIG_BT_MESH_CDB_SUBNET_COUNT 3
#define CONFIG_BT_MESH_CDB_APP_KEY_COUNT 1

#define CONFIG_BT_MESH_RPL_STORE_TIMEOUT 120
#define CONFIG_BT_MESH_STORE_TIMEOUT 2
#define CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE 0
#define CONFIG_BT_MESH_IV_UPDATE_SEQ_LIMIT 0x800000
#define CONFIG_BT_MESH_FRIEND_QUEUE_SIZE 16
#define CONFIG_BT_MESH_FRIEND_LPN_COUNT 2


#define CONFIG_BT_MESH_FRIEND_SEG_RX 1
#define CONFIG_BT_MESH_FRIEND_SUB_LIST_SIZE 3
#define CONFIG_BT_MESH_LPN_GROUPS 8
#define CONFIG_BT_MESH_FRIEND_RECV_WIN 255
#define CONFIG_BT_MESH_LPN_POLL_TIMEOUT 300
#define CONFIG_BT_MESH_LPN_RECV_DELAY 100
#define CONFIG_BT_MESH_LPN_RETRY_TIMEOUT 8
#define CONFIG_BT_MESH_LPN_MIN_QUEUE_SIZE 1
#define CONFIG_BT_MESH_LPN_RSSI_FACTOR 0
#define CONFIG_BT_MESH_LPN_RECV_WIN_FACTOR 0
#define CONFIG_BT_MESH_LPN_RECV_DELAY 100
#define CONFIG_BT_MESH_LPN_SCAN_LATENCY 10
#define CONFIG_BT_MESH_LPN_INIT_POLL_TIMEOUT CONFIG_BT_MESH_LPN_POLL_TIMEOUT

#define CONFIG_BT_LIM_ADV_TIMEOUT 30
#define CONFIG_BT_BUF_CMD_TX_COUNT 3
#define CONFIG_BT_BUF_EVT_RX_SIZE (255 + 32)
#define CONFIG_BT_BUF_CMD_TX_SIZE (255 + 32)
#define CONFIG_BT_MESH_DEBUG_USE_ID_ADDR 1


#define CONFIG_BT_ISO_TX_BUF_COUNT 5
#define CONFIG_BT_ISO_RX_BUF_COUNT 1
#define CONFIG_BT_BUF_ACL_RX_SIZE 256
#define CONFIG_BT_BUF_ACL_RX_COUNT 10
#define CONFIG_BT_BUF_EVT_RX_COUNT 10
#define CONFIG_BT_SMP_MIN_ENC_KEY_SIZE 1


#define CONFIG_BLE_CONN_NUM 1



#define CONFIG_ASSERT 1
#define CONFIG_ASSERT_LEVEL 0
#define CONFIG_ASSERT_VERBOSE
//#define CONFIG_ASSERT_NO_FILE_INFO

// end zephyr ble mesh config

#define Z_QUEUE_TEST_CFG            1
#define Z_POLL_TEST_CFG             1
#define Z_WORK_TEST_CFG             1
#define Z_SEM_TEST_CFG              1
#define Z_MUTEX_TEST_CFG            1
#define Z_RTOS_TEST_CFG             1
#define Z_NET_BUF_TEST_CFG          1

//#define CONFIG_NET_BUF_LOG 1
//#define CONFIG_BT_MESH_DEBUG_NET 1

#ifndef __packed
#define __packed                     __attribute__((packed))
#endif








#if 0
/** Key size used in Bluetooth's ECC domain. */
#define BT_ECC_KEY_SIZE            32
/** Length of a Bluetooth ECC public key coordinate. */
#define BT_PUB_KEY_COORD_LEN       (BT_ECC_KEY_SIZE)
/** Length of a Bluetooth ECC public key. */
#define BT_PUB_KEY_LEN             (2 * (BT_PUB_KEY_COORD_LEN))
/** Length of a Bluetooth ECC private key. */
#define BT_PRIV_KEY_LEN            (BT_ECC_KEY_SIZE)
/** Length of a Bluetooth Diffie-Hellman key. */
#define BT_DH_KEY_LEN              (BT_ECC_KEY_SIZE)
#endif
#define SETTINGS_MAX_DIR_DEPTH	8	/* max depth of settings tree */
#define SETTINGS_MAX_NAME_LEN	(8 * SETTINGS_MAX_DIR_DEPTH)
#define SETTINGS_MAX_VAL_LEN	256
#define SETTINGS_NAME_SEPARATOR	'/'
#define SETTINGS_NAME_END '='

#endif

