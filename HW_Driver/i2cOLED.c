#include "i2cOLED.h"
//#include "i2c.h"
//#include "OLEDfont.h"


//uint8_t CMD_Data[]={
//        0xAE,	/*11. Display OFF/ON: (AEH - AFH)*/
//
//        0x00,	/*1. ҳ��ַģʽ����������ʼ��ַ��λ(Set Lower Column Address)(00H - 0FH)*/
//        0x10,	/*2. ҳ��ַģʽ����������ʼ��ַ��λ(Set Higher Column Address)(10H - 17H)*/
//
//        0x20,	/*3. �����ڴ��ַģʽ(Set Memory addressing mode)(20H - 21H)*/
//
//                /*4. ���öԱȶ�(Set Contrast Control Register)(Double Bytes Command)*/
//        0x81,	/*The Contrast Control Mode Set: (81H)*/
//        0x2F,	/*Contrast Data Register Set: (00H - FFH)(00:�Աȶ���ͣ�FF:�Աȶ����)*/
//
//        0xA0,	/*5. Set Segment Re-map: (A0H - A1H) ��ֱ����*/
//
//                /*6. Set Multiplex Ration: (Double Bytes Command)*/
//        0xA8,	/*Multiplex Ration Mode Set: (A8H)*/
//        0x4F,	/*Multiplex Ration Data Set: (00H - 7FH)��duty = 1/80��*/
//
//        0xA4,	/*7. Set Entire Display OFF/ON: (A4H - A5H)*/
//
//        0xA6,	/*8. Set Normal/Reverse Display: (A6H -A7H)(A6���ڵװ��֣�A7���׵׺���)*/
//
//                /*9. ������ʾƫ��(Set Display Offset)(Double Bytes Command)*/
//        0xD3,	/*Display Offset Mode Set: (D3H)*/
//        0x68,	/*Display Offset Data Set: (00H �C 7FH) ��-18��=COM-24��128-25=103��*/
//
//                /*10. Set DC-DC Setting: (Double Bytes Command)*/
//        0xAD,	/*DC-DC Control Mode Set: (ADH)*/
//        0x8A,   /*DC-DC ON/OFF Mode Set: (8AH - 8BH) (8A:disable; 8B:enable) */
//
//        0xC0,	/*13. Set Common Output Scan Direction: (C0H - C8H)ˮƽ����*/
//
//                /*14. Set Display Clock Divide Ratio/Oscillator Frequency: (Double Bytes Command)*/
//        0xD5,	/*Divide Ratio/Oscillator Frequency Mode Set: (D5H)*/
//        0x91,   /*Divide Ratio/Oscillator Frequency Data Set: (00H - FFH)*/
//
//                /*15. ����Ԥ�������(Set Dis-charge/Pre-charge Period)(Double Bytes Command)*/
//        0xD9,	/*Pre-charge Period Mode Set: (D9H)*/
//        0x22,   /*Dis-charge/Pre-charge Period Data Set: (00H - FFH)*/
//
//                /*16. Set VCOM Deselect Level: (Double Bytes Command)*/
//        0xDB,	/*VCOM Deselect Level Mode Set: (DBH)*/
//        0x3F,   /*VCOM Deselect Level Data Set: (00H - FFH) ��ϸ�����뿴����� */
//
//                /*17. ������Ļ��ʼ��(Set Display Start Line)(Double Bytes Command)*/
//        0xDC,	/*The Display Start line Mode Set: (DCH)*/
//        0x00,   /*The Display Start line Register Set: (00H �C 7FH)*/
//
//        0xAF,	/*11. Display OFF/ON: (AEH - AFH)*/



