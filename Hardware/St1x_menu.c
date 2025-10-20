//
// Created by Leon on 2023/5/5.
//
//�ο����룺https://gitee.com/xxpcb/stm32_useful_demo/tree/master/03_stm32_%E5%A4%9A%E7%BA%A7%E8%8F%9C%E5%8D%95

#include <stdlib.h>
#include "St1x_menu.h"
#include "main.h"
#include "u8g2.h"
#include "St1xADC.h"
#include "St1x_key.h"
#include "tim.h"

u8g2_t u8g2;

key_table table[43] =
        {
                 //0
                 {0, 0, 0, 1, (*Welcome)},
                 //1
                 {1, 5, 2, 6, (*MainMenu1)},
                 {2, 1, 3, 11,(*MainMenu2)},
                 {3, 2, 4, 16,(*MainMenu3)},
                 {4, 3, 5, 21,(*MainMenu4)},
                 {5, 4, 1, 0, (*MainMenu5)},
                 //1-1
                 {6, 10,7, 26,(*fun_a21)},
                 {7, 6, 8, 27,(*fun_a22)},
                 {8, 7, 9, 28,(*fun_a23)},
                 {9, 8, 10,29,(*fun_a24)},
                {10,9, 6, 1, (*fun_a25)},
                //1-2
                {11,15,12,31,(*fun_b21)},
                {12,11,13,32,(*fun_b22)},
                {13,12,14,33,(*fun_b23)},
                {14,13,15,0, (*fun_b24)},
                {15,14,11,1, (*fun_b25)},
                //1-3
                {16,20,17,36,(*fun_c21)},
                {17,16,18,37,(*fun_c22)},
                {18,17,19,38,(*fun_c23)},
                {19,18,20,0, (*fun_c24)},
                {20,19,16,1, (*fun_c25)},
                //1-4
                {21,25,22,41,(*fun_d21)},
                {22,21,23,42,(*fun_d22)},
                {23,22,24,43,(*fun_d23)},
                {24,23,25,0, (*fun_d24)},
                {25,24,21,1, (*fun_d25)},
                //1-1-1
                {26,30,27,31,(*fun_e21)},
                {27,26,28,37,(*fun_e22)},
                {28,27,29,43,(*fun_e23)},
                {29,28,30,0, (*fun_e24)},
                {30,29,26,6, (*fun_e25)},
                //1-2-1
                {31,33,32,5, (*fun_a31)},
                {32,31,33,6, (*fun_a32)},
                {33,32,31,7, (*fun_a33)},
                //1-3-1
                {36,38,37,9, (*fun_b31)},
                {37,36,38,10,(*fun_b32)},
                {38,37,36,16,(*fun_b33)},
                //1-4-1
                {41,43,42,13,(*fun_c31)},
                {42,41,43,14,(*Volume)},
                {43,42,41,21,(*fun_c33)},
        };

/*********1***********/
void MainMenu1()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 16,">");
    u8g2_DrawStr(&u8g2,16,16,"MainMenu 1");
    u8g2_DrawStr(&u8g2,16,32,"MainMenu 2");
    u8g2_DrawStr(&u8g2,16,48,"MainMenu 3");
    u8g2_DrawStr(&u8g2,16,64,"MainMenu 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to Welcome");
}

void MainMenu2()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 32,">");
    u8g2_DrawStr(&u8g2,16,16,"MainMenu 1");
    u8g2_DrawStr(&u8g2,16,32,"MainMenu 2");
    u8g2_DrawStr(&u8g2,16,48,"MainMenu 3");
    u8g2_DrawStr(&u8g2,16,64,"MainMenu 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to Welcome");
}

void MainMenu3()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 48,">");
    u8g2_DrawStr(&u8g2,16,16,"MainMenu 1");
    u8g2_DrawStr(&u8g2,16,32,"MainMenu 2");
    u8g2_DrawStr(&u8g2,16,48,"MainMenu 3");
    u8g2_DrawStr(&u8g2,16,64,"MainMenu 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to Welcome");
}

void MainMenu4()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 64,">");
    u8g2_DrawStr(&u8g2,16,16,"MainMenu 1");
    u8g2_DrawStr(&u8g2,16,32,"MainMenu 2");
    u8g2_DrawStr(&u8g2,16,48,"MainMenu 3");
    u8g2_DrawStr(&u8g2,16,64,"MainMenu 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to Welcome");
}

