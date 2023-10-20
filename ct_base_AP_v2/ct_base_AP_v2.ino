#include "WiFi.h"
#include <WiFiClient.h>
#include "M5Atom.h"
#include "config.h"
#include <WebServer.h>
#include <esp_wifi.h>
#include <WiFiClientSecure.h>
#include "UniversalTelegramBot.h"
#include <HTTPClient.h>

// Connection state
boolean connected = false;

//The udp library class

// Telegram
HTTPClient http;
int len;
bool dataAvailable = false;
unsigned long bot_lasttime;  // last time messages' scan has been done
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
byte buff[40000] = { 0 };
int currentByte;
hw_timer_t* timer0 = NULL;

int32_t mytimer;
WebServer server(80);
IPAddress ip;

void deepsleep() {
  M5.dis.drawpix(0, 0x000000);
  delay(150);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);  //1 = High, 0 = Low
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}

// after wd triggered, go to sleep mode
/*void ARDUINO_ISR_ATTR sleepWD() {
  to_sleep = true;
}*/


void setupWiFi() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  WiFi.begin(ssidName, ssidPswd);
  M5.dis.drawpix(0, 0xff0000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  M5.dis.drawpix(0, 0x000000);
  WiFi.setSleep(false);
  ip = WiFi.localIP();
  Serial.println(F("WiFi connected"));
  Serial.println("");
  Serial.println(ip);
}
void setup_server() {
  server.on("/", HTTP_GET, handle_help);
  server.on("/rele1", HTTP_GET, handle_rele1);
  server.on("/rele2", HTTP_GET, handle_rele1);
  server.on("/emergency", HTTP_GET, handleemergency);
  server.on("/auth_emergency", HTTP_GET, handleauthemergency);
  server.on("/notify", HTTP_GET, handlenotification);
  server.onNotFound(handleNotFound);
  server.begin();
}
void handlenotification() {
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\nResponse: 200";
  server.send(200, "text/plain", message);
  sendImg(CHATID);
  message = "Someone knocking at the door!\n";
  bot.sendMessage(CHATID, message, "HTML");
}

void handleNotFound() {
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\nResponse: 404";
  server.send(404, "text/plain", message);
}
void handle_rele1() {
  String message = "Triggering rele 1!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  M5.dis.drawpix(0, 0x0000ff);
  digitalWrite(RELE1, HIGH);
  delay(500);
  digitalWrite(RELE1, LOW);
  server.send(200, "text/plain", message);
  M5.dis.drawpix(0, 0x000000);
}
void handle_rele2() {
  String message = "Triggering rele 2!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  M5.dis.drawpix(0, 0x0000ff);
  digitalWrite(RELE2, HIGH);
  delay(500);
  digitalWrite(RELE2, LOW);
  server.send(200, "text/plain", message);
  M5.dis.drawpix(0, 0x000000);
}

void handle_help() {
  String message = "Usage:\n\n";
  message += "/rele1: Trigger rele 1\n";
  message += "/rele2: Trigger rele 2\n";
  message += "/emergency: Call emergency via telegram\n";
  server.send(200, "text/plain", message);
}

void setup() {
  M5.begin(true, false, true);
  M5.dis.drawpix(0, 0x000000);
  Serial.begin(115200);
  pinMode(RELE1, OUTPUT);
  pinMode(RELE2, OUTPUT);
  digitalWrite(RELE1, LOW);
  digitalWrite(RELE2, LOW);
  Serial.println(" * Configuring WiFi");
  setupWiFi();
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  // Add root certificate for api.telegram.org
  Serial.println(" * Configuring HTTP Server");
  setup_server();
  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org");  // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);
}

void loop() {
  server.handleClient();
  delay(2);  //allow the cpu to switch to other tasks
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
}