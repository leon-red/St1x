//
// Created by leonm on 2023/4/26.
//
#include "ws2812.h"
#include "main.h"
#include "St1xADC.h"
#include <stdlib.h>
#include <math.h>

struct RGB_24bits RGB;

/*д������ʱ��*/
void Send_A_bit(unsigned char VAL) {
    if (VAL != 1) {
        LED_RED_GPIO_Port->BSRR = LED_RGB_Pin;
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        LED_RED_GPIO_Port->BSRR = (uint32_t) LED_RGB_Pin << 16u;

        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    } else {
        LED_RED_GPIO_Port->BSRR = LED_RGB_Pin;
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        LED_RED_GPIO_Port->BSRR = (uint32_t) LED_RGB_Pin << 16u;

        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    }
}

void Reset_LED() {
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RGB_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RGB_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);

}


/*����24λ�ַ�������RGB��Ϣ��8λ��*/
void Send_24bits(struct RGB_24bits RGB_VAL) {
    unsigned char i;
    for (i = 0; i < 8; i++) {
        Send_A_bit(RGB_VAL.G_VAL >> (7 - i) & 0x01);//ע���ǴӸ�λ�ȷ�
    }
    for (i = 8; i < 16; i++) {
        Send_A_bit(RGB_VAL.R_VAL >> (15 - i) & 0x01);
    }
    for (i = 16; i < 24; i++) {
        Send_A_bit(RGB_VAL.B_VAL >> (23 - i) & 0x01);
    }

}

void Test_ws2812(void) {
    RGB.G_VAL = 20;
    RGB.R_VAL = 20;
    RGB.B_VAL = 20;

    Reset_LED();

    Send_24bits(RGB);
}

int i = 0;
char flag = 0;

void WS2812_Test2(void) {
    RGB.G_VAL = 0;
    RGB.R_VAL = 0;
    RGB.B_VAL = 0;
    i++;
    if (flag == 0) {
        RGB.B_VAL = 255 - i;
        RGB.R_VAL = i;
        if (i == 255) {
            i = 0;
            flag = 1;
        }
    } else if (flag == 1) {
        RGB.R_VAL = 255 - i;
        RGB.G_VAL = i;
        if (i == 255) {
            i = 0;
            flag = 2;
        }
    } else if (flag == 2) {
        RGB.G_VAL = 255 - i;
        RGB.B_VAL = i;
        if (i == 255) {
            i = 0;
            flag = 0;
        }
    }
    Reset_LED();
    Send_24bits(RGB);
}

void Show_All_Colors() {
    struct RGB_24bits color;
    for (int r = 0; r < 255; r++) {
        for (int g = 0; g < 255; g++) {
            for (int b = 0; b < 255; b++) {
                color.R_VAL = r;
                color.G_VAL = g;
                color.B_VAL = b;
                Send_24bits(color);
            }
        }
    }
}

/**
 * @brief RGB三色渐变过渡灯效
 * 红绿蓝三种颜色平滑渐变过渡，用于开机后没有加热和工作状态时的显示
 */
void RGBChaseEffect(void) {
    static uint32_t last_chase_time = 0;
    static uint8_t current_phase = 0;  // 0: 红->绿, 1: 绿->蓝, 2: 蓝->红
    static uint8_t transition_value = 0;
    static uint8_t initialized = 0;
    
    uint32_t current_time = HAL_GetTick();
    
    // 每5ms更新一次渐变效果
    if (current_time - last_chase_time < 5) {
        return;
    }
    last_chase_time = current_time;
    
    // 首次调用时初始化LED
    if (!initialized) {
        Reset_LED();
        initialized = 1;
    }
    
    // 更新渐变值
    transition_value += 2;
    if (transition_value >= 255) {
        transition_value = 0;
        current_phase = (current_phase + 1) % 3;
    }
    
    // 根据当前渐变阶段设置RGB值
    switch (current_phase) {
        case 0:  // 红->绿渐变
            RGB.R_VAL = 255 - transition_value;
            RGB.G_VAL = transition_value;
            RGB.B_VAL = 0;
            break;
        case 1:  // 绿->蓝渐变
            RGB.R_VAL = 0;
            RGB.G_VAL = 255 - transition_value;
            RGB.B_VAL = transition_value;
            break;
        case 2:  // 蓝->红渐变
            RGB.R_VAL = transition_value;
            RGB.G_VAL = 0;
            RGB.B_VAL = 255 - transition_value;
            break;
    }
    
    // 更新LED显示
    Send_24bits(RGB);
}

/**
 * @brief 统一LED状态机效果
 * 智能判断系统状态，选择显示合适的LED效果：
 * - 加热/工作状态：显示呼吸灯效果
 * - 空闲状态：显示RGB三色渐变过渡效果
 */
