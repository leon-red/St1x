//
// Created by Leon on 2023/5/5.
//
//参考代码：https://gitee.com/xxpcb/stm32_useful_demo/tree/master/03_stm32_%E5%A4%9A%E7%BA%A7%E8%8F%9C%E5%8D%95

#include "menu.h"
#include "main.h"
#include "u8g2.h"
#include "St1xADC.h"
#include "key.h"

u8g2_t u8g2;

key_table table[30] =
        {
                //0
                {0,  0,  0,  1,  (*Welcome)},

                //第1层
                {1,  4,  2,  5,  (*MainMenu)},
                {2,  1,  3,  9,  (*fun_b1)},
                {3,  2,  4,  13, (*fun_c1)},
                {4,  3,  1,  0,  (*fun_d1)},

                //第2层
                {5,  8,  6,  17, (*fun_a21)},
                {6,  5,  7,  18, (*fun_a22)},
                {7,  6,  8,  19, (*fun_a23)},
                {8,  7,  5,  1,  (*fun_a24)},

                {9,  12, 10, 20, (*fun_b21)},
                {10, 9,  11, 21, (*fun_b22)},
                {11, 10, 12, 22, (*fun_b23)},
                {12, 11, 9,  2,  (*fun_b24)},

                {13, 16, 14, 23, (*fun_c21)},
                {14, 13, 15, 24, (*fun_c22)},
                {15, 14, 16, 25, (*fun_c23)},
                {16, 15, 13, 3,  (*fun_c24)},

                //第3层
                {17, 17, 17, 5,  (*fun_a31)},
                {18, 18, 18, 6,  (*fun_a32)},
                {19, 19, 19, 7,  (*fun_a33)},

                {20, 20, 20, 9,  (*fun_b31)},
                {21, 21, 21, 10, (*fun_b32)},
                {22, 22, 22, 11, (*fun_b33)},

                {23, 23, 23, 13, (*fun_c31)},
                {24, 24, 24, 14, (*Volume)},
                {25, 25, 25, 15, (*fun_c33)},
        };

/*********1***********/
void MainMenu()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,">");
    u8g2_DrawStr(&u8g2,16,16,"Weather");
    u8g2_DrawStr(&u8g2,16,32,"Music");
    u8g2_DrawStr(&u8g2,16,48,"Device Info");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_b1()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,32,">");
    u8g2_DrawStr(&u8g2,16,16,"Weather");
    u8g2_DrawStr(&u8g2,16,32,"Music");
    u8g2_DrawStr(&u8g2,16,48,"Device Info");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_c1()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,48,">");
    u8g2_DrawStr(&u8g2,16,16,"Weather");
    u8g2_DrawStr(&u8g2,16,32,"Music");
    u8g2_DrawStr(&u8g2,16,48,"Device Info");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_d1()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,64,">");
    u8g2_DrawStr(&u8g2,16,16,"Weather");
    u8g2_DrawStr(&u8g2,16,32,"Music");
    u8g2_DrawStr(&u8g2,16,48,"Device Info");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

/*********2***********/
void fun_a21()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,">");
    u8g2_DrawStr(&u8g2,16,16,"HangZhou");
    u8g2_DrawStr(&u8g2,16,32,"BeiJing");
    u8g2_DrawStr(&u8g2,16,48,"ShangHai");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_a22()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,32,">");
    u8g2_DrawStr(&u8g2,16,16,"HangZhou");
    u8g2_DrawStr(&u8g2,16,32,"BeiJing");
    u8g2_DrawStr(&u8g2,16,48,"ShangHai");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_a23()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,48,">");
    u8g2_DrawStr(&u8g2,16,16,"HangZhou");
    u8g2_DrawStr(&u8g2,16,32,"BeiJing");
    u8g2_DrawStr(&u8g2,16,48,"ShangHai");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_a24()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,64,">");
    u8g2_DrawStr(&u8g2,16,16,"HangZhou");
    u8g2_DrawStr(&u8g2,16,32,"BeiJing");
    u8g2_DrawStr(&u8g2,16,48,"ShangHai");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_b21()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,">");
    u8g2_DrawStr(&u8g2,16,16,"WindyHill");
    u8g2_DrawStr(&u8g2,16,32,"New Boy");
    u8g2_DrawStr(&u8g2,16,48,"Kiss The Rain");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_b22()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,32,">");
    u8g2_DrawStr(&u8g2,16,16,"WindyHill");
    u8g2_DrawStr(&u8g2,16,32,"New Boy");
    u8g2_DrawStr(&u8g2,16,48,"Kiss The Rain");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_b23()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,48,">");
    u8g2_DrawStr(&u8g2,16,16,"WindyHill");
    u8g2_DrawStr(&u8g2,16,32,"New Boy");
    u8g2_DrawStr(&u8g2,16,48,"Kiss The Rain");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_b24()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,64,">");
    u8g2_DrawStr(&u8g2,16,16,"WindyHill");
    u8g2_DrawStr(&u8g2,16,32,"New Boy");
    u8g2_DrawStr(&u8g2,16,48,"Kiss The Rain");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_c21()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,">");
    u8g2_DrawStr(&u8g2,16,16,"WiFi Info");
    u8g2_DrawStr(&u8g2,16,32,"Volume");
    u8g2_DrawStr(&u8g2,16,48,"Version");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_c22()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,32,">");
    u8g2_DrawStr(&u8g2,16,16,"WiFi Info");
    u8g2_DrawStr(&u8g2,16,32,"Volume");
    u8g2_DrawStr(&u8g2,16,48,"Version");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_c23()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,48,">");
    u8g2_DrawStr(&u8g2,16,16,"WiFi Info");
    u8g2_DrawStr(&u8g2,16,32,"Volume");
    u8g2_DrawStr(&u8g2,16,48,"Version");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

