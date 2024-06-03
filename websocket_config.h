bool recording = false;
WebSocketsClient webSocket;
bool hasSetupWebsocket = false;
int ws_attempts = 0;
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.printf("[WSc] Disconnected! %d\n", ws_attempts);
        if (ws_attempts > 10){
//            WiFi.disconnect(true);
//            WiFi.mode(WIFI_OFF);
//            WiFi.mode(WIFI_MODE_NULL);
            lightSleep = true;
          }

           ws_attempts++;
        
        break;
    case WStype_CONNECTED:
        Serial.printf("[WSc] Connected to url: %s\n", payload);

        // send message to server when Connected
        break;
    case WStype_TEXT:
        Serial.printf("[WSc] get text: %s\n", payload);
        {
            std::string str(payload, payload + length);
            bool isAudio = str.find("\"audio\"") != std::string::npos;
            if (isAudio && str.find("\"start\"") != std::string::npos)
            {
                Serial.println("start playback");
                lv_disp_trig_activity(NULL);
                speaker_offset = 0;
                InitI2SSpeakerOrMic(MODE_SPK);
            }
            else if (isAudio && str.find("\"end\"") != std::string::npos)
            {
                Serial.println("end playback");
                // speaker_play(speakerdata0, speaker_offset);
                // speaker_offset = 0;
            }
        }
        // send message to server
        // webSocket.sendTXT("message here");
        break;
    case WStype_BIN:
//        Serial.printf("[WSc] get binary length: %u\n", length);
        memcpy(speakerdata0 + speaker_offset, payload, length);
        speaker_offset += length;
        size_t bytes_written;
        i2s_write(SPEAKER_I2S_NUMBER, speakerdata0, speaker_offset, &bytes_written, portMAX_DELAY);
        speaker_offset = 0;

        // send data to server
        // webSocket.sendBIN(payload, length);
        break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
        break;
    }
}

void websocket_setup(String server_domain, int port)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Not connected to WiFi. Abandoning setup websocket");
        return;
    }
    Serial.println("connected to WiFi");
    webSocket.begin(server_domain, port, "/");
    webSocket.onEvent(webSocketEvent);
    // webSocket.setAuthorization("user", "Password");
    webSocket.setReconnectInterval(5000);
}
