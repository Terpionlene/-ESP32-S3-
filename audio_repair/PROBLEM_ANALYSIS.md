# 百度语音识别 "json param token error" 问题分析

## 问题现象

调用百度语音识别API时，返回错误：
```
{"err_no":500,"err_msg":"json param token error"}
```

## 可能原因

### 1. Token过期或错误（最常见）

**原因**：百度语音识别API的access_token有效期为30天，过期后需要重新获取。

**验证方法**：
```cpp
// 在代码中添加调试信息
Serial.println("Using token: " + String(baidu_token));
```

**修复方法**：
- 使用 `get_token.py` 脚本获取新token
- 或调用 `baidu_get_token()` 函数自动获取

### 2. Token格式错误

**原因**：token中包含换行符、空格或其他非法字符。

**验证方法**：
```cpp
// 检查token长度（正常应该是很长的字符串）
Serial.println("Token length: " + String(strlen(baidu_token)));
```

**修复方法**：
- 确保token是完整的，没有被截断
- 确保token没有换行符或空格

### 3. JSON格式错误

**原因**：构建JSON字符串时格式不正确。

**常见错误**：
- 字符串拼接时缺少引号
- 数据长度计算错误
- Base64编码错误

**验证方法**：
```cpp
// 打印完整JSON（注意：数据很长，只打印前500字符）
Serial.println("JSON preview: " + jsonStr.substring(0, 500));
```

**修复方法**：
- 使用String类构建JSON，避免strcat错误
- 确保所有字段都正确格式化

### 4. 网络问题

**原因**：WiFi连接不稳定，导致请求超时或数据传输不完整。

**验证方法**：
```cpp
if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
}
```

**修复方法**：
- 确保WiFi信号稳定
- 增加HTTP超时时间

### 5. API Key/Secret Key错误

**原因**：在百度AI平台申请的API Key或Secret Key不正确。

**验证方法**：
- 登录百度AI平台检查：https://console.bce.baidu.com/ai/?fromai=1#/ai/speech/app/list
- 确保使用的是"语音识别"应用的API Key/Secret Key

## 修复代码改进点

### 改进1：自动获取Token

```cpp
bool baidu_get_token() {
    // 调用百度OAuth API获取token
    // 解析JSON响应提取access_token
    // 存储到baidu_token变量
}
```

### 改进2：完整错误处理

```cpp
if (httpCode == HTTP_CODE_OK) {
    // 检查response中是否包含result字段
    if (doc.containsKey("result")) {
        // 成功
    } else {
        // 失败，打印错误信息
        Serial.println("Error code: " + String(doc["err_no"].as<int>()));
        Serial.println("Error message: " + doc["err_msg"].as<String>());
    }
}
```

### 改进3：JSON构建优化

使用String类代替strcat，避免缓冲区溢出和格式错误：

```cpp
String jsonStr = String("{") +
    "\"format\":\"pcm\"," +
    "\"rate\":" + String(SAMPLE_RATE) + "," +
    "\"dev_pid\":1537," +
    "\"channel\":1," +
    "\"cuid\":\"esp32-sense-001\"," +
    "\"token\":\"" + String(baidu_token) + "\"," +
    "\"len\":" + String(data_len * 2) + "," +
    "\"speech\":\"" + base64::encode((uint8_t*)data, data_len * 2).c_str() + "\"" +
    "}";
```

## 调试步骤

1. **检查WiFi连接**：确保设备已连接WiFi
2. **获取Token**：发送 't' 命令获取新token
3. **检查Token**：确保token长度正常（约100+字符）
4. **开始录音**：发送 '1' 命令开始录音和识别
5. **查看日志**：根据日志定位具体问题

## 常见错误码

| 错误码 | 错误信息 | 解决方法 |
|--------|---------|---------|
| 500 | json param token error | Token错误或过期，重新获取 |
| 3300 | input error | 输入参数错误，检查JSON格式 |
| 3301 | audio error | 音频数据错误，检查采样率和格式 |
| 3302 | recognition error | 识别失败，检查音频质量 |
| 3303 | resource error | 资源错误，稍后重试 |
| 3304 | auth error | 认证错误，检查API Key和Secret Key |
| 3305 | quota error | 配额不足，检查API调用次数 |