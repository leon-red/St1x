#include "St1xStatic.h"
#include "lis2dw12_reg.h"
#include "main.h"
#include "spi.h"
#include "St1xADC.h"
#include "St1xKey.h"
#include "St1xPID.h"
#include "St1xFlash.h"
#include "u8g2.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// 传感器相关变量
static stmdev_ctx_t dev_ctx;
static int16_t data_raw_acceleration[3];
static float acceleration_mg[3];
static uint8_t whoamI;
static uint8_t sensor_initialized = 0;
static float last_acceleration_mg[3] = {0.0f, 0.0f, 0.0f};  // 上次加速度值
static float acceleration_change[3] = {0.0f, 0.0f, 0.0f};   // 加速度变化量

// 静置时间控制参数（可配置，单位：分钟）
#define DEFAULT_STANDBY_TIME_REDUCE_TEMP 1    // 15分钟后进入休眠状态
#define DEFAULT_STANDBY_TIME_TURN_OFF 2       // 30分钟停止加热
#define DEFAULT_REDUCED_TEMPERATURE 160.0f     // 休眠状态下的温度维持

// 时间单位转换宏
#define MINUTES_TO_MILLISECONDS(minutes) ((minutes) * 60 * 1000)

// 静置时间控制变量
static uint32_t last_movement_time = 0;               // 上次检测到运动的时间
static uint32_t standby_start_time = 0;               // 开始静置的时间
static uint8_t is_in_standby_mode = 0;                // 是否处于待机模式（降低温度）
static uint8_t is_in_sleep_mode = 0;                  // 是否处于睡眠模式（停止加热）
static float original_target_temperature = 0.0f;      // 原始目标温度
static uint8_t standby_mode_initialized = 0;          // 静置模式是否已初始化
static uint32_t last_debug_print_time = 0;            // 手动停止标记
static uint8_t manually_stopped = 0;                  // 是否手动停止加热

// OLED屏幕亮度控制相关变量
static uint8_t oled_brightness = 255; // 默认最大亮度
static uint8_t oled_standby_brightness = 76; // 30%亮度 (255 * 0.3 = 76.5)

// 全局u8g2对象指针，用于定时器回调
static u8g2_t* global_u8g2 = NULL;

// 传感器误差阈值，用于过滤小幅度的数值跳变
#define MOVEMENT_THRESHOLD 50.0f                      // 运动检测阈值 (mg)
#define STANDBY_TIME_THRESHOLD 5000                   // 静置时间阈值 (ms)

// 静置时间控制参数（可配置，单位：毫秒）
static uint32_t standby_time_reduce_temp = MINUTES_TO_MILLISECONDS(DEFAULT_STANDBY_TIME_REDUCE_TEMP);
static uint32_t standby_time_turn_off = MINUTES_TO_MILLISECONDS(DEFAULT_STANDBY_TIME_TURN_OFF);
static float reduced_temperature = DEFAULT_REDUCED_TEMPERATURE;

// SPI读写函数声明
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static void platform_delay(uint32_t ms);

/**
 * @brief 设置静置时间控制参数（单位：分钟）
 * @param time_to_reduce_temp 静置多长时间后开始降低温度（分钟）
 * @param time_to_turn_off    静置多长时间后停止加热（分钟）
 * @param reduced_temp        降低后的温度值
 */
void St1xStatic_SetStandbyParameters(uint32_t time_to_reduce_temp, uint32_t time_to_turn_off, float reduced_temp) {
    standby_time_reduce_temp = MINUTES_TO_MILLISECONDS(time_to_reduce_temp);
    standby_time_turn_off = MINUTES_TO_MILLISECONDS(time_to_turn_off);
    reduced_temperature = reduced_temp;
    
    // 确保参数在合理范围内
    if (reduced_temperature < 50.0f) reduced_temperature = 50.0f;
    if (reduced_temperature > max_temperature_limit) reduced_temperature = max_temperature_limit;
    
    if (standby_time_reduce_temp > standby_time_turn_off) {
        standby_time_reduce_temp = standby_time_turn_off;
    }
}

