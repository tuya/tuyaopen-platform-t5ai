#pragma once
#ifdef __cplusplus
extern "C" {
#endif


void gatt_server_demo_init();
void gatts_demo_event_notify(uint16_t opcode, int status);

#ifdef __cplusplus
}
#endif