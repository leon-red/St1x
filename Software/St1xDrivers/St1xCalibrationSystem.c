// 精简版校准系统实现 - 保持所有接口不变，大幅精简内部实现
#include "St1xCalibrationSystem.h"
#include "St1xFocusedHeating.h"
#include "u8g2.h"
#include "St1xKey.h"
#include "St1xFlash.h"
#include "tim.h"
#include <stdio.h>
#include <math.h>

// 精简配置常量
#define CAL_POINTS 9
#define TEMP_TOLERANCE 5.0f
#define STABLE_TIME 1000
#define UPDATE_INT 20
#define TEMP_STEP 1.0f
#define LOW_VOLT_TIME 5000

// 使用St1xADC.h中已定义的统一温度限制宏
#include "St1xADC.h"  // 包含温度限制宏定义

// 精简温度点定义
static const float cal_temps[CAL_POINTS] = {
    80.0f, 130.0f, 180.0f, 230.0f, 280.0f,
    330.0f, 380.0f, 430.0f, 480.0f
};

// 精简数据结构
typedef struct {
    float target_temp;
    float adj_temp;
    float offset;
    CalibrationPointState state;
    uint32_t heat_start;
    uint32_t stable_start;
} CalPoint;

// 精简全局变量
static CalibrationSystemState sys_state = CAL_STATE_IDLE;
static CalPoint cal_points[CAL_POINTS];
static uint8_t cur_point = 0;
static uint32_t last_update = 0;
static uint8_t save_ok = 0;
static uint32_t low_volt_start = 0;
static uint8_t orig_heat_state = 0;
static float orig_target_temp = 360.0f;  // 新增：保存原始目标温度

// 外部依赖
extern void setT12Temperature(float temp);
extern void startHeatingControlTimer(void);
extern void stopHeatingControlTimer(void);
extern float target_temperature;
extern uint8_t heating_status;
extern TIM_HandleTypeDef htim2;
extern float max_temperature_limit;  // 添加温度限制变量

// 精简辅助函数
static void InitCalPoint(uint8_t idx) {
    if (idx >= CAL_POINTS) return;
    CalPoint* p = &cal_points[idx];
    p->target_temp = cal_temps[idx];
    p->adj_temp = cal_temps[idx];
    p->offset = 0.0f;
    p->state = CAL_POINT_WAITING;
    p->heat_start = 0;
    p->stable_start = 0;
}

static void SetTemp(uint8_t idx, float temp) {
    if (idx >= CAL_POINTS) return;
    cal_points[idx].adj_temp = temp;
    setT12Temperature(temp);
    target_temperature = temp;
}

static void AdjustOffset(uint8_t idx, float adj) {
    CalPoint* p = &cal_points[idx];
    p->offset += adj;
    p->adj_temp = p->target_temp + p->offset;
    
    // 温度保护
    // 温度保护
    if (p->adj_temp > CALIBRATION_TEMPERATURE_LIMIT) p->adj_temp = CALIBRATION_TEMPERATURE_LIMIT;
    else if (p->adj_temp < 0.0f) p->adj_temp = 0.0f;
    
    SetTemp(idx, p->adj_temp);
}

static void StartPoint(uint8_t idx) {
    if (idx >= CAL_POINTS) return;
    CalPoint* p = &cal_points[idx];
    SetTemp(idx, p->adj_temp);
    p->state = CAL_POINT_HEATING;
    p->heat_start = HAL_GetTick();
}

static const char* GetStateText(CalibrationPointState s) {
    switch (s) {
        case CAL_POINT_WAITING:  return "Waiting";
        case CAL_POINT_HEATING:  return "Heating...";
        case CAL_POINT_STABLE:   return "Stable";
        case CAL_POINT_ADJUSTED: return "Confirmed";
        default: return "Unknown";
    }
}

static const char* GetHelpText(CalibrationPointState s) {
    switch (s) {
        case CAL_POINT_HEATING:  return "Heating...Wait";
        case CAL_POINT_STABLE:   return "UP/DN:Adj MODE:OK";
        case CAL_POINT_ADJUSTED: return "Completed";
        default: return "";
    }
}

