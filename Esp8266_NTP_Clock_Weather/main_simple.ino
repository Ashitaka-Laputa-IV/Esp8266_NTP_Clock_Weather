/**
 * ESP8266 NTP时钟天气站 - 简化版
 * 基于和风天气API的智能时钟天气显示
 * 
 * 项目特点：
 * - 代码结构简洁，适合初学者学习
 * - 注释详细，逻辑清晰
 * - 功能完整，包含所有核心模块
 * - 易于修改和扩展
 * 
 * 主要功能：
 * - 显示当前时间和日期（通过NTP网络时间协议同步）
 * - 显示当前天气信息（温度和风天气API获取）
 * - 显示未来3天天气预报
 * - 支持WiFi自动配置和网页配网
 * - 使用DS18B20温度传感器监测环境温度
 * - OLED显示屏图形化输出
 * 
 * 硬件要求：
 * - ESP8266开发板（NodeMCU、Wemos D1 Mini等）
 * - OLED显示屏（SSD1306或SH1106，128x64）
 * - DS18B20温度传感器
 * - WiFi网络连接
 * 
 * 引脚连接：
 * - OLED SDA -> D2 (GPIO4)
 * - OLED SCL -> D5 (GPIO14) 
 * - DS18B20数据线 -> D7 (GPIO13)
 * 
 * 使用步骤：
 * 1. 修改配置参数（WiFi、API密钥等）
 * 2. 连接硬件
 * 3. 编译上传到ESP8266
 * 4. 设备启动后会自动连接WiFi
 * 5. 如果自动连接失败，可通过网页配网
 * 6. 设备将显示时间、天气和温度信息
 */

// ========== 引入必要的库文件 ==========

#include <Arduino.h>          // Arduino核心库
#include <DS18B20.h>          // 温度传感器库
#include <time.h>             // 时间处理库

#include <ESP8266WiFi.h>      // ESP8266 WiFi功能
#include <ESP8266WebServer.h> // Web服务器（用于配网）

#include "HeFeng.h"           // 和风天气API客户端
#include "SH1106Wire.h"       // OLED显示驱动（SH1106芯片）
#include "OLEDDisplayUi.h"    // OLED UI管理
#include "WeatherStationFonts.h"  // 字体定义
#include "WeatherStationImages.h" // 图像和图标定义

// ========== 系统配置参数 ==========
// 注意：以下参数需要根据实际情况修改

// WiFi网络配置
const char* WIFI_SSID = "";      // 您的WiFi网络名称（SSID）
const char* WIFI_PASSWORD = "";  // 您的WiFi密码

// 和风天气API配置（需要申请免费API密钥）
const char* HEFENG_KEY = "";      // 和风天气API密钥，从官网申请
const char* HEFENG_LOCATION = ""; // 城市代码，如："beijing"或"shanghai"

// 硬件引脚配置（根据实际接线修改）
const int DISPLAY_ADDRESS = 0x3c;  // OLED显示屏I2C地址（通常为0x3c）
const int SDA_PIN = D2;            // I2C数据线（SDA）连接的引脚
const int SCL_PIN = D5;            // I2C时钟线（SCL）连接的引脚
const int TEMP_SENSOR_PIN = D7;    // DS18B20温度传感器数据引脚

// 系统参数配置
const int TIMEZONE = -8;           // 时区设置（中国为UTC+8，这里填-8）

// 数据更新间隔（单位：秒）
const int WEATHER_UPDATE_INTERVAL = 20 * 60;  // 天气数据更新间隔（20分钟）
const int TEMP_UPDATE_INTERVAL = 10;          // 温度传感器更新间隔（10秒）

// ========== 全局变量定义 ==========

// 硬件设备对象
DS18B20 tempSensor(TEMP_SENSOR_PIN);  // 温度传感器对象
SH1106Wire display(DISPLAY_ADDRESS, SDA_PIN, SCL_PIN);  // OLED显示对象
OLEDDisplayUi ui(&display);           // UI管理器

// 天气数据存储
HeFeng weatherClient;                 // 天气API客户端
HeFengCurrentData currentWeather;     // 当前天气数据
HeFengForeData forecastData[3];       // 3天天气预报数据

// 系统状态变量
String currentTemp = "-1.0";          // 当前温度值（字符串格式）
bool needWeatherUpdate = false;       // 是否需要更新天气数据的标志
long lastWeatherUpdate = 0;           // 上次天气更新时间戳
long lastTempUpdate = 0;              // 上次温度更新时间戳

// ========== 函数声明 ==========
// 提前声明所有函数，便于代码组织

void setup();        // 系统初始化函数
void loop();         // 主循环函数

// WiFi连接相关函数
bool autoConnectWifi();      // 自动连接WiFi
void webConfigWifi();        // 网页配网功能

