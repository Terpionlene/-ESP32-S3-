# PPT Controller - WiFi Version

基于WiFi+HTTP的PPT翻页控制方案，无需蓝牙，通过ESP32连接WiFi发送HTTP请求到电脑端Python服务器，实现PPT翻页和音量控制。

## 工作原理

```
ESP32旋钮 → WiFi → HTTP请求 → Python服务器 → Windows API → PPT翻页/音量调节
```

## 文件结构

```
PPT_solution/
├── CMakeLists.txt          # 项目CMake配置
├── sdkconfig.defaults      # ESP-IDF默认配置
├── main/
│   ├── CMakeLists.txt      # 主组件CMake配置
│   └── app_main.c          # ESP32主程序
└── python_server/
    └── ppt_controller.py   # 电脑端Python服务器
```

## 配置说明

### 1. ESP32端配置

编辑 `main/app_main.c` 修改以下宏定义：

```c
#define WIFI_SSID "你的WiFi名称"
#define WIFI_PASSWORD "你的WiFi密码"
#define SERVER_IP "电脑的IP地址"
#define SERVER_PORT 8765
```

### 2. 旋钮引脚配置

| 引脚 | 功能 |
|------|------|
| GPIO1 | EC11编码器A相 |
| GPIO2 | EC11编码器B相 |
| GPIO4 | EC11编码器按键 |

## 使用步骤

### 步骤1：获取电脑IP地址

在电脑上打开命令提示符，输入：
```cmd
ipconfig
```
找到无线局域网适配器的IPv4地址，例如 `192.168.1.100`

### 步骤2：修改ESP32代码

将 `main/app_main.c` 中的 `SERVER_IP` 修改为电脑的IP地址。

### 步骤3：编译烧录ESP32

在VS Code中使用ESP-IDF扩展编译并烧录。

### 步骤4：运行Python服务器

在电脑上打开命令提示符，进入 `python_server` 目录：

```cmd
cd PPT_solution\python_server
python ppt_controller.py
```

看到以下输出表示服务器启动成功：
```
PPT Controller Server started on 0.0.0.0:8765
Waiting for ESP32 connections...
```

### 步骤5：打开PPT并开始演示

打开PPT文件进入放映模式，然后：
- **顺时针旋转旋钮** → PPT下一页
- **逆时针旋转旋钮** → PPT上一页
- **按下旋钮** → 音量增加

## 支持的控制指令

| 指令 | 动作 | Windows按键 |
|------|------|-------------|
| next | PPT下一页 | Page Down |
| prev | PPT上一页 | Page Up |
| volume_up | 音量增加 | Volume Up |
| volume_down | 音量减少 | Volume Down |

## 注意事项

1. ESP32和电脑必须连接到同一个WiFi网络
2. 确保电脑防火墙允许端口8765的入站连接
3. 如果连接失败，请检查电脑IP地址是否正确
4. Python服务器需要在Windows环境下运行