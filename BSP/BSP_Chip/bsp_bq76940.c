#include "bsp_bq76940.h"

static uint16_t g_bq_gain_uV = 365;
static int16_t g_bq_offset_mV = 0;
static uint8_t g_bq_read_use_pec = (uint8_t)BQ76940_USE_PEC;

static void bq_wake_pulse(void)
{
	if (!BQ76940_USE_WAKE_PIN)
	{
		return;
	}

	HAL_GPIO_WritePin(BQ76940_WAKE_GPIO_PORT, BQ76940_WAKE_GPIO_PIN, GPIO_PIN_RESET);
	HAL_Delay((uint32_t)BQ76940_WAKE_LOW_MS);
	HAL_GPIO_WritePin(BQ76940_WAKE_GPIO_PORT, BQ76940_WAKE_GPIO_PIN, GPIO_PIN_SET);
	HAL_Delay((uint32_t)BQ76940_WAKE_HIGH_MS);
	if (!BQ76940_WAKE_END_HIGH)
	{
		HAL_GPIO_WritePin(BQ76940_WAKE_GPIO_PORT, BQ76940_WAKE_GPIO_PIN, GPIO_PIN_RESET);
	}
	HAL_Delay((uint32_t)BQ76940_WAKE_POST_MS);
}

static uint8_t bq_read_u8(uint8_t reg, uint8_t *val)
{
	if (g_bq_read_use_pec)
	{
		return BSP_I2C_Read_Byte_PEC(BQ76940_ADDR, reg, val);
	}
	return BSP_I2C_Read_Byte(BQ76940_ADDR, reg, val);
}

static uint8_t bq_write_u8(uint8_t reg, uint8_t val)
{
	if (BQ76940_USE_PEC)
	{
		return BSP_I2C_Write_Byte_PEC(BQ76940_ADDR, reg, val);
	}
	return BSP_I2C_Write_Byte(BQ76940_ADDR, reg, val);
}

static uint8_t bq_read_buf(uint8_t reg, uint8_t *buf, uint8_t len)
{
	if (g_bq_read_use_pec)
	{
		return BSP_I2C_Read_Buffer_PEC(BQ76940_ADDR, reg, buf, len);
	}
	return BSP_I2C_Read_Buffer(BQ76940_ADDR, reg, buf, len);
}

static float bq_adc_raw_to_mV(uint16_t raw14)
{
	float uV = (float)raw14 * (float)g_bq_gain_uV;
	float mV = (uV / 1000.0f) + (float)g_bq_offset_mV;
	return mV;
}

uint8_t BQ76940_Init(void)
{
	bq_wake_pulse();
	g_bq_read_use_pec = (uint8_t)BQ76940_USE_PEC;

	if (BSP_I2C_IsDeviceReady(BQ76940_ADDR) != 0)
	{
		return 1;
	}

	uint8_t gain1 = 0;
	uint8_t gain2 = 0;
	uint8_t offset = 0;

	if (bq_read_u8(BQ76940_REG_ADCGAIN1, &gain1) != 0)
	{
		if (g_bq_read_use_pec)
		{
			g_bq_read_use_pec = 0;
			if (bq_read_u8(BQ76940_REG_ADCGAIN1, &gain1) != 0)
			{
				return 1;
			}
		}
		else
		{
			return 1;
		}
	}
	if (bq_read_u8(BQ76940_REG_ADCGAIN2, &gain2) != 0)
	{
		return 1;
	}
	if (bq_read_u8(BQ76940_REG_ADCOFFSET, &offset) != 0)
	{
		return 1;
	}

	uint16_t gain_raw = (uint16_t)(((uint16_t)(gain2 & 0x07) << 8) | gain1);
	g_bq_gain_uV = (uint16_t)(365u + gain_raw);
	g_bq_offset_mV = (int16_t)(int8_t)offset;

	uint8_t sys_ctrl1 = 0;
	uint8_t sys_ctrl2 = 0;

	if (bq_read_u8(BQ76940_REG_SYS_CTRL1, &sys_ctrl1) != 0)
	{	
		return 1;
	}
	if (bq_read_u8(BQ76940_REG_SYS_CTRL2, &sys_ctrl2) != 0)
	{
		return 1;
	}

	sys_ctrl1 |= (uint8_t)(BQ76940_SYS_CTRL1_ADC_EN | BQ76940_SYS_CTRL1_TEMP_SEL);
	sys_ctrl2 |= BQ76940_SYS_CTRL2_CC_EN;

	if (bq_write_u8(BQ76940_REG_SYS_CTRL1, sys_ctrl1) != 0)
	{
		return 1;
	}
	if (bq_write_u8(BQ76940_REG_SYS_CTRL2, sys_ctrl2) != 0)
	{
		return 1;
	}

	return 0;
}

