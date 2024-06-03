static bool lightSleep = false;

String server_domain = "";
int server_port = 10001;
bool SERVER_CONNECTED = false;

#include "driver/adc.h"

#include <driver/i2s.h>
#include <LilyGoLib.h>
#include <LV_Helper.h>
#include <lvgl.h>

#include <driver/i2s.h>
#include <AsyncTCP.h> //https://github.com/me-no-dev/AsyncTCP using the latest dev version from @me-no-dev
#include <DNSServer.h>
#include <ESPAsyncWebServer.h> //https://github.com/me-no-dev/ESPAsyncWebServer using the latest dev version from @me-no-dev
#include <esp_wifi.h>          //Used for mpdu_rx_disable android workaround
#include <ArduinoHttpClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <SPIFFS.h>
#include <driver/gpio.h>
#include <Preferences.h>
Preferences preferences;

//custom includes
#include "connect_wifi.h"
#include "mic_speaker.h"
#include "websocket_config.h"
#include "utils.h"
#include "webserver.h"



#define LVGL_MESSAGE_PROGRESS_CHANGED_ID        (88)

#define RADIO_TRANSMIT_PAGE_ID                  9
#define WIFI_SCAN_PAGE_ID                       8
#define MIC_IR_PAGE_ID                          11

#define AI_PAGE_ID 1
#define HOME_PAGE_ID 0



#define DEFAULT_SCREEN_TIMEOUT_DIM                  15*1000
//#define DEFAULT_SCREEN_TIMEOUT_SCREEN_OFF           30*1000
#define DEFAULT_SCREEN_TIMEOUT_SLEEP                120*1000

#define DEFAULT_COLOR                           (lv_color_make(255, 255, 255))


static int DEFAULT_SCREEN_TIMEOUT_SCREEN_OFF;



LV_FONT_DECLARE(font_firacode_60);
LV_FONT_DECLARE(microphone);

LV_IMG_DECLARE(fox);



#define MIC_SYMBOL "\xEF\x84\xB0"


static lv_obj_t *battery_percent;
static lv_obj_t *weather_celsius;
static lv_obj_t *step_counter;

static lv_obj_t *tileview;
static lv_obj_t *label_datetime;
static lv_obj_t *charge_cont;

static lv_timer_t *transmitTask;
static lv_timer_t *clockTimer;

static lv_obj_t *clock_label;
static lv_obj_t * battery_icon;

static TaskHandle_t audioRecordingTaskHandler;

static lv_style_t button_default_style;
static lv_style_t button_press_style;
// Save the ID of the current page
static uint8_t pageId = 0;
// Flag used to indicate whether to use light sleep, currently unavailable

static bool sportsIrq = false;
// Flag used for PMU interrupt trigger status
static bool pmuIrq = false;
// Flag used to select whether to turn off the screen
static bool canScreenOff = true;
static bool can_set_brightness = false;
// Flag used to detect USB insertion status
static bool usbPlugIn = false;
// Save pedometer steps
static uint32_t stepCounter = 0;
// Save Radio Transmit Interval
static uint32_t configTransmitInterval = 0;
// Save brightness value
static RTC_DATA_ATTR int brightnessLevel = 0;




typedef  struct _lv_datetime {
  lv_obj_t *obj;
  const char *name;
  uint16_t minVal;
  uint16_t maxVal;
  uint16_t defaultVal;
  uint8_t digitFormat;
} lv_datetime_t;

static lv_datetime_t lv_datetime [] = {
  {NULL, "Year", 2023, 2099, 2023, 4},
  {NULL, "Mon", 1, 12, 4, 2},
  {NULL, "Day", 1, 30, 12, 2},
  {NULL, "Hour", 0, 24, 22, 2},
  {NULL, "Min", 0, 59, 30, 2},
  {NULL, "Sec", 0, 59, 0, 2}
};


void factory_ui();
void aiView(lv_obj_t *parent);
void homeView(lv_obj_t *parent);



volatile bool talk_btn_pressed = false;
volatile bool talk_btn_released = false;
lv_obj_t * fox_gif;


void datetimeVeiw(lv_obj_t *parent);

void settingPMU();
void settingSensor();
void settingButtonStyle();
void PMUHandler();
void lowPowerEnergyHandler();
void destoryChargeUI();




bool isServerURLStored() {
  preferences.begin("network", true); // Open Preferences with the "network" namespace in ReadOnly mode
  String serverURL = preferences.getString("server_url", ""); // Get stored server URL, if any
  preferences.end(); // Close the Preferences
  return !serverURL.isEmpty();
}

void setup()
{
  // Stop wifi
  //  WiFi.mode(WIFI_MODE_NULL);

  btStop();

  setCpuFrequencyMhz(240);

  Serial.begin(115200);

  //Mount SPIFFS
  SPIFFS.begin();

  watch.begin();

  settingPMU();

  settingSensor();
  //  connectToWifi("Home 4G", "GHbook1991@@");
  WiFi.mode(WIFI_AP_STA);
  startSoftAccessPoint(ssid, password, localIP, gatewayIP);
  setUpDNSServer(dnsServer, localIP);
  WiFi.scanNetworks(true);
  setUpWebserver(server, localIP);

  tryReconnectWiFi();

  // If WiFi reconnect fails, start the soft access point for the captive portal.
  if (WiFi.status() != WL_CONNECTED) {
    startSoftAccessPoint(ssid, password, localIP, gatewayIP);
    setUpDNSServer(dnsServer, localIP);
    WiFi.scanNetworks(true); // Start scanning for networks in preparation for the captive portal.
    setUpWebserver(server, localIP); // Set up the web server for the captive portal.
  }

  server.begin();
  beginLvglHelper(false);

  settingButtonStyle();
  factory_ui();

  //  preferences.begin("sys_config", false);
  //  preferences.clear();
  //  preferences.end();

  //  int brightnessLevel = getPreference("sys_config", "sc_brightness", 53, UCHAR);


  usbPlugIn =  watch.isVbusIn();
  //  listDir(SPIFFS, "/", 0);

  xTaskCreate(audio_recording_task, "AUDIO", 4096, NULL, 4, &audioRecordingTaskHandler);
  //  vTaskSuspend(audioRecordingTaskHandler);
  //  preferences.begin("sys_config", false);
  //  preferences.clear();
  //  preferences.putInt("sc_brightness", 123);
  //  preferences.end();
  //
  //  preferences.begin("sys_config", true);
  //  int brightness = preferences.getInt("sc_brightness", 20);
  //  Serial.print("Retrieved Brightness: ");
  //  Serial.println(brightness);
  //  preferences.end();

}



