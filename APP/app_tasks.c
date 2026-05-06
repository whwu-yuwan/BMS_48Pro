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

void DataCollectTask(void *arg){
	(void)arg;
	float cell_v[BQ76940_CELL_NUM + 1] = {0};
	float current;
	for ( ; ; ){
		osMutexAcquire(g_mutex_i2c, osWaitForever);
		BQ76940_ReadVoltage(cell_v);
		BQ76940_ReadCurrent(&current);
		osMutexRelease(g_mutex_i2c);

		osMutexAcquire(g_mutex_data, osWaitForever);
		for (uint8_t i = 0; i < (uint8_t)(BQ76940_CELL_NUM + 1); i++)
		{
			bms_data.cell_voltage[i] = cell_v[i];
		}
		bms_data.current = current;
		bms_data.temp += 0.1f;
		float temp = bms_data.temp;
		float current_a = bms_data.current;
		uint8_t soc = bms_data.soc;
		osMutexRelease(g_mutex_data);
		if ((temp >= 3) || (current_a >= 2)){
			APP_Trigger_Fault_Task();
		}
		printf("[采样任务] 运行中... Tick: %u | 模拟读取15串总电压:[%.2f]电流:[%.2f]温度:[%.2f]\r\n",
			osKernelGetTickCount(),cell_v[BQ76940_CELL_NUM], current_a, temp);
		printf("[采样任务] 运行中... Tick: %u | 读取电池1:[%.2f],电池2:[%.2f],电池3:[%.2f],电池4:[%.2f],电池5:[%.2f],电池6:[%.2f],电池7:[%.2f],电池8:[%.2f],电池9:[%.2f],电池10:[%.2f],电池11:[%.2f],电池12:[%.2f],电池13:[%.2f],电池14:[%.2f],电池15:[%.2f]\r\n",
			 osKernelGetTickCount(), cell_v[0], cell_v[1], cell_v[2], cell_v[3], cell_v[4], cell_v[5], cell_v[6], cell_v[7], cell_v[8], cell_v[9], cell_v[10], cell_v[11], cell_v[12], cell_v[13], cell_v[14]);

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
			tx.id = 0x321u;
			tx.dlc = 6u;
			tx.data[0] = (uint8_t)(pack_mV & 0xFFu);
			tx.data[1] = (uint8_t)((pack_mV >> 8) & 0xFFu);
			tx.data[2] = (uint8_t)(current_mA & 0xFFu);
			tx.data[3] = (uint8_t)((current_mA >> 8) & 0xFFu);
			tx.data[4] = soc;
			tx.data[5] = cell_count;

			(void)osMessageQueuePut(g_queue_can_tx, &tx, 0u, 0u);
		}

		osDelay(TASK_PERIOD_DATA_COLLECT);
	}
}