void fun_c24()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,64,">");
    u8g2_DrawStr(&u8g2,16,16,"WiFi Info");
    u8g2_DrawStr(&u8g2,16,32,"Volume");
    u8g2_DrawStr(&u8g2,16,48,"Version");
    u8g2_DrawStr(&u8g2,16,64,"return");
}

/*********3***********/
void fun_a31()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"HangZhou Weather");
    u8g2_DrawStr(&u8g2,0,32,"Sunny");
    u8g2_DrawStr(&u8g2,0,48,"");
    u8g2_DrawStr(&u8g2,0,64,"Enter to Return");
}

void fun_a32()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"BeiJing Weather");
    u8g2_DrawStr(&u8g2,0,32,"Sunny");
    u8g2_DrawStr(&u8g2,0,48,"");
    u8g2_DrawStr(&u8g2,0,64,"Enter to Return");
}

void fun_a33()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"ShangHai Weather");
    u8g2_DrawStr(&u8g2,0,32,"Sunny");
    u8g2_DrawStr(&u8g2,0,48,"");
    u8g2_DrawStr(&u8g2,0,64,"Enter to Return");
}

void fun_b31()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"*** Music ***");
    u8g2_DrawStr(&u8g2,0,32,"WindyHill");
    u8g2_DrawStr(&u8g2,0,48,"");
    u8g2_DrawStr(&u8g2,0,64,"Enter to Return");
}

void fun_b32()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"*** Music ***");
    u8g2_DrawStr(&u8g2,0,32,"New Boy");
    u8g2_DrawStr(&u8g2,0,48,"");
    u8g2_DrawStr(&u8g2,0,64,"Enter to Return");
}

void fun_b33()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"*** Music ***");
    u8g2_DrawStr(&u8g2,0,32,"Kill The Rain");
    u8g2_DrawStr(&u8g2,0,48,"");
    u8g2_DrawStr(&u8g2,0,64,"Enter to Return");
}

void fun_c31()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"WiFI info");
    u8g2_DrawStr(&u8g2,0,32,"ssid:xxx");
    u8g2_DrawStr(&u8g2,0,48,"passwd:xxx");
    u8g2_DrawStr(&u8g2,0,64,"Enter to Return");
}

void Volume()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"Volume info");
    u8g2_DrawStr(&u8g2,0,32,"volume:xx");
    u8g2_DrawStr(&u8g2,0,48,"");
    u8g2_DrawStr(&u8g2,0,64,"Enter to Return");
}

void fun_c33()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"Version info");
    u8g2_DrawStr(&u8g2,0,32,"version:V1.0.0");
    u8g2_DrawStr(&u8g2,0,48,"");
    u8g2_DrawStr(&u8g2,0,64,"Enter to Return");
}

/*********0***********/
void Welcome() {
    while (Mode == 1) {
//    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
//    u8g2_DrawStr(&u8g2,0,16,"STM32");
//    u8g2_DrawStr(&u8g2,0,32,"Multi Menu Test");
//    u8g2_DrawStr(&u8g2,0,48,"");
//    u8g2_DrawStr(&u8g2, 24, 80, "Enter to Test");
        Iron_PullUp();
        DMA_ADC_TEST(&u8g2);
    }
}

void (*current_operation_index)();
uchar func_index = 0; //初始显示欢迎界面
uchar last_index = 127; //last初始为无效值
void menu_init(void)
{
    if ((Up == 0) || (Down == 0) || (Mode == 0)) {
        HAL_Delay(10);//消抖
        if (Up == 0) {
            func_index = table[func_index].up;    //向上翻
            while (!Up);//松手检测
        }
        if (Down == 0) {
            func_index = table[func_index].down;    //向下翻
            while (!Down);
        }
        if (Mode == 0) {
            func_index = table[func_index].enter;    //确认
            while (!Mode);
        }
    }

    if (func_index != last_index) {
        current_operation_index = table[func_index].current_operation;

        u8g2_ClearBuffer(&u8g2);
        (*current_operation_index)();//执行当前操作函数
        u8g2_SendBuffer(&u8g2);

        last_index = func_index;
    }
}

