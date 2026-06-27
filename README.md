# -ESP32-S3-
设计一款面向桌面场景的AIoT智能交互中枢，实现"语音+旋钮+触控"三维多模态交互
## 一、快速开始

### 1. 环境准备

- **ESP-IDF 版本**：v5.1.2（必须，esp-sr兼容性最好）
- **目标芯片**：ESP32-S3 WROOM-1U（8MB Flash，无PSRAM）
- **开发系统**：Windows / Linux / macOS

Windows 安装：
# 下载 ESP-IDF v5.1.2 安装包
# https://dl.espressif.com/dl/esp-idf/
# 安装后打开 "ESP-IDF 5.1 PowerShell"



# 设置目标芯片
idf.py set-target esp32s3

# 编译
idf.py build

# 烧录 + 监控（替换 COMx 为实际串口号）
idf.py -p COM5 flash monitor

# 仅监控
idf.py -p COM5 monitor
## 二、三人分工与模块边界

### 成员A — 语音+音频负责人（`main/audio/`）

**职责**：esp-sr语音识别、音频输入输出

**文件**：

- `audio/audio_manager.c/h` — I2S麦克风输入 + 扬声器输出驱动 ✅（框架已完成）
- `audio/speech_engine.c/h` — WakeNet + MultiNet封装 ⚠️（Mock模式，需集成esp-sr）

**待完成**：

1. 在 `idf_component.yml` 中添加 `espressif/esp-sr` 组件
2. 在 `speech_engine.c` 中初始化 WakeNet 和 MultiNet
3. 在 `speech_task` 中调用识别API，结果通过 `speech_callback` 传出
4. 添加WAV音频播放功能（可选，提示音用）

**参考**：

- esp-sr 官方示例：`esp-sr/examples/speech_recognition/`

***

### 成员B — UI+交互负责人（`main/ui/` + `main/input/`）

**职责**：LVGL界面、LCD驱动、触摸屏驱动、编码器交互

**文件**：

- `input/encoder.c/h` — EC11旋转编码器驱动 ✅（已完成，可用）
- `input/touch.c/h` — 触摸屏驱动 ⚠️（Mock模式，需实现真实I2C驱动）
- `ui/ui_manager.c/h` — LVGL管理 + UI任务 ⚠️（Mock模式，需集成LVGL）
- `ui/screens/screen_main.c` — 主菜单界面 ⚠️
- `ui/screens/screen_light.c` — 灯光控制界面 ⚠️
- `ui/screens/screen_voice.c` — 语音状态界面 ⚠️

**待完成**：

1. 添加 `espressif/esp_lvgl_port` 和 `lvgl/lvgl` 组件
2. 实现 SPI LCD 驱动（ST7789等，根据实际屏幕芯片调整）
3. 实现 I2C 触摸驱动（CST816D / FT6236 等）
4. 绘制所有界面 + 动画 + 中文字体
5. 连接事件总线，UI响应外部事件

**参考**：

- esp\_lvgl\_port 示例：`esp_lvgl_port/examples/`
- LVGL v9 文档：<https://docs.lvgl.io/>

***

### 成员C — 系统+网络负责人（`main/control/` + `main/net/` + `main/utils/`）

**职责**：系统架构、事件总线、设备控制、网络通信、整体联调

**文件**：

- `utils/event_bus.c/h` — 事件总线（Queue + 发布订阅） ✅（已完成）
- `utils/app_config.c/h` — NVS配置管理 ✅（已完成）
- `control/ctrl_manager.c/h` — 主控状态机 + 命令执行 ✅（框架已完成）
- `net/wifi_mgr.c/h` — WiFi管理 ✅（已完成）
- `net/ble_mgr.c/h` — BLE管理 ⚠️（Stub，需实现GATT配网 + HID）
- `common/event_ids.h` — 事件ID定义 ✅
- `common/cmd_table.h` — 命令词表 ✅
- `common/app_types.h` — 全局状态类型 ✅
- `common/board_pin.h` — 引脚定义 ✅

**待完成**：

1. 实现 BLE GATT 配网服务
2. 实现 BLE HID（PPT翻页功能）
3. 实现 HTTP/MQTT 设备控制客户端
4. 完善主控状态机（更复杂的场景联动）
5. 整体联调 + 稳定性测试 + 内存优化

***