// 精简状态处理
static void ProcessCalPoint(uint8_t idx) {
    if (idx >= CAL_POINTS || sys_state != CAL_STATE_RUNNING) return;
    
    CalPoint* p = &cal_points[idx];
    extern float filtered_temperature;
    float cur_temp = filtered_temperature;
    float temp_diff = fabs(cur_temp - p->adj_temp);
    uint32_t now = HAL_GetTick();
    
    if (p->state == CAL_POINT_HEATING) {
        if (temp_diff <= TEMP_TOLERANCE) {
            if (p->stable_start == 0) {
                p->stable_start = now;
            } else if ((now - p->stable_start) >= STABLE_TIME) {
                p->state = CAL_POINT_STABLE;
            }
        } else {
            p->stable_start = 0;
        }
    }
}

// 精简界面绘制
static void DrawInterface(u8g2_t *u8g2, uint8_t idx) {
    if (idx >= CAL_POINTS) return;
    
    CalPoint* p = &cal_points[idx];
    u8g2_ClearBuffer(u8g2);
    
    extern uint8_t isUSBVoltageSufficient(void);
    if (!isUSBVoltageSufficient()) {
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(u8g2, 0, 20, "Low Voltage!");
        u8g2_DrawStr(u8g2, 0, 32, "Cannot calibrate");
        
        if ((HAL_GetTick() - low_volt_start) >= LOW_VOLT_TIME) {
            CalibrationSystem_Stop();
        }
    } else if (sys_state == CAL_STATE_COMPLETE) {
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        u8g2_DrawStr(u8g2, 0, 20, "Calibration Complete!");
        u8g2_DrawStr(u8g2, 0, 32, save_ok ? "Data Saved OK" : "Save Failed!");
        u8g2_DrawStr(u8g2, 0, 44, "Press MODE to exit");
    } else if (sys_state == CAL_STATE_RUNNING) {
        char buf[32];
        u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
        
        sprintf(buf, "P%d/%d", idx + 1, CAL_POINTS);
        u8g2_DrawStr(u8g2, 0, 10, buf);
        u8g2_DrawStr(u8g2, 70, 10, GetStateText(p->state));
        
        extern float filtered_temperature;
        float cur_temp = filtered_temperature;
        
        sprintf(buf, "T:%.1fC", p->adj_temp);
        u8g2_DrawStr(u8g2, 0, 24, buf);
        
        sprintf(buf, "A:%.1fC", cur_temp);
        u8g2_DrawStr(u8g2, 60, 24, buf);
        
        if (p->offset >= 0) sprintf(buf, "O:+%.1fC", p->offset);
        else sprintf(buf, "O:%.1fC", p->offset);
        u8g2_DrawStr(u8g2, 0, 36, buf);
        
        u8g2_DrawStr(u8g2, 0, 48, GetHelpText(p->state));
    }
    
    u8g2_SendBuffer(u8g2);
}

// 核心接口实现
void CalibrationSystem_Init(void) {
    sys_state = CAL_STATE_IDLE;
    cur_point = 0;
    save_ok = 0;
    last_update = 0;
    low_volt_start = 0;
    
    for (int i = 0; i < CAL_POINTS; i++) {
        InitCalPoint(i);
    }
}

void CalibrationSystem_Start(void) {
    if (sys_state != CAL_STATE_IDLE) return;
    
    orig_heat_state = heating_status;
    orig_target_temp = target_temperature;  // 新增：保存原始目标温度
    
    if (CAL_POINTS > 0) {
        CalPoint* first = &cal_points[0];
        first->target_temp = cal_temps[0];
        first->adj_temp = cal_temps[0];
        SetTemp(0, first->adj_temp);
    }
    
    heating_status = 1;
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    startHeatingControlTimer();
    
    // 设置校准模式温度限制
    max_temperature_limit = CALIBRATION_TEMPERATURE_LIMIT;
    
    // 设置专注加热状态机为校准模式
    FocusedHeating_SetCalibrationMode(1);
    
    sys_state = CAL_STATE_VOLTAGE_CHECK;
    low_volt_start = HAL_GetTick();
}

void CalibrationSystem_Stop(void) {
    if (sys_state == CAL_STATE_IDLE) return;
    
    stopHeatingControlTimer();
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
    
    heating_status = orig_heat_state;
    
    // 恢复正常模式温度限制
    max_temperature_limit = NORMAL_TEMPERATURE_LIMIT;
    
    // 设置专注加热状态机为正常模式
    FocusedHeating_SetCalibrationMode(0);
    
    // 优化：无论原始加热状态如何，都恢复原始目标温度
    // 确保用户设置的温度在退出校准系统后得到正确恢复
    target_temperature = orig_target_temp;
    
    // 关键修复：确保主系统加热控制状态完全恢复
    // 如果原始状态是加热状态，需要重新启动加热控制
    if (orig_heat_state == 1) {
        // 优化：使用最小延时，避免长时间阻塞
        // 10ms的延时对于状态恢复是必要的，但可以优化为更短的时间
        uint32_t start_time = HAL_GetTick();
        while ((HAL_GetTick() - start_time) < 5) {
            // 短暂延时，确保状态稳定
        }
        
        // 重新启动PWM输出
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
        // 重新启动加热控制定时器
        startHeatingControlTimer();
        // 重置采样状态机
        extern uint8_t sampling_phase;
        extern uint32_t sample_start_time;
        extern uint16_t saved_pwm_value;
        sampling_phase = 0;
        sample_start_time = 0;
        saved_pwm_value = 0;
    }
    
    sys_state = CAL_STATE_IDLE;
    cur_point = 0;
    save_ok = 0;
}

