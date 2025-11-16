// 校准系统实现文件
#include "St1xCalibrationSystem_old.h"
#include "u8g2.h"
#include "St1xKey.h"
#include "St1xFlash.h"
#include "tim.h"
#include <stdio.h>
#include <math.h>

// 系统配置常量
#define CALIBRATION_POINTS 9
#define TEMPERATURE_TOLERANCE 5.0f
#define TEMPERATURE_STABLE_TIME 1000
#define UPDATE_INTERVAL 20
#define TEMPERATURE_ADJUST_STEP 1.0f
#define LOW_VOLTAGE_DISPLAY_TIME 5000

// 校准温度点定义
static const float calibration_temperatures[CALIBRATION_POINTS] = {
    80.0f, 130.0f, 180.0f, 230.0f, 280.0f,
    330.0f, 380.0f, 430.0f, 480.0f
};

// 校准点数据结构
typedef struct {
    uint8_t point_id;
    float target_temperature;
    float adjusted_temperature;
    float calibration_offset;
    CalibrationPointState state;
    uint32_t heating_start_time;
    uint32_t stable_start_time;
    float max_temp_deviation;
    uint8_t heating_cycles;
} CalibrationPoint;

// 全局变量
static CalibrationSystemState system_state = CAL_STATE_IDLE;
static CalibrationPoint calibration_points[CALIBRATION_POINTS];
static uint8_t current_point_index = 0;
static uint32_t last_update_time = 0;
static uint8_t save_result = 0;
static uint8_t low_voltage_state = 0;
static uint32_t low_voltage_start_time = 0;
static float calibration_temperature_limit = 500.0f;
static uint8_t original_heating_state = 0;

// 外部变量声明
extern void setT12Temperature(float temperature);
extern void startHeatingControlTimer(void);
extern void stopHeatingControlTimer(void);
extern float target_temperature;
extern uint8_t heating_status;
extern uint8_t heating_control_enabled;
extern TIM_HandleTypeDef htim2;

// 静态函数声明
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

// 系统初始化函数
void CalibrationSystem_Init(void) {
    system_state = CAL_STATE_IDLE;
    current_point_index = 0;
    save_result = 0;
    last_update_time = 0;
    
    low_voltage_state = 0;
    low_voltage_start_time = 0;
    
    for (int i = 0; i < CALIBRATION_POINTS; i++) {
        InitializeCalibrationPoint(i);
    }
    
    calibration_temperature_limit = 500.0f;
}

// 辅助函数
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

// 温度设置函数
static void SetCalibrationPointTemperature(uint8_t point_index, float temperature) {
    if (point_index >= CALIBRATION_POINTS) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    point->adjusted_temperature = temperature;
    setT12Temperature(temperature);
    
    extern float target_temperature;
    target_temperature = temperature;
}

// 辅助函数：调整温度偏移量
static void AdjustTemperatureOffset(uint8_t point_index, float adjustment) {
    CalibrationPoint* point = &calibration_points[point_index];
    
    point->calibration_offset += adjustment;
    point->adjusted_temperature = point->target_temperature + point->calibration_offset;
    
    // 温度保护
    if (point->adjusted_temperature > calibration_temperature_limit) {
        point->adjusted_temperature = calibration_temperature_limit;
        point->calibration_offset = point->adjusted_temperature - point->target_temperature;
    } else if (point->adjusted_temperature < 0.0f) {
        point->adjusted_temperature = 0.0f;
        point->calibration_offset = point->adjusted_temperature - point->target_temperature;
    }
    
    SetCalibrationPointTemperature(point_index, point->adjusted_temperature);
}

static void StartCalibrationPoint(uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    
    SetCalibrationPointTemperature(point_index, point->adjusted_temperature);
    
    point->state = CAL_POINT_HEATING;
    point->heating_start_time = HAL_GetTick();
    point->heating_cycles = 1;
}

static const char* GetPointStateText(CalibrationPointState state) {
    switch (state) {
        case CAL_POINT_WAITING:  return "Waiting";
        case CAL_POINT_HEATING:  return "Heating...";
        case CAL_POINT_STABLE:   return "Stable";
        case CAL_POINT_ADJUSTED: return "Confirmed";
        default: return "Unknown";
    }
}

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

void CalibrationSystem_Start(void) {
    if (system_state != CAL_STATE_IDLE) {
        return;
    }
    
    original_heating_state = heating_status;
    
    if (CALIBRATION_POINTS > 0) {
        CalibrationPoint* first_point = &calibration_points[0];
        first_point->target_temperature = calibration_temperatures[0];
        first_point->adjusted_temperature = calibration_temperatures[0];
        
        SetCalibrationPointTemperature(0, first_point->adjusted_temperature);
    }
    
    heating_status = 1;
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    startHeatingControlTimer();
    
    system_state = CAL_STATE_VOLTAGE_CHECK;
    low_voltage_start_time = HAL_GetTick();
}

