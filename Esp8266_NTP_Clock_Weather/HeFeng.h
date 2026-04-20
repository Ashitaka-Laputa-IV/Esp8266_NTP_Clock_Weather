/**The MIT License (MIT)

Copyright (c) 2018 by Daniel Eichhorn - ThingPulse

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at https://thingpulse.com
*/

// HeFeng Weather API Client Header
// 和风天气API客户端头文件
// 提供获取当前天气和天气预报的功能

#ifndef HEFENG_H_
#define HEFENG_H_

#include <ArduinoJson.h>

// 当前天气数据结构
struct HeFengCurrentData {
  String cond_txt;        // 天气状况描述
  String fl;              // 体感温度
  String tmp;             // 当前温度
  String hum;             // 湿度
  String wind_sc;         // 风力等级
  String icon_meteo_con;  // 天气图标代码
};

// 天气预报数据结构
struct HeFengForeData {
  String date_str;        // 日期字符串
  String tmp_min;         // 最低温度
  String tmp_max;         // 最高温度
  String icon_meteo_con;  // 天气图标代码
};

// 和风天气API客户端类
class HeFeng {
 private:
  // 根据天气代码获取对应的图标字符
  String GetMeteoconIcon(String cond_code);

 public:
  // 构造函数
  HeFeng();
  
  // 更新当前天气数据
  void DoUpdateCurr(HeFengCurrentData* data, String key, String location);
  
  // 更新天气预报数据
  void DoUpdateFore(HeFengForeData* data, String key, String location);
};

#endif  // HEFENG_H_
