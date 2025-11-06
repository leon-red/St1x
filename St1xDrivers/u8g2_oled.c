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

/*******************************************I2C通信处理函数*******************************************/
/**
 * @brief 通过I2C接口发送数据到OLED显示屏
 * @param u8x8 指向u8x8对象的指针
 * @param msg 消息类型
 * @param arg_int 参数整数
 * @param arg_ptr 参数指针
 * @return 成功返回1，失败返回0
 */
uint8_t u8x8_byte_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    // 缓冲区用于存储待发送的数据，最大32字节
    static uint8_t buffer[32];  /* u8g2/u8x8 will never send more than 32 bytes between START_TRANSFER and END_TRANSFER */
    static uint8_t buf_idx;
    uint8_t *data;

    switch(msg)
    {
        case U8X8_MSG_BYTE_SEND:
            // 发送数据到缓冲区
            data = (uint8_t *)arg_ptr;
            while( arg_int > 0 )
            {
                buffer[buf_idx++] = *data;
                data++;
                arg_int--;
            }
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            // 开始传输前清空缓冲区索引
            buf_idx = 0;
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            // 使用HAL库通过I2C发送缓冲区中的所有数据
            HAL_I2C_Master_Transmit(&hi2c1, u8x8_GetI2CAddress(u8x8), buffer, buf_idx, 500);
//            HAL_I2C_Master_Transmit_DMA(&hi2c1,u8x8_GetI2CAddress(u8x8),buffer,buf_idx);
//            HAL_I2C_Master_Receive_DMA(&hi2c1,u8x8_GetI2CAddress(u8x8),buffer,buf_idx);
            break;
        default:
            return 0;
    }
    return 1;
}

/*******************************************SPI通信处理函数*******************************************/
/**
 * @brief 通过SPI接口发送数据到OLED显示屏
 * @param u8x8 指向u8x8对象的指针
 * @param msg 消息类型
 * @param arg_int 参数整数
 * @param arg_ptr 参数指针
 * @return 成功返回1，失败返回0
 */
uint8_t u8x8_byte_4wire_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_BYTE_SEND: 
            /*通过SPI发送arg_int个字节数据*/
            HAL_SPI_Transmit_DMA(&hspi2,(uint8_t *)arg_ptr,arg_int);
            while (hspi2.TxXferCount);
            break;
        case U8X8_MSG_BYTE_INIT: 
            /*初始化SPI接口*/
            break;
        case U8X8_MSG_BYTE_SET_DC: 
            /*设置DC信号，区分传输的是数据还是命令*/
            HAL_GPIO_WritePin(OLED_DC_GPIO_Port,OLED_DC_Pin,arg_int);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            // 片选使能并等待
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
            u8x8->gpio_and_delay_cb(u8x8,U8X8_MSG_DELAY_NANO,u8x8->display_info->post_chip_enable_wait_ns,NULL);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            // 传输完成后取消片选
            u8x8->gpio_and_delay_cb(u8x8,U8X8_MSG_DELAY_NANO,u8x8->display_info->pre_chip_disable_wait_ns,NULL);
            u8x8_gpio_SetCS(u8x8,u8x8->display_info->chip_disable_level);
            break;
        default:
            return 0;
    }
    return 1;
}

/**********************************************OLED屏幕延时函数**********************************************/
/**
 * @brief OLED显示延时函数
 * @param u8x8 指向u8x8对象的指针
 * @param msg 延时消息类型
 * @param arg_int 延时时间参数
 * @param arg_ptr 参数指针
 * @return 成功返回1，失败返回0
 */
uint8_t u8x8_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_DELAY_MILLI:
            // 毫秒级延时
            HAL_Delay(arg_int);
            break;
            //Function which delays 10us
        case U8X8_MSG_DELAY_10MICRO:
            // 微秒级延时(10微秒)
            for (uint16_t n = 0; n < 320; n++)
            {
                __NOP();
            }

            break;
            //Function which delays 100ns
        case U8X8_MSG_DELAY_100NANO:
            // 纳秒级延时(100纳秒)
            __NOP();
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * @brief STM32 GPIO和延时控制函数
 * @param u8x8 指向u8x8对象的指针
 * @param msg 控制消息类型
 * @param arg_int 参数整数
 * @param arg_ptr 参数指针
 * @return 成功返回1，失败返回0
 */
