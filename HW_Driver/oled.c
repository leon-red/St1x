#include "oled.h"
#include "oledfont.h"
#include "ASCII_8x16.h"
#include "main.h"
#include "spi.h"

uint8_t OLED_GRAM[96][16];

//���Ժ���
void OLED_ColorTurn(uint8_t i)
{
	if(i==0)
		{
			OLED_WR_Byte(0xA6,OLED_CMD);//������ʾ
		}
	if(i==1)
		{
			OLED_WR_Byte(0xA7,OLED_CMD);//��ɫ��ʾ
		}
}

//��Ļ��ת180��
void OLED_DisplayTurn(uint8_t i)
{
	if(i==0)
	{
		OLED_WR_Byte(0xD3,OLED_CMD); /*set display offset*/
		OLED_WR_Byte(0x68,OLED_CMD); /*18*/
		OLED_WR_Byte(0xC0,OLED_CMD);//������ʾ
		OLED_WR_Byte(0xA0,OLED_CMD);
	}
	if(i==1)
	{
		OLED_WR_Byte(0xD3,OLED_CMD); /*set display offset*/
		OLED_WR_Byte(0x18,OLED_CMD); /*18*/
		OLED_WR_Byte(0xC8,OLED_CMD);//��ת��ʾ
		OLED_WR_Byte(0xA1,OLED_CMD);
	}
}

void OLED_WR_Byte(uint8_t dat,uint8_t cmd)
{
	if(cmd)
	  OLED_DC_Set();
	else 
	  OLED_DC_Clr();		  
	OLED_CS_Clr();
    HAL_SPI_Transmit_DMA(&hspi2,&dat,1);
	OLED_CS_Set();
	OLED_DC_Set();
}

//����OLED��ʾ 
void OLED_DisPlay_On(void)
{
	OLED_WR_Byte(0x8D,OLED_CMD);//��ɱ�ʹ��
	OLED_WR_Byte(0x14,OLED_CMD);//������ɱ�
	OLED_WR_Byte(0xAF,OLED_CMD);//������Ļ
}

//�ر�OLED��ʾ 
void OLED_DisPlay_Off(void)
{
	OLED_WR_Byte(0x8D,OLED_CMD);//��ɱ�ʹ��
	OLED_WR_Byte(0x10,OLED_CMD);//�رյ�ɱ�
	OLED_WR_Byte(0xAE,OLED_CMD);//�ر���Ļ
}

//�����Դ浽OLED
void OLED_Refresh(void)
{
	uint8_t i,n;
	for(i=0;i<16;i++)
	{
		 OLED_WR_Byte(0xb0+i,OLED_CMD); //��������ʼ��ַ
		 OLED_WR_Byte(0x00,OLED_CMD);   //���õ�����ʼ��ַ
		 OLED_WR_Byte(0x10,OLED_CMD);   //���ø�����ʼ��ַ
	   for(n=0;n<80;n++)
		 OLED_WR_Byte(OLED_GRAM[n][i],OLED_DATA);
  }
}

//��������
void OLED_Clear(void)
{
	uint8_t i,n;
	for(i=0;i<16;i++)
	{
	   for(n=0;n<80;n++)
			{
			 OLED_GRAM[n][i]=0;//�����������
			}
  }
	OLED_Refresh();//������ʾ
}

//���� 
//x:0~127
//y:0~63
//t:1 ��� 0,���
void OLED_DrawPoint(uint8_t x,uint8_t y,uint8_t t)
{
	uint8_t i,m,n;
    uint8_t x0=x,y0=y;
    if(HORIZONTAL==90)
    {
        x=79-y0;
        y=x0;
    }
    else if(HORIZONTAL==270)
    {
        x=y0;
        y=127-x0;
    }
	i=y/8;
	m=y%8;
	n=1<<m;
	if(t){OLED_GRAM[x][i]|=n;}
	else
	{
		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
		OLED_GRAM[x][i]|=n;
		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
	}
}

