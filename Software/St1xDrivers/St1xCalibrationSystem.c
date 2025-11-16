// 校准系统实现文件
// 包含必要的头文件
#include "St1xCalibrationSystem.h"
#include "u8g2.h"
#include "St1xKey.h"
#include "St1xFlash.h"
#include "tim.h"
#include <stdio.h>
#include <math.h>

// ============================================================================
// 系统配置常量
// ============================================================================

#define CALIBRATION_POINTS 9           // 校准点数量
#define TEMPERATURE_TOLERANCE 5.0f     // 温度容差(°C)
#define TEMPERATURE_STABLE_TIME 1000   // 温度稳定时间(ms)
#define UPDATE_INTERVAL 20             // 更新间隔(ms)
#define TEMPERATURE_ADJUST_STEP 1.0f   // 温度调整步长(°C)
#define LOW_VOLTAGE_DISPLAY_TIME 5000   // 低电压显示时间(ms)

// ============================================================================
// 校准温度点定义
// ============================================================================

static const float calibration_temperatures[CALIBRATION_POINTS] = {
    80.0f, 130.0f, 180.0f, 230.0f, 280.0f,
    330.0f, 380.0f, 430.0f, 480.0f
};

// ============================================================================
// 数据结构定义
// ============================================================================

// 校准点数据结构定义
typedef struct {
    uint8_t point_id;              // 校准点ID
    float target_temperature;      // 目标温度
    float adjusted_temperature;    // 调整后温度
    float calibration_offset;      // 校准偏移量
    CalibrationPointState state;   // 校准点状态
    uint32_t heating_start_time;  // 加热开始时间
    uint32_t stable_start_time;    // 稳定开始时间
    float max_temp_deviation;      // 最大温度偏差
    uint8_t heating_cycles;        // 加热循环次数
} CalibrationPoint;

// ============================================================================
// 全局变量
// ============================================================================

static CalibrationSystemState system_state = CAL_STATE_IDLE;  // 系统状态
static CalibrationPoint calibration_points[CALIBRATION_POINTS];  // 校准点数组
static uint8_t current_point_index = 0;      // 当前校准点索引
static uint32_t last_update_time = 0;        // 最后更新时间
static uint8_t save_result = 0;              // 保存结果
static uint8_t low_voltage_state = 0;        // 低电压状态
static uint32_t low_voltage_start_time = 0;   // 低电压开始时间
static float calibration_temperature_limit = 500.0f;  // 温度限制
static uint8_t original_heating_state = 0;   // 保存进入校准前的加热状态

// ============================================================================
// 外部变量声明
// ============================================================================

extern void setT12Temperature(float temperature);
extern void startHeatingControlTimer(void);
extern void stopHeatingControlTimer(void);
extern float target_temperature;
extern uint8_t heating_status;
extern uint8_t heating_control_enabled;
extern TIM_HandleTypeDef htim2;

// ============================================================================
// 静态函数声明
// ============================================================================

static void InitializeCalibrationPoint(uint8_t point_index);
static void SetPointTemperature(uint8_t point_index, float temperature);
static void SaveCalibrationData(void);
static void ProcessVoltageCheckState(uint32_t current_time);
static void ProcessCalibrationPoint(uint8_t point_index);
static void DrawCalibrationInterface(u8g2_t *u8g2, uint8_t point_index);
static const char* GetPointStateText(CalibrationPointState state);
static const char* GetPointHelpText(CalibrationPointState state);
static KeyType CalibrationKey_Scan(void);
static void HandleCalibrationPointCompleted(uint8_t point_index);
static void HandleVoltageCheckKey(KeyType key);
static void HandleRunningStateKey(KeyType key);
static void HandleCompleteStateKey(KeyType key);

// ============================================================================
// 系统初始化函数
// ============================================================================

/**
 * @brief 初始化校准系统
 * 重置所有系统状态和变量到初始值
 */
void CalibrationSystem_Init(void) {
    system_state = CAL_STATE_IDLE;
    current_point_index = 0;
    save_result = 0;
    last_update_time = 0;
    
    low_voltage_state = 0;
    low_voltage_start_time = 0;
    
    // 初始化所有校准点
    for (int i = 0; i < CALIBRATION_POINTS; i++) {
        InitializeCalibrationPoint(i);
    }
    
    calibration_temperature_limit = 500.0f;
}

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 初始化单个校准点
 * @param point_index 校准点索引
 */
