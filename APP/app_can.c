#include "app_can.h"

uint8_t APP_CAN_Init(void)
{
	return 0u;
}

uint8_t APP_CAN_SendTest(uint8_t counter)
{
	uint8_t data[1] = {counter};
	return BSP_CAN_SendStd((uint16_t)APP_CAN_ID_TEST_STD, data, 1u, 10u);
}

uint8_t APP_CAN_SendBmsBasic(const APP_CAN_BmsBasic_t *basic)
{
	if (basic == NULL)
	{
		return 2u;
	}

	uint8_t data[6] = {0};
	data[0] = (uint8_t)(basic->pack_mV & 0xFFu);
	data[1] = (uint8_t)((basic->pack_mV >> 8) & 0xFFu);
	data[2] = (uint8_t)((uint16_t)basic->current_mA & 0xFFu);
	data[3] = (uint8_t)(((uint16_t)basic->current_mA >> 8) & 0xFFu);
	data[4] = basic->soc;
	data[5] = basic->cell_count;
	return BSP_CAN_SendStd((uint16_t)APP_CAN_ID_BMS_BASIC_STD, data, 6u, 10u);
}

uint8_t APP_CAN_TryReceive(BSP_CAN_Frame_t *frame)
{
	return BSP_CAN_TryReceive(frame);
}

uint8_t APP_CAN_DecodeBmsBasic(const BSP_CAN_Frame_t *frame, APP_CAN_BmsBasic_t *out)
{
	if ((frame == NULL) || (out == NULL))
	{
		return 2u;
	}

	if ((frame->ide != (uint8_t)CAN_ID_STD) || (frame->id != (uint32_t)APP_CAN_ID_BMS_BASIC_STD) || (frame->dlc < 6u))
	{
		return 1u;
	}

	uint16_t pack_mV = (uint16_t)((uint16_t)frame->data[0] | ((uint16_t)frame->data[1] << 8));
	int16_t current_mA = (int16_t)((uint16_t)frame->data[2] | ((uint16_t)frame->data[3] << 8));

	out->pack_mV = pack_mV;
	out->current_mA = current_mA;
	out->soc = frame->data[4];
	out->cell_count = frame->data[5];
	return 0u;
}

uint8_t APP_CAN_DecodeMosCtrl(const BSP_CAN_Frame_t *frame, APP_CAN_MosCtrl_t *out)
{
	if ((frame == NULL) || (out == NULL))
	{
		return 2u;
	}

	if ((frame->ide != (uint8_t)CAN_ID_STD) || (frame->id != (uint32_t)APP_CAN_ID_MOS_CTRL_STD) || (frame->dlc < 2u))
	{
		return 1u;
	}

	out->charge_mos = frame->data[0];
	out->discharge_mos = frame->data[1];
	return 0u;
}
