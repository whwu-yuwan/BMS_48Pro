#include "app_tasks.h"
#include <stdio.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "bms_config.h"
#include "cmsis_os2.h"
#include "iwdg.h"
#include "bsp_bq76940.h"
#include "bsp_can.h"
#include "app_can.h"

osSemaphoreId_t g_sem_fault_trigger = NULL;
osMutexId_t g_mutex_data = NULL;
osMutexId_t g_mutex_i2c = NULL;
osMessageQueueId_t g_queue_can_tx = NULL;
osMessageQueueId_t g_queue_alarm = NULL;
BMS_Data_t bms_data;

static osThreadId_t g_task_data_collect = NULL;
static osThreadId_t g_task_fault_protect = NULL;
static osThreadId_t g_task_charge_control = NULL;
static osThreadId_t g_task_soc_calc = NULL;
static osThreadId_t g_task_balance = NULL;
static osThreadId_t g_task_can_comm = NULL;
static osThreadId_t g_task_assist = NULL;

static uint8_t decodeIfCtrlMos(const BSP_CAN_Frame_t *rx)
{
	if (rx == NULL)
	{
		return 1u;
	}
	if (rx->ide != (uint8_t)CAN_ID_STD)
	{
		return 1u;
	}
	if (rx->rtr != (uint8_t)CAN_RTR_DATA)
	{
		return 1u;
	}

	APP_CAN_MosCtrl_t mos = {0};
	if (APP_CAN_DecodeMosCtrl(rx, &mos) != 0u)
	{
		return 1u;
	}

	uint8_t changed = 0u;

	osMutexAcquire(g_mutex_data, osWaitForever);
	if (((mos.charge_mos == 0u) || (mos.charge_mos == 1u)) && (bms_data.charge_mos != mos.charge_mos))
	{
		bms_data.charge_mos = mos.charge_mos;
		changed = 1u;
	}
	if (((mos.discharge_mos == 0u) || (mos.discharge_mos == 1u)) && (bms_data.discharge_mos != mos.discharge_mos))
	{
		bms_data.discharge_mos = mos.discharge_mos;
		changed = 1u;
	}
	uint8_t chg_now = bms_data.charge_mos;
	uint8_t dsg_now = bms_data.discharge_mos;
	osMutexRelease(g_mutex_data);

	if (changed != 0u)
	{
		printf("[CAN_MOS] ID:0x%lX chg=%u dsg=%u\r\n",
			   (unsigned long)rx->id, (unsigned int)chg_now, (unsigned int)dsg_now);
	}
	return 0u;
}

static uint16_t app_f32_v_to_u16_mV(float v)
{
	float mv = v * 1000.0f;
	if (mv < 0.0f)
	{
		mv = 0.0f;
	}
	if (mv > 65535.0f)
	{
		mv = 65535.0f;
	}
	return (uint16_t)(mv + 0.5f);
}


/*==================================任务区=========================================*/
/**
 * @brief 采样任务
 * 
 * @param arg 任务参数
 */