void destoryAllUI()
{
  if (!tileview) {
    return;
  }
  lv_obj_del(tileview);
  tileview = NULL;
}



void PMUHandler()
{
  if (pmuIrq) {
    pmuIrq = false;
    watch.readPMU();
    if (watch.isVbusInsertIrq()) {
      Serial.println("isVbusInsert");
      usbPlugIn = true;
    }
    if (watch.isVbusRemoveIrq()) {
      Serial.println("isVbusRemove");
      usbPlugIn = false;
    }
    if (watch.isBatChagerDoneIrq()) {
      Serial.println("isBatChagerDone");

    }
    if (watch.isBatChagerStartIrq()) {
      Serial.println("isBatChagerStart");
    }
    // Clear watch Interrupt Status Register
    watch.clearPMU();
  }
}

void SensorHandler()
{
  if (sportsIrq) {
    sportsIrq = false;
    // The interrupt status must be read after an interrupt is detected
    uint16_t status = watch.readBMA();
    Serial.printf("Accelerometer interrupt mask : 0x%x\n", status);

    if (watch.isPedometer()) {
      stepCounter = watch.getPedometerCounter();
      Serial.printf("Step count interrupt,step Counter:%u\n", stepCounter);
    }
    if (watch.isActivity()) {
      Serial.println("Activity interrupt");
    }
    if (watch.isTilt()) {
      Serial.println("Tilt interrupt");
    }
    if (watch.isDoubleTap()) {
      Serial.println("DoubleTap interrupt");
    }
    if (watch.isAnyNoMotion()) {
      Serial.println("Any motion / no motion interrupt");
    }
  }
}


void lowPowerEnergyHandler()
{
  //  Serial.println("Enter light sleep mode!");
  uint32_t inactivityDuration = lv_disp_get_inactive_time(NULL);
  DEFAULT_SCREEN_TIMEOUT_SCREEN_OFF = getPreference("sys_config", "sc_off_timeout", 30, INT) * 1000;

  if (inactivityDuration > DEFAULT_SCREEN_TIMEOUT_SCREEN_OFF) { // More than 30 seconds
    watch.decrementBrightness(NULL);
    watch.clearPMU();

    watch.configreFeatureInterrupt(
      SensorBMA423::INT_STEP_CNTR |   // Pedometer interrupt
      SensorBMA423::INT_ACTIVITY |    // Activity interruption
      SensorBMA423::INT_TILT |        // Tilt interrupt
      SensorBMA423::INT_WAKEUP |      // DoubleTap interrupt
      SensorBMA423::INT_ANY_NO_MOTION,// Any  motion / no motion interrupt
      false);

    sportsIrq = false;
    pmuIrq = false;

    vTaskSuspend(audioRecordingTaskHandler);

    setCpuFrequencyMhz(80);

    destoryAllUI();

    unsigned long previousMillis = 0;  // Stores the last time the delay interval was checked
    const unsigned long interval = 300;  // The desired delay interval in milliseconds
    disableWiFi();

    while (!pmuIrq && !sportsIrq && !watch.getTouched()) {
      unsigned long currentMillis = millis();  // Get the current time

      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;  // Save the current time
        // Your code to execute every interval goes here
      }

      // Other code that needs to run continuously can go here
      inactivityDuration = lv_disp_get_inactive_time(NULL);

      if (inactivityDuration > DEFAULT_SCREEN_TIMEOUT_SLEEP) { // More than 120 seconds
        if (false) {
          disableWiFi();
          watch.configreFeatureInterrupt(
            SensorBMA423::INT_WAKEUP, // DoubleTap interrupt
            true);
          esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

          gpio_wakeup_enable ((gpio_num_t)BOARD_PMU_INT, GPIO_INTR_LOW_LEVEL);
          gpio_wakeup_enable ((gpio_num_t)BOARD_BMA423_INT1, GPIO_INTR_HIGH_LEVEL);

          esp_sleep_enable_gpio_wakeup ();
          esp_light_sleep_start();
        }
      }
    }



    setCpuFrequencyMhz(240);
    tryReconnectWiFi();

    // If WiFi reconnect fails, start the soft access point for the captive portal.
    if (WiFi.status() != WL_CONNECTED || !SERVER_CONNECTED) {
      startSoftAccessPoint(ssid, password, localIP, gatewayIP);
      setUpDNSServer(dnsServer, localIP);
      WiFi.scanNetworks(true); // Start scanning for networks in preparation for the captive portal.
      setUpWebserver(server, localIP); // Set up the web server for the captive portal.
    }

    factory_ui();

    // Clear Interrupts in Loop
    // watch.readBMA();
    // watch.clearPMU();

    watch.configreFeatureInterrupt(
      SensorBMA423::INT_STEP_CNTR |   // Pedometer interrupt
      SensorBMA423::INT_ACTIVITY |    // Activity interruption
      SensorBMA423::INT_TILT |        // Tilt interrupt
      SensorBMA423::INT_WAKEUP |      // DoubleTap interrupt
      SensorBMA423::INT_ANY_NO_MOTION,// Any  motion / no motion interrupt
      true);


    //lv_timer_resume(transmitTask);
    vTaskResume(audioRecordingTaskHandler);

    lv_disp_trig_activity(NULL);
    // Run once
    lv_task_handler();


    watch.incrementalBrightness(getPreference("sys_config", "sc_brightness", 20, UCHAR));

  } else if (inactivityDuration > DEFAULT_SCREEN_TIMEOUT_DIM) { // More than 15 seconds
    lv_task_handler();
    if (watch.getBrightness() == 1) {
      return;
    }
    watch.decrementBrightness(1);
    can_set_brightness = true;

  } else {

  }

}