/**
 * @brief 使用默认宏定义参数设置静置时间控制
 */
void St1xStatic_SetDefaultStandbyParameters(void) {
    St1xStatic_SetStandbyParameters(
        DEFAULT_STANDBY_TIME_REDUCE_TEMP,
        DEFAULT_STANDBY_TIME_TURN_OFF,
        DEFAULT_REDUCED_TEMPERATURE
    );
}

/**
 * @brief 检查是否处于静置状态并控制温度
 */
static void St1xStatic_CheckStandbyControl(void) {
    // 确保传感器已初始化
    if (!sensor_initialized) return;
    
    uint32_t current_time = HAL_GetTick();
    uint8_t reg;
    
    // 检查是否有新数据
    lis2dw12_flag_data_ready_get(&dev_ctx, &reg);
    
    // 即使没有新数据，我们也进行检测，以确保能检测到静止状态
    // 读取加速度数据
    memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
    lis2dw12_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
    
    // 转换为mg单位
    acceleration_mg[0] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[0]);
    acceleration_mg[1] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[1]);
    acceleration_mg[2] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[2]);
    
    // 计算各轴加速度变化量
    acceleration_change[0] = fabsf(acceleration_mg[0] - last_acceleration_mg[0]);
    acceleration_change[1] = fabsf(acceleration_mg[1] - last_acceleration_mg[1]);
    acceleration_change[2] = fabsf(acceleration_mg[2] - last_acceleration_mg[2]);
    
    // 保存当前加速度值供下次计算使用
    last_acceleration_mg[0] = acceleration_mg[0];
    last_acceleration_mg[1] = acceleration_mg[1];
    last_acceleration_mg[2] = acceleration_mg[2];
    
    // 计算总加速度变化量
    float total_accel_change = acceleration_change[0] + acceleration_change[1] + acceleration_change[2];

    // 只有当加速度变化超过运动检测阈值时才视为有效运动
    if (total_accel_change > MOVEMENT_THRESHOLD) {
        last_movement_time = current_time;
        standby_start_time = current_time; // 重置静置开始时间
        
        // 如果处于待机模式或睡眠模式，则退出静置模式并恢复原始温度
        if (is_in_standby_mode || is_in_sleep_mode) {
            is_in_standby_mode = 0;
            is_in_sleep_mode = 0;
            if (!heating_status && !manually_stopped) {
                // 如果加热已停止且不是手动停止的，重新启动加热
                heating_status = 1;
                startHeatingControlTimer();
                setT12Temperature(original_target_temperature);
            } else if (heating_status) {
                // 如果仍在加热，只是恢复原始温度
                setT12Temperature(original_target_temperature);
            }
        }
        // 如果不是静置模式但加热已停止且不是手动停止的，说明是设备被拿起后需要恢复加热
        // 但只有在之前确实在加热的情况下才恢复加热
        else if (!heating_status && !manually_stopped && (is_in_standby_mode || is_in_sleep_mode)) {
            heating_status = 1;
            startHeatingControlTimer();
            setT12Temperature(original_target_temperature);
            manually_stopped = 0;  // 重置手动停止标记
        }
        return;
    }
    
    // 只有在加热状态下才进行静置检测
    if (!heating_status) {
        // 如果不在加热状态，重置静置模式初始化状态，确保下次加热时重新初始化
        standby_mode_initialized = 0;
        return;
    }
    
    // 如果从未初始化过静置模式，则进行初始化
    if (!standby_mode_initialized) {
        last_movement_time = current_time;
        standby_start_time = current_time;
        standby_mode_initialized = 1;
        manually_stopped = 0;  // 重新开始加热时重置手动停止标记
        return; // 初始化后立即返回，等待下一次检查
    }
    
    // 检查距离上次运动的时间
    uint32_t time_since_last_movement = current_time - last_movement_time;
    
    // 检查是否已静置足够时间
    uint32_t standby_duration = current_time - standby_start_time;
    
    // 只有当距离上次运动时间超过阈值时，才认为可以进入静置状态
    if (time_since_last_movement >= STANDBY_TIME_THRESHOLD) {
        // 如果静置时间超过设定的关闭时间，则进入睡眠模式（停止加热）
        if (standby_duration >= standby_time_turn_off) {
            // 停止加热，进入睡眠模式
            if (heating_status) {
                __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, 0);
                stopHeatingControlTimer();
                heating_status = 0;
                is_in_sleep_mode = 1;  // 进入睡眠模式
                is_in_standby_mode = 0; // 退出待机模式
                manually_stopped = 0;  // 自动停止，标记为非手动停止
            }
        }
        // 如果静置时间超过设定的降低温度时间，则进入待机模式（降低温度）
        else if (standby_duration >= standby_time_reduce_temp) {
            if (!is_in_standby_mode && !is_in_sleep_mode) {
                // 刚进入待机模式，保存原始温度
                original_target_temperature = target_temperature;
                is_in_standby_mode = 1;
                is_in_sleep_mode = 0; // 确保不在睡眠模式
            }
            
            // 设置降低后的温度
            if (heating_status && target_temperature != reduced_temperature) {
                setT12Temperature(reduced_temperature);
            }
        }
    }
}

