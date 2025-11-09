//
// Created by leonm on 2023/4/26.
//
#include "ws2812.h"
#include "main.h"
#include "St1xADC.h"
#include <stdlib.h>
#include <math.h>

struct RGB_24bits RGB;

/**
 * @brief HSV到RGB颜色转换函数
 * @param h 色相 (0-360)
 * @param s 饱和度 (0-100)
 * @param v 亮度 (0-100)
 * @return 转换后的RGB颜色结构体
 */
struct RGB_24bits HSVtoRGB(float h, float s, float v) {
    struct RGB_24bits rgb;
    
    // 归一化参数
    s /= 100.0f;
    v /= 100.0f;
    
    // 计算色相区域
    int region = (int)(h / 60.0f);
    float remainder = (h / 60.0f) - region;
    
    // 计算中间值
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * remainder);
    float t = v * (1.0f - s * (1.0f - remainder));
    
    // 根据色相区域设置RGB值
    switch (region) {
        case 0:  // 红色到黄色
            rgb.R_VAL = (uint8_t)(v * 255);
            rgb.G_VAL = (uint8_t)(t * 255);
            rgb.B_VAL = (uint8_t)(p * 255);
            break;
        case 1:  // 黄色到绿色
            rgb.R_VAL = (uint8_t)(q * 255);
            rgb.G_VAL = (uint8_t)(v * 255);
            rgb.B_VAL = (uint8_t)(p * 255);
            break;
        case 2:  // 绿色到青色
            rgb.R_VAL = (uint8_t)(p * 255);
            rgb.G_VAL = (uint8_t)(v * 255);
            rgb.B_VAL = (uint8_t)(t * 255);
            break;
        case 3:  // 青色到蓝色
            rgb.R_VAL = (uint8_t)(p * 255);
            rgb.G_VAL = (uint8_t)(q * 255);
            rgb.B_VAL = (uint8_t)(v * 255);
            break;
        case 4:  // 蓝色到紫色
            rgb.R_VAL = (uint8_t)(t * 255);
            rgb.G_VAL = (uint8_t)(p * 255);
            rgb.B_VAL = (uint8_t)(v * 255);
            break;
        default: // 紫色到红色
            rgb.R_VAL = (uint8_t)(v * 255);
            rgb.G_VAL = (uint8_t)(p * 255);
            rgb.B_VAL = (uint8_t)(q * 255);
            break;
    }
    
    return rgb;
}

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

/**
 * @brief 使用HSV模型的彩虹渐变效果
 * 相比RGB模型，HSV实现彩虹渐变更加简单直观
 */
void HSV_RainbowEffect(void) {
    static uint32_t last_update_time = 0;
    static float hue = 0.0f;  // 色相值 (0-360)
    
    uint32_t current_time = HAL_GetTick();
    
    // 每20ms更新一次颜色
    if (current_time - last_update_time < 20) {
        return;
    }
    last_update_time = current_time;
    
    // 更新色相值（每秒完成一个完整的彩虹循环）
    hue += 0.72f;  // 360° / 500ms = 0.72°/ms
    if (hue >= 360.0f) {
        hue = 0.0f;
    }
    
    // 使用HSV模型生成颜色（固定饱和度和亮度）
    RGB = HSVtoRGB(hue, 100.0f, 100.0f);  // 全饱和度，全亮度
    
    // 更新LED显示
    Reset_LED();
    Send_24bits(RGB);
}

/**
 * @brief 使用HSV模型的呼吸灯效果
 * 通过调整亮度值实现平滑的呼吸效果
 */
void HSV_BreathingEffect(uint8_t hue) {
    static uint32_t last_update_time = 0;
    static uint8_t brightness = 0;
    static uint8_t direction = 0;  // 0: 渐亮, 1: 渐暗
    
    uint32_t current_time = HAL_GetTick();
    
    // 每10ms更新一次亮度
    if (current_time - last_update_time < 10) {
        return;
    }
    last_update_time = current_time;
    
    // 更新亮度值
    if (direction == 0) {
        brightness += 2;
        if (brightness >= 100) {
            brightness = 100;
            direction = 1;
        }
    } else {
        brightness -= 2;
        if (brightness <= 0) {
            brightness = 0;
            direction = 0;
        }
    }
    
    // 使用HSV模型生成颜色（固定色相和饱和度，变化亮度）
    RGB = HSVtoRGB((float)hue, 100.0f, (float)brightness);
    
    // 更新LED显示
    Reset_LED();
    Send_24bits(RGB);
}
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
 * @brief RGB亮度调整函数（高性能）
 * 直接按比例缩放RGB分量，适合实时亮度控制
 * @param color 要调整的RGB颜色
 * @param brightness 亮度比例 (0.0-1.0)
 */
void RGB_AdjustBrightness(struct RGB_24bits* color, float brightness) {
    // 限制亮度范围
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;
    
    // 直接缩放RGB分量（快速计算）
    color->R_VAL = (uint8_t)(color->R_VAL * brightness);
    color->G_VAL = (uint8_t)(color->G_VAL * brightness);
    color->B_VAL = (uint8_t)(color->B_VAL * brightness);
}

/**
 * @brief HSV智能亮度控制函数
 * 使用HSV模型选择基础颜色，然后用RGB调整亮度
 * 兼顾颜色选择的直观性和亮度调整的性能
 * @param hue 色相 (0-360)
 * @param saturation 饱和度 (0-100)
 * @param target_brightness 目标亮度 (0-100)
 */