void CalibrationSystem_Stop(void) {
    if (system_state == CAL_STATE_IDLE) {
        return;
    }
    
    stopHeatingControlTimer();
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
    
    heating_status = original_heating_state;
    
    extern float max_temperature_limit;
    max_temperature_limit = 460.0f;
    
    extern float target_temperature;
    target_temperature = 360.0f;
    
    setT12Temperature(360.0f);
    
    system_state = CAL_STATE_IDLE;
    current_point_index = 0;
    save_result = 0;
    
    low_voltage_state = 0;
}

// ============================================================================
// 主更新函数
// ============================================================================

void CalibrationSystem_Update(u8g2_t *u8g2) {
    uint32_t current_time = HAL_GetTick();
    
    if ((current_time - last_update_time) < UPDATE_INTERVAL) {
        return;
    }
    last_update_time = current_time;
    
    KeyType key = CalibrationKey_Scan();
    
    if (key != KEY_NONE) {
        CalibrationSystem_HandleKey(key);
    }
    
    switch (system_state) {
        case CAL_STATE_VOLTAGE_CHECK:
            ProcessVoltageCheckState(current_time);
            break;
            
        case CAL_STATE_RUNNING:
            ProcessCalibrationPoint(current_point_index);
            break;
            
        default:
            break;
    }
    
    DrawCalibrationInterface(u8g2, current_point_index);
}

// ============================================================================
// 状态处理函数
// ============================================================================

static void ProcessVoltageCheckState(uint32_t current_time) {
    extern uint8_t isUSBVoltageSufficient(void);
    if (isUSBVoltageSufficient()) {
        extern float max_temperature_limit;
        max_temperature_limit = 500.0f;
        
        current_point_index = 0;
        save_result = 0;
        system_state = CAL_STATE_RUNNING;
        
        for (int i = 0; i < CALIBRATION_POINTS; i++) {
            InitializeCalibrationPoint(i);
        }
        
        StartCalibrationPoint(0);
    } else {
        if ((current_time - low_voltage_start_time) >= LOW_VOLTAGE_DISPLAY_TIME) {
            CalibrationSystem_Stop();
        }
    }
}

static void HandleCalibrationPointCompleted(uint8_t point_index) {
    if (point_index == (CALIBRATION_POINTS - 1)) {
        system_state = CAL_STATE_COMPLETE;
        
        stopHeatingControlTimer();
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
        
        extern uint8_t heating_status;
        heating_status = 0;
        
        SaveCalibrationData();
        
        printf("校准完成，加热已停止，数据已保存\n");
    } else {
        current_point_index++;
        
        StartCalibrationPoint(current_point_index);
    }
}

// ============================================================================
// 按键处理函数
// ============================================================================

static void HandleVoltageCheckKey(KeyType key) {
    if (key == KEY_MODE || key == KEY_MODE_LONG) {
        system_state = CAL_STATE_RUNNING;
        
        extern float max_temperature_limit;
        max_temperature_limit = 500.0f;
        
        current_point_index = 0;
        StartCalibrationPoint(current_point_index);
    }
}

