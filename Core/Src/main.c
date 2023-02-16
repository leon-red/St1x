/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "u8g2.h"
#include "u8g2_OLED.h"
#include "St1xadc.h"
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
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/********** LED *********/
//uint16_t duty_num = 10; //LED
/********** LED *********/

/************************ADC测试***************************/

/************************ADC测试***************************/


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

/*****初始化按键*****/
//    KEY_Init();
/*****初始化按键*****/
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_USB_DEVICE_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
/************************ADC初始化***************************/
    ADC_IN_1();
    uint16_t DMA_ADC[2];
    HAL_ADCEx_Calibration_Start(&hadc1);//ADC采样校准
    HAL_ADC_Start_DMA(&hadc1,(uint32_t*)&DMA_ADC,2);//启动DMA，采集数据存入的变量地址，长度1
/************************ADC测试***************************/

/************************LED测试***************************/
    uint16_t duty_num = 10; //LED
    HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_4);
/************************LED测试***************************/

//    OLED_Init();
//    OLED_Clear();
//    Display_Char_CRAM(255,0,10,50,16);
//    OLED_ColorTurn(1);
//    OLED_DisplayTurn(1);
//    OLED_ShowString(0,0,"www.taxing.com",16);
//    OLED_ShowCHinese(0,4,0);//
//    OLED_ShowCHinese(16,4,1);//
//    OLED_ShowCHinese(32,4,2);//
//    OLED_ShowCHinese(48,4,6);//
//    OLED_ShowCHinese(0,6,3);//
//    OLED_ShowCHinese(16,6,4);//
//    OLED_ShowCHinese(32,6,5);//
//    OLED_ShowCHinese(48,6,6);//

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
/*****初始化OLED*****/
    u8g2_t u8g2;
    oled_Init(&u8g2);
/*****初始化OLED*****/
  while (1)
  {
/************************ADC测试***************************/
//      printf("ADC1分压电压=%04f USB电压分压电压=%04f\r\n",DMA_ADC[0]*3.3/4095,DMA_ADC[1]*3.3/4095);
//      printf("ADC1采样数据=%d ADC2采样数据=%d\r\n",DMA_ADC[0],DMA_ADC[1]);
//      HAL_Delay(800);
//      printf("/************************************/\r\n");
/************************ADC测试***************************/

/************************LED测试***************************/
      HAL_Delay(50);
      duty_num = duty_num + 10;
      if (duty_num > 1000) {
          duty_num = 0;
      }
              __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, duty_num);
              __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, duty_num);
              __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_4, duty_num);
/************************LED测试***************************/
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
/*****按键事件测试**********/
//      button_ticks();
//      HAL_Delay(5);
/*****按键事件测试**********/

/***************** 屏幕测试 *******************/
      u8g2_FirstPage(&u8g2);
      do {
          u8g2DrawTest(&u8g2);
          draw(&u8g2);
      } while (u8g2_NextPage(&u8g2));
/***************** 屏幕测试 *******************/

/************************ 读取I2C地址测试 **********************************/
//      uint8_t rec;
//      uint8_t data[10];
//
//      for(uint8_t address = 0;address < 255;address ++)
//      {
//          rec = HAL_I2C_Mem_Read(&hi2c1, address,0,I2C_MEMADD_SIZE_8BIT,data,7,255);
//          if(rec==HAL_OK){
//              printf("OLED显示屏地址：0x%x\r\n",address);
//          }
//          HAL_Delay(10);
//      }
/************************ 读取I2C地址测试 **********************************/

/************************ LIS2DW12测试 **********************************/
//      printf("WHO AM I：%d\r\n\n",LIS2DW12_WHO_AM_I);
//      HAL_Delay(500);
//      //LIS2DW12测试
//      uint8_t OUT_X_L;
//      uint8_t OUT_X_H;
//      uint8_t OUT_Y_L;
//      uint8_t OUT_Y_H;
//      uint8_t OUT_Z_L;
//      uint8_t OUT_Z_H;
//      HAL_I2C_Mem_Read(&hi2c1, 0x33,0x28, I2C_MEMADD_SIZE_8BIT, &OUT_X_L,1,100);
//      HAL_I2C_Mem_Read(&hi2c1, 0x33,0x29, I2C_MEMADD_SIZE_8BIT, &OUT_X_H,2,100);
//      HAL_I2C_Mem_Read(&hi2c1, 0x33,0x2A, I2C_MEMADD_SIZE_8BIT, &OUT_Y_L,3,100);
//      HAL_I2C_Mem_Read(&hi2c1, 0x33,0x2B, I2C_MEMADD_SIZE_8BIT, &OUT_Y_H,4,100);
//      HAL_I2C_Mem_Read(&hi2c1, 0x33,0x2C, I2C_MEMADD_SIZE_8BIT, &OUT_Z_L,5,100);
//      HAL_I2C_Mem_Read(&hi2c1, 0x33,0x2D, I2C_MEMADD_SIZE_8BIT, &OUT_Z_H,6,100);
//      printf("OUT_X_L：%d\r\n",OUT_X_L);
//      printf("OUT_X_H：%d\r\n",OUT_X_H);
//      printf("OUT_Y_L：%d\r\n",OUT_Y_L);
//      printf("OUT_Y_H：%d\r\n",OUT_Y_H);
//      printf("OUT_Z_L：%d\r\n",OUT_Z_L);
//      printf("OUT_Z_H：%d\r\n",OUT_Z_H);
//      HAL_Delay(300);
/************************ LIS2DW12测试 **********************************/
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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

#ifdef  USE_FULL_ASSERT
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
