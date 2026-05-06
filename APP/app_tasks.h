#ifndef __APP_TASKS_H
#define __APP_TASKS_H

#include <stdint.h>
#include "cmsis_os2.h"
#include "iwdg.h"
#include "bsp_bq76940.h"
#include "bsp_can.h"

typedef struct {
    float cell_voltage[BQ76940_CELL_NUM + 1]; // 15串 + 总电压
    float current;
    float temp;//温度
    uint8_t soc;
    uint8_t is_fault;
}BMS_Data_t;

// 	信号量
extern osSemaphoreId_t g_sem_fault_trigger;
extern osMutexId_t g_mutex_data;
extern osMutexId_t g_mutex_i2c;
extern osMessageQueueId_t g_queue_can_tx;
extern osMessageQueueId_t g_queue_alarm;

extern IWDG_HandleTypeDef hiwdg;
extern BMS_Data_t bms_data;

typedef enum
{
    APP_ALARM_OV = 1,
    APP_ALARM_OC = 2,
    APP_ALARM_OT = 3,
} APP_AlarmCode_t;

typedef struct
{
    uint8_t code;
    int16_t value;
    uint32_t tick;
} APP_AlarmMsg_t;

//	七大任务
void DataCollectTask(void *arg);    
void FaultProtectTask(void *arg);   
void ChargeControlTask(void *arg);  
void SocCalcTask(void *arg);       
void BalanceTask(void *arg);        
void CanCommTask(void *arg);        
void AssistTask(void *arg);         

//	任务初始化
void APP_Task_Create(void);         
void APP_Trigger_Fault_Task(void);

#endif
