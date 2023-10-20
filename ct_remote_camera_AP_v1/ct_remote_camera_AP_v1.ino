

#include "OV2640.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include "OV2640Streamer.h"



//#define SOFTAP_MODE // If you want to run our own softap turn this on
#define ENABLE_WEBSERVER

#define LED_BUILTIN 4
#define SIG_OUT 13
#define SIG_IN 12

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

OV2640 cam;

#ifdef ENABLE_WEBSERVER
WebServer server(80);
#endif


#ifdef SOFTAP_MODE
IPAddress apIP = IPAddress(192, 168, 1, 1);
#else
#include "wifikeys.h"
#endif

#ifdef ENABLE_WEBSERVER
void handle_jpg_stream(void) {
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  while (1) {
    cam.run();
    if (!client.connected())
      break;
    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);

    client.write((char *)cam.getfb(), cam.getSize());
    server.sendContent("\r\n");
    if (!client.connected())
      break;
  }
}

void handle_jpg(void) {
  WiFiClient client = server.client();

  cam.run();
  if (!client.connected()) {
    return;
  }
  int img_size = cam.getSize();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-disposition: inline; filename=capture.jpg\r\n";
  response += "Content-Length: ";
  response += img_size;
  response += "\r\n";
  response += "Content-type: image/jpeg\r\n\r\n";
  server.sendContent(response);
  client.write((char *)cam.getfb(), cam.getSize());
}
void handle_snap(void) {
  WiFiClient client = server.client();

  cam.run();
  cam.getfb();
  cam.run();
  if (!client.connected()) {
    return;
  }
  int img_size = cam.getSize();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-disposition: inline; filename=capture.jpg\r\n";
  response += "Content-Length: ";
  response += img_size;
  response += "\r\n";
  response += "Content-type: image/jpeg\r\n\r\n";
  server.sendContent(response);
  client.write((char *)cam.getfb(), cam.getSize());
}

void ledon() {
  String message = "Led On!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  digitalWrite(LED_BUILTIN, HIGH);
  server.send(200, "text/plain", message);
}
void ledoff() {
  String message = "Led Off!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  digitalWrite(LED_BUILTIN, LOW);
  server.send(200, "text/plain", message);
}

void wake_mic() {
  String message = "Waking up Audio streaming!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  digitalWrite(SIG_OUT, HIGH);
  delay(500);
  digitalWrite(SIG_OUT, LOW);
  server.send(200, "text/plain", message);
}

void handleNotFound() {
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);
}
#endif

void lcdMessage(String msg) {
#ifdef ENABLE_OLED
  if (hasDisplay) {
    display.clear();
    display.drawString(128 / 2, 32 / 2, msg);
    display.display();
  }
#endif
}

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SIG_OUT, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(SIG_OUT, LOW);
#ifdef ENABLE_OLED
  hasDisplay = display.init();
  if (hasDisplay) {
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
  }
#endif
  lcdMessage("booting");

  Serial.begin(115200);
  //while (!Serial);            //wait for serial connection.

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 16500000;
  config.pixel_format = PIXFORMAT_JPEG;   // JPEG
  //config.frame_size = FRAMESIZE_QVGA; // QGA
  config.frame_size = FRAMESIZE_QVGA; // QGA
  config.jpeg_quality = 10;
  config.fb_count = 1;

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  cam.init(config);

  IPAddress ip;

#ifdef SOFTAP_MODE
  const char *hostname = "devcam";
  // WiFi.hostname(hostname); // FIXME - find out why undefined
  lcdMessage("starting softAP");
  WiFi.mode(WIFI_AP);

  bool result = WiFi.softAP(hostname, "12345678", 1, 0);
  delay(2000);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  if (!result) {
    Serial.println("AP Config failed.");
    return;
  } else {
    Serial.println("AP Config Success.");
    Serial.print("AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());

    ip = WiFi.softAPIP();

    Serial.print("Stream Link: rtsp://");
    Serial.print(ip);
    Serial.println(":8554/mjpeg/1");
  }
#else
  lcdMessage(String("join ") + ssid);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  ip = WiFi.localIP();
  Serial.println(F("WiFi connected"));
  Serial.println("");
  Serial.println(ip);
#endif

  lcdMessage(ip.toString());

#ifdef ENABLE_WEBSERVER
  server.on("/", HTTP_GET, handle_jpg_stream);
  server.on("/jpg", HTTP_GET, handle_jpg);
  server.on("/snap", HTTP_GET, handle_snap);
  server.on("/ledon",HTTP_GET, ledon);
  server.on("/ledoff",HTTP_GET, ledoff);
  server.on("/audio",HTTP_GET, wake_mic);
  server.onNotFound(handleNotFound);
  server.begin();
#endif



}

CStreamer *streamer;
WiFiClient client;  // FIXME, support multiple clients

void loop() {
#ifdef ENABLE_WEBSERVER
  server.handleClient();
#endif

}