//����
//x1,y1:�������
//x2,y2:��������
void OLED_DrawLine(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t mode)
{
	uint16_t t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //������������ 
	delta_y=y2-y1;
	uRow=x1;//�����������
	uCol=y1;
	if(delta_x>0)incx=1; //���õ������� 
	else if (delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//ˮƽ�� 
	else {incy=-1;delta_y=-delta_x;}
	if(delta_x>delta_y)distance=delta_x; //ѡȡ�������������� 
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		OLED_DrawPoint(uRow,uCol,mode);//����
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}
//x,y:Բ������
//r:Բ�İ뾶
void OLED_DrawCircle(uint8_t x,uint8_t y,uint8_t r)
{
	int a, b,num;
    a = 0;
    b = r;
    while(2 * b * b >= r * r)      
    {
        OLED_DrawPoint(x + a, y - b,1);
        OLED_DrawPoint(x - a, y - b,1);
        OLED_DrawPoint(x - a, y + b,1);
        OLED_DrawPoint(x + a, y + b,1);
 
        OLED_DrawPoint(x + b, y + a,1);
        OLED_DrawPoint(x + b, y - a,1);
        OLED_DrawPoint(x - b, y - a,1);
        OLED_DrawPoint(x - b, y + a,1);
        
        a++;
        num = (a * a + b * b) - r*r;//���㻭�ĵ���Բ�ĵľ���
        if(num > 0)
        {
            b--;
            a--;
        }
    }
}



//��ָ��λ����ʾһ���ַ�,���������ַ�
//x:0~127
//y:0~63
//size1:ѡ������ 6x8/6x12/8x16/12x24
//mode:0,��ɫ��ʾ;1,������ʾ
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size1,uint8_t mode)
{
	uint8_t i,m,temp,size2,chr1;
	uint8_t x0=x,y0=y;
	if(size1==8)size2=6;
	else size2=(size1/8+((size1%8)?1:0))*(size1/2);  //�õ�����һ���ַ���Ӧ������ռ���ֽ���
	chr1=chr-' ';  //����ƫ�ƺ��ֵ
	for(i=0;i<size2;i++)
	{
		if(size1==8)
			  {temp=asc2_0806[chr1][i];} //����0806����
		else if(size1==12)
        {temp=asc2_1206[chr1][i];} //����1206����
		else if(size1==16)
        {temp=asc2_1608[chr1][i];} //����1608����
		else if(size1==24)
        {temp=asc2_2412[chr1][i];} //����2412����
		else return;
		for(m=0;m<8;m++)
		{
			if(temp&0x01)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp>>=1;
			y++;
		}
		x++;
		if((size1!=8)&&((x-x0)==size1/2))
		{x=x0;y0=y0+8;}
		y=y0;
  }
}


//��ʾ�ַ���
//x,y:�������  
//size1:�����С 
//*chr:�ַ�����ʼ��ַ 
//mode:0,��ɫ��ʾ;1,������ʾ
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *chr,uint8_t size1,uint8_t mode)
{
	while((*chr>=' ')&&(*chr<='~'))//�ж��ǲ��ǷǷ��ַ�!
	{
		OLED_ShowChar(x,y,*chr,size1,mode);
		if(size1==8)x+=6;
		else x+=size1/2;
		chr++;
  }
}

//m^n
uint32_t OLED_Pow(uint8_t m,uint8_t n)
{
	uint32_t result=1;
	while(n--)
	{
	  result*=m;
	}
	return result;
}

//��ʾ����
//x,y :�������
//num :Ҫ��ʾ������
//len :���ֵ�λ��
//size:�����С
//mode:0,��ɫ��ʾ;1,������ʾ
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size1,uint8_t mode)
{
	uint8_t t,temp,m=0;
	if(size1==8)m=2;
	for(t=0;t<len;t++)
	{
		temp=(num/OLED_Pow(10,len-t-1))%10;
			if(temp==0)
			{
				OLED_ShowChar(x+(size1/2+m)*t,y,'0',size1,mode);
      }
			else 
			{
			  OLED_ShowChar(x+(size1/2+m)*t,y,temp+'0',size1,mode);
			}
  }
}

