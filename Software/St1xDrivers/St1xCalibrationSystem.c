#include "St1xCalibrationSystem.h"
#include "u8g2.h"
#include "St1xKey.h"
#include "St1xADC.h"
#include <stdio.h>
#include <math.h>

// 硬件抽象层接口指针
static const CalibrationHardwareInterface* hw_interface = NULL;

// 校准系统配置
#define CALIBRATION_POINTS 9
#define TEMPERATURE_TOLERANCE 5.0f
#define TEMPERATURE_STABLE_TIME 1000
#define KEY_DEBOUNCE_TIME 10
#define UPDATE_INTERVAL 50  // 50ms更新间隔
#define TEMPERATURE_ADJUST_STEP 1.0f  // 温度调整步进值
#define CALIBRATION_TEMPERATURE_LIMIT 500.0f  // 校准模式下的温度限制

// 调试开关 - 设置为1启用调试输出，0禁用
#define CALIBRATION_DEBUG_ENABLED 0

// 调试宏定义
#if CALIBRATION_DEBUG_ENABLED
    #define CAL_DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
    #define CAL_DEBUG_PRINTF(...) ((void)0)
#endif

// 校准点温度定义
static const float calibration_temperatures[CALIBRATION_POINTS] = {
    100.0f, 150.0f, 200.0f, 250.0f, 300.0f, 
    350.0f, 400.0f, 450.0f, 500.0f
};

// 独立校准点结构体定义
typedef struct {
    uint8_t point_id;                    // 校准点ID（1-9）
    float target_temperature;           // 目标温度
    float adjusted_temperature;         // 调整后的目标温度
    float calibration_offset;           // 校准偏移量
    CalibrationPointState state;        // 当前状态
    uint32_t heating_start_time;       // 加热开始时间
    uint32_t stable_start_time;        // 稳定开始时间
    float max_temp_deviation;          // 最大温度偏差记录
    uint8_t heating_cycles;            // 加热循环次数
} CalibrationPoint;

// 独立界面结构体定义
typedef struct {
    uint8_t point_id;                    // 界面ID（对应校准点）
    const char* title;                  // 界面标题
    const char* status_text;            // 状态文本
    const char* help_text;              // 帮助文本
    uint8_t display_mode;              // 显示模式（0=简洁，1=详细）
    uint32_t last_update_time;         // 最后更新时间
    uint8_t update_counter;            // 更新计数器
} CalibrationInterface;

// 系统状态变量
static CalibrationSystemState system_state = CAL_STATE_IDLE;
static CalibrationPoint calibration_points[CALIBRATION_POINTS];
static CalibrationInterface calibration_interfaces[CALIBRATION_POINTS];
static uint8_t current_point_index = 0;
static uint32_t last_update_time = 0;
static uint32_t last_key_time = 0;
static uint8_t save_result = 0;
static uint8_t cal_key_debug_display = 0;
static uint32_t cal_key_debug_time = 0;
static CalibrationKeyType last_displayed_key = CAL_KEY_NONE;

// 电压不足状态变量
static uint8_t low_voltage_state = 0;           // 电压不足状态标志
static uint32_t low_voltage_start_time = 0;   // 电压不足开始时间
#define LOW_VOLTAGE_DISPLAY_TIME 5000          // 电压不足显示时间（5秒）

// 私有函数声明
static void InitializeCalibrationPoint(uint8_t point_index);
static void InitializeCalibrationInterface(uint8_t point_index);
static void SetPointTemperature(uint8_t point_index);
static void SaveCalibrationData(void);
static void RestoreOriginalSettings(void);
static void ProcessVoltageCheckState(uint32_t current_time);
static void ProcessCalibrationPoint(uint8_t point_index);
static void DrawCalibrationInterface(u8g2_t *u8g2, uint8_t point_index);
static const char* GetPointStateText(CalibrationPointState state);
static const char* GetPointHelpText(CalibrationPointState state);

/**
 * @brief 初始化独立校准系统
 */
