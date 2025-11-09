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

// 声明外部变量
extern uint8_t heating_status;
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
// 定义u8g2全局变量
u8g2_t u8g2;

// 菜单活动状态变量
uint8_t menu_active = 0;

// OLED更新计时
static uint32_t last_oled_update = 0;

// 调试显示标志
static uint8_t debug_display_enabled = 0;  // 默认关闭调试显示 0关闭 1开启

// 系统运行模式
typedef enum {
    SYSTEM_MODE_NORMAL,     // 正常运行模式
    SYSTEM_MODE_MENU,       // 菜单模式
    SYSTEM_MODE_CALIBRATION // 校准模式
} SystemMode;

static SystemMode current_system_mode = SYSTEM_MODE_NORMAL;  // 当前系统模式
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

// 系统模式管理函数
void enterMenuMode(void);
void enterCalibrationMode(void);
void exitCalibrationMode(void);

// 系统初始化函数
void System_Init(void);
void LED_Init(void);
void Display_Init(void);
void ADC_Init(void);
void Timer_Init(void);
void AppModules_Init(void);

// 系统状态处理函数
void System_ModeHandler(uint32_t current_time);
void System_NormalModeHandler(uint32_t current_time);
void System_MenuModeHandler(void);
void System_CalibrationModeHandler(uint32_t current_time);

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
    // 系统状态监控（始终运行）
    systemStatusMonitor();
    
    // 定期检查静置状态（始终检查，即使在不加热状态下也要检查运动以恢复加热）
    static uint32_t last_standby_check = 0;
    uint32_t current_time = HAL_GetTick();
    if ((current_time - last_standby_check) >= 1000) { // 每秒检查一次
        // 调用静置控制检查函数（通过公共接口）
        St1xStatic_DisplayData(NULL); // 传入NULL因为我们只关心检查逻辑，不关心显示
        last_standby_check = current_time;
    }
    
    // 静置状态下的屏幕和LED控制（优先级高于普通LED效果）
    St1xStatic_TimerCallback();
    
    // LED控制由统一的LED状态机处理，避免状态冲突
    // HeatingStatusLEDEffect() 已被整合到 UnifiedLEDStateMachine() 中
    
    // 根据当前系统模式处理不同的逻辑
    System_ModeHandler(current_time);
    
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
 * @brief 系统初始化 - 负责所有外设和模块的初始化
 */
void System_Init(void) {
    // 初始化LED
    LED_Init();
    
    // 初始化OLED显示
    Display_Init();
    
    // 初始化ADC
    ADC_Init();
    
    // 初始化定时器
    Timer_Init();
    
    // 初始化应用模块
    AppModules_Init();
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
    // 注意：暂时不启动TIM2中断，避免过早启动PID控制
    // 首次显示更新完成后，在System_NormalModeHandler中再启动定时器
}

/**
 * @brief 应用模块初始化
 */
void AppModules_Init(void) {
    Key_Init();                     // 初始化按键处理
    St1xStatic_Init();             // 初始化静态传感器显示
    CalibrationSystem_Init();      // 初始化独立校准系统
    St1xStatic_SetDefaultStandbyParameters();  // 设置默认参数
    
    // 初始化冷端补偿温度（使用刚上电时的环境温度）
    initializeColdJunctionTemperature();
}

//===========================
// 按键处理函数
//===========================
// handleMainTemperatureAdjust和handleMenuKey函数已在St1xKey.c中定义

//===========================
// 系统模式管理函数
//===========================

void enterMenuMode(void) {
    current_system_mode = SYSTEM_MODE_MENU;
    menu_active = 1;
    Menu_InitSystem();  // 初始化菜单系统
}

void enterCalibrationMode(void) {
    current_system_mode = SYSTEM_MODE_CALIBRATION;
    CalibrationSystem_Start();
}

// 退出校准模式
void exitCalibrationMode(void) {
    CalibrationSystem_Stop();
    current_system_mode = SYSTEM_MODE_NORMAL;
}