void DataCollectTask(void *arg){
	(void)arg;
	float cell_v[BQ76940_CELL_NUM + 1] = {0};
	float temp = 0.0f;
	float current = 0.0f;
	for ( ; ; ){
		osMutexAcquire(g_mutex_i2c, osWaitForever);
		uint8_t ok_v = (BQ76940_ReadVoltage(cell_v) == 0u) ? 1u : 0u;
		uint8_t ok_i = (BQ76940_ReadCurrent(&current) == 0u) ? 1u : 0u;
		uint8_t ok_temp = (BQ76940_ReadTemp(&temp) == 0u) ? 1u : 0u;
		osMutexRelease(g_mutex_i2c);

		float pack_v = 0.0f;
		if (ok_v != 0u)
		{
			for (uint8_t i = 0; i < BQ76940_CELL_NUM; i++)
			{
				if (((BQ76940_CELL_PRESENT_MASK >> i) & 0x01u) != 0u)
				{
					pack_v += cell_v[i];
				}
			}
		}
		cell_v[BQ76940_CELL_NUM] = pack_v;
		if (ok_i == 0u)
		{
			current = 0.0f;
		}
		if (ok_temp == 0u)
		{
			temp = 0.0f;
		}

		osMutexAcquire(g_mutex_data, osWaitForever);
		for (uint8_t i = 0; i < (uint8_t)BQ76940_CELL_NUM; i++)
		{
			bms_data.cell_voltage[i] = cell_v[i];
		}
		bms_data.cell_voltage[BQ76940_CELL_NUM] = pack_v;
		bms_data.current = current;
		bms_data.temp = temp;
		float temp = bms_data.temp;
		float current_a = bms_data.current;
		uint8_t soc = bms_data.soc;
		osMutexRelease(g_mutex_data);
		if ((temp >= 3) || (current_a >= 2)){
			APP_Trigger_Fault_Task();
		}
		printf("[采样任务] 运行中... Tick: %u | 模拟读取15串总电压:[%.2f]电流:[%.2f]温度:[%.2f]\r\n",
			osKernelGetTickCount(), pack_v, current_a, temp);
		printf("[采样任务] 运行中... Tick: %u | 读取电池1:[%.2f],电池2:[%.2f],电池3:[%.2f],电池4:[%.2f],电池5:[%.2f],电池6:[%.2f],电池7:[%.2f],电池8:[%.2f],电池9:[%.2f],电池10:[%.2f],电池11:[%.2f],电池12:[%.2f],电池13:[%.2f],电池14:[%.2f],电池15:[%.2f]\r\n",
			 osKernelGetTickCount(), cell_v[0], cell_v[1], cell_v[2], cell_v[3], cell_v[4], cell_v[5], cell_v[6], cell_v[7], cell_v[8], cell_v[9], cell_v[10], cell_v[11], cell_v[12], cell_v[13], cell_v[14]);
		
		// 处理CAN接收数据: 处理MOS控制帧
		if (g_queue_can_tx != NULL)
		{
			float pack_v = 0.0f;
			for (uint8_t i = 0; i < BQ76940_CELL_NUM; i++)
			{
				if (((BQ76940_CELL_PRESENT_MASK >> i) & 0x01u) != 0u)
				{
					pack_v += cell_v[i];
				}
			}

			float pack_mV_f = pack_v * 1000.0f;
			if (pack_mV_f < 0.0f)
			{
				pack_mV_f = 0.0f;
			}
			if (pack_mV_f > 65535.0f)
			{
				pack_mV_f = 65535.0f;
			}
			uint16_t pack_mV = (uint16_t)(pack_mV_f + 0.5f);

			float current_mA_f = current_a * 1000.0f;
			if (current_mA_f > 32767.0f)
			{
				current_mA_f = 32767.0f;
			}
			if (current_mA_f < -32768.0f)
			{
				current_mA_f = -32768.0f;
			}
			int16_t current_mA = (int16_t)(current_mA_f);

			uint8_t cell_count = BQ76940_GetPresentCellCount();

			BSP_CAN_Frame_t tx = {0};
			tx.ide = (uint8_t)CAN_ID_STD;
			tx.rtr = (uint8_t)CAN_RTR_DATA;
			tx.id = (uint32_t)APP_CAN_ID_BMS_BASIC_STD;
			tx.dlc = 6u;
			tx.data[0] = (uint8_t)(pack_mV & 0xFFu);
			tx.data[1] = (uint8_t)((pack_mV >> 8) & 0xFFu);
			tx.data[2] = (uint8_t)(current_mA & 0xFFu);
			tx.data[3] = (uint8_t)((current_mA >> 8) & 0xFFu);
			tx.data[4] = soc;
			tx.data[5] = cell_count;

			(void)osMessageQueuePut(g_queue_can_tx, &tx, 0u, 0u);

			// 单电池电压数据报文 1 - 4串
			tx.ide = (uint8_t)CAN_ID_STD;
			tx.rtr = (uint8_t)CAN_RTR_DATA;
			tx.id = (uint32_t)APP_CAN_ID_CELL_VOLT_STD_1_4;
			tx.dlc = 8u;
			uint16_t mv0 = app_f32_v_to_u16_mV(cell_v[0]);
			uint16_t mv1 = app_f32_v_to_u16_mV(cell_v[1]);
			uint16_t mv2 = app_f32_v_to_u16_mV(cell_v[2]);
			uint16_t mv3 = app_f32_v_to_u16_mV(cell_v[3]);
			tx.data[0] = (uint8_t)(mv0 & 0xFFu);
			tx.data[1] = (uint8_t)((mv0 >> 8) & 0xFFu);
			tx.data[2] = (uint8_t)(mv1 & 0xFFu);
			tx.data[3] = (uint8_t)((mv1 >> 8) & 0xFFu);
			tx.data[4] = (uint8_t)(mv2 & 0xFFu);
			tx.data[5] = (uint8_t)((mv2 >> 8) & 0xFFu);
			tx.data[6] = (uint8_t)(mv3 & 0xFFu);
			tx.data[7] = (uint8_t)((mv3 >> 8) & 0xFFu);
			(void)osMessageQueuePut(g_queue_can_tx, &tx, 0u, 0u);

			// 单电池电压数据报文 5 - 8串
			tx.ide = (uint8_t)CAN_ID_STD;
			tx.rtr = (uint8_t)CAN_RTR_DATA;
			tx.id = (uint32_t)APP_CAN_ID_CELL_VOLT_STD_5_8;
			tx.dlc = 8u;
			uint16_t mv4 = app_f32_v_to_u16_mV(cell_v[4]);
			uint16_t mv5 = app_f32_v_to_u16_mV(cell_v[5]);
			uint16_t mv6 = app_f32_v_to_u16_mV(cell_v[6]);
			uint16_t mv7 = app_f32_v_to_u16_mV(cell_v[7]);
			tx.data[0] = (uint8_t)(mv4 & 0xFFu);
			tx.data[1] = (uint8_t)((mv4 >> 8) & 0xFFu);
			tx.data[2] = (uint8_t)(mv5 & 0xFFu);
			tx.data[3] = (uint8_t)((mv5 >> 8) & 0xFFu);
			tx.data[4] = (uint8_t)(mv6 & 0xFFu);
			tx.data[5] = (uint8_t)((mv6 >> 8) & 0xFFu);
			tx.data[6] = (uint8_t)(mv7 & 0xFFu);
			tx.data[7] = (uint8_t)((mv7 >> 8) & 0xFFu);
			(void)osMessageQueuePut(g_queue_can_tx, &tx, 0u, 0u);

			// 单电池电压数据报文 9 - 12串
			tx.ide = (uint8_t)CAN_ID_STD;
			tx.rtr = (uint8_t)CAN_RTR_DATA;
			tx.id = (uint32_t)APP_CAN_ID_CELL_VOLT_STD_9_12;
			tx.dlc = 8u;
			uint16_t mv8 = app_f32_v_to_u16_mV(cell_v[8]);
			uint16_t mv9 = app_f32_v_to_u16_mV(cell_v[9]);
			uint16_t mv10 = app_f32_v_to_u16_mV(cell_v[10]);
			uint16_t mv11 = app_f32_v_to_u16_mV(cell_v[11]);
			tx.data[0] = (uint8_t)(mv8 & 0xFFu);
			tx.data[1] = (uint8_t)((mv8 >> 8) & 0xFFu);
			tx.data[2] = (uint8_t)(mv9 & 0xFFu);
			tx.data[3] = (uint8_t)((mv9 >> 8) & 0xFFu);
			tx.data[4] = (uint8_t)(mv10 & 0xFFu);
			tx.data[5] = (uint8_t)((mv10 >> 8) & 0xFFu);
			tx.data[6] = (uint8_t)(mv11 & 0xFFu);
			tx.data[7] = (uint8_t)((mv11 >> 8) & 0xFFu);
			(void)osMessageQueuePut(g_queue_can_tx, &tx, 0u, 0u);

			// 单电池电压数据报文 13 - 15串
			tx.ide = (uint8_t)CAN_ID_STD;
			tx.rtr = (uint8_t)CAN_RTR_DATA;
			tx.id = (uint32_t)APP_CAN_ID_CELL_VOLT_STD_13_15;
			tx.dlc = 6u;
			uint16_t mv12 = app_f32_v_to_u16_mV(cell_v[12]);
			uint16_t mv13 = app_f32_v_to_u16_mV(cell_v[13]);
			uint16_t mv14 = app_f32_v_to_u16_mV(cell_v[14]);
			tx.data[0] = (uint8_t)(mv12 & 0xFFu);
			tx.data[1] = (uint8_t)((mv12 >> 8) & 0xFFu);
			tx.data[2] = (uint8_t)(mv13 & 0xFFu);
			tx.data[3] = (uint8_t)((mv13 >> 8) & 0xFFu);
			tx.data[4] = (uint8_t)(mv14 & 0xFFu);
			tx.data[5] = (uint8_t)((mv14 >> 8) & 0xFFu);
			(void)osMessageQueuePut(g_queue_can_tx, &tx, 0u, 0u);
		}

		osDelay(TASK_PERIOD_DATA_COLLECT);
	}
}

