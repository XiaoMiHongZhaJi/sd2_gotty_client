#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoWebsockets.h>
#include <Arduino.h>
#include "base64.hpp"
using namespace websockets;
#include <WiFiClientSecure.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "TFTTerminal.h"

#define LCD_BL_PIN 5

String SMOD = "";     //串口数据存储
int LCD_BL_PWM = 30;  //屏幕亮度0-100，默认30

// WiFi 配置
const char* ssid = "MYWIFI";
const char* password = "12222222";

void requestURL(String url);
void handleHttp(const String& url);
void handleWebSocket(const String& url);

WebsocketsClient client;

TFT_eSPI tft = TFT_eSPI();
TFTTerminal term(tft);

void setup() {
  Serial.begin(115200);
  Serial.println("Serial start...");

  pinMode(LCD_BL_PIN, OUTPUT);
  analogWrite(LCD_BL_PIN, 1023 - (LCD_BL_PWM * 10));
  Serial.println("Backlight ON");

  term.tft_init();

  term.println("Hello World");
  Serial.println("Hello World printed to TFT");

  Serial.println("Connecting WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi Connected!");
  term.println("WiFi Connected!");
  delay(1000);
}

void Serial_set() {
  String incomingByte = "";
  if (Serial.available() > 0) {
    while (Serial.available() > 0) {
      incomingByte += char(Serial.read());  //读取单个字符值，转换为字符，并按顺序一个个赋值给incomingByte
      delay(2);                             //不能省略，因为读取缓冲区数据需要时间
    }
    String text = incomingByte.c_str();

    if (SMOD == "1") {
      if (incomingByte == "0") {
        SMOD = "";
        Serial.println("已退出输入");
        return;
      }
      Serial.println("输出文本: " + text);
      term.printWithEmoji(text);
    } else if (SMOD == "2") {
      if (incomingByte == "0") {
        SMOD = "";
        Serial.println("已退出输入");
        if (client.available()) {
          client.close();
        }
        return;
      }
      requestURL(text);
    } else if (SMOD == "3") {
      //int n = atoi(xxx.c_str());//String转int
      int LCDBL = atoi(incomingByte.c_str());
      if (LCDBL >= 0 && LCDBL <= 100) {
        delay(5);
        SMOD = "";
        Serial.printf("亮度已调整为: ");
        analogWrite(LCD_BL_PIN, 1023 - (LCDBL * 10));
        Serial.println(LCDBL);
        Serial.println("");
      } else {
        Serial.println("亮度调整错误, 请输入 0 - 100");
      }
    } else if (SMOD == "4") {
      int RoSet = atoi(incomingByte.c_str());
      if (RoSet >= 0 && RoSet <= 3) {
        SMOD = "";
        tft.setRotation(RoSet);
        tft.fillScreen(0x0000);
        term.println("Hello World");
        Serial.print("屏幕方向已设置为：");
        Serial.println(RoSet);
      } else {
        Serial.println("屏幕方向值错误, 请输入0-3内的值");
      }
    } else if (incomingByte == "9") {
      Serial.println("准备重置...");
      ESP.restart();
    } else if (incomingByte == "0") {
      SMOD = "";
      Serial.println("已退出输入");
    } else {
      SMOD = incomingByte;
      delay(2);
      if (SMOD == "1")
        Serial.println("请输入要显示的文本: (0 退出)");
      else if (SMOD == "2")
        Serial.println("请输入要访问的网址: (0 退出)");
      else if (SMOD == "3")
        Serial.println("请输入亮度值: 0 - 100");
      else if (SMOD == "4")
        Serial.println("请输入屏幕方向: 0 - 3");
      else {
        Serial.println("");
        Serial.println("请输入指令: ");
        Serial.println("1: 显示文本");
        Serial.println("2: 网络请求");
        Serial.println("3: 设置亮度");
        Serial.println("4: 设置屏幕方向");
        Serial.println("9: 重置");
        Serial.println("0: 退出连续输入");
        Serial.println("");
      }
    }
  }
}

// ============= HTTP 请求 =============
void handleHttp(const String& url) {

  Serial.println("HTTP GET: " + url);

  if (WiFi.status() == WL_CONNECTED) {
    if (url.startsWith("https://")) {
      WiFiClientSecure client;
      client.setInsecure();  // 跳过证书检查
      HTTPClient https;
      if (https.begin(client, url)) {
        int httpCode = https.GET();
        if (httpCode > 0) {
          Serial.println("HTTP " + String(httpCode));
          String payload = https.getString();
          term.println(payload);
        } else {
          term.println("HTTP ERR " + String(httpCode));
        }
        https.end();
      }
    } else {
      HTTPClient http;
      http.begin(url);
      int httpCode = http.GET();
      if (httpCode > 0) {
        term.println("HTTP " + String(httpCode));
        String payload = http.getString();
        term.println(payload);
      } else {
        term.println("HTTP ERR " + String(httpCode));
      }
      http.end();
    }
  } else {
    term.println("WiFi not connected!");
  }
}

