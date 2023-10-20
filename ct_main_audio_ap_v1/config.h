// Button
#define BUTTON 39
#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

#define SPEAKER_I2S_NUMBER I2S_NUM_0

const int BLOCK_SIZE = 128;

#define SIG_IN 26
#define SIG_OUT 32

#define SSID_NAME "OpenWrt"
#define SSID_PASSWORD "password"

// UDP Destination
IPAddress udpAddress(192, 168, 4, 205);
const int udpPort = 3333;