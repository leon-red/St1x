//
// Created by Leon on 2023/5/8.
//

#include "St1x_menu.h"
#include <stdlib.h>
#include "main.h"
#include "u8g2.h"
#include "key.h"
#include "tim.h"

/*****************************************************/
const uint8_t dianzan[] U8X8_PROGMEM = {
        0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x3E,
        0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
        0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00,
        0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00,
        0x7F, 0x00, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00,
        0x00, 0x80, 0x7F, 0x00, 0x00, 0x00, 0xC0, 0x7F,
        0x00, 0x00, 0x00, 0xE0, 0x7F, 0x00, 0x00, 0x00,
        0xF8, 0x7F, 0x00, 0x00, 0xF0, 0xF8, 0xFF, 0xFF,
        0x01, 0xFC, 0xF8, 0xFF, 0xFF, 0x07, 0xFC, 0xF8,
        0xFF, 0xFF, 0x07, 0xFE, 0xF8, 0xFF, 0xFF, 0x07,
        0xFE, 0xF8, 0xFF, 0xFF, 0x07, 0xFE, 0xF8, 0xFF,
        0xFF, 0x07, 0xFE, 0xF8, 0xFF, 0xFF, 0x07, 0xFE,
        0xF8, 0xFF, 0xFF, 0x07, 0xFE, 0xF8, 0xFF, 0xFF,
        0x03, 0xFE, 0xF8, 0xFF, 0xFF, 0x03, 0xFE, 0xF8,
        0xFF, 0xFF, 0x03, 0xFE, 0xF8, 0xFF, 0xFF, 0x03,
        0xFE, 0xF8, 0xFF, 0xFF, 0x01, 0xFE, 0xF8, 0xFF,
        0xFF, 0x01, 0xFE, 0xF8, 0xFF, 0xFF, 0x01, 0xFE,
        0xF8, 0xFF, 0xFF, 0x01, 0xFE, 0xF8, 0xFF, 0xFF,
        0x00, 0xFE, 0xF8, 0xFF, 0xFF, 0x00, 0xFC, 0xF8,
        0xFF, 0x7F, 0x00, 0xFC, 0xF8, 0xFF, 0x3F, 0x00,
        0xF8, 0xF8, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*"����",0*/
        /* (36 X 35 )*/
};

const uint8_t shoucang[] U8X8_PROGMEM = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00,
        0x00, 0x1F, 0x00, 0x00, 0x00, 0x80, 0x1F, 0x00,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0xC0,
        0x3F, 0x00, 0x00, 0x00, 0xC0, 0x3F, 0x00, 0x00,
        0x00, 0xC0, 0x7F, 0x00, 0x00, 0x00, 0xE0, 0xFF,
        0x00, 0x00, 0x00, 0xF0, 0xFF, 0x01, 0x00, 0x00,
        0xFC, 0xFF, 0x03, 0x00, 0xE0, 0xFF, 0xFF, 0xFF,
        0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0x07, 0xFE, 0xFF,
        0xFF, 0xFF, 0x07, 0xFC, 0xFF, 0xFF, 0xFF, 0x07,
        0xFC, 0xFF, 0xFF, 0xFF, 0x03, 0xF8, 0xFF, 0xFF,
        0xFF, 0x01, 0xF0, 0xFF, 0xFF, 0xFF, 0x00, 0xE0,
        0xFF, 0xFF, 0x7F, 0x00, 0xC0, 0xFF, 0xFF, 0x3F,
        0x00, 0x80, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0xFF,
        0xFF, 0x1F, 0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00,
        0x00, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0xFF, 0xFF,
        0x0F, 0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x00, 0x00,
        0xFF, 0xFF, 0x0F, 0x00, 0x00, 0xFF, 0xFF, 0x0F,
        0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0xFF,
        0xFF, 0x1F, 0x00, 0x80, 0xFF, 0xF0, 0x1F, 0x00,
        0x80, 0x3F, 0xC0, 0x1F, 0x00, 0x80, 0x1F, 0x00,
        0x1F, 0x00, 0x00, 0x07, 0x00, 0x1C, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00,/*"�ղ�",0*/
        /* (36 X 37 )*/

};


