/**
 * ESP8266 NTP Clock Weather Station - 显示管理器实现
 * 负责OLED显示的所有功能实现
 */

#include "display_manager.h"

#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

// 构造函数
DisplayManager::DisplayManager() 
    : display_(kDisplayI2cAddress, kSdaPin, kSclPin), 
      ui_(&display_),
      current_temperature_("-1.0") {
  // 初始化帧数组
  frames_[0] = DrawDateTime;
  frames_[1] = DrawCurrentWeather;
  frames_[2] = DrawForecast;
  overlays_[0] = DrawHeaderOverlay;
}

// 初始化显示
void DisplayManager::Initialize() {
  display_.init();
  display_.clear();
  display_.display();
  
  display_.setFont(ArialMT_Plain_10);
  display_.setTextAlignment(TEXT_ALIGN_CENTER);
  display_.setContrast(255);
  
  // 配置UI
  ui_.setTargetFPS(kTargetFps);
  ui_.setActiveSymbol(activeSymbole);
  ui_.setInactiveSymbol(inactiveSymbole);
  ui_.setIndicatorPosition(BOTTOM);
  ui_.setIndicatorDirection(LEFT_RIGHT);
  ui_.setFrameAnimation(SLIDE_LEFT);
  ui_.setFrames(frames_, 3);
  ui_.setTimePerFrame(kFrameAnimationDelay);
  ui_.setOverlays(overlays_, 1);
  
  ui_.init();
}

// 显示连接WiFi的动画
void DisplayManager::ShowWifiConnecting(int counter) {
  display_.clear();
  display_.drawString(64, 10, "Connecting to WiFi");
  display_.drawXbm(46, 30, 8, 8, 
                   counter % 3 == 0 ? activeSymbole : inactiveSymbole);
  display_.drawXbm(60, 30, 8, 8, 
                   counter % 3 == 1 ? activeSymbole : inactiveSymbole);
  display_.drawXbm(74, 30, 8, 8, 
                   counter % 3 == 2 ? activeSymbole : inactiveSymbole);
  display_.display();
}

// 显示AP模式配置页面
void DisplayManager::ShowApConfigPage(int counter) {
  display_.clear();
  display_.drawString(64, 5, "WIFI AP:wifi_clock");
  display_.drawString(64, 20, "192.168.4.1");
  display_.drawString(64, 35, "waiting for config wifi.");
  display_.drawXbm(46, 50, 8, 8, 
                   counter % 3 == 0 ? activeSymbole : inactiveSymbole);
  display_.drawXbm(60, 50, 8, 8, 
                   counter % 3 == 1 ? activeSymbole : inactiveSymbole);
  display_.drawXbm(74, 50, 8, 8, 
                   counter % 3 == 2 ? activeSymbole : inactiveSymbole);
  display_.display();
}

// 显示进度条
void DisplayManager::ShowProgress(int percentage, const String& label) {
  display_.clear();
  display_.setTextAlignment(TEXT_ALIGN_CENTER);
  display_.setFont(ArialMT_Plain_10);
  display_.drawString(64, 10, label);
  display_.drawProgressBar(2, 28, 124, 10, percentage);
  display_.display();
}

// 更新UI
int DisplayManager::UpdateUi() {
  return ui_.update();
}

// 获取UI状态
OLEDDisplayUiState* DisplayManager::GetUiState() {
  return ui_.getUiState();
}

// 设置当前温度
void DisplayManager::SetCurrentTemperature(const String& temp) {
  current_temperature_ = temp;
}

// 设置天气数据
void DisplayManager::SetWeatherData(const HeFengCurrentData& current_weather, 
                                   const HeFengForeData* forecast_data) {
  current_weather_ = current_weather;
  for (int i = 0; i < 3; i++) {
    forecast_data_[i] = forecast_data[i];
  }
}

// 绘制日期时间帧
void DisplayManager::DrawDateTime(OLEDDisplay* display, OLEDDisplayUiState* state, 
                                 int16_t x, int16_t y) {
  time_t now = time(nullptr);
  struct tm* time_info = localtime(&now);
  char buff[16];

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  
  sprintf_P(buff, PSTR("%04d-%02d-%02d, %s"), time_info->tm_year + 1900,
            time_info->tm_mon + 1, time_info->tm_mday,
            kWeekdayNames[time_info->tm_wday].c_str());
  display->drawString(64 + x, 5 + y, String(buff));
  
  display->setFont(ArialMT_Plain_24);
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), time_info->tm_hour, 
            time_info->tm_min, time_info->tm_sec);
  display->drawString(64 + x, 22 + y, String(buff));
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

// 绘制当前天气帧
void DisplayManager::DrawCurrentWeather(OLEDDisplay* display, 
                                       OLEDDisplayUiState* state, 
                                       int16_t x, int16_t y) {
  // 获取显示管理器实例
  DisplayManager* manager = reinterpret_cast<DisplayManager*>(state->user_data);
  
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y,
                      manager->current_weather_.cond_txt + " | Wind: " +
                      manager->current_weather_.wind_sc);

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp = manager->current_weather_.tmp + "°C";
  display->drawString(60 + x, 3 + y, temp);
  
  display->setFont(ArialMT_Plain_10);
  display->drawString(70 + x, 26 + y,
                      manager->current_weather_.fl + "°C | " + 
                      manager->current_weather_.hum + "%");
  
  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(32 + x, 0 + y, manager->current_weather_.icon_meteo_con);
}

// 绘制天气预报帧
void DisplayManager::DrawForecast(OLEDDisplay* display, OLEDDisplayUiState* state, 
                                 int16_t x, int16_t y) {
  // 获取显示管理器实例
  DisplayManager* manager = reinterpret_cast<DisplayManager*>(state->user_data);
  
  DrawForecastDetails(display, x, y, 0);
  DrawForecastDetails(display, x + 44, y, 1);
  DrawForecastDetails(display, x + 88, y, 2);
}

// 绘制天气预报详情
void DisplayManager::DrawForecastDetails(OLEDDisplay* display, int x, int y, 
                                        int day_index) {
  // 获取显示管理器实例
  DisplayManager* manager = nullptr; // 需要通过其他方式获取
  
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y, manager->forecast_data_[day_index].date_str);
  
  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 12, 
                      manager->forecast_data_[day_index].icon_meteo_con);

  String temp = manager->forecast_data_[day_index].tmp_min + " | " + 
                manager->forecast_data_[day_index].tmp_max;
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 34, temp);
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

// 绘制头部覆盖层
void DisplayManager::DrawHeaderOverlay(OLEDDisplay* display, 
                                      OLEDDisplayUiState* state) {
  // 获取显示管理器实例
  DisplayManager* manager = reinterpret_cast<DisplayManager*>(state->user_data);
  
  time_t now = time(nullptr);
  struct tm* time_info = localtime(&now);
  char buff[14];
  sprintf_P(buff, PSTR("%02d:%02d"), time_info->tm_hour, time_info->tm_min);

  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(6, 54, String(buff));
  
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  String temp = manager->current_temperature_ + "°C";
  display->drawString(128, 54, temp);
  
  display->drawHorizontalLine(0, 52, 128);
}