static void InitializeCalibrationPoint(uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    
    point->point_id = point_index + 1;
    point->target_temperature = calibration_temperatures[point_index];
    point->adjusted_temperature = calibration_temperatures[point_index];
    point->calibration_offset = 0.0f;
    point->state = CAL_POINT_WAITING;
    point->heating_start_time = 0;
    point->stable_start_time = 0;
    point->max_temp_deviation = 0.0f;
    point->heating_cycles = 0;
}

/**
 * @brief 获取校准点状态文本
 * @param state 校准点状态
 * @return 状态描述字符串
 */
static const char* GetPointStateText(CalibrationPointState state) {
    switch (state) {
        case CAL_POINT_WAITING:  return "Waiting";
        case CAL_POINT_HEATING:  return "Heating...";
        case CAL_POINT_STABLE:   return "Stable";
        case CAL_POINT_ADJUSTED: return "Confirmed";
        default: return "Unknown";
    }
}

/**
 * @brief 获取校准点帮助文本
 * @param state 校准点状态
 * @return 帮助信息字符串
 */
static const char* GetPointHelpText(CalibrationPointState state) {
    switch (state) {
        case CAL_POINT_HEATING:  return "Heating...Wait";
        case CAL_POINT_STABLE:   return "UP/DN:Adj MODE:OK";
        case CAL_POINT_ADJUSTED: return "Completed";
        default: return "";
    }
}

// ============================================================================
// 系统控制函数
// ============================================================================

/**
 * @brief 启动校准系统
 * 初始化校准系统并进入电压检测状态，设置第一个校准点的温度参数
 */
void CalibrationSystem_Start(void) {
    if (system_state != CAL_STATE_IDLE) {
        return;
    }
    
    // 保存进入校准前的加热状态
    original_heating_state = heating_status;
    
    // 设置第一个校准点的温度到PID目标值
    if (CALIBRATION_POINTS > 0) {
        CalibrationPoint* first_point = &calibration_points[0];
        first_point->target_temperature = calibration_temperatures[0];
        first_point->adjusted_temperature = calibration_temperatures[0];
        
        // 设置PID目标温度（必须在启动定时器之前设置）
        SetPointTemperature(0, first_point->adjusted_temperature);
        
        // 显式设置目标温度变量
        extern float target_temperature;
        target_temperature = first_point->adjusted_temperature;
    }
    
    // 设置加热状态为启用（校准系统强制启用加热）
    heating_status = 1;
    
    // 启动PWM输出（关键修复：确保加热器实际通电）
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    
    // 启动PID控制器的定时器（必须在设置目标温度和加热状态之后）
    startHeatingControlTimer();
    
    // 进入电压检测状态
    system_state = CAL_STATE_VOLTAGE_CHECK;
    low_voltage_start_time = HAL_GetTick();
}

/**
 * @brief 停止校准系统
 * 停止所有加热控制，重置系统状态到空闲状态，恢复默认目标温度和原始加热状态
 */
void CalibrationSystem_Stop(void) {
    if (system_state == CAL_STATE_IDLE) {
        return;
    }
    
    // 停止PID控制器的定时器
    stopHeatingControlTimer();
    
    // 停止PWM输出（关键修复：确保加热器实际断电）
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
    
    // 恢复进入校准前的加热状态
    heating_status = original_heating_state;
    
    // 恢复正常的温度限制
    extern float max_temperature_limit;
    max_temperature_limit = 460.0f; // 正常温度限制
    
    // 恢复默认目标温度（360°C），避免校准温度影响主界面显示
    extern float target_temperature;
    target_temperature = 360.0f; // 默认目标温度
    
    // 重置PID控制器的设定点，确保PID系统也使用默认温度
    setT12Temperature(360.0f);
    
    // 重置系统状态
    system_state = CAL_STATE_IDLE;
    current_point_index = 0;
    save_result = 0;
    
    // 重置电压不足状态
    low_voltage_state = 0;
}

// ============================================================================
// 主更新函数
// ============================================================================

/**
 * @brief 校准系统更新（在主循环中调用）
 * @param u8g2 图形显示对象指针
 * 处理按键输入、状态逻辑和界面绘制，控制更新频率为50ms
 */
