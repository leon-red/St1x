#include "i2coled.h"
#include "i2c.h"


uint8_t CMD_Data[]={
        0xAE,	//关显示
        0x02,	//设置列地址低四位（00-0F）。（0 0 0 0 A3 A2 A1 A0）
        0x10,	//设置列地址高四位（10-1F）。（0 0 0 1 A7 A6 A5 A4）
        0x50,	//指定起始行地址（40-7F），共64行（每行一像素）
        0xB0,	//指定起始页地址（B0-B7），共8页（每页8像素）
        0x81,	//启用对比度数据寄存器。双字节命令。
        0xCF,	//对比度设置（00-FF），共256个级别。设置完对比度数据后，对比度数据寄存器自动释放。
        0xA1,	//A1：正常显示，A0：水平反转显示。当显示数据被写入或读取时，列地址增加1。
        0xC8,	//C0-C7：正常显示，C8-CF：垂直翻转。设置公共输出扫描方向。
        0xA6,	//A6：关反显，A7：开反显。在不重写显示数据RAM内容的情况下反转显示开/关状态。
        0xA8,	//设置多路复用比率。此命令将默认的64种多路传输模式切换到1到64之间的任意多路传输比率。双字节命令，设置完这条命令后需要写入比率数据。
        0x3F,	//多路定量数据集（00-3F）。3F：64路多路传输比率。
        0xD3,	//设置显示偏移模式。双字节命令，设置完这条命令后需要写入显示偏移模式数据。
        0x00,	//偏移量（00-3F）。00：不偏移。
        0xD5,	//设置显示时钟分频比/振荡器频率。双字节命令，设置完这条命令后需要写入显示时钟分频比/振荡器频率数据。
        0x80,	//分频比/振荡器频率数据（00-FF）。0x80：分频比为1，振荡器频率为+15%
        0xD9,	//设置预充电周期的持续时间。间隔以DCLK的个数计。POR是2个DCLKs。双字节命令，设置完这条命令后需要写入预充电周期的持续时间。
        0xF1,	//周期持续时间（00-FF）。0xF1：预充电周期的持续时间为1个DCLK，掉电周期的持续时间为15个DCLK。
        0xDA,	//设置公用焊盘硬件配置。此命令用于设置公共信号板配置（顺序或替代）以匹配OLED面板硬件布局。双字节命令，设置完这条命令后需要写入公用焊盘硬件配置。
        0x12,	//顺序/替代模式设置（02-12）。0x02：顺序模式。0x12：替代模式。
        0xDB,	//设置VCOM取消选择级别。此命令用于在取消选择阶段设置公共焊盘输出电压电平。双字节命令。
        0x40,	//VCOM取消选择级别数据（00-FF）。
        0xA4,	//关闭/打开整个显示。0xA4：正常显示。0xA5：整个显示。此命令的优先级高于正常/反向显示命令。
        0xA6,
        0xAF,	//开显示
};      //初始化命令


void WriteCmd(void)
{
    uint8_t i = 0;
    for(i=0; i<27; i++)
    {
        HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,CMD_Data+i,1,0x100);
    }
}
//向设备写控制命令
void OLED_WR_CMD(uint8_t cmd)
{
    HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,&cmd,1,0x100);
}
//向设备写数据
void OLED_WR_DATA(uint8_t data)
{
    HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x40,I2C_MEMADD_SIZE_8BIT,&data,1,0x100);
}
//初始化oled屏幕
void OLED_Init(void)
{
    HAL_Delay(200);

    WriteCmd();
}
//清屏
void OLED_Clear(void)
{
    uint8_t i,n;
    for(i=0;i<8;i++)
    {
        OLED_WR_CMD(0xb0+i);
        OLED_WR_CMD (0x00);
        OLED_WR_CMD (0x10);
        for(n=0;n<128;n++)
            OLED_WR_DATA(0);
    }
}

//反显函数
void OLED_ColorTurn(uint8_t i)
{
    if(i==0)
    {
        OLED_WR_CMD(0xA6);//正常显示
    }
    if(i==1)
    {
        OLED_WR_CMD(0xA7);//反色显示
    }
}