const uint8_t toubi[] U8X8_PROGMEM = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x1F,
        0x00, 0x00, 0x00, 0xF0, 0xFF, 0x01, 0x00, 0x00,
        0xFC, 0xFF, 0x07, 0x00, 0x00, 0xFF, 0xFF, 0x0F,
        0x00, 0x80, 0xFF, 0xFF, 0x1F, 0x00, 0xC0, 0xFF,
        0xFF, 0x7F, 0x00, 0xE0, 0x07, 0x00, 0x7C, 0x00,
        0xF0, 0x03, 0x00, 0xFC, 0x00, 0xF0, 0x03, 0x00,
        0xFC, 0x01, 0xF8, 0xFF, 0xF1, 0xFF, 0x01, 0xF8,
        0xFF, 0xF1, 0xFF, 0x03, 0xF8, 0x7F, 0xC0, 0xFF,
        0x03, 0xFC, 0x1F, 0x00, 0xFF, 0x03, 0xFC, 0x07,
        0x00, 0xFE, 0x07, 0xFC, 0x07, 0x01, 0xFC, 0x07,
        0xFC, 0xC3, 0x31, 0xF8, 0x07, 0xFC, 0xE1, 0xF1,
        0xF8, 0x07, 0xFC, 0xF1, 0xF1, 0xF0, 0x07, 0xFC,
        0xF1, 0xF1, 0xF0, 0x07, 0xFC, 0xF1, 0xF1, 0xF1,
        0x07, 0xFC, 0xF1, 0xF1, 0xF1, 0x07, 0xFC, 0xF1,
        0xF1, 0xF1, 0x03, 0xF8, 0xF1, 0xF1, 0xF1, 0x03,
        0xF8, 0xFF, 0xF1, 0xFF, 0x03, 0xF8, 0xFF, 0xF1,
        0xFF, 0x01, 0xF0, 0xFF, 0xF1, 0xFF, 0x01, 0xF0,
        0xFF, 0xF1, 0xFF, 0x00, 0xE0, 0xFF, 0xF1, 0x7F,
        0x00, 0xC0, 0xFF, 0xFF, 0x7F, 0x00, 0x80, 0xFF,
        0xFF, 0x3F, 0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x00,
        0x00, 0xFC, 0xFF, 0x07, 0x00, 0x00, 0xF0, 0xFF,
        0x01, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, /*"Ͷ��",0*/
        /* (36 X 36 )*/

};

//
//typedef struct {
//    unsigned char val;         //��ǰ��ȡ�ĵ�ƽ
//    unsigned char last_val;    //��һ�εĵ�ƽ
//    unsigned char change;      //��Ǹı�
//    unsigned char press_tick;  //����ʱ��
//    unsigned char long_press_flag; //������־ ���������ɿ���ʱ���⵽
//} KEY_T;
//typedef struct {
//    unsigned char id;        //������
//    unsigned char press;       //�Ƿ���
//    unsigned char update_flag; //�Ƿ�����
//    unsigned char long_press;  //�Ƿ񳤰�
//} KEY_MSG;
//
//typedef struct {
//    char *str;    //����
//    unsigned char len;     //����
//} SETTING_LIST;
//
SETTING_LIST list[] = //�����б�
        {
                {"list", 4},
                {"ab",   2},
                {"abc",  3},
        };

short menu_x, menu_x_trg; //�˵�x �Ͳ˵���xĿ��
short menu_y, menu_y_trg; //�˵�y �Ͳ˵���yĿ��


short pic_x, pic_x_trg; //ͼƬҳ��x ��ͼƬ��xĿ��
short pic_y, pic_y_trg; //ͼƬҳ��y ��ͼƬ��yĿ��

short setting_x = 0, setting_x_trg = 0; //���ý���x �����ý���xĿ��
short setting_y = 18, setting_y_trg = 18; //���ý���y �����ý���yĿ��

short frame_len, frame_len_trg; //���Ŀ��
short frame_y, frame_y_trg; //����y

enum {
    E_MENU,   //�˵�����
    E_PIC,    //ͼƬ����
    E_SETTING,  //���ý���
    E_UI_MAX, //�������ֵ
} E_UI;

enum {
    E_SELECT_SETTING, //�˵������ѡ��->����
    E_SELECT_PIC,     //�˵������ѡ��->ͼƬ
};

enum {
    E_NONE,           //����״̬�� ��
    E_STATE_MENU,     //����״̬->�˵�����
    E_STATE_RUN_MENU, //����״̬->����˵�
    E_STATE_RUN_SETTING,//����״̬->��������
    E_STATE_RUN_PIC,//����״̬->����ͼƬ
    E_STATE_ARRIVE, //����״̬->����
};