void CalibrationSystem_Update(u8g2_t *u8g2) {
    uint32_t current_time = HAL_GetTick();
    
    // 控制更新频率（50ms间隔）
    if ((current_time - last_update_time) < UPDATE_INTERVAL) {
        return;
    }
    last_update_time = current_time;
    
    // 处理按键输入（使用与主系统相同的状态机机制）
    KeyType key = CalibrationKey_Scan();
    
    // 如果有按键触发，则处理按键
    if (key != KEY_NONE) {
        CalibrationSystem_HandleKey(key);
    }
    
    // 根据系统状态处理不同逻辑
    switch (system_state) {
        case CAL_STATE_VOLTAGE_CHECK:
            ProcessVoltageCheckState(current_time);
            break;
            
        case CAL_STATE_RUNNING:
            // 处理当前校准点
            ProcessCalibrationPoint(current_point_index);
            break;
            
        default:
            // IDLE和COMPLETE状态不需要特殊处理
            break;
    }
    
    // 绘制界面
    DrawCalibrationInterface(u8g2, current_point_index);
}

// ============================================================================
// 状态处理函数
// ============================================================================

/**
 * @brief 处理电压检测状态
 * @param current_time 当前系统时间
 * 检查USB电压是否足够，如果足够则进入校准运行状态
 */
static void ProcessVoltageCheckState(uint32_t current_time) {
    // 检查电压是否足够
    extern uint8_t isUSBVoltageSufficient(void);
    if (isUSBVoltageSufficient()) {
        // 电压足够，进入校准运行状态
        
        // 在校准模式下临时提高温度限制到500度
        extern float max_temperature_limit;
        max_temperature_limit = 500.0f; // 校准温度限制
        
        // 重置状态
        current_point_index = 0;
        save_result = 0;
        system_state = CAL_STATE_RUNNING;
        
        // 重置所有校准点
        for (int i = 0; i < CALIBRATION_POINTS; i++) {
            InitializeCalibrationPoint(i);
        }
        
        // 启动第一个校准点
        SetPointTemperature(0, calibration_points[0].adjusted_temperature);
        calibration_points[0].state = CAL_POINT_HEATING;
        calibration_points[0].heating_start_time = current_time;
        calibration_points[0].heating_cycles = 1;
        
        // 立即将校准点温度写入目标温度变量，取代主界面的默认温度值
        extern float target_temperature;
        target_temperature = calibration_points[0].adjusted_temperature;
        
        // 注意：定时器已经在CalibrationSystem_Start()中启动，这里不需要重复启动
        // PID控制器现在应该已经开始工作了
    } else {
        // 电压不足，检查是否超过5秒显示时间
        if ((current_time - low_voltage_start_time) >= LOW_VOLTAGE_DISPLAY_TIME) {
            // 超过5秒，返回主界面
            CalibrationSystem_Stop();
        }
    }
}

/**
 * @brief 处理校准点完成后的逻辑
 * @param point_index 完成的校准点索引
 * 根据当前校准点索引决定是完成整个校准过程还是切换到下一个校准点
 */
static void HandleCalibrationPointCompleted(uint8_t point_index) {
    // 如果是最后一个校准点，完成整个校准过程
    if (point_index == (CALIBRATION_POINTS - 1)) {
        system_state = CAL_STATE_COMPLETE;
        
        // 校准完成，立即停止加热以确保安全
        // 停止PID控制器的定时器
        stopHeatingControlTimer();
        
        // 停止PWM输出（关键修复：确保加热器实际断电）
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
        
        // 禁用加热状态
        extern uint8_t heating_status;
        heating_status = 0;  // 禁用加热
        
        // 保存校准数据
        SaveCalibrationData();
        
        printf("校准完成，加热已停止，数据已保存\n");
    } else {
        // 切换到下一个校准点
        current_point_index++;
        CalibrationPoint* next_point = &calibration_points[current_point_index];
        
        // 设置下一个校准点的目标温度（目标温度 + 偏移量）
        SetPointTemperature(current_point_index, next_point->target_temperature + next_point->calibration_offset);
        
        // 显式设置目标温度变量
        extern float target_temperature;
        target_temperature = next_point->target_temperature + next_point->calibration_offset;
        
        // 启动下一个校准点的加热过程
        next_point->state = CAL_POINT_HEATING;
        next_point->heating_start_time = HAL_GetTick();
        next_point->heating_cycles = 1;
    }
}

// ============================================================================
// 按键处理函数
// ============================================================================

