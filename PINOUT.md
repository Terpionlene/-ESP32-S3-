# ESP32-S3 引脚配置 (PINOUT)

## 项目信息

| 项目 | 信息 |
|------|------|
| 团队名称 | 仅剩三小时 |
| 团队编号 | 28095 |
| 目标芯片 | ESP32-S3 |
| Flash 大小 | 16MB |
| PSRAM | 已启用 |
| CMake 版本 | 3.20 |
| ESP-IDF 版本 | v6.0.1 |

---

## 硬件引脚连接

### ST7789 LCD 屏幕 (8引脚)

| LCD引脚号 | 标识 | 功能 | ESP32-S3 GPIO | 开发板位置 |
|-----------|------|------|---------------|------------|
| 1 | GND | 地 | GND | 左侧最后一个引脚 |
| 2 | 3.3V | 电源 | 3V3 | 左侧第一个引脚 |
| 3 | SCL | SPI时钟 | GPIO12 | 左侧第22个引脚 |
| 4 | SDA | SPI数据 | GPIO11 | 左侧第18个引脚 |
| 5 | RES | 复位 | GPIO8 | 左侧第13个引脚 |
| 6 | DC | 数据/命令 | GPIO9 | 左侧第16个引脚 |
| 7 | CS | 片选 | GPIO10 | 左侧第17个引脚 |
| 8 | BLK | 背光 | GPIO7 | 左侧第8个引脚 |

**屏幕参数：**
- 分辨率：240 x 280
- 接口：SPI
- 颜色深度：RGB565 (16位)
- SPI 时钟频率：40MHz
- 颜色反转：已启用
- Y 轴偏移：20 像素

**注意**：LCD 的 SCL/SDA 是 SPI 接口的，不是 I2C！不要跟音频模块的 I2C SCL/SDA 混淆。

---

### LED 指示灯

| ESP32-S3 引脚 | 功能 | 说明 |
|---------------|------|------|
| GPIO38 | LED 输出 | 开发板绿色 LED，0.5秒间隔闪烁 |

---

### ES8311 音频模块 (10引脚, 从左到右)

| 音频模块引脚号 | 标识 | 功能 | ESP32-S3 GPIO | 开发板位置 |
|---------------|------|------|---------------|------------|
| 1 | GND | 地 | GND | 任意G引脚 |
| 2 | VCC | 电源 | 3V3 | 左侧第一个引脚 |
| 3 | 3V3 | 电源 | 3V3 | 左侧第一个引脚 |
| 4 | DO | 数据输出 (播放) | GPIO15 | 左侧第9个引脚 |
| 5 | WS | 左右时钟 | GPIO17 | 左侧第12个引脚 |
| 6 | DI | 数据输入 (录音) | GPIO18 | 左侧第13个引脚 |
| 7 | BCK | 位时钟 | GPIO16 | 左侧第11个引脚 |
| 8 | MCK | 主时钟 | GPIO13 | 左侧第21个引脚 |
| 9 | SCL | I2C时钟 | GPIO6 | 左侧第7个引脚 |
| 10 | SDA | I2C数据 | GPIO5 | 左侧第6个引脚 |

---

### EC11 旋转编码器 (6引脚, 从左到右)

| 编码器引脚号 | 标识 | 功能 | ESP32-S3 GPIO | 开发板位置 |
|-------------|------|------|---------------|------------|
| 1 | B | 相位B | GPIO2 | 右侧第6个引脚 |
| 2 | C | 公共端 | 3V3 | 左侧第一个引脚 |
| 3 | A | 相位A | GPIO1 | 右侧第4个引脚 |
| 4 | SW | 按键 | GPIO4 | 左侧第4个引脚 |
| 5 | GND | 地 | GND | 任意G引脚 |
| 6 | VCC | 电源 | 3V3 | 左侧第一个引脚 |

**注意**：编码器引脚顺序是 B-C-A-SW-GND-VCC，不要接错！

---

## SPI 总线配置

| 配置项 | 值 |
|--------|-----|
| SPI 总线 | SPI2_HOST |
| 时钟频率 | 40 MHz |
| SPI 模式 | Mode 0 |
| 传输队列深度 | 10 |
| DMA 通道 | 自动分配 |

---

## 电源和 USB

| 功能 | 说明 |
|------|------|
| USB 接口 | Type-C / Micro USB |
| 供电方式 | USB 5V |
| 烧录协议 | UART (通过 USB 转串口) |
| 波特率 | 115200 |

