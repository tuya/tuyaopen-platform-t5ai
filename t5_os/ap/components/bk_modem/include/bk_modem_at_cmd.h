
#ifndef _BK_MODEM_AT_H_
#define _BK_MODEM_AT_H_

#define ATD						"ATD\r\n"
#define ATO						"ATO\r\n"
#define ATH						"ATH\r\n"
#define AT_C					"AT&C\r\n"
#define AT_D					"AT&D\r\n"
#define AT_CONNECT_CMD			"ATD*99#\r\n"
#define AT_CHANGE_TO_AT_MODE	"+++"

#define AT						"AT\r\n"
#define ATI                     "ATI\r\n"
#define AT_COPS					"AT+COPS?\r\n"
#define AT_CPIN					"AT+CPIN?\r\n"
#define AT_CSQ					"AT+CSQ\r\n"
#define AT_CGDCONT				"AT+CGDCONT?\r\n"
#define AT_CFUN_0				"AT+CFUN=0\r\n"
#define AT_CFUN_1				"AT+CFUN=1\r\n"
#define AT_CGREG				"AT+CGREG?\r\n"
#define AT_CEREG				"AT+CEREG?\r\n"
#define AT_CCID                 "AT+CCID\r\n"
#define AT_ECICCID              "AT+ECICCID\r\n"
#define AT_CBC                  "AT+CBC\r\n"

#define AT_CGSN                 "AT+CGSN?\r\n"
#define AT_CFSN                 "AT+CFSN?\r\n"
#define AT_CFUN                 "AT+CFUN?\r\n"
#define AT_CGMR                 "AT+CGMR?\r\n"
#define AT_IPR                  "AT+IPR="

#define AT_RSP_OK				"OK"
#define AT_RSP_ERROR			"ERROR"
#define AT_RSP_CONNECT			"CONNECT"
#define AT_RSP_CPIN				"+CPIN: READY"
#define AT_RSP_CGREG1			"+CGREG: 0,1"
#define AT_RSP_CGREG5			"+CGREG: 0,5"
#define AT_RSP_CEREG1			"+CEREG: 0,1"
#define AT_RSP_CEREG5			"+CEREG: 0,5"


///ec at begin
#define AT_ECPCFG 				"AT+ECPCFG=\"usbCtrl\",1\r\n"
#define AT_ECNETCFG_Q			"AT+ECNETCFG?\r\n"
#define AT_ECNETCFG_S			"AT+ECNETCFG=\"nat\",1,\"192.168.10.2\"\r\n"
#define AT_ECNETDEVCTL			"AT+ECNETDEVCTL=3,1,1\r\n"
#define AT_ECRST				"AT+ECRST\r\n"
///ec at end

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
//AT_CCID
bk_err_t bk_modem_at_ccid(void);
//AT_CBC
bk_err_t bk_modem_at_cbc(void);
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
//ATI
bk_err_t bk_modem_at_get_ati(void);
//AT_CGSN
bk_err_t bk_modem_at_get_cgsn(void);
//AT_CFSN
bk_err_t bk_modem_at_get_cfsn(void);
//AT_CFUN
bk_err_t bk_modem_at_get_cfun(void);
//AT_CGMR
bk_err_t bk_modem_at_get_cgmr(void);
bk_err_t bk_modem_at_change_baudrate(uint32_t baudrate);

// receive at command responce
void bk_modem_at_rcv_resp(const char *resp,uint32_t len);
bk_err_t bk_modem_at_init(void);
bk_err_t bk_modem_at_dinit(void);
bk_err_t bk_modem_at_cfun(uint8_t value);
bk_err_t bk_modem_at_get_ps_reg(void);
bk_err_t bk_modem_ec_at_check_nat(void);
bk_err_t bk_modem_ec_at_set_nat(void);
bk_err_t bk_modem_ec_at_close_rndis(void);
bk_err_t bk_modem_ec_at_open_datapath(void);
bk_err_t bk_modem_ec_at_rst(void);
#endif
