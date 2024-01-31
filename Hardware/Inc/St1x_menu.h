//
// Created by Leon on 2023/5/8.
//

#ifndef ST1X_ST1X_MENU_H
#define ST1X_ST1X_MENU_H


typedef struct {
    unsigned char val;         //当前读取的电平
    unsigned char last_val;    //上一次的电平
    unsigned char change;      //标记改变
    unsigned char press_tick;  //按下时间
    unsigned char long_press_flag; //长按标志 用来屏蔽松开的时候检测到
} KEY_T;

typedef struct {
    unsigned char id;        //按键号
    unsigned char press;       //是否按下
    unsigned char update_flag; //是否最新
    unsigned char long_press;  //是否长按
} KEY_MSG;

typedef struct {
    char *str;    //内容
    unsigned char len;     //字数
} SETTING_LIST;

void key_init(void); //按键初始化
void key_scan(void);
void key_proc(void);
void key_down_cb(void);//此函数是按键松开后有计数 认为按键短按
void key_press_cb(void);
void menu_ui_show(short offset_y);
void pic_ui_show(short offset_y);
void menu_proc(KEY_MSG *msg);
void pic_proc(KEY_MSG *msg);
void setting_proc(KEY_MSG *msg);
void ui_proc(KEY_MSG *msg);
void St1x_main_menu(void);

#endif //ST1X_ST1X_MENU_H