static void HandleRunningStateKey(KeyType key) {
    if (current_point_index >= CALIBRATION_POINTS) {
        return;
    }
    
    CalibrationPoint* point = &calibration_points[current_point_index];
    
    switch (point->state) {
        case CAL_POINT_HEATING:
            if (key == KEY_MODE) {
                point->state = CAL_POINT_STABLE;
                point->stable_start_time = HAL_GetTick();
            } else if (key == KEY_MODE_LONG) {
                system_state = CAL_STATE_IDLE;
            }
            break;
            
        case CAL_POINT_STABLE:
            if (key == KEY_UP) {
                AdjustTemperatureOffset(current_point_index, TEMPERATURE_ADJUST_STEP);
            } else if (key == KEY_DOWN) {
                AdjustTemperatureOffset(current_point_index, -TEMPERATURE_ADJUST_STEP);
            } else if (key == KEY_MODE) {
                point->state = CAL_POINT_ADJUSTED;
            } else if (key == KEY_MODE_LONG) {
                system_state = CAL_STATE_IDLE;
            }
            break;
            
        case CAL_POINT_ADJUSTED:
            if (key == KEY_MODE) {
                HandleCalibrationPointCompleted(current_point_index);
            } else if (key == KEY_MODE_LONG) {
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

// 校准点处理函数
static void ProcessCalibrationPoint(uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    if (system_state != CAL_STATE_RUNNING) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    extern float filtered_temperature;
    float current_temp = filtered_temperature;
    float target_temp = point->adjusted_temperature;
    float temp_diff = fabs(current_temp - target_temp);
    uint32_t current_time = HAL_GetTick();
    
    if (temp_diff > point->max_temp_deviation) {
        point->max_temp_deviation = temp_diff;
    }
    
    switch (point->state) {
        case CAL_POINT_HEATING:
            if (temp_diff <= TEMPERATURE_TOLERANCE) {
                if (point->stable_start_time == 0) {
                    point->stable_start_time = current_time;
                } else if ((current_time - point->stable_start_time) >= TEMPERATURE_STABLE_TIME) {
                    point->state = CAL_POINT_STABLE;
                }
            } else {
                point->stable_start_time = 0;
            }
            break;
            
        case CAL_POINT_STABLE:
            break;
            
        case CAL_POINT_ADJUSTED:
            HandleCalibrationPointCompleted(point_index);
            break;
            
        default:
            break;
    }
}

// 界面绘制函数
static void DrawCalibrationInterface(u8g2_t *u8g2, uint8_t point_index) {
    if (point_index >= CALIBRATION_POINTS) return;
    
    CalibrationPoint* point = &calibration_points[point_index];
    u8g2_ClearBuffer(u8g2);
    
    if (!isUSBVoltageSufficient()) {
        if (!low_voltage_state) {
            low_voltage_state = 1;
            low_voltage_start_time = HAL_GetTick();
        }
        
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(u8g2, 0, 20, "Low Voltage!");
        u8g2_DrawStr(u8g2, 0, 32, "Cannot calibrate");
        u8g2_SendBuffer(u8g2);
        
        if ((HAL_GetTick() - low_voltage_start_time) >= LOW_VOLTAGE_DISPLAY_TIME) {
            CalibrationSystem_Stop();
            low_voltage_state = 0;
        }
        return;
    } else {
        low_voltage_state = 0;
    }
    
    if (system_state == CAL_STATE_VOLTAGE_CHECK) {
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        
        if (isUSBVoltageSufficient()) {
            u8g2_DrawStr(u8g2, 0, 20, "Voltage OK");
            u8g2_DrawStr(u8g2, 0, 32, "Starting Calibration...");
        } else {
            u8g2_DrawStr(u8g2, 0, 20, "Low Voltage!");
            u8g2_DrawStr(u8g2, 0, 32, "Please check power");
            
            uint32_t current_time = HAL_GetTick();
            uint32_t remaining_time = LOW_VOLTAGE_DISPLAY_TIME - (current_time - low_voltage_start_time);
            uint32_t seconds = remaining_time / 1000;
            
            char countdown_str[32];
            sprintf(countdown_str, "Return in %lu sec", seconds);
            u8g2_DrawStr(u8g2, 0, 44, countdown_str);
        }
    } else if (system_state == CAL_STATE_COMPLETE) {
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(u8g2, 0, 20, "Calibration Complete!");
        if (save_result) {
            u8g2_DrawStr(u8g2, 0, 32, "Data Saved OK");
        } else {
            u8g2_DrawStr(u8g2, 0, 32, "Save Failed!");
        }
        u8g2_DrawStr(u8g2, 0, 44, "Press MODE to exit");
    } else if (system_state == CAL_STATE_RUNNING) {
        char buffer[32];
        
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        sprintf(buffer, "P%d/%d", point->point_id, CALIBRATION_POINTS);
        u8g2_DrawStr(u8g2, 0, 10, buffer);
        
        const char* current_status = GetPointStateText(point->state);
        u8g2_DrawStr(u8g2, 70, 10, current_status);
        
        extern float filtered_temperature;
        float current_temp = filtered_temperature;
        sprintf(buffer, "T:%.1fC", point->adjusted_temperature);
        u8g2_DrawStr(u8g2, 0, 24, buffer);
        
        sprintf(buffer, "A:%.1fC", current_temp);
        u8g2_DrawStr(u8g2, 60, 24, buffer);
        
        if (point->calibration_offset >= 0) {
            sprintf(buffer, "O:+%.1fC", point->calibration_offset);
        } else {
            sprintf(buffer, "O:%.1fC", point->calibration_offset);
        }
        u8g2_DrawStr(u8g2, 0, 36, buffer);
        
        float temp_diff = current_temp - point->target_temperature;
        if (temp_diff >= 0) {
            sprintf(buffer, "D:+%.1fC", temp_diff);
        } else {
            sprintf(buffer, "D:%.1fC", temp_diff);
        }
        u8g2_DrawStr(u8g2, 60, 36, buffer);
        
        const char* current_help = GetPointHelpText(point->state);
        u8g2_DrawStr(u8g2, 0, 48, current_help);
    }
    
    u8g2_SendBuffer(u8g2);
}

// ============================================================================
// 完成状态处理函数
// ============================================================================

static void HandleCompleteStateKey(KeyType key) {
    if (key == KEY_MODE || key == KEY_MODE_LONG) {
        CalibrationSystem_Stop();
        printf("校准完成，退出校准系统\n");
    }
}

// ============================================================================
// 主按键处理函数
// ============================================================================

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
            break;
    }
}

static KeyType CalibrationKey_Scan(void) {
    return Key_Scan();
}

static void SaveCalibrationData(void) {
    float calibration_offsets[CALIBRATION_POINTS];
    for (int i = 0; i < CALIBRATION_POINTS; i++) {
        calibration_offsets[i] = calibration_points[i].calibration_offset;
    }

    save_result = St1xFlash_SaveCalibrationData(calibration_offsets, CALIBRATION_POINTS);
}

uint8_t CalibrationSystem_IsActive(void) {
    return (system_state != CAL_STATE_IDLE);
}