// 显示绘制函数（由UI框架回调）
void drawDateTime(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawHeader(OLEDDisplay* display, OLEDDisplayUiState* state);
void showProgress(OLEDDisplay* display, int percent, String label);

// 数据更新函数
void updateWeatherData();    // 更新天气数据
void updateTemperature();    // 更新温度数据

// ========== 主程序入口 ==========

/**
 * setup函数 - 系统初始化
 * 在设备启动时执行一次，用于初始化所有硬件和配置
 */
void setup() {
  // 初始化串口通信，用于调试输出
  Serial.begin(115200);
  Serial.println("\nESP8266 NTP时钟天气站启动...");
  Serial.println("版本：简化版");
  Serial.println("作者：基于ThingPulse项目修改");

  // 初始化OLED显示屏
  display.init();              // 初始化显示驱动
  display.clear();             // 清空屏幕
  display.display();           // 更新显示
  display.setFont(ArialMT_Plain_10);      // 设置默认字体
  display.setTextAlignment(TEXT_ALIGN_CENTER); // 设置文本对齐方式
  Serial.println("OLED显示屏初始化完成");

  // 配置UI框架参数
  ui.setTargetFPS(30);                     // 设置目标帧率为30FPS
  ui.setActiveSymbol(activeSymbole);       // 设置活动状态图标
  ui.setInactiveSymbol(inactiveSymbole);   // 设置非活动状态图标
  ui.setIndicatorPosition(BOTTOM);         // 设置指示器位置在底部
  ui.setIndicatorDirection(LEFT_RIGHT);    // 设置指示器方向
  ui.setFrameAnimation(SLIDE_LEFT);        // 设置帧切换动画为向左滑动

  // 设置显示帧（三个主要页面）
  FrameCallback frames[] = { 
    drawDateTime,        // 第一帧：日期时间
    drawCurrentWeather,  // 第二帧：当前天气
    drawForecast         // 第三帧：天气预报
  };
  ui.setFrames(frames, 3);                 // 注册3个显示帧
  ui.setTimePerFrame(7500);                // 每帧显示7.5秒

  // 设置覆盖层（在每帧上叠加显示的信息）
  OverlayCallback overlays[] = { drawHeader };  // 头部信息覆盖层
  ui.setOverlays(overlays, 1);             // 注册1个覆盖层

  ui.init();  // 初始化UI框架
  Serial.println("UI框架初始化完成");

  // 连接WiFi网络
  Serial.println("开始连接WiFi...");
  if (!autoConnectWifi()) {
    // 如果自动连接失败，启动网页配网模式
    Serial.println("自动连接失败，启动网页配网");
    webConfigWifi();
  }

  // 配置NTP网络时间同步
  Serial.println("配置NTP时间同步...");
  configTime(TIMEZONE * 3600, 0, "pool.ntp.org", "0.cn.pool.ntp.org");
  
  // 首次获取天气数据
  Serial.println("首次获取天气数据...");
  updateWeatherData();
  
  Serial.println("系统初始化完成，开始主循环");
  Serial.println("================================");
}

/**
 * loop函数 - 主循环
 * 在setup完成后循环执行，处理实时任务
 */
void loop() {
  // 检查是否需要更新天气数据（基于时间间隔）
  unsigned long currentTime = millis();
  if (currentTime - lastWeatherUpdate > (WEATHER_UPDATE_INTERVAL * 1000L)) {
    needWeatherUpdate = true;          // 设置更新标志
    lastWeatherUpdate = currentTime;   // 更新最后更新时间
    Serial.println("检测到需要更新天气数据");
  }

  // 检查是否需要更新温度数据
  if (currentTime - lastTempUpdate > (TEMP_UPDATE_INTERVAL * 1000L)) {
    updateTemperature();               // 更新温度数据
    lastTempUpdate = currentTime;      // 更新最后更新时间
  }

  // 在合适的时机更新天气数据（避免在动画过程中更新）
  if (needWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateWeatherData();               // 执行天气数据更新
    needWeatherUpdate = false;         // 清除更新标志
    Serial.println("天气数据更新完成");
  }

  // 更新UI显示（返回剩余的时间预算）
  int remainingTime = ui.update();
  
  // 如果还有剩余时间，可以执行其他任务或延时
  if (remainingTime > 0) {
    delay(remainingTime);  // 延时以避免过度占用CPU
  }
}

// ========== WiFi网络功能 ==========

/**
 * autoConnectWifi - 自动连接WiFi
 * 尝试使用保存的凭据自动连接WiFi
 * 
 * @return bool - 连接成功返回true，失败返回false
 */
bool autoConnectWifi() {
  Serial.println("尝试自动连接WiFi...");
  
  WiFi.mode(WIFI_STA);        // 设置WiFi模式为工作站模式
  WiFi.begin();               // 开始连接（使用保存的凭据）
  
  // 尝试连接，最多等待10秒（20次*500ms）
  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      // 连接成功
      Serial.println("WiFi连接成功！");
      Serial.print("IP地址: ");
      Serial.println(WiFi.localIP());
      Serial.print("信号强度: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
      return true;
    }
    
    delay(500);      // 等待500ms
    Serial.print(".");  // 显示连接进度
  }
  
  // 连接失败
  Serial.println("\n自动连接WiFi失败");
  return false;
}