/**
 * @brief 处理电压检测状态下的按键
 * @param key 按键类型
 * 电压检测状态下，短按确认电压正常，长按跳过电压检测
 */
static void HandleVoltageCheckKey(KeyType key) {
    if (key == KEY_MODE) {
        // 确认电压正常，进入校准运行状态
        system_state = CAL_STATE_RUNNING;
        
        // 启动第一个校准点
        current_point_index = 0;
        SetPointTemperature(current_point_index, calibration_points[current_point_index].adjusted_temperature);
        
        // 显式设置目标温度变量
        extern float target_temperature;
        target_temperature = calibration_points[current_point_index].adjusted_temperature;
        
        // 设置第一个校准点为加热状态
        calibration_points[current_point_index].state = CAL_POINT_HEATING;
    } else if (key == KEY_MODE_LONG) {
        // 跳过电压检测，直接进入校准运行状态
        system_state = CAL_STATE_RUNNING;
        
        // 启动第一个校准点
        current_point_index = 0;
        SetPointTemperature(current_point_index, calibration_points[current_point_index].adjusted_temperature);
        
        // 显式设置目标温度变量
        extern float target_temperature;
        target_temperature = calibration_points[current_point_index].adjusted_temperature;
        
        // 设置第一个校准点为加热状态
        calibration_points[current_point_index].state = CAL_POINT_HEATING;
    }
}

/**
 * @brief 处理运行状态下的按键
 * @param key 按键类型
 * 根据当前校准点状态处理不同的按键操作
 */
static void HandleRunningStateKey(KeyType key) {
    if (current_point_index >= CALIBRATION_POINTS) {
        return;
    }
    
    CalibrationPoint* point = &calibration_points[current_point_index];
    
    switch (point->state) {
        case CAL_POINT_HEATING:
            // 加热状态下，短按跳过加热，长按退出校准
            if (key == KEY_MODE) {
                // 跳过加热，直接进入稳定状态
                point->state = CAL_POINT_STABLE;
                point->stable_start_time = HAL_GetTick();
            } else if (key == KEY_MODE_LONG) {
                // 退出校准系统
                system_state = CAL_STATE_IDLE;
            }
            break;
            
        case CAL_POINT_STABLE:
            // 稳定状态下，处理温度偏差修正和确认
            if (key == KEY_UP) {
                // UP按键：增加偏移量（补偿负偏差）
                point->calibration_offset += TEMPERATURE_ADJUST_STEP;
                
                // 重新计算调整后的目标温度
                point->adjusted_temperature = point->target_temperature + point->calibration_offset;
                
                // 温度上限保护
                if (point->adjusted_temperature > calibration_temperature_limit) {
                    point->adjusted_temperature = calibration_temperature_limit;
                    point->calibration_offset = point->adjusted_temperature - point->target_temperature;
                }
                
                // 更新PID目标温度
                setT12Temperature(point->adjusted_temperature);
                
            } else if (key == KEY_DOWN) {
                // DOWN按键：减少偏移量（补偿正偏差）
                point->calibration_offset -= TEMPERATURE_ADJUST_STEP;
                
                // 重新计算调整后的目标温度
                point->adjusted_temperature = point->target_temperature + point->calibration_offset;
                
                // 温度下限保护
                if (point->adjusted_temperature < 0.0f) {
                    point->adjusted_temperature = 0.0f;
                    point->calibration_offset = point->adjusted_temperature - point->target_temperature;
                }
                
                // 更新PID目标温度
                setT12Temperature(point->adjusted_temperature);
                
            } else if (key == KEY_MODE) {
                // 短按确认调整，进入调整完成状态
                point->state = CAL_POINT_ADJUSTED;
            } else if (key == KEY_MODE_LONG) {
                // 长按退出校准系统
                system_state = CAL_STATE_IDLE;
            }
            break;
            
        case CAL_POINT_ADJUSTED:
            // 调整状态下，短按确认调整，长按退出校准
            if (key == KEY_MODE) {
                // 确认调整，进入下一个校准点
                HandleCalibrationPointCompleted(current_point_index);
            } else if (key == KEY_MODE_LONG) {
                // 退出校准系统
                system_state = CAL_STATE_IDLE;
            }
            break;
            
        default:
            break;
    }
}

// ============================================================================
// 校准点处理函数
// ============================================================================

