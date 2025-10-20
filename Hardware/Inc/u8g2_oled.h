//
// Created by leonm on 2023/2/11.
//

#ifndef ST1X_U8G2_OLED_H
#define ST1X_U8G2_OLED_H
#include "u8g2.h"

uint8_t u8x8_delay(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr);
uint8_t u8x8_stm32_gpio_and_delay(U8X8_UNUSED u8x8_t *u8x8,U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int,U8X8_UNUSED void *arg_ptr);
uint8_t u8x8_byte_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
void spi_oled_Init(u8g2_t *u8g2);
void I2C_oled_Init(u8g2_t *u8g2);
void draw(u8g2_t *u8g2);
void testDrawPixelToFillScreen(u8g2_t *u8g2);
void testDrawProcess(u8g2_t *u8g2);
void testShowFont(u8g2_t *u8g2);
void testDrawFrame(u8g2_t *u8g2);
void testDrawRBox(u8g2_t *u8g2);
void testDrawCircle(u8g2_t *u8g2);
void testDrawFilledEllipse(u8g2_t *u8g2);
void testDrawXBM(u8g2_t *u8g2);
void u8g2DrawTest(u8g2_t *u8g2);
void testDrawMulti(u8g2_t *u8g2);

#endif //ST1X_U8G2_OLED_H
