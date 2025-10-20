//
// Created by leonm on 2023/2/11.
// OLED驱动源码地址：github.com/olikraus/u8g2
//
//

#include "u8g2_oled.h"
#include "u8g2.h"
#include "i2c.h"
#include "stdio.h"
#include "spi.h"

/*******************************************I2C驱动*******************************************/
uint8_t u8x8_byte_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[32];  /* u8g2/u8x8 will never send more than 32 bytes between START_TRANSFER and END_TRANSFER */
    static uint8_t buf_idx;
    uint8_t *data;

    switch(msg)
    {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
            while( arg_int > 0 )
            {
                buffer[buf_idx++] = *data;
                data++;
                arg_int--;
            }
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_idx = 0;
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            HAL_I2C_Master_Transmit(&hi2c1, u8x8_GetI2CAddress(u8x8), buffer, buf_idx, 500);
//            HAL_I2C_Master_Transmit_DMA(&hi2c1,u8x8_GetI2CAddress(u8x8),buffer,buf_idx);
//            HAL_I2C_Master_Receive_DMA(&hi2c1,u8x8_GetI2CAddress(u8x8),buffer,buf_idx);
            break;
        default:
            return 0;
    }
    return 1;
}

/*******************************************SPI驱动*******************************************/

uint8_t u8x8_byte_4wire_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_BYTE_SEND: /*通过SPI发送arg_int个字节数据*/
            HAL_SPI_Transmit_DMA(&hspi2,(uint8_t *)arg_ptr,arg_int);
            while (hspi2.TxXferCount);
            break;
        case U8X8_MSG_BYTE_INIT: /*初始化函数*/
            break;
        case U8X8_MSG_BYTE_SET_DC: /*设置DC引脚,表明发送的是数据还是命令*/
            HAL_GPIO_WritePin(OLED_DC_GPIO_Port,OLED_DC_Pin,arg_int);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
            u8x8->gpio_and_delay_cb(u8x8,U8X8_MSG_DELAY_NANO,u8x8->display_info->post_chip_enable_wait_ns,NULL);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            u8x8->gpio_and_delay_cb(u8x8,U8X8_MSG_DELAY_NANO,u8x8->display_info->pre_chip_disable_wait_ns,NULL);
            u8x8_gpio_SetCS(u8x8,u8x8->display_info->chip_disable_level);
            break;
        default:
            return 0;
    }
    return 1;
}

/**********************************************OLED微秒延时函数**********************************************/
uint8_t u8x8_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int);
            break;
            //Function which delays 10us
        case U8X8_MSG_DELAY_10MICRO:
            for (uint16_t n = 0; n < 320; n++)
            {
                __NOP();
            }

            break;
            //Function which delays 100ns
        case U8X8_MSG_DELAY_100NANO:
            __NOP();
            break;
        default:
            return 0;
    }
    return 1;
}

uint8_t u8x8_stm32_gpio_and_delay(U8X8_UNUSED u8x8_t *u8x8,
                                  U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int,
                                  U8X8_UNUSED void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_GPIO_AND_DELAY_INIT: /*delay和GPIO的初始化，在main中已经初始化完成了*/
            break;
        case U8X8_MSG_DELAY_MILLI: /*延时函数*/
            HAL_Delay(arg_int);
            break;
        case U8X8_MSG_GPIO_CS: /*片选信号*/
            HAL_GPIO_WritePin(OLED_CS_GPIO_Port,OLED_CS_Pin,arg_int);
            break;
        case U8X8_MSG_GPIO_DC: /*设置DC引脚,表明发送的是数据还是命令*/
            HAL_GPIO_WritePin(OLED_DC_GPIO_Port,OLED_DC_Pin,arg_int);
            break;
        case U8X8_MSG_GPIO_RESET:
            break;
    }
    return 1;
}
/**********************************************OLED微秒延时函数**********************************************/

