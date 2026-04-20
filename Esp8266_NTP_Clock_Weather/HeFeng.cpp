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

// HeFeng Weather API Client Implementation
// 和风天气API客户端实现文件

#include "HeFeng.h"

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>

// 构造函数
HeFeng::HeFeng() {}

// 更新当前天气数据
// 参数: data - 存储天气数据的结构体指针
//       key - 和风天气API密钥
//       location - 地理位置代码
void HeFeng::DoUpdateCurr(HeFengCurrentData* data, String key,
                          String location) {
  // 创建安全的HTTPS客户端
  std::unique_ptr<BearSSL::WiFiClientSecure> client(
      new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  
  // 构建API请求URL
  String url = "https://free-api.heweather.net/s6/weather/now?lang=en&location=" +
               location + "&key=" + key;
  Serial.print("[HTTPS] begin...\n");
  
  // 发起HTTPS请求
  if (https.begin(*client, url)) {
    int http_code = https.GET();
    
    if (http_code > 0) {
      Serial.printf("[HTTPS] GET... code: %d\n", http_code);
      
      // 请求成功，解析JSON响应
      if (http_code == HTTP_CODE_OK ||
          http_code == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(payload);
        
        // 解析JSON数据
        DynamicJsonDocument json_buffer(2048);
        deserializeJson(json_buffer, payload);
        JsonObject root = json_buffer.as<JsonObject>();

        // 提取天气数据
        String tmp = root["HeWeather6"][0]["now"]["tmp"];
        data->tmp = tmp;
        String fl = root["HeWeather6"][0]["now"]["fl"];
        data->fl = fl;
        String hum = root["HeWeather6"][0]["now"]["hum"];
        data->hum = hum;
        String wind_sc = root["HeWeather6"][0]["now"]["wind_sc"];
        data->wind_sc = wind_sc;
        String cond_code = root["HeWeather6"][0]["now"]["cond_code"];
        String meteocon_icon = GetMeteoconIcon(cond_code);
        String cond_txt = root["HeWeather6"][0]["now"]["cond_txt"];
        data->cond_txt = cond_txt;
        data->icon_meteo_con = meteocon_icon;
      }
    } else {
      // HTTP请求失败
      Serial.printf("[HTTPS] GET... failed, error: %s\n",
                    https.errorToString(http_code).c_str());
      // 设置默认错误值
      data->tmp = "-1";
      data->fl = "-1";
      data->hum = "-1";
      data->wind_sc = "-1";
      data->cond_txt = "no network";
      data->icon_meteo_con = ")";
    }
    https.end();
  } else {
    // HTTPS连接失败
    Serial.printf("[HTTPS] Unable to connect\n");
    // 设置默认错误值
    data->tmp = "-1";
    data->fl = "-1";
    data->hum = "-1";
    data->wind_sc = "-1";
    data->cond_txt = "no network";
    data->icon_meteo_con = ")";
  }
}

// 更新天气预报数据
// 参数: data - 存储天气预报数据的数组指针
//       key - 和风天气API密钥
//       location - 地理位置代码
void HeFeng::DoUpdateFore(HeFengForeData* data, String key,
                          String location) {
  // 创建安全的HTTPS客户端
  std::unique_ptr<BearSSL::WiFiClientSecure> client(
      new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  
  // 构建API请求URL
  String url =
      "https://free-api.heweather.net/s6/weather/forecast?lang=en&location=" +
      location + "&key=" + key;
  Serial.print("[HTTPS] begin...\n");
  
  // 发起HTTPS请求
  if (https.begin(*client, url)) {
    int http_code = https.GET();
    
    if (http_code > 0) {
      Serial.printf("[HTTPS] GET... code: %d\n", http_code);
      
      // 请求成功，解析JSON响应
      if (http_code == HTTP_CODE_OK ||
          http_code == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(payload);
        
        // 解析JSON数据
        DynamicJsonDocument json_buffer(8192);
        deserializeJson(json_buffer, payload);
        JsonObject root = json_buffer.as<JsonObject>();
        
        // 提取3天的天气预报数据
        for (int i = 0; i < 3; i++) {
          String tmp_min = root["HeWeather6"][0]["daily_forecast"][i]["tmp_min"];
          data[i].tmp_min = tmp_min;
          String tmp_max = root["HeWeather6"][0]["daily_forecast"][i]["tmp_max"];
          data[i].tmp_max = tmp_max;
          String datestr = root["HeWeather6"][0]["daily_forecast"][i]["date"];
          data[i].date_str = datestr.substring(5, datestr.length());
          String cond_code =
              root["HeWeather6"][0]["daily_forecast"][i]["cond_code_d"];
          String meteocon_icon = GetMeteoconIcon(cond_code);
          data[i].icon_meteo_con = meteocon_icon;
        }
      }
    } else {
      // HTTP请求失败
      Serial.printf("[HTTPS] GET... failed, error: %s\n",
                    https.errorToString(http_code).c_str());
      // 设置默认错误值
      for (int i = 0; i < 3; i++) {
        data[i].tmp_min = "-1";
        data[i].tmp_max = "-1";
        data[i].date_str = "N/A";
        data[i].icon_meteo_con = ")";
      }
    }
    https.end();
  } else {
    // HTTPS连接失败
    Serial.printf("[HTTPS] Unable to connect\n");
    // 设置默认错误值
    for (int i = 0; i < 3; i++) {
      data[i].tmp_min = "-1";
      data[i].tmp_max = "-1";
      data[i].date_str = "N/A";
      data[i].icon_meteo_con = ")";
    }
  }
}

// 根据天气代码获取对应的图标字符
// 参数: cond_code - 和风天气的天气状况代码
// 返回: 对应的图标字符
String HeFeng::GetMeteoconIcon(String cond_code) {
  // 天气代码与图标的映射关系
  if (cond_code == "100" || cond_code == "9006") {
    return "B";  // 晴
  }
  if (cond_code == "999") {
    return ")";  // 未知
  }
  if (cond_code == "104") {
    return "D";  // 阴
  }
  if (cond_code == "500") {
    return "E";  // 薄雾
  }
  if (cond_code == "503" || cond_code == "504" || cond_code == "507" ||
      cond_code == "508") {
    return "F";  // 扬沙/沙尘暴
  }
  if (cond_code == "499" || cond_code == "901") {
    return "G";  // 热
  }
  if (cond_code == "103") {
    return "H";  // 晴间多云
  }
  if (cond_code == "502" || cond_code == "511" || cond_code == "512" ||
      cond_code == "513") {
    return "L";  // 强沙尘暴
  }
  if (cond_code == "501" || cond_code == "509" || cond_code == "510" ||
      cond_code == "514" || cond_code == "515") {
    return "M";  // 雾
  }
  if (cond_code == "102") {
    return "N";  // 多云
  }
  if (cond_code == "213") {
    return "O";  // 冰雹
  }
  if (cond_code == "302" || cond_code == "303") {
    return "P";  // 雷阵雨
  }
  if (cond_code == "305" || cond_code == "308" || cond_code == "309" ||
      cond_code == "314" || cond_code == "399") {
    return "Q";  // 小雨
  }
  if (cond_code == "306" || cond_code == "307" || cond_code == "310" ||
      cond_code == "311" || cond_code == "312" || cond_code == "315" ||
      cond_code == "316" || cond_code == "317" || cond_code == "318") {
    return "R";  // 中雨/大雨
  }
  if (cond_code == "200" || cond_code == "201" || cond_code == "202" ||
      cond_code == "203" || cond_code == "204" || cond_code == "205" ||
      cond_code == "206" || cond_code == "207" || cond_code == "208" ||
      cond_code == "209" || cond_code == "210" || cond_code == "211" ||
      cond_code == "212") {
    return "S";  // 雷暴
  }
  if (cond_code == "300" || cond_code == "301") {
    return "T";  // 阵雨
  }
  if (cond_code == "400" || cond_code == "408") {
    return "U";  // 小雪
  }
  if (cond_code == "407") {
    return "V";  // 雨夹雪
  }
  if (cond_code == "401" || cond_code == "402" || cond_code == "403" ||
      cond_code == "409" || cond_code == "410") {
    return "W";  // 中雪/大雪
  }
  if (cond_code == "304" || cond_code == "313" || cond_code == "404" ||
      cond_code == "405" || cond_code == "406") {
    return "X";  // 冻雨/雨夹雪
  }
  if (cond_code == "101") {
    return "Y";  // 多云转晴
  }
  return ")";  // 默认未知图标
}
