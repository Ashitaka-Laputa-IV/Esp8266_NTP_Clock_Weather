/**
 * ESP8266 NTP Clock Weather Station - 显示管理器
 * 负责OLED显示的所有功能
 */

#ifndef DISPLAY_MANAGER_H_
#define DISPLAY_MANAGER_H_

#include <OLEDDisplayUi.h>
#include "SH1106Wire.h"
#include "config.h"
#include "HeFeng.h"

class DisplayManager {
 public:
  // 构造函数
  DisplayManager();
  
  // 初始化显示
  void Initialize();
  
  // 显示连接WiFi的动画
  void ShowWifiConnecting(int counter);
  
  // 显示AP模式配置页面
  void ShowApConfigPage(int counter);
  
  // 显示进度条
  void ShowProgress(int percentage, const String& label);
  
  // 更新UI
  int UpdateUi();
  
  // 获取UI状态
  OLEDDisplayUiState* GetUiState();
  
  // 设置当前温度
  void SetCurrentTemperature(const String& temp);
  
  // 设置天气数据
  void SetWeatherData(const HeFengCurrentData& current_weather, 
                      const HeFengForeData* forecast_data);

 private:
  SH1106Wire display_;  // OLED显示对象
  OLEDDisplayUi ui_;    // UI管理器
  
  // 显示数据
  String current_temperature_;
  HeFengCurrentData current_weather_;
  HeFengForeData forecast_data_[3];
  
  // 帧回调函数
  static void DrawDateTime(OLEDDisplay* display, OLEDDisplayUiState* state, 
                          int16_t x, int16_t y);
  static void DrawCurrentWeather(OLEDDisplay* display, OLEDDisplayUiState* state, 
                                int16_t x, int16_t y);
  static void DrawForecast(OLEDDisplay* display, OLEDDisplayUiState* state, 
                          int16_t x, int16_t y);
  static void DrawHeaderOverlay(OLEDDisplay* display, OLEDDisplayUiState* state);
  
  // 绘制天气预报详情
  static void DrawForecastDetails(OLEDDisplay* display, int x, int y, int day_index);
  
  // 帧数组
  FrameCallback frames_[3];
  OverlayCallback overlays_[1];
};

#endif  // DISPLAY_MANAGER_H_