void loop()
{
  // Don't use delay here, should use elapsed time
  uint32_t last_dns_ms = 0;
  if ((millis() - last_dns_ms) > DNS_INTERVAL) {
    last_dns_ms = millis();            // seems to help with stability, if you are doing other things in the loop this may not be needed
    dnsServer.processNextRequest();    // I call this atleast every 10ms in my other projects (can be higher but I haven't tested it for stability)
  }
  SensorHandler();

  PMUHandler();
  if (WiFi.status() == WL_CONNECTED && !hasSetupWebsocket)
  {
    if (server_domain != "")
    {
      Serial.println("Setting up websocket to 01OS " + server_domain + ":" + server_port);
      websocket_setup(server_domain, server_port);
      InitI2SSpeakerOrMic(MODE_SPK);

      hasSetupWebsocket = true;
      Serial.println("Websocket connection flow completed");
    }
  }

  if (WiFi.status() == WL_CONNECTED && hasSetupWebsocket)
  {

    if (talk_btn_pressed)
    {
      Serial.println("Recording...");
      webSocket.sendTXT("{\"role\": \"user\", \"type\": \"audio\", \"format\": \"bytes.raw\", \"start\": true}");
      InitI2SSpeakerOrMic(MODE_MIC);
      recording = true;
      data_offset = 0;
      Serial.println("Recording ready.");
      talk_btn_pressed = false;
    }
    else if (talk_btn_released)
    {
      Serial.println("Stopped recording.");
      webSocket.sendTXT("{\"role\": \"user\", \"type\": \"audio\", \"format\": \"bytes.raw\", \"end\": true}");
      flush_microphone();
      recording = false;
      data_offset = 0;
      talk_btn_released = false;
    }

    //M5.update();
    webSocket.loop();
  }


  bool screenTimeoutDim = lv_disp_get_inactive_time(NULL) < DEFAULT_SCREEN_TIMEOUT_DIM;
  if (screenTimeoutDim || !canScreenOff) {
    if (!screenTimeoutDim) {

      lv_disp_trig_activity(NULL);
    }
    if (can_set_brightness) {

      watch.incrementalBrightness(getPreference("sys_config", "sc_brightness", 20, UCHAR));
      //      watch.setBrightness(getPreference("sys_config", "sc_brightness", 20, UCHAR));
      can_set_brightness = false;
    }
    lv_task_handler();
    delay(2);
  } else {
    lowPowerEnergyHandler();
  }

}

void tileview_change_cb(lv_event_t *e)
{
  static uint16_t lastPageID = 0;
  lv_obj_t *tileview = lv_event_get_target(e);
  pageId = lv_obj_get_index(lv_tileview_get_tile_act(tileview));
  lv_event_code_t c = lv_event_get_code(e);
  Serial.print("Code : ");
  Serial.print(c);
  uint32_t count =  lv_obj_get_child_cnt(tileview);
  Serial.print(" Count:");
  Serial.print(count);
  Serial.print(" pageId:");
  Serial.println(pageId);

  switch (pageId) {
    case AI_PAGE_ID:
      vTaskResume(audioRecordingTaskHandler);
      canScreenOff = true;
      break;
    default:

      if (lastPageID == AI_PAGE_ID) {
        vTaskSuspend(audioRecordingTaskHandler);
      }
      canScreenOff = true;
      break;
  }
  lastPageID = pageId;
}

void factory_ui()
{

  if (tileview) {
    return;
  }

  static lv_style_t bgStyle;
  lv_style_init(&bgStyle);
  lv_style_set_bg_color(&bgStyle, lv_color_black());

  tileview = lv_tileview_create(lv_scr_act());
  lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_style(tileview, &bgStyle, LV_PART_MAIN);
  lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
  lv_obj_add_event_cb(tileview, tileview_change_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t *t0 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR | LV_DIR_BOTTOM);

  lv_obj_t *t0_1 = lv_tileview_add_tile(tileview, 0, 1, LV_DIR_TOP);

  lv_obj_t *t1 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR | LV_DIR_BOTTOM);

  //lv_obj_t *t0 = lv_tileview_add_tile(tileview, 0, 0,  LV_DIR_HOR);
  //lv_obj_t *t1 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
  //lv_obj_t *t2 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);


  //  analogclock(t1);

  //  aiView(t0);
  //
  //  homeView(t1);
  //  SettingsView(t2);

  homeView(t0);
  aiView(t0_1);

  SettingsView(t1);

  lv_disp_trig_activity(NULL);

  lv_obj_set_tile(tileview, t0, LV_ANIM_OFF);
}



static void talk_btn_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  switch (code) {
    case LV_EVENT_PRESSED:
      talk_btn_pressed = true;
      talk_btn_released = false;
      Serial.println("Pressed");
      //      lv_obj_clear_flag(listening_gif, LV_OBJ_FLAG_HIDDEN); // Show the gif
      watch.setWaveform(0, 78);
      watch.run();
      break;
    case LV_EVENT_RELEASED:
      talk_btn_pressed = false;
      talk_btn_released = true;
      Serial.println("Not Pressed");
      //      lv_obj_add_flag(listening_gif, LV_OBJ_FLAG_HIDDEN); // Hide the gif
      watch.setWaveform(0, 78);
      watch.run();
      break;
    default:
      break;
      //    case LV_EVENT_PRESS_LOST:
      //    case LV_EVENT_SCROLL_BEGIN:
      //      talk_btn_pressed = false;
      //      lv_obj_add_flag(listening_gif, LV_OBJ_FLAG_HIDDEN); // Ensure the gif is hidden in these cases as well
      //      break;
  }
}

lv_obj_t* create_button(lv_obj_t *parent, const char *text, lv_align_t align, lv_coord_t x_offset, lv_coord_t y_offset, lv_color_t bg_color) {

  static lv_style_t btn_style;
  lv_style_init(&btn_style);
  lv_style_set_radius(&btn_style, 50);
  lv_style_set_bg_color(&btn_style, bg_color);
  lv_style_set_text_color(&btn_style, lv_color_white());
  lv_style_set_text_font(&btn_style, &lv_font_montserrat_26);
  lv_style_set_border_opa(&btn_style, LV_OPA_TRANSP);

  lv_obj_t *btn = lv_obj_create(parent);
  lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);
  //  lv_obj_add_event_cb(btn, talk_btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_add_style(btn, &btn_style, 0);
  lv_obj_align(btn, align, x_offset, y_offset);
  lv_obj_set_size(btn, 206, 44);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

