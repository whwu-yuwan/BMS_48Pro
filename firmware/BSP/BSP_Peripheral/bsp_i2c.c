#include "bsp_i2c.h"
#include "i2c.h"
#include "crc8.h"

static uint8_t bsp_i2c_calc_pec_write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint8_t len)
{
	uint8_t crc = 0x00u;
	uint8_t addr_w = (uint8_t)(dev_addr << 1);
	crc = CRC8_Update(crc, &addr_w, 1, 0x07u);
	crc = CRC8_Update(crc, &reg, 1, 0x07u);
	crc = CRC8_Update(crc, data, len, 0x07u);
	return crc;
}

static uint8_t bsp_i2c_calc_pec_read(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint8_t len)
{
	uint8_t crc = 0x00u;
	uint8_t addr_w = (uint8_t)(dev_addr << 1);
	uint8_t addr_r = (uint8_t)((dev_addr << 1) | 0x01u);
	crc = CRC8_Update(crc, &addr_w, 1, 0x07u);
	crc = CRC8_Update(crc, &reg, 1, 0x07u);
	crc = CRC8_Update(crc, &addr_r, 1, 0x07u);
	crc = CRC8_Update(crc, data, len, 0x07u);
	return crc;
}

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
		printf("BSP_I2C_Read_Byte: data is NULL\r\n");
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
		printf("BSP_I2C_Read_Buffer: buf is NULL or len is 0\r\n");
		return 1;
	}
	if (HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, 1000) == HAL_OK)
	{
		return 0;
	}
	printf("error");
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

uint8_t BSP_I2C_Write_Byte_PEC(uint8_t dev_addr, uint8_t reg, uint8_t data)
{
	uint8_t buf[2];
	buf[0] = data;
	buf[1] = bsp_i2c_calc_pec_write(dev_addr, reg, &buf[0], 1);

	if (HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, 2, 1000) == HAL_OK)
	{
		return 0;
	}
	return 1;
}

uint8_t BSP_I2C_Read_Byte_PEC(uint8_t dev_addr, uint8_t reg, uint8_t *data)
{
	if (data == NULL)
	{
		return 1;
	}

	uint8_t rx[2];
	if (HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, rx, 2, 1000) != HAL_OK)
	{
		return 1;
	}

	uint8_t pec = bsp_i2c_calc_pec_read(dev_addr, reg, &rx[0], 1);
	if (pec != rx[1])
	{
		return 1;
	}

	*data = rx[0];
	return 0;
}

uint8_t BSP_I2C_Read_Buffer_PEC(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
	if ((buf == NULL) || (len == 0))
	{
		return 1;
	}

	uint8_t rx[256];
	if (len > 255u)
	{
		return 1;
	}

	uint16_t rx_len = (uint16_t)len + 1u;
	if (HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, rx, rx_len, 1000) != HAL_OK)
	{
		return 1;
	}

	uint8_t pec = bsp_i2c_calc_pec_read(dev_addr, reg, &rx[0], len);
	if (pec != rx[len])
	{
		return 1;
	}

	for (uint8_t i = 0; i < len; i++)
	{
		buf[i] = rx[i];
	}
	return 0;
}

uint8_t BSP_I2C_Write_Buffer_PEC(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
	if ((buf == NULL) || (len == 0))
	{
		return 1;
	}

	if (len > 255u)
	{
		return 1;
	}

	uint8_t tx[256];
	for (uint8_t i = 0; i < len; i++)
	{
		tx[i] = buf[i];
	}
	tx[len] = bsp_i2c_calc_pec_write(dev_addr, reg, tx, len);

	uint16_t tx_len = (uint16_t)len + 1u;
	if (HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(dev_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, tx, tx_len, 1000) == HAL_OK)
	{
		return 0;
	}
	return 1;
}