//            //u8g2
//        0xAE, /*display off*/
//        0x00, /*set lower column address*/
//        0x10, /*set higher column address*/
//        0x20, /* Set Memory addressing mode (0x20/0x21) */
//        0x81, /*contract control*/
//        0x6f, /*b0*/
//        0xA0, /*set segment remap*/
//        0xC0, /*Com scan direction*/
//        0xA4, /*Disable Entire Display On (0xA4/0xA5)*/
//        0xA6, /*normal / reverse*/
//        0xD5, /*set osc division*/
//        0x91,
//        0xD9, /*set pre-charge period*/
//        0x22,
//        0xdb, /*set vcomh*/
//        0x3f,
//        0xA8, /*multiplex ratio*/
//        0x4F, /*duty = 1/80*/
//        0xD3, /*set display offset*/
//        0x68, /*18*/
//        0xdc, /*Set Display Start Line*/
//        0x00,
//        0xad, /*set charge pump enable*/
//        0x8a, /*Set DC-DC enable (a=0:disable, a=1:enable) */
//        0xAF, /*display ON*/
//};      //��ʼ������
//
//void WriteCmd(void)
//{
//    uint8_t i = 0;
//    for(i=0; i<27; i++)
//    {
//        HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,CMD_Data+i,1,0x100);
//    }
//}
////���豸д��������
//void OLED_WR_CMD(uint8_t cmd)
//{
//    HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,&cmd,1,0x100);
//}
////���豸д����
//void OLED_WR_DATA(uint8_t data)
//{
//    HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x40,I2C_MEMADD_SIZE_8BIT,&data,1,0x100);
//}
////��ʼ��oled��Ļ
//void OLED_Init(void)
//{
//    HAL_Delay(200);
//    WriteCmd();
//}
////����
//void OLED_Clear(void)
//{
//    uint8_t i,n;
//    for(i=0;i<16;i++)
//    {
//        OLED_WR_CMD(0xb0+i);
//        OLED_WR_CMD (0x00);
//        OLED_WR_CMD (0x10);
//        for(n=0;n<128;n++)
//            OLED_WR_DATA(0);
//    }
//}
////����OLED��ʾ
//void OLED_Display_On(void)
//{
//    OLED_WR_CMD(0X8D);  //SET DCDC����
//    OLED_WR_CMD(0X14);  //DCDC ON
//    OLED_WR_CMD(0XAF);  //DISPLAY ON
//}
////�ر�OLED��ʾ
//void OLED_Display_Off(void)
//{
//    OLED_WR_CMD(0X8D);  //SET DCDC����
//    OLED_WR_CMD(0X10);  //DCDC OFF
//    OLED_WR_CMD(0XAE);  //DISPLAY OFF
//}
//void OLED_Set_Pos(uint8_t x, uint8_t y)
//{
//    OLED_WR_CMD(0xb0+y);
//    OLED_WR_CMD(((x&0xf0)>>4)|0x10);
//    OLED_WR_CMD(x&0x0f);
//}
//
//void OLED_On(void)
//{
//    uint8_t i,n;
//    for(i=0;i<8;i++)
//    {
//        OLED_WR_CMD(0xb0+i);    //����ҳ��ַ��0~7��
//        OLED_WR_CMD(0x00);      //������ʾλ�á��е͵�ַ
//        OLED_WR_CMD(0x10);      //������ʾλ�á��иߵ�ַ
//        for(n=0;n<128;n++)
//            OLED_WR_DATA(1);
//    } //������ʾ
//}
//unsigned int oled_pow(uint8_t m,uint8_t n)
//{
//    unsigned int result=1;
//    while(n--)result*=m;
//    return result;
//}
////��ʾ2������
////x,y :�������
////len :���ֵ�λ��
////size:�����С
////mode:ģʽ	0,���ģʽ;1,����ģʽ
////num:��ֵ(0~4294967295);
//void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t size2)
//{
//    uint8_t t,temp;
//    uint8_t enshow=0;
//    for(t=0;t<len;t++)
//    {
//        temp=(num/oled_pow(10,len-t-1))%10;
//        if(enshow==0&&t<(len-1))
//        {
//            if(temp==0)
//            {
//                OLED_ShowChar(x+(size2/2)*t,y,' ',size2);
//                continue;
//            }else enshow=1;
//
//        }
//        OLED_ShowChar(x+(size2/2)*t,y,temp+'0',size2);
//    }
//}
////��ָ��λ����ʾһ���ַ�,���������ַ�
////x:0~127
////y:0~63
////mode:0,������ʾ;1,������ʾ
////size:ѡ������ 16/12
//void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t Char_Size)
//{
//    unsigned char c=0,i=0;
//    c=chr-' ';//�õ�ƫ�ƺ��ֵ
//    if(x>128-1){x=0;y=y+2;}
//    if(Char_Size ==16)
//    {
//        OLED_Set_Pos(x,y);
//        for(i=0;i<8;i++)
//            OLED_WR_DATA(F8X16[c*16+i]);
//        OLED_Set_Pos(x,y+1);
//        for(i=0;i<8;i++)
//            OLED_WR_DATA(F8X16[c*16+i+8]);
//    }
//    else {
//        OLED_Set_Pos(x,y);
//        for(i=0;i<6;i++)
//            OLED_WR_DATA(F6x8[c][i]);
//
//    }
//}
//
////��ʾһ���ַ��Ŵ�
//void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *chr,uint8_t Char_Size)
//{
//    unsigned char j=0;
//    while (chr[j]!='\0')
//    {		OLED_ShowChar(x,y,chr[j],Char_Size);
//        x+=8;
//        if(x>120){x=0;y+=2;}
//        j++;
//    }
//}
////��ʾ����
////hzk ��ȡģ����ó�������
//void OLED_ShowCHinese(uint8_t x,uint8_t y,uint8_t no)
//{
//    uint8_t t,adder=0;
//    OLED_Set_Pos(x,y);
//    for(t=0;t<16;t++)
//    {
//        OLED_WR_DATA(Hzk[2*no][t]);
//        adder+=1;
//    }
//    OLED_Set_Pos(x,y+1);
//    for(t=0;t<16;t++)
//    {
//        OLED_WR_DATA(Hzk[2*no+1][t]);
//        adder+=1;
//    }
//}
//
