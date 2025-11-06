#ifndef ST1XSTATIC_H_
#define ST1XSTATIC_H_

#include "main.h"
#include "u8g2.h"

// 函数声明
void St1xStatic_Init(void);
void St1xStatic_Display(u8g2_t* u8g2);
void St1xStatic_Action(void);
void St1xStatic_DisplayData(u8g2_t* u8g2);

#endif /* ST1XSTATIC_H_ */