#ifndef __BMS_CONFIG_H
#define __BMS_CONFIG_H

#include "freertos.h"
#include <stdint.h>

#define TASK_PRIO_FAULT_PROTECT   osPriorityHigh
#define TASK_PRIO_CHARGE_CONTROL  osPriorityAboveNormal
#define TASK_PRIO_CAN_COMM		  osPriorityAboveNormal    
#define TASK_PRIO_SOC_CALC		  osPriorityNormal
#define TASK_PRIO_DATA_COLLECT    osPriorityNormal
#define TASK_PRIO_ASSIST          osPriorityLow
#define TASK_PRIO_BALANCE   	  osPriorityLow

#define TASK_STACK_FAULT_PROTECT   256
#define TASK_STACK_CHARGE_CONTROL  128
#define TASK_STACK_CAN_COMM        256
#define TASK_STACK_DATA_COLLECT    128
#define TASK_STACK_SOC_CALC        128
#define TASK_STACK_BALANCE         128
#define TASK_STACK_ASSIST          128

#define TASK_PERIOD_DATA_COLLECT   250
#define TASK_PERIOD_CHARGE_CONTROL 100
#define TASK_PERIOD_SOC_CALC       250        
#define TASK_PERIOD_CAN_COMM       100
#define TASK_PERIOD_ASSIST         100
#define TASK_PERIOD_BALANCE_START  750
#define TASK_PERIOD_BALANCE_TIME   500
#define TASK_PERIOD_BALANCE_END    250

#endif
