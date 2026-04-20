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

// Monsteryuan forked from Daniel Eichhorn/ThingPulse ESP8266 Weather Station

#include <Arduino.h>
#include <DS18B20.h>
#include <coredecls.h>
#include <sys/time.h>
#include <time.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include "HeFeng.h"
#include "OLEDDisplayUi.h"
#include "SH1106Wire.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "Wire.h"

// Begin Settings

DS18B20 ds(D7);

constexpr int kTz = -8;
constexpr int kDstMn = 0;

constexpr int kUpdateIntervalSecs = 20 * 60;
constexpr int kUpdateCurrIntervalSecs = 10;

constexpr int kI2cDisplayAddress = 0x3c;
#if defined(ESP8266)
constexpr int kSdaPin = D2;
constexpr int kSdcPin = D5;
#endif

const String kWdayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String kMonthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// End Settings

SH1106Wire display(kI2cDisplayAddress, kSdaPin, kSdcPin);
OLEDDisplayUi ui(&display);

HeFengCurrentData current_weather;
HeFengForeData fore_weather[3];
HeFeng he_feng_client;

constexpr int kTzMn = (kTz)*60;
constexpr int kTzSec = (kTz)*3600;
constexpr int kDstSec = (kDstMn)*60;

const char* kHefengKey = "";
const char* kHefengLocation = "";
time_t now;

bool ready_for_weather_update = false;

String last_update = "--";

long time_since_last_w_update = 0;
long time_since_last_curr_update = 0;

String curr_temp = "-1.0";

// Function prototypes
void DrawProgress(OLEDDisplay* display, int percentage, String label);
void UpdateData(OLEDDisplay* display);
void DrawDateTime(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x,
                  int16_t y);
void DrawCurrentWeather(OLEDDisplay* display, OLEDDisplayUiState* state,
                        int16_t x, int16_t y);
void DrawForecast(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x,
                  int16_t y);
void DrawForecastDetails(OLEDDisplay* display, int x, int y, int day_index);
void DrawHeaderOverlay(OLEDDisplay* display, OLEDDisplayUiState* state);
void SetReadyForWeatherUpdate();

FrameCallback frames[] = {DrawDateTime, DrawCurrentWeather, DrawForecast};
int number_of_frames = 3;

OverlayCallback overlays[] = {DrawHeaderOverlay};
int number_of_overlays = 1;

bool AutoConfig() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  Serial.print("AutoConfig Waiting......");
  int counter = 0;
  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("AutoConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      WiFi.printDiag(Serial);
      return true;
    } else {
      delay(500);
      Serial.print(".");
      display.clear();
      display.drawString(64, 10, "Connecting to WiFi");
      display.drawXbm(46, 30, 8, 8,
                      counter % 3 == 0 ? activeSymbole : inactiveSymbole);
      display.drawXbm(60, 30, 8, 8,
                      counter % 3 == 1 ? activeSymbole : inactiveSymbole);
      display.drawXbm(74, 30, 8, 8,
                      counter % 3 == 2 ? activeSymbole : inactiveSymbole);
      display.display();
      counter++;
    }
  }
  Serial.println("AutoConfig Faild!");
  return false;
}

ESP8266WebServer server(80);
String kHtmlTitle =
    "<!DOCTYPE html><html><head><meta http-equiv=\"Content-Type\" "
    "content=\"text/html; charset=utf-8\"><meta name=\"viewport\" "
    "content=\"width=device-width, initial-scale=1.0\"><meta "
    "http-equiv=\"X-UA-Compatible\" content=\"ie=edge\"><title>ESP8266网页配网"
    "</title>";
String kHtmlScriptOne =
    "<script type=\"text/javascript\">function wifi(){var ssid = "
    "s.value;var password = p.value;var xmlhttp=new "
    "XMLHttpRequest();xmlhttp.open(\"GET\",\"/"
    "HandleWifi?ssid=\"+ssid+\"&password=\"+password,true);xmlhttp.send();"
    "xmlhttp.onload = function(e){alert(this.responseText);}}</script>";