String base64DecodeToString(const String& input) {
  int len = input.length();

  // 创建可写缓冲区，并复制输入
  unsigned char in[len + 1];
  memcpy(in, input.c_str(), len);
  in[len] = '\0';  // null terminator

  // 输出缓冲区
  unsigned char out[len];
  unsigned int decodedLen = decode_base64(in, out);

  // 添加 null terminator
  out[decodedLen] = '\0';

  return String((char*)out);
}

String trim(const String& s) {
  int start = 0;
  int end = s.length() - 1;

  // 去掉前面的空白字符
  while (start <= end && isspace((unsigned char)s[start])) {
    start++;
  }

  // 去掉后面的空白字符
  while (end >= start && isspace((unsigned char)s[end])) {
    end--;
  }

  if (start > end) return "";  // 全是空白
  return s.substring(start, end + 1);
}

// ============= WebSocket =============
void onMessageCallback(WebsocketsMessage message) {

  String msg = message.data();
  //Serial.println("onMessageCallback data: " + msg);

  if (msg.length() < 1) return;

  char channel = msg[0];

  String content = msg.substring(1);
  // Serial.println("onMessageCallback content: " + content);

  String decoded = base64DecodeToString(content);
  Serial.println("onMessageCallback: " + decoded);

  decoded = trim(decoded);
  if (decoded.length() > 0) {
    term.printWithEmoji(decoded);
  }
}


void handleWebSocket(const String& requestURL) {
  String url = requestURL;
  Serial.println("WebSocket: " + url);

  // 自动补全前缀，只处理 ws:// 或 wss://
  if (!url.startsWith("ws://") && !url.startsWith("wss://")) {
    url = "ws://" + url;  // 默认 ws://
  }

  // 连接 WebSocket
  client.onMessage(onMessageCallback);
  client.onEvent([&](WebsocketsClient& c, WebsocketsEvent e, String d) {
    Serial.println("WebSocket onEvent");
    if (e == WebsocketsEvent::ConnectionOpened) {

      Serial.println("WebSocket Connection Opened");
      term.println("[*] Connected to gotty");

      // String jsonStr = "{\"Input\":\"\"}";
      String jsonStr = "{\"method\":\"subscribe\",\"source\":\"pc-web\",\"product\":\"snapshot\",\"items\":[{\"code\":\"002415\",\"name\":\"海康威视\",\"market\":\"ab\",\"financeType\":\"stock\"}]}";
      c.send(jsonStr);

    } else if (e == WebsocketsEvent::ConnectionClosed) {

      Serial.println("WebSocket Connection Closed");
      term.println("[*] Disconnected from gotty");
    }
  });

  term.println("Connecting to WebSocket...");
  Serial.println("Connecting to WebSocket: " + url);
  if (client.connect(url)) {
    term.println("WebSocket connection started");
    Serial.println("WebSocket connection started");
    client.ping();
  } else {
    term.println("WebSocket connection failed");
    Serial.println("WebSocket connection failed");
    // 显示更详细信息
    Serial.print("URL: ");
    Serial.println(url);

    // WiFi 连接状态
    wl_status_t wifiStatus = WiFi.status();
    Serial.print("WiFi status: ");
    Serial.println(wifiStatus);

    // 显示本地 IP
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());

// 如果库支持错误信息，可以加上
#ifdef WEBSOCKETS_HAS_STATUS
    Serial.print("WebSocket last error: ");
    Serial.println(client.lastError());
#endif
  }
}

// ============= 统一入口 =============
void requestURL(String url) {
  url.trim();
  if (url.startsWith("http://") || url.startsWith("https://")) {
    handleHttp(url);
  } else if (url.startsWith("ws://") || url.startsWith("wss://")) {
    handleWebSocket(url);
  } else {
    // 默认补 http://
    handleHttp("http://" + url);
  }
}

unsigned long lastPing = 0;
void loop() {
  // 空
  Serial_set();
  client.poll();
  if (millis() - lastPing > 20000) {  // 每20秒发一次
    if (client.available()) {
      client.ping();
      Serial.println("Sent keep-alive ping: " + lastPing);
    }
    lastPing = millis();
  }
}