/**
 * @brief 故障保护任务
 * 
 * @param arg 任务参数
 */
void FaultProtectTask(void *arg){
	(void)arg;
	float max_cell_v;
	float current;
    for(;;)
    {
		if (osSemaphoreAcquire(g_sem_fault_trigger, osWaitForever) == osOK){
			osMutexAcquire(g_mutex_data, osWaitForever);
			max_cell_v = 0.0f;
			for (uint8_t i = 0; i < BQ76940_CELL_NUM; i++)
			{
				if (((BQ76940_CELL_PRESENT_MASK >> i) & 0x01u) != 0u)
				{
					float v = bms_data.cell_voltage[i];
					if (v > max_cell_v)
					{
						max_cell_v = v;
					}
				}
			}
			current = bms_data.current;
			osMutexRelease(g_mutex_data);
			if (max_cell_v >= 4.20f){
				printf("[故障任务] 运行中... Tick: %u | 模拟检测故障状态: 过压[%.2f]\r\n", osKernelGetTickCount(), max_cell_v);
				if (g_queue_alarm != NULL)
				{
					APP_AlarmMsg_t alarm = {0};
					alarm.code = (uint8_t)APP_ALARM_OV;
					float mv = max_cell_v * 1000.0f;
					if (mv > 32767.0f) mv = 32767.0f;
					if (mv < -32768.0f) mv = -32768.0f;
					alarm.value = (int16_t)mv;
					alarm.tick = osKernelGetTickCount();
					(void)osMessageQueuePut(g_queue_alarm, &alarm, 0u, 0u);
				}
				osMutexAcquire(g_mutex_data, osWaitForever);
				bms_data.temp = 0.f;	
				osMutexRelease(g_mutex_data);
			}
			if (current >= 2){
				printf("[故障任务] 运行中... Tick: %u | 模拟检测故障状态: 过流[%.2f]\r\n", osKernelGetTickCount(), current);
				if (g_queue_alarm != NULL)
				{
					APP_AlarmMsg_t alarm = {0};
					alarm.code = (uint8_t)APP_ALARM_OC;
					float ma = current * 1000.0f;
					if (ma > 32767.0f) ma = 32767.0f;
					if (ma < -32768.0f) ma = -32768.0f;
					alarm.value = (int16_t)ma;
					alarm.tick = osKernelGetTickCount();
					(void)osMessageQueuePut(g_queue_alarm, &alarm, 0u, 0u);
				}
				osMutexAcquire(g_mutex_data, osWaitForever);
				bms_data.current = 0.f;
				osMutexRelease(g_mutex_data);
			}
		}
    }
}