//===========================
// 系统状态处理函数
//===========================

/**
 * @brief 系统模式处理主函数
 * @param current_time 当前系统时间(ms)
 */
void System_ModeHandler(uint32_t current_time) {
    switch (current_system_mode) {
        case SYSTEM_MODE_NORMAL:
            System_NormalModeHandler(current_time);
            break;
            
        case SYSTEM_MODE_MENU:
            System_MenuModeHandler();
            break;
            
        case SYSTEM_MODE_CALIBRATION:
            System_CalibrationModeHandler(current_time);
            break;
    }
}

/**
 * @brief 正常运行模式处理
 * @param current_time 当前系统时间(ms)
 */
void System_NormalModeHandler(uint32_t current_time) {
    KeyType key = Key_Scan();
    static uint8_t key_debug_display = 0;
    static uint32_t key_debug_time = 0;
    static uint32_t menu_enter_delay = 0;
    
    // 处理按键输入
    if (key != KEY_NONE) {
        // 处理温度调节按键
        if (key == KEY_UP || key == KEY_DOWN) {
            handleMainTemperatureAdjust(key);
        }
        
        // 处理MODE键短按 - 加热控制
        if (key == KEY_MODE) {
            handleHeatingControl();
        }
        
        // 显示按键状态（用于调试）
        key_debug_display = 1;
        key_debug_time = current_time;
        
        // 处理菜单键 - 如果检测到长按，进入菜单模式
        if (key == KEY_MODE_LONG) {
            // 延迟进入菜单，避免屏幕刷新冲突
            menu_enter_delay = current_time + 200;
        }
    }
    
    // 延迟进入菜单模式
    if (menu_enter_delay > 0 && current_time >= menu_enter_delay) {
        menu_enter_delay = 0;
        enterMenuMode();
    }
    
    // 更新OLED显示（只在特定条件下更新，避免频繁刷新）
    static uint8_t need_display_update = 0;
    
    // 检测需要更新显示的条件
    if (key != KEY_NONE) {
        // 有按键按下时，需要更新显示
        need_display_update = 1;
    }
    
    // 温度变化超过阈值时也需要更新显示
    static float last_displayed_temp = 0;
    extern float display_filtered_temperature;  // 直接使用显示滤波温度变量
    float current_temp = display_filtered_temperature;
    if (fabs(current_temp - last_displayed_temp) > 0.5f) {
        need_display_update = 1;
        last_displayed_temp = current_temp;
    }
    
    // 定期更新显示（每100ms一次，减少刷新频率）
    if ((current_time - last_oled_update) >= 100) {
        need_display_update = 1;
    }
    
    // 如果需要更新显示，则执行显示操作
    if (need_display_update) {
        drawMainDisplay(&u8g2);
        
        // 如果启用了调试显示，则显示调试信息
        if (debug_display_enabled) {
            St1xStatic_DisplayDebugInfo(&u8g2);
        }
        
        u8g2_SendBuffer(&u8g2);
        last_oled_update = current_time;
        need_display_update = 0;  // 重置更新标志
        
        // 首次显示更新完成后，启动TIM2中断（PID控制）
        static uint8_t timer_started = 0;
        if (!timer_started) {
            HAL_TIM_Base_Start_IT(&htim2);  // 启动TIM2中断
            timer_started = 1;
        }
    }
}

/**
 * @brief 菜单模式处理
 */
void System_MenuModeHandler(void) {
    if (!Menu_Process()) {
        // 菜单已退出，返回正常模式
        current_system_mode = SYSTEM_MODE_NORMAL;
        menu_active = 0;
    }
}

/**
 * @brief 校准模式处理
 * @param current_time 当前系统时间(ms)
 */
void System_CalibrationModeHandler(uint32_t current_time) {
    KeyType key = Key_Scan();
    
    // 处理校准模式下的按键
    if (key != KEY_NONE) {
        // 将按键传递给校准系统处理
        CalibrationSystem_HandleKey(key);
    }
    
    CalibrationSystem_Update(&u8g2);
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