void MainMenu5()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 80,">");
    u8g2_DrawStr(&u8g2,16,16,"MainMenu 1");
    u8g2_DrawStr(&u8g2,16,32,"MainMenu 2");
    u8g2_DrawStr(&u8g2,16,48,"MainMenu 3");
    u8g2_DrawStr(&u8g2,16,64,"MainMenu 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to Welcome");
}
/*********1-1***********/
void fun_a21()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 16,">");
    u8g2_DrawStr(&u8g2,16,16,"a21 1");
    u8g2_DrawStr(&u8g2,16,32,"a21 2");
    u8g2_DrawStr(&u8g2,16,48,"a21 3");
    u8g2_DrawStr(&u8g2,16,64,"a21 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_a22()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 32,">");
    u8g2_DrawStr(&u8g2,16,16,"a22 1");
    u8g2_DrawStr(&u8g2,16,32,"a22 2");
    u8g2_DrawStr(&u8g2,16,48,"a22 3");
    u8g2_DrawStr(&u8g2,16,64,"a22 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_a23()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 48,">");
    u8g2_DrawStr(&u8g2,16,16,"a23 1");
    u8g2_DrawStr(&u8g2,16,32,"a23 2");
    u8g2_DrawStr(&u8g2,16,48,"a23 3");
    u8g2_DrawStr(&u8g2,16,64,"a23 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_a24()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 64,">");
    u8g2_DrawStr(&u8g2,16,16,"a24 1");
    u8g2_DrawStr(&u8g2,16,32,"a24 2");
    u8g2_DrawStr(&u8g2,16,48,"a24 3");
    u8g2_DrawStr(&u8g2,16,64,"a24 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_a25()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 80,">");
    u8g2_DrawStr(&u8g2,16,16,"a25 1");
    u8g2_DrawStr(&u8g2,16,32,"a25 2");
    u8g2_DrawStr(&u8g2,16,48,"a25 3");
    u8g2_DrawStr(&u8g2,16,64,"a25 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

/*********1-2***********/
void fun_b21()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 16,">");
    u8g2_DrawStr(&u8g2,16,16,"b21 1");
    u8g2_DrawStr(&u8g2,16,32,"b21 2");
    u8g2_DrawStr(&u8g2,16,48,"b21 3");
    u8g2_DrawStr(&u8g2,16,64,"b21 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_b22()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 32,">");
    u8g2_DrawStr(&u8g2,16,16,"b22 1");
    u8g2_DrawStr(&u8g2,16,32,"b22 2");
    u8g2_DrawStr(&u8g2,16,48,"b22 3");
    u8g2_DrawStr(&u8g2,16,64,"b22 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_b23()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 48,">");
    u8g2_DrawStr(&u8g2,16,16,"b23 1");
    u8g2_DrawStr(&u8g2,16,32,"b23 2");
    u8g2_DrawStr(&u8g2,16,48,"b23 3");
    u8g2_DrawStr(&u8g2,16,64,"b23 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_b24()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 64,">");
    u8g2_DrawStr(&u8g2,16,16,"b24 1");
    u8g2_DrawStr(&u8g2,16,32,"b24 2");
    u8g2_DrawStr(&u8g2,16,48,"b24 3");
    u8g2_DrawStr(&u8g2,16,64,"b24 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_b25()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 80,">");
    u8g2_DrawStr(&u8g2,16,16,"b25 1");
    u8g2_DrawStr(&u8g2,16,32,"b25 2");
    u8g2_DrawStr(&u8g2,16,48,"b25 3");
    u8g2_DrawStr(&u8g2,16,64,"b25 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

/*********1-3***********/
void fun_c21()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 16,">");
    u8g2_DrawStr(&u8g2,16,16,"c21 1");
    u8g2_DrawStr(&u8g2,16,32,"c21 2");
    u8g2_DrawStr(&u8g2,16,48,"c21 3");
    u8g2_DrawStr(&u8g2,16,64,"c21 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_c22()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 32,">");
    u8g2_DrawStr(&u8g2,16,16,"c22 1");
    u8g2_DrawStr(&u8g2,16,32,"c22 2");
    u8g2_DrawStr(&u8g2,16,48,"c22 3");
    u8g2_DrawStr(&u8g2,16,64,"c22 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_c23()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 48,">");
    u8g2_DrawStr(&u8g2,16,16,"c23 1");
    u8g2_DrawStr(&u8g2,16,32,"c23 2");
    u8g2_DrawStr(&u8g2,16,48,"c23 3");
    u8g2_DrawStr(&u8g2,16,64,"c23 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_c24()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 64,">");
    u8g2_DrawStr(&u8g2,16,16,"c24 1");
    u8g2_DrawStr(&u8g2,16,32,"c24 2");
    u8g2_DrawStr(&u8g2,16,48,"c24 3");
    u8g2_DrawStr(&u8g2,16,64,"c24 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_c25()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 80,">");
    u8g2_DrawStr(&u8g2,16,16,"c25 1");
    u8g2_DrawStr(&u8g2,16,32,"c25 2");
    u8g2_DrawStr(&u8g2,16,48,"c25 3");
    u8g2_DrawStr(&u8g2,16,64,"c25 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

/*********1-4***********/
void fun_d21()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 16,">");
    u8g2_DrawStr(&u8g2,16,16,"d21 1");
    u8g2_DrawStr(&u8g2,16,32,"d21 2");
    u8g2_DrawStr(&u8g2,16,48,"d21 3");
    u8g2_DrawStr(&u8g2,16,64,"d21 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_d22()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 32,">");
    u8g2_DrawStr(&u8g2,16,16,"d22 1");
    u8g2_DrawStr(&u8g2,16,32,"d22 2");
    u8g2_DrawStr(&u8g2,16,48,"d22 3");
    u8g2_DrawStr(&u8g2,16,64,"d22 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_d23()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 48,">");
    u8g2_DrawStr(&u8g2,16,16,"d23 1");
    u8g2_DrawStr(&u8g2,16,32,"d23 2");
    u8g2_DrawStr(&u8g2,16,48,"d23 3");
    u8g2_DrawStr(&u8g2,16,64,"d23 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_d24()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 64,">");
    u8g2_DrawStr(&u8g2,16,16,"d24 1");
    u8g2_DrawStr(&u8g2,16,32,"d24 2");
    u8g2_DrawStr(&u8g2,16,48,"d24 3");
    u8g2_DrawStr(&u8g2,16,64,"d24 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

void fun_d25()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 80,">");
    u8g2_DrawStr(&u8g2,16,16,"d25 1");
    u8g2_DrawStr(&u8g2,16,32,"d25 2");
    u8g2_DrawStr(&u8g2,16,48,"d25 3");
    u8g2_DrawStr(&u8g2,16,64,"d25 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to MainMenu");
}

/*********1-1-1***********/
void fun_e21()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 16,">");
    u8g2_DrawStr(&u8g2,16,16,"e21 1");
    u8g2_DrawStr(&u8g2,16,32,"e21 2");
    u8g2_DrawStr(&u8g2,16,48,"e21 3");
    u8g2_DrawStr(&u8g2,16,64,"e21 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to a21");
}

void fun_e22()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 32,">");
    u8g2_DrawStr(&u8g2,16,16,"e22 1");
    u8g2_DrawStr(&u8g2,16,32,"e22 2");
    u8g2_DrawStr(&u8g2,16,48,"e22 3");
    u8g2_DrawStr(&u8g2,16,64,"e22 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to a21");
}

void fun_e23()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 48,">");
    u8g2_DrawStr(&u8g2,16,16,"e23 1");
    u8g2_DrawStr(&u8g2,16,32,"e23 2");
    u8g2_DrawStr(&u8g2,16,48,"e23 3");
    u8g2_DrawStr(&u8g2,16,64,"e23 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to a21");
}

void fun_e24()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 64,">");
    u8g2_DrawStr(&u8g2,16,16,"e24 1");
    u8g2_DrawStr(&u8g2,16,32,"e24 2");
    u8g2_DrawStr(&u8g2,16,48,"e24 3");
    u8g2_DrawStr(&u8g2,16,64,"e24 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to a21");
}

void fun_e25()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0, 80,">");
    u8g2_DrawStr(&u8g2,16,16,"e25 1");
    u8g2_DrawStr(&u8g2,16,32,"e25 2");
    u8g2_DrawStr(&u8g2,16,48,"e25 3");
    u8g2_DrawStr(&u8g2,16,64,"e25 4");
    u8g2_DrawStr(&u8g2,16,80,"Enter to a21");
}

/*********1-2-1***********/
void fun_a31()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"a31 1");
    u8g2_DrawStr(&u8g2,0,32,"a31 2");
    u8g2_DrawStr(&u8g2,0,48,"a31 3");
    u8g2_DrawStr(&u8g2,0,64,"a31 4");
    u8g2_DrawStr(&u8g2,0,80,"Enter to Return");
}

void fun_a32()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"a32 1");
    u8g2_DrawStr(&u8g2,0,32,"a32 2");
    u8g2_DrawStr(&u8g2,0,48,"a32 3");
    u8g2_DrawStr(&u8g2,0,64,"a32 4");
    u8g2_DrawStr(&u8g2,0,80,"Enter to Return");
}