char ui_select = 0; //��ǰUI��ѡ��
char ui_index = 0; //��ǰUI���ĸ�����
char ui_state = 0;//UI״̬��

int key_long_press_tick = 0;//������������
KEY_T key[2] = {0}; //��������
KEY_MSG key_msg = {0};//������Ϣ

unsigned char get_io_val(unsigned char ch) //��ȡio�ĵ�ƽ
{
    if (ch == 0)            //ͨ��0 ����2
    {
        return HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin);
//        return digitalRead(2);
    } else {
        return HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port, KEY_DOWN_Pin);
//        return digitalRead(3);  //ͨ��1 ����3
    }
}


void key_init(void) //������ʼ��
{
    for (int i = 0; i < 2; i++) {
        key[i].val = key[i].last_val = get_io_val(i);//��������ʼ�����ϵ�ʱ���״̬
    }
}

void key_scan(void) {
    for (int i = 0; i < 2; i++) {
        key[i].val = get_io_val(i); //��ȡ��ǰ�İ���״̬
        if (key[i].val != key[i].last_val) //������������ı�
        {
            key[i].last_val = key[i].val; //����״̬
            key[i].change = 1;            //��¼�ı�
        }
    }
}


void key_proc(void) {
    for (int i = 0; i < 2; i++) {
        if (key[i].change == 1) //��������ı��� ������Ϊ�а�������/�ɿ���
        {
            key[i].change = 0;//�����־
            if (key[i].val == 0)//�������
            {
                key_long_press_tick = 10; //��ʼ��������ʱ
            } else //����ɿ�
            {
                if (key[i].long_press_flag) //���������
                {
                    key[i].long_press_flag = 0; //����ɿ���־
                    key[i].press_tick = 0;//���̧�����
                } else {
                    key[i].press_tick = 1;//���û���� ����Ϊ�Ƕ̰� ����
                }
            }
        }
    }
}

void key_down_cb(void)//�˺����ǰ����ɿ����м��� ��Ϊ�����̰�
{
    for (int i = 0; i < 2; i++) {
        if (key[i].press_tick) //����а�������
        {
            key[i].press_tick--;  //������һ
            if (!key[i].press_tick) //���Ϊ0
            {
                key_msg.id = i; //���Ͷ̰�msg
                key_msg.press = 1;
                key_msg.update_flag = 1;
                key_msg.long_press = 0;
            }
        }
    }
}

void key_press_cb(void) //�����ص� 1ms����һ��
{
    if (key_long_press_tick != 0)//�������
    {
        key_long_press_tick--;//--
        if (key_long_press_tick == 0) //�����ʱʱ�����
        {
            for (int i = 0; i < 2; i++) {
                if (key[i].val == 0)//�������� ������а��µ� ��֤������Ϊ����
                {
                    key_msg.id = i;
                    key_msg.press = 1;
                    key_msg.update_flag = 1;
                    key_msg.long_press = 1;
                    key[i].long_press_flag = 1; //���ͳ���msg
                }
            }
        }
    }
    key_down_cb();
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    if(htim->Instance==TIM1){
        HAL_GPIO_TogglePin(KEY_UP_GPIO_Port,KEY_UP_Pin);//����Ĵ��������Ŀ�����޸�
        HAL_GPIO_TogglePin(KEY_DOWN_GPIO_Port,KEY_DOWN_Pin);//����Ĵ��������Ŀ�����޸�
    }
}


//unsigned long millis()
//{
//    unsigned long m;
//    uint8_t oldSREG = SREG;
//
//    cli();
//    m = timer0_millis;
//    SREG = oldSREG;
//
//    return m;
//}

//void system_tick(void)//ϵͳ1msʱ��
//{
//    static unsigned long tick = 0;
//    if (tick != millis())
//    if (tick != millis()) {
//        tick = millis();
//        key_scan();
//        key_press_cb();
//    }
//}

int ui_run(short *a, short *a_trg, uint8_t step, uint8_t slow_cnt) //ui ����Ч��ʵ�� ǰ�潲��
{
    uint8_t temp;

    temp = abs(*a_trg - *a) > slow_cnt ? step : 1;
    if (*a < *a_trg) {
        *a += temp;
    } else if (*a > *a_trg) {
        *a -= temp;
    } else {
        return 0;
    }
    return 1;
}