/**
 * @brief 充放电控制任务
 * 
 * @param arg 任务参数
 */
void ChargeControlTask(void *arg){
	(void)arg;
    for(;;)
    {
		static uint8_t last_charge_mos = 0xFFu;
		static uint8_t last_discharge_mos = 0xFFu;

		osMutexAcquire(g_mutex_data, osWaitForever);
		uint8_t charge_mos = bms_data.charge_mos;
		uint8_t discharge_mos = bms_data.discharge_mos;
		osMutexRelease(g_mutex_data);
		printf("[充放电任务] 运行中... Tick: %u | 判断充放电状态: 充电mos: %d, 放电mos: %d\r\n", osKernelGetTickCount(), charge_mos, discharge_mos);
		uint8_t charge_req = (charge_mos != 0u) ? 1u : 0u;
		uint8_t discharge_req = (discharge_mos != 0u) ? 1u : 0u;
		if ((charge_req != last_charge_mos) || (discharge_req != last_discharge_mos))
		{
			uint8_t chg_now = 0u;
			uint8_t dsg_now = 0u;
			osMutexAcquire(g_mutex_i2c, osWaitForever);
			(void)BQ76940_SetChargeMOS(charge_req);
			(void)BQ76940_SetDischargeMOS(discharge_req);
			uint8_t rd = BQ76940_ReadMosState(&chg_now, &dsg_now);
			osMutexRelease(g_mutex_i2c);
			last_charge_mos = charge_req;
			last_discharge_mos = discharge_req;
			if (rd == 0u)
			{
				printf("[MOS_REG] req chg=%u dsg=%u | reg chg=%u dsg=%u\r\n",
					   (unsigned int)charge_req, (unsigned int)discharge_req,
					   (unsigned int)chg_now, (unsigned int)dsg_now);
			}
			else
			{
				printf("[MOS_REG] read fail\r\n");
			}
		}

        osDelay(TASK_PERIOD_CHARGE_CONTROL);
    }
}