uint8_t u8x8_stm32_gpio_and_delay(U8X8_UNUSED u8x8_t *u8x8,
                                  U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int,
                                  U8X8_UNUSED void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_GPIO_AND_DELAY_INIT: 
            /*delay和GPIO的初始化，实际上在main函数中已经初始化完成*/
            break;
        case U8X8_MSG_DELAY_MILLI: 
            /*延时函数*/
            HAL_Delay(arg_int);
            break;
        case U8X8_MSG_GPIO_CS: 
            /*片选信号*/
            HAL_GPIO_WritePin(OLED_CS_GPIO_Port,OLED_CS_Pin,arg_int);
            break;
        case U8X8_MSG_GPIO_DC: 
            /*设置DC信号，区分传输的是数据还是命令*/
            HAL_GPIO_WritePin(OLED_DC_GPIO_Port,OLED_DC_Pin,arg_int);
            break;
        case U8X8_MSG_GPIO_RESET:
            // 复位信号处理
            break;
    }
    return 1;
}
/**********************************************OLED屏幕延时函数**********************************************/

/************************************************OLED初始化函数************************************************/
/**
 * @brief 初始化OLED显示屏(I2C接口)
 * @param u8g2 指向u8g2对象的指针
 */
void I2C_oled_Init(u8g2_t *u8g2)    
{
    // 设置OLED相关GPIO引脚状态
    HAL_GPIO_WritePin(OLED_Reset_GPIO_Port,OLED_Reset_Pin,1);
    HAL_GPIO_WritePin(OLED_IM_GPIO_Port,OLED_IM_Pin,1);
    HAL_GPIO_WritePin(OLED_CS_GPIO_Port,OLED_CS_Pin,0);
    HAL_GPIO_WritePin(OLED_DC_GPIO_Port,OLED_DC_Pin,0);
    
    // 配置OLED显示参数(使用SH1107驱动，80x128分辨率)
    u8g2_Setup_sh1107_i2c_tk078f288_80x128_f(u8g2, U8G2_R3, u8x8_byte_i2c, u8x8_delay);
    u8g2_InitDisplay(u8g2);     // 发送初始化序列到显示屏，初始化后显示屏处于睡眠模式
    u8g2_ClearDisplay(u8g2);    // 清屏
    u8g2_SetPowerSave(u8g2, 0); // 唤醒显示屏
}

/**
 * @brief 初始化OLED显示屏(SPI接口)
 * @param u8g2 指向u8g2对象的指针
 */
void spi_oled_Init(u8g2_t *u8g2)    
{
    // 设置OLED接口模式为SPI
    HAL_GPIO_WritePin(OLED_IM_GPIO_Port,OLED_IM_Pin,0);
    
    // 配置OLED显示参数(使用SH1107驱动，80x128分辨率)
    u8g2_Setup_sh1107_tk078f288_80x128_f(u8g2, U8G2_R3, u8x8_byte_4wire_hw_spi,u8x8_stm32_gpio_and_delay);
    u8g2_InitDisplay(u8g2);     // 发送初始化序列到显示屏，初始化后显示屏处于睡眠模式
    u8g2_ClearDisplay(u8g2);    // 清屏
//    u8g2_SetFlipMode(u8g2,0);
    u8g2_SetPowerSave(u8g2, 0); // 唤醒显示屏
}

// 定义一个宏，用于发送显示缓冲区并延时
#define SEND_BUFFER_DISPLAY_MS(u8g2, ms)\
  do {\
    u8g2_SendBuffer(u8g2); \
    HAL_Delay(ms);\
  }while(0);

/************************************************测试函数************************************************/
//参考地址： blog.csdn.net/black_sneak/article/details/126312657

/************************************************OLED绘图示例************************************************/
/**
 * @brief 绘制基本图形示例
 * @param u8g2 指向u8g2对象的指针
 */
