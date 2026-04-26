#include "app_tasks.h"
#include <stdio.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "bms_config.h"
#include "cmsis_os2.h"
#include "iwdg.h"

osSemaphoreId_t g_sem_fault_trigger = NULL;
osMutexId_t g_mutex_data = NULL;
osMutexId_t g_mutex_i2c = NULL;
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
	for ( ; ; ){
		osMutexAcquire(g_mutex_data, osWaitForever);
		bms_data.voltage += 0.1f;
		bms_data.current += 0.1f;
		float voltage = bms_data.voltage;
		float current = bms_data.current;
		osMutexRelease(g_mutex_data);
		if ((voltage >= 3) || (current >= 2)){
			APP_Trigger_Fault_Task();
		}
		printf("[采样任务] 运行中... Tick: %u | 模拟读取13串电压:[%.2f]电流:[%.2f]温度:[]\r\n", osKernelGetTickCount(), voltage, current);
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
			voltage = bms_data.voltage;
			current = bms_data.current;
			osMutexRelease(g_mutex_data);
			if (voltage >= 3){
				printf("[故障任务] 运行中... Tick: %u | 模拟检测故障状态: 过压[%.2f]\r\n", osKernelGetTickCount(), voltage);
				osMutexAcquire(g_mutex_data, osWaitForever);
				bms_data.voltage = 0.f;
				osMutexRelease(g_mutex_data);
			}
			if (current >= 2){
				printf("[故障任务] 运行中... Tick: %u | 模拟检测故障状态: 过流[%.2f]\r\n", osKernelGetTickCount(), current);
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
		 printf("[充放电任务] 运行中... Tick: %u | 模拟判断充放电状态\r\n", osKernelGetTickCount());
        osDelay(TASK_PERIOD_CHARGE_CONTROL);
    }
}

void SocCalcTask(void *arg){
	(void)arg;
	uint8_t soc = 50;
    for(;;)
    {
		osMutexAcquire(g_mutex_data, osWaitForever);
		bms_data.soc = soc;
		osMutexRelease(g_mutex_data);
		printf("[SOC任务] 运行中... Tick: %u | 模拟计算SOC: %d \r\n", osKernelGetTickCount(), soc);
        osDelay(TASK_PERIOD_SOC_CALC);
    }
}

void BalanceTask(void *arg){
	(void)arg;
    for(;;)
    {
		printf("[均衡任务] 运行中... Tick: %u | 模拟控制电芯均衡\r\n", osKernelGetTickCount());
        osDelay(TASK_PERIOD_BALANCE_START + TASK_PERIOD_BALANCE_TIME + TASK_PERIOD_BALANCE_END);
    }
}

void CanCommTask(void *arg){
    (void)arg;
    for(;;)
    {
		printf("[CAN任务] 运行中... Tick: %u | 模拟CAN收发数据\r\n", osKernelGetTickCount());
        osDelay(TASK_PERIOD_CAN_COMM);
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





