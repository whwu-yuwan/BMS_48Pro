#include "bsp_can.h"

uint8_t BSP_CAN_ConfigFilterAcceptAll(void)
{
	CAN_FilterTypeDef filter = {0};
	filter.FilterBank = 0;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	filter.FilterIdHigh = 0x0000;
	filter.FilterIdLow = 0x0000;
	filter.FilterMaskIdHigh = 0x0000;
	filter.FilterMaskIdLow = 0x0000;
	filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	filter.FilterActivation = ENABLE;
	filter.SlaveStartFilterBank = 14;

	return (HAL_CAN_ConfigFilter(&hcan, &filter) == HAL_OK) ? 0u : 1u;
}

// 发送CAN数据
static uint8_t bsp_can_send(uint32_t id, uint8_t ide, uint8_t rtr, const uint8_t *data, uint8_t dlc, uint32_t timeout_ms)
{
	if (dlc > 8u)
	{
		return 1u;
	}

	if (HAL_CAN_GetState(&hcan) != HAL_CAN_STATE_LISTENING)
	{
		return 3u;
	}

	CAN_TxHeaderTypeDef header = {0};
	header.IDE = ide;
	header.RTR = rtr;
	header.DLC = dlc;

	if (ide == CAN_ID_STD)
	{
		header.StdId = id & 0x7FFu;
	}
	else
	{
		header.ExtId = id & 0x1FFFFFFFu;
	}

	uint32_t mailbox = 0;
	if (HAL_CAN_AddTxMessage(&hcan, &header, (uint8_t *)data, &mailbox) != HAL_OK)
	{
		return 1u;
	}

	uint32_t start = HAL_GetTick();
	while (HAL_CAN_IsTxMessagePending(&hcan, mailbox) != 0u)
	{
		if ((HAL_GetTick() - start) >= timeout_ms)
		{
			(void)HAL_CAN_AbortTxRequest(&hcan, mailbox);
			return 2u;
		}
	}

	return 0u;
}

// 发送标准ID数据
uint8_t BSP_CAN_SendStd(uint16_t std_id, const uint8_t *data, uint8_t dlc, uint32_t timeout_ms)
{
	return bsp_can_send((uint32_t)std_id, CAN_ID_STD, CAN_RTR_DATA, data, dlc, timeout_ms);
}

// 发送扩展ID数据
uint8_t BSP_CAN_SendExt(uint32_t ext_id, const uint8_t *data, uint8_t dlc, uint32_t timeout_ms)
{
	return bsp_can_send(ext_id, CAN_ID_EXT, CAN_RTR_DATA, data, dlc, timeout_ms);
}

// 尝试接收数据
uint8_t BSP_CAN_TryReceive(BSP_CAN_Frame_t *frame)
{
	if (frame == NULL)
	{
		return 2u;
	}

	if (HAL_CAN_GetState(&hcan) != HAL_CAN_STATE_LISTENING)
	{
		return 3u;
	}

	if (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) == 0u)
	{
		return 1u;
	}

	CAN_RxHeaderTypeDef rx = {0};
	uint8_t data[8] = {0};
	if (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rx, data) != HAL_OK)
	{
		return 2u;
	}

	frame->ide = (uint8_t)rx.IDE;
	frame->rtr = (uint8_t)rx.RTR;
	frame->dlc = (uint8_t)rx.DLC;
	frame->id = (rx.IDE == CAN_ID_STD) ? (uint32_t)rx.StdId : (uint32_t)rx.ExtId;
	for (uint8_t i = 0; i < 8u; i++)
	{
		frame->data[i] = data[i];
	}

	return 0u;
}
