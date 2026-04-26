#ifndef __BSP_BQ76940_H
#define __BSP_BQ76940_H

#include "main.h"
#include "bsp_i2c.h"  

// ==================== 配置 ====================
#define BQ76940_ADDR            0x08    // BQ76940 I2C地址
#define BQ76940_CELL_NUM        13      // 13串

#define BQ76940_REG_SYS_STAT    0x00
#define BQ76940_REG_SYS_CTRL1   0x04
#define BQ76940_REG_SYS_CTRL2   0x05

#define BQ76940_REG_VC1_HI      0x0C
#define BQ76940_REG_TS1_HI      0x2C
#define BQ76940_REG_CC_HI       0x32

#define BQ76940_REG_ADCGAIN1    0x50
#define BQ76940_REG_ADCGAIN2    0x51
#define BQ76940_REG_ADCOFFSET   0x52

#define BQ76940_SYS_CTRL1_ADC_EN    0x10
#define BQ76940_SYS_CTRL1_TEMP_SEL  0x08

#define BQ76940_SYS_CTRL2_CHG_ON    0x01
#define BQ76940_SYS_CTRL2_DSG_ON    0x02
#define BQ76940_SYS_CTRL2_CC_EN     0x40

#ifndef BQ76940_SHUNT_RESISTOR_MOHM
#define BQ76940_SHUNT_RESISTOR_MOHM 1.0f
#endif

#ifndef BQ76940_CC_LSB_UV
#define BQ76940_CC_LSB_UV 8.44f
#endif

// ==================== 外部接口 ====================
uint8_t BQ76940_Init(void);                            // 初始化
uint8_t BQ76940_ReadVoltage(float *volt_array);        // 读13串电压
uint8_t BQ76940_ReadCurrent(float *current);           // 读总电流
uint8_t BQ76940_ReadTemp(float *temp);                 // 读温度
uint8_t BQ76940_ReadFault(uint8_t *fault);             // 读故障
uint8_t BQ76940_ClearFault(void);                      // 清除故障
uint8_t BQ76940_SetDischargeMOS(uint8_t onoff);       // 放电MOS
uint8_t BQ76940_SetChargeMOS(uint8_t onoff);           // 充电MOS

#endif
