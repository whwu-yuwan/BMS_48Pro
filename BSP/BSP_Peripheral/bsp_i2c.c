#include "bsp_i2c.h"
#include "i2c.h"

uint8_t BSP_I2C_Write_Byte(uint8_t dev_addr, uint8_t reg, uint8_t data) {
	if (HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 1000) == HAL_OK)
	{
		return 0;
	}
	return 1;
}

uint8_t BSP_I2C_Read_Byte(uint8_t dev_addr, uint8_t reg, uint8_t *data) {
	if (data == NULL)
	{
		return 1;
	}
	if (HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, data, 1, 1000) == HAL_OK)
	{
		return 0;
	}
	return 1;
}

uint8_t BSP_I2C_Read_Buffer(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint8_t len) {
	if ((buf == NULL) || (len == 0))
	{
		return 1;
	}
	if (HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, 1000) == HAL_OK)
	{
		return 0;
	}
	return 1;
}

uint8_t BSP_I2C_Write_Buffer(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint8_t len) {
	if ((buf == NULL) || (len == 0))
	{
		return 1;
	}
	if (HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, 1000) == HAL_OK)
	{
		return 0;
	}
	return 1;
}

uint8_t BSP_I2C_IsDeviceReady(uint8_t dev_addr) {
	if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(dev_addr << 1), 2, 1000) == HAL_OK)
	{
		return 0;
	}
	return 1;
}

