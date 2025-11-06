#include "IronKey.h"

volatile uint8_t KeyNum = 0;

uint8_t Key_GetNum(void) {
    uint8_t tempKeyNum = KeyNum; // 保存当前 KeyNum 的值
    KeyNum = 0; // 清零 KeyNum，准备接收下一次中断事件
    return tempKeyNum;
}
