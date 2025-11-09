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
float getFilteredTemperature(void);
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

// 温度限制常量定义
#define NORMAL_TEMPERATURE_LIMIT 460.0f  // 正常模式下的温度限制
#define CALIBRATION_TEMPERATURE_LIMIT 500.0f  // 校准模式下的温度限制

// 环境温度获取相关常量
#define ADC_SAMPLES_FOR_DENOISE 32  // ADC去噪采样次数
#define CHIP_TEMP_V25 1.43f         // STM32F1内部温度传感器在25°C时的电压(V)
#define CHIP_TEMP_AVG_SLOPE 4.3f    // STM32F1内部温度传感器平均斜率(mV/°C)
#define VREFINT_CAL 1.21f           // 内部参考电压校准值(V)

// 全局温度限制变量（可在校准模式下临时修改）
extern float max_temperature_limit;

// 加热状态变量声明
extern uint8_t heating_status;

// DMA ADC数据缓冲区声明
extern uint16_t DMA_ADC[3];

// 环境温度获取函数声明
float getChipInternalTemperature(void);
float getFilteredChipTemperature(void);
float getAmbientTemperatureEstimate(void);
void updateAmbientTemperatureFilter(void);
void initializeColdJunctionTemperature(void);

// 环境温度相关变量声明
extern float ambient_temperature;
extern float chip_temperature_filtered;

#endif /* ST1XADC_H_ */