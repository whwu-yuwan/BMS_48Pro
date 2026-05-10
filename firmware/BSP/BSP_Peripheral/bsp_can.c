#include "bsp_can.h"
#include "app_can.h"
#include <stdio.h>

static const char *bsp_can_lec_str(uint32_t lec)
{
	switch (lec & 0x7u)
	{
	case 0u: return "NoError";
	case 1u: return "Stuff";
	case 2u: return "Form";
	case 3u: return "Ack";
	case 4u: return "BitRecessive";
	case 5u: return "BitDominant";
	case 6u: return "Crc";
	case 7u: return "SetBySoftware";
	default: return "Unknown";
	}
}

uint8_t BSP_CAN_ConfigFilterAcceptAll(void)
{
	CAN_FilterTypeDef filter = {0};
	filter.FilterBank = 0;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
#if CAN_SELFTEST_LOOPBACK
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	filter.FilterIdHigh = 0x0000;
	filter.FilterIdLow = 0x0000;
	filter.FilterMaskIdHigh = 0x0000;
	filter.FilterMaskIdLow = 0x0000;
#else
	filter.FilterScale = CAN_FILTERSCALE_16BIT;

	uint16_t base_id = (uint16_t)(((uint16_t)APP_CAN_ID_RX_MOS_CTRL_STD & 0x7FFu) << 5);
	uint16_t mask = (uint16_t)((0x7FEu & 0x7FFu) << 5);

	filter.FilterIdHigh = base_id;
	filter.FilterMaskIdHigh = mask;
	filter.FilterIdLow = base_id;
	filter.FilterMaskIdLow = mask;
#endif
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

	HAL_CAN_StateTypeDef state = HAL_CAN_GetState(&hcan);
	if (state != HAL_CAN_STATE_LISTENING)
	{
		printf("[CAN_TX] state=%d not listening\r\n", (int)state);
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
		uint32_t err = HAL_CAN_GetError(&hcan);
		printf("[CAN_TX] add fail ide=%u id=0x%lX dlc=%u err=0x%08lX\r\n",
			   (unsigned int)ide, (unsigned long)id, (unsigned int)dlc, (unsigned long)err);
		return 1u;
	}

	uint32_t start = HAL_GetTick();
	while (HAL_CAN_IsTxMessagePending(&hcan, mailbox) != 0u)
	{
		if ((HAL_GetTick() - start) >= timeout_ms)
		{
			(void)HAL_CAN_AbortTxRequest(&hcan, mailbox);
			uint32_t err = HAL_CAN_GetError(&hcan);
			uint32_t esr = hcan.Instance->ESR;
			uint32_t tsr = hcan.Instance->TSR;
			uint32_t msr = hcan.Instance->MSR;

			uint32_t lec = (esr >> 4) & 0x7u;
			uint32_t ewg = (esr >> 0) & 0x1u;
			uint32_t epv = (esr >> 1) & 0x1u;
			uint32_t boff = (esr >> 2) & 0x1u;
			uint32_t tec = (esr >> 16) & 0xFFu;
			uint32_t rec = (esr >> 24) & 0xFFu;

			printf("[CAN_TX] timeout ide=%u id=0x%lX dlc=%u mb=%lu err=0x%08lX\r\n",
				   (unsigned int)ide, (unsigned long)id, (unsigned int)dlc, (unsigned long)mailbox,
				   (unsigned long)err);
			printf("[CAN_TX] ESR=0x%08lX (EWG=%lu EPV=%lu BOFF=%lu LEC=%lu/%s TEC=%lu REC=%lu) TSR=0x%08lX MSR=0x%08lX\r\n",
				   (unsigned long)esr,
				   (unsigned long)ewg, (unsigned long)epv, (unsigned long)boff,
				   (unsigned long)lec, bsp_can_lec_str(lec),
				   (unsigned long)tec, (unsigned long)rec,
				   (unsigned long)tsr, (unsigned long)msr);
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
