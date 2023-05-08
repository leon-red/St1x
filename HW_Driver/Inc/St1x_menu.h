//
// Created by Leon on 2023/5/8.
//

#ifndef ST1X_ST1X_MENU_H
#define ST1X_ST1X_MENU_H


typedef struct {
    unsigned char val;         //��ǰ��ȡ�ĵ�ƽ
    unsigned char last_val;    //��һ�εĵ�ƽ
    unsigned char change;      //��Ǹı�
    unsigned char press_tick;  //����ʱ��
    unsigned char long_press_flag; //������־ ���������ɿ���ʱ���⵽
} KEY_T;

typedef struct {
    unsigned char id;        //������
    unsigned char press;       //�Ƿ���
    unsigned char update_flag; //�Ƿ�����
    unsigned char long_press;  //�Ƿ񳤰�
} KEY_MSG;

typedef struct {
    char *str;    //����
    unsigned char len;     //����
} SETTING_LIST;

void key_init(void); //������ʼ��
void key_scan(void);
void key_proc(void);
void key_down_cb(void);//�˺����ǰ����ɿ����м��� ��Ϊ�����̰�
void key_press_cb(void);
void menu_ui_show(short offset_y);
void pic_ui_show(short offset_y);
void menu_proc(KEY_MSG *msg);
void pic_proc(KEY_MSG *msg);
void setting_proc(KEY_MSG *msg);
void ui_proc(KEY_MSG *msg);
void St1x_main_menu(void);

#endif //ST1X_ST1X_MENU_H
