

#define SPEAKER_I2S_NUMBER I2S_NUM_0
const int BLOCK_SIZE = 128;

// Button
#define BUTTON 39
// INterface with Camera
#define RELE1 22
#define RELE2 19

// WiFi network name and password:
const char* ssidName = "OpenWrt";
const char* ssidPswd = "password";


#define URL_JPG "http://192.168.4.196/jpg"
#define URL_LEDON "http://192.168.4.196/ledon"
#define URL_LEDOFF "http://192.168.4.196/ledoff"
#define URL_AUDIO "http://192.168.4.196/audio"

// Telegram
//Please create a boot telegram and a group chat, inviting the boot
// example https://randomnerdtutorials.com/telegram-control-esp32-esp8266-nodemcu-outputs/

const unsigned long BOT_MTBS = 1000;  // mean time between scan messages
#define BOT_TOKEN "my-boot-token"
#define CHATID "my-numeric-chatid" // Get from telegram
// Watchdog
const int wdtTimeout0 = 20000;  //time in ms to trigger the watchdog
const int wdtTimeout1 = 500;  //time in ms to trigger the watchdog