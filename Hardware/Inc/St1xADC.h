//
// Created by leonm on 2023/2/13.
//

#ifndef ST1XADC_H
#define ST1XADC_H
#include "u8g2.h"
#include "stm32f1xx_hal.h"

void DMA_ADC_TEST(u8g2_t *u8g2);
void Display_ADC(u8g2_t *u8g2);
void TEMP(u8g2_t *u8g2,TIM_HandleTypeDef *htim);
void DrawProcess(u8g2_t *u8g2);
void button(void);

// PWM �źſ��� ADC ����
void controlADCSampling(TIM_HandleTypeDef *htim);
// ��ֵ�˲�
void applyMeanFilterAndRemoveOutliers();
void calculateVoltageAndPrint();
// �� OLED ��Ļ�ϻ�����Ϣ
void drawOnOLED(u8g2_t *u8g2);

// T12����ͷ�¶ȿ�����غ���
float calculateT12Temperature(uint16_t adcValue);
void controlT12ADCSampling(TIM_HandleTypeDef *htim);
float pidTemperatureControl(float current_temp);
void setT12Temperature(float temperature);
void t12TemperatureControlLoop(void);

// PID�������ṹ������
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