void fun_a33()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"a33 1");
    u8g2_DrawStr(&u8g2,0,32,"a33 2");
    u8g2_DrawStr(&u8g2,0,48,"a33 3");
    u8g2_DrawStr(&u8g2,0,64,"a33 4");
    u8g2_DrawStr(&u8g2,0,80,"Enter to Return");
}

/*********1-3-1***********/
void fun_b31()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"b31 1");
    u8g2_DrawStr(&u8g2,0,32,"b31 2");
    u8g2_DrawStr(&u8g2,0,48,"b31 3");
    u8g2_DrawStr(&u8g2,0,64,"b31 4");
    u8g2_DrawStr(&u8g2,0,80,"Enter to Return");
}

void fun_b32()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"b32 1");
    u8g2_DrawStr(&u8g2,0,32,"b32 2");
    u8g2_DrawStr(&u8g2,0,48,"b32 3");
    u8g2_DrawStr(&u8g2,0,64,"b32 4");
    u8g2_DrawStr(&u8g2,0,80,"Enter to Return");
}

void fun_b33()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"b33 1");
    u8g2_DrawStr(&u8g2,0,32,"b33 2");
    u8g2_DrawStr(&u8g2,0,48,"b33 3");
    u8g2_DrawStr(&u8g2,0,64,"b33 4");
    u8g2_DrawStr(&u8g2,0,80,"Enter to Return");
}

