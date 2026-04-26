#ifndef __BSP_I2C_H
#define __BSP_I2C_H

#include <stdint.h>

uint8_t BSP_I2C_Write_Byte(uint8_t dev_addr, uint8_t reg, uint8_t data);
uint8_t BSP_I2C_Read_Byte(uint8_t dev_addr, uint8_t reg, uint8_t *data);
uint8_t BSP_I2C_Read_Buffer(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t BSP_I2C_Write_Buffer(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t BSP_I2C_IsDeviceReady(uint8_t dev_addr);


#endif
