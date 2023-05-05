//
// Created by Leon on 2023/5/5.
//

#ifndef ST1X_MENU_H
#define ST1X_MENU_H
#include "main.h"
#include "u8g2.h"

#define Up HAL_GPIO_ReadPin(KEY_UP_GPIO_Port,KEY_UP_Pin)
#define Mode HAL_GPIO_ReadPin(KEY_MODE_GPIO_Port,KEY_MODE_Pin)
#define Down HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port,KEY_DOWN_Pin)

#define uchar unsigned char

typedef struct
{
    uchar current;
    uchar up;//向上翻索引号
    uchar down;//向下翻索引号
    uchar enter;//确认索引号
    void (*current_operation)();
} key_table;

extern key_table table[30];

extern u8g2_t u8g2;

extern void MainMenu();
extern void fun_b1();
extern void fun_c1();
extern void fun_d1();

extern void fun_a21();
extern void fun_a22();
extern void fun_a23();
extern void fun_a24();

extern void fun_b21();
extern void fun_b22();
extern void fun_b23();
extern void fun_b24();

extern void fun_c21();
extern void fun_c22();
extern void fun_c23();
extern void fun_c24();

extern void fun_a31();
extern void fun_a32();
extern void fun_a33();

extern void fun_b31();
extern void fun_b32();
extern void fun_b33();

extern void fun_c31();
extern void Volume();
extern void fun_c33();

extern void Welcome();

void menu_init(void);

#endif //ST1X_MENU_H