/**
 * webConfigWifi - 网页配网功能
 * 当自动连接失败时，启动AP模式并提供网页配置界面
 */
void webConfigWifi() {
  Serial.println("启动网页配网模式...");
  
  // 设置WiFi为AP+STA模式（同时支持接入点和客户端）
  WiFi.mode(WIFI_AP_STA);
  
  // 启动接入点（热点）
  WiFi.softAP("weather_clock");  // 热点名称
  Serial.print("AP热点已启动，SSID: weather_clock, IP: ");
  Serial.println(WiFi.softAPIP());
  
  // 创建Web服务器
  ESP8266WebServer server(80);
  
  // 配置Web路由
  
  // 主页 - 显示配网表单
  server.on("/", []() {
    String html = "<html><head><meta charset='UTF-8'><title>WiFi配置</title></head><body>";
    html += "<h2>ESP8266天气时钟 - WiFi配置</h2>";
    html += "<p>请填写您的WiFi信息：</p>";
    html += "<form action='/connect' method='GET'>";
    html += "WiFi名称 (SSID): <input type='text' name='ssid' required><br><br>";
    html += "WiFi密码: <input type='password' name='password'><br><br>";
    html += "<input type='submit' value='连接WiFi'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
  });
  
  // 连接处理页面
  server.on("/connect", []() {
    String ssid = server.arg("ssid");      // 获取SSID参数
    String password = server.arg("password"); // 获取密码参数
    
    Serial.print("接收到配网请求，SSID: ");
    Serial.println(ssid);
    
    server.send(200, "text/html", "<html><body><h2>正在连接WiFi...</h2></body></html>");
    
    // 开始连接指定的WiFi
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // 等待连接结果（最多30秒）
    for (int i = 0; i < 30; i++) {
      if (WiFi.status() == WL_CONNECTED) {
        break;  // 连接成功，退出循环
      }
      delay(1000);  // 等待1秒
    }
  });
  
  server.begin();  // 启动Web服务器
  Serial.println("Web服务器已启动，可通过浏览器访问配网页面");
  
  // 在OLED上显示配网信息
  display.clear();
  display.drawString(64, 10, "WiFi配网模式");
  display.drawString(64, 25, "SSID: weather_clock");
  display.drawString(64, 40, "IP: 192.168.4.1");
  display.display();
  
  // 等待WiFi连接成功
  Serial.println("等待用户配置WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    server.handleClient();  // 处理Web请求
    delay(100);             // 短暂延时
  }
  
  // 连接成功，清理资源
  server.stop();           // 停止Web服务器
  WiFi.mode(WIFI_STA);     // 切换回工作站模式
  Serial.println("网页配网成功！");
}

// ========== 显示绘制功能 ==========

/**
 * drawDateTime - 绘制日期时间页面
 * UI框架回调函数，用于显示当前日期和时间
 * 
 * @param display - 显示对象指针
 * @param state - UI状态信息
 * @param x - 绘制起始X坐标
 * @param y - 绘制起始Y坐标
 */
void drawDateTime(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // 获取当前时间
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  
  // 格式化日期字符串
  char dateStr[20];
  sprintf(dateStr, "%04d-%02d-%02d %s", 
          timeinfo->tm_year + 1900,   // 年份（需要加1900）
          timeinfo->tm_mon + 1,       // 月份（0-11，需要加1）
          timeinfo->tm_mday,          // 日期
          // 星期几的英文缩写
          (timeinfo->tm_wday == 0) ? "Sun" : 
          (timeinfo->tm_wday == 1) ? "Mon" :
          (timeinfo->tm_wday == 2) ? "Tue" :
          (timeinfo->tm_wday == 3) ? "Wed" :
          (timeinfo->tm_wday == 4) ? "Thu" :
          (timeinfo->tm_wday == 5) ? "Fri" : "Sat");
  
  // 格式化时间字符串
  char timeStr[10];
  sprintf(timeStr, "%02d:%02d:%02d", 
          timeinfo->tm_hour,    // 小时
          timeinfo->tm_min,     // 分钟
          timeinfo->tm_sec);    // 秒钟
  
  // 绘制日期（使用16号字体）
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 5 + y, String(dateStr));
  
  // 绘制时间（使用24号字体，更显眼）
  display->setFont(ArialMT_Plain_24);
  display->drawString(64 + x, 25 + y, String(timeStr));
}