//void setup(void)
//{
//    u8g2.begin();
//    u8g2_SetFont(u8g2_font_t0_22_mf); //��������
//    frame_len = frame_len_trg = list[ui_select].len * 13; //��ʼ������ĳ���
//    ui_index = E_MENU;//����UIĬ�Ͻ���˵�
//}

void menu_ui_show(short offset_y) //�˵�ui��ʾ
{
    u8g2_DrawXBMP(&u8g2, menu_x - 40, offset_y + 20, 36, 35, dianzan);
    u8g2_DrawXBMP(&u8g2, menu_x + 45, offset_y + 20, 36, 36, toubi);
}

void setting_ui_show(short offset_y) //����UI��ʾ
{
    int list_len = sizeof(list) / sizeof(SETTING_LIST);

    for (int i = 0; i < list_len; i++) {
        u8g2_DrawStr(&u8g2, setting_x + 2, offset_y + i * 20 + 18, list[i].str); // ��һ�����λ��
    }
    u8g2_SetDrawColor(&u8g2, 2);
    u8g2_DrawRBox(&u8g2, setting_x, frame_y, frame_len, 22, 3);
    u8g2_SetDrawColor(&u8g2, 1);

//    u8g2_DrawRFrame(&u8g2,setting_x, offset_y + frame_y, frame_len, 22, 3);
    ui_run(&frame_y, &frame_y_trg, 5, 4);
    ui_run(&frame_len, &frame_len_trg, 10, 5);
}

void pic_ui_show(short offset_y) //ͼƬUI��ʾ
{
    u8g2_DrawXBMP(&u8g2, 40, offset_y + 20, 36, 37, shoucang);
}

void menu_proc(KEY_MSG *msg) //�˵�������
{
    int list_len = 2;//�˵���Ԫ�ظ���

    if (msg->update_flag && msg->press) //����а�������
    {
        msg->update_flag = 0;//���������־
        if (msg->long_press == 0)//����Ƕ̰�
        {
            ui_state = E_STATE_MENU;//״̬������Ϊ�˵�����״̬
            if (msg->id)//�жϰ���id
            {
                ui_select++;//����
                if (ui_select >= list_len) {
                    ui_select = list_len - 1;
                }
            } else {
                ui_select--;//����
                if (ui_select < 0) {
                    ui_select = 0;
                }
            }
            menu_x_trg = ui_select * 90;//�޸Ĳ˵���xĿ��ֵ ʵ�ֲ˵�����
        } else {
            msg->long_press = 1;//�������
            if (msg->id == 1)//�жϰ���id
            {
                if (ui_select == E_SELECT_SETTING)//���Ŀ��uiΪ����
                {
                    ui_index = E_SETTING;//�ѵ�ǰ��UI��Ϊ����ҳ�棬һ��������Ĵ�����������
                    ui_state = E_STATE_RUN_SETTING; //UI״̬����Ϊ ��������
                    menu_y_trg = -64;//���ò˵������Ŀ��
                    setting_y_trg = 0;//�������ý����Ŀ��
                    setting_y = 64;//�������ý���ĳ�ʼֵ
                    ui_select = 0;
                } else if (ui_select == E_SELECT_PIC)//�����ͼƬ
                {
                    ui_index = E_PIC;//ͬ��
                    ui_state = E_STATE_RUN_PIC;//ͬ��
                    menu_y_trg = -64;
                    pic_y_trg = 0;
                    pic_y = 64;
                }
            }
        }
    }
    switch (ui_state) //�˵�����UI״̬��
    {
        case E_STATE_MENU://�˵�״̬
        {
            ui_run(&menu_x, &menu_x_trg, 10, 4);//ֻ����x��
            menu_ui_show(0);//��ʾ
            break;
        }

        case E_STATE_RUN_SETTING://������������
        {
            static uint8_t flag = 0;//����һ��������������ж��Ƿ�λ
            if (ui_run(&setting_y, &setting_y_trg, 10, 4) == 0) {
                flag |= 0xf0;//������õ�λ�� ����0xF0
            }
            if (ui_run(&menu_y, &menu_y_trg, 10, 4) == 0) {
                flag |= 0x0f;//����˵���λ�� ����0x0F
            }
            if (flag == 0xff)// 0 | 0xf0 | 0x0f = 0xff
            { //�����λ�� ״̬��Ϊ��λ
                flag = 0;             //�����־
                ui_state = E_STATE_ARRIVE;
            }
            menu_ui_show(menu_y);//��ʾ�˵�UI
            setting_ui_show(setting_y);//��ʾ����ui
            break;
        }
        case E_STATE_RUN_PIC://ͬ��
        {
            static uint8_t flag = 0;
            if (ui_run(&pic_y, &pic_y_trg, 10, 4) == 0) {
                flag |= 0xf0;
            }
            if (ui_run(&menu_y, &menu_y_trg, 10, 4) == 0) {
                flag |= 0x0f;
            }
            if (flag == 0xff) {
                flag = 0;
                ui_state = E_STATE_ARRIVE;
            }
            menu_ui_show(menu_y);
            pic_ui_show(pic_y);
            break;
        }
        default://Ĭ�� ��λ����none ��ʾ�˵�ui
            menu_ui_show(menu_y);
            break;
    }
}