void FaultProtectTask(void *arg){
	(void)arg;
	float voltage;
	float current;
    for(;;)
    {
		if (osSemaphoreAcquire(g_sem_fault_trigger, osWaitForever) == osOK){
			osMutexAcquire(g_mutex_data, osWaitForever);
			voltage = bms_data.cell_voltage[BQ76940_CELL_NUM];
			current = bms_data.current;
			osMutexRelease(g_mutex_data);
			if (voltage >= 3){
				printf("[故障任务] 运行中... Tick: %u | 模拟检测故障状态: 过压[%.2f]\r\n", osKernelGetTickCount(), voltage);
				if (g_queue_alarm != NULL)
				{
					APP_AlarmMsg_t alarm = {0};
					alarm.code = (uint8_t)APP_ALARM_OV;
					float mv = voltage * 1000.0f;
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

void ChargeControlTask(void *arg){
	(void)arg;
    for(;;)
    {
		osMutexAcquire(g_mutex_data, osWaitForever);
		uint8_t charge_mos = bms_data.charge_mos;
		uint8_t discharge_mos = bms_data.discharge_mos;
		osMutexRelease(g_mutex_data);
		printf("[充放电任务] 运行中... Tick: %u | 判断充放电状态: 充电mos: %d, 放电mos: %d\r\n", osKernelGetTickCount(), charge_mos, discharge_mos);
		osMutexAcquire(g_mutex_i2c, osWaitForever);
		(void)BQ76940_SetChargeMOS((charge_mos != 0u) ? 1u : 0u);
		(void)BQ76940_SetDischargeMOS((discharge_mos != 0u) ? 1u : 0u);
		osMutexRelease(g_mutex_i2c);

        osDelay(TASK_PERIOD_CHARGE_CONTROL);
    }
}

void SocCalcTask(void *arg){
	(void)arg;
	float soc = 0.f;
	uint8_t voltage;
    for(;;)
    {
		osMutexAcquire(g_mutex_data, osWaitForever);
		voltage = bms_data.cell_voltage[BQ76940_CELL_NUM];
		osMutexRelease(g_mutex_data);
		soc = BQ76940_CalcSOC(voltage);
		osMutexAcquire(g_mutex_data, osWaitForever);
		bms_data.soc = soc;
		osMutexRelease(g_mutex_data);
		printf("[SOC任务] 运行中... Tick: %u | 计算SOC: %d%% \r\n", osKernelGetTickCount(), soc);
        osDelay(TASK_PERIOD_SOC_CALC);
    }
}

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
		/*
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
			if (voltage[i] - voltage_min > 0.1f){
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
		*/
		printf("[均衡任务] 运行中... Tick: %u | 模拟控制电芯均衡: %d\r\n", osKernelGetTickCount(), onoff);
        osDelay(TASK_PERIOD_BALANCE_START + TASK_PERIOD_BALANCE_TIME + TASK_PERIOD_BALANCE_END);
    }
}

void CanCommTask(void *arg){
    (void)arg;
    for(;;)
    {
		BSP_CAN_Frame_t rx = {0};
		while (BSP_CAN_TryReceive(&rx) == 0u)
		{
			if (rx.ide != (uint8_t)CAN_ID_STD)
			{
				continue;
			}
			printf("[CAN_RX] Tick:%u IDE:%u ID:0x%lX DLC:%u D:%02X %02X %02X %02X %02X %02X %02X %02X\r\n",
				   osKernelGetTickCount(),
				   (unsigned int)rx.ide,
				   (unsigned long)rx.id,
				   (unsigned int)rx.dlc,
				   rx.data[0], rx.data[1], rx.data[2], rx.data[3], rx.data[4], rx.data[5], rx.data[6], rx.data[7]);

			APP_CAN_MosCtrl_t mos = {0};
			if (APP_CAN_DecodeMosCtrl(&rx, &mos) == 0u)
			{
				osMutexAcquire(g_mutex_data, osWaitForever);
				if ((mos.charge_mos == 0u) || (mos.charge_mos == 1u))
				{
					bms_data.charge_mos = mos.charge_mos;
				}
				if ((mos.discharge_mos == 0u) || (mos.discharge_mos == 1u))
				{
					bms_data.discharge_mos = mos.discharge_mos;
				}
				osMutexRelease(g_mutex_data);
				printf("[CAN_MOS] set_chg=%u set_dsg=%u\r\n", (unsigned int)mos.charge_mos, (unsigned int)mos.discharge_mos);
			}
		}

		if (g_queue_alarm != NULL)
		{
			APP_AlarmMsg_t alarm = {0};
			while (osMessageQueueGet(g_queue_alarm, &alarm, NULL, 0u) == osOK)
			{
				uint8_t data[4] = {0};
				data[0] = alarm.code;
				data[1] = (uint8_t)((uint16_t)alarm.value & 0xFFu);
				data[2] = (uint8_t)(((uint16_t)alarm.value >> 8) & 0xFFu);
				data[3] = (uint8_t)(alarm.tick & 0xFFu);
				(void)BSP_CAN_SendStd(0x320u, data, 4u, 10u);
				printf("[CAN_ALARM] code=%u value=%d tick=%u\r\n", (unsigned int)alarm.code, (int)alarm.value, (unsigned int)alarm.tick);
			}
		}


		if (g_queue_can_tx != NULL)
		{
			BSP_CAN_Frame_t tx = {0};
			while (osMessageQueueGet(g_queue_can_tx, &tx, NULL, 0u) == osOK)
			{
				if (tx.ide == (uint8_t)CAN_ID_STD)
				{
					(void)BSP_CAN_SendStd((uint16_t)tx.id, tx.data, tx.dlc, 10u);
				}
				else
				{
					(void)BSP_CAN_SendExt(tx.id, tx.data, tx.dlc, 10u);
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
	g_sem_fault_trigger = osSemaphoreNew(5, 0, NULL);
	g_mutex_data = osMutexNew(NULL);
	g_mutex_i2c = osMutexNew(NULL);
	g_queue_can_tx = osMessageQueueNew(16u, sizeof(BSP_CAN_Frame_t), NULL);
	g_queue_alarm = osMessageQueueNew(8u, sizeof(APP_AlarmMsg_t), NULL);
	
	g_task_data_collect = osThreadNew(DataCollectTask, NULL, &(osThreadAttr_t){
		.name = "DataCollectTask",
		.priority = TASK_PRIO_DATA_COLLECT,
        .stack_size = TASK_STACK_DATA_COLLECT * 4,
	});
	
	g_task_fault_protect = osThreadNew(FaultProtectTask, NULL, &(osThreadAttr_t){
		.name = "FaultProtectTask",
		.priority = TASK_PRIO_FAULT_PROTECT,
        .stack_size = TASK_STACK_FAULT_PROTECT * 4,
	});

	g_task_charge_control = osThreadNew(ChargeControlTask, NULL, &(osThreadAttr_t){
		.name = "ChargeControlTask",
		.priority = TASK_PRIO_CHARGE_CONTROL,
		.stack_size = TASK_STACK_CHARGE_CONTROL * 4,
	});

	g_task_soc_calc = osThreadNew(SocCalcTask, NULL, &(osThreadAttr_t){
		.name = "SocCalcTask",
		.priority = TASK_PRIO_SOC_CALC,
		.stack_size = TASK_STACK_SOC_CALC * 4,
	});

	g_task_can_comm = osThreadNew(CanCommTask, NULL, &(osThreadAttr_t){
		.name = "CanCommTask",
		.priority = TASK_PRIO_CAN_COMM,
		.stack_size = TASK_STACK_CAN_COMM * 4,
	});

	g_task_balance = osThreadNew(BalanceTask, NULL, &(osThreadAttr_t){
		.name = "BalanceTask",
		.priority = TASK_PRIO_BALANCE,
		.stack_size = TASK_STACK_BALANCE * 4,
	});

	g_task_assist = osThreadNew(AssistTask, NULL, &(osThreadAttr_t){
		.name = "AssistTask",
		.priority = TASK_PRIO_ASSIST,
		.stack_size = TASK_STACK_ASSIST * 4,
	});
}