## 三、模块间通信协议

所有模块通过 **事件总线** 通信，不直接调用对方内部函数。

### 事件发送

```c
#include "utils/event_bus.h"

// 简单事件
event_bus_post_simple(EVT_VOICE_CMD, CMD_LIGHT_ON);

// 带详细数据的事件
app_event_t evt = {
    .id = EVT_CTRL_LIGHT_BRIGHTNESS,
    .int_val = 80,
    .float_val = 0.8f,
};
strcpy(evt.str_val, "卧室灯");
event_bus_post(&evt, pdMS_TO_TICKS(100));
```

### 事件接收

```c
// 方式1：订阅回调（推荐）
void my_event_handler(app_event_t *event, void *arg) {
    switch (event->id) {
        case EVT_VOICE_CMD:
            // 处理语音命令
            break;
        default:
            break;
    }
}
event_bus_subscribe(my_event_handler, NULL);
```

### 事件ID表（`common/event_ids.h`）

| 事件组 | ID                       | 说明      | 数据                  |
| --- | ------------------------ | ------- | ------------------- |
| 语音  | `EVT_VOICE_WAKEUP`       | 检测到唤醒词  | —                   |
| 语音  | `EVT_VOICE_CMD`          | 识别到命令词  | int\_val = cmd\_id  |
| 语音  | `EVT_VOICE_ERROR`        | 识别失败    | —                   |
| 输入  | `EVT_ENCODER_ROTATE`     | 旋钮旋转    | int\_val = 方向(1/-1) |
| 输入  | `EVT_ENCODER_CLICK`      | 旋钮单击    | —                   |
| 输入  | `EVT_ENCODER_LONG_PRESS` | 旋钮长按    | —                   |
| 控制  | `EVT_CTRL_LIGHT_ON`      | 开灯指令    | —                   |
| 控制  | `EVT_CTRL_LIGHT_OFF`     | 关灯指令    | —                   |
| 控制  | `EVT_CTRL_PPT_NEXT`      | PPT下一页  | —                   |
| 系统  | `EVT_SYS_WIFI_CONNECTED` | WiFi已连接 | —                   |

> **规则**：新增事件必须先在 `event_ids.h` 定义，三个人都知道后再使用。

***

## 四、硬件接线

### GPIO分配表

| 模块           | 引脚     | ESP32-S3 GPIO |
| ------------ | ------ | ------------- |
| **LCD SPI**  | SCK    | 12            |
| <br />       | MOSI   | 11            |
| <br />       | CS     | 10            |
| <br />       | DC     | 9             |
| <br />       | RST    | 8             |
| <br />       | BL（背光） | 7             |
| **触摸 I2C**   | SDA    | 5             |
| <br />       | SCL    | 6             |
| <br />       | INT    | 4             |
| **麦克风 I2S0** | SCK    | 16            |
| <br />       | WS     | 17            |
| <br />       | D1（麦1） | 18            |
| <br />       | D2（麦2） | 21            |
| <br />       | MCLK   | 15            |
| **功放 I2S1**  | BCLK   | 42            |
| <br />       | LRC    | 41            |
| <br />       | DOUT   | 40            |
| **编码器**      | A相     | 1             |
| <br />       | B相     | 2             |
| <br />       | 按键     | 14            |

> 详细说明见 `common/board_pin.h`

***

## 五、目录结构

```
esp-AIoT/
├── CMakeLists.txt              # 顶层构建文件
├── partitions.csv              # Flash分区表（8MB）
├── sdkconfig.defaults          # 默认配置
├── .gitignore
├── README.md                   # 本文档
└── main/
    ├── CMakeLists.txt          # main组件构建文件
    ├── app_main.c              # 程序入口
    ├── common/                 # ⭐ 公共接口（三人都依赖）
    │   ├── event_ids.h         #   事件ID定义
    │   ├── cmd_table.h         #   命令词表
    │   ├── app_types.h         #   全局类型/状态
    │   └── board_pin.h         #   引脚定义
    ├── audio/                  # 成员A — 语音+音频
    │   ├── audio_manager.c/h
    │   └── speech_engine.c/h
    ├── ui/                     # 成员B — UI
    │   ├── ui_manager.c/h
    │   └── screens/
    │       ├── screen_main.c/h
    │       ├── screen_light.c/h
    │       └── screen_voice.c/h
    ├── input/                  # 成员B — 输入
    │   ├── encoder.c/h
    │   └── touch.c/h
    ├── control/                # 成员C — 控制
    │   └── ctrl_manager.c/h
    ├── net/                    # 成员C — 网络
    │   ├── wifi_mgr.c/h
    │   └── ble_mgr.c/h
    └── utils/                  # 成员C — 工具
        ├── event_bus.c/h
        └── app_config.c/h
```

