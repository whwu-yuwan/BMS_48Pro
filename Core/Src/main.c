/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "can.h"
#include "i2c.h"
#include "iwdg.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "bsp_bq76940.h"
#include "crc8.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
static void BQ76940_Test_Init(void);
static void BQ76940_Test_Poll(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint8_t g_bq_inited = 0;

// 计算PEC
static uint8_t bq_calc_pec_read_u8(uint8_t dev_addr, uint8_t reg, uint8_t data)
{
  uint8_t crc = 0x00u;
  uint8_t addr = ((dev_addr << 1) | 1u);
  crc = CRC8_Update(crc, &addr, 1u, 0x07u);
  crc = CRC8_Update(crc, &data, 1u, 0x07u);
  return crc;
}

// 打印寄存器值，包含PEC
static void bq_dump_reg_u8_with_pec(uint8_t reg)
{
  uint8_t rx[2] = {0};
  if (BSP_I2C_Read_Buffer(BQ76940_ADDR, reg, rx, 2u) != 0)
  {
    printf("R[0x%02X]: read fail\r\n", reg);
    return;
  }
  uint8_t exp = bq_calc_pec_read_u8(BQ76940_ADDR, reg, rx[0]);
  printf("R[0x%02X]: D=%02X PEC=%02X EXP=%02X OK=%u\r\n", reg, rx[0], rx[1], exp, (rx[1] == exp) ? 1u : 0u);
}

static void BQ76940_Test_Init(void)
{
  g_bq_inited = 0;

  for (uint8_t i = 0; i < 5; i++)
  {
    if (BQ76940_Init() == 0)
    {
      g_bq_inited = 1;
      break;
    }
    HAL_IWDG_Refresh(&hiwdg);
    HAL_Delay(20);
  }

  if (g_bq_inited)
  {
    printf("BQ76940 init OK\r\n");
  }
  else
  {
    printf("BQ76940 init FAIL\r\n");
  }
}

static void BQ76940_Test_Poll(void)
{
  static uint32_t last_print_ms = 0;
  static uint32_t last_diag_ms = 0;

  HAL_IWDG_Refresh(&hiwdg);

  if (!g_bq_inited)
  {
    HAL_Delay(10);
    return;
  }

  uint32_t now = HAL_GetTick();
  if ((now - last_print_ms) < 500u)
  {
    HAL_Delay(10);
    return;
  }
  last_print_ms = now;

  float cell_v[BQ76940_CELL_NUM + 1] = {0};
  float pack_i = 0.0f;
  float ts1_v = 0.0f;
  uint8_t fault = 0;

  uint8_t ok_v = (BQ76940_ReadVoltage(cell_v) == 0);
  uint8_t ok_i = (BQ76940_ReadCurrent(&pack_i) == 0);
  uint8_t ok_t = (BQ76940_ReadTemp(&ts1_v) == 0);
  uint8_t ok_f = (BQ76940_ReadFault(&fault) == 0);

  printf("BQ:V=%u I=%u T=%u F=%u | I=%.3fA TS1=%.3fV STAT=0x%02X \r\n",
         ok_v, ok_i, ok_t, ok_f, pack_i, ts1_v, fault);

  printf("c1=%.3fV,c2=%.3fV,c3=%.3fV,c4=%.3fV,c5=%.3fV,c6=%.3fV,c7=%.3fV,c8=%.3fV,c9=%.3fV,c10=%.3fV,c11=%.3fV,c12=%.3fV,c13=%.3fV,c14=%.3fV,c15=%.3fV\r\n",
         cell_v[0], cell_v[1], cell_v[2], cell_v[3], cell_v[4], cell_v[5], cell_v[6], cell_v[7], cell_v[8], cell_v[9], cell_v[10], cell_v[11], cell_v[12], cell_v[13], cell_v[14]);

  if ((now - last_diag_ms) >= 2000u)
  {
    last_diag_ms = now;

    uint8_t adc_gain1 = 0;
    uint8_t adc_gain2 = 0;
    uint8_t adc_offset = 0;
    uint8_t sys_ctrl1 = 0;
    uint8_t sys_ctrl2 = 0;

    uint8_t ok_gain1 = (BSP_I2C_Read_Byte(BQ76940_ADDR, BQ76940_REG_ADCGAIN1, &adc_gain1) == 0);
    uint8_t ok_gain2 = (BSP_I2C_Read_Byte(BQ76940_ADDR, BQ76940_REG_ADCGAIN2, &adc_gain2) == 0);
    uint8_t ok_offset = (BSP_I2C_Read_Byte(BQ76940_ADDR, BQ76940_REG_ADCOFFSET, &adc_offset) == 0);
    uint8_t ok_sc1 = (BSP_I2C_Read_Byte(BQ76940_ADDR, BQ76940_REG_SYS_CTRL1, &sys_ctrl1) == 0);
    uint8_t ok_sc2 = (BSP_I2C_Read_Byte(BQ76940_ADDR, BQ76940_REG_SYS_CTRL2, &sys_ctrl2) == 0);

    printf("BQ_DIAG:ADCGAIN1(0x%02X)=%u ADCGAIN2(0x%02X)=%u ADCOFFSET(0x%02X)=%u SYS_CTRL1(0x%02X)=%u SYS_CTRL2(0x%02X)=%u\r\n",
           ok_gain1 ? adc_gain1 : 0xFF, ok_gain1, ok_gain2 ? adc_gain2 : 0xFF, ok_gain2, ok_offset ? adc_offset : 0xFF, ok_offset, ok_sc1 ? sys_ctrl1 : 0xFF, ok_sc1, ok_sc2 ? sys_ctrl2 : 0xFF, ok_sc2);

    printf("BQ_DIAG:SYS_CTRL1(ADC_EN=%u TEMP_SEL=%u SHUT_A=%u SHUT_B=%u) SYS_CTRL2(CC_EN=%u CHG_ON=%u DSG_ON=%u)\r\n",
           (sys_ctrl1 >> 4) & 0x01u, (sys_ctrl1 >> 3) & 0x01u, (sys_ctrl1 >> 1) & 0x01u, sys_ctrl1 & 0x01u,
           (sys_ctrl2 >> 6) & 0x01u, sys_ctrl2 & 0x01u, (sys_ctrl2 >> 1) & 0x01u);
  }


  BQ76940_ClearFault();
  }


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init();
  MX_I2C1_Init();
  MX_IWDG_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("Start...\r\n");
  BQ76940_Test_Init();
  
  
  
  /* USER CODE END 2 */

  /* Init scheduler */
  //osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  //MX_FREERTOS_Init();

  /* Start scheduler */
  //osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    //BQ76940_Test_Poll();
	
	  bq_dump_reg_u8_with_pec(BQ76940_REG_VC1_HI);
    bq_dump_reg_u8_with_pec((uint8_t)(BQ76940_REG_VC1_HI + 1u));
	
	HAL_IWDG_Refresh(&hiwdg);
	HAL_Delay(50);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