void CalibrationSystem_Init(const CalibrationHardwareInterface* hw_interface_ptr) {
    CAL_DEBUG_PRINTF("[CalibrationSystem] Initializing independent calibration system...\n");
    
    // 检查硬件接口是否有效
    if (hw_interface_ptr == NULL) {
        CAL_DEBUG_PRINTF("[CalibrationSystem] ERROR: Hardware interface is NULL!\n");
        return;
    }
    
    // 保存硬件接口指针
    hw_interface = hw_interface_ptr;
    
    // 重置系统状态
    system_state = CAL_STATE_IDLE;
    current_point_index = 0;
    save_result = 0;
    last_update_time = 0;
    last_key_time = 0;
    
    // 重置电压不足状态
    low_voltage_state = 0;
    low_voltage_start_time = 0;
    
    // 初始化所有校准点（不加载已保存的数据）
    for (int i = 0; i < CALIBRATION_POINTS; i++) {
        InitializeCalibrationPoint(i);
        InitializeCalibrationInterface(i);
    }
    
    CAL_DEBUG_PRINTF("[CalibrationSystem] Initialization completed successfully\n");
}

/**
 * @brief 初始化单个校准点
 */
static void InitializeCalibrationPoint(uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    
    point->point_id = point_index + 1;  // ID从1开始
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
 * @brief 初始化单个校准界面
 */
static void InitializeCalibrationInterface(uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    
    CalibrationInterface* interface = &calibration_interfaces[point_index];
    CalibrationPoint* point = &calibration_points[point_index];
    
    interface->point_id = point->point_id;
    interface->title = "Calibration Point";
    interface->status_text = GetPointStateText(point->state);
    interface->help_text = GetPointHelpText(point->state);
    interface->display_mode = 1;  // 详细模式
    interface->last_update_time = 0;
    interface->update_counter = 0;
}

/**
 * @brief 获取状态文本
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
 * @brief 获取帮助文本（简洁版）
 */
static const char* GetPointHelpText(CalibrationPointState state) {
    switch (state) {
        case CAL_POINT_HEATING:  return "Heating...Wait";
        case CAL_POINT_STABLE:   return "UP/DN:Adj MODE:OK";
        case CAL_POINT_ADJUSTED: return "Completed";
        default: return "";
    }
}

/**
 * @brief 启动独立校准系统
 */
void CalibrationSystem_Start(void) {
    if (system_state != CAL_STATE_IDLE) {
        CAL_DEBUG_PRINTF("[CalibrationSystem] Cannot start: system is not idle\n");
        return;
    }
    
    // 检查硬件接口是否已初始化
    if (hw_interface == NULL) {
        CAL_DEBUG_PRINTF("[CalibrationSystem] ERROR: Hardware interface not initialized!\n");
        return;
    }
    
    // 进入电压检测状态
    system_state = CAL_STATE_VOLTAGE_CHECK;
    low_voltage_start_time = hw_interface->getTick();
    CAL_DEBUG_PRINTF("[CalibrationSystem] Entering voltage check state\n");
}

/**
 * @brief 停止独立校准系统
 */
void CalibrationSystem_Stop(void) {
    if (system_state == CAL_STATE_IDLE) {
        return;
    }
    
    if (hw_interface == NULL) {
        CAL_DEBUG_PRINTF("[CalibrationSystem] ERROR: Hardware interface not initialized!\n");
        return;
    }
    
    CAL_DEBUG_PRINTF("[CalibrationSystem] Stopping calibration system...\n");
    
    // 停止加热控制
    if (hw_interface->stopHeating) {
        hw_interface->stopHeating();
    }
    
    // 恢复正常的温度限制
    extern float max_temperature_limit;
    max_temperature_limit = NORMAL_TEMPERATURE_LIMIT;
    CAL_DEBUG_PRINTF("[CalibrationSystem] Temperature limit restored to %.1f°C\n", max_temperature_limit);
    
    // 重置系统状态
    system_state = CAL_STATE_IDLE;
    current_point_index = 0;
    save_result = 0;
    
    // 重置电压不足状态
    low_voltage_state = 0;
    
    CAL_DEBUG_PRINTF("[CalibrationSystem] Calibration system stopped\n");
}

/**
 * @brief 独立校准系统更新（在主循环中调用）
 */
void CalibrationSystem_Update(u8g2_t *u8g2) {
    if (hw_interface == NULL) {
        return;
    }
    
    uint32_t current_time = hw_interface->getTick();
    
    // 控制更新频率（50ms间隔）
    if ((current_time - last_update_time) < UPDATE_INTERVAL) {
        return;
    }
    last_update_time = current_time;
    
    // 处理按键输入（独立校准系统自己处理按键）
    static CalibrationKeyType last_key = CAL_KEY_NONE;
    
    CalibrationKeyType key = hw_interface->keyScan();
    
    // 系统运行中的按键处理 - 优化响应速度
    if (key != CAL_KEY_NONE && key != last_key) {
        // 减少防抖时间到50ms，与主系统保持一致
        if ((current_time - last_key_time) >= 50) {  
            CalibrationSystem_HandleKey(key);
            last_key_time = current_time;
        }
    }
    
    last_key = key;
    
    // 处理电压检测状态
    if (system_state == CAL_STATE_VOLTAGE_CHECK) {
        ProcessVoltageCheckState(current_time);
    }
    
    // 处理当前校准点（使用新的结构体方式）
    if (system_state == CAL_STATE_RUNNING) {
        ProcessCalibrationPoint(current_point_index);
    }
    
    // 绘制界面
    DrawCalibrationInterface(u8g2, current_point_index);
}

/**
 * @brief 处理电压检测状态
 */
static void ProcessVoltageCheckState(uint32_t current_time) {
    if (hw_interface == NULL) return;
    
    // 检查电压是否足够
    if (hw_interface->voltageCheck && hw_interface->voltageCheck()) {
        // 电压足够，进入校准运行状态
        CAL_DEBUG_PRINTF("[CalibrationSystem] Voltage sufficient, starting calibration...\n");
        
        // 在校准模式下临时提高温度限制到500度
        extern float max_temperature_limit;
        max_temperature_limit = CALIBRATION_TEMPERATURE_LIMIT;
        CAL_DEBUG_PRINTF("[CalibrationSystem] Temperature limit temporarily increased to %.1f°C\n", max_temperature_limit);
        
        // 重置状态
        current_point_index = 0;
        save_result = 0;
        system_state = CAL_STATE_RUNNING;
        
        // 重置所有校准点
        for (int i = 0; i < CALIBRATION_POINTS; i++) {
            InitializeCalibrationPoint(i);
        }
        
        // 启动第一个校准点
        SetPointTemperature(0);
        calibration_points[0].state = CAL_POINT_HEATING;
        calibration_points[0].heating_start_time = current_time;
        calibration_points[0].heating_cycles = 1;
        
        // 启动加热控制
        if (hw_interface->startHeating) {
            hw_interface->startHeating();
        }
        
        CAL_DEBUG_PRINTF("[CalibrationSystem] Calibration started successfully\n");
    } else {
        // 电压不足，检查是否超过5秒显示时间
        if ((current_time - low_voltage_start_time) >= LOW_VOLTAGE_DISPLAY_TIME) {
            // 超过5秒，返回主界面
            CAL_DEBUG_PRINTF("[CalibrationSystem] Low voltage timeout, returning to main menu\n");
            CalibrationSystem_Stop();
        }
    }
}

/**
 * @brief 处理单个校准点逻辑（新的结构体方式）
 */
static void ProcessCalibrationPoint(uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    if (system_state != CAL_STATE_RUNNING) return;
    if (hw_interface == NULL) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    float current_temp = hw_interface->getTemperature();
    float target_temp = point->adjusted_temperature;
    float temp_diff = fabs(current_temp - target_temp);
    uint32_t current_time = hw_interface->getTick();
    
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
                    // 更新界面状态文本
                    calibration_interfaces[point_index].status_text = GetPointStateText(CAL_POINT_STABLE);
                    calibration_interfaces[point_index].help_text = GetPointHelpText(CAL_POINT_STABLE);
                    CAL_DEBUG_PRINTF("[CalibrationSystem] Point %d reached stable state\n", point->point_id);
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
            // 当前点已完成，进入下一个点
            CAL_DEBUG_PRINTF("[CalibrationSystem] Point %d completed, moving to next point\n", point->point_id);
            
            // 如果有下一个点，启动下一个点
            if (point_index + 1 < CALIBRATION_POINTS) {
                current_point_index = point_index + 1;
                SetPointTemperature(current_point_index);
                calibration_points[current_point_index].state = CAL_POINT_HEATING;
                calibration_points[current_point_index].heating_start_time = current_time;
                calibration_points[current_point_index].heating_cycles++;
                
                // 更新新点的界面状态
                calibration_interfaces[current_point_index].status_text = GetPointStateText(CAL_POINT_HEATING);
                calibration_interfaces[current_point_index].help_text = GetPointHelpText(CAL_POINT_HEATING);
            } else {
                // 所有点完成
                SaveCalibrationData();
                system_state = CAL_STATE_COMPLETE;
                CAL_DEBUG_PRINTF("[CalibrationSystem] All calibration points completed\n");
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief 绘制独立校准界面（每个点独立界面）
 */
static void DrawCalibrationInterface(u8g2_t *u8g2, uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    CalibrationInterface* interface = &calibration_interfaces[point_index];
//    float current_temp = hw_interface->getTemperature();
    
    u8g2_ClearBuffer(u8g2);
    
    // 检查电压
    if (hw_interface->voltageCheck && !hw_interface->voltageCheck()) {
        if (!low_voltage_state) {
            // 首次检测到电压不足，设置状态和计时
            low_voltage_state = 1;
            low_voltage_start_time = hw_interface->getTick();
        }
        
        // 显示电压不足提示
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(u8g2, 0, 20, "Low Voltage!");
        u8g2_DrawStr(u8g2, 0, 32, "Cannot calibrate");
        u8g2_SendBuffer(u8g2);
        
        // 检查是否达到显示时间
        if ((hw_interface->getTick() - low_voltage_start_time) >= LOW_VOLTAGE_DISPLAY_TIME) {
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
        if (hw_interface->voltageCheck && hw_interface->voltageCheck()) {
            // 电压足够，显示准备进入校准
            u8g2_DrawStr(u8g2, 0, 20, "Voltage OK");
            u8g2_DrawStr(u8g2, 0, 32, "Starting Calibration...");
        } else {
            // 电压不足，显示低电压警告
            u8g2_DrawStr(u8g2, 0, 20, "Low Voltage!");
            u8g2_DrawStr(u8g2, 0, 32, "Please check power");
            
            // 显示倒计时
            uint32_t current_time = hw_interface->getTick();
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
        // 独立校准点界面 - 紧凑布局（使用缩写）
        char buffer[32];
        
        // 标题栏 - 校准点信息
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        sprintf(buffer, "P%d/%d", point->point_id, CALIBRATION_POINTS);
        u8g2_DrawStr(u8g2, 0, 10, buffer);
        
        // 状态信息 - 右侧显示
        interface->status_text = GetPointStateText(point->state);
        u8g2_DrawStr(u8g2, 70, 10, interface->status_text);
        
        // 第一行：目标温度和实测温度
        sprintf(buffer, "T:%.1fC", point->target_temperature);
        u8g2_DrawStr(u8g2, 0, 24, buffer);
        
        float current_temp = hw_interface->getTemperature();
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
        
        // 详细模式下的额外信息
        if (interface->display_mode == 1) {
            // 第三行：最大偏差和加热循环
            sprintf(buffer, "M:%.1fC", point->max_temp_deviation);
            u8g2_DrawStr(u8g2, 0, 48, buffer);
            
            sprintf(buffer, "C:%d", point->heating_cycles);
            u8g2_DrawStr(u8g2, 60, 48, buffer);
            
            // 第四行：帮助文本（简化）
            interface->help_text = GetPointHelpText(point->state);
            u8g2_DrawStr(u8g2, 0, 60, interface->help_text);
        } else {
            // 简洁模式：只显示帮助文本
            interface->help_text = GetPointHelpText(point->state);
            u8g2_DrawStr(u8g2, 0, 48, interface->help_text);
        }
        
//        // 按键调试信息
//        if (cal_key_debug_display && (hw_interface->getTick() - cal_key_debug_time) < 1000) {
//            char key_str[16];
//            switch (last_displayed_key) {
//                case CAL_KEY_UP:   sprintf(key_str, "KEY:UP"); break;
//                case CAL_KEY_DOWN: sprintf(key_str, "KEY:DOWN"); break;
//                case CAL_KEY_MODE: sprintf(key_str, "KEY:MODE"); break;
//                default: sprintf(key_str, "KEY:?"); break;
//            }
//            u8g2_DrawStr(u8g2, 80, 76, key_str);
//        } else {
//            cal_key_debug_display = 0;
//        }
    }
    
    u8g2_SendBuffer(u8g2);
}

/**
 * @brief 处理按键输入（适配结构体方式）
 */
void CalibrationSystem_HandleKey(CalibrationKeyType key) {
    if (system_state != CAL_STATE_RUNNING && system_state != CAL_STATE_COMPLETE) {
        return;
    }
    
    if (hw_interface == NULL) {
        return;
    }
    
    uint32_t current_time = hw_interface->getTick();
    
    // 按键防抖
    if ((current_time - last_key_time) < KEY_DEBOUNCE_TIME) {
        return;
    }
    last_key_time = current_time;
    
    // 设置按键调试显示（统一设置一次）
    cal_key_debug_display = 1;
    cal_key_debug_time = current_time;
    last_displayed_key = key;
    
    // 完成状态下只响应MODE键退出
    if (system_state == CAL_STATE_COMPLETE) {
        if (key == CAL_KEY_MODE) {
            printf("[CalibrationSystem] User requested exit from complete state\n");
            CalibrationSystem_Stop();
        }
        return;
    }
    
    // 检查电压
    if (hw_interface->voltageCheck && !hw_interface->voltageCheck()) {
        return;
    }
    
    CalibrationPoint* current_point = &calibration_points[current_point_index];
    
    switch (key) {
        case CAL_KEY_UP:
            // 修正校准偏移量，而不是修改目标温度
            current_point->calibration_offset += TEMPERATURE_ADJUST_STEP;
            // 重新计算调整后的温度（目标温度 + 偏移量）
            current_point->adjusted_temperature = current_point->target_temperature + current_point->calibration_offset;
            printf("[CalibrationSystem] Point %d calibration offset adjusted to %.1f°C (Target:%.1f°C, Adjusted:%.1f°C)\n", 
                   current_point->point_id, current_point->calibration_offset,
                   current_point->target_temperature, current_point->adjusted_temperature);
            break;
            
        case CAL_KEY_DOWN:
            // 修正校准偏移量，而不是修改目标温度
            current_point->calibration_offset -= TEMPERATURE_ADJUST_STEP;
            // 重新计算调整后的温度（目标温度 + 偏移量）
            current_point->adjusted_temperature = current_point->target_temperature + current_point->calibration_offset;
            printf("[CalibrationSystem] Point %d calibration offset adjusted to %.1f°C (Target:%.1f°C, Adjusted:%.1f°C)\n", 
                   current_point->point_id, current_point->calibration_offset,
                   current_point->target_temperature, current_point->adjusted_temperature);
            break;
            
        case CAL_KEY_MODE:
            if (current_point->state == CAL_POINT_STABLE) {
                // 确认当前校准点，进入下一个校准点
                current_point->state = CAL_POINT_ADJUSTED;
                printf("[CalibrationSystem] Point %d confirmed\n", current_point->point_id);
            } else if (current_point->state == CAL_POINT_HEATING) {
                // 在加热状态下，可以强制跳过当前点（用于紧急情况）
                current_point->state = CAL_POINT_ADJUSTED;
                printf("[CalibrationSystem] Point %d skipped by user\n", current_point->point_id);
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief 私有函数：设置指定点的温度
 */
static void SetPointTemperature(uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    if (hw_interface == NULL) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    float target_temp = point->adjusted_temperature;
    
    if (hw_interface->setTemperature) {
        hw_interface->setTemperature(target_temp);
    }
    
    printf("[CalibrationSystem] Setting Point %d temperature to %.1f°C\n", 
           point->point_id, target_temp);
}

/**
 * @brief 私有函数：保存校准数据
 */
static void SaveCalibrationData(void) {
    if (hw_interface == NULL) {
        save_result = 0;
        printf("[CalibrationSystem] ERROR: Hardware interface not initialized!\n");
        return;
    }
    
    // 将调整后的温度值转换为相对于原始温度的偏移量并保存
    float temp_offsets[CALIBRATION_POINTS];
    for (int i = 0; i < CALIBRATION_POINTS; i++) {
        temp_offsets[i] = calibration_points[i].adjusted_temperature - calibration_temperatures[i];
    }
    
    if (hw_interface->saveData) {
        hw_interface->saveData(temp_offsets, CALIBRATION_POINTS);
        save_result = 1;
    } else {
        save_result = 0;
    }
    
    printf("[CalibrationSystem] Calibration data saved: %s\n", 
           save_result ? "SUCCESS" : "FAILED");
}

/**
 * @brief 私有函数：恢复原始设置
 */
static void RestoreOriginalSettings(void) {
    printf("[CalibrationSystem] Restoring original settings...\n");
    
    // 使用硬件抽象层接口停止加热控制定时器
    if (hw_interface != NULL && hw_interface->stopHeatingTimer != NULL) {
        hw_interface->stopHeatingTimer();
    }
    
    // 使用硬件抽象层接口停止加热
    if (hw_interface != NULL && hw_interface->stopHeating != NULL) {
        hw_interface->stopHeating();
    }
    
    // 打印校准统计信息
    printf("[CalibrationSystem] Calibration statistics:\n");
    for (int i = 0; i < CALIBRATION_POINTS; i++) {
        CalibrationPoint* point = &calibration_points[i];
        printf("  Point %d: Target=%.1f°C, Adjusted=%.1f°C, Offset=%.1f°C, MaxDev=%.1f°C, Cycles=%d\n",
               point->point_id, point->target_temperature, point->adjusted_temperature,
               point->calibration_offset, point->max_temp_deviation, point->heating_cycles);
    }
}

/**
 * @brief 检查系统是否激活
 */
uint8_t CalibrationSystem_IsActive(void) {
    return (system_state != CAL_STATE_IDLE);
}

/**
 * @brief 获取系统状态
 */
CalibrationSystemState CalibrationSystem_GetState(void) {
    return system_state;
}

// ==================== 兼容原有校准系统的接口 ====================

/**
 * @brief 兼容接口：退出校准模式（使用独立校准系统）
 */
void St1xCalibration_Exit(void) {
    CalibrationSystem_Stop();
}

/**
 * @brief 兼容接口：处理校准按键（使用独立校准系统）
 */
void St1xCalibration_HandleKey(CalibrationKeyType key) {
    // 如果独立校准系统未激活，先启动它
    if (!CalibrationSystem_IsActive()) {
        CalibrationSystem_Start();
    }
    
    // 使用独立校准系统的按键处理
    CalibrationSystem_HandleKey(key);
}

/**
 * @brief 兼容接口：校准主循环（使用独立校准系统）
 */
void St1xCalibration_MainLoop(u8g2_t *u8g2) {
    // 如果独立校准系统未激活，先启动它
    if (!CalibrationSystem_IsActive()) {
        CalibrationSystem_Start();
    }
    
    // 使用独立校准系统的更新函数
    CalibrationSystem_Update(u8g2);
}

/**
 * @brief 兼容接口：检查是否在校准中（使用独立校准系统）
 */
uint8_t St1xCalibration_IsInProgress(void) {
    return CalibrationSystem_IsActive();
}

/**
 * @brief 兼容接口：清除校准数据（使用独立校准系统）
 */
void St1xCalibration_ClearData(void) {
    // 使用硬件抽象层接口清除Flash中的校准数据
    if (hw_interface != NULL && hw_interface->saveData != NULL) {
        // 传递空的偏移量数组来清除数据
        float empty_offsets[CALIBRATION_POINTS] = {0};
        hw_interface->saveData(empty_offsets, CALIBRATION_POINTS);
    }
    
    // 如果系统当前空闲，重新初始化以清除内存中的数据
    if (system_state == CAL_STATE_IDLE) {
        for (int i = 0; i < CALIBRATION_POINTS; i++) {
            calibration_points[i].calibration_offset = 0.0f;
        }
    }
}