***

## 六、开发规范

### 1. Git 分支策略

```
main          # 稳定版本，每天晚上合并
  ├── feat/audio    # 成员A的分支
  ├── feat/ui       # 成员B的分支
  └── feat/system   # 成员C的分支
```

**每日工作流**：

```bash
# 早上：拉取最新 main
git checkout feat/audio
git pull origin main

# 白天：开发 + 提交
git add .
git commit -m "feat(audio): 集成WakeNet唤醒检测"
git push origin feat/audio

# 晚上例会：成员C合并各分支到 main
```

### 2. 代码规范

- 文件名：小写 + 下划线（`audio_manager.c`）
- 函数名：模块前缀 + 动词（`audio_manager_init()`）
- 宏定义：全大写（`PIN_LCD_SCK`）
- 注释：复杂逻辑加注释，简单逻辑不加
- 日志：使用 `ESP_LOGI/W/E`，TAG 为模块名

### 3. 接口修改流程

修改公共头文件（`common/` 下的文件）必须三个人都同意：

1. 提出修改建议
2. 三人确认
3. 修改并推送到 main
4. 各自拉取合并

***

## 七、命令词表

当前支持10个命令词（`common/cmd_table.h`）：

| 命令词  | 对应功能   |
| ---- | ------ |
| 打开灯光 | 开灯     |
| 关闭灯光 | 关灯     |
| 调亮一点 | 亮度+10% |
| 调暗一点 | 亮度-10% |
| 打开空调 | 开空调    |
| 关闭空调 | 关空调    |
| 下一页  | PPT下一页 |
| 上一页  | PPT上一页 |
| 音量加  | 音量+10  |
| 音量减  | 音量-10  |

> 如需添加，在 `cmd_table.h` 的 `g_cmd_table` 数组中追加。

***

## 八、7天里程碑速查

| 天      | 成员A（语音）      | 成员B（UI）     | 成员C（系统）      |
| ------ | ------------ | ----------- | ------------ |
| **D1** | 麦克风+音频驱动     | 屏幕点亮+编码器    | 工程框架+事件总线    |
| **D2** | WakeNet唤醒    | LVGL+中文+主界面 | WiFi+HTTP客户端 |
| **D3** | MultiNet命令识别 | 灯光+语音界面     | 设备控制逻辑       |
| **D4** | 语音接入事件总线     | 编码器/触摸接入UI  | 三模块联调        |
| **D5** | 识别率优化        | 动画+主题美化     | BLE+HID+稳定性  |
| **D6** | 抗干扰优化        | UI最终美化      | 性能/内存优化      |
| **D7** | 演示彩排         | 展示准备        | 全流程彩排+预案     |

***

## 九、常见问题

### Q: 编译报找不到某个头文件？

A: 检查 `main/CMakeLists.txt` 中的 `INCLUDE_DIRS` 是否包含对应目录。

### Q: 如何添加新的 esp-sr / LVGL 组件？

A: 在 `main/idf_component.yml` 中添加依赖：

```yaml
dependencies:
  espressif/esp-sr: "~1.0"
  espressif/esp_lvgl_port: "^2.0.0"
```

然后 `idf.py reconfigure` 会自动下载。

### Q: 修改了引脚定义在哪里改？

A: 统一在 `common/board_pin.h` 中修改，所有模块引用这个文件。

### Q: 如何新增一个命令词？

A:

1. 在 `cmd_table.h` 的 `cmd_id_t` 中加枚举值
2. 在 `g_cmd_table` 数组中加条目
3. 在 `ctrl_manager.c` 的 `ctrl_manager_execute_cmd()` 中加处理逻辑
4. 在语音模型中训练对应的命令词

***

## 十、联系方式

- 团队：仅剩三小时
- 团队编号：28095
- 赛题：乐鑫科技赛题 — 智能交互驱动的AIoT应用系统

