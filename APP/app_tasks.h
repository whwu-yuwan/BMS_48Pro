#ifndef __APP_TASKS_C
#define __APP_TASKS_C

#include "cmsis_os2.h"
#include "iwdg.h"

typedef struct {
    float voltage;
    float current;
    uint8_t soc;
    uint8_t is_fault;
}BMS_Data_t;

// 	信号量
extern osSemaphoreId_t g_sem_fault_trigger;
extern osMutexId_t g_mutex_data;
extern osMutexId_t g_mutex_i2c;

extern IWDG_HandleTypeDef hiwdg;
extern BMS_Data_t bms_data;

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