void createBightnessSlide(lv_obj_t *parent) {
  //  lv_obj_t *label = lv_label_create(parent);
  String text;

  // Create a transition
  static const lv_style_prop_t props[] = {LV_STYLE_BG_COLOR, LV_STYLE_PROP_INV};
  static lv_style_transition_dsc_t transition_dsc;
  lv_style_transition_dsc_init(&transition_dsc, props, lv_anim_path_linear, 300, 0, NULL);

  // Initialize and set styles
  static lv_style_t style_indicator;
  static lv_style_t style_knob;

  lv_style_init(&style_indicator);
  lv_style_set_bg_opa(&style_indicator, LV_OPA_COVER);
  lv_style_set_bg_color(&style_indicator, lv_color_white());
  lv_style_set_radius(&style_indicator, LV_RADIUS_CIRCLE);
  lv_style_set_transition(&style_indicator, &transition_dsc);

  lv_style_init(&style_knob);
  lv_style_set_bg_opa(&style_knob, LV_OPA_COVER);
  lv_style_set_bg_color(&style_knob, lv_color_white());
  lv_style_set_border_color(&style_knob, lv_color_white());
  lv_style_set_border_width(&style_knob, 2);
  lv_style_set_radius(&style_knob, LV_RADIUS_CIRCLE);
  lv_style_set_pad_all(&style_knob, 6); // Makes the knob larger
  lv_style_set_transition(&style_knob, &transition_dsc);

  //    static lv_style_t title_label_style;
  //    lv_style_init(&title_label_style);
  //    lv_style_set_text_color(&title_label_style, lv_color_white());
  //    lv_obj_add_style(label, &title_label_style, LV_PART_MAIN);
  //    lv_label_set_text(label, "Screen Brightness");
  //     lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

  //  static lv_style_t label_style;
  //  lv_style_init(&label_style);
  //  lv_style_set_text_color(&label_style, lv_color_white());
  //  lv_obj_add_style(label, &label_style, LV_PART_MAIN);
  //  lv_label_set_text(label, text.c_str());
  //  lv_obj_set_style_text_font(label, &font_jetBrainsMono, LV_PART_MAIN);
  //  lv_obj_set_style_text_color(label, DEFAULT_COLOR, LV_PART_MAIN);
  //  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);

  // Create a slider and add the style
  lv_obj_t *slider = lv_slider_create(parent);
  lv_obj_set_size(slider, 200, 20);
  lv_slider_set_range(slider, 5, 255);
  lv_obj_add_style(slider, &style_indicator, LV_PART_INDICATOR);
  lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);

  uint8_t sc_brightness_val = getPreference("sys_config", "sc_brightness", 53, UCHAR);
  int percentage  = levelToPercentage(sc_brightness_val);
  lv_slider_set_value(slider, sc_brightness_val, LV_ANIM_OFF);
  //  lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 30);

  // Create a label below the slider
  lv_obj_t *slider_label = lv_label_create(parent);
  lv_label_set_text_fmt(slider_label, "%u%%", percentage);
  lv_obj_set_style_text_color(slider_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, slider_label);
  lv_obj_align_to(slider_label, slider, LV_ALIGN_CENTER, 0, 0);
}

void SettingsView(lv_obj_t *parent) {
  lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);


  static lv_style_t bgStyle;
  lv_style_init(&bgStyle);
  lv_style_set_bg_color(&bgStyle, lv_color_black());


  /*Create a menu object*/
  lv_obj_t *menu = lv_menu_create(parent);
  lv_obj_set_size(menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
  lv_obj_set_scrollbar_mode(menu, LV_SCROLLBAR_MODE_OFF);
  lv_obj_center(menu);
  lv_obj_align(menu, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_add_style(menu, &bgStyle, LV_PART_MAIN);

  /*Modify the header*/
  static lv_style_t header_style;
  lv_style_init(&header_style);
  lv_style_set_text_color(&header_style, lv_color_white());
  lv_style_set_text_font(&header_style, &lv_font_montserrat_16);
  lv_style_set_text_align(&header_style, LV_TEXT_ALIGN_RIGHT);

  lv_obj_t *header = lv_menu_get_main_header(menu);
  lv_obj_add_style(header, &header_style, 0);

  static lv_style_t back_btn_style;
  lv_style_init(&back_btn_style);
  lv_style_set_text_color(&back_btn_style, lv_color_white());
  lv_style_set_text_font(&back_btn_style, &lv_font_montserrat_16);
  lv_style_set_pad_all(&back_btn_style, 6); // Makes the knob larger


  lv_obj_t *back_btn = lv_menu_get_main_header_back_btn(menu);
  lv_obj_add_style(back_btn, &back_btn_style, 0);
  lv_obj_t *back_btn_label = lv_label_create(back_btn);
  lv_label_set_text(back_btn_label, "Back");


  lv_obj_t *cont;
  lv_obj_t *section;
  lv_obj_t *menu_label;
  lv_obj_t *menu_btn;



  static lv_style_t title_label_style;
  lv_style_init(&title_label_style);
  lv_style_set_text_color(&title_label_style, lv_color_white());



  /*Create sub pages*/
  lv_obj_t *display_page = lv_menu_page_create(menu, "Dispaly");
  lv_obj_set_scrollbar_mode(display_page, LV_SCROLLBAR_MODE_OFF);
  cont = lv_menu_cont_create(display_page);
  //  lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT); // Make container fill the width of the page
  lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);


  menu_label = lv_label_create(cont);
  lv_obj_add_style(menu_label, &title_label_style, 0);
  lv_label_set_text(menu_label, "Screen Brightness");


  cont = lv_menu_cont_create(display_page);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  createBightnessSlide(cont);

  cont = lv_menu_cont_create(display_page);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

  lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  menu_label = lv_label_create(cont);
  lv_obj_add_style(menu_label, &title_label_style, 0);
  lv_label_set_text(menu_label, "Screen Off Timeout");


  cont = lv_menu_cont_create(display_page);
  lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t *dim_timeout = createSpinBox(cont, lv_spinbox_event_cb);




  lv_obj_t *wifi_page = lv_menu_page_create(menu, "WiFi");
  lv_obj_set_scrollbar_mode(wifi_page, LV_SCROLLBAR_MODE_OFF);

  cont = lv_menu_cont_create(wifi_page);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  menu_label = lv_label_create(cont);
  lv_obj_add_style(menu_label, &title_label_style, 0);
  lv_label_set_text(menu_label, "You are connected to:");


  cont = lv_menu_cont_create(wifi_page);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

   static lv_style_t content_style;
  lv_style_init(&content_style);
  lv_style_set_text_color(&content_style, lv_color_white());
  lv_style_set_text_font(&content_style, &lv_font_montserrat_24);


  menu_label = lv_label_create(cont);
  lv_obj_add_style(menu_label, &content_style, 0);
  
  if (WiFi.status() == WL_CONNECTED)
    {
        lv_label_set_text(menu_label,WiFi.SSID().c_str());
    }
    else
    {
        lv_label_set_text(menu_label, "No Wifi Connection!");
    }



  lv_obj_t *date_time_page = lv_menu_page_create(menu, "Date&Time");
  lv_obj_set_scrollbar_mode(date_time_page, LV_SCROLLBAR_MODE_OFF);
  cont = lv_menu_cont_create(date_time_page);
  datetimeVeiw(cont);


  /*Create a main page*/
  lv_obj_t *main_page = lv_menu_page_create(menu, "Settings");

  cont = lv_menu_cont_create(main_page);
  menu_btn = create_button(cont, "Display", LV_ALIGN_CENTER, 0, 0, lv_color_hex(0x1E1C1C));
  lv_menu_set_load_page_event(menu, menu_btn, display_page);

  cont = lv_menu_cont_create(main_page);
  menu_btn = create_button(cont, "WiFi", LV_ALIGN_CENTER, 0, 0, lv_color_hex(0x1E1C1C));
  lv_menu_set_load_page_event(menu, menu_btn, wifi_page);

  cont = lv_menu_cont_create(main_page);
  menu_btn = create_button(cont, "Date&Time", LV_ALIGN_CENTER, 0, 0, lv_color_hex(0x1E1C1C));
  lv_menu_set_load_page_event(menu, menu_btn, date_time_page);

  lv_menu_set_page(menu, main_page);
}

