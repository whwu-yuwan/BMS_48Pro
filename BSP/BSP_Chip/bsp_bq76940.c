#include "bsp_bq76940.h"
#include "math.h"

/*
 * BQ76940 驱动说明：
 * - I2C 地址在 bsp_bq76940.h 里配置（BQ76940_ADDR）。
 * - 支持 PEC(CRC8/SMBus PEC) 读写：写按 BQ76940_USE_PEC 决定是否附带 PEC；读按 BQ76940_READ_USE_PEC 决定是否使用 PEC 校验。
 * - 支持 WAKE 唤醒脚：上电后可通过对指定 GPIO 输出一个上升沿/脉冲唤醒器件。
 */

/* ADC 增益/偏移：由器件寄存器 ADCGAIN/ADCOFFSET 计算得到 */
static uint16_t g_bq_gain_uV = 365;
static int16_t g_bq_offset_mV = 0;

/* 上电/复位后唤醒：对 WAKE 引脚输出一次脉冲或上升沿（具体电平保持可配置） */
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

/* 读 1 字节寄存器：按编译期开关决定是否使用 PEC 读 */
static uint8_t bq_read_u8(uint8_t reg, uint8_t *val)
{
	if (BQ76940_READ_USE_PEC)
	{
		return BSP_I2C_Read_Byte_PEC(BQ76940_ADDR, reg, val);
	}
	return BSP_I2C_Read_Byte(BQ76940_ADDR, reg, val);
}

static uint8_t bq_read_u16(uint8_t reg_hi, uint16_t *val)
{
	uint8_t hi = 0;
	uint8_t lo = 0;
	if (val == NULL)
	{
		return 1;
	}
	if (bq_read_u8(reg_hi, &hi) != 0)
	{
		return 1;
	}
	if (bq_read_u8((uint8_t)(reg_hi + 1u), &lo) != 0)
	{
		return 1;
	}
	*val = (uint16_t)(((uint16_t)hi << 8) | lo);
	return 0;
}



/* 写 1 字节寄存器：按 BQ76940_USE_PEC 决定是否附带 PEC */
static uint8_t bq_write_u8(uint8_t reg, uint8_t val)
{
	if (BQ76940_USE_PEC)
	{
		return BSP_I2C_Write_Byte_PEC(BQ76940_ADDR, reg, val);
	}
	return BSP_I2C_Write_Byte(BQ76940_ADDR, reg, val);
}

/* 14-bit ADC 原始值转换为 mV：raw14 * gain(uV/LSB) + offset(mV) */
static float bq_adc_raw_to_mV(uint16_t raw14)
{
	float uV = (float)raw14 * (float)g_bq_gain_uV;
	float mV = (uV / 1000.0f) + (float)g_bq_offset_mV;
	return mV;
}

/* 16-bit ADC 原始值转换为mA: */
static float bq_adc_raw_to_mA(int16_t raw16, float resistor, float LSB){
	float v_sense = ((float)raw16 * (LSB * 1e-6f));
	float r_shunt = resistor * 1e-3f;
	if (r_shunt <= 0.0f)
	{
		return 1;
	}
	return v_sense / r_shunt;
}

static float bq_adc_mV_to_oC(float ntc_mv) {
	
    const float R_PULL  = 10000.0f;   // 内置上拉电阻
    const float VCC_MV  = 3300.0f;    // 上拉电源
    const float R0      = 10000.0f;   // 25℃ NTC阻值
    const float B_VALUE = 3950.0f;    // NTC B值
    const float T0      = 298.15f;    // 25℃ 开尔文温度

    // 防除零
    if(ntc_mv >= VCC_MV) ntc_mv = VCC_MV - 1;
    if(ntc_mv < 0) ntc_mv = 0;

    // 1. 电压 → NTC电阻
    float r_ntc = R_PULL * ntc_mv / (VCC_MV - ntc_mv);

    // 2. 电阻 → 温度
    float ln = logf(r_ntc / R0);
    float temp_k = 1.0f / ((1.0f/T0) + (ln/B_VALUE));
    return temp_k - 273.15f;
}


