#include "i2coled.h"
#include "i2c.h"


uint8_t CMD_Data[]={
        0xAE,	//����ʾ
        0x02,	//�����е�ַ����λ��00-0F������0 0 0 0 A3 A2 A1 A0��
        0x10,	//�����е�ַ����λ��10-1F������0 0 0 1 A7 A6 A5 A4��
        0x50,	//ָ����ʼ�е�ַ��40-7F������64�У�ÿ��һ���أ�
        0xB0,	//ָ����ʼҳ��ַ��B0-B7������8ҳ��ÿҳ8���أ�
        0x81,	//���öԱȶ����ݼĴ�����˫�ֽ����
        0xCF,	//�Աȶ����ã�00-FF������256������������Աȶ����ݺ󣬶Աȶ����ݼĴ����Զ��ͷš�
        0xA1,	//A1��������ʾ��A0��ˮƽ��ת��ʾ������ʾ���ݱ�д����ȡʱ���е�ַ����1��
        0xC8,	//C0-C7��������ʾ��C8-CF����ֱ��ת�����ù������ɨ�跽��
        0xA6,	//A6���ط��ԣ�A7�������ԡ��ڲ���д��ʾ����RAM���ݵ�����·�ת��ʾ��/��״̬��
        0xA8,	//���ö�·���ñ��ʡ������Ĭ�ϵ�64�ֶ�·����ģʽ�л���1��64֮��������·������ʡ�˫�ֽ���������������������Ҫд��������ݡ�
        0x3F,	//��·�������ݼ���00-3F����3F��64·��·������ʡ�
        0xD3,	//������ʾƫ��ģʽ��˫�ֽ���������������������Ҫд����ʾƫ��ģʽ���ݡ�
        0x00,	//ƫ������00-3F����00����ƫ�ơ�
        0xD5,	//������ʾʱ�ӷ�Ƶ��/����Ƶ�ʡ�˫�ֽ���������������������Ҫд����ʾʱ�ӷ�Ƶ��/����Ƶ�����ݡ�
        0x80,	//��Ƶ��/����Ƶ�����ݣ�00-FF����0x80����Ƶ��Ϊ1������Ƶ��Ϊ+15%
        0xD9,	//����Ԥ������ڵĳ���ʱ�䡣�����DCLK�ĸ����ơ�POR��2��DCLKs��˫�ֽ���������������������Ҫд��Ԥ������ڵĳ���ʱ�䡣
        0xF1,	//���ڳ���ʱ�䣨00-FF����0xF1��Ԥ������ڵĳ���ʱ��Ϊ1��DCLK���������ڵĳ���ʱ��Ϊ15��DCLK��
        0xDA,	//���ù��ú���Ӳ�����á��������������ù����źŰ����ã�˳����������ƥ��OLED���Ӳ�����֡�˫�ֽ���������������������Ҫд�빫�ú���Ӳ�����á�
        0x12,	//˳��/���ģʽ���ã�02-12����0x02��˳��ģʽ��0x12�����ģʽ��
        0xDB,	//����VCOMȡ��ѡ�񼶱𡣴�����������ȡ��ѡ��׶����ù������������ѹ��ƽ��˫�ֽ����
        0x40,	//VCOMȡ��ѡ�񼶱����ݣ�00-FF����
        0xA4,	//�ر�/��������ʾ��0xA4��������ʾ��0xA5��������ʾ������������ȼ���������/������ʾ���
        0xA6,
        0xAF,	//����ʾ
};      //��ʼ������


void WriteCmd(void)
{
    uint8_t i = 0;
    for(i=0; i<27; i++)
    {
        HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,CMD_Data+i,1,0x100);
    }
}
//���豸д��������
void OLED_WR_CMD(uint8_t cmd)
{
    HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,&cmd,1,0x100);
}
//���豸д����
void OLED_WR_DATA(uint8_t data)
{
    HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x40,I2C_MEMADD_SIZE_8BIT,&data,1,0x100);
}
//��ʼ��oled��Ļ
void OLED_Init(void)
{
    HAL_Delay(200);

    WriteCmd();
}
//����
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

//���Ժ���
void OLED_ColorTurn(uint8_t i)
{
    if(i==0)
    {
        OLED_WR_CMD(0xA6);//������ʾ
    }
    if(i==1)
    {
        OLED_WR_CMD(0xA7);//��ɫ��ʾ
    }
}

