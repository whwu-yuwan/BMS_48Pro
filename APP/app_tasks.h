#ifndef __APP_TASKS_C
#define __APP_TASKS_C

void DataCollectTask(void *arg);    // 数据采集任务
void FaultProtectTask(void *arg);  // 故障保护任务
void ChargeControlTask(void *arg);  // 充放电控制任务
void SocCalcTask(void *arg);        // SOC计算任务
void BalanceTask(void *arg);        // 均衡任务
void CanCommTask(void *arg);        // CAN通信任务
void AssistTask(void *arg);         // 辅助功能任务

#endif