/*********1-4-1***********/
void fun_c31()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"c31 1");
    u8g2_DrawStr(&u8g2,0,32,"c31 2");
    u8g2_DrawStr(&u8g2,0,48,"c31 3");
    u8g2_DrawStr(&u8g2,0,64,"c31 4");
    u8g2_DrawStr(&u8g2,0,80,"Enter to Return");
}

void Volume()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"Volume 1");
    u8g2_DrawStr(&u8g2,0,32,"volume 2");
    u8g2_DrawStr(&u8g2,0,48,"Volume 3");
    u8g2_DrawStr(&u8g2,0,64,"Volume 4");
    u8g2_DrawStr(&u8g2,0,80,"Enter to Return");
}

void fun_c33()
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
    u8g2_DrawStr(&u8g2,0,16,"c33 1");
    u8g2_DrawStr(&u8g2,0,32,"c33 2");
    u8g2_DrawStr(&u8g2,0,48,"c33 3");
    u8g2_DrawStr(&u8g2,0,64,"c33 4");
    u8g2_DrawStr(&u8g2,0,80,"Enter to Return");
}



void (*current_operation_index)();
uchar func_index = 0; //��ʼ��ʾ��ӭ����
uchar last_index = 127; //last��ʼΪ��Чֵ
void menu_init(void)
{
    if ((Up == 0) || (Down == 0) || (Mode == 0)) {
        HAL_Delay(10);//����
        if (Up == 0) {
            func_index = table[func_index].up;    //���Ϸ�
            while (!Up);//���ּ��
        }
        if (Down == 0) {
            func_index = table[func_index].down;    //���·�
            while (!Down);
        }
        if (Mode == 0) {
            func_index = table[func_index].enter;    //ȷ��
            while (!Mode);
        }
    }

    if (func_index != last_index) {
        current_operation_index = table[func_index].current_operation;

        u8g2_ClearBuffer(&u8g2);
        (*current_operation_index)();//ִ�е�ǰ��������
        u8g2_SendBuffer(&u8g2);

        last_index = func_index;
    }
}

/*********0***********/
void Welcome() {
    while (Mode == 1) {
//    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tf);
//    u8g2_DrawStr(&u8g2,0,16,"STM32");
//    u8g2_DrawStr(&u8g2,0,32,"Multi Menu Test");
//    u8g2_DrawStr(&u8g2,0,48,"");
//    u8g2_DrawStr(&u8g2, 24, 80, "Enter to Test");
    DMA_ADC_TEST(&u8g2);
    Iron_PullUp();
    }
}