
#ifndef _BK_MODEM_AT_H_
#define _BK_MODEM_AT_H_

#define AT_ECPCFG 				"AT+ECPCFG\r"
#define AT_ECNETCFG				"AT+ECNETCFG\r"
#define AT_ECNETDEVCTL			"AT+ECNETDEVCTL\r"
#define ATD						"ATD\r"
#define ATO						"ATO\r"
#define ATH						"ATH\r"
#define AT_C					"AT&C\r"
#define AT_D					"AT&D\r"
#define AT_CONNECT_CMD			"ATD*99#\r"
#define AT_CHANGE_TO_AT_MODE	"+++"

#define AT						"AT\r"
#define AT_COPS					"AT+COPS?\r"
#define AT_CPIN					"AT+CPIN?\r"
#define AT_CSQ					"AT+CSQ\r"
#define AT_CGDCONT				"AT+CGDCONT?\r"
#define AT_CFUN_0				"AT+CFUN=0\r"
#define AT_CFUN_1				"AT+CFUN=1\r"
#define AT_CGREG				"AT+CGREG?\r"

#define AT_RSP_OK				"OK"
#define AT_RSP_ERROR			"ERROR"
#define AT_RSP_CONNECT			"CONNECT"
#define AT_RSP_CPIN				"+CPIN: READY"
#define AT_RSP_CGREG1			"+CGREG: 0,1"
#define AT_RSP_CGREG5			"+CGREG: 0,5"

//AT
bk_err_t bk_modem_at_ready(void);
//AT_COPS
bk_err_t bk_modem_at_get_operator_name(void);
//AT_CPIN
bk_err_t bk_modem_at_cpin(void);
//AT_CSQ
bk_err_t bk_modem_at_csq(void);
//AT_CGDCONT
bk_err_t bk_modem_at_cgdcont(uint8_t cid,char *type,char *apn);
//AT_CGDCONT?
bk_err_t bk_modem_at_cgdcont_check(void);
//+++
bk_err_t bk_modem_at_enter_cmd_mode(void);
// PPP connect
bk_err_t bk_modem_at_ppp_connect(void);
//AT_ECPCFG
bk_err_t bk_modem_at_set_plat_configure(void);
//AT_ECNETCFG
bk_err_t bk_modem_at_set_network_adapter_parameter_configuration(void);
//AT_ECNETDEVCTL
bk_err_t bk_modem_at_set_data_path_control(void);
//ATD
bk_err_t bk_modem_at_atd(void);
//ATO
bk_err_t bk_modem_at_get_dce_data_state(void);
//ATH
bk_err_t bk_modem_at_disconnect(void);
//AT_C
bk_err_t bk_modem_at_control_dcd(void);
//AT_D
bk_err_t bk_modem_at_change_ue_resp_mode(void);

// receive at command responce
void bk_modem_at_rcv_resp(const char *resp,uint32_t len);
bk_err_t bk_modem_at_init(void);
bk_err_t bk_modem_at_dinit(void);
bk_err_t bk_modem_at_cfun(uint8_t value);
bk_err_t bk_modem_at_get_ps_reg(void);
#endif