/**
 * @brief 初始化静态传感器显示模块
 */
void St1xStatic_Init(void) {
    if (sensor_initialized) return;
    
    // 初始化传感器驱动接口
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.mdelay = platform_delay;
    dev_ctx.handle = &hspi2;  // 使用SPI2接口
    
    // 等待传感器启动
    platform_delay(20);
    
    // 检查设备ID
    lis2dw12_device_id_get(&dev_ctx, &whoamI);
    
    if (whoamI == LIS2DW12_ID) {
        uint8_t rst;
        // 恢复默认配置
        lis2dw12_reset_set(&dev_ctx, PROPERTY_ENABLE);
        
        do {
            lis2dw12_reset_get(&dev_ctx, &rst);
        } while (rst);
        
        // 启用块数据更新
        lis2dw12_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
        
        // 设置满量程
        lis2dw12_full_scale_set(&dev_ctx, LIS2DW12_2g);
        
        // 配置滤波链
        lis2dw12_filter_path_set(&dev_ctx, LIS2DW12_LPF_ON_OUT);
        lis2dw12_filter_bandwidth_set(&dev_ctx, LIS2DW12_ODR_DIV_4);
        
        // 配置功耗模式
        lis2dw12_power_mode_set(&dev_ctx, LIS2DW12_HIGH_PERFORMANCE);
        
        // 设置输出数据速率
        lis2dw12_data_rate_set(&dev_ctx, LIS2DW12_XL_ODR_25Hz);
        
        sensor_initialized = 1;
        standby_mode_initialized = 0; // 重新初始化静置模式
    }
}

/**
 * @brief 菜单项动作函数
 */
void St1xStatic_Action(void) {
    // 初始化传感器
    St1xStatic_Init();
}

/**
 * @brief 传感器数据显示函数
 */
