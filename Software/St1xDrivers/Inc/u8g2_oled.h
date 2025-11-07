//
// Created by leonm on 2023/2/11.
//

#ifndef ST1X_U8G2_OLED_H
#define ST1X_U8G2_OLED_H

// 包含U8g2图形库头文件
#include "u8g2.h"

// 延迟函数声明，用于OLED显示控制
uint8_t u8x8_delay(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr);

// STM32 GPIO和延迟控制函数声明
uint8_t u8x8_stm32_gpio_and_delay(U8X8_UNUSED u8x8_t *u8x8,U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int,U8X8_UNUSED void *arg_ptr);

// I2C通信字节传输函数声明
uint8_t u8x8_byte_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

// SPI接口OLED初始化函数声明
void spi_oled_Init(u8g2_t *u8g2);

// I2C接口OLED初始化函数声明
void I2C_oled_Init(u8g2_t *u8g2);

// 绘制基本图形示例函数声明
void draw(u8g2_t *u8g2);

// 像素填充屏幕测试函数声明
void testDrawPixelToFillScreen(u8g2_t *u8g2);

// 进度条绘制测试函数声明
void testDrawProcess(u8g2_t *u8g2);

// 字体显示测试函数声明
void testShowFont(u8g2_t *u8g2);

// 矩形框绘制测试函数声明
void testDrawFrame(u8g2_t *u8g2);

// 圆角矩形绘制测试函数声明
void testDrawRBox(u8g2_t *u8g2);

// 圆形绘制测试函数声明
void testDrawCircle(u8g2_t *u8g2);

// 实心椭圆绘制测试函数声明
void testDrawFilledEllipse(u8g2_t *u8g2);

// XBM图像格式绘制测试函数声明
void testDrawXBM(u8g2_t *u8g2);

// 所有绘图测试的集合函数声明
void u8g2DrawTest(u8g2_t *u8g2);

// 多种图形组合绘制测试函数声明
void testDrawMulti(u8g2_t *u8g2);

#endif //ST1X_U8G2_OLED_H