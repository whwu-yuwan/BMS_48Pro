#ifndef __BSP_CAN_H
#define __BSP_CAN_H

#include <stdint.h>
#include "can.h"

typedef struct
{
	uint32_t id;
	uint8_t ide;
	uint8_t rtr;
	uint8_t dlc;
	uint8_t data[8];
} BSP_CAN_Frame_t;

uint8_t BSP_CAN_ConfigFilterAcceptAll(void);

uint8_t BSP_CAN_SendStd(uint16_t std_id, const uint8_t *data, uint8_t dlc, uint32_t timeout_ms);
uint8_t BSP_CAN_SendExt(uint32_t ext_id, const uint8_t *data, uint8_t dlc, uint32_t timeout_ms);

uint8_t BSP_CAN_TryReceive(BSP_CAN_Frame_t *frame);

#endif
