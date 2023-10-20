
// I2S
#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

#define SPEAKER_I2S_NUMBER I2S_NUM_0
const int BLOCK_SIZE = 128;

// Button
#define BUTTON 39
// INterface with Camera
#define SIG_IN 25
#define SIG_OUT 13

// WiFi network name and password:
const char* ssidName = "OpenWrt";
const char* ssidPswd = "password";

#define URL_NOTIFY "http://192.168.4.108/notify"

// UDP Destination
IPAddress udpAddress(192, 168, 4, 184);
const int udpPort = 3333;


// Watchdog
const int wdtTimeout0 = 20000;  //time in ms to trigger the watchdog
const int wdtTimeout1 = 500;  //time in ms to trigger the watchdog