void draw(u8g2_t *u8g2)
{
    u8g2_ClearBuffer(u8g2);  // 清除显示缓冲区

    // 设置字体模式和方向，绘制字母"U"
    u8g2_SetFontMode(u8g2, 1); /*字体模式选择*/
    u8g2_SetFontDirection(u8g2, 0); /*字体方向选择*/
    u8g2_SetFont(u8g2, u8g2_font_inb24_mf); /*字体选择*/
    u8g2_DrawStr(u8g2, 0, 20, "U");

    // 绘制数字"8"
    u8g2_SetFontDirection(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_inb30_mn);
    u8g2_DrawStr(u8g2, 21,8,"8");

    // 绘制字母"g"和特殊符号"²"
    u8g2_SetFontDirection(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_inb24_mf);
    u8g2_DrawStr(u8g2, 51,30,"g");
    u8g2_DrawStr(u8g2, 67,30,"\xb2");

    // 绘制线条装饰
    u8g2_DrawHLine(u8g2, 2, 35, 47);
    u8g2_DrawHLine(u8g2, 3, 36, 47);
    u8g2_DrawVLine(u8g2, 45, 32, 12);
    u8g2_DrawVLine(u8g2, 46, 33, 12);

    // 显示项目相关信息
    u8g2_SetFont(u8g2, u8g2_font_4x6_tr);
    u8g2_DrawStr(u8g2, 1,54,"github.com/olikraus/u8g2");

    u8g2_SetFont(u8g2, u8g2_font_4x6_tr);
    u8g2_DrawStr(u8g2, 1,64,"github.com/leon-red/St1x");

    u8g2_SendBuffer(u8g2);  // 发送缓冲区内容到显示屏
    HAL_Delay(1500);        // 延时1.5秒
}

/************************************************测试像素填充屏幕************************************************/
/**
 * @brief 像素填充整个屏幕测试
 * @param u8g2 指向u8g2对象的指针
 */
void testDrawPixelToFillScreen(u8g2_t *u8g2)
{
    int t = 1000;
    u8g2_ClearBuffer(u8g2);  // 清除显示缓冲区

    // 使用像素点填充整个屏幕
    for (int j = 0; j < 64; j++)
    {
        for (int i = 0; i < 128; i++)
        {
            u8g2_DrawPixel(u8g2,i, j);  // 在(i,j)位置绘制像素点
        }
    }
    HAL_Delay(1000);  // 延时1秒
}

/************************************************OLED进度条绘制演示************************************************/
/**
 * @brief 进度条绘制演示
 * @param u8g2 指向u8g2对象的指针
 */
void testDrawProcess(u8g2_t *u8g2)
{
    // 逐步增加进度条长度
    for(int i=10;i<=128;i=i+1)
    {
        u8g2_ClearBuffer(u8g2);  // 清除显示缓冲区

        char buff[20];
        // 计算并格式化进度百分比
        sprintf(buff,"%.4f%%",(float)(i/128.0*100));

        // 显示标题
        u8g2_SetFont(u8g2,u8g2_font_ncenB18_tf);
        u8g2_DrawStr(u8g2,0,32,"St1x U8g2");  // 字符串显示

        // 显示当前进度
        u8g2_SetFont(u8g2,u8g2_font_ncenB08_tf);
        u8g2_DrawStr(u8g2,36,48,buff);        // 当前进度显示

        // 绘制进度条(实心矩形)和进度条边框(空心矩形)
        u8g2_DrawRBox(u8g2,0,56,i,10,4);      // 圆角矩形绘制
        u8g2_DrawRFrame(u8g2,0,56,128,10,4);  // 圆角矩形边框

        u8g2_SendBuffer(u8g2);  // 发送缓冲区内容到显示屏
    }
}

/************************************************测试字体显示************************************************/
/**
 * @brief 字体显示测试
 * @param u8g2 指向u8g2对象的指针
 */
void testShowFont(u8g2_t *u8g2)
{
    int t = 1000;
    char testStr[13] = "STM32F103CBT6";  // 测试字符串

    u8g2_ClearBuffer(u8g2);  // 清除显示缓冲区

    // 使用不同字体显示相同字符串
    u8g2_SetFont(u8g2,u8g2_font_u8glib_4_tf);
    u8g2_DrawStr(u8g2,0,5,testStr);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);  // 发送缓冲区并延时

    u8g2_SetFont(u8g2,u8g2_font_ncenB08_tf);
    u8g2_DrawStr(u8g2,0,30,testStr);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);  // 发送缓冲区并延时

    u8g2_SetFont(u8g2,u8g2_font_ncenB10_tr);
    u8g2_DrawStr(u8g2,0,60,testStr);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);  // 发送缓冲区并延时
}

/************************************************测试矩形绘制************************************************/
/**
 * @brief 矩形绘制测试
 * @param u8g2 指向u8g2对象的指针
 */
