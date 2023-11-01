| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- |


# 通过 Rainmaker 来实时显示天气以及家电开关控制示例

本示例基于 ESP-IDF 来实现通过手机 APP ESP-RainMaker 来远程控制 WS2812 灯带、显示实时的天气信息还有发送提醒。

本项目实现了以下功能：

- 使用 ESP-IDF 模版进行 Wi-Fi 连接
- HTTP 请求
- JSON 格式的数据运用/解析
- 驱动 WS2812 
- RainMaker 云端交互

本实例提供详细的视频讲解：[ESP-IDF VSCode 开发 【沉浸式教程】](https://www.bilibili.com/video/BV1X34y1M7L8/)

# 示例使用说明

## **准备硬件**

  - **ESP32-C3** 开发板，如 [ESP32-C3-DevKitM-1](https://item.taobao.com/item.htm?spm=a1z10.5-c.w4002-8715811636.13.4d1d570eddzv6K&id=638189770914)
  - **USB 数据线** （A 转 Micro-B）
  - 电脑（Windows、Linux 或 macOS）
  - 杜邦线
  - WS2812 灯环（本示例使用 16 个 LED 版本）

## **搭建开发环境**

### **安装 VSCode**

- 如果你还没有安装 VSCode，请先下载并安装它。

1. 从 [VSCode 官网](https://code.visualstudio.com/Download) 获取安装包 
2. 使用下载的 `.exe`文件，一键安装 VSCode

### **安装 ESP-IDF 插件**

- 更详细的安装流程可以参考 [乐鑫 B 站教学视频](https://www.bilibili.com/video/BV1V24y1T75n/)

1. 在 VSCode 界面左边进入插件管理界面，搜索 esp-idf，点击下载带有乐鑫图标的插件
2. 通过左上角的 `view` 打开命令面板，搜索 configure 然后打开安装配置界面
3. 选择 EXPRESS 使用快速安装模式进行安装，更详细的过程和配置请参考 [乐鑫 B 站教学视频](https://www.bilibili.com/video/BV1V24y1T75n/)

### **手机下载 ESP RainMaker APP**

- [ESP-RainMaker 手机 APP 下载链接](https://gitee.com/EspressifSystems/esp-rainmaker#phone-apps)

## **获取百度地图 API 密钥**

   - 前往 [百度开发者平台](http://lbsyun.baidu.com/) 点击 **创建应用** 获取开发者密钥。

# **配置项目**

## 必要配置
- 在 **Kconfig.projbuild** 中配置 `HTTP_URL_ADDRESS`（百度地图 API 服务地址）

    ```c
    HTTP_URL_ADDRESS = "https://api.map.baidu.com/weather/v1/?district_id=区县的行政区划编码&data_type=all&ak=百度开发者密钥";
    ```

## 自定义选配
- 根据自己的芯片或者开发板型号在 **Kconfig.projbuild** 中配置 `BOARD_GPIO_BOOT` （BOOT 键对应的 GPIO）。默认为 ESP32-C3 的 GPIO 9
- 在 **Kconfig.projbuild** 中配置 `LED_GPIO_INPUT` (控制 WS2812 的 GPIO)。默认为 GPIO 10
- 在 **Kconfig.projbuild** 中配置 `LED_COUNT` (LED 的数量)。默认为 16 个

## **编译和烧录**

- 参考 [ESP-IDF VSCode 开发 【沉浸式教程】](https://www.bilibili.com/video/BV1X34y1M7L8/)

# 示例输出

```shell
I (32209) esp_rmaker_time: SNTP Synchronised.
I (32209) esp_rmaker_time: The current time is: Tue Oct 24 19:23:18 2023 +0800[CST], DST: No.
I (44859) esp_rmaker_param: Received params: {"今日天气":{"get_weather":true}}
I (44859) HTTP_CLIENT: Received write request via : Cloud
I (44859) HTTP_CLIENT: Device name: 今日天气 - Device param: get_weather
I (44869) HTTP_CLIENT: Received value = true for 今日天气 - get_weather
I (44879) HTTP_CLIENT: weather_get = true 
I (44879) esp_rmaker_param: Reporting params: {"今日天气":{"get_weather":true}}
I (45779) APP_DRIVER: HTTP GET Status = 200, content_length = 1150
I (45789) APP_DRIVER: 地区 浦东新区
I (45789) APP_DRIVER: 天气 晴
I (45789) APP_DRIVER: 温度 22
I (45789) APP_DRIVER: 湿度 82
I (45799) APP_DRIVER: 风力 0级
I (45799) esp_rmaker_param: Reporting Time Series Data for 今日天气.Temperature
I (45809) esp_rmaker_param: Reporting params: {"今日天气":{"Temperature":22.00000}}
I (45819) esp_rmaker_param: Reporting params: {"今日天气":{"天气":"晴"}}
I (45829) esp_rmaker_param: Reporting params: {"今日天气":{"地区":"浦东新区"}}
I (45839) esp_rmaker_param: Reporting params: {"今日天气":{"风力":"0级"}}
I (45849) esp_rmaker_param: Reporting params: {"今日天气":{"湿度":82}}
I (45949) esp_rmaker_param: Reporting alert: {"esp.alert.str":"高温天气！小心中暑"}
I (45949) APP_DRIVER: HTTP_EVENT_DISCONNECTED
```

# 故障排除

程序上传失败
- 硬件连接不正确，检查 **COM 端口** 和 **USB 驱动**

ESP RainMaker APP 闪退
- 检查 ESP RainMaker APP 是否更新到最新版本

# 技术支持和反馈

- 对于技术问题，请访问 [esp32.com](https://esp32.com/) 论坛
- 对于功能请求或错误报告，请创建 [GitHub issue](https://github.com/espressif/esp-idf/issues) 问题