void St1xStatic_DisplayData(u8g2_t* u8g2) {
    // 检查静置控制
    St1xStatic_CheckStandbyControl();
    
    // 调用屏幕和LED控制函数
    St1xStatic_StandbyDisplayControl(u8g2);
    
    // 如果传入NULL，只执行检查逻辑，不进行显示
    if (u8g2 == NULL) {
        return;
    }
    
    uint8_t reg;
    
    // 读取数据前先检查传感器是否已初始化
    if (!sensor_initialized) {
        u8g2_ClearBuffer(u8g2);
        u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
        u8g2_DrawStr(u8g2, 0, 20, "Sensor Not Found!");
        u8g2_SendBuffer(u8g2);
        return;
    }
    
    // 检查是否有新数据
    lis2dw12_flag_data_ready_get(&dev_ctx, &reg);
    
    if (reg) {
        // 读取加速度数据
        memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
        lis2dw12_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
        
        // 转换为mg单位
        acceleration_mg[0] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[0]);
        acceleration_mg[1] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[1]);
        acceleration_mg[2] = lis2dw12_from_fs2_to_mg(data_raw_acceleration[2]);
    }
    
    // 显示数据
    u8g2_ClearBuffer(u8g2);
    
    // 显示标题
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(u8g2, 0, 10, "6-Axis Sensor Data");
    
    // 显示X轴数据
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    char x_str[20];
    sprintf(x_str, "X: %6.2f mg", acceleration_mg[0]);
    u8g2_DrawStr(u8g2, 0, 25, x_str);
    
    // 显示Y轴数据
    char y_str[20];
    sprintf(y_str, "Y: %6.2f mg", acceleration_mg[1]);
    u8g2_DrawStr(u8g2, 0, 37, y_str);
    
    // 显示Z轴数据
    char z_str[20];
    sprintf(z_str, "Z: %6.2f mg", acceleration_mg[2]);
    u8g2_DrawStr(u8g2, 0, 49, z_str);
    
    // 显示设备ID
    char id_str[20];
    sprintf(id_str, "ID: 0x%02X", whoamI);
    u8g2_DrawStr(u8g2, 0, 61, id_str);
    
    u8g2_SendBuffer(u8g2);
}

/*
 * @brief 写入设备寄存器 (平台相关)
 * @param handle 设备句柄
 * @param reg 寄存器地址
 * @param bufp 要写入的数据缓冲区
 * @param len 数据长度
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle, &reg, 1, 1000);
    HAL_SPI_Transmit(handle, (uint8_t *) bufp, len, 1000);
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_SET);
    return 0;
}

/*
 * @brief 读取设备寄存器 (平台相关)
 * @param handle 设备句柄
 * @param reg 寄存器地址
 * @param bufp 读取数据缓冲区
 * @param len 数据长度
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    reg |= 0x80;
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle, &reg, 1, 1000);
    HAL_SPI_Receive(handle, bufp, len, 1000);
    HAL_GPIO_WritePin(G_CS_GPIO_Port, G_CS_Pin, GPIO_PIN_SET);
    return 0;
}

/*
 * @brief 平台延迟函数
 * @param ms 延迟毫秒数
 */
static void platform_delay(uint32_t ms) {
    HAL_Delay(ms);
}

/**
 * @brief 显示调试信息在OLED屏幕上
 * @param u8g2 u8g2显示对象指针
 */
void St1xStatic_DisplayDebugInfo(u8g2_t* u8g2) {
    // 显示调试信息
    u8g2_SetFont(u8g2, u8g2_font_spleen6x12_mf);
    
    // 只有在加热状态下才显示时间信息
    uint32_t time_since_last_movement = 0;
    
    if (heating_status) {
        time_since_last_movement = HAL_GetTick() - last_movement_time;
    }
    
    float total_accel = St1xStatic_GetTotalAcceleration();
    uint8_t in_standby = St1xStatic_IsInStandbyMode();
    
    // 格式化并显示信息
    char debug_str1[30];
    char debug_str2[30];
    
    sprintf(debug_str1, "A:%.0f SB:%d HT:%d", 
            total_accel, in_standby, heating_status);
    sprintf(debug_str2, "L:%lu", 
            time_since_last_movement/1000);
    
    // 在指定位置显示(X15, Y26)
    u8g2_DrawStr(u8g2, 15, 26, debug_str1);
    u8g2_DrawStr(u8g2, 15, 38, debug_str2);  // 第二行在Y=38位置
}

/**
 * @brief 获取当前静置持续时间（毫秒）
 */
uint32_t St1xStatic_GetStandbyDuration(void) {
    if (!standby_mode_initialized || !heating_status) {
        return 0;
    }
    return HAL_GetTick() - standby_start_time;
}

