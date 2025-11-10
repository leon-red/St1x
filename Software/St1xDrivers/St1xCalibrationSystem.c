#include "St1xCalibrationSystem.h"
#include "u8g2.h"
#include "St1xKey.h"
#include "St1xADC.h"
#include "St1xPID.h"
#include "St1xFlash.h"
#include "tim.h"
#include "main.h"
#include <stdio.h>
#include <math.h>

// 校准系统配置
#define CALIBRATION_POINTS 9
#define TEMPERATURE_TOLERANCE 5.0f
#define TEMPERATURE_STABLE_TIME 1000
#define UPDATE_INTERVAL 50
#define TEMPERATURE_ADJUST_STEP 1.0f

// 校准点温度定义
static const float calibration_temperatures[CALIBRATION_POINTS] = {
    100.0f, 150.0f, 200.0f, 250.0f, 300.0f, 
    350.0f, 400.0f, 450.0f, 500.0f
};

// 简化校准点结构体
typedef struct {
    float target_temp;      // 目标温度
    float offset;          // 校准偏移量
    uint8_t state;         // 状态: 0=等待, 1=加热, 2=稳定, 3=完成
    uint32_t stable_time;  // 稳定开始时间
} CalibrationPoint;

// 系统状态变量
static uint8_t system_state = 0;  // 0=空闲, 1=运行, 2=完成
static CalibrationPoint points[CALIBRATION_POINTS];
static uint8_t current_point = 0;
static uint32_t last_update = 0;

// 私有函数声明
static void ProcessCalibrationPoint(void);
static void DrawCalibrationInterface(u8g2_t *u8g2);

/**
 * @brief 初始化校准系统
 */
void CalibrationSystem_Init(void) {
    system_state = 0;
    current_point = 0;
    last_update = 0;
    
    // 初始化校准点
    for (int i = 0; i < CALIBRATION_POINTS; i++) {
        points[i].target_temp = calibration_temperatures[i];
        points[i].offset = 0.0f;
        points[i].state = 0;
        points[i].stable_time = 0;
    }
}

/**
 * @brief 启动校准系统
 */
void CalibrationSystem_Start(void) {
    if (system_state != 0) return;
    
    // 检查电压
    extern uint8_t isUSBVoltageSufficient(void);
    if (!isUSBVoltageSufficient()) {
        return; // 电压不足，不启动
    }
    
    system_state = 1;
    current_point = 0;
    
    // 设置第一个校准点
    points[0].state = 1;
    setT12Temperature(points[0].target_temp + points[0].offset);
    
    // 临时提高温度限制
    extern float max_temperature_limit;
    max_temperature_limit = 500.0f;
}

/**
 * @brief 停止校准系统
 */
void CalibrationSystem_Stop(void) {
    if (system_state == 0) return;
    
    // 停止加热
    extern void StopCalibrationHeating(void);
    StopCalibrationHeating();
    
    // 恢复温度限制
    extern float max_temperature_limit;
    max_temperature_limit = 460.0f;
    
    system_state = 0;
}

/**
 * @brief 校准系统更新
 */
void CalibrationSystem_Update(u8g2_t *u8g2) {
    uint32_t current_time = HAL_GetTick();
    
    // 控制更新频率
    if ((current_time - last_update) < UPDATE_INTERVAL) return;
    last_update = current_time;
    
    // 处理按键
    KeyType key = Key_Scan();
    if (key != KEY_NONE) {
        CalibrationSystem_HandleKey(key);
    }
    
    // 处理校准逻辑
    if (system_state == 1) {
        ProcessCalibrationPoint();
    }
    
    // 绘制界面
    DrawCalibrationInterface(u8g2);
}

/**
 * @brief 处理校准点逻辑
 */
static void ProcessCalibrationPoint(void) {
    if (current_point >= CALIBRATION_POINTS) return;
    
    CalibrationPoint* point = &points[current_point];
    extern float filtered_temperature;
    float current_temp = filtered_temperature;
    float target_temp = point->target_temp + point->offset;
    float temp_diff = fabs(current_temp - target_temp);
    uint32_t current_time = HAL_GetTick();
    
    switch (point->state) {
        case 1: // 加热状态
            if (temp_diff <= TEMPERATURE_TOLERANCE) {
                if (point->stable_time == 0) {
                    point->stable_time = current_time;
                } else if ((current_time - point->stable_time) >= TEMPERATURE_STABLE_TIME) {
                    point->state = 2; // 进入稳定状态
                }
            } else {
                point->stable_time = 0; // 温度不稳定，重置计时
            }
            break;
            
        case 2: // 稳定状态（等待用户调整）
            // 状态由按键处理
            break;
            
        case 3: // 完成状态
            // 切换到下一个点
            if (current_point < CALIBRATION_POINTS - 1) {
                current_point++;
                points[current_point].state = 1;
                setT12Temperature(points[current_point].target_temp + points[current_point].offset);
            } else {
                system_state = 2; // 所有点完成
            }
            break;
    }
}

