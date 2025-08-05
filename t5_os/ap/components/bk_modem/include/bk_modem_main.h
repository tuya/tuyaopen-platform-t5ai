
#ifndef _BK_MODEM_MAIN_H_
#define _BK_MODEM_MAIN_H_

#include <common/bk_include.h>

#define BK_MODEM_LOG 				"bkmodem"
#define BK_MODEM_LOGI(...)			BK_LOGI(BK_MODEM_LOG, ##__VA_ARGS__)
#define BK_MODEM_LOGW(...)			BK_LOGW(BK_MODEM_LOG, ##__VA_ARGS__)
#define BK_MODEM_LOGE(...)			BK_LOGE(BK_MODEM_LOG, ##__VA_ARGS__)
#define BK_MODEM_LOGD(...)			BK_LOGD(BK_MODEM_LOG, ##__VA_ARGS__)
#define BK_MODEM_LOGV(...)			BK_LOGV(BK_MODEM_LOG, ##__VA_ARGS__)
#define BK_MODEM_LOGI_RAW(...)		BK_RAW_LOGI(BK_MODEM_LOG, ##__VA_ARGS__)
#define BK_MODEM_LOGD_RAW(...)		BK_RAW_LOGD(BK_MODEM_LOG, ##__VA_ARGS__)

enum bk_modem_msg_e
{
    MSG_MODEM_CONN_IND,
    /// check modem valid
    MSG_MODEM_CHECK,
    /// PPP START 
    MSG_PPP_START,
    /// PPP CONNECT IND
    MSG_PPP_CONNECT_IND,
    /// PPP STATUS IND 
    MSG_PPP_STATUS_IND,    
    /// PPP STOP
    MSG_PPP_STOP,
    /// MODEM DISC
    MSG_MODEM_DISC_IND,
    /// POWER ON CPU1
    MSG_MODEM_USBH_POWER_ON,
};

enum bk_modem_state_e
{
    /// wait modem connect
    WAIT_MODEM_CONN,
    /// check modem valid
    MODEM_CHECK,
    /// PPP START
    PPP_START,    
    /// PPP STOP
    PPP_STOP,    
    /// MODEM_DISC
    MODEM_DISC,
};

enum bk_modem_ppp_stop_reason_e
{
    ACTIVE_STOP,
    NO_CARRIER_STOP,
    ABNORMAL_STOP,
    DSIC_STOP,
};

typedef struct bus_message 
{
    uint32_t type;
    uint32_t arg;
    uint32_t len;
    void *sema;
    void *cb;
    void *param;
} BUS_MSG_T;

enum bk_modem_ppp_mode_e
{
    PPP_INIT_MODE,
    PPP_CMD_MODE,
    PPP_DATA_MODE,
};

struct bk_modem_env_s
{
    enum bk_modem_ppp_mode_e bk_modem_ppp_mode;
    bool is_ppp_started;
    uint8_t port_num;
    uint8_t port_idx;
};

extern struct bk_modem_env_s bk_modem_env;
extern void bk_modem_set_state(enum bk_modem_state_e bk_modem_state);
extern enum bk_modem_state_e bk_modem_get_state(void);
extern int bk_modem_send_msg(int type, uint32_t arg, uint32_t len, void *param);
extern void bk_modem_power_on_modem(void);
extern void bk_modem_power_off_modem(void);
extern void bk_modem_del_resource(void);
#endif