/**
 * @brief 获取当前总加速度值（mg）
 */
float St1xStatic_GetTotalAcceleration(void) {
    if (!sensor_initialized) {
        return 0.0f;
    }
    
    // 返回加速度变化量之和
    return acceleration_change[0] + acceleration_change[1] + acceleration_change[2];
}

/**
 * @brief 检查是否处于待机模式（降低温度）
 */
uint8_t St1xStatic_IsInStandbyMode(void) {
    // 只有在加热状态下才考虑待机模式
    if (!heating_status) {
        return 0;
    }
    return is_in_standby_mode;
}

/**
 * @brief 检查是否处于睡眠模式（停止加热）
 */
uint8_t St1xStatic_IsInSleepMode(void) {
    return is_in_sleep_mode;
}

/**
 * @brief 设置手动停止标记
 * @param stopped 是否手动停止
 */
void St1xStatic_SetManuallyStopped(uint8_t stopped) {
    manually_stopped = stopped;
}

/**
 * @brief 检查是否手动停止加热
 * @return 1表示手动停止，0表示非手动停止
 */
uint8_t St1xStatic_IsManuallyStopped(void) {
    return manually_stopped;
}

/**
 * @brief OLED屏幕亮度控制函数
 * @param u8g2 u8g2显示对象指针
 * @param brightness 亮度值 (0-255)
 */
static void OLED_SetBrightness(u8g2_t* u8g2, uint8_t brightness) {
    // 使用u8x8_SetContrast函数设置OLED对比度（亮度）
    // 注意：u8g2库中通常使用u8x8_SetContrast来控制屏幕亮度
    if (u8g2 != NULL) {
        u8x8_SetContrast(u8g2_GetU8x8(u8g2), brightness);
    }
}



/**
 * @brief 静置状态下的屏幕亮度控制
 * 统一调度机制：静置触发待机（降低温度）时降低亮度，静置触发睡眠（停止发热）时关闭屏幕
 * @param u8g2 u8g2显示对象指针
 */
void St1xStatic_StandbyDisplayControl(u8g2_t* u8g2) {
    // 检查当前静置状态
    uint32_t standby_duration = St1xStatic_GetStandbyDuration();
    uint8_t in_standby_mode = St1xStatic_IsInStandbyMode();
    
    // 统一调度逻辑：
    // 1. 静置触发睡眠状态（停止发热）- 关闭屏幕
    if (!heating_status && !manually_stopped) {
        // 静置触发睡眠：停止加热且非手动停止，关闭屏幕
        if (u8g2 != NULL) {
            OLED_SetBrightness(u8g2, 0);
        }
    }
    // 2. 静置触发待机状态（降低温度）- 降低亮度
    else if (in_standby_mode && heating_status) {
        // 静置触发待机：降低温度但仍保持加热，设置屏幕亮度为30%
        if (u8g2 != NULL) {
            OLED_SetBrightness(u8g2, oled_standby_brightness);
        }
    }
    // 3. 正常工作状态 - 恢复正常亮度
    else {
        // 正常工作状态：恢复OLED屏幕正常亮度
        if (u8g2 != NULL) {
            OLED_SetBrightness(u8g2, oled_brightness);
        }
    }
}

/**
 * @brief 设置全局u8g2对象指针
 * 用于定时器回调函数持续控制屏幕亮度
 * @param u8g2 u8g2显示对象指针
 */
void St1xStatic_SetGlobalU8g2(u8g2_t* u8g2) {
    global_u8g2 = u8g2;
}

/**
 * @brief 定时器回调函数
 * 每100ms调用一次，持续控制屏幕亮度
 */
void St1xStatic_TimerCallback(void) {
    static uint32_t last_call_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 每100ms执行一次控制
    if (current_time - last_call_time < 100) {
        return;
    }
    last_call_time = current_time;
    
    // 调用屏幕亮度控制函数
    St1xStatic_StandbyDisplayControl(global_u8g2);
}