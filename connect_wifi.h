const char *ssid = "FOX-1Watch"; // FYI The SSID can't have a space in it.
// const char * password = "12345678"; //Atleast 8 chars
const char *password = NULL; // no password

#define MAX_CLIENTS 4  // ESP32 supports up to 10 but I have not tested it yet
#define WIFI_CHANNEL 6 // 2.4ghz channel 6 https://en.wikipedia.org/wiki/List_of_WLAN_channels#2.4_GHz_(802.11b/g/n/ax)

const IPAddress localIP(4, 3, 2, 1);          // the IP address the web server, Samsung requires the IP to be in public space
const IPAddress gatewayIP(4, 3, 2, 1);        // IP address of the network should be the same as the local IP for captive portals
const IPAddress subnetMask(255, 255, 255, 0); // no need to change: https://avinetworks.com/glossary/subnet-mask/
void setModemSleep();
void wakeModemSleep();

void connectToWifi(String ssid, String password)
{
    if (WiFi.status() == WL_CONNECTED){
       return;
      }
    adc_power_on();
    WiFi.disconnect(false); 
    WiFi.setSleep(false);
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection to establish
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(1000);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Connected to Wi-Fi");
        // Store credentials on successful connection
        preferences.begin("wifi", false); // Open Preferences with my-app namespace. RW-mode is false by default.
        preferences.putString("ssid", ssid); // Put your SSID.
        preferences.putString("password", password); // Put your PASSWORD.
        preferences.end(); // Close the Preferences.
    }
    else
    {
        Serial.println("Failed to connect to Wi-Fi. Check credentials.");
    }
}

void startSoftAccessPoint(const char *ssid, const char *password, const IPAddress &localIP, const IPAddress &gatewayIP)
{
    // Set the WiFi mode to access point and station
    Serial.println("Setting up Access Point");
    WiFi.setSleep(false);
    WiFi.mode(WIFI_MODE_AP);

    // Define the subnet mask for the WiFi network
    Serial.println("setting up subnet");
    const IPAddress subnetMask(255, 255, 255, 0);

    // Configure the soft access point with a specific IP and subnet mask
    Serial.println("setting up specific IP");
    WiFi.softAPConfig(localIP, gatewayIP, subnetMask);

    // Start the soft access point with the given ssid, password, channel, max number of clients
    Serial.println("setting up ssid and password");
    WiFi.softAP(ssid, password, WIFI_CHANNEL, 0, MAX_CLIENTS);

    // Disable AMPDU RX on the ESP32 WiFi to fix a bug on Android
    Serial.println("Disable AMPDU RX on the ESP32 WiFi to fix a bug on Android");
//    esp_wifi_stop();
//    esp_wifi_deinit();
//    wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
//    my_config.ampdu_rx_enable = false;
//    esp_wifi_init(&my_config);
//    esp_wifi_start();
//    vTaskDelay(100 / portTICK_PERIOD_MS); // Add a small delay
}

void disableWiFi(){
    adc_power_off();
    WiFi.disconnect(true);  // Disconnect from the network
    WiFi.mode(WIFI_OFF);    // Switch WiFi off
    WiFi.setSleep(true);
    Serial2.println("");
    Serial2.println("WiFi disconnected!");
}
void disableBluetooth(){
    // Quite unusefully, no relevable power consumption
    btStop();
    Serial2.println("");
    Serial2.println("Bluetooth stop!");
}
 
void setModemSleep() {
    disableWiFi();
    disableBluetooth();;
}
 
//void enableWiFi(){
//    adc_power_on();
//    delay(200);
// 
//    WiFi.disconnect(false);  // Reconnect the network
//    WiFi.mode(WIFI_STA);    // Switch WiFi off
// 
//    delay(200);
// 
//    Serial2.println("START WIFI");
//    WiFi.begin(STA_SSID, STA_PASS);
// 
//    while (WiFi.status() != WL_CONNECTED) {
//        delay(500);
//        Serial2.print(".");
//    }
// 
//    Serial2.println("");
//    Serial2.println("WiFi connected");
//    Serial2.println("IP address: ");
//    Serial2.println(WiFi.localIP());
//}
// 
//void wakeModemSleep() {
//    setCpuFrequencyMhz(240);
//    enableWiFi();
//}
