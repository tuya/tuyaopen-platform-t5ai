#ifndef LWIP_PING_H
#define LWIP_PING_H

extern int ping(char* target_name, uint32_t times, size_t size);
void ping_stop(void);
void ping_start(char* target_name, uint32_t times, size_t size);

uint8_t get_ping_state();
#endif /* LWIP_PING_H */