/**
 * @brief 处理单个校准点逻辑
 * @param point_index 校准点索引
 * 监控温度稳定性并管理校准点状态转换
 */
static void ProcessCalibrationPoint(uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    if (system_state != CAL_STATE_RUNNING) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    extern float filtered_temperature;
    float current_temp = filtered_temperature;
    float target_temp = point->adjusted_temperature;
    float temp_diff = fabs(current_temp - target_temp);
    uint32_t current_time = HAL_GetTick();
    
    // 记录最大温度偏差
    if (temp_diff > point->max_temp_deviation) {
        point->max_temp_deviation = temp_diff;
    }
    
    switch (point->state) {
        case CAL_POINT_HEATING:
            // 检查温度是否稳定
            if (temp_diff <= TEMPERATURE_TOLERANCE) {
                if (point->stable_start_time == 0) {
                    point->stable_start_time = current_time;
                } else if ((current_time - point->stable_start_time) >= TEMPERATURE_STABLE_TIME) {
                    point->state = CAL_POINT_STABLE;
                }
            } else {
                // 温度不稳定，重置稳定计时器
                point->stable_start_time = 0;
            }
            break;
            
        case CAL_POINT_STABLE:
            // 等待用户调整或确认
            // 这个状态由按键处理来驱动
            break;
            
        case CAL_POINT_ADJUSTED:
            // 处理校准点完成后的逻辑
            HandleCalibrationPointCompleted(point_index);
            break;
            
        default:
            break;
    }
}

// ============================================================================
// 界面绘制函数
// ============================================================================

/**
 * @brief 绘制校准界面
 * @param u8g2 图形显示对象指针
 * @param point_index 当前校准点索引
 * 根据系统状态显示不同的界面内容
 */
static void DrawCalibrationInterface(u8g2_t *u8g2, uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    u8g2_ClearBuffer(u8g2);
    
    // 检查电压
    if (!isUSBVoltageSufficient()) {
        if (!low_voltage_state) {
            // 首次检测到电压不足，设置状态和计时
            low_voltage_state = 1;
            low_voltage_start_time = HAL_GetTick();
        }
        
        // 显示电压不足提示
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(u8g2, 0, 20, "Low Voltage!");
        u8g2_DrawStr(u8g2, 0, 32, "Cannot calibrate");
        u8g2_SendBuffer(u8g2);
        
        // 检查是否达到显示时间
        if ((HAL_GetTick() - low_voltage_start_time) >= LOW_VOLTAGE_DISPLAY_TIME) {
            // 时间到，自动停止校准并返回主系统
            CalibrationSystem_Stop();
            low_voltage_state = 0;  // 重置状态
        }
        return;
    } else {
        // 电压恢复正常，重置电压不足状态
        low_voltage_state = 0;
    }
    
    if (system_state == CAL_STATE_VOLTAGE_CHECK) {
        // 电压检测状态显示
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        
        // 检查电压是否足够
        if (isUSBVoltageSufficient()) {
            // 电压足够，显示准备进入校准
            u8g2_DrawStr(u8g2, 0, 20, "Voltage OK");
            u8g2_DrawStr(u8g2, 0, 32, "Starting Calibration...");
        } else {
            // 电压不足，显示低电压警告
            u8g2_DrawStr(u8g2, 0, 20, "Low Voltage!");
            u8g2_DrawStr(u8g2, 0, 32, "Please check power");
            
            // 显示倒计时
            uint32_t current_time = HAL_GetTick();
            uint32_t remaining_time = LOW_VOLTAGE_DISPLAY_TIME - (current_time - low_voltage_start_time);
            uint32_t seconds = remaining_time / 1000;
            
            char countdown_str[32];
            sprintf(countdown_str, "Return in %lu sec", seconds);
            u8g2_DrawStr(u8g2, 0, 44, countdown_str);
        }
    } else if (system_state == CAL_STATE_COMPLETE) {
        // 完成界面
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(u8g2, 0, 20, "Calibration Complete!");
        if (save_result) {
            u8g2_DrawStr(u8g2, 0, 32, "Data Saved OK");
        } else {
            u8g2_DrawStr(u8g2, 0, 32, "Save Failed!");
        }
        u8g2_DrawStr(u8g2, 0, 44, "Press MODE to exit");
    } else if (system_state == CAL_STATE_RUNNING) {
        // 校准点界面 - 紧凑布局
        char buffer[32];
        
        // 标题栏 - 校准点信息
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        sprintf(buffer, "P%d/%d", point->point_id, CALIBRATION_POINTS);
        u8g2_DrawStr(u8g2, 0, 10, buffer);
        
        // 状态信息 - 右侧显示
        const char* current_status = GetPointStateText(point->state);
        u8g2_DrawStr(u8g2, 70, 10, current_status);
        
        // 第一行：目标温度和实测温度
        extern float filtered_temperature;
        float current_temp = filtered_temperature;
        sprintf(buffer, "T:%.1fC", point->adjusted_temperature);
        u8g2_DrawStr(u8g2, 0, 24, buffer);
        
        sprintf(buffer, "A:%.1fC", current_temp);
        u8g2_DrawStr(u8g2, 60, 24, buffer);
        
        // 第二行：校准偏移量和温度偏差
        if (point->calibration_offset >= 0) {
            sprintf(buffer, "O:+%.1fC", point->calibration_offset);
        } else {
            sprintf(buffer, "O:%.1fC", point->calibration_offset);
        }
        u8g2_DrawStr(u8g2, 0, 36, buffer);
        
        // 温度偏差（实测-目标）
        float temp_diff = current_temp - point->target_temperature;
        if (temp_diff >= 0) {
            sprintf(buffer, "D:+%.1fC", temp_diff);
        } else {
            sprintf(buffer, "D:%.1fC", temp_diff);
        }
        u8g2_DrawStr(u8g2, 60, 36, buffer);
        
        // 第三行：帮助文本
        const char* current_help = GetPointHelpText(point->state);
        u8g2_DrawStr(u8g2, 0, 48, current_help);
    }
    
    u8g2_SendBuffer(u8g2);
}

