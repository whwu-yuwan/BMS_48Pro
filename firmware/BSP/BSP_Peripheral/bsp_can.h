#ifndef __BSP_CAN_H
#define __BSP_CAN_H

#include <stdint.h>
#include "can.h"

/**
 * @brief CAN报文结构体
 * 
 * @param id CAN报文ID
 * @param ide 是否为标准ID
 * @param rtr 是否为远程传输请求
 * @param dlc 数据长度
 * @param data 数据指针
 * @return BSP_CAN_Frame_t CAN报文结构体
 * */
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