void testDrawFrame(u8g2_t *u8g2)
{
    int t = 1000;
    int x = 16;   // 矩形起始x坐标
    int y = 32;   // 矩形起始y坐标
    int w = 50;   // 矩形宽度
    int h = 20;   // 矩形高度
    u8g2_ClearBuffer(u8g2);  // 清除显示缓冲区
    
    // 显示测试标题
    u8g2_DrawStr(u8g2,0, 15, "DrawFrame");

    // 绘制两个空心矩形
    u8g2_DrawFrame(u8g2, x, y, w, h);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFrame(u8g2, x+w+5, y-10, w-20, h+20);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
}

/************************************************绘制圆角矩形************************************************/
/**
 * @brief 圆角矩形绘制测试
 * @param u8g2 指向u8g2对象的指针
 */
void testDrawRBox(u8g2_t *u8g2)
{
    int t = 1000;
    int x = 16;   // 圆角矩形起始x坐标
    int y = 32;   // 圆角矩形起始y坐标
    int w = 50;   // 圆角矩形宽度
    int h = 20;   // 圆角矩形高度
    int r = 3;    // 圆角半径
    u8g2_ClearBuffer(u8g2);  // 清除显示缓冲区
    
    // 显示测试标题
    u8g2_DrawStr(u8g2,0, 15, "DrawRBox");

    // 绘制两个实心圆角矩形
    u8g2_DrawRBox(u8g2, x, y, w, h, r);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawRBox(u8g2, x+w+5, y-10, w-20, h+20, r);
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
}

/************************************************绘制圆形************************************************/
/**
 * @brief 圆形绘制测试
 * @param u8g2 指向u8g2对象的指针
 */
void testDrawCircle(u8g2_t *u8g2)
{
    int t = 600;
    int stx = 0;    // 图形起始x坐标
    int sty = 16;   // 图形起始y坐标
    int with = 16;  // 一个图形的宽度
    int r = 15;     // 圆的半径
    u8g2_ClearBuffer(u8g2);  // 清除显示缓冲区
    
    // 显示测试标题
    u8g2_DrawStr(u8g2, 0, 15, "DrawCircle");

    // 分别绘制圆的四个部分和完整圆
    u8g2_DrawCircle(u8g2, stx, sty - 1 + with, r, U8G2_DRAW_UPPER_RIGHT); // 右上
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawCircle(u8g2, stx + with, sty, r, U8G2_DRAW_LOWER_RIGHT); // 右下
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawCircle(u8g2, stx - 1 + with * 3, sty - 1 + with, r, U8G2_DRAW_UPPER_LEFT); // 左上
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawCircle(u8g2, stx - 1 + with * 4, sty, r, U8G2_DRAW_LOWER_LEFT); // 左下
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawCircle(u8g2, stx - 1 + with * 2, sty - 1 + with * 2, r, U8G2_DRAW_ALL); // 全圆
    SEND_BUFFER_DISPLAY_MS(u8g2,t);

    u8g2_DrawCircle(u8g2, 32*3, 32, 31, U8G2_DRAW_ALL); // 实心圆
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
}

/************************************************绘制实心椭圆************************************************/
/**
 * @brief 实心椭圆绘制测试
 * @param u8g2 指向u8g2对象的指针
 */
void testDrawFilledEllipse(u8g2_t *u8g2)
{
    int t = 800;
    int with = 16;  // 一个图形的宽度
    int rx = 27;    // 椭圆x轴半径
    int ry = 22;    // 椭圆y轴半径
    u8g2_ClearBuffer(u8g2);  // 清除显示缓冲区
    
    // 显示测试标题
    u8g2_DrawStr(u8g2,0, 14, "DrawFilledEllipse");

    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFilledEllipse(u8g2, 0, with, rx, ry, U8G2_DRAW_LOWER_RIGHT); // 右下
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFilledEllipse(u8g2, with * 4 - 1, with, rx, ry, U8G2_DRAW_LOWER_LEFT); // 左下
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFilledEllipse(u8g2, 0, with * 4 - 1, rx, ry, U8G2_DRAW_UPPER_RIGHT); // 右上
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFilledEllipse(u8g2, with * 4 - 1, with * 4 - 1, rx, ry, U8G2_DRAW_UPPER_LEFT); // 左上
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
    u8g2_DrawFilledEllipse(u8g2, with * 6, with * 2.5, rx, ry, U8G2_DRAW_ALL); // 全椭圆
    SEND_BUFFER_DISPLAY_MS(u8g2,t);
}