void HeatingStatusLEDEffect(void) {
    static uint32_t last_update_time = 0;
    static uint8_t initialized = 0;
    
    uint32_t current_time = HAL_GetTick();
    
    // 每5ms更新一次LED效果
    if (current_time - last_update_time < 5) {
        return;
    }
    last_update_time = current_time;
    
    // 首次调用时初始化LED
    if (!initialized) {
        Reset_LED();
        initialized = 1;
    }
    
    // 获取系统状态变量
    extern uint8_t heating_control_enabled;
    extern uint8_t heating_status;
    extern uint8_t focused_heating_mode;
    
    // 判断当前系统状态，添加温度接近目标的特殊处理
    extern float target_temperature;
    float current_temp = getDisplayFilteredTemperature();
    float temp_diff = target_temperature - current_temp;  // 温度差
    
    // 当温度接近目标温度时（差值<15°C），优先显示加热状态的红绿过渡效果
    // 避免在接近目标温度时频繁切换状态导致闪烁
    if ((!heating_control_enabled || !heating_status) && temp_diff > 15.0f) {
        // 空闲状态且温度差较大：显示RGB三色平滑渐变过渡效果（闭环过渡）
        static uint16_t cycle_position = 0;  // 0-767的循环位置
        
        // 更新循环位置（每5ms增加4，约384ms完成一个完整循环）
        cycle_position += 1;
        if (cycle_position >= 768) {
            cycle_position = 0;
        }
        
        // 将768个位置分为3个256的阶段，每个阶段对应一种颜色过渡
        uint8_t phase = cycle_position / 256;  // 0:红->绿, 1:绿->蓝, 2:蓝->红
        uint8_t position_in_phase = cycle_position % 256;  // 当前阶段内的位置
        
        // 根据相位和位置计算RGB值，实现平滑的闭环过渡
        switch (phase) {
            case 0:  // 红->绿过渡：红色渐暗，绿色渐亮
                RGB.R_VAL = 255 - position_in_phase;
                RGB.G_VAL = position_in_phase;
                RGB.B_VAL = 0;
                break;
            case 1:  // 绿->蓝过渡：绿色渐暗，蓝色渐亮
                RGB.R_VAL = 0;
                RGB.G_VAL = 255 - position_in_phase;
                RGB.B_VAL = position_in_phase;
                break;
            case 2:  // 蓝->红过渡：蓝色渐暗，红色渐亮
                RGB.R_VAL = position_in_phase;
                RGB.G_VAL = 0;
                RGB.B_VAL = 255 - position_in_phase;
                break;
        }
    } else {
        // 加热/工作状态：显示红色从暗到亮再到暗的渐变过程
        
        // 限制温度差范围：0-100°C
        if (temp_diff < 0.0f) temp_diff = 0.0f;
        if (temp_diff > 100.0f) temp_diff = 100.0f;
        
        // 创建红色渐变效果：根据温度差控制红色亮度
        // 温度差越大，红色越亮；温度差越小，红色越暗
        uint8_t red_brightness = (uint8_t)(temp_diff * 2.55f);  // 0-255的亮度范围
        
        // 添加呼吸效果：红色从暗到亮再到暗的循环
        static uint32_t breath_cycle = 0;
        static uint8_t breath_direction = 0;  // 0: 渐亮, 1: 渐暗
        static uint8_t breath_value = 0;
        
        breath_cycle++;
        if (breath_cycle >= 10) {  // 每50ms更新一次呼吸效果
            breath_cycle = 0;
            
            // 更新呼吸值
            if (breath_direction == 0) {
                breath_value += 5;
                if (breath_value >= 100) {
                    breath_value = 100;
                    breath_direction = 1;
                }
            } else {
                breath_value -= 5;
                if (breath_value <= 0) {
                    breath_value = 0;
                    breath_direction = 0;
                }
            }
        }
        
        // 计算最终的红色亮度：基础亮度 + 呼吸效果
        uint8_t final_red = red_brightness;
        if (red_brightness > 0) {
            // 只有当基础亮度不为0时才添加呼吸效果
            uint8_t breath_amplitude = (uint8_t)(red_brightness * 0.3f);  // 呼吸幅度为基础亮度的30%
            uint8_t breath_offset = (uint8_t)(breath_value * breath_amplitude / 100.0f);
            
            if (breath_direction == 0) {
                // 渐亮阶段：从暗到亮
                final_red = red_brightness - breath_amplitude + breath_offset;
            } else {
                // 渐暗阶段：从亮到暗
                final_red = red_brightness - breath_offset;
            }
        }
        
        // 设置RGB值：纯红色渐变效果
        RGB.R_VAL = final_red;
        RGB.G_VAL = 0;
        RGB.B_VAL = 0;
        
        // 在温度差较小（<20°C）时，添加轻微的黄色调，表示接近目标
        if (temp_diff < 20.0f) {
            RGB.G_VAL = (uint8_t)((20.0f - temp_diff) * 5.0f);  // 0-100的绿色调
        }
    }
    
    // 更新LED显示
    Send_24bits(RGB);
}