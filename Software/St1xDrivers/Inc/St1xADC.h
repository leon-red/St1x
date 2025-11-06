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

#endif /* ST1XADC_H_ */