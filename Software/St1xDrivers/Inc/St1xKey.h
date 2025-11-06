#ifndef __ST1X_KEY_H
#define __ST1X_KEY_H

#include "gpio.h"
#include "usart.h"
#include "St1xADC.h"
#include "St1xPID.h"
#include "tim.h"
#include "St1xMenu.h"
#include <stdio.h>

// 按键枚举类型
typedef enum {
    KEY_NONE = 0,
    KEY_UP = 1,
    KEY_DOWN = 2,
    KEY_MODE = 3,
    KEY_MODE_LONG = 4
} KeyType;

// 函数声明
void Key_Init(void);
KeyType Key_Scan(void);
uint8_t Key_GetNum_New(void);

// 外部变量声明
extern float target_temperature;

// 按键处理函数
void handleHeatingControl(void);
void handleTemperatureAdjust(int direction);
void handleMainTemperatureAdjust(KeyType key);
void handleMenuKey(void);

#endif