/**
 * @brief 处理按键输入
 */
void CalibrationSystem_HandleKey(KeyType key) {
    if (system_state == 0) return;
    
    if (system_state == 2) {
        // 完成状态：任意按键退出
        system_state = 0;
        return;
    }
    
    if (current_point >= CALIBRATION_POINTS) return;
    CalibrationPoint* point = &points[current_point];
    
    switch (point->state) {
        case 1: // 加热状态
            if (key == KEY_MODE) {
                point->state = 2; // 跳过加热
            } else if (key == KEY_MODE_LONG) {
                system_state = 0; // 退出校准
            }
            break;
            
        case 2: // 稳定状态
            if (key == KEY_UP) {
                point->offset += TEMPERATURE_ADJUST_STEP;
                setT12Temperature(point->target_temp + point->offset);
            } else if (key == KEY_DOWN) {
                point->offset -= TEMPERATURE_ADJUST_STEP;
                setT12Temperature(point->target_temp + point->offset);
            } else if (key == KEY_MODE) {
                point->state = 3; // 确认当前点
            } else if (key == KEY_MODE_LONG) {
                system_state = 0; // 退出校准
            }
            break;
    }
}

/**
 * @brief 绘制校准界面
 */
static void DrawCalibrationInterface(u8g2_t *u8g2) {
    u8g2_ClearBuffer(u8g2);
    
    if (system_state == 0) return;
    
    char buffer[32];
    u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
    
    if (system_state == 2) {
        // 完成界面
        u8g2_DrawStr(u8g2, 0, 20, "Calibration Complete!");
        u8g2_DrawStr(u8g2, 0, 32, "Press any key to exit");
    } else {
        // 校准点界面
        CalibrationPoint* point = &points[current_point];
        extern float filtered_temperature;
        float current_temp = filtered_temperature;
        
        // 标题行
        sprintf(buffer, "P%d/%d ", current_point + 1, CALIBRATION_POINTS);
        u8g2_DrawStr(u8g2, 0, 10, buffer);
        
        // 状态显示
        const char* status_text = "";
        switch (point->state) {
            case 1: status_text = "Heating"; break;
            case 2: status_text = "Adjust"; break;
            case 3: status_text = "Confirm"; break;
        }
        u8g2_DrawStr(u8g2, 70, 10, status_text);
        
        // 温度信息
        sprintf(buffer, "T:%.0fC", point->target_temp + point->offset);
        u8g2_DrawStr(u8g2, 0, 24, buffer);
        
        sprintf(buffer, "A:%.0fC", current_temp);
        u8g2_DrawStr(u8g2, 60, 24, buffer);
        
        // 偏移量
        if (point->offset >= 0) {
            sprintf(buffer, "O:+%.0fC", point->offset);
        } else {
            sprintf(buffer, "O:%.0fC", point->offset);
        }
        u8g2_DrawStr(u8g2, 0, 36, buffer);
        
        // 操作提示
        if (point->state == 1) {
            u8g2_DrawStr(u8g2, 0, 48, "MODE:Skip  LONG:Exit");
        } else if (point->state == 2) {
            u8g2_DrawStr(u8g2, 0, 48, "UP/DN:Adj MODE:OK");
        }
    }
    
    u8g2_SendBuffer(u8g2);
}

/**
 * @brief 保存校准数据
 */
void CalibrationSystem_SaveData(void) {
    if (system_state != 2) return;
    
    float offsets[CALIBRATION_POINTS];
    for (int i = 0; i < CALIBRATION_POINTS; i++) {
        offsets[i] = points[i].offset;
    }
    
    St1xFlash_SaveCalibrationData(offsets, CALIBRATION_POINTS);
}

/**
 * @brief 检查系统是否激活
 */
uint8_t CalibrationSystem_IsActive(void) {
    return (system_state != 0);
}