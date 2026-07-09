# Audio Repair - 百度语音识别修复方案

## 问题描述

调用百度语音识别API时出现错误：
```
{"err_no":500,"err_msg":"json param token error"}
```

## 文件结构

```
audio_repair/
├── app_main.cpp       # 修复后的完整代码
├── get_token.py       # Python脚本获取Token
├── PROBLEM_ANALYSIS.md # 问题分析文档
└── README.md          # 使用说明
```

## 修复内容

### 1. 自动获取Token

代码会在启动时自动调用百度OAuth API获取access_token，避免手动配置过期的token。

### 2. 完整错误处理

增加了详细的错误日志输出，方便定位问题：
- HTTP状态码
- 错误码和错误信息
- JSON解析失败原因

### 3. JSON构建优化

使用String类构建JSON，避免strcat缓冲区溢出和格式错误。

## 使用步骤

### 步骤1：获取百度API Key和Secret Key

1. 登录百度AI平台：https://console.bce.baidu.com/ai/
2. 创建语音识别应用
3. 获取API Key和Secret Key

### 步骤2：修改代码配置

编辑 `app_main.cpp`，修改以下配置：

```cpp
const char* ssid = "你的WiFi名称";
const char* password = "你的WiFi密码";
const char* BAIDU_API_KEY = "你的API Key";
const char* BAIDU_SECRET_KEY = "你的Secret Key";
```

### 步骤3：编译烧录

使用Arduino IDE编译并烧录到ESP32S3 Sense开发板。

### 步骤4：测试

打开串口监视器（波特率115200）：

| 命令 | 功能 |
|------|------|
| `1` | 开始录音并识别 |
| `t` | 获取新的Token |

## 调试日志示例

### 正常启动

```
========================================
ESP32S3 Sense Audio Repair
========================================
I2S initialized successfully
Connecting to WiFi: J09 502
.........................
WiFi connected! IP: 192.168.1.105

Getting Baidu token...
Token obtained successfully!

========================================
Ready! Send '1' to start recognition
========================================
```

### 成功识别

```
----------- Start Recording -----------
Recording complete!
----------- Sending to Baidu -----------
JSON length: 45230
Sending POST request...
Response: {"corpus_no":"666666","err_msg":"success.","err_no":0,"result":["你好"],"sn":"123456"}
Recognition result: 你好
Success! Result: 你好
----------- Recognition Done -----------
```

### Token过期错误

```
Response: {"err_no":500,"err_msg":"json param token error"}
Error code: 500
Error message: json param token error
```

## 常见问题

### Q1：Token获取失败

**原因**：API Key或Secret Key错误

**解决**：检查百度AI平台的应用配置，确保使用的是语音识别应用的API Key和Secret Key。

### Q2：识别结果为空

**原因**：音频数据采集问题或采样率不匹配

**解决**：检查I2S配置，确保采样率为16000Hz，且麦克风正常工作。

### Q3：HTTP请求失败

**原因**：网络连接问题或百度API服务异常

**解决**：检查WiFi连接，确保能访问互联网；稍后重试。

## 技术参数

| 参数 | 值 |
|------|------|
| 采样率 | 16000Hz |
| 采样位深 | 16位 |
| 录音时间 | 2秒 |
| 通道数 | 单声道 |
| 语言模型 | 中文普通话 (dev_pid: 1537) |