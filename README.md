# St1x
 - S: STM32F103CBT6
 - T: T系列烙铁头
 - 1x: 12 & 13
### 基于SMT32F103CBT6自动调温烙铁, 使用T12 & T13烙铁头实现
***
- 打开St1x.ico使用STM32CulbMX6.5版本打开文件之后, 选择自己的IDE, 点击GENERATE CODE即可。

- 下载u8g2的支持库, 将csrc整个文件夹及内容添加进自己的项目中, u8g2库下载地址: https://github.com/olikraus/u8g2

- 精减“ u8g2_d_setup.c ”文件, 目前只使用了“ u8g2_Setup_sh1107_i2c_128x80_f 以及 u8g2_Setup_sh1107_i2c_tk078f288_80x128_f ”这两个I2C其中一个作驱动, 具体有啥差别我也不知道, 其它不需要的可以直接删除。

- 精减“ u8g2_d_setup.c ”文件, 只保留" uint8_t *u8g2_m_10_16_f(uint8_t *page_cnt) "这条, 其它的可以注释掉或者直接删除。

- 将csrc多余的驱动文件删除, " u8x8_d_xxxx.c "这类型命名的均为不同型号的OLED的驱动文件, 只需要只保留“u8x8_d_sh1107.c"这个驱动, 其它的驱动可以删除。

- 更详细的精减u8g2库可以自行搜索, 我就不再详细描述了。

- 将St1x.ioc版本由STM32CulbMX6.5更新到STM32CulbMX6.7, 同时把所有关于u8g2相关的库加入进去, 直接编译下载即可使用不需要再重新整理。
***
# 功能实现结果
## 硬件设计
  - [x] 原理图设计
  - [X] PCB设计
  - [x] 硬件电路调试
    - [X] PD快充协议触发
    - [x] 系统供电
    - [X] 按键
    - [X] 烙铁头温度控制
    - [X] 屏幕升压供电
    - [X] 工作指示灯
    - [x] 蜂鸣器提示
    - [x] 烙铁头电动势运放电路
    - [x] 加速度传感器
    - [x] 屏幕显示
***
## 外观设计
- [x] ID设计
- [x] 外壳3D设计
***
## 软件设计
- [x] O-LED_U8G2 驱动(SPI)
- [x] O-LED 厂家驱动(SPI)
- [ ] 界面显示
- [x] 按键控制
- [x] PID算法
- [x] ADC温度读取
- [x] USB输入电压检测
- [x] 自动温度控制
- [ ] RGB_LED指示
- [ ] WS2812B_RGB指示
- [ ] 蜂鸣器事件
***
# 图片展示
![T12.png](Electronics%2FImage%2FT12.png)

![SCH.png](Electronics%2FImage%2FSCH.png)

![St1x_TOP.png](Electronics%2FImage%2FSt1x_TOP.png)

![St1x_BOT.png](Electronics%2FImage%2FSt1x_BOT.png)

![St1x_MB.png](Electronics%2FImage%2FSt1x_MB.png)

![1.jpg](Electronics%2FImage%2F1.jpg)

![2.jpg](Electronics%2FImage%2F2.jpg)

[SCH.pdf](Electronics%2FSCH.pdf)