//屏幕旋转180度
void OLED_DisplayTurn(uint8_t i)
{
    if(i==0)
    {
        OLED_WR_CMD(0xD3); /*set display offset*/
        OLED_WR_CMD(0x68); /*18*/
        OLED_WR_CMD(0xC0);//正常显示
        OLED_WR_CMD(0xA0);
    }
    if(i==1)
    {
        OLED_WR_CMD(0xD3); /*set display offset*/
        OLED_WR_CMD(0x18); /*18*/
        OLED_WR_CMD(0xC8);//反转显示
        OLED_WR_CMD(0xA1);
    }
    if(i==3)
    {
        OLED_WR_CMD(0xD3); /*set display offset*/
        OLED_WR_CMD(0x18); /*18*/
        OLED_WR_CMD(0xC8);//反转显示
        OLED_WR_CMD(0xA1);
    }
    if(i==4)
    {
        OLED_WR_CMD(0xD3); /*set display offset*/
        OLED_WR_CMD(0x18); /*18*/
        OLED_WR_CMD(0xC8);//反转显示
        OLED_WR_CMD(0xA1);
    }
}

//开启OLED显示
void OLED_Display_On(void)
{
    OLED_WR_CMD(0X8D);  //SET DCDC命令
    OLED_WR_CMD(0X14);  //DCDC ON
    OLED_WR_CMD(0XAF);  //DISPLAY ON
}
//关闭OLED显示
void OLED_Display_Off(void)
{
    OLED_WR_CMD(0X8D);  //SET DCDC命令
    OLED_WR_CMD(0X10);  //DCDC OFF
    OLED_WR_CMD(0XAE);  //DISPLAY OFF
}
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
    OLED_WR_CMD(0xb0+y);
    OLED_WR_CMD(((x&0xf0)>>4)|0x10);
    OLED_WR_CMD(x&0x0f);
}

void OLED_On(void)
{
    uint8_t i,n;
    for(i=0;i<8;i++)
    {
        OLED_WR_CMD(0xb0+i);    //设置页地址（0~7）
        OLED_WR_CMD(0x00);      //设置显示位置—列低地址
        OLED_WR_CMD(0x10);      //设置显示位置—列高地址
        for(n=0;n<128;n++)
            OLED_WR_DATA(1);
    } //更新显示
}
unsigned int oled_pow(uint8_t m,uint8_t n)
{
    unsigned int result=1;
    while(n--)result*=m;
    return result;
}
//显示2个数字
//x,y :起点坐标
//len :数字的位数
//size:字体大小
//mode:模式	0,填充模式;1,叠加模式
//num:数值(0~4294967295);
void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t size2)
{
    uint8_t t,temp;
    uint8_t enshow=0;
    for(t=0;t<len;t++)
    {
        temp=(num/oled_pow(10,len-t-1))%10;
        if(enshow==0&&t<(len-1))
        {
            if(temp==0)
            {
                OLED_ShowChar(x+(size2/2)*t,y,' ',size2);
                continue;
            }else enshow=1;

        }
        OLED_ShowChar(x+(size2/2)*t,y,temp+'0',size2);
    }
}

//显示一个字符号串
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *chr,uint8_t Char_Size)
{
    unsigned char j=0;
    while (chr[j]!='\0')
    {		OLED_ShowChar(x,y,chr[j],Char_Size);
        x+=8;
        if(x>120){x=0;y+=2;}
        j++;
    }
}

