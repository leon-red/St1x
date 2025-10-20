//
// Created by Leon on 2023/2/18.
//

#ifndef ST1X_ST1X_LED_H
#define ST1X_ST1X_LED_H

void LED_Init_stop();
void LED_Init();
void LED_RED();
void LED_GREEN();
void LED_BLUE();

void KEY_Task(void);
void LED_Task(void);

void pwm_up();
void pwm_down();
#endif //ST1X_ST1X_LED_H
