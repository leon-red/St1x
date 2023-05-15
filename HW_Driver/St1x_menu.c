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
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*"点赞",0*/
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
        0x00,/*"收藏",0*/
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
        0x00, 0x00, 0x00, 0x00, /*"投币",0*/
        /* (36 X 36 )*/

};

//
//typedef struct {
//    unsigned char val;         //当前读取的电平
//    unsigned char last_val;    //上一次的电平
//    unsigned char change;      //标记改变
//    unsigned char press_tick;  //按下时间
//    unsigned char long_press_flag; //长按标志 用来屏蔽松开的时候检测到
//} KEY_T;
//typedef struct {
//    unsigned char id;        //按键号
//    unsigned char press;       //是否按下
//    unsigned char update_flag; //是否最新
//    unsigned char long_press;  //是否长按
//} KEY_MSG;
//
//typedef struct {
//    char *str;    //内容
//    unsigned char len;     //字数
//} SETTING_LIST;
//
SETTING_LIST list[] = //设置列表
        {
                {"list", 4},
                {"ab",   2},
                {"abc",  3},
        };

short menu_x, menu_x_trg; //菜单x 和菜单的x目标
short menu_y, menu_y_trg; //菜单y 和菜单的y目标


short pic_x, pic_x_trg; //图片页面x 和图片的x目标
short pic_y, pic_y_trg; //图片页面y 和图片的y目标

short setting_x = 0, setting_x_trg = 0; //设置界面x 和设置界面x目标
short setting_y = 18, setting_y_trg = 18; //设置界面y 和设置界面y目标

short frame_len, frame_len_trg; //框框的宽度
short frame_y, frame_y_trg; //框框的y

enum {
    E_MENU,   //菜单界面
    E_PIC,    //图片界面
    E_SETTING,  //设置界面
    E_UI_MAX, //界面最大值
} E_UI;

enum {
    E_SELECT_SETTING, //菜单界面的选择->设置
    E_SELECT_PIC,     //菜单界面的选择->图片
};

enum {
    E_NONE,           //运行状态机 无
    E_STATE_MENU,     //运行状态->菜单运行
    E_STATE_RUN_MENU, //运行状态->跑向菜单
    E_STATE_RUN_SETTING,//运行状态->跑向设置
    E_STATE_RUN_PIC,//运行状态->跑向图片
    E_STATE_ARRIVE, //运行状态->到达
};

char ui_select = 0; //当前UI的选项
char ui_index = 0; //当前UI在哪个界面
char ui_state = 0;//UI状态机

int key_long_press_tick = 0;//按键长按计数
KEY_T key[2] = {0}; //按键对象
KEY_MSG key_msg = {0};//按键消息

unsigned char get_io_val(unsigned char ch) //获取io的电平
{
    if (ch == 0)            //通道0 引脚2
    {
        return HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin);
//        return digitalRead(2);
    } else {
        return HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port, KEY_DOWN_Pin);
//        return digitalRead(3);  //通道1 引脚3
    }
}


void key_init(void) //按键初始化
{
    for (int i = 0; i < 2; i++) {
        key[i].val = key[i].last_val = get_io_val(i);//将按键初始化成上电时候的状态
    }
}

void key_scan(void) {
    for (int i = 0; i < 2; i++) {
        key[i].val = get_io_val(i); //获取当前的按键状态
        if (key[i].val != key[i].last_val) //如果按键发生改变
        {
            key[i].last_val = key[i].val; //更新状态
            key[i].change = 1;            //记录改变
        }
    }
}


void key_proc(void) {
    for (int i = 0; i < 2; i++) {
        if (key[i].change == 1) //如果按键改变了 可以认为有按键按下/松开了
        {
            key[i].change = 0;//清除标志
            if (key[i].val == 0)//如果按下
            {
                key_long_press_tick = 10; //开始长按倒计时
            } else //如果松开
            {
                if (key[i].long_press_flag) //如果长按了
                {
                    key[i].long_press_flag = 0; //清除松开标志
                    key[i].press_tick = 0;//清掉抬起计数
                } else {
                    key[i].press_tick = 1;//如果没长按 则认为是短按 计数
                }
            }
        }
    }
}

void key_down_cb(void)//此函数是按键松开后有计数 认为按键短按
{
    for (int i = 0; i < 2; i++) {
        if (key[i].press_tick) //如果有按键计数
        {
            key[i].press_tick--;  //计数减一
            if (!key[i].press_tick) //如果为0
            {
                key_msg.id = i; //发送短按msg
                key_msg.press = 1;
                key_msg.update_flag = 1;
                key_msg.long_press = 0;
            }
        }
    }
}