void SocCalcTask(void *arg){
	(void)arg;
	uint8_t soc = 0u;
	float pack_v = 0.0f;
    for(;;)
    {
		osMutexAcquire(g_mutex_data, osWaitForever);
		pack_v = bms_data.cell_voltage[BQ76940_CELL_NUM];
		osMutexRelease(g_mutex_data);
		soc = BQ76940_CalcSOC(pack_v);
		osMutexAcquire(g_mutex_data, osWaitForever);
		bms_data.soc = soc;
		osMutexRelease(g_mutex_data);
		printf("[SOC任务] 运行中... Tick: %u | 计算SOC: %u%% \r\n", osKernelGetTickCount(), (unsigned int)soc);
        osDelay(TASK_PERIOD_SOC_CALC);
    }
}

/**
 * @brief 电池均衡任务
 * 
 * @param arg 任务参数
 */
void BalanceTask(void *arg){
	(void)arg;
	float voltage[BQ76940_CELL_NUM] = {0};
	uint8_t i;
	float voltage_min = 5.f;
	uint8_t onoff = 0;
	uint8_t mask1 = 0;
	uint8_t mask2 = 0;
	uint8_t mask3 = 0;
    for(;;)
    {
		osMutexAcquire(g_mutex_data, osWaitForever);
		for (i = 0; i < BQ76940_CELL_NUM; i++)
		{
			voltage[i] = bms_data.cell_voltage[i];
		}
		osMutexRelease(g_mutex_data);
		for (i = 0; i < BQ76940_CELL_NUM; i++)
		{
			if (voltage[i] < voltage_min){
				voltage_min = voltage[i];
			}
		}
		for (i = 0; i < BQ76940_CELL_NUM; i++)
		{
			if (voltage[i] - voltage_min > 0.2f){
				onoff = 1;
				if (i < 5){
					mask1 |= (1 << i);
				}
				else if (i < 10){
					mask2 |= (1 << (i - 5));
				}
				else{
					mask3 |= (1 << (i - 10));
				}
			}
		}
		BQ76940_SetBalanceMOS(onoff,mask1, mask2, mask3);
		printf("[均衡任务] 运行中... Tick: %u | 模拟控制电芯均衡: %d\r\n", osKernelGetTickCount(), onoff);
        osDelay(TASK_PERIOD_BALANCE_START + TASK_PERIOD_BALANCE_TIME + TASK_PERIOD_BALANCE_END);
    }
}

/**
 * @brief CAN通信任务
 * 
 * @param arg 任务参数
 */
