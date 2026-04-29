#ifndef __BSP_BQ76940_H
#define __BSP_BQ76940_H

#include "main.h"
#include "bsp_i2c.h"  

// ==================== 配置 ====================
#define BQ76940_ADDR            0x08    // BQ76940 I2C地址
#define BQ76940_CELL_NUM        15      // 15串

/* 电芯存在掩码：bit0=cell1 ... bit14=cell15，1=有电芯，0=无电芯 */
#ifndef BQ76940_CELL_PRESENT_MASK
#define BQ76940_CELL_PRESENT_MASK 0x4E73u
#endif

#ifndef BQ76940_USE_PEC
#define BQ76940_USE_PEC 1
#endif

/* 读寄存器是否使用 PEC 校验：0=不使用，1=使用 */
#define BQ76940_READ_USE_PEC 0

#define BQ76940_USE_WAKE_PIN 1
#define BQ76940_WAKE_GPIO_PORT GPIOA
#define BQ76940_WAKE_GPIO_PIN GPIO_PIN_8
#define BQ76940_WAKE_LOW_MS 2u
#define BQ76940_WAKE_HIGH_MS 2u
#define BQ76940_WAKE_POST_MS 10u
#define BQ76940_WAKE_END_HIGH 1


#define BQ76940_REG_SYS_STAT    0x00
#define BQ76940_REG_SYS_CTRL1   0x04
#define BQ76940_REG_SYS_CTRL2   0x05

#define BQ76940_REG_VC1_HI      0x0C
#define BQ76940_REG_TS1_HI      0x2C
#define BQ76940_REG_CC_HI       0x32

#define BQ76940_REG_ADCGAIN1    0x50
#define BQ76940_REG_ADCOFFSET   0x51
#define BQ76940_REG_ADCGAIN2    0x59

#define BQ76940_SYS_CTRL1_ADC_EN    0x10
#define BQ76940_SYS_CTRL1_TEMP_SEL  0x08

#define BQ76940_SYS_CTRL2_CHG_ON    0x01
#define BQ76940_SYS_CTRL2_DSG_ON    0x02
#define BQ76940_SYS_CTRL2_CC_EN     0x40

#ifndef BQ76940_SHUNT_RESISTOR_MOHM
#define BQ76940_SHUNT_RESISTOR_MOHM 4.0f
#endif

#ifndef BQ76940_CC_LSB_UV
#define BQ76940_CC_LSB_UV 8.44f
#endif

// ==================== 外部接口 ====================
uint8_t BQ76940_Init(void);                            // 初始化
uint8_t BQ76940_ReadVoltage(float *volt_array);        // 读15串电压（无电芯通道按掩码置0）
uint8_t BQ76940_ReadCurrent(float *current);           // 读总电流
uint8_t BQ76940_ReadTemp(float *temp);                 // 读温度
uint8_t BQ76940_ReadFault(uint8_t *fault);             // 读故障
uint8_t BQ76940_ClearFault(void);                      // 清除故障
uint8_t BQ76940_SetDischargeMOS(uint8_t onoff);       // 放电MOS
uint8_t BQ76940_SetChargeMOS(uint8_t onoff);           // 充电MOS

#endif