String kHtmlScriptTwo =
    "<script>function "
    "c(l){document.getElementById('s').value=l.innerText||l.textContent;"
    "document.getElementById('p').focus();}</script>";
String kHtmlHeadBodyBegin = "</head><body>请输入wifi信息进行配网:";
String kHtmlFormOne =
    "<form>WiFi名称：<input id='s' name='s' type=\"text\" "
    "placeholder=\"请输入您WiFi的名称\"><br>WiFi密码：<input id='p' name='p' "
    "type=\"text\" placeholder=\"请输入您WiFi的密码\"><br><input "
    "type=\"button\" value=\"扫描\" onclick=\"window.location.href = "
    "'/HandleScanWifi'\"><input type=\"button\" value=\"连接\" "
    "onclick=\"wifi()\"></form>";
String kHtmlBodyHtmlEnd = "</body></html>";

void HandleRoot() {
  Serial.println("root page");
  String str = kHtmlTitle + kHtmlScriptOne + kHtmlScriptTwo +
               kHtmlHeadBodyBegin + kHtmlFormOne + kHtmlBodyHtmlEnd;
  server.send(200, "text/html", str);
}

void HandleScanWifi() {
  Serial.println("scan start");

  String html_form_table_begin =
      "<table><head><tr><th>序号</th><th>名称</th><th>强度</th></tr></"
      "head><body>";
  String html_form_table_end = "</body></table>";
  String html_form_table_con = "";
  String html_table;
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
    html_table = "NO WIFI !!!";
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
      html_form_table_con =
          html_form_table_con + "<tr><td align=\"center\">" +
          String(i + 1) + "</td><td align=\"center\">" +
          "<a href='#p' onclick='c(this)'>" + WiFi.SSID(i) +
          "</a>" + "</td><td align=\"center\">" + WiFi.RSSI(i) + "</td></tr>";
    }
    html_table =
        html_form_table_begin + html_form_table_con + html_form_table_end;
  }
  Serial.println("");

  String scanstr = kHtmlTitle + kHtmlScriptOne + kHtmlScriptTwo +
                   kHtmlHeadBodyBegin + kHtmlFormOne + html_table +
                   kHtmlBodyHtmlEnd;

  server.send(200, "text/html", scanstr);
}

void HandleWifi() {
  String wifis = server.arg("ssid");
  String wifip = server.arg("password");
  Serial.println("received:" + wifis);
  server.send(200, "text/html", "连接中..");
  WiFi.begin(wifis, wifip);
}

void HandleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void HtmlConfig() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("wifi_clock");

  IPAddress my_ip = WiFi.softAPIP();

  if (MDNS.begin("clock")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", HandleRoot);
  server.on("/HandleWifi", HTTP_GET, HandleWifi);
  server.on("/HandleScanWifi", HandleScanWifi);
  server.onNotFound(HandleNotFound);
  MDNS.addService("http", "tcp", 80);
  server.begin();
  Serial.println("HTTP server started");
  int counter = 0;
  while (1) {
    server.handleClient();
    MDNS.update();
    delay(500);
    display.clear();
    display.drawString(64, 5, "WIFI AP:wifi_clock");
    display.drawString(64, 20, "192.168.4.1");
    display.drawString(64, 35, "waiting for config wifi.");
    display.drawXbm(46, 50, 8, 8,
                    counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 50, 8, 8,
                    counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 50, 8, 8,
                    counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();
    counter++;
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("HtmlConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      Serial.println("HTML连接成功");
      break;
    }
  }
  server.close();
  WiFi.mode(WIFI_STA);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  display.init();
  display.clear();
  display.display();

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  bool wifi_config = AutoConfig();
  if (wifi_config == false) {
    HtmlConfig();
  }

  ui.setTargetFPS(30);

  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);

  ui.setIndicatorPosition(BOTTOM);

  ui.setIndicatorDirection(LEFT_RIGHT);

  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, number_of_frames);
  ui.setTimePerFrame(7500);
  ui.setOverlays(overlays, number_of_overlays);

  ui.init();

  Serial.println("");
  configTime(kTzSec, kDstSec, "pool.ntp.org", "0.cn.pool.ntp.org",
             "1.cn.pool.ntp.org");
  UpdateData(&display);
}