void CanCommTask(void *arg){
    (void)arg;
    for(;;)
    {
		// 处理CAN接收数据: 处理MOS控制帧
		BSP_CAN_Frame_t rx = {0};
		while (APP_CAN_TryReceive(&rx) == 0u)
		{
			if ((rx.ide == (uint8_t)CAN_ID_STD) && (rx.id == (uint32_t)APP_CAN_ID_RX_MOS_CTRL_STD) && (rx.dlc >= 2u))
			{
				uint8_t charge_mos = rx.data[0];
				uint8_t discharge_mos = rx.data[1];

				osMutexAcquire(g_mutex_data, osWaitForever);
				if ((charge_mos == 0u) || (charge_mos == 1u))
				{
					bms_data.charge_mos = charge_mos;
				}
				if ((discharge_mos == 0u) || (discharge_mos == 1u))
				{
					bms_data.discharge_mos = discharge_mos;
				}
				osMutexRelease(g_mutex_data);

				printf("[CAN_RX_MOS_CTRL] id=0x%lX charge_mos=%u discharge_mos=%u\r\n",
					   (unsigned long)rx.id, (unsigned int)charge_mos, (unsigned int)discharge_mos);
			}
			if ((rx.ide == (uint8_t)CAN_ID_STD) && (rx.id == (uint32_t)APP_CAN_ID_RX_ALARM_STD) && (rx.dlc >= 1u))
			{
				printf("[CAN_RX_CLR_ALARM] id=0x%lX cmd=%u\r\n",
					   (unsigned long)rx.id, (unsigned int)rx.data[0]);
				continue;
			}
		}

		// 处理告警队列中的数据包
		if (g_queue_alarm != NULL)
		{
			APP_AlarmMsg_t alarm = {0};
			while (osMessageQueueGet(g_queue_alarm, &alarm, NULL, 0u) == osOK)
			{
				uint8_t data[APP_CAN_ALARM_DLC] = {0};
				data[0] = alarm.code;
				data[1] = (uint8_t)((uint16_t)alarm.value & 0xFFu);
				data[2] = (uint8_t)(((uint16_t)alarm.value >> 8) & 0xFFu);
				data[3] = (uint8_t)(alarm.tick & 0xFFu);
				(void)BSP_CAN_SendStd(APP_CAN_ID_ALARM_STD, data, APP_CAN_ALARM_DLC, APP_CAN_TX_TIMEOUT_MS);
				printf("[CAN_ALARM] code=%u value=%d tick=%u\r\n", (unsigned int)alarm.code, (int)alarm.value, (unsigned int)alarm.tick);
			}
		}

		// 处理采集任务队列中的数据包
		if (g_queue_can_tx != NULL)
		{
			BSP_CAN_Frame_t tx = {0};
			while (osMessageQueueGet(g_queue_can_tx, &tx, NULL, 0u) == osOK)
			{
				if (tx.ide == (uint8_t)CAN_ID_STD)
				{
					(void)BSP_CAN_SendStd((uint16_t)tx.id, tx.data, tx.dlc, APP_CAN_TX_TIMEOUT_MS);
				}
				else
				{
					(void)BSP_CAN_SendExt(tx.id, tx.data, tx.dlc, APP_CAN_TX_TIMEOUT_MS);
				}
			}
		}
        osDelay(10);
    }
}

void AssistTask(void *arg){
	(void)arg;
	uint32_t last_report_tick = 0;
    for(;;)
    {
		uint32_t now = osKernelGetTickCount();
		if ((now - last_report_tick) >= 1000u)
		{
			last_report_tick = now;
			printf("[辅助任务] StackHW(words): Data=%u Fault=%u Charge=%u SOC=%u Balance=%u CAN=%u Assist=%u\r\n",
				   (unsigned int)uxTaskGetStackHighWaterMark((TaskHandle_t)g_task_data_collect),
				   (unsigned int)uxTaskGetStackHighWaterMark((TaskHandle_t)g_task_fault_protect),
				   (unsigned int)uxTaskGetStackHighWaterMark((TaskHandle_t)g_task_charge_control),
				   (unsigned int)uxTaskGetStackHighWaterMark((TaskHandle_t)g_task_soc_calc),
				   (unsigned int)uxTaskGetStackHighWaterMark((TaskHandle_t)g_task_balance),
				   (unsigned int)uxTaskGetStackHighWaterMark((TaskHandle_t)g_task_can_comm),
				   (unsigned int)uxTaskGetStackHighWaterMark(NULL));
		}

        HAL_IWDG_Refresh(&hiwdg);
        osDelay(TASK_PERIOD_ASSIST);
    }
}