static uint8_t bq_get_gain12_offset(uint16_t *g_gain, int16_t *g_offset) {
	/* 读取 ADC 增益/偏移用于后续电压/温度换算 */
	uint8_t gain1;
	uint8_t gain2;
	uint8_t offset;
	if (bq_read_u8(BQ76940_REG_ADCGAIN1, &gain1) != 0)
	{
		return 1;
	}
	if (bq_read_u8(BQ76940_REG_ADCGAIN2, &gain2) != 0)
	{
		return 1;
	}
	if (bq_read_u8(BQ76940_REG_ADCOFFSET, &offset) != 0)
	{
		return 1;
	}
	/* ADCGAIN<4:0> = ADCGAIN1[6:5](=bits4..3) + ADCGAIN2[7:5](=bits2..0)，单位 uV/LSB */
	uint8_t gain_code = (uint8_t)((((gain1 >> 5) & 0x03u) << 3) | ((gain2 >> 5) & 0x07u));
	*g_gain = (uint16_t)(365u + (uint16_t)gain_code);
	/* ADCOFFSET 为有符号 8-bit，单位 mV */
	*g_offset = (int16_t)(int8_t)offset;
	return 0;
}

static uint8_t bq_isBQ76940Ready(void){
	if (BSP_I2C_IsDeviceReady(BQ76940_ADDR) != 0)
	{
		return 1;
	}
	return 0;
}

static uint8_t bq_config_ctrl1_2(void){
	uint8_t sys_ctrl1 = 0;
	uint8_t sys_ctrl2 = 0;

	/* 打开 ADC、温度采样源选择、库仑计等功能 */
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

uint8_t BQ76940_Init(void)
{
	//上电后先唤醒器件（需要 WAKE 上升沿/脉冲时启用）
	bq_wake_pulse();
	//探测 I2C 设备是否应答 
	if (bq_isBQ76940Ready() != 0){
		return 1;
	}
	//获取gain和offset
	if (bq_get_gain12_offset(&g_bq_gain_uV, &g_bq_offset_mV) != 0){
		return 1;
	}
	//初始化ctrl1_2
	if (bq_config_ctrl1_2() != 0){
		return 1;
	}
	return 0;
}

/* 读取 15 串电芯电压（单位：V） */
uint8_t BQ76940_ReadVoltage(float *volt_array)
{
	if (volt_array == NULL)
	{
		return 1;
	}
	for (uint8_t i = 0; i < BQ76940_CELL_NUM; i++)
	{
		if (((BQ76940_CELL_PRESENT_MASK >> i) & 0x01u) == 0u)
		{
			volt_array[i] = 0.0f;
			continue;
		}

		uint16_t raw = 0;
		uint8_t reg_hi = (uint8_t)(BQ76940_REG_VC1_HI + (uint8_t)(i * 2u));
		if (bq_read_u16(reg_hi, &raw) != 0)
		{
			return 1;
		}

		/* 每节电压 2 字节，高字节在前；有效位为 14-bit */
		raw &= 0x3FFFu;
		float mV = bq_adc_raw_to_mV(raw);
		volt_array[i] = mV / 1000.0f;
	}
	return 0;
}

/* 读取电流（单位：A），使用库仑计瞬时寄存器 CC_HI/CC_LO */
uint8_t BQ76940_ReadCurrent(float *current)
{
	if (current == NULL)
	{
		return 1;
	}

	uint16_t u16 = 0;
	if (bq_read_u16(BQ76940_REG_CC_HI, &u16) != 0)
	{
		return 1;
	}

	/* CC 为有符号 16-bit；转换为 Vsense 后除以分流电阻得到电流 */
	int16_t raw = (int16_t)u16;
	*current = bq_adc_raw_to_mA(raw, BQ76940_SHUNT_RESISTOR_MOHM, BQ76940_CC_LSB_UV);

	return 0;
}

/* 读取温度通道（此处读取 TS1），返回值单位：V（由 ADC 转换而来，后续可再换算为温度） */
uint8_t BQ76940_ReadTemp(float *temp)
{
	if (temp == NULL)
	{
		return 1;
	}

	uint16_t raw = 0;
	if (bq_read_u16(BQ76940_REG_TS1_HI, &raw) != 0)
	{
		return 1;
	}
	raw &= 0x3FFFu;
	*temp = bq_adc_raw_to_mV(raw);
	*temp = bq_adc_mV_to_oC(*temp);
	return 0;
}

/* 读取系统状态（故障位等） */
uint8_t BQ76940_ReadFault(uint8_t *fault)
{
	if (fault == NULL)
	{
		return 1;
	}
	return bq_read_u8(BQ76940_REG_SYS_STAT, fault);
}

/* 清除故障：按手册要求对 SYS_STAT 进行读后回写以清除相应锁存位 */
uint8_t BQ76940_ClearFault(void)
{
	uint8_t sys_stat = 0;
	if (bq_read_u8(BQ76940_REG_SYS_STAT, &sys_stat) != 0)
	{
		return 1;
	}
	return bq_write_u8(BQ76940_REG_SYS_STAT, sys_stat);
}

/* 控制放电 MOS（DSG） */
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

/* 控制充电 MOS（CHG） */
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
