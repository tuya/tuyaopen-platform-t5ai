#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_bt_types.h"
#include "components/bluetooth/bk_dm_bt.h"
#include "components/bluetooth/bk_dm_gap_bt.h"

enum
{
    BT_MNG_MODE_PAIRING = 1,       /**< connectable and discoverable*/
    BT_MNG_MODE_RECONNECTING,      /**< no-connectable and no-discoverable*/
    BT_MNG_MODE_CONNECTEED,        /**< no-connectable and no-discoverable*/
    BT_MNG_MODE_CONNECTABLE,       /**< connectable and no-discoverable */
};

enum
{
    BT_STATE_IDLE = 0,
    BT_STATE_WAIT_FOR_RECONNECT,
    BT_STATE_RECONNECTING,
    BT_STATE_LINK_CONNECTED,
    BT_STATE_PROFILE_CONNECTED,
    BT_STATE_KEY_MISSING,
};

typedef void (*btm_gap_event_cb)(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param);
typedef void (*btm_start_profile_connect_cb)(uint8_t *remote_addr);
typedef void (*btm_start_profile_disconnect_cb)(uint8_t *remote_addr);
typedef void (*btm_stop_profile_connect_cb)();
typedef struct 
{
    btm_gap_event_cb gap_cb;
    btm_start_profile_connect_cb start_connect_cb;
    btm_stop_profile_connect_cb stop_connect_cb;
    btm_start_profile_disconnect_cb start_disconnect_cb;
} btm_callback_s;


int bt_manager_register_callback(btm_callback_s *cb);
void bt_manager_start_reconnect(uint8_t *addr, uint8_t immediate);
void bt_manager_set_mode(uint8_t mode);
int bt_manager_init();
uint8_t bt_manager_get_connect_state();
void bt_manager_set_connect_state(uint8_t state);
uint8_t* bt_manager_get_reconnect_device();
uint8_t* bt_manager_get_connected_device();