/************************************************OLED初始化************************************************/
void I2C_oled_Init(u8g2_t *u8g2)    //初始化_I2C驱动
{
    HAL_GPIO_WritePin(OLED_Reset_GPIO_Port,OLED_Reset_Pin,1);
    HAL_GPIO_WritePin(OLED_IM_GPIO_Port,OLED_IM_Pin,1);
    HAL_GPIO_WritePin(OLED_CS_GPIO_Port,OLED_CS_Pin,0);
    HAL_GPIO_WritePin(OLED_DC_GPIO_Port,OLED_DC_Pin,0);
    u8g2_Setup_sh1107_i2c_tk078f288_80x128_f(u8g2, U8G2_R3, u8x8_byte_i2c, u8x8_delay);
    u8g2_InitDisplay(u8g2);     //send init sequence to the display, display is in sleep mode after this,
    u8g2_ClearDisplay(u8g2);    //清空屏幕
    u8g2_SetPowerSave(u8g2, 0); //wake up display
}

void spi_oled_Init(u8g2_t *u8g2)    //初始化_SPI驱动
{
    HAL_GPIO_WritePin(OLED_IM_GPIO_Port,OLED_IM_Pin,0);
    u8g2_Setup_sh1107_tk078f288_80x128_f(u8g2, U8G2_R3, u8x8_byte_4wire_hw_spi,u8x8_stm32_gpio_and_delay);
    u8g2_InitDisplay(u8g2);     //send init sequence to the display, display is in sleep mode after this,
    u8g2_ClearDisplay(u8g2);    //清空屏幕
//    u8g2_SetFlipMode(u8g2,0);
    u8g2_SetPowerSave(u8g2, 0); //wake up display
}

#define SEND_BUFFER_DISPLAY_MS(u8g2, ms)\
  do {\
    u8g2_SendBuffer(u8g2); \
    HAL_Delay(ms);\
  }while(0);

/************************************************下面是测试例子************************************************/
//测试例子来自这个网址： blog.csdn.net/black_sneak/article/details/126312657
/************************************************OLED绘图函数************************************************/
void draw(u8g2_t *u8g2)
{
    u8g2_ClearBuffer(u8g2);

    u8g2_SetFontMode(u8g2, 1); /*字体模式选择*/
    u8g2_SetFontDirection(u8g2, 0); /*字体方向选择*/
    u8g2_SetFont(u8g2, u8g2_font_inb24_mf); /*字库选择*/
    u8g2_DrawStr(u8g2, 0, 20, "U");

    u8g2_SetFontDirection(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_inb30_mn);
    u8g2_DrawStr(u8g2, 21,8,"8");

    u8g2_SetFontDirection(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_inb24_mf);
    u8g2_DrawStr(u8g2, 51,30,"g");
    u8g2_DrawStr(u8g2, 67,30,"\xb2");

    u8g2_DrawHLine(u8g2, 2, 35, 47);
    u8g2_DrawHLine(u8g2, 3, 36, 47);
    u8g2_DrawVLine(u8g2, 45, 32, 12);
    u8g2_DrawVLine(u8g2, 46, 33, 12);

    u8g2_SetFont(u8g2, u8g2_font_4x6_tr);
    u8g2_DrawStr(u8g2, 1,54,"github.com/olikraus/u8g2");

    u8g2_SetFont(u8g2, u8g2_font_4x6_tr);
    u8g2_DrawStr(u8g2, 1,64,"github.com/leon-red/St1x");

    u8g2_SendBuffer(u8g2);
    HAL_Delay(1500);
}

/************************************************画点填充************************************************/
void testDrawPixelToFillScreen(u8g2_t *u8g2)
{
    int t = 1000;
    u8g2_ClearBuffer(u8g2);

    for (int j = 0; j < 64; j++)
    {
        for (int i = 0; i < 128; i++)
        {
            u8g2_DrawPixel(u8g2,i, j);
        }
    }
    HAL_Delay(1000);
}
/************************************************画点填充************************************************/

