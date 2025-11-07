# St1x
 - S: STM32F103CBT6
 - T: T系列烙铁头
 - 1x: 12 & 13 & 16
### 基于SMT32F103自动调温烙铁, 使用T12 & T13 & T16 烙铁头实现
***
# 非常感谢以下开源的作者与项目
- 项目参考：[Eddddddddy](https://github.com/Eddddddddy/T12-PD-SolderingPen)
- AI编码助手：[北京引力弹弓科技有限公司](https://www.trae.cn/)
- OLED驱动： [U8G2](https://github.com/olikraus/u8g2) 库, 具体使用方法参考 [U8G2官方WiKi](https://github.com/olikraus/u8g2)
- 下载&调试工具： [PowerWriter PWLINK2](https://item.taobao.com/item.htm?id=675067753017&mi_id=0000kcOF0EfjT0zOQPjCa-2Qonzzei_QfJZvz_5v8zdkguU&spm=tbpc.boughtlist.suborder_itemtitle.1.51e22e8dHyS53r)
- 开发环境:
  - [Clion 2023.3.6](https://www.jetbrains.com/zh-cn/clion/download/other.html)
  - [STM32CulbMX6.15.0](https://www.st.com.cn/content/st_com/zh/stm32cubemx.html)
  - [openocd-v0.12.0-i686-w64-mingw32](https://github.com/openocd-org/openocd)
  - [mingw64-x86_64-13.2.0-release-win32-seh-msvcrt-rt_v11-rev1](https://sourceforge.net/projects/mingw-w64/files/mingw-w64/mingw-w64-release/)
  - [arm-gnu-toolchain-13.2.Rel1-mingw-w64-i686-arm-none-eabi](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

- 使用`STM32CulbMX6.15.0`打开`St1x.ioc`, 点击`GENERATE CODE`生成初始化代码, 再使用`Clion 2023.3.6`编译下载。 有时间再配合图文讲解完善操作~~~~~~
***
# 功能实现结果
## 硬件设计
  - [x] 原理图设计
  - [x] PCB设计
  - [x] 硬件电路调试
    - [x] PD快充协议触发
    - [x] 系统供电
    - [x] 按键
    - [x] 烙铁头温度控制
    - [x] 屏幕升压供电
    - [x] 工作指示灯
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
- [x] 界面显示
- [x] 按键控制
- [x] PID算法
- [x] ADC温度读取
- [x] USB输入电压检测
- [x] 自动温度控制
- [x] 静置检测
- [ ] RGB_LED指示
- [ ] WS2812B_RGB指示
- [ ] 蜂鸣器事件
***
# 图片展示
![T12.png](T12.png)

![St1x_TOP](St1x_TOP.png)

![St1x_BOT](St1x_BOT.png)

![St1x_MB](St1x_MB.png)

![1.jpg](1.jpg)


![2](2.jpg)

# 原理图
![SCH](SCH.png)
[点击打开PDF原理图](HardwarDesign/SCH.pdf)