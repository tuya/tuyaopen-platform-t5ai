#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void bt_hidd_init();
int hidd_demo_cli_init(void);
void clib_bt_hidd_send_report(char key);
void hidd_demo_key_init();
#ifdef __cplusplus
}
#endif