#ifndef __DOORBELL_BOARDING_H__
#define __DOORBELL_BOARDING_H__

#include "ble_boarding.h"

typedef struct
{
	ble_boarding_info_t boarding_info;
	uint16_t channel;
} doorbell_boarding_info_t;


int doorbell_boarding_init(void);
void doorbell_boarding_event_notify(uint16_t opcode, int status);
int doorbell_boarding_notify(uint8_t *data, uint16_t length);
void doorbell_boarding_event_notify_with_data(uint16_t opcode, int status, char *payload, uint16_t length);

#endif
