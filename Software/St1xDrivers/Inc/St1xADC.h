/*
 * St1xADC.h
 *
 *  Created on: 2023年2月13日
 *      Author: Administrator
 */

#ifndef ST1XADC_H_
#define ST1XADC_H_

#include "adc.h"
#include "St1xPID.h"
#include "tim.h"

// 前向声明u8g2_t结构体，避免包含整个u8g2.h头文件
struct u8g2_struct;
typedef struct u8g2_struct u8g2_t;

// 温度校准参数结构体
typedef struct {
    float offset;
    float scale;
    uint16_t min_adc;
    uint16_t max_adc;
} TemperatureCalibration;

// 函数声明
float calculateT12Temperature(uint16_t adcValue);
void updateTemperatureFilter(uint16_t adcValue);
uint8_t isUSBVoltageSufficient(void);
uint8_t checkUSBVoltage(void);
uint8_t checkTemperatureSafety(void);
void controlADCSampling(TIM_HandleTypeDef *htim);
void drawOnOLED(u8g2_t *u8g2);
void updateDisplayTemperatureFilter(uint16_t adcValue);
float getDisplayFilteredTemperature(void);

// 系统状态监控函数
void systemStatusMonitor(void);

// 温度计算方式选择宏定义
// 设置为1使用9点校准插值计算，设置为0使用原有线性计算
#define USE_9POINT_CALIBRATION 1

// 温度限制常量定义
#define NORMAL_TEMPERATURE_LIMIT 550.0f  // 正常模式下的温度限制
#define CALIBRATION_TEMPERATURE_LIMIT 650.0f  // 校准模式下的温度限制

// 环境温度获取相关常量
#define CHIP_TEMP_V25 1.43f         // STM32F1内部温度传感器在25°C时的电压(V)
#define CHIP_TEMP_AVG_SLOPE 4.3f    // STM32F1内部温度传感器平均斜率(mV/°C)

// ==================== 温度传感器参数宏定义（供外部调用） ====================
#define THERMAL_VOLTAGE_PARAMETER 0.0048f  // 热电偶电压-温度转换系数（mV/°C）
#define ADC_REFERENCE_VOLTAGE 3.3f         // ADC参考电压
#define ADC_MAX_VALUE 4095                 // ADC最大值

// 全局温度限制变量（可在校准模式下临时修改）
extern float max_temperature_limit;

// 加热状态变量声明
extern uint8_t heating_status;

// DMA ADC数据缓冲区声明
extern uint16_t DMA_ADC[3];

// 环境温度相关变量声明
extern float ambient_temperature;

// 控制温度滤波变量声明
extern float filtered_temperature;

// 目标温度变量声明
extern float target_temperature;

// 系统状态标志声明
extern uint8_t adc_sampling_flag;
extern uint8_t heating_control_enabled;

// 采样状态机变量声明
extern uint8_t sampling_phase;
extern uint32_t sample_start_time;
extern uint16_t saved_pwm_value;

// 时间记录变量声明
extern uint32_t last_control_time;
extern uint32_t heating_start_time;

// 环境温度获取函数声明
float getChipInternalTemperature(void);
void updateAmbientTemperatureFilter(void);
void initializeColdJunctionTemperature(void);

// ==================== 校准系统接口函数声明 ====================

/**
 * setCalibrationTemperature - 设置校准目标温度
 * 功能：为校准系统设置目标温度值
 * @param temperature 目标温度值（°C）
 */
void setCalibrationTemperature(float temperature);

/**
 * isUSBVoltageSufficient - USB电压检测（校准系统专用）
 * 功能：检测USB输入电压是否满足加热要求
 * @return 1=电压足够（≥15V），0=电压不足
 */
uint8_t isUSBVoltageSufficient(void);

/**
 * scanCalibrationKeys - 扫描校准按键
 * 功能：检测校准模式下的按键操作
 * @return 按键状态（0=无按键，1=温度+，2=温度-，3=确认，4=取消）
 */
//uint8_t scanCalibrationKeys(void);

/**
 * saveCalibrationData - 保存校准数据
 * 功能：将校准结果保存到非易失存储器
 * @param offsets 温度偏移量数组
 * @param count 数据点数量
 */
//void saveCalibrationData(float* offsets, uint8_t count);

/**
 * StopCalibrationHeating - 停止校准加热
 * 功能：在校准模式下停止加热控制
 */
void StopCalibrationHeating(void);

/**
 * StartCalibrationHeating - 启动校准加热
 * 功能：在校准模式下启动加热控制
 */
void StartCalibrationHeating(void);

/**
 * SyncPIDParametersFromMainSystem - 同步PID参数
 * 功能：从主系统同步PID参数到校准系统
 */
void SyncPIDParametersFromMainSystem(void);

// 9点校准插值计算相关函数声明
void selectVoltageCalculationMethod(uint8_t method);
uint8_t getCurrentVoltageCalculationMethod(void);
void setCalibrationPoint(uint8_t index, uint16_t adc_value, float temperature);
void getCalibrationPoint(uint8_t index, uint16_t* adc_value, float* temperature);
float getCalibrationOffset(uint8_t index);
void reloadCalibrationData(void);
float calculateT12TemperatureEnhanced(uint16_t adcValue);

// 环境温度传感器配置
#define EXTERNAL_TEMP_SENSOR_ENABLED 0  // 设置为1启用外部温度传感器
#define ENVIRONMENT_TEMP_SAMPLING_INTERVAL 30000  // 环境温度采样间隔(ms)

// 环境温度传感器类型定义
typedef enum {
    TEMP_SENSOR_CHIP_INTERNAL = 0,  // 使用芯片内部温度传感器
    TEMP_SENSOR_EXTERNAL_DS18B20,   // 使用外部DS18B20传感器
    TEMP_SENSOR_EXTERNAL_NTC        // 使用外部NTC热敏电阻
} TempSensorType;

// 环境温度传感器配置结构体
typedef struct {
    TempSensorType sensor_type;      // 传感器类型
    float current_temperature;       // 当前温度值
    float filtered_temperature;      // 滤波后温度值
    uint32_t last_sample_time;      // 上次采样时间
    uint8_t sensor_available;        // 传感器是否可用
} EnvironmentSensor;

// 新增环境温度相关函数声明
void initEnvironmentTemperatureSensor(void);
float getRealEnvironmentTemperature(void);
void updateEnvironmentTemperatureFilter(void);
float getEstimatedEnvironmentTemperature(void);
void setEnvironmentSensorType(TempSensorType sensor_type);
TempSensorType getCurrentEnvironmentSensorType(void);
uint8_t isExternalTempSensorAvailable(void);

// 增强的温度计算函数声明
float calculateT12TemperatureEnhancedWithEnvCompensation(uint16_t adcValue);
void selectVoltageCalculationMethod(uint8_t method);
uint8_t getCurrentVoltageCalculationMethod(void);
void setCalibrationPoint(uint8_t index, uint16_t adc_value, float temperature);
void getCalibrationPoint(uint8_t index, uint16_t* adc_value, float* temperature);
float getCalibrationOffset(uint8_t index);
void reloadCalibrationData(void);
float calculateT12TemperatureEnhanced(uint16_t adcValue);

#endif /* ST1XADC_H_ */