/************************************************OLED进度条测试函数************************************************/
void testDrawProcess(u8g2_t *u8g2)
{
    for(int i=10;i<=128;i=i+1)
    {
        u8g2_ClearBuffer(u8g2);

        char buff[20];
        sprintf(buff,"%.4f%%",(float)(i/128.0*100));

        u8g2_SetFont(u8g2,u8g2_font_ncenB18_tf);
        u8g2_DrawStr(u8g2,0,32,"St1x U8g2");//字符显示

        u8g2_SetFont(u8g2,u8g2_font_ncenB08_tf);
        u8g2_DrawStr(u8g2,36,48,buff);//当前进度显示

        u8g2_DrawRBox(u8g2,0,56,i,10,4);//圆角填充框矩形框
        u8g2_DrawRFrame(u8g2,0,56,128,10,4);//圆角矩形

        u8g2_SendBuffer(u8g2);
    }
}
/************************************************OLED进度条测试函数************************************************/

/************************************************字体测试************************************************/
void testShowFont(u8g2_t *u8g2)
{
    int t = 1000;
    char testStr[13] = "STM32F103CBT6";

    u8g2_ClearBuffer(u8g2);

    u8g2_SetFont(u8g2,u8g2_font_u8glib_4_tf);
    u8g2_DrawStr(u8g2,0,5,testStr);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);

    u8g2_SetFont(u8g2,u8g2_font_ncenB08_tf);
    u8g2_DrawStr(u8g2,0,30,testStr);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);

    u8g2_SetFont(u8g2,u8g2_font_ncenB10_tr);
    u8g2_DrawStr(u8g2,0,60,testStr);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
}
/************************************************字体测试************************************************/

/************************************************画空心矩形************************************************/
void testDrawFrame(u8g2_t *u8g2)
{
    int t = 1000;
    int x = 16;
    int y = 32;
    int w = 50;
    int h = 20;
    u8g2_ClearBuffer(u8g2);
    u8g2_DrawStr(u8g2,0, 15, "DrawFrame");

    u8g2_DrawFrame(u8g2, x, y, w, h);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFrame(u8g2, x+w+5, y-10, w-20, h+20);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
}
/************************************************画空心矩形************************************************/

/************************************************画实心圆角矩形************************************************/
void testDrawRBox(u8g2_t *u8g2)
{
    int t = 1000;
    int x = 16;
    int y = 32;
    int w = 50;
    int h = 20;
    int r = 3;
    u8g2_ClearBuffer(u8g2);
    u8g2_DrawStr(u8g2,0, 15, "DrawRBox");

    u8g2_DrawRBox(u8g2, x, y, w, h, r);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawRBox(u8g2, x+w+5, y-10, w-20, h+20, r);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
}
/************************************************画实心圆角矩形************************************************/

/************************************************画空心圆************************************************/
void testDrawCircle(u8g2_t *u8g2)
{
    int t = 600;
    int stx = 0;  //画图起始x
    int sty = 16; //画图起始y
    int with = 16;//一个图块的间隔
    int r = 15;   //圆的半径
    u8g2_ClearBuffer(u8g2);
    u8g2_DrawStr(u8g2, 0, 15, "DrawCircle");

    u8g2_DrawCircle(u8g2, stx, sty - 1 + with, r, U8G2_DRAW_UPPER_RIGHT); //右上
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawCircle(u8g2, stx + with, sty, r, U8G2_DRAW_LOWER_RIGHT); //右下
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawCircle(u8g2, stx - 1 + with * 3, sty - 1 + with, r, U8G2_DRAW_UPPER_LEFT); //左上
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawCircle(u8g2, stx - 1 + with * 4, sty, r, U8G2_DRAW_LOWER_LEFT); //左下
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawCircle(u8g2, stx - 1 + with * 2, sty - 1 + with * 2, r, U8G2_DRAW_ALL);//整个圆
    SEND_BUFFER_DISPLAY_MS(u8g2,t);

    u8g2_DrawCircle(u8g2, 32*3, 32, 31, U8G2_DRAW_ALL);//右侧整个圆
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
}
/************************************************画空心圆************************************************/