void loop() {
  if (millis() - time_since_last_w_update > (1000L * kUpdateIntervalSecs)) {
    SetReadyForWeatherUpdate();
    time_since_last_w_update = millis();
  }
  if (millis() - time_since_last_curr_update >
      (1000L * kUpdateCurrIntervalSecs)) {
    if (ui.getUiState()->frameState == FIXED) {
      curr_temp = String(ds.getTempC(), 1);
      time_since_last_curr_update = millis();
    }
  }

  if (ready_for_weather_update && ui.getUiState()->frameState == FIXED) {
    UpdateData(&display);
  }

  int remaining_time_budget = ui.update();

  if (remaining_time_budget > 0) {
    delay(remaining_time_budget);
  }
}

void DrawProgress(OLEDDisplay* display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void UpdateData(OLEDDisplay* display) {
  DrawProgress(display, 30, "Updating weather...");

  for (int i = 0; i < 5; i++) {
    he_feng_client.DoUpdateCurr(&current_weather, kHefengKey, kHefengLocation);
    if (current_weather.cond_txt != "no network") {
      break;
    }
  }
  DrawProgress(display, 50, "Updating forecasts...");

  for (int i = 0; i < 5; i++) {
    he_feng_client.DoUpdateFore(fore_weather, kHefengKey, kHefengLocation);
    if (fore_weather[0].date_str != "N/A") {
      break;
    }
  }

  ready_for_weather_update = false;
  DrawProgress(display, 100, "Done...");
  delay(1000);
}

void DrawDateTime(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x,
                  int16_t y) {
  now = time(nullptr);
  struct tm* time_info;
  time_info = localtime(&now);
  char buff[16];

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  String date = kWdayNames[time_info->tm_wday];

  sprintf_P(buff, PSTR("%04d-%02d-%02d, %s"), time_info->tm_year + 1900,
            time_info->tm_mon + 1, time_info->tm_mday,
            kWdayNames[time_info->tm_wday].c_str());
  display->drawString(64 + x, 5 + y, String(buff));
  display->setFont(ArialMT_Plain_24);

  sprintf_P(buff, PSTR("%02d:%02d:%02d"), time_info->tm_hour, time_info->tm_min,
            time_info->tm_sec);
  display->drawString(64 + x, 22 + y, String(buff));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void DrawCurrentWeather(OLEDDisplay* display, OLEDDisplayUiState* state,
                        int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y,
                      current_weather.cond_txt + " | Wind: " +
                          current_weather.wind_sc);

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp = current_weather.tmp + "°C";
  display->drawString(60 + x, 3 + y, temp);
  display->setFont(ArialMT_Plain_10);
  display->drawString(70 + x, 26 + y,
                      current_weather.fl + "°C | " + current_weather.hum + "%");
  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(32 + x, 0 + y, current_weather.icon_meteo_con);
}

void DrawForecast(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x,
                  int16_t y) {
  DrawForecastDetails(display, x, y, 0);
  DrawForecastDetails(display, x + 44, y, 1);
  DrawForecastDetails(display, x + 88, y, 2);
}

void DrawForecastDetails(OLEDDisplay* display, int x, int y, int day_index) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y, fore_weather[day_index].date_str);
  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 12, fore_weather[day_index].icon_meteo_con);

  String temp =
      fore_weather[day_index].tmp_min + " | " + fore_weather[day_index].tmp_max;
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 34, temp);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void DrawHeaderOverlay(OLEDDisplay* display, OLEDDisplayUiState* state) {
  now = time(nullptr);
  struct tm* time_info;
  time_info = localtime(&now);
  char buff[14];
  sprintf_P(buff, PSTR("%02d:%02d"), time_info->tm_hour, time_info->tm_min);

  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(6, 54, String(buff));
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  String temp = curr_temp + "°C";
  display->drawString(128, 54, temp);
  display->drawHorizontalLine(0, 52, 128);
}

void SetReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  ready_for_weather_update = true;
}