void HSV_SmartBrightnessControl(float hue, float saturation, float target_brightness) {
    static struct RGB_24bits base_color;
    static float last_hue = -1.0f;
    static float last_saturation = -1.0f;
    
    // 只有当色相或饱和度改变时才重新计算基础颜色
    if (hue != last_hue || saturation != last_saturation) {
        // 使用HSV模型选择基础颜色（全亮度）
        base_color = HSVtoRGB(hue, saturation, 100.0f);
        last_hue = hue;
        last_saturation = saturation;
    }
    
    // 使用RGB模型快速调整亮度（避免实时HSV转换）
    float brightness_factor = target_brightness / 100.0f;
    RGB_AdjustBrightness(&base_color, brightness_factor);
    
    // 更新LED显示
    Reset_LED();
    Send_24bits(base_color);
}

/**
 * @brief 温度状态智能LED控制
 * 根据烙铁温度状态智能选择颜色和亮度
 * @param current_temp 当前温度
 * @param target_temp 目标温度
 * @param heating_status 加热状态
 */
void TemperatureSmartLEDControl(float current_temp, float target_temp, uint8_t heating_status) {
    float temp_diff = target_temp - current_temp;
    
    if (!heating_status) {
        // 空闲状态：蓝色呼吸灯效果
        static uint8_t breath_brightness = 0;
        static uint8_t breath_direction = 0;
        
        // 呼吸效果计算
        if (breath_direction == 0) {
            breath_brightness += 2;
            if (breath_brightness >= 60) {  // 最大亮度60%
                breath_direction = 1;
            }
        } else {
            breath_brightness -= 2;
            if (breath_brightness <= 10) {  // 最小亮度10%
                breath_direction = 0;
            }
        }
        
        // 使用HSV智能控制：蓝色，高饱和度，呼吸亮度
        HSV_SmartBrightnessControl(240.0f, 80.0f, (float)breath_brightness);
        
    } else if (temp_diff > 50.0f) {
        // 温度差较大：红色，高亮度
        HSV_SmartBrightnessControl(0.0f, 100.0f, 80.0f);
        
    } else if (temp_diff > 20.0f) {
        // 温度差中等：橙色，中等亮度
        HSV_SmartBrightnessControl(30.0f, 100.0f, 60.0f);
        
    } else if (temp_diff > 5.0f) {
        // 温度接近：黄色，低亮度
        HSV_SmartBrightnessControl(60.0f, 100.0f, 40.0f);
        
    } else {
        // 达到目标温度：绿色，稳定亮度
        HSV_SmartBrightnessControl(120.0f, 100.0f, 30.0f);
    }
}

/**
 * @brief 快速RGB颜色设置（最高性能）
 * 直接设置RGB值，用于需要最高性能的场景
 * @param r 红色 (0-255)
 * @param g 绿色 (0-255)
 * @param b 蓝色 (0-255)
 */
void FastRGB_SetColor(uint8_t r, uint8_t g, uint8_t b) {
    RGB.R_VAL = r;
    RGB.G_VAL = g;
    RGB.B_VAL = b;
    
    Reset_LED();
    Send_24bits(RGB);
}

/**
 * @brief 系统状态LED指示器
 * 根据系统不同状态显示相应的LED颜色
 * @param system_state 系统状态
 * 0: 关机/待机, 1: 开机准备, 2: 加热中, 3: 达到温度, 4: 错误状态
 */
void SystemStatusLEDIndicator(uint8_t system_state) {
    // 静态变量需要在函数开头声明
    static uint8_t heat_brightness = 30;
    static uint8_t heat_direction = 0;
    static uint32_t last_blink_time = 0;
    static uint8_t blink_state = 0;
    
    switch (system_state) {
        case 0:  // 关机/待机：无光
            FastRGB_SetColor(0, 0, 0);
            break;
            
        case 1:  // 开机准备：蓝色渐变
            HSV_SmartBrightnessControl(240.0f, 100.0f, 50.0f);
            break;
            
        case 2:  // 加热中：稳定红色
            FastRGB_SetColor(80, 0, 0);
            break;
            
        case 3:  // 达到温度：稳定绿色
            FastRGB_SetColor(0, 80, 0);
            break;
            
        case 4:  // 错误状态：闪烁红色
            {
                uint32_t current_time = HAL_GetTick();
                if (current_time - last_blink_time > 500) {  // 500ms闪烁
                    last_blink_time = current_time;
                    blink_state = !blink_state;
                    
                    if (blink_state) {
                        FastRGB_SetColor(100, 0, 0);  // 亮红色
                    } else {
                        FastRGB_SetColor(0, 0, 0);    // 熄灭
                    }
                }
            }
            break;
    }
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
        // 加热/工作状态：根据温度差显示红色到黄色的渐变
        
        // 限制温度差范围：0-100°C
        if (temp_diff < 0.0f) temp_diff = 0.0f;
        if (temp_diff > 100.0f) temp_diff = 100.0f;
        
        // 根据温度差计算红色亮度：温度差越大，红色越亮
        uint8_t red_brightness = (uint8_t)(temp_diff * 2.55f);  // 0-255的亮度范围
        
        // 设置RGB值：纯红色到红黄色的渐变
        RGB.R_VAL = red_brightness;
        RGB.G_VAL = 0;
        RGB.B_VAL = 0;
        
        // 在温度差较小（<30°C）时，添加黄色调，表示接近目标温度
        if (temp_diff < 30.0f) {
            // 温度差越小，黄色调越明显（绿色分量增加）
            RGB.G_VAL = (uint8_t)((30.0f - temp_diff) * 8.5f);  // 0-255的绿色调
        }
    }
    
    // 更新LED显示
    Send_24bits(RGB);
}