// ============================================================================
// 完成状态处理函数
// ============================================================================

/**
 * @brief 处理完成状态下的按键
 * @param key 按键类型
 * 完成状态下，短按或长按MODE键都退出校准系统
 */
static void HandleCompleteStateKey(KeyType key) {
    if (key == KEY_MODE || key == KEY_MODE_LONG) {
        // 退出校准系统，调用Stop函数确保目标温度正确恢复
        CalibrationSystem_Stop();
        printf("校准完成，退出校准系统\n");
    }
}

// ============================================================================
// 主按键处理函数
// ============================================================================

/**
 * @brief 处理按键输入（适配结构体方式）
 * @param key 按键类型
 * 根据系统状态分发按键处理到相应的处理函数
 */
void CalibrationSystem_HandleKey(KeyType key) {
    if (system_state == CAL_STATE_IDLE) {
        return;
    }
    
    switch (system_state) {
        case CAL_STATE_VOLTAGE_CHECK:
            HandleVoltageCheckKey(key);
            break;
            
        case CAL_STATE_RUNNING:
            HandleRunningStateKey(key);
            break;
            
        case CAL_STATE_COMPLETE:
            HandleCompleteStateKey(key);
            break;
            
        default:
            // 其他状态不处理按键
            break;
    }
}

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 设置指定校准点的温度
 * @param point_index 校准点索引
 * @param temperature 目标温度值
 */
static void SetPointTemperature(uint8_t point_index, float temperature) {
    if (point_index >= CALIBRATION_POINTS) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    float target_temp = temperature;
    
    // 使用PID控制器的温度设置函数
    setT12Temperature(target_temp);
}

/**
 * @brief 按键扫描函数
 * @return 按键类型
 * 直接使用主系统的按键扫描函数，无需转换
 */
static KeyType CalibrationKey_Scan(void) {
    return Key_Scan();
}

/**
 * @brief 保存校准数据到Flash
 * 保存所有校准点的偏移量到非易失性存储器
 */
static void SaveCalibrationData(void) {
    // 保存校准偏移量
    float calibration_offsets[CALIBRATION_POINTS];
    for (int i = 0; i < CALIBRATION_POINTS; i++) {
        calibration_offsets[i] = calibration_points[i].calibration_offset;
    }

    // 使用Flash保存校准数据
    save_result = St1xFlash_SaveCalibrationData(calibration_offsets, CALIBRATION_POINTS);
}

/**
 * @brief 检查校准系统是否激活
 * @return 系统激活状态（1=激活，0=空闲）
 */
uint8_t CalibrationSystem_IsActive(void) {
    return (system_state != CAL_STATE_IDLE);
}