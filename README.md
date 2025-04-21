# AD9834 信号源

作者: TerayTech Bilibili: https://space.bilibili.com/24434095

[English Version](README_EN.md) | [日本語版](README_JP.md)

![Web界面](img/web.png)
![设备实物图](img/IMG_5558(20250421-213748).JPG)

基于ESP32C3和AD9834的可编程信号发生器，支持串口控制,掉电保存和OLED显示。

## 硬件要求

- ESP32C3开发板
- AD9834 DDS信号发生器模块（75MHz有源晶振）
- 0.91寸OLED显示屏（128x32分辨率，I2C接口）
- 连接线

## 引脚连接

### AD9834连接

| AD9834引脚 | ESP32C3引脚 | 功能描述 |
|------------|------------|---------|
| SYNC       | GPIO 0     | SPI片选信号 |
| RESET      | GPIO 1     | 复位信号 |
| SCLK       | GPIO 2     | SPI时钟信号 |
| DATA       | GPIO 3     | SPI数据信号(MOSI) |
| FSE        | GPIO 4     | 频率寄存器选择 |
| PSE        | GPIO 21    | 相位寄存器选择 |

### OLED显示屏连接

| OLED引脚 | ESP32C3引脚 | 功能描述 |
|----------|------------|---------|
| SDA      | GPIO 6     | I2C数据线 |
| SCL      | GPIO 7     | I2C时钟线 |
| VCC      | 3.3V       | 电源正极 |
| GND      | GND        | 电源负极 |

## 功能特点

- 支持正弦波和三角波输出
- 双频率寄存器（F0/F1）和双相位寄存器（P0/P1）
- 通过串口命令控制所有参数
- OLED实时显示当前频率、相位和波形类型
- 使用ESP32C3的USB-CDC功能，无需外部USB转串口芯片

## 串口命令

| 命令     | 描述                           | 示例      |
|---------|--------------------------------|-----------|
| F0xxxxx | 设置频率寄存器0的值（单位：Hz）    | F01000    |
| F1xxxxx | 设置频率寄存器1的值（单位：Hz）    | F110000   |
| P0xxxx  | 设置相位寄存器0的值（范围：0-4095）| P00       |
| P1xxxx  | 设置相位寄存器1的值（范围：0-4095）| P11024    |
| SF0     | 选择频率寄存器0                  | SF0       |
| SF1     | 选择频率寄存器1                  | SF1       |
| SP0     | 选择相位寄存器0                  | SP0       |
| SP1     | 选择相位寄存器1                  | SP1       |
| WS      | 设置波形为正弦波                  | WS        |
| WT      | 设置波形为三角波                  | WT        |
| ?/H     | 显示帮助信息                     | ?         |

## 软件依赖

- Arduino框架
- Adafruit SSD1306库（用于OLED显示）
- Adafruit GFX库（用于OLED显示）
- AD983X Arduino库（用于控制AD9834）

## 编译与上传

本项目使用PlatformIO进行管理，配置如下：

```ini
[env:airm2m_core_esp32c3]
platform = espressif32
board = airm2m_core_esp32c3
framework = arduino
monitor_speed = 115200
build_flags = 
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
lib_deps =
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.5
    adafruit/Adafruit BusIO@^1.14.1
```

## 使用方法

1. 按照上述引脚连接图连接硬件
2. 编译并上传程序到ESP32C3
3. 打开串口监视器（波特率115200）
4. 使用串口命令控制信号发生器
5. 在OLED屏幕上查看当前设置

## 注意事项

- AD9834的时钟基准为75MHz，这决定了输出频率的精度和范围
- 最大输出频率约为37.5MHz（时钟频率的一半）
- 相位值范围为0-4095，对应0-360度
- 使用USB-CDC功能时，某些操作系统可能需要安装驱动程序

## 扩展功能

- 可以通过修改代码添加更多波形类型（如方波）
- 可以添加频率扫描功能
- 可以添加按键控制界面，减少对串口的依赖

## 许可证

本项目采用MIT许可证开源。

## 贡献

欢迎提交问题报告和功能请求。如果您想贡献代码，请提交拉取请求。