/**
 * drawCurrentWeather - 绘制当前天气页面
 * 显示当前温度、天气状况、体感温度和湿度
 */
void drawCurrentWeather(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // 绘制天气状况和风力信息（底部小字）
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y, 
                      currentWeather.cond_txt + " | 风力: " + currentWeather.wind_sc);

  // 绘制当前温度（大字体，左侧）
  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp = currentWeather.tmp + "°C";
  display->drawString(60 + x, 3 + y, temp);
  
  // 绘制体感温度和湿度（小字体，温度下方）
  display->setFont(ArialMT_Plain_10);
  display->drawString(70 + x, 26 + y, 
                      currentWeather.fl + "°C | " + currentWeather.hum + "%");
  
  // 绘制天气图标（右侧，使用特殊字体）
  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(32 + x, 0 + y, currentWeather.icon_meteo_con);
}

/**
 * drawForecast - 绘制天气预报页面
 * 显示未来3天的天气预测，水平排列
 */
void drawForecast(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // 循环绘制3天的天气预报
  for (int i = 0; i < 3; i++) {
    int posX = x + i * 44;  // 每项间隔44像素
    
    // 绘制日期
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(posX + 20, y, forecastData[i].date_str);
    
    // 绘制天气图标
    display->setFont(Meteocons_Plain_21);
    display->drawString(posX + 20, y + 12, forecastData[i].icon_meteo_con);
    
    // 绘制温度范围（最低-最高）
    String temp = forecastData[i].tmp_min + " | " + forecastData[i].tmp_max;
    display->setFont(ArialMT_Plain_10);
    display->drawString(posX + 20, y + 34, temp);
  }
}

/**
 * drawHeader - 绘制头部覆盖层
 * 在每个页面的底部显示当前时间和传感器温度
 */
void drawHeader(OLEDDisplay* display, OLEDDisplayUiState* state) {
  // 获取当前时间（只显示小时和分钟）
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  
  // 设置显示属性
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  
  // 在左侧显示时间
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(6, 54, String(timeStr));
  
  // 在右侧显示传感器温度
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  String temp = currentTemp + "°C";
  display->drawString(128, 54, temp);
  
  // 绘制分隔线
  display->drawHorizontalLine(0, 52, 128);
}

/**
 * showProgress - 显示进度条
 * 用于显示数据更新进度
 * 
 * @param display - 显示对象
 * @param percent - 进度百分比（0-100）
 * @param label - 进度标签文字
 */
void showProgress(OLEDDisplay* display, int percent, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);           // 显示标签文字
  display->drawProgressBar(2, 28, 124, 10, percent);  // 绘制进度条
  display->display();
}

// ========== 数据更新功能 ==========

/**
 * updateWeatherData - 更新天气数据
 * 从和风天气API获取当前天气和预报数据
 */
void updateWeatherData() {
  Serial.println("开始更新天气数据...");
  
  // 显示更新进度
  showProgress(&display, 30, "更新天气中...");
  
  // 尝试获取当前天气数据（最多重试5次）
  for (int i = 0; i < 5; i++) {
    weatherClient.DoUpdateCurr(&currentWeather, HEFENG_KEY, HEFENG_LOCATION);
    if (currentWeather.cond_txt != "no network") {
      Serial.println("当前天气数据获取成功");
      break;  // 成功获取，退出重试循环
    }
    Serial.println("当前天气数据获取失败，重试...");
    delay(1000);  // 等待1秒后重试
  }
  
  // 更新进度显示
  showProgress(&display, 60, "更新预报中...");
  
  // 尝试获取天气预报数据（最多重试5次）
  for (int i = 0; i < 5; i++) {
    weatherClient.DoUpdateFore(forecastData, HEFENG_KEY, HEFENG_LOCATION);
    if (forecastData[0].date_str != "N/A") {
      Serial.println("天气预报数据获取成功");
      break;  // 成功获取，退出重试循环
    }
    Serial.println("天气预报数据获取失败，重试...");
    delay(1000);  // 等待1秒后重试
  }
  
  // 显示完成进度
  showProgress(&display, 100, "完成");
  delay(1000);  // 显示完成状态1秒
  
  Serial.println("天气数据更新完成");
}

/**
 * updateTemperature - 更新温度传感器数据
 * 从DS18B20传感器读取当前环境温度
 */
void updateTemperature() {
  // 读取温度值并保留1位小数
  currentTemp = String(tempSensor.getTempC(), 1);
  
  // 调试输出（可选）
  // Serial.print("温度传感器读数: ");
  // Serial.println(currentTemp);
}