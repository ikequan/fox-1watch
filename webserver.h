#define DNS_INTERVAL 30 // Define the DNS interval in milliseconds between processing DNS requests

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30 * 1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;


const String localIPURL = "http://4.3.2.1";   // a string version of the local IP with http, used for redirecting clients to your webpage


String getWifiList() {
  String wifi_list = "";
  int n = WiFi.scanComplete();
  for (int i = 0; i < n; ++i)
  {
    wifi_list += WiFi.SSID(i) + ",";
  }

  return wifi_list;
}



DNSServer dnsServer;
AsyncWebServer server(80);


bool connectTo01OS(String server_address)
{
  int err = 0;
  int port = 80;

  String domain;
  String portStr;

  // Remove http and https, as it causes errors in HttpClient, the library relies on adding the host header itself
  if (server_address.startsWith("http://")) {
    server_address.remove(0, 7);

  } else if (server_address.startsWith("https://")) {
    server_address.remove(0, 8);

  }

  // Remove trailing slash, causes issues
  if (server_address.endsWith("/")) {
    server_address.remove(server_address.length() - 1);
  }

  int colonIndex = server_address.indexOf(':');
  if (colonIndex != -1) {
    domain = server_address.substring(0, colonIndex);
    portStr = server_address.substring(colonIndex + 1);
  } else {
    domain = server_address;
    portStr = "";
  }

  WiFiClient c;


  //If there is a port, set it
  if (portStr.length() > 0) {
    port = portStr.toInt();
  }

  HttpClient http(c, domain.c_str(), port);
  Serial.println("Connecting to 01OS at " + domain + ":" + port + "/ping");

  if (domain.indexOf("ngrok") != -1) {
    http.sendHeader("ngrok-skip-browser-warning", "80");
  }

  err = http.get("/ping");
  bool connectionSuccess = false;

  if (err == 0)
  {
    Serial.println("Started the ping request");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);

      if (err == 200)
      {
        server_domain = domain;
        server_port = port;
        connectionSuccess = true;
        preferences.begin("network", false); // Use a different namespace for network settings
        preferences.putString("server_url", server_address); // Store the server URL
        preferences.end(); // Close the Preferences
      }

      err = http.skipResponseHeaders();
      if (err >= 0)
      {
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body:");

        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ((http.connected() || http.available()) &&
               ((millis() - timeoutStart) < kNetworkTimeout))
        {
          if (http.available())
          {
            c = http.read();
            // Print out this character
            Serial.print(c);
            Serial.print("");

            bodyLen--;
            // We read something, reset the timeout counter
            timeoutStart = millis();
          }
          else
          {
            // We haven't got any data, so let's pause to allow some to
            // arrive
            delay(kNetworkDelay);
          }
        }
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
    else
    {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connection failed: ");
    Serial.println(err);
  }
  return connectionSuccess;
}


void tryReconnectToServer() {
  preferences.begin("network", true); // Open Preferences with the "network" namespace in ReadOnly mode
  String serverURL = preferences.getString("server_url", ""); // Get stored server URL, if any
  preferences.end(); // Close the Preferences

  if (!serverURL.isEmpty()) {
    Serial.println("Trying to reconnect to server with stored URL: " + serverURL);
    // Attempt to connect to the server using the stored URL
    if (connectTo01OS(serverURL)) {
      Serial.println("Reconnected to server using stored URL.");
      SERVER_CONNECTED = true;
    } else {
      Serial.println("Failed to reconnect to server. Proceeding with normal startup.");
      // Proceed with your normal startup routine, possibly involving user input to get a new URL
      SERVER_CONNECTED = false;
    }
  } else {
    Serial.println("No stored server URL. Proceeding with normal startup.");
   SERVER_CONNECTED = false;
  }
}

void tryReconnectWiFi() {
  Serial.println("Checking for stored WiFi credentials...");
  preferences.begin("wifi", true); // Open Preferences with my-app namespace in ReadOnly mode
  String ssid = preferences.getString("ssid", ""); // Get stored SSID, if any
  String password = preferences.getString("password", ""); // Get stored password, if any
  preferences.end(); // Close the Preferences

  if (ssid != "") { // Check if we have stored credentials
    Serial.println("Trying to connect to WiFi with stored credentials.");
    adc_power_on();
    WiFi.setSleep(false);
    WiFi.disconnect(false);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to Wi-Fi using stored credentials.");
      
      if (!SERVER_CONNECTED){
          tryReconnectToServer();
        }
      return;
    } else {
      Serial.println("Failed to connect to Wi-Fi. Starting captive portal.");
    }
  } else {
    Serial.println("No stored WiFi credentials. Starting captive portal.");
  }
}


