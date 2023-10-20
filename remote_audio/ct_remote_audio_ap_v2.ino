/**
 * 
 * Inspired by 
 * ESP32 I2S UDP Streamer by GrahamM
 */


#include <driver/i2s.h>
#include <soc/i2s_reg.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include "AsyncUDP.h"
#include "esp_system.h"
#include "M5Atom.h"
#include "config.h"
#include "audio_sample.h"

#define MODE_MIC 0
#define MODE_SPK 1
#define DATA_SIZE 1024

#define BUTTON_PIN_BITMASK 0x2000000  // 2^25 in hex
// Connection state
boolean connected = false;

//The udp library class
AsyncUDP udp;
AsyncUDP udpL;
HTTPClient http;

bool rx = false;
bool onrx = false;
bool to_sleep = false;
bool to_rx = false;
bool to_tx = false;
hw_timer_t* timer0 = NULL;
//hw_timer_t* timer1 = NULL;
int32_t mytimer;
int32_t buffer[1024];
volatile uint16_t rPtr = 0;
unsigned int bytes_read = 0;
unsigned char sounddata_data2[50000];

void deepsleep() {
  M5.dis.drawpix(0, 0x000000);
  delay(150);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);  //1 = High, 0 = Low
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}

// after wd triggered, go to sleep mode
void ARDUINO_ISR_ATTR sleepWD() {
  to_sleep = true;
}


void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidName, ssidPswd);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed");
    M5.dis.drawpix(0, 0xff0000);
    to_sleep = true;
  }
  WiFi.setSleep(false);
}
void InitI2SSpeakerOrMic(int mode) {
  esp_err_t err = ESP_OK;
  i2s_driver_uninstall(SPEAKER_I2S_NUMBER);
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,  // is fixed at 12bit, stereo, MSB
    .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = BLOCK_SIZE,
  };
  if (mode == MODE_MIC) {
    // i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM); ///echo
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);  //  inmp441
  } else {
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    i2s_config.use_apll = false;
    i2s_config.tx_desc_auto_clear = true;
  }

  err += i2s_driver_install(SPEAKER_I2S_NUMBER, &i2s_config, 0, NULL);
  i2s_pin_config_t tx_pin_config;

  tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
  tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
  tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
  tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
  tx_pin_config.mck_io_num = GPIO_NUM_0;

  //Serial.println("Init i2s_set_pin");
  err += i2s_set_pin(SPEAKER_I2S_NUMBER, &tx_pin_config);
  //Serial.println("Init i2s_set_clk");
  err += i2s_set_clk(SPEAKER_I2S_NUMBER, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}


// Receive messages from remote
void set_rx() {
  M5.dis.drawpix(0, 0x0000ff);
  Serial.println("entering on rx mode");
  InitI2SSpeakerOrMic(MODE_SPK);
  onrx = true;
  return;
}


void set_tx() {
  InitI2SSpeakerOrMic(MODE_MIC);
  M5.dis.drawpix(0, 0x00ff00);
  onrx = false;
}

void setup_listner() {
  if (udpL.listen(udpPort)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());

    udpL.onPacket([](AsyncUDPPacket packet) {
      Serial.printf("Received packet Length: %d\n", packet.length());
      rx = true;
      if (!onrx) {
        to_rx = true;
      }
      mytimer = millis();
      timerWrite(timer0, 0);  //reset timer (feed watchdog)
      i2s_write(SPEAKER_I2S_NUMBER,
                packet.data(),
                packet.length(),  // Number of bytes
                &bytes_read,
                portMAX_DELAY);  // No timeout
      return;
    });
  }
}
void setup() {
  M5.begin(true, false, true);

  to_sleep = false;
  M5.dis.drawpix(0, 0x00ff00);
  Serial.begin(115200);
  pinMode(SIG_IN, INPUT);
  // deep sleep after watchdog
  timer0 = timerBegin(0, 80, true);                    //timer 0, div 80
  timerAttachInterrupt(timer0, &sleepWD, true);        //attach callback
  timerAlarmWrite(timer0, wdtTimeout0 * 1000, false);  //set time in us
  timerAlarmEnable(timer0);                            //enable interrupt



  Serial.println(" * Configuring WiFi");
  setupWiFi();
  Serial.println(" * Configuring Listner");
  setup_listner();
  Serial.println(" * Configuring I2S");

  InitI2SSpeakerOrMic(MODE_MIC);
  mytimer = millis();
  if (!connected) {
    if (udp.connect(udpAddress, udpPort)) {
      connected = true;
      if (connected) {
        Serial.println(" * Connected to host");
      } else {
        Serial.println(" Not connected to host");
      }
    }
  }
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    notify();
  }
  play_ring();
}

void play_ring() {
  int i = 0;
  for (i = 44; i < 148524; i += 1024) {
    udp.write(sounddata_data + i, 1024);
    delay(9);
  }
}

void notify() {
  for (int x = 0; x < 10; x++) {
    http.begin(URL_NOTIFY);
    int httpCode = http.GET();
    if (httpCode == 200) { break; }
    delay(50);
  }
}

void loop() {
  static uint8_t state = 0;
  if (onrx) {
    if ((millis() - mytimer) > 500) {
      set_tx();
    }
  }
  if (to_rx) {
    set_rx();
    to_rx = false;
  }
  if (to_sleep) {
    deepsleep();
  }

  if (!onrx) {
    mic_record();
  }
  M5.update();
  if (M5.Btn.isPressed()) {
    notify();
    play_ring();
    timerWrite(timer0, 0);  //reset timer (feed watchdog)
  }

  if (!connected) {
    if (udp.connect(udpAddress, udpPort)) {
      connected = true;
      if (connected) {
        Serial.println(" * Connected to host");
      } else {
        Serial.println(" Not connected to host");
      }
    }
  } else {
    switch (state) {
      case 0:  // wait for index to pass halfway
        if (rPtr > 1023) {
          state = 1;
        }
        break;
      case 1:  // send the first half of the buffer
        state = 2;
        udp.write((uint8_t*)buffer, 1024);
        break;
      case 2:  // wait for index to wrap
        if (rPtr < 1023) {
          state = 3;
        }
        break;
      case 3:  // send second half of the buffer
        udp.write((uint8_t*)buffer + 1024, 1024);
        break;
    }
  }
}