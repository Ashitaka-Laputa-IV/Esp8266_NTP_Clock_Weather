/**
 * ESP8266 NTP Clock Weather Station - 配置文件
 * 配置参数和常量定义
 */

#ifndef CONFIG_H_
#define CONFIG_H_

// WiFi配置
constexpr char kWifiSsid[] = "";  // WiFi名称
constexpr char kWifiPassword[] = "";  // WiFi密码

// 和风天气API配置
constexpr char kHefengKey[] = "";  // 和风天气API密钥
constexpr char kHefengLocation[] = "";  // 地理位置代码

// 时区配置
constexpr int kTimezone = -8;  // 时区 (UTC+8)
constexpr int kDstMinutes = 0;  // 夏令时分钟数

// 更新间隔配置
constexpr int kWeatherUpdateIntervalSecs = 20 * 60;  // 天气更新间隔 (20分钟)
constexpr int kTemperatureUpdateIntervalSecs = 10;  // 温度传感器更新间隔 (10秒)

// 显示配置
constexpr int kDisplayI2cAddress = 0x3c;  // OLED显示I2C地址
constexpr int kSdaPin = D2;  // SDA引脚
constexpr int kSclPin = D5;  // SCL引脚

// 温度传感器配置
constexpr int kDs18b20Pin = D7;  // DS18B20数据引脚

// 日期时间显示配置
const String kWeekdayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String kMonthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// NTP服务器配置
constexpr char* kNtpServers[] = {
  "pool.ntp.org",
  "0.cn.pool.ntp.org", 
  "1.cn.pool.ntp.org"
};
constexpr int kNtpServerCount = 3;

// 显示动画配置
constexpr int kFrameAnimationDelay = 7500;  // 帧切换延迟 (毫秒)
constexpr int kTargetFps = 30;  // 目标帧率

// 计算时区相关常量
constexpr int kTzSeconds = kTimezone * 3600;  // 时区秒数
constexpr int kDstSeconds = kDstMinutes * 60;  // 夏令时秒数

#endif  // CONFIG_H_
