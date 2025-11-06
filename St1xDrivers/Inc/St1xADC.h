/**
 * @file    St1xADC.h
 * @brief   T12烙铁控制器 - 温度测量头文件
 * @author  Leon Red
 * @date    2023-02-13
 * @version 1.0
 * 
 * @details 这个文件是用来处理温度测量的部分，就像一个温度计
 *          主要功能：
 *          1. 读取温度传感器的数据
 *          2. 把传感器数据转换成温度值（比如320度）
 *          3. 让温度读数更稳定，不会跳来跳去
 *          4. 检查是否安全（电压够不够，温度会不会太高）
 *          5. 在屏幕上显示温度等信息
 */

#ifndef ST1XADC_H
#define ST1XADC_H

#include "u8g2.h"
#include "stm32f1xx_hal.h"

// ==================== 数据结构定义 ====================

/**
 * @brief 温度传感器校准参数
 * @details 用来调整温度测量准确性的参数
 */
typedef struct {
    float offset;       /**< 温度偏移量 - 调整零点误差 */
    float gain;         /**< 增益系数 - 调整灵敏度 */
    uint16_t adc_zero;  /**< 零点ADC值 - 0°C时的读数 */
    uint16_t adc_span;  /**< 满量程ADC值 - 最高温度时的读数 */
} TemperatureCalibration;

// ==================== 全局变量声明 ====================

/** @brief 加热状态 - 0表示没加热，1表示正在加热 */
extern uint8_t heating_status;

/** @brief OLED屏幕显示对象 */
extern u8g2_t u8g2;

// ==================== 核心功能函数声明 ====================

/**
 * @brief 把传感器读数转换成温度值
 * @param adcValue 传感器读数（数字）
 * @return 温度值（摄氏度）
 * @details 就像把电压值换算成温度值
 */
float calculateT12Temperature(uint16_t adcValue);

// ==================== 数据处理函数声明 ====================

/**
 * @brief 获取稳定的温度值
 * @return 平均后的温度值
 * @details 多次测量取平均，让温度显示更稳定
 */
float getFilteredTemperature(void);

/**
 * @brief 更新温度平均值
 * @param adcValue 新的传感器读数
 * @details 把新读数加入到平均计算中
 */
void updateTemperatureFilter(uint16_t adcValue);

// ==================== 辅助功能函数声明 ====================

/**
 * @brief 选择合适的时间读取传感器
 * @param htim 定时器
 * @details 避免在加热时读数，减少干扰
 */
void controlADCSampling(TIM_HandleTypeDef *htim);

/**
 * @brief 在屏幕上显示信息
 * @param u8g2 屏幕对象
 * @details 显示温度、电压等信息
 */
void drawOnOLED(u8g2_t *u8g2);

// ==================== 安全检测函数声明 ====================

/**
 * @brief 检查USB电压够不够
 * @return 1够用，0不够用
 * @details 电压不够就不能加热，防止损坏设备
 */
uint8_t isUSBVoltageSufficient(void);

/**
 * @brief 检查USB电压并保护设备
 * @return 1正常，0异常
 * @details 电压异常时自动停止加热
 */
uint8_t checkUSBVoltage(void);

/**
 * @brief 检查温度是否安全
 * @return 1安全，0不安全
 * @details 防止温度过高损坏设备或造成危险
 */
uint8_t checkTemperatureSafety(void);

#endif