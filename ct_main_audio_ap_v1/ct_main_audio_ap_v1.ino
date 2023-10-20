
#include "WiFi.h"
#include "AsyncUDP.h"
#include <driver/i2s.h>
#include <soc/i2s_reg.h>
#include "config.h"
#include "M5Atom.h"



bool onrx = false;
bool ontx = false;
boolean connected = false;
AsyncUDP udp;
AsyncUDP udp2;

#define MODE_MIC 0
#define MODE_SPK 1
#define DATA_SIZE 1024


int32_t mytimer;
bool receiving;
bool onreceiving;
int32_t buffer[1024];        // Effectively two 1024 byte buffers
volatile uint16_t rPtr = 0;  // Yes, I should be using a pointer.
unsigned int bytes_read = 0;

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
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);  ///echo
                                                                                   // i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);  //  inmp441
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


void setup_listner() {
  if (udp.listen(udpPort)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());

    udp.onPacket([](AsyncUDPPacket packet) {
      // Serial.printf("packet Length: %d\n", packet.length());
      if (onrx) {
        Serial.printf("packet Length: %d\n", packet.length());
        mytimer = millis();
        receiving = true;
        i2s_write(SPEAKER_I2S_NUMBER,
                  packet.data(),
                  packet.length(),  // Number of bytes
                  &bytes_read,
                  portMAX_DELAY);  // No timeout
      } else {
        Serial.println("Not on RX");
      }
      return;
    });
  }
}

void setup() {
  M5.begin(true, false, true);
  M5.dis.drawpix(0, 0x000000);
  // M5.Power.begin();
  Serial.begin(115200);
  pinMode(SIG_IN, INPUT);
  pinMode(SIG_OUT, OUTPUT);
  digitalWrite(SIG_OUT, LOW);
  onrx = true;
  receiving = false;
  onreceiving = false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_NAME, SSID_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    M5.dis.drawpix(0, 0xff0000);
    delay(500);
    Serial.print(F("."));
  }
  WiFi.setSleep(false);
  M5.dis.drawpix(0, 0x000000);
  InitI2SSpeakerOrMic(MODE_SPK);
  setup_listner();
  
}


void loop() {
  static uint8_t state = 0;
  if (receiving) {
    if (!onreceiving) { 
    digitalWrite(SIG_OUT, HIGH);
    M5.dis.drawpix(0, 0x00ff00);
    onreceiving = true;
    }
    if ((millis() - mytimer) > 3000) {
      receiving = false;
      Serial.println("No more packets");
      M5.dis.drawpix(0, 0x000000);
      onreceiving = false;
    }

  } else {
    digitalWrite(SIG_OUT, LOW);
  }

  M5.update();
 
  if (M5.Btn.isPressed()) {
    Serial.println("Pressed");
    if (!ontx) {
      udp.close();
      InitI2SSpeakerOrMic(MODE_MIC);
      ontx = true;
      onrx = false;
      M5.dis.drawpix(0, 0x0000ff);
    }
    mic_record();
    if (!connected) {
      if (udp2.connect(udpAddress, udpPort)) {
        connected = true;
        Serial.println(" * Connected to host");
      }
    }
    switch (state) {
      case 0:  // wait for index to pass halfway
        if (rPtr > 1023) {
          state = 1;
        }
        break;
      case 1:  // send the first half of the buffer
        Serial.println("Sending data: state 1");
        state = 2;
        udp2.write((uint8_t*)buffer, 1024);
        break;
      case 2:  // wait for index to wrap
        if (rPtr < 1023) {
          state = 3;
        }
        break;
      case 3:  // send second half of the buffer
        Serial.println("Sending data: state 3");
        state = 0;
        udp2.write((uint8_t*)buffer + 1024, 1024);
        break;
    }
  } else {
    ontx = false;
    //Serial.println("Not in tx");
    if (!onrx) {
      Serial.println(WiFi.status());
      Serial.println("Initialising speaker");
      InitI2SSpeakerOrMic(MODE_SPK);
      Serial.println("Init done");

      setup_listner();
      onrx = true;
      ontx = false;
      M5.dis.drawpix(0, 0x00ff00);
    }
    unsigned long currentMillis = millis();
  }
}