uint8_t BQ76940_ReadVoltage(float *volt_array)
{
	if (volt_array == NULL)
	{
		return 1;
	}
	uint8_t buf[BQ76940_CELL_NUM * 2];
	if (bq_read_buf(BQ76940_REG_VC1_HI, buf, (uint8_t)sizeof(buf)) != 0)
	{
		return 1;
	}
	for (uint8_t i = 0; i < BQ76940_CELL_NUM; i++)
	{
		uint16_t raw = (uint16_t)(((uint16_t)buf[i * 2] << 8) | buf[i * 2 + 1]);
		raw &= 0x3FFFu;
		float mV = bq_adc_raw_to_mV(raw);
		volt_array[i] = mV / 1000.0f;
	}
	return 0;
}

uint8_t BQ76940_ReadCurrent(float *current)
{
	if (current == NULL)
	{
		return 1;
	}

	uint8_t buf[2];
	if (bq_read_buf(BQ76940_REG_CC_HI, buf, 2) != 0)
	{
		return 1;
	}

	int16_t raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
	float v_sense = ((float)raw * (BQ76940_CC_LSB_UV * 1e-6f));
	float r_shunt = BQ76940_SHUNT_RESISTOR_MOHM * 1e-3f;
	if (r_shunt <= 0.0f)
	{
		return 1;
	}

	*current = v_sense / r_shunt;
	return 0;
}

uint8_t BQ76940_ReadTemp(float *temp)
{
	if (temp == NULL)
	{
		return 1;
	}

	uint8_t buf[2];
	if (bq_read_buf(BQ76940_REG_TS1_HI, buf, 2) != 0)
	{
		return 1;
	}

	uint16_t raw = (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
	raw &= 0x3FFFu;
	*temp = bq_adc_raw_to_mV(raw) / 1000.0f;
	return 0;
}

uint8_t BQ76940_ReadFault(uint8_t *fault)
{
	if (fault == NULL)
	{
		return 1;
	}

	return bq_read_u8(BQ76940_REG_SYS_STAT, fault);
}

uint8_t BQ76940_ClearFault(void)
{
	uint8_t sys_stat = 0;
	if (bq_read_u8(BQ76940_REG_SYS_STAT, &sys_stat) != 0)
	{
		return 1;
	}
	return bq_write_u8(BQ76940_REG_SYS_STAT, sys_stat);
}

uint8_t BQ76940_SetDischargeMOS(uint8_t onoff)
{
	uint8_t sys_ctrl2 = 0;
	if (bq_read_u8(BQ76940_REG_SYS_CTRL2, &sys_ctrl2) != 0)
	{
		return 1;
	}

	if (onoff != 0)
	{
		sys_ctrl2 |= BQ76940_SYS_CTRL2_DSG_ON;
	}
	else
	{
		sys_ctrl2 &= (uint8_t)~BQ76940_SYS_CTRL2_DSG_ON;
	}

	return bq_write_u8(BQ76940_REG_SYS_CTRL2, sys_ctrl2);
}

uint8_t BQ76940_SetChargeMOS(uint8_t onoff)
{
	uint8_t sys_ctrl2 = 0;
	if (bq_read_u8(BQ76940_REG_SYS_CTRL2, &sys_ctrl2) != 0)
	{
		return 1;
	}

	if (onoff != 0)
	{
		sys_ctrl2 |= BQ76940_SYS_CTRL2_CHG_ON;
	}
	else
	{
		sys_ctrl2 &= (uint8_t)~BQ76940_SYS_CTRL2_CHG_ON;
	}

	return bq_write_u8(BQ76940_REG_SYS_CTRL2, sys_ctrl2);
}
