/*
Download a jpe from the camera and send to Telegram as binary
*/
void sendImg(String chat_id) {
int httpCode;
  for (int x = 0; x < 10; x++) {
    http.begin(URL_JPG);
    httpCode = http.GET();
    if (httpCode == 200) { break; }
  }
  if (httpCode <= 0) {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  } else {
    if (httpCode != HTTP_CODE_OK) {
      Serial.printf("[HTTP] Not OK!\n");
    } else {
      dataAvailable = true;
      len = http.getSize();
      if (len <= 0) {
        Serial.printf("[HTTP] Unknow content size: %d\n", len);
      } else {
        WiFiClient* stream = http.getStreamPtr();
        uint8_t* p = buff;
        int l = len;
        while (http.connected() && (l > 0 || len == -1)) {
          // get available data size
          size_t size = stream->available();

          if (size) {
            int s = ((size > sizeof(buff)) ? sizeof(buff) : size);
            int c = stream->readBytes(p, s);
            p += c;
            if (l > 0) {
              l -= c;
            }
          }
        }
      }
    }
    Serial.print("Chat ID:");
    Serial.println(chat_id);
    currentByte = 0;

    bot.sendPhotoByBinary(chat_id, "image/jpeg", len,
                          isMoreDataAvailable, getNextByte,
                          nullptr, nullptr);
  }
}
bool isMoreDataAvailable() {
  return (len - currentByte);
}

uint8_t getNextByte() {
  currentByte++;
  return (buff[currentByte - 1]);
}


byte* getNextBuffer() {
  return buff;
}

int getNextBufferLen() {
  return len - 6;
}

void handleemergency() {
  M5.dis.drawpix(0, 0xff0000);
  String message = "\xF0\x9F\x9A\xA8 <b>Help Requested</b>\xF0\x9F\x9A\xA8.\n";
  message += "Please call back .\n";
  bot.sendMessage(CHATID, message, "HTML");
  message = "Triggering rele 2!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);
  M5.dis.drawpix(0, 0x000000);
}

void handleauthemergency() {
  M5.dis.drawpix(0, 0xff0000);
  String message = "\xF0\x9F\x9A\xA8 <b>Emergency Request</b>\xF0\x9F\x9A\xA8.\n";
  message += "Automatic request from device.\n";
  message += "Please check if everything is ok.\n";
  bot.sendMessage(CHATID, message, "HTML");
  message = "Triggering rele 2!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);
  M5.dis.drawpix(0, 0x000000);
}

void handleNewMessages(int numNewMessages) {
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";

    if (text == "/open_gate") {
      digitalWrite(RELE1, HIGH);
      delay(500);
      digitalWrite(RELE1, LOW);
      bot.sendMessage(chat_id, "Door opened", "");
    }
    if (text == "/light_on") {
      digitalWrite(RELE2, HIGH);

      bot.sendMessage(chat_id, "Light On", "");
    }
    if (text == "/light_off") {
      digitalWrite(RELE2, LOW);
      bot.sendMessage(chat_id, "Light Off", "");
    }
    if (text == "/get_photo") {
      Serial.print("Sending photo from Chat ID:");
      Serial.println(chat_id);
      sendImg(chat_id);
    }

    if (text == "/start") {
      String welcome = "Welcome Remote Bot, " + from_name + ".\n";
      welcome += "These commands are available.\n\n";
      welcome += "/get_photo : getting a photo from the camera\n";
      welcome += "/open_gate :Open the main gate\n";
      welcome += "/light_on : Turn on external light\n";
      welcome += "/light_off : Turn off external light\n";


      bot.sendMessage(chat_id, welcome, "");
    }
  }
}