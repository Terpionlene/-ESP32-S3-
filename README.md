# ESP32-S3 AI Smart Knob

基于 ESP32-S3-WROOM2-N32R16V 芯片的 AI 智能旋钮交互设备，实现语音对话、PPT 遥控、智能家居控制等功能。

## 项目概述

本项目开发了一款面向桌面场景的 AIoT 智能交互中枢，通过旋钮（EC11）实现"语音 + 旋钮"双模态交互。

**主要功能**：
- 🎙️ AI 语音对话（DeepSeek + 百度 ASR/TTS）
- 📊 PPT 智能遥控（HTTP 远程控制）
- 🏠 智能家居控制（空调等）
- ⚙️ 系统设置（WiFi、音量、亮度）

## 硬件规格

| 项目 | 规格 |
|------|------|
| **主控芯片** | ESP32-S3-WROOM2-N32R16V（32MB Flash + 16MB PSRAM） |
| **显示屏** | ST7789 240×280 SPI LCD |
| **音频编解码器** | ES8311（I2S） |
| **功放** | NS4150B |
| **旋转编码器** | EC11（旋转 + 按键） |
| **LED** | GPIO 38 状态指示 |

## 引脚分配

| 功能 | GPIO | 说明 |
|------|------|------|
| **LCD SPI** | SCK:12, MOSI:11, CS:10, DC:9, RES:8, BLK:7 | 240×280 ST7789 |
| **ES8311 I2C** | SCL:6, SDA:5 | 音频编解码器控制 |
| **ES8311 I2S** | MCK:13, BCK:16, WS:17, DO:15, DI:18 | 音频数据传输 |
| **NS4150B EN** | GPIO 14 | 功放使能 |
| **EC11 Encoder** | A:1, B:2, BTN:4 | 旋钮输入 |
| **LED** | GPIO 38 | 状态指示灯 |

详细定义见 [main/board_pin.h](main/board_pin.h)

## 软件架构

```
┌─────────────────────────────────────────────────┐
│                  应用层                          │
│  AI对话 / PPT遥控 / 智能家居控制 / 系统设置      │
├──────────┬──────────┬──────────┬───────────────┤
│  UI层    │  输入层  │  显示层  │  控制层       │
│ lvgl_ui  │lvgl_input│lvgl_disp │ppt_controller │
├──────────┴──────────┴──────────┴───────────────┤
│           驱动层 (audio_es8311 / board_pin)     │
├─────────────────────────────────────────────────┤
│    ESP-IDF / FreeRTOS / LVGL v9 / WiFi / HTTP  │
└─────────────────────────────────────────────────┘
```

## 快速开始

### 环境准备

- **ESP-IDF 版本**：v6.0.1
- **目标芯片**：ESP32-S3-WROOM2-N32R16V
- **开发系统**：Windows / Linux / macOS

### 编译与烧录

```bash
# 设置目标芯片
idf.py set-target esp32s3

# 配置（设置 API Key 等）
idf.py menuconfig

# 编译
idf.py build

# 烧录 + 监控（替换 COMx 为实际串口号）
idf.py -p COM3 flash monitor
```

### API 配置

在 `idf.py menuconfig` → "API Configuration" 中设置：

| 配置项 | 说明 |
|--------|------|
| DeepSeek API Key | AI 对话服务密钥 |
| Baidu API Key | 百度 ASR/TTS 密钥 |
| Baidu Secret Key | 百度 ASR/TTS 密钥 |

## 功能说明

### 1. AI 语音对话

- 进入 CHAT 界面，按下旋钮开始录音（5秒）
- 录音后自动发送到百度 ASR 识别为文字
- 文字发送到 DeepSeek AI 生成回复
- 回复显示在聊天界面

### 2. PPT 智能遥控

**使用流程**：

1. 在电脑上运行 PPT 控制服务：
   ```bash
   python ppt_server.py
   ```

2. 在设备上进入 **SMART → PPT CONTROL**