/************************************************测试多图形绘制************************************************/
/**
 * @brief 多图形绘制测试
 * @param u8g2 指向u8g2对象的指针
 */
void testDrawMulti(u8g2_t *u8g2)
{
    // 绘制网格点
    u8g2_ClearBuffer(u8g2);
    for (int j = 0; j < 64; j+=16)
    {
        for (int i = 0; i < 128; i+=16)
        {
            u8g2_DrawPixel(u8g2, i, j);  // 在(i,j)位置绘制像素点
            u8g2_SendBuffer(u8g2);       // 发送到显示屏
        }
    }

    // 绘制实心矩形并逐渐增大
    u8g2_ClearBuffer(u8g2);
    for(int i=30; i>0; i-=2)
    {
        u8g2_DrawBox(u8g2,i*2,i,128-i*4,64-2*i);  // 绘制实心矩形
        u8g2_SendBuffer(u8g2);                    // 发送到显示屏
    }
    
    // 绘制空心矩形并逐渐减小
    u8g2_ClearBuffer(u8g2);
    for(int i=0; i<32; i+=2)
    {
        u8g2_DrawFrame(u8g2,i*2,i,128-i*4,64-2*i); // 绘制空心矩形
        u8g2_SendBuffer(u8g2);                     // 发送到显示屏
    }

    // 绘制实心圆角矩形并逐渐增大
    u8g2_ClearBuffer(u8g2);
    for(int i=30; i>0; i-=2)
    {
        u8g2_DrawRBox(u8g2,i*2,i,128-i*4,64-2*i,10-i/3);  // 绘制实心圆角矩形
        u8g2_SendBuffer(u8g2);                            // 发送到显示屏
    }
    
    // 绘制空心圆角矩形并逐渐减小
    u8g2_ClearBuffer(u8g2);
    for(int i=0; i<32; i+=2)
    {
        u8g2_DrawRFrame(u8g2,i*2,i,128-i*4,64-2*i,10-i/3); // 绘制空心圆角矩形
        u8g2_SendBuffer(u8g2);                             // 发送到显示屏
    }

    // 绘制实心圆并逐渐增大
    u8g2_ClearBuffer(u8g2);
    for(int i=2; i<64; i+=3)
    {
        u8g2_DrawDisc(u8g2,64,32,i, U8G2_DRAW_ALL);  // 绘制实心圆
        u8g2_SendBuffer(u8g2);                      // 发送到显示屏
    }
    
    // 绘制空心圆并逐渐减小
    u8g2_ClearBuffer(u8g2);
    for(int i=64; i>0; i-=3)
    {
        u8g2_DrawCircle(u8g2,64,32,i, U8G2_DRAW_ALL); // 绘制空心圆
        u8g2_SendBuffer(u8g2);                       // 发送到显示屏
    }

    // 绘制实心椭圆并逐渐增大
    u8g2_ClearBuffer(u8g2);
    for(int i=2; i<32; i+=3)
    {
        u8g2_DrawFilledEllipse(u8g2,64,32, i*2, i, U8G2_DRAW_ALL); // 绘制实心椭圆
        u8g2_SendBuffer(u8g2);                                    // 发送到显示屏
    }
    
    // 绘制空心椭圆并逐渐减小
    u8g2_ClearBuffer(u8g2);
    for(int i=32; i>0; i-=3)
    {
        u8g2_DrawEllipse(u8g2,64,32, i*2, i, U8G2_DRAW_ALL); // 绘制空心椭圆
        u8g2_SendBuffer(u8g2);                              // 发送到显示屏
    }
}

/**
 * @brief 所有绘图测试的集合函数
 * @param u8g2 指向u8g2对象的指针
 */
void u8g2DrawTest(u8g2_t *u8g2)
{
    // 依次执行各个测试函数
    testDrawProcess(u8g2);        // 进度条绘制测试
    testDrawFrame(u8g2);          // 矩形绘制测试
    testDrawRBox(u8g2);           // 圆角矩形绘制测试
    testDrawCircle(u8g2);         // 圆形绘制测试
    testDrawFilledEllipse(u8g2);  // 椭圆绘制测试
    testShowFont(u8g2);           // 字体显示测试
    testDrawMulti(u8g2);          // 多图形绘制测试
}

