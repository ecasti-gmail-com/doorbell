/*
#define SSID_NAME "WLAN-176633"
#define SSID_PASSWORD "1577193787313842"
*/


#include <M5CoreS3.h>
#include "config.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <M5GFX.h>
M5GFX display;

HTTPClient http;
IPAddress ip;
lgfx::touch_point_t tp[3];

bool receiving;
bool emergency = false;
int emergency_time;
int32_t mytimer;
int timeout;
typedef struct acc {
  float x;
  float y;
  float z;
};
acc acc_value[20];
short index_acc;

void acc_update() {
  M5.IMU.Update();
  acc_value[index_acc].x = M5.IMU.accel_data.x;
  acc_value[index_acc].y = M5.IMU.accel_data.y;
  acc_value[index_acc].z = M5.IMU.accel_data.z;
  index_acc++;
  if (index_acc > 19) { index_acc = 0; };
}
bool acc_check() {
  float x = 0;
  float y = 0;
  float z = 0;
  for (int i = 0; i < 20; i++) {
    x += acc_value[i].x;
    y += acc_value[i].y;
    z += acc_value[i].z;
  }
  /* USBSerial.print("X: ");
  USBSerial.print(x);
  USBSerial.print(" Y: ");
  USBSerial.print(y);
  USBSerial.print(" Z: ");
  USBSerial.println(z);*/
  // ((M5.IMU.accel_data.x > 1) || (M5.IMU.accel_data.x < -1) || (M5.IMU.accel_data.y < -1) || (M5.IMU.accel_data.z < 1))
  if ((x > 20) || (x < -20) || (y < -30) || (z < -30)) {
    return true;
  } else {
    return false;
  }
}

void setup() {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(EMERGENCY_BUTTON, INPUT_PULLUP);
  pinMode(SIG_IN, INPUT);
  pinMode(SIG_OUT, OUTPUT);
  digitalWrite(SIG_OUT, LOW);
  receiving = false;

  M5.begin(true, true, true);
  M5.Axp.powerModeSet(POWER_MODE_USB_IN_BUS_OUT);
  display.begin();
  display.powerSaveOff();
  display.setBrightness(128);
  display.fillRect(0, 0, 320, 240, 0xF800);
  display.setTextColor(0xFFFF);
  display.setTextSize(3);
  display.setCursor(70, 100);
  display.print("Connecting");
  esp_wifi_set_ps(WIFI_PS_NONE);
  WiFi.begin(SSID_NAME, SSID_PASSWORD);
  /* http.setReuse(true);
  http.setConnectTimeout(500);
  http.setTimeout(1200);
 */
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    USBSerial.print(F("."));
  }
  ip = WiFi.localIP();
  USBSerial.print("Ip: ");
  USBSerial.println(ip);
  display.fillRect(0, 0, 320, 240, 0xF800);
  display.setCursor(70, 100);
  display.print(ip);
  delay(1000);
  M5.IMU.Init();
  display.clearDisplay();
  display.powerSaveOn();
  display.setBrightness(0);
}

void loop() {
  acc_update();
  delay(10);
  if (digitalRead(BUTTON1) == 0) {
    http.begin(URL_AUDIO);
    int httpCode = http.GET();
    USBSerial.println("AUDIO");
  }
  if (digitalRead(BUTTON2) == 0) {
    for (int x = 0; x < 3; x++) {
      http.begin(URL_GATE);
      int httpCode = http.GET();
      USBSerial.println("Opening the gate");
      if (httpCode == 200) { break; }
      display.fillRect(0, 200, 320, 40, 0xF800);
      display.setTextColor(0xFFFF);
      display.setTextSize(3);
      display.setCursor(20, 210);
      display.print("Door opened");
      delay(50);
    }
  }
  if (digitalRead(EMERGENCY_BUTTON) == 0) {
    for (int x = 0; x < 10; x++) {
      http.begin(URL_EMERGENCY);
      int httpCode = http.GET();
      USBSerial.println("Calling for emergency");
      if (httpCode == 200) { break; }
      delay(50);
    }
    display.powerSaveOff();
    display.setBrightness(128);
    display.fillRect(0, 0, 320, 240, 0xF800);
    display.setTextColor(0xFFFF);
    display.setTextSize(3);
    display.setCursor(70, 100);
    display.print("Emergency");
    display.setCursor(70, 150);
    display.print("requested");
  }

  if ((digitalRead(SIG_IN) == 1) && (receiving == false)) {
    USBSerial.println("Entered");
    receiving = true;
    timeout = VIDEO_TIMEOUT;
    display.powerSaveOff();
    display.setBrightness(128);
    USBSerial.println("enabling display");
    USBSerial.print("SIG_IN: ");
    USBSerial.println(receiving);
  }

  if (digitalRead(SIG_IN) == 1) {
    mytimer = millis();  // keep receiving


  } else {
    if (receiving) {
      if ((millis() - mytimer) > timeout) {
        receiving = false;
        // on_receiving = false;
        display.clearDisplay();
        display.powerSaveOn();
        display.setBrightness(0);
        USBSerial.println("No more packets");
        for (int x = 0; x < 3; x++) {
          http.begin(URL_LEDOFF);
          int httpCode = http.GET();

          USBSerial.print("Led off = ");
          USBSerial.println(httpCode);
          if (httpCode == 200) { break; }
          delay(50);
        }

        USBSerial.print("timeout = ");
        USBSerial.println(timeout);
      }
    }
  }

  int nums = display.getTouchRaw(tp, 3);
  if (nums) {
    if (receiving) {
      http.begin(URL_LEDON);
      int httpCode = http.GET();
    } else {
      receiving = true;
      mytimer = millis();  // longer timeout
      timeout = TOUCH_TIMEOUT;
      display.powerSaveOff();
      display.setBrightness(128);
      USBSerial.println("enabling display by touch");
      USBSerial.print("timeout = ");
      USBSerial.println(timeout);
    }
  }

  if (receiving) {
    if (WiFi.status() != WL_CONNECTED) {
      USBSerial.println("Not connected");
    } else {
      display.drawJpgUrl(URL_JPG, 0, 0);
      // USBSerial.println("receiving");
      //delay(250);
    }
  }
  if (acc_check()) {
    display.powerSaveOff();
    display.setBrightness(128);
    display.fillRect(0, 0, 320, 240, 0xF800);
    display.setTextColor(0xFFFF);
    display.setTextSize(3);
    display.setCursor(70, 100);
    display.print("Emergency");
    display.setCursor(70, 150);
    display.print("requested");
    if (!emergency) {
      USBSerial.println("Calling for help");
      for (int x = 0; x < 10; x++) {

        http.begin(URL_SIDEWAY);
        int httpCode = http.GET();
        if (httpCode == 200) { break; }
        emergency = true;
        emergency_time = millis();
      }
    }
  }
  if ((emergency) && ((millis() - mytimer) > EMERGENCY_TIMEOUT)) {
    emergency = false;
    display.clearDisplay();
    display.powerSaveOn();
    display.setBrightness(0);
  }
}