void aiView(lv_obj_t *parent) {

  lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);
  lv_obj_t *label;

  fox_gif = lv_gif_create(parent);
  lv_gif_set_src(fox_gif, &fox);
  lv_obj_align(fox_gif, LV_ALIGN_TOP_MID, 0, 20);


  //  // Create a battery icon
  //  static lv_style_t battery_style;
  //  lv_style_init(&battery_style);
  //  lv_style_set_text_color(&battery_style, lv_color_white());
  //  lv_style_set_text_font(&battery_style, &lv_font_montserrat_16);


  //  battery_icon = lv_label_create(parent);
  //  lv_obj_add_style(battery_icon, &battery_style, 0);
  //  //  lv_label_set_text(battery_icon, "80%" LV_SYMBOL_BATTERY_3);
  //  lv_obj_align(battery_icon, LV_ALIGN_TOP_MID, 0, 5);

  //  static lv_style_t style_talk_btn;
  //  lv_style_init(&style_talk_btn);
  //
  //
  //  lv_style_set_border_opa(&style_talk_btn, LV_OPA_TRANSP);  // Border opacity
  //  lv_style_set_border_width(&style_talk_btn, 0);  // Ensure no border width
  //  lv_style_set_outline_opa(&style_talk_btn, LV_OPA_TRANSP);  // Outline opacity
  //  lv_style_set_outline_width(&style_talk_btn, 0);  // Ensure no outline width
  //  lv_style_set_border_width(&style_talk_btn, 0);
  //  lv_style_set_outline_width(&style_talk_btn, 0);
  //  lv_style_set_pad_all(&style_talk_btn, 0);  // Padding

  static lv_style_t style_talk_btn;
  lv_style_init(&style_talk_btn);
  lv_style_set_radius(&style_talk_btn, 50);
  lv_color_t btn_bg = lv_color_white();
  lv_style_set_bg_color(&style_talk_btn, btn_bg);
  lv_style_set_text_color(&style_talk_btn, lv_color_black());
  lv_style_set_text_font(&style_talk_btn, &microphone);

  lv_obj_t *talk_btn = lv_btn_create(parent);
  lv_obj_add_event_cb(talk_btn, talk_btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_add_style(talk_btn, &style_talk_btn, 0);
  lv_obj_align(talk_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_size(talk_btn, 200, 50);
  label = lv_label_create(talk_btn);
  lv_label_set_text(label, MIC_SYMBOL);
  lv_obj_center(label);


  //  lv_timer_create([](lv_timer_t *timer2) {
  //    //    time_t now;
  //    //    struct tm  timeinfo;
  //    //    time(&now);
  //    //    localtime_r(&now, &timeinfo);
  //    //    static  bool rever = false;
  //    //    lv_label_set_text_fmt(clock_label, rever ? "%02d:%02d" : "%02d %02d", timeinfo.tm_hour, timeinfo.tm_min);
  //    //    lv_label_set_text_fmt(clock_label, rever ? "%02d:%02d" : "%02d %02d", timeinfo.tm_hour, timeinfo.tm_min);
  //    // Assuming you have a function to get battery percentage
  //    int percent = watch.getBatteryPercent();
  //    const char* battery_status_icon = "";
  //
  //    if (percent == 100) {
  //      battery_status_icon = LV_SYMBOL_BATTERY_FULL;
  //    } else if (percent >= 75) {
  //      battery_status_icon = LV_SYMBOL_BATTERY_3;
  //    } else if (percent >= 50) {
  //      battery_status_icon = LV_SYMBOL_BATTERY_2;
  //    } else if (percent >= 25) {
  //      battery_status_icon = LV_SYMBOL_BATTERY_1;
  //    } else {
  //      battery_status_icon = LV_SYMBOL_BATTERY_EMPTY; // Make sure this symbol is defined
  //    }
  //
  //    const char* charging_icon = "";
  //    if (usbPlugIn) {
  //      charging_icon = LV_SYMBOL_CHARGE;
  //    }
  //
  //
  //    const char* wifi_icon = "";
  //    if (WiFi.status() == WL_CONNECTED) {
  //      wifi_icon = LV_SYMBOL_WIFI;
  //    }
  //
  //    // Use format specifiers correctly
  //    lv_label_set_text_fmt(battery_icon, "%d%% %s %s %s", percent == -1 ? 0 : percent, battery_status_icon, charging_icon, wifi_icon);
  //
  //    //    rever = !rever;
  //  },
  //  1000, NULL);
}


void homeView(lv_obj_t *parent) {

  lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);
  lv_obj_t *label;

  fox_gif = lv_gif_create(parent);
  lv_gif_set_src(fox_gif, &fox);
  lv_obj_align(fox_gif, LV_ALIGN_BOTTOM_MID, 0, -10);

  clock_label = lv_label_create(parent);
  lv_obj_set_style_text_font(clock_label, &font_firacode_60, LV_PART_MAIN);
  lv_obj_set_style_text_color(clock_label, lv_color_white(), LV_PART_MAIN);
  lv_label_set_text(clock_label, "9:45");
  lv_obj_align(clock_label, LV_ALIGN_TOP_MID, 0, 20); // Align below the logo



  // Create a battery icon
  static lv_style_t battery_style;
  lv_style_init(&battery_style);
  lv_style_set_text_color(&battery_style, lv_color_white());
  lv_style_set_text_font(&battery_style, &lv_font_montserrat_16);


  battery_icon = lv_label_create(parent);
  lv_obj_add_style(battery_icon, &battery_style, 0);
  lv_label_set_text(battery_icon, "80%" LV_SYMBOL_BATTERY_3);
  lv_obj_align(battery_icon, LV_ALIGN_TOP_MID, 0, 5);

  static lv_style_t style_talk_btn;
  lv_style_init(&style_talk_btn);


  lv_style_set_border_opa(&style_talk_btn, LV_OPA_TRANSP);  // Border opacity
  lv_style_set_border_width(&style_talk_btn, 0);  // Ensure no border width
  lv_style_set_outline_opa(&style_talk_btn, LV_OPA_TRANSP);  // Outline opacity
  lv_style_set_outline_width(&style_talk_btn, 0);  // Ensure no outline width
  lv_style_set_border_width(&style_talk_btn, 0);
  lv_style_set_outline_width(&style_talk_btn, 0);
  lv_style_set_pad_all(&style_talk_btn, 0);  // Padding


  lv_timer_create([](lv_timer_t *timer) {
    time_t now;
    struct tm  timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    static  bool rever = false;
    lv_label_set_text_fmt(clock_label, rever ? "%02d:%02d" : "%02d %02d", timeinfo.tm_hour, timeinfo.tm_min);
    //    lv_label_set_text_fmt(clock_label, rever ? "%02d:%02d" : "%02d %02d", timeinfo.tm_hour, timeinfo.tm_min);
    // Assuming you have a function to get battery percentage
    int percent = watch.getBatteryPercent();
    const char* battery_status_icon = "";

    if (percent == 100) {
      battery_status_icon = LV_SYMBOL_BATTERY_FULL;
    } else if (percent >= 75) {
      battery_status_icon = LV_SYMBOL_BATTERY_3;
    } else if (percent >= 50) {
      battery_status_icon = LV_SYMBOL_BATTERY_2;
    } else if (percent >= 25) {
      battery_status_icon = LV_SYMBOL_BATTERY_1;
    } else {
      battery_status_icon = LV_SYMBOL_BATTERY_EMPTY; // Make sure this symbol is defined
    }

    const char* charging_icon = "";
    if (usbPlugIn) {
      charging_icon = LV_SYMBOL_CHARGE;
    }


    const char* wifi_icon = "";
    if (WiFi.status() == WL_CONNECTED) {
      wifi_icon = LV_SYMBOL_WIFI;
    }

    // Use format specifiers correctly
    lv_label_set_text_fmt(battery_icon, "%d%% %s %s %s", percent == -1 ? 0 : percent, battery_status_icon, charging_icon, wifi_icon);

    rever = !rever;
  },
  1000, NULL);
}




