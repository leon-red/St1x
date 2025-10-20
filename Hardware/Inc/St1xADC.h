//
// Created by leonm on 2023/2/13.
//

#ifndef ST1XADC_H
#define ST1XADC_H
#include "u8g2.h"
#include "stm32f1xx_hal.h"

extern u8g2_t u8g2;

void DMA_ADC_TEST(u8g2_t *u8g2);
void Display_ADC(u8g2_t *u8g2);
void TEMP(u8g2_t *u8g2,TIM_HandleTypeDef *htim);
void DrawProcess(u8g2_t *u8g2);
void button(void);

// PWM 信号控制 ADC 采样
void controlADCSampling(TIM_HandleTypeDef *htim);
// 均值滤波
void applyMeanFilterAndRemoveOutliers();
void calculateVoltageAndPrint();
// 在 OLED 屏幕上绘制信息
void drawOnOLED(u8g2_t *u8g2);

// T12烙铁头温度控制相关函数
float calculateT12Temperature(uint16_t adcValue);
void controlT12ADCSampling(TIM_HandleTypeDef *htim);
float pidTemperatureControl(float current_temp);
void setT12Temperature(float temperature);
void t12TemperatureControlLoop(void);


// PID控制器结构体定义
typedef struct {
    float kp;
    float ki;  
    float kd;
    float setpoint;
    float integral;
    float prev_error;
    uint32_t last_time;
} PID_Controller;

extern PID_Controller t12_pid;

#endif