//��Ļ��ת180��
void OLED_DisplayTurn(uint8_t i)
{
    if(i==0)
    {
        OLED_WR_CMD(0xD3); /*set display offset*/
        OLED_WR_CMD(0x68); /*18*/
        OLED_WR_CMD(0xC0);//������ʾ
        OLED_WR_CMD(0xA0);
    }
    if(i==1)
    {
        OLED_WR_CMD(0xD3); /*set display offset*/
        OLED_WR_CMD(0x18); /*18*/
        OLED_WR_CMD(0xC8);//��ת��ʾ
        OLED_WR_CMD(0xA1);
    }
    if(i==3)
    {
        OLED_WR_CMD(0xD3); /*set display offset*/
        OLED_WR_CMD(0x18); /*18*/
        OLED_WR_CMD(0xC8);//��ת��ʾ
        OLED_WR_CMD(0xA1);
    }
    if(i==4)
    {
        OLED_WR_CMD(0xD3); /*set display offset*/
        OLED_WR_CMD(0x18); /*18*/
        OLED_WR_CMD(0xC8);//��ת��ʾ
        OLED_WR_CMD(0xA1);
    }
}

//����OLED��ʾ
void OLED_Display_On(void)
{
    OLED_WR_CMD(0X8D);  //SET DCDC����
    OLED_WR_CMD(0X14);  //DCDC ON
    OLED_WR_CMD(0XAF);  //DISPLAY ON
}
//�ر�OLED��ʾ
void OLED_Display_Off(void)
{
    OLED_WR_CMD(0X8D);  //SET DCDC����
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
        OLED_WR_CMD(0xb0+i);    //����ҳ��ַ��0~7��
        OLED_WR_CMD(0x00);      //������ʾλ�á��е͵�ַ
        OLED_WR_CMD(0x10);      //������ʾλ�á��иߵ�ַ
        for(n=0;n<128;n++)
            OLED_WR_DATA(1);
    } //������ʾ
}
unsigned int oled_pow(uint8_t m,uint8_t n)
{
    unsigned int result=1;
    while(n--)result*=m;
    return result;
}
//��ʾ2������
//x,y :�������
//len :���ֵ�λ��
//size:�����С
//mode:ģʽ	0,���ģʽ;1,����ģʽ
//num:��ֵ(0~4294967295);
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

//��ʾһ���ַ��Ŵ�
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

//����2
//
///**
//*	����
//**/
//void OLED_Clear()
//{
//    uint8_t i, j;
//
//    for(j = 0; j < 8; j++) {
//        OLED_WR_CMD(0x02);//��д����ʼ��ַ����λ
//        OLED_WR_CMD(0x10);//����ʼ��ַ����λ
//        OLED_WR_CMD(0xB0 + j);//������ʾҳ�ĵ�ַ����һҳ��ַΪ0xB0
//
//        for(i = 0; i < 128; i++) {
//            OLED_WR_CMD(0x00);//д��ʾ����
//        }
//    }
//}
//
///**
//*	�����ַ���ʾ��ʼx����ĸߵ�λ��x1������λ��x2������λ��
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
///** ��ʾָ���ֱ��ʵ�Char�ַ�
//*   dat��    �ַ�
//*   xPos��   �ַ���ʾ��ʼx���꣨0-127��
//*   yPos��   �ַ���ʾ��ʼy���꣨0-7��
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
//            OLED_WR_CMD(0x02 + x2);//��д����ʼ��ַ����λ
//        else
//            OLED_WR_CMD(0x00 + x2);
//
//        OLED_WR_CMD(0x10 + x1);//����ʼ��ַ����λ
//
//        OLED_WR_CMD(0xB0 + j);//������ʾҳ�ĵ�ַ����һҳ��ַΪ0xB0
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
///** ��ʾָ���ֱ��ʵ�String�ַ���
//*   dat��    �ַ�
//*   xPos��   �ַ���ʾ��ʼx���꣨0-127��
//*   yPos��   �ַ���ʾ��ʼy���꣨0-7��
//*   width��  �ַ����ȣ����أ�
//*   height���ַ��߶ȣ����أ�
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
///** ��ʾBMPͼƬ
//*	dat��	ͼƬ����
//*	xPos��	ͼƬ��ʾ��ʼx���꣨0-127��
//*	yPos��	ͼƬ��ʾ��ʼy���꣨0-7��
//*	width��	ͼƬ���ȣ����أ�
//*	height��ͼƬ�߶ȣ����أ�
//*
//*	ͼƬȡģ��ʽΪ��
//*		�����ʽ������
//*		ȡģ��ʽ������ʽ
//*		ȡģ�������򣨵�λ��ǰ��
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
//            OLED_WR_CMD(0x02 + x2);//��д����ʼ��ַ����λ
//        else
//            OLED_WR_CMD(0x00 + x2);
//
//        OLED_WR_CMD(0x10 + x1);//����ʼ��ַ����λ
//
//        OLED_WR_CMD(0xB0 + yPos);//������ʾҳ�ĵ�ַ����һҳ��ַΪ0xB0
//
//        for(i = 0; i < width; i++) {
//            OLED_WR_CMD(*dat++);//д��ʾ����
//        }
//
//        yPos++;//����
//    }
//}