void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP)
{
  // Set the TTL for DNS response and start the DNS server
  dnsServer.setTTL(3600);
  dnsServer.start(53, "*", localIP);
}







void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP)
{
  //======================== Webserver ========================
  // WARNING IOS (and maybe macos) WILL NOT POP UP IF IT CONTAINS THE WORD "Success" https://www.esp8266.com/viewtopic.php?f=34&t=4398
  // SAFARI (IOS) IS STUPID, G-ZIPPED FILES CAN'T END IN .GZ https://github.com/homieiot/homie-esp8266/issues/476 this is fixed by the webserver serve static function.
  // SAFARI (IOS) there is a 128KB limit to the size of the HTML. The HTML can reference external resources/images that bring the total over 128KB
  // SAFARI (IOS) popup browser has some severe limitations (javascript disabled, cookies disabled)

  // Required
  server.on("/connecttest.txt", [](AsyncWebServerRequest * request)
  {
    request->redirect("http://logout.net");
  }); // windows 11 captive portal workaround
  server.on("/wpad.dat", [](AsyncWebServerRequest * request)
  {
    request->send(404);
  }); // Honestly don't understand what this is but a 404 stops win 10 keep calling this repeatedly and panicking the esp32 :)

  // Background responses: Probably not all are Required, but some are. Others might speed things up?
  // A Tier (commonly used by modern systems)
  server.on("/generate_204", [](AsyncWebServerRequest * request)
  {
    request->redirect(localIPURL);
  }); // android captive portal redirect
  server.on("/redirect", [](AsyncWebServerRequest * request)
  {
    request->redirect(localIPURL);
  }); // microsoft redirect
  server.on("/hotspot-detect.html", [](AsyncWebServerRequest * request)
  {
    request->redirect(localIPURL);
  }); // apple call home
  server.on("/canonical.html", [](AsyncWebServerRequest * request)
  {
    request->redirect(localIPURL);
  }); // firefox captive portal call home
  server.on("/success.txt", [](AsyncWebServerRequest * request)
  {
    request->send(200);
  }); // firefox captive portal call home
  server.on("/ncsi.txt", [](AsyncWebServerRequest * request)
  {
    request->redirect(localIPURL);
  }); // windows call home

  // B Tier (uncommon)
  //  server.on("/chrome-variations/seed",[](AsyncWebServerRequest *request){request->send(200);}); //chrome captive portal call home
  //  server.on("/service/update2/json",[](AsyncWebServerRequest *request){request->send(200);}); //firefox?
  //  server.on("/chat",[](AsyncWebServerRequest *request){request->send(404);}); //No stop asking Whatsapp, there is no internet connection
  //  server.on("/startpage",[](AsyncWebServerRequest *request){request->redirect(localIPURL);});

  // return 404 to webpage icon
  server.on("/favicon.ico", [](AsyncWebServerRequest * request)
  {
    request->send(404);
  }); // webpage icon

  // Serve Basic HTML Page
  //    server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request)
  //              {
  //  String htmlContent = "";
  //    Serial.printf("Wifi scan complete: %d . WIFI_SCAN_RUNNING: %d", WiFi.scanComplete(), WIFI_SCAN_RUNNING);
  //    if(WiFi.scanComplete() > 0) {
  //      // Scan complete, process results
  //      Serial.println("Done scanning wifi");
  //      htmlContent = generateHTMLWithSSIDs();
  //      // WiFi.scanNetworks(true); // Start a new scan in async mode
  //    }
  //    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", htmlContent);
  //    response->addHeader("Cache-Control", "public,max-age=31536000");  // save this file to cache for 1 year (unless you refresh)
  //    request->send(response);
  //    Serial.println("Served HTML Page");
  //    });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html", "text/html");
    fs::File file = SPIFFS.open("/index.html");
    response->addHeader("Content-Length", (String) file.size ());
    //    response->addHeader("Cache-Control", "public,max-age=31536000");
    request->send(response);
    Serial.println("Served HTML Page");
    file.close();
  });

  server.on("/get-wifi-list", HTTP_GET, [](AsyncWebServerRequest * request) {
    String wifi_list = "";
    if (WiFi.status() == WL_CONNECTED) {
      request->send(200, F("text/plain"),  "wifi_connected");
    } else {
      if (WiFi.scanComplete() > 0) {
        // Scan complete, process results
        Serial.println("Done scanning wifi");
        wifi_list = getWifiList();
        WiFi.scanNetworks(true); // Start a new scan in async mode
      }
      Serial.println("Done scanning wifi");
      request->send(200, F("text/plain"),  wifi_list);
    }



  });


  server.on("/is_active", HTTP_GET, [](AsyncWebServerRequest * request) {
    lv_disp_trig_activity(NULL);
    request->send(200, F("text/plain"),  "sleep restted");
  });



  // the catch all
  server.onNotFound([](AsyncWebServerRequest * request)
  {
    request->redirect(localIPURL);
    Serial.print("onnotfound ");
    Serial.print(request->host());  // This gives some insight into whatever was being requested on the serial monitor
    Serial.print(" ");
    Serial.print(request->url());
    Serial.print(" sent redirect to " + localIPURL + "\n");
  });

  server.on("/wifi-connect", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    String ssid;
    String password;

    // Check if SSID parameter exists and assign it
    if (request->hasParam("ssid", true)) {
      ssid = request->getParam("ssid", true)->value();
    }

    // Check if Password parameter exists and assign it
    if (request->hasParam("password", true)) {
      password = request->getParam("password", true)->value();
    }
    // Serial.println(ssid);
    // Serial.println(password);

    // Attempt to connect to the Wi-Fi network with these credentials
    connectToWifi(ssid, password);

    // Redirect user or send a response back
    if (WiFi.status() == WL_CONNECTED) {
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "success");
      response->addHeader("Cache-Control", "public,max-age=31536000");  // save this file to cache for 1 year (unless you refresh)
      request->send(response);
      Serial.println("Served Post connection HTML Page");
    } else {
      request->send(200, "text/plain",  "failed");
    }
  });

  server.on("/server-connect", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    String server_address;

    // Check if server_address parameter exists and assign it
    if (request->hasParam("server_address", true)) {
      server_address = request->getParam("server_address", true)->value();

    }

    Serial.println(server_address);

    // Attempt to connect to the OS Server
    bool connectedToServer = connectTo01OS(server_address);

    // Redirect user or send a response back


    if (connectedToServer)
    {
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "success");
      response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");  // Prevent caching of this page
      request->send(response);
      Serial.println(" ");
      Serial.println("Connected to 01 websocket!");
      Serial.println(" ");
      Serial.println("Served success HTML Page");
      SERVER_CONNECTED  = true;
    }
    else
    {

      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "failed");
      response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");  // Prevent caching of this page
      request->send(response);
      Serial.println("Served Post connection HTML Page with error message");
    }
  });

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
}