static void slider_event_cb(lv_event_t *e) {
  lv_obj_t *slider = lv_event_get_target(e);
  lv_obj_t *slider_label = (lv_obj_t *)lv_event_get_user_data(e);
  uint8_t level = (uint8_t)lv_slider_get_value(slider);  // Brightness level (0-255)
  //    int percentage = map(level, 0, 255, 0, 100);  // Map level to percentage for display
  int percentage  = levelToPercentage(level);

  // Set the label text to show percentage
  lv_label_set_text_fmt(slider_label, "%u%%", percentage);
  lv_obj_align_to(slider_label, slider, LV_ALIGN_CENTER, 0, 0);

  // Set brightness using the actual level
  watch.setBrightness(level);

  // Save the actual brightness level, not the percentage
  setPreference("sys_config", "sc_brightness", level, UCHAR);

  // Retrieve the brightness level from preferences
  uint8_t saved_level = getPreference("sys_config", "sc_brightness", 53, UCHAR);
}


static void light_sw_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    Serial.printf("State: %s\n", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off");
    lightSleep = lv_obj_has_state(obj, LV_STATE_CHECKED);
  }
}




static void lv_dim_screen_spinbox_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    bool *inc =  (bool *)lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_current_target(e);
    lv_datetime_t *datetime_obj =  (lv_datetime_t *)lv_obj_get_user_data(target);
    if (!datetime_obj) {
      Serial.println("datetime_obj is null");
      return;
    }
    Serial.print(datetime_obj->name);

    if (*inc) {
      lv_spinbox_increment(datetime_obj->obj);
    } else {
      lv_spinbox_decrement(datetime_obj->obj);
    }

  }
}


static void lv_spinbox_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    bool *inc =  (bool *)lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_current_target(e);
    lv_datetime_t *datetime_obj =  (lv_datetime_t *)lv_obj_get_user_data(target);
    if (!datetime_obj) {
      Serial.println("datetime_obj is null");
      return;
    }
    Serial.print(datetime_obj->name);

    if (*inc) {
      lv_spinbox_increment(datetime_obj->obj);
    } else {
      lv_spinbox_decrement(datetime_obj->obj);
    }

  }
}

static void lv_spinbox_increment_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  //    lv_obj_t *spinbox = lv_event_get_current_target(e);
  lv_obj_t *spinbox  =  (lv_obj_t *)lv_event_get_user_data(e);
  if (code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
    lv_spinbox_increment(spinbox);
    int32_t spinbox_value =  lv_spinbox_get_value(spinbox);
    setPreference("sys_config", "sc_off_timeout", spinbox_value, INT);
    Serial.print("Getting Spinbox Value: "); Serial.println(spinbox_value);
  }
}

static void lv_spinbox_decrement_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  //    lv_obj_t *spinbox = lv_event_get_current_target(e);
  lv_obj_t *spinbox  =  (lv_obj_t *)lv_event_get_user_data(e);
  if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    lv_spinbox_decrement(spinbox);
    int32_t spinbox_value =  lv_spinbox_get_value(spinbox);
    setPreference("sys_config", "sc_off_timeout", spinbox_value, INT);
    Serial.print("Getting Spinbox Value: "); Serial.println(spinbox_value);
  }
}