void CalibrationSystem_Update(u8g2_t *u8g2) {
    uint32_t now = HAL_GetTick();
    if ((now - last_update) < UPDATE_INT) return;
    last_update = now;
    
    KeyType key = Key_Scan();
    if (key != KEY_NONE) {
        CalibrationSystem_HandleKey(key);
    }
    
    // 状态处理
    if (sys_state == CAL_STATE_VOLTAGE_CHECK) {
        extern uint8_t isUSBVoltageSufficient(void);
        if (isUSBVoltageSufficient()) {
            cur_point = 0;
            save_ok = 0;
            sys_state = CAL_STATE_RUNNING;
            StartPoint(0);
        } else if ((now - low_volt_start) >= LOW_VOLT_TIME) {
            CalibrationSystem_Stop();
        }
    } else if (sys_state == CAL_STATE_RUNNING) {
        ProcessCalPoint(cur_point);
    }
    
    DrawInterface(u8g2, cur_point);
}

// 精简按键处理
void CalibrationSystem_HandleKey(CalibrationKeyType key) {
    if (sys_state == CAL_STATE_IDLE) return;
    
    switch (sys_state) {
        case CAL_STATE_VOLTAGE_CHECK:
            if (key == KEY_MODE || key == KEY_MODE_LONG) {
                sys_state = CAL_STATE_RUNNING;
                cur_point = 0;
                
                // 设置校准模式温度限制
                max_temperature_limit = CALIBRATION_TEMPERATURE_LIMIT;
                
                StartPoint(cur_point);
            }
            break;
            
        case CAL_STATE_RUNNING:
            if (cur_point >= CAL_POINTS) return;
            CalPoint* p = &cal_points[cur_point];
            
            switch (p->state) {
                case CAL_POINT_HEATING:
                    if (key == KEY_MODE) p->state = CAL_POINT_STABLE;
                    else if (key == KEY_MODE_LONG) sys_state = CAL_STATE_IDLE;
                    break;
                    
                case CAL_POINT_STABLE:
                    if (key == KEY_UP) AdjustOffset(cur_point, TEMP_STEP);
                    else if (key == KEY_DOWN) AdjustOffset(cur_point, -TEMP_STEP);
                    else if (key == KEY_MODE) p->state = CAL_POINT_ADJUSTED;
                    else if (key == KEY_MODE_LONG) sys_state = CAL_STATE_IDLE;
                    break;
                    
                case CAL_POINT_ADJUSTED:
                    if (key == KEY_MODE) {
                        if (cur_point == (CAL_POINTS - 1)) {
                            sys_state = CAL_STATE_COMPLETE;
                            // 优化：不强制停止加热，让CalibrationSystem_Stop统一处理
                            // 只停止加热控制，保持PWM状态
                            stopHeatingControlTimer();
                            
                            // 恢复正常模式温度限制
                            max_temperature_limit = NORMAL_TEMPERATURE_LIMIT;
                            
                            // 保存数据
                            float offsets[CAL_POINTS];
                            for (int i = 0; i < CAL_POINTS; i++) {
                                offsets[i] = cal_points[i].offset;
                            }
                            save_ok = St1xFlash_SaveCalibrationData(offsets, CAL_POINTS);
                        } else {
                            cur_point++;
                            StartPoint(cur_point);
                        }
                    } else if (key == KEY_MODE_LONG) {
                        sys_state = CAL_STATE_IDLE;
                    }
                    break;
            }
            break;
            
        case CAL_STATE_COMPLETE:
            if (key == KEY_MODE || key == KEY_MODE_LONG) {
                CalibrationSystem_Stop();
            }
            break;
    }
}

uint8_t CalibrationSystem_IsActive(void) {
    return (sys_state != CAL_STATE_IDLE);
}