//��ʾ����
//x,y:�������
//num:���ֶ�Ӧ�����
//mode:0,��ɫ��ʾ;1,������ʾ
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t num,uint8_t size1,uint8_t mode)
{
	uint8_t m,temp;
	uint8_t x0=x,y0=y;
	uint16_t i,size3=(size1/8+((size1%8)?1:0))*size1;  //�õ�����һ���ַ���Ӧ������ռ���ֽ���
	for(i=0;i<size3;i++)
	{
		if(size1==16)
				{temp=Hzk1[num][i];}//����16*16����
		else if(size1==24)
				{temp=Hzk2[num][i];}//����24*24����
		else if(size1==32)       
				{temp=Hzk3[num][i];}//����32*32����
		else if(size1==64)
				{temp=Hzk4[num][i];}//����64*64����
		else return;
		for(m=0;m<8;m++)
		{
			if(temp&0x01)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp>>=1;
			y++;
		}
		x++;
		if((x-x0)==size1)
		{x=x0;y0=y0+8;}
		y=y0;
	}
}

//num ��ʾ���ֵĸ���
//space ÿһ����ʾ�ļ��
//mode:0,��ɫ��ʾ;1,������ʾ
void OLED_ScrollDisplay(uint8_t num,uint8_t space,uint8_t mode)
{
	uint8_t i,n,t=0,m=0,r;
	while(1)
	{
		if(m==0)
		{
	    OLED_ShowChinese(80,48,t,16,mode); //д��һ�����ֱ�����OLED_GRAM[][]������
			t++;
		}
		if(t==num)
			{
				for(r=0;r<16*space;r++)      //��ʾ���
				 {
					for(i=1;i<96;i++)
						{
							for(n=0;n<16;n++)
							{
								OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
							}
						}
           OLED_Refresh();
				 }
        t=0;
      }
		m++;
		if(m==16){m=0;}
		for(i=1;i<96;i++)   //ʵ������
		{
			for(n=0;n<16;n++)
			{
				OLED_GRAM[i-1][n]=OLED_GRAM[i][n];
			}
		}
		OLED_Refresh();
	}
}

//x,y���������
//sizex,sizey,ͼƬ����
//BMP[]��Ҫд���ͼƬ����
//mode:0,��ɫ��ʾ;1,������ʾ
void OLED_ShowPicture(uint8_t x, uint8_t y, uint8_t sizex, uint8_t sizey, uint8_t BMP[], uint8_t mode) {
    uint16_t j = 0;
    uint8_t i, n, temp, m;
    uint8_t x0 = x, y0 = y;
    sizey = sizey / 8 + ((sizey % 8) ? 1 : 0);
    for (n = 0; n < sizey; n++) {
        for (i = 0; i < sizex; i++) {
            temp = BMP[j];
            j++;
            for (m = 0; m < 8; m++) {
                if (temp & 0x01)OLED_DrawPoint(x, y, mode);
                else OLED_DrawPoint(x, y, !mode);
                temp >>= 1;
                y++;
            }
            x++;
            if ((x - x0) == sizex) {
                x = x0;
                y0 = y0 + 8;
            }
            y = y0;
        }
    }
}

//OLED�ĳ�ʼ��
void OLED_Init(void)
{
	OLED_RES_Set();
	OLED_WR_Byte(0xAE,OLED_CMD); /*display off*/
	OLED_WR_Byte(0x00,OLED_CMD); /*set lower column address*/
	OLED_WR_Byte(0x10,OLED_CMD); /*set higher column address*/
	OLED_WR_Byte(0x20,OLED_CMD); /* Set Memory addressing mode (0x20/0x21) */
	OLED_WR_Byte(0x81,OLED_CMD); /*contract control*/
	OLED_WR_Byte(0x6f,OLED_CMD); /*b0*/
	OLED_WR_Byte(0xA0,OLED_CMD); /*set segment remap*/
	OLED_WR_Byte(0xC0,OLED_CMD); /*Com scan direction*/
	OLED_WR_Byte(0xA4,OLED_CMD); /*Disable Entire Display On (0xA4/0xA5)*/
	OLED_WR_Byte(0xA6,OLED_CMD); /*normal / reverse*/
	OLED_WR_Byte(0xD5,OLED_CMD); /*set osc division*/
	OLED_WR_Byte(0x91,OLED_CMD);
	OLED_WR_Byte(0xD9,OLED_CMD); /*set pre-charge period*/
	OLED_WR_Byte(0x22,OLED_CMD);
	OLED_WR_Byte(0xdb,OLED_CMD); /*set vcomh*/
	OLED_WR_Byte(0x3f,OLED_CMD);
	OLED_WR_Byte(0xA8,OLED_CMD); /*multiplex ratio*/
	OLED_WR_Byte(0x4F,OLED_CMD); /*duty = 1/80*/
	OLED_WR_Byte(0xD3,OLED_CMD); /*set display offset*/
	OLED_WR_Byte(0x68,OLED_CMD); /*18*/
	OLED_WR_Byte(0xdc,OLED_CMD); /*Set Display Start Line*/
	OLED_WR_Byte(0x00,OLED_CMD);
	OLED_WR_Byte(0xad,OLED_CMD); /*set charge pump enable*/
	OLED_WR_Byte(0x8a,OLED_CMD); /*Set DC-DC enable (a=0:disable; a=1:enable) */
	OLED_Clear();
	OLED_WR_Byte(0xAF,OLED_CMD); /*display ON*/ 
}