void key_press_cb(void) //按键回调 1ms调用一次
{
    if (key_long_press_tick != 0)//如果按下
    {
        key_long_press_tick--;//--
        if (key_long_press_tick == 0) //如果计时时间结束
        {
            for (int i = 0; i < 2; i++) {
                if (key[i].val == 0)//遍历按键 如果还有按下的 则证明按键为长按
                {
                    key_msg.id = i;
                    key_msg.press = 1;
                    key_msg.update_flag = 1;
                    key_msg.long_press = 1;
                    key[i].long_press_flag = 1; //发送长按msg
                }
            }
        }
    }
    key_down_cb();
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    if(htim->Instance==TIM1){
        HAL_GPIO_TogglePin(KEY_UP_GPIO_Port,KEY_UP_Pin);//这里的代码根据项目自行修改
        HAL_GPIO_TogglePin(KEY_DOWN_GPIO_Port,KEY_DOWN_Pin);//这里的代码根据项目自行修改
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

//void system_tick(void)//系统1ms时钟
//{
//    static unsigned long tick = 0;
//    if (tick != millis())
//    if (tick != millis()) {
//        tick = millis();
//        key_scan();
//        key_press_cb();
//    }
//}

int ui_run(short *a, short *a_trg, uint8_t step, uint8_t slow_cnt) //ui 滑动效果实现 前面讲了
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
//    u8g2_SetFont(u8g2_font_t0_22_mf); //设置字体
//    frame_len = frame_len_trg = list[ui_select].len * 13; //初始化方框的长宽
//    ui_index = E_MENU;//设置UI默认进入菜单
//}

void menu_ui_show(short offset_y) //菜单ui显示
{
    u8g2_DrawXBMP(&u8g2, menu_x - 40, offset_y + 20, 36, 35, dianzan);
    u8g2_DrawXBMP(&u8g2, menu_x + 45, offset_y + 20, 36, 36, toubi);
}

void setting_ui_show(short offset_y) //设置UI显示
{
    int list_len = sizeof(list) / sizeof(SETTING_LIST);

    for (int i = 0; i < list_len; i++) {
        u8g2_DrawStr(&u8g2, setting_x + 2, offset_y + i * 20 + 18, list[i].str); // 第一段输出位置
    }
    u8g2_SetDrawColor(&u8g2, 2);
    u8g2_DrawRBox(&u8g2, setting_x, frame_y, frame_len, 22, 3);
    u8g2_SetDrawColor(&u8g2, 1);

//    u8g2_DrawRFrame(&u8g2,setting_x, offset_y + frame_y, frame_len, 22, 3);
    ui_run(&frame_y, &frame_y_trg, 5, 4);
    ui_run(&frame_len, &frame_len_trg, 10, 5);
}

void pic_ui_show(short offset_y) //图片UI显示
{
    u8g2_DrawXBMP(&u8g2, 40, offset_y + 20, 36, 37, shoucang);
}

void menu_proc(KEY_MSG *msg) //菜单处理函数
{
    int list_len = 2;//菜单的元素个数

    if (msg->update_flag && msg->press) //如果有按键按下
    {
        msg->update_flag = 0;//清楚按键标志
        if (msg->long_press == 0)//如果是短按
        {
            ui_state = E_STATE_MENU;//状态机设置为菜单运行状态
            if (msg->id)//判断按键id
            {
                ui_select++;//增加
                if (ui_select >= list_len) {
                    ui_select = list_len - 1;
                }
            } else {
                ui_select--;//减少
                if (ui_select < 0) {
                    ui_select = 0;
                }
            }
            menu_x_trg = ui_select * 90;//修改菜单的x目标值 实现菜单滚动
        } else {
            msg->long_press = 1;//如果长按
            if (msg->id == 1)//判断按键id
            {
                if (ui_select == E_SELECT_SETTING)//如果目标ui为设置
                {
                    ui_index = E_SETTING;//把当前的UI设为设置页面，一会会在他的处理函数中运算
                    ui_state = E_STATE_RUN_SETTING; //UI状态设置为 跑向设置
                    menu_y_trg = -64;//设置菜单界面的目标
                    setting_y_trg = 0;//设置设置界面的目标
                    setting_y = 64;//设置设置界面的初始值
                    ui_select = 0;
                } else if (ui_select == E_SELECT_PIC)//如果是图片
                {
                    ui_index = E_PIC;//同上
                    ui_state = E_STATE_RUN_PIC;//同上
                    menu_y_trg = -64;
                    pic_y_trg = 0;
                    pic_y = 64;
                }
            }
        }
    }
    switch (ui_state) //菜单界面UI状态机
    {
        case E_STATE_MENU://菜单状态
        {
            ui_run(&menu_x, &menu_x_trg, 10, 4);//只运算x轴
            menu_ui_show(0);//显示
            break;
        }

        case E_STATE_RUN_SETTING://运行跑向设置
        {
            static uint8_t flag = 0;//定义一个缓存变量用以判断是否到位
            if (ui_run(&setting_y, &setting_y_trg, 10, 4) == 0) {
                flag |= 0xf0;//如果设置到位了 或上0xF0
            }
            if (ui_run(&menu_y, &menu_y_trg, 10, 4) == 0) {
                flag |= 0x0f;//如果菜单到位了 或上0x0F
            }
            if (flag == 0xff)// 0 | 0xf0 | 0x0f = 0xff
            { //如果到位了 状态置为到位
                flag = 0;             //清除标志
                ui_state = E_STATE_ARRIVE;
            }
            menu_ui_show(menu_y);//显示菜单UI
            setting_ui_show(setting_y);//显示设置ui
            break;
        }
        case E_STATE_RUN_PIC://同上
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
        default://默认 到位或者none 显示菜单ui
            menu_ui_show(menu_y);
            break;
    }
}

void pic_proc(KEY_MSG *msg)//几个UI的proc函数都差不多，要注意ui_select为多少就会运行哪个proc
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
    uint8_t index;//UI的索引
    void (*cb)(KEY_MSG *);//ui的执行函数
} UI_LIST;

UI_LIST ui_list[] = //UI表
        {
                {E_MENU,    menu_proc},
                {E_PIC,     pic_proc},
                {E_SETTING, setting_proc},
        };

void ui_proc(KEY_MSG *msg) {
    int i;

    for (i = 0; i < E_UI_MAX; i++) {
        if (ui_index == ui_list[i].index)//如果当前索引等于UI表中的索引
        {
            if (ui_list[i].cb)//执行UI对应的回调函数
            {
                u8g2_ClearBuffer(&u8g2);         // 清除内部缓冲区
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
//    key_proc(); //按键扫描
//    system_tick();//系统tick
//    ui_proc(&key_msg);
//}
