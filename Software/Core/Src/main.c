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
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include "u8g2.h"
#include "u8g2_oled.h"
#include "St1xADC.h"
#include "bmp.h"
#include "lis2dw12.h"
#include "St1xPID.h"
#include "ws2812.h"
#include "St1xMenu.h"
#include "St1xKey.h"
#include "St1xStatic.h"
#include "St1xCalibrationSystem.h"
#include "Buzzer.h"
#include "St1xSystemManager.h"

// 声明外部变量
extern uint8_t heating_status;

// 校准系统所需的外部函数声明
extern float getCalibrationTemperature(void);
extern uint8_t isUSBVoltageSufficient(void);
extern void StopCalibrationHeating(void);
extern void StartCalibrationHeating(void);
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 显示更新间隔(ms)
#define DISPLAY_UPDATE_INTERVAL 100
// 静置检查间隔(ms)
#define STANDBY_CHECK_INTERVAL 1000
// 菜单进入延迟(ms)
#define MENU_ENTER_DELAY 200
// 温度显示变化阈值(°C)
#define TEMP_DISPLAY_THRESHOLD 0.5f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 定义u8g2全局变量
u8g2_t u8g2;

// 菜单活动状态变量
uint8_t menu_active = 0;

// OLED更新计时
uint32_t last_oled_update = 0;

// 调试显示标志
uint8_t debug_display_enabled = 0;  // 默认关闭调试显示 0关闭 1开启

// 使用系统管理模块中定义的系统模式
extern SystemMode current_system_mode;

// 系统状态标志
uint8_t timer_started = 0;  // 定时器启动标志
uint32_t last_standby_check = 0;  // 上次静置检查时间
float last_displayed_temp = 0;  // 上次显示的温度值
uint32_t menu_enter_delay = 0;  // 菜单进入延迟计时
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

// 系统初始化函数（保留硬件相关初始化）
void System_Init(void);
void LED_Init(void);
void Display_Init(void);
void ADC_Init(void);
void Timer_Init(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//===========================
// 中断回调函数
//===========================

/**
 * @brief TIM2定时器中断回调函数
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        heatingControlTimerCallback();  // 调用加热控制逻辑
    }
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
    // 系统初始化
    System_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t current_time = HAL_GetTick();
    
    // 系统状态监控（始终运行）
    systemStatusMonitor();
    
    // 定期检查静置状态
    if ((current_time - last_standby_check) >= STANDBY_CHECK_INTERVAL) {
        St1xStatic_DisplayData(NULL); // 检查静置状态
        last_standby_check = current_time;
    }
    
    // 静置状态下的屏幕和LED控制
    St1xStatic_TimerCallback();
    
    // 根据当前系统模式处理不同的逻辑
    SystemManager_ModeHandler(current_time);
    
    // 各模块有独立的定时刷新机制，无需主循环延时
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

//===========================
// 系统初始化函数
//===========================

/**
 * @brief 系统初始化 - 负责硬件外设的初始化
 */
void System_Init(void) {
    LED_Init();
    Display_Init();
    ADC_Init();
    Timer_Init();
    SystemManager_AppModulesInit();  // 应用模块初始化已移动到系统管理模块
}

/**
 * @brief LED初始化
 */
void LED_Init(void) {
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, 0);  // 红色LED
    __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_4, 0);  // 绿色LED
    __HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 0);  // 蓝色LED
}

/**
 * @brief 显示初始化
 */
void Display_Init(void) {
    spi_oled_Init(&u8g2);           // 初始化OLED
    u8g2_DrawXBMP(&u8g2,0,0,128,80,Logo);  // 显示开机LOGO
    u8g2_SendBuffer(&u8g2);
    buzzerStartupBeep();            // 播放开机启动音
    HAL_Delay(1000);
    u8g2_ClearBuffer(&u8g2);
}

/**
 * @brief ADC初始化
 */
void ADC_Init(void) {
    HAL_ADCEx_Calibration_Start(&hadc1);  // ADC自动校准
}

/**
 * @brief 定时器初始化
 */
void Timer_Init(void) {
    // 暂时不启动TIM2中断，避免过早启动PID控制
    // 首次显示更新完成后，在System_NormalModeHandler中再启动定时器
}













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
    while (1) {
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