//��ʾӢ��������8*16��ASCII��
//ȡģ��СΪ8*16��ȡģ��ʽΪ�������Ҵ��ϵ��¡�������8���¸�λ��
void OLED_DISPLAY_8x16(uint8_t x, //��ʾ���ֵ�ҳ���꣨��0��7�����˴������޸ģ�
                       uint8_t y, //��ʾ���ֵ������꣨��0��128��
                       uint16_t w) { //Ҫ��ʾ���ֵı��
    uint8_t j, t, c = 0;
    uint8_t buf[4];
    y = y + 2; //��OLED������������оƬ�Ǵ�0x02����Ϊ��������һ�У�����Ҫ����ƫ����
    for (t = 0; t < 2; t++) {
        buf[0] = 0x00 + x;
        buf[1] = y / 16 + 0x10;
        buf[2] = y % 16;
        OLED_WR_Byte(buf[0], OLED_CMD);
        OLED_WR_Byte(buf[1], OLED_CMD);
        OLED_WR_Byte(buf[2], OLED_CMD);
//        HAL_SPI_Transmit_DMA(&hspi2,&buf[0],1);//ҳ��ַ����0xB0��0xB7��
//        HAL_SPI_Transmit_DMA(&hspi2,&buf[1],1);//��ʼ�е�ַ�ĸ�4λ
//        HAL_SPI_Transmit_DMA(&hspi2,&buf[2],1);//��ʼ�е�ַ�ĵ�4λ
        for (j = 0; j < 8; j++) { //��ҳ�������
            buf[3] = ASCII_8x16[(w * 16) + c - 512];
            OLED_WR_Byte(buf[3], OLED_CMD);
//            HAL_SPI_Transmit_DMA(&hspi2,&buf[3],1);//Ϊ�˺�ASII����ӦҪ��512
            c++;
        }
        x++; //ҳ��ַ��1
    }
}

//��LCM����һ���ַ���,����64�ַ�֮�ڡ�
//Ӧ�ã�OLED_DISPLAY_8_16_BUFFER(0," DoYoung Studio");
void OLED_DISPLAY_8x16_BUFFER(uint8_t row,uint8_t *str){
    uint8_t r=0;
    while(*str != '\0'){
        OLED_DISPLAY_8x16(row,r*8,*str++);
        r++;
    }
}

//OLED Printf
void OLED_printf_US(uint8_t row, uint8_t *str, uint8_t i) {
    uint8_t r = 0;
    while (i != r) {
        OLED_DISPLAY_8x16(row, r * 8, *str++);
        r++;
    }
}

void OLED_printf(uint8_t row, char *fmt, ...)
{
    char buff[17];  //���ڴ��ת��������ݡ����ȡ�
    uint16_t i = 0;
    va_list art_ptr;
    va_start(art_ptr, fmt);
    vsnprintf(buff, 17, fmt, art_ptr);   //����ת��
    i = strlen(buff); //�ó����ݳ���
    if (strlen(buff) > 16)i = 16;   //������ȴ������ֵ ���򳤶ȵ������ֵ��������ֺ��ԣ�
    OLED_printf_US(row, (uint8_t *) buff, i);
    va_end(art_ptr);
}