void pic_proc(KEY_MSG *msg)//����UI��proc��������࣬Ҫע��ui_selectΪ���پͻ������ĸ�proc
{

    if (msg->update_flag && msg->press) {
        msg->update_flag = 0;
        if (msg->long_press == 1) {
            msg->long_press = 0;
            if (msg->id == 1) {

            } else {
                ui_index = E_MENU;
                ui_state = E_STATE_RUN_PIC;
                menu_y_trg = -0;
                pic_y_trg = 64;
            }

        }
    }
    switch (ui_state) {
        case E_STATE_RUN_PIC: {
            static uint8_t pic_flag = 0;
            if (ui_run(&pic_y, &pic_y_trg, 10, 4) == 0) {
                pic_flag |= 0xf0;
            }
            if (ui_run(&menu_y, &menu_y_trg, 10, 4) == 0) {
                pic_flag |= 0x0f;
            }
            if (pic_flag == 0xff) {
                pic_flag = 0;
                ui_state = E_STATE_ARRIVE;
            }
            break;
        }
    }
    menu_ui_show(menu_y);
    pic_ui_show(pic_y);
}


void setting_proc(KEY_MSG *msg) {
    int list_len = sizeof(list) / sizeof(SETTING_LIST);

    if (msg->update_flag && msg->press) {
        msg->update_flag = 0;
        if (msg->long_press == 0) {
            if (msg->id) {
                ui_select++;
                if (ui_select >= list_len) {
                    ui_select = list_len - 1;
                }
            } else {
                ui_select--;
                if (ui_select < 0) {
                    ui_select = 0;
                }
            }
            frame_y_trg = ui_select * 20;
            frame_len_trg = list[ui_select].len * 13;
        } else {
            msg->long_press = 0;
            if (msg->id == 1) {

            } else {
                ui_index = E_MENU;
                ui_state = E_STATE_RUN_SETTING;
                menu_y_trg = -0;
                setting_y_trg = 64;
                ui_select = 0;
            }

        }
    }

    switch (ui_state) {
        case E_STATE_RUN_SETTING: {
            static uint8_t flag = 0;
            if (ui_run(&setting_y, &setting_y_trg, 10, 4) == 0) {
                flag |= 0xf0;
            }
            if (ui_run(&menu_y, &menu_y_trg, 10, 4) == 0) {
                flag |= 0x0f;
            }
            if (flag == 0xff) {
                flag = 0;
                ui_state = E_STATE_ARRIVE;
            }
            break;
        }

    }
    menu_ui_show(menu_y);
    setting_ui_show(setting_y);
}

typedef struct {
    uint8_t index;//UI������
    void (*cb)(KEY_MSG *);//ui��ִ�к���
} UI_LIST;

UI_LIST ui_list[] = //UI��
        {
                {E_MENU,    menu_proc},
                {E_PIC,     pic_proc},
                {E_SETTING, setting_proc},
        };

void ui_proc(KEY_MSG *msg) {
    int i;

    for (i = 0; i < E_UI_MAX; i++) {
        if (ui_index == ui_list[i].index)//�����ǰ��������UI���е�����
        {
            if (ui_list[i].cb)//ִ��UI��Ӧ�Ļص�����
            {
                u8g2_ClearBuffer(&u8g2);         // ����ڲ�������
                ui_list[i].cb(msg);
                u8g2_SendBuffer(&u8g2);          // transfer internal memory to the
            }
        }
    }
}

void St1x_main_menu(void){
    key_proc();
    ui_proc(&key_msg);
}
//void loop(void)
//{
//    key_proc(); //����ɨ��
//    system_tick();//ϵͳtick
//    ui_proc(&key_msg);
//}