//方案2
//
///**
//*	清屏
//**/
//void OLED_Clear()
//{
//    uint8_t i, j;
//
//    for(j = 0; j < 8; j++) {
//        OLED_WR_CMD(0x02);//先写列起始地址低四位
//        OLED_WR_CMD(0x10);//列起始地址高四位
//        OLED_WR_CMD(0xB0 + j);//设置显示页的地址，第一页地址为0xB0
//
//        for(i = 0; i < 128; i++) {
//            OLED_WR_CMD(0x00);//写显示数据
//        }
//    }
//}
//
///**
//*	计算字符显示起始x坐标的高低位。x1：高四位。x2：低四位。
//**/
//void Get_X_Pos_Bits(uint8_t xPos, uint8_t *x1, uint8_t *x2)
//{
//    *x1 = (xPos + 2) / 16;
//
//    if(xPos >= 0 && xPos <= 13)
//        *x2 = xPos;
//    else if(xPos <= 29)
//        *x2 = xPos - 14;
//    else if(xPos <= 45)
//        *x2 = xPos - 30;
//    else if(xPos <= 61)
//        *x2 = xPos - 46;
//    else if(xPos <= 77)
//        *x2 = xPos - 62;
//    else if(xPos <= 93)
//        *x2 = xPos - 78;
//    else if(xPos <= 109)
//        *x2 = xPos - 94;
//    else if(xPos <= 125)
//        *x2 = xPos - 110;
//    else
//        *x2 = xPos - 126;
//}
//
///** 显示指定分辨率的Char字符
//*   dat：    字符
//*   xPos：   字符显示起始x坐标（0-127）
//*   yPos：   字符显示起始y坐标（0-7）
//**/
//void Display_Char_CRAM(uint8_t dat, uint8_t xPos, uint8_t yPos, uint8_t width, uint8_t height)
//{
//    uint8_t i, j, l, k, x1 = 0, x2 = 0;
//
//    if(xPos > 127 || yPos > 7) {
//        return;
//    }
//
//    Get_X_Pos_Bits(xPos, &x1, &x2);
//
//    dat = dat - ' ';
//
//    for(j = yPos, l = 0; l < height / 8; j++, l++) {
//        if(xPos >= 0 && xPos <= 13)
//            OLED_WR_CMD(0x02 + x2);//先写列起始地址低四位
//        else
//            OLED_WR_CMD(0x00 + x2);
//
//        OLED_WR_CMD(0x10 + x1);//列起始地址高四位
//
//        OLED_WR_CMD(0xB0 + j);//设置显示页的地址，第一页地址为0xB0
//
//        for(i = xPos, k = 0; k < width; i++, k++) {
//            if(width == 24) {
//                //OLED_WR_CMD(CharFont2424[dat][l * width + k], WRITE_DATA);
//            }
//
//            if(width == 12) {
//                //OLED_WR_CMD(CharFont1216[dat][l * width + k], WRITE_DATA);
//            }
//
//            if(width == 5) {
//                OLED_WR_DATA(CharFont0508[dat][l * width + k]);
//            }
//
//        }
//    }
//}
///** 显示指定分辨率的String字符串
//*   dat：    字符
//*   xPos：   字符显示起始x坐标（0-127）
//*   yPos：   字符显示起始y坐标（0-7）
//*   width：  字符长度（像素）
//*   height：字符高度（像素）
//**/
//void Display_String_CRAM(uint8_t *dat, uint8_t xPos, uint8_t yPos, uint8_t width, uint8_t height)
//{
//    uint8_t i;
//
//    if(xPos > 127 || yPos > 7) {
//        return;
//    }
//
//    i = xPos;
//
//    while(*dat != '\0') {
//        Display_Char_CRAM(*dat, i, yPos, width, height);
//
//        if(width == 5) {
//            i = i + 1 + width;
//        } else {
//            i += width;
//        }
//
//        dat++;
//    }
//}
//
//
///** 显示BMP图片
//*	dat：	图片数据
//*	xPos：	图片显示起始x坐标（0-127）
//*	yPos：	图片显示起始y坐标（0-7）
//*	width：	图片长度（像素）
//*	height：图片高度（像素）
//*
//*	图片取模方式为：
//*		点阵格式：阴码
//*		取模方式：列行式
//*		取模走向：逆向（低位在前）
//**/
//void Display_BMP(uint8_t *dat, uint8_t xPos, uint8_t yPos, uint8_t width, uint8_t height)
//{
//    uint8_t i, j, x1 = 0, x2 = 0;
//
//    if((xPos < 0 || xPos > 127) && (yPos < 0 || yPos > 7)) {
//        return;
//    }
//
//    Get_X_Pos_Bits(xPos, &x1, &x2);
//
//    for(j = 0; j < height / 8; j++) {
//        if(xPos >= 0 && xPos <= 13)
//            OLED_WR_CMD(0x02 + x2);//先写列起始地址低四位
//        else
//            OLED_WR_CMD(0x00 + x2);
//
//        OLED_WR_CMD(0x10 + x1);//列起始地址高四位
//
//        OLED_WR_CMD(0xB0 + yPos);//设置显示页的地址，第一页地址为0xB0
//
//        for(i = 0; i < width; i++) {
//            OLED_WR_CMD(*dat++);//写显示数据
//        }
//
//        yPos++;//换行
//    }
//}