3. 旋钮操作：
   - **旋转**：上一页 / 下一页
   - **单击**：音量 +
   - **双击**：音量 -
   - **长按1秒**：退出 PPT 模式

### 3. WiFi 连接

- 自动连接配置的 WiFi（见 `sdkconfig.defaults`）
- 在 SETTINGS → WiFi 中查看连接状态和 IP 地址

### 4. PPT 服务器配置

- 在 SETTINGS → PPT SERVER 中配置服务器 IP 和端口
- 默认 IP：192.168.137.1（电脑热点网关）
- 默认端口：8765

## 目录结构

```
ESP32-S3-AI-Smart-Knob/
├── main/                          # 核心项目代码
│   ├── app_main.c                 # 主程序入口
│   ├── lvgl_ui.c/h                # LVGL 界面（8个屏幕）
│   ├── lvgl_input.c/h             # EC11 旋钮输入处理
│   ├── lvgl_display.c/h           # ST7789 LCD 驱动
│   ├── ppt_controller.c/h         # PPT HTTP 控制器
│   ├── voice_chat.c/h             # 语音对话流程
│   ├── audio_es8311.c/h           # ES8311 音频驱动
│   ├── board_pin.h                # 引脚定义
│   ├── icons.c/h                  # 图标资源
│   ├── test_audio.h               # 测试音频数据
│   ├── Kconfig.projbuild          # API 配置项
│   └── CMakeLists.txt
├── components/
│   └── lv_conf.h                  # LVGL 配置
├── ppt_server.py                  # 电脑端 PPT 控制服务
├── aip-cpp-sdk-4.16.7/            # 百度 AI C++ SDK
├── audio_repair/                  # 音频修复模块
├── PPT_solution/                  # PPT 解决方案参考代码
├── CMakeLists.txt                 # 顶层构建文件
├── sdkconfig.defaults             # 默认配置
├── idf_component.yml              # 组件依赖
└── README.md                      # 本文档
```

## FreeRTOS 任务

| 任务名 | 核心 | 优先级 | 功能 |
|--------|------|--------|------|
| lvgl_task | CPU1 | 4 | LVGL 界面渲染 |
| voice_chat_task | CPU1 | 5 | 语音对话处理 |
| record_task | CPU1 | 5 | I2S 音频录音 |
| ppt_task | CPU0 | 3 | PPT HTTP 请求 |
| led_task | CPU0 | 2 | LED 状态指示 |

## UI 屏幕列表

| 屏幕 | 功能 |
|------|------|
| MAIN | 主菜单（AI Chat / Smart Ctrl / Settings） |
| SETTINGS | 设置（WiFi / Volume / Brightness / PPT Server） |
| SMART | 智能控制（PPT Control / Air Conditioner） |
| CHAT | AI 聊天界面 |
| SOUND | 音量调节 |
| BRIGHTNESS | 亮度调节 |
| WIFI | WiFi 状态显示 |
| PPT_SETTINGS | PPT 服务器配置 |
| PPT_CONTROL | PPT 遥控界面 |

## 使用说明

### 旋钮操作

| 操作 | 功能 |
|------|------|
| 顺时针旋转 | 焦点右移 / 下一项 |
| 逆时针旋转 | 焦点左移 / 上一项 |
| 单击 | 确认 / 选中 |

### PPT 模式操作

| 操作 | 功能 |
|------|------|
| 顺时针旋转 | PPT 下一页 |
| 逆时针旋转 | PPT 上一页 |
| 单击 | 音量 + |
| 双击 | 音量 - |
| 长按1秒 | 退出 PPT 模式 |

## 技术栈

| 技术 | 版本 |
|------|------|
| ESP-IDF | v6.0.1 |
| LVGL | v9.x |
| DeepSeek API | deepseek-v4-flash |
| 百度 AI | ASR + TTS |
| Python | 3.x（电脑端 PPT 服务） |

## 团队信息

- **团队名称**：仅剩三小时
- **团队编号**：28095
- **赛题**：乐鑫科技赛题 — 智能交互驱动的 AIoT 应用系统

## 许可证

MIT License