/************************************************画实心椭圆************************************************/
void testDrawFilledEllipse(u8g2_t *u8g2)
{
    int t = 800;
    int with = 16;//一个图块的间隔
    int rx = 27;  //椭圆x方向的半径
    int ry = 22;  //椭圆y方向的半径
    u8g2_ClearBuffer(u8g2);
    u8g2_DrawStr(u8g2,0, 14, "DrawFilledEllipse");

    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFilledEllipse(u8g2, 0, with, rx, ry, U8G2_DRAW_LOWER_RIGHT);//右下
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFilledEllipse(u8g2, with * 4 - 1, with, rx, ry, U8G2_DRAW_LOWER_LEFT); //左下
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFilledEllipse(u8g2, 0, with * 4 - 1, rx, ry, U8G2_DRAW_UPPER_RIGHT); //右上
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFilledEllipse(u8g2, with * 4 - 1, with * 4 - 1, rx, ry, U8G2_DRAW_UPPER_LEFT); //左上
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFilledEllipse(u8g2, with * 6, with * 2.5, rx, ry, U8G2_DRAW_ALL);//整个椭圆
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
}
/************************************************画实心椭圆************************************************/

void testDrawMulti(u8g2_t *u8g2)
{
    u8g2_ClearBuffer(u8g2);
    for (int j = 0; j < 64; j+=16)
    {
        for (int i = 0; i < 128; i+=16)
        {
            u8g2_DrawPixel(u8g2, i, j);
            u8g2_SendBuffer(u8g2);
        }
    }

    //实心矩形逐渐变大
    u8g2_ClearBuffer(u8g2);
    for(int i=30; i>0; i-=2)
    {
        u8g2_DrawBox(u8g2,i*2,i,128-i*4,64-2*i);
        u8g2_SendBuffer(u8g2);
    }
    //空心矩形逐渐变小
    u8g2_ClearBuffer(u8g2);
    for(int i=0; i<32; i+=2)
    {
        u8g2_DrawFrame(u8g2,i*2,i,128-i*4,64-2*i);
        u8g2_SendBuffer(u8g2);
    }

    //实心圆角矩形逐渐变大
    u8g2_ClearBuffer(u8g2);
    for(int i=30; i>0; i-=2)
    {
        u8g2_DrawRBox(u8g2,i*2,i,128-i*4,64-2*i,10-i/3);
        u8g2_SendBuffer(u8g2);
    }
    //空心圆角矩形逐渐变小
    u8g2_ClearBuffer(u8g2);
    for(int i=0; i<32; i+=2)
    {
        u8g2_DrawRFrame(u8g2,i*2,i,128-i*4,64-2*i,10-i/3);
        u8g2_SendBuffer(u8g2);
    }

    //实心圆逐渐变大
    u8g2_ClearBuffer(u8g2);
    for(int i=2; i<64; i+=3)
    {
        u8g2_DrawDisc(u8g2,64,32,i, U8G2_DRAW_ALL);
        u8g2_SendBuffer(u8g2);
    }
    //空心圆逐渐变小
    u8g2_ClearBuffer(u8g2);
    for(int i=64; i>0; i-=3)
    {
        u8g2_DrawCircle(u8g2,64,32,i, U8G2_DRAW_ALL);
        u8g2_SendBuffer(u8g2);
    }

    //实心椭圆逐渐变大
    u8g2_ClearBuffer(u8g2);
    for(int i=2; i<32; i+=3)
    {
        u8g2_DrawFilledEllipse(u8g2,64,32, i*2, i, U8G2_DRAW_ALL);
        u8g2_SendBuffer(u8g2);
    }
    //空心椭圆逐渐变小
    u8g2_ClearBuffer(u8g2);
    for(int i=32; i>0; i-=3)
    {
        u8g2_DrawEllipse(u8g2,64,32, i*2, i, U8G2_DRAW_ALL);
        u8g2_SendBuffer(u8g2);
    }
}
//测试上面的例子
void u8g2DrawTest(u8g2_t *u8g2)
{
    testDrawProcess(u8g2);
    testDrawFrame(u8g2);
    testDrawRBox(u8g2);
    testDrawCircle(u8g2);
    testDrawFilledEllipse(u8g2);
    testShowFont(u8g2);
    testDrawMulti(u8g2);
}