lv_obj_t *createSpinBox(lv_obj_t *parent, lv_event_cb_t event_cb)
{
  static lv_style_t cont_style;
  lv_style_init(&cont_style);
  lv_style_set_bg_opa(&cont_style, LV_OPA_TRANSP);
  lv_style_set_bg_img_opa(&cont_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&cont_style, DEFAULT_COLOR);

  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, 185, 45);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_scroll_dir(cont, LV_DIR_NONE);

  lv_obj_align_to(cont, parent, LV_ALIGN_OUT_TOP_MID, 0, -10);

  lv_obj_set_style_pad_all(cont, 1, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);

  lv_coord_t w = 40;
  lv_coord_t h = 40;
  lv_obj_t *plus_btn = lv_btn_create(cont);
  lv_obj_set_size(plus_btn, w, h);
  lv_obj_set_style_bg_img_src(plus_btn, LV_SYMBOL_PLUS, 0);
  lv_obj_add_style(plus_btn, &button_default_style, LV_PART_MAIN);
  lv_obj_add_style(plus_btn, &button_press_style, LV_STATE_PRESSED);

  lv_obj_t *spinbox = lv_spinbox_create(cont);
  lv_obj_set_scrollbar_mode(spinbox, LV_SCROLLBAR_MODE_OFF);
  lv_spinbox_set_step(spinbox, 1);
  lv_spinbox_set_rollover(spinbox, false);
  lv_spinbox_set_cursor_pos(spinbox, 0);

  lv_spinbox_set_digit_format(spinbox, 2, 0);
  lv_spinbox_set_range(spinbox, 15, 60);
  lv_spinbox_set_value(spinbox, getPreference("sys_config", "sc_off_timeout", 15, INT));
  lv_obj_set_style_text_align(spinbox, LV_TEXT_ALIGN_CENTER, 0);


  lv_obj_set_width(spinbox, 80);
  lv_obj_set_height(spinbox, h + 2);

  lv_obj_set_style_bg_color(spinbox, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_bg_color(spinbox, lv_color_black(), LV_PART_CURSOR);
  lv_obj_set_style_text_color(spinbox, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_text_font(spinbox, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_border_color(spinbox, DEFAULT_COLOR, LV_PART_MAIN);


  lv_obj_t * minus_btn = lv_btn_create(cont);
  lv_obj_set_size(minus_btn, w, h);
  lv_obj_set_style_bg_img_src(minus_btn, LV_SYMBOL_MINUS, 0);
  lv_obj_add_style(minus_btn, &button_default_style, LV_PART_MAIN);
  lv_obj_add_style(minus_btn, &button_press_style, LV_STATE_PRESSED);

  lv_obj_add_event_cb(plus_btn, lv_spinbox_increment_event_cb, LV_EVENT_ALL,  spinbox);
  lv_obj_add_event_cb(minus_btn, lv_spinbox_decrement_event_cb, LV_EVENT_ALL, spinbox);

  return spinbox;
}


lv_obj_t *createAdjustButton(lv_obj_t *parent, const char *txt, lv_event_cb_t event_cb, void *user_data)
{
  static lv_style_t cont_style;
  lv_style_init(&cont_style);
  lv_style_set_bg_opa(&cont_style, LV_OPA_TRANSP);
  lv_style_set_bg_img_opa(&cont_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&cont_style, DEFAULT_COLOR);

  lv_obj_t *label_cont = lv_obj_create(parent);
  lv_obj_set_size(label_cont, 210, 90);
  lv_obj_set_scrollbar_mode(label_cont, LV_SCROLLBAR_MODE_OFF);
  // lv_obj_set_flex_flow(label_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_scroll_dir(label_cont, LV_DIR_NONE);
  lv_obj_set_style_pad_top(label_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(label_cont, 2, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(label_cont, LV_OPA_TRANSP, LV_PART_MAIN);
  //    lv_obj_set_style_border_width(label_cont, 5, LV_PART_MAIN);
  //    lv_obj_set_style_border_color(label_cont, DEFAULT_COLOR, LV_PART_MAIN);

  lv_obj_t *label = lv_label_create(label_cont);
  lv_label_set_text(label, txt);
  //    lv_obj_set_style_text_font(label, &font_siegra, LV_PART_MAIN);
  lv_obj_set_style_text_color(label, DEFAULT_COLOR, LV_PART_MAIN);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 5, 0);


  lv_obj_t *cont = lv_obj_create(label_cont);
  lv_obj_set_size(cont, 185, 45);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_scroll_dir(cont, LV_DIR_NONE);
  lv_obj_align_to(cont, label, LV_ALIGN_OUT_BOTTOM_LEFT, -6, 5);

  lv_obj_set_style_pad_all(cont, 1, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);

  lv_coord_t w = 40;
  lv_coord_t h = 40;
  lv_obj_t *btn = lv_btn_create(cont);
  lv_obj_set_size(btn, w, h);
  lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_PLUS, 0);
  lv_obj_add_style(btn, &button_default_style, LV_PART_MAIN);
  lv_obj_add_style(btn, &button_press_style, LV_STATE_PRESSED);



  static bool increment = 1;
  static bool decrement = 0;
  lv_obj_set_user_data(btn, user_data);
  lv_obj_add_event_cb(btn, event_cb, LV_EVENT_ALL, &increment);

  lv_obj_t *spinbox = lv_spinbox_create(cont);
  lv_spinbox_set_step(spinbox, 1);
  lv_spinbox_set_rollover(spinbox, false);
  lv_spinbox_set_cursor_pos(spinbox, 0);

  if (user_data) {
    lv_datetime_t *datetime_obj = (lv_datetime_t *)user_data;
    lv_spinbox_set_digit_format(spinbox, datetime_obj->digitFormat, 0);
    lv_spinbox_set_range(spinbox, datetime_obj->minVal, datetime_obj->maxVal);
    lv_spinbox_set_value(spinbox, datetime_obj->defaultVal);
  }
  lv_obj_set_width(spinbox, 65);
  lv_obj_set_height(spinbox, h + 2);

  lv_obj_set_style_bg_opa(spinbox, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_text_color(spinbox, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_border_color(spinbox, DEFAULT_COLOR, LV_PART_MAIN);
  //    lv_obj_set_style_text_font(spinbox, &font_sandbox, LV_PART_MAIN);

  // lv_obj_set_style_bg_opa(spinbox, LV_OPA_TRANSP, LV_PART_SELECTED);
  // lv_obj_set_style_bg_opa(spinbox, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_bg_opa(spinbox, LV_OPA_TRANSP, LV_PART_CURSOR);


  btn = lv_btn_create(cont);
  lv_obj_set_size(btn, w, h);
  lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_MINUS, 0);
  lv_obj_add_style(btn, &button_default_style, LV_PART_MAIN);
  lv_obj_add_style(btn, &button_press_style, LV_STATE_PRESSED);
  lv_obj_set_user_data(btn, user_data);
  // lv_obj_add_event_cb(btn, lv_spinbox_decrement_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(btn, event_cb, LV_EVENT_ALL, &decrement);

  return spinbox;
}

static void datetime_event_handler(lv_event_t *e)
{
  Serial.println("Save setting datetime.");
  int32_t year =  lv_spinbox_get_value(lv_datetime[0].obj);
  int32_t month =  lv_spinbox_get_value(lv_datetime[1].obj);
  int32_t day =  lv_spinbox_get_value(lv_datetime[2].obj);
  int32_t hour =  lv_spinbox_get_value(lv_datetime[3].obj);
  int32_t minute =  lv_spinbox_get_value(lv_datetime[4].obj);
  int32_t second =  lv_spinbox_get_value(lv_datetime[5].obj);

  Serial.printf("Y=%dM=%dD=%d H:%dM:%dS:%d\n", year, month, day,
                hour, minute, second);

  watch.setDateTime(year, month, day, hour, minute, second);

  // Reading time synchronization from RTC to system time
  watch.hwClockRead();
}




void datetimeVeiw(lv_obj_t *parent)
{
  lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);

  //set default datetime
  time_t now;
  struct tm  info;
  time(&now);
  localtime_r(&now, &info);
  lv_datetime[0].defaultVal = info.tm_year + 1900;
  lv_datetime[1].defaultVal = info.tm_mon + 1;
  lv_datetime[2].defaultVal = info.tm_mday;
  lv_datetime[3].defaultVal = info.tm_hour;
  lv_datetime[4].defaultVal = info.tm_min ;
  lv_datetime[5].defaultVal = info.tm_sec ;


  static lv_style_t cont_style;
  lv_style_init(&cont_style);
  lv_style_set_bg_opa(&cont_style, LV_OPA_TRANSP);
  lv_style_set_bg_img_opa(&cont_style, LV_OPA_TRANSP);
  lv_style_set_line_opa(&cont_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&cont_style, 0);
  lv_style_set_text_color(&cont_style, DEFAULT_COLOR);

  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, lv_disp_get_hor_res(NULL), 400);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scroll_dir(cont, LV_DIR_VER);
  lv_obj_add_style(cont, &cont_style, LV_PART_MAIN);

  for (int i = 0; i < sizeof(lv_datetime) / sizeof(lv_datetime[0]); ++i) {
    lv_datetime[i].obj =  createAdjustButton(cont, lv_datetime[i].name, lv_spinbox_event_cb, &(lv_datetime[i]));
  }

  lv_obj_t *btn_cont = lv_obj_create(cont);
  lv_obj_set_size(btn_cont, 210, 60);
  lv_obj_set_scrollbar_mode(btn_cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_scroll_dir(btn_cont, LV_DIR_NONE);
  lv_obj_set_style_pad_top(btn_cont, 5, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(btn_cont, 5, LV_PART_MAIN);
  lv_obj_set_style_border_opa(btn_cont, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, LV_PART_MAIN);

  lv_obj_t *btn = lv_btn_create(btn_cont);
  lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_SAVE, 0);
  lv_obj_set_size(btn, 180, 50);
  lv_obj_add_style(btn, &button_default_style, LV_PART_MAIN);
  lv_obj_add_style(btn, &button_press_style, LV_STATE_PRESSED);
  lv_obj_add_event_cb(btn, datetime_event_handler, LV_EVENT_CLICKED, NULL);
}




void settingButtonStyle()
{
  /*Init the button_default_style for the default state*/
  lv_style_init(&button_default_style);

  lv_style_set_radius(&button_default_style, LV_RADIUS_CIRCLE);

  lv_style_set_bg_opa(&button_default_style, LV_OPA_100);
  lv_style_set_bg_color(&button_default_style, lv_color_white());
  //  lv_style_set_bg_grad_color(&button_default_style, lv_palette_darken(LV_PALETTE_YELLOW, 2));
  //  lv_style_set_bg_grad_dir(&button_default_style, LV_GRAD_DIR_VER);

  //  lv_style_set_border_opa(&button_default_style, LV_OPA_40);
  //  lv_style_set_border_width(&button_default_style, 2);
  //  lv_style_set_border_color(&button_default_style, lv_palette_main(LV_PALETTE_GREY));
  //
  //  lv_style_set_shadow_width(&button_default_style, 8);
  //  lv_style_set_shadow_color(&button_default_style, lv_palette_main(LV_PALETTE_GREY));
  //  lv_style_set_shadow_ofs_y(&button_default_style, 8);

  //  lv_style_set_outline_opa(&button_default_style, LV_OPA_COVER);
  //  lv_style_set_outline_color(&button_default_style, lv_palette_main(LV_PALETTE_YELLOW));
  //
  lv_style_set_text_color(&button_default_style, lv_color_black());
  //  lv_style_set_pad_all(&button_default_style, 10);

  /*Init the pressed button_default_style*/
  lv_style_init(&button_press_style);

  /*Add a large outline when pressed*/
  //  lv_style_set_outline_width(&button_press_style, 30);
  //  lv_style_set_outline_opa(&button_press_style, LV_OPA_TRANSP);
  //
  //  lv_style_set_translate_y(&button_press_style, 5);
  //  lv_style_set_shadow_ofs_y(&button_press_style, 3);
  //  lv_style_set_bg_color(&button_press_style, lv_palette_darken(LV_PALETTE_YELLOW, 2));
  //  lv_style_set_bg_grad_color(&button_press_style, lv_palette_darken(LV_PALETTE_YELLOW, 4));

  /*Add a transition to the outline*/
  static lv_style_transition_dsc_t trans;
  static lv_style_prop_t props[] = {LV_STYLE_OUTLINE_WIDTH, LV_STYLE_OUTLINE_OPA, LV_STYLE_PROP_INV};
  lv_style_transition_dsc_init(&trans, props, lv_anim_path_linear, 300, 0, NULL);

  lv_style_set_transition(&button_press_style, &trans);

}
/*
 ************************************
        HARDWARE SETTING
 ************************************
*/
void setSportsFlag()
{
  sportsIrq = true;
}

void settingSensor()
{

  //Default 4G ,200HZ
  watch.configAccelerometer();

  watch.enableAccelerometer();

  watch.enablePedometer();

  watch.configInterrupt();

  watch.enableFeature(SensorBMA423::FEATURE_STEP_CNTR |
                      SensorBMA423::FEATURE_ANY_MOTION |
                      SensorBMA423::FEATURE_NO_MOTION |
                      SensorBMA423::FEATURE_ACTIVITY |
                      SensorBMA423::FEATURE_TILT |
                      SensorBMA423::FEATURE_WAKEUP,
                      true);


  watch.enablePedometerIRQ();
  watch.enableTiltIRQ();
  watch.enableWakeupIRQ();
  watch.enableAnyNoMotionIRQ();
  watch.enableActivityIRQ();


  watch.attachBMA(setSportsFlag);

}

void setPMUFlag()
{
  pmuIrq = true;
}

void settingPMU()
{
  watch.clearPMU();

  watch.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  // Enable the required interrupt function
  watch.enableIRQ(
    // XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
    XPOWERS_AXP2101_VBUS_INSERT_IRQ   | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS
    XPOWERS_AXP2101_PKEY_SHORT_IRQ    | XPOWERS_AXP2101_PKEY_LONG_IRQ       |  //POWER KEY
    XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ       //CHARGE
    // XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
  );
  watch.attachPMU(setPMUFlag);

}
