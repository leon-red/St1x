#include "IronKey.h"

volatile uint8_t KeyNum = 0;

//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
//    if (GPIO_Pin == KEY_DOWN_Pin) {
//        KeyNum = 1; // 按键1的按下状态
//    } else if (GPIO_Pin == KEY_UP_Pin) {
//        KeyNum = 2; // 按键2的按下状态
//    } else if (GPIO_Pin == KEY_MODE_Pin) {
//        KeyNum = 3; // 按键3的按下状态
//    }
//}

uint8_t Key_GetNum(void) {
    uint8_t tempKeyNum = KeyNum; // 保存当前 KeyNum 的值
    KeyNum = 0; // 清零 KeyNum，准备接收下一次中断事件
    return tempKeyNum;
}