---

## 开发板预留功能

| 模块 | 功能 | 状态 |
|------|------|------|
| ✅ ST7789 LCD | 屏幕显示驱动 | 已集成 |
| ✅ LED 测试 | GPIO 基础测试 | 已集成 |
| ⏳ ES8311 | 音频模块驱动 | 引脚已预留 |
| ⏳ EC11 | 旋转编码器驱动 | 引脚已预留 |
| ⏳ LVGL | UI 界面框架 | 待开发 |
| ⏳ WiFi | 无线网络 | 待开发 |
| ⏳ BLE | 低功耗蓝牙 | 待开发 |

---

## 编译和烧录

### 环境变量

```powershell
$env:IDF_TOOLS_PATH = "D:\Espressif\tools"
$env:IDF_PATH = "D:\esp\v6.0.1\esp-idf"
$env:IDF_PYTHON_ENV_PATH = "D:\Espressif\tools\python\v6.0.1\venv"
$env:ESP_IDF_VERSION = "6.0.1"
```

### 编译项目

```powershell
D:\Espressif\tools\python\v6.0.1\venv\Scripts\python.exe D:\esp\v6.0.1\esp-idf\tools\idf.py build
```

### 烧录固件

```powershell
# 将 COM3 替换为实际串口端口号
D:\Espressif\tools\python\v6.0.1\venv\Scripts\python.exe D:\esp\v6.0.1\esp-idf\tools\idf.py -p COM3 flash
```

### 串口监控

```powershell
D:\Espressif\tools\python\v6.0.1\venv\Scripts\python.exe D:\esp\v6.0.1\esp-idf\tools\idf.py -p COM3 monitor
```

---

## 输出日志示例

程序运行后，串口输出：

```
I (xxx) esp-aiot-knob: ==========================================
I (xxx) esp-aiot-knob: ESP32-S3 Test Program
I (xxx) esp-aiot-knob: Team: 仅剩三小时
I (xxx) esp-aiot-knob: Team ID: 28095
I (xxx) esp-aiot-knob: ==========================================
I (xxx) esp-aiot-knob: 正在初始化 ST7789 屏幕...
I (xxx) esp-aiot-knob: 屏幕初始化成功！开始红绿蓝颜色测试...
I (xxx) esp-aiot-knob: LED ON  [0]
I (xxx) esp-aiot-knob: LED OFF [1]
I (xxx) esp-aiot-knob: LED ON  [2]
...
```

---

## 注意事项

1. **烧录模式**：按住 BOOT 键再按 EN/RST 键进入下载模式
2. **COM 端口**：在 Windows 设备管理器中确认实际端口号
3. **背光控制**：GPIO7 需配置为输出模式并拉高电平才能点亮屏幕
4. **颜色反转**：ST7789 屏幕通常需要开启颜色反转才能显示正常颜色
5. **Y 轴偏移**：1.69 寸 240x280 屏幕需要 Y 轴偏移 20 像素
6. **引脚冲突**：GPIO1/2 是启动模式引脚，使用编码器时需注意启动模式配置

---

## 文件结构

```
esp32/
├── CMakeLists.txt              # 顶层构建文件
├── sdkconfig.defaults          # 默认配置
├── PINOUT.md                   # 本文档 - 引脚配置
├── .vscode/                    # VS Code 配置
├── main/
│   ├── CMakeLists.txt          # 主模块构建文件
│   └── app_main.c              # 程序入口（包含引脚定义）
└── build/                      # 编译输出
```

---

## 修改引脚配置

如需修改引脚配置，编辑 [main/app_main.c](main/app_main.c) 文件开头的宏定义：

```c
// LCD SPI
#define PIN_LCD_SCK   12
#define PIN_LCD_MOSI  11
#define PIN_LCD_CS    10
#define PIN_LCD_DC    9
#define PIN_LCD_RST   8
#define PIN_LCD_BL    7

// ES8311 Audio (预留)
#define PIN_I2S_BCLK  16
#define PIN_I2S_LRCK  17
#define PIN_I2S_DIN   18
#define PIN_I2S_DOUT  15
#define PIN_I2C_SDA   5
#define PIN_I2C_SCL   6

// Encoder (预留)
#define PIN_ENC_A     1
#define PIN_ENC_B     2
#define PIN_ENC_BTN   4
```

修改后重新编译和烧录即可。

---

*最后更新：2026-06-29*