void APP_Trigger_Fault_Task(void)
{
    if (g_sem_fault_trigger != NULL)
    {
        osSemaphoreRelease(g_sem_fault_trigger); 
    }
}

void APP_Task_Create(void){
	g_sem_fault_trigger = osSemaphoreNew(APP_FAULT_SEM_MAX, 0, NULL);
	g_mutex_data = osMutexNew(NULL);
	g_mutex_i2c = osMutexNew(NULL);
	g_queue_can_tx = osMessageQueueNew(APP_CAN_TX_QUEUE_LEN, sizeof(BSP_CAN_Frame_t), NULL);
	g_queue_alarm = osMessageQueueNew(APP_ALARM_QUEUE_LEN, sizeof(APP_AlarmMsg_t), NULL);

	if ((g_sem_fault_trigger == NULL) || (g_mutex_data == NULL) || (g_mutex_i2c == NULL) || (g_queue_can_tx == NULL) || (g_queue_alarm == NULL))
	{
		printf("[APP] RTOS obj create FAIL sem=%p m_data=%p m_i2c=%p q_can=%p q_alarm=%p\r\n",
			   g_sem_fault_trigger, g_mutex_data, g_mutex_i2c, g_queue_can_tx, g_queue_alarm);
	}
	
	g_task_data_collect = osThreadNew(DataCollectTask, NULL, &(osThreadAttr_t){
		.name = "DataCollectTask",
		.priority = TASK_PRIO_DATA_COLLECT,
        .stack_size = TASK_STACK_DATA_COLLECT * 6,
	});
	if (g_task_data_collect == NULL) { printf("[APP] Create DataCollectTask FAIL\r\n"); }
	
	g_task_fault_protect = osThreadNew(FaultProtectTask, NULL, &(osThreadAttr_t){
		.name = "FaultProtectTask",
		.priority = TASK_PRIO_FAULT_PROTECT,
        .stack_size = TASK_STACK_FAULT_PROTECT * 4,
	});
	if (g_task_fault_protect == NULL) { printf("[APP] Create FaultProtectTask FAIL\r\n"); }

	g_task_charge_control = osThreadNew(ChargeControlTask, NULL, &(osThreadAttr_t){
		.name = "ChargeControlTask",
		.priority = TASK_PRIO_CHARGE_CONTROL,
		.stack_size = TASK_STACK_CHARGE_CONTROL * 4,
	});
	if (g_task_charge_control == NULL) { printf("[APP] Create ChargeControlTask FAIL\r\n"); }

	g_task_soc_calc = osThreadNew(SocCalcTask, NULL, &(osThreadAttr_t){
		.name = "SocCalcTask",
		.priority = TASK_PRIO_SOC_CALC,
		.stack_size = TASK_STACK_SOC_CALC * 4,
	});
	if (g_task_soc_calc == NULL) { printf("[APP] Create SocCalcTask FAIL\r\n"); }

	g_task_can_comm = osThreadNew(CanCommTask, NULL, &(osThreadAttr_t){
		.name = "CanCommTask",
		.priority = TASK_PRIO_CAN_COMM,
		.stack_size = TASK_STACK_CAN_COMM * 4,
	});
	if (g_task_can_comm == NULL) { printf("[APP] Create CanCommTask FAIL\r\n"); }

	g_task_balance = osThreadNew(BalanceTask, NULL, &(osThreadAttr_t){
		.name = "BalanceTask",
		.priority = TASK_PRIO_BALANCE,
		.stack_size = TASK_STACK_BALANCE * 4,
	});
	if (g_task_balance == NULL) { printf("[APP] Create BalanceTask FAIL\r\n"); }

	g_task_assist = osThreadNew(AssistTask, NULL, &(osThreadAttr_t){
		.name = "AssistTask",
		.priority = TASK_PRIO_ASSIST,
		.stack_size = TASK_STACK_ASSIST * 4,
	});
	if (g_task_assist == NULL) { printf("[APP] Create AssistTask FAIL\r\n"); }
}


