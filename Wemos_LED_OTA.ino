#include <ESP8266WiFi.h>

/*------------------------------
 * FASTLED 
 -----------------------------*/
 #include <bitswap.h>
#include <chipsets.h>
#include <color.h>
#include <colorpalettes.h>
#include <colorutils.h>
#include <controller.h>
#include <cpp_compat.h>
#include <dmx.h>
#include <FastLED.h>
#include <fastled_config.h>
#include <fastled_delay.h>
#include <fastled_progmem.h>
#include <fastpin.h>
#include <fastspi.h>
#include <fastspi_bitbang.h>
#include <fastspi_dma.h>
#include <fastspi_nop.h>
#include <fastspi_ref.h>
#include <fastspi_types.h>
#include <hsv2rgb.h>
#include <led_sysdefs.h>
#include <lib8tion.h>
#include <noise.h>
#include <pixelset.h>
#include <pixeltypes.h>
#include <platforms.h>
#include <power_mgt.h>
#define DATA_PIN    5
//#define CLK_PIN   4
#define LED_TYPE    WS2812
#define COLOR_ORDER RGB
#define NUM_LEDS    300
CRGB leds[NUM_LEDS];
#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  60

/*------------------------------
 * Web Server
 -----------------------------*/
#include <ESP8266WebServer.h> //For web socket 
#include <ArduinoJson.h> //For sending 
ESP8266WebServer server(80);
bool flashEnable = 0;
int flashLoop = 0;
int flashCount = 0;
int flashRed = 0;
int flashGreen = 0;
int flashblue = 0;

/*------------------------------
 * OTA 
 -----------------------------*/
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid = "ssid";
const char* password = "password";

void setup() {
  /*------------------------------
  * OTA and Serial
  -----------------------------*/
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("BinaryLEDS");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  /*------------------------------
  * FASTLED
  -----------------------------*/
   // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  /*------------------------------
  * WEBSERVER
  -----------------------------*/
  server.on("/pew", handlePew);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  //Enable Over-The-Air-Updates
  ArduinoOTA.handle();
  //Wait for requests
  server.handleClient();
}

void handlePew() {
  //Turn on BuiltinLED
  ledOn();
  //Setup a JSON doc
  StaticJsonDocument<200> jsonObj;
  char JSONmessageBuffer[200];
  
  //Handle the LED Call 
  uint8_t hue = 0;
  for(int i = 1; i < NUM_LEDS; i++) {
   // Set the i'th led to on 
    leds[i] = CHSV(hue++, 255, 255);
    // Set the i'th led to off
    leds[i - 1] = CRGB::Black;
    // Show the leds
     FastLED.show(); 
//    // now that we've shown the leds, reset the i'th led to black
//    // leds[i] = CRGB::Black;
//    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(.2);
  }

  //Return response
  jsonObj["length"] = 1;
  jsonObj["direction"] = 1;
  jsonObj["r"] = 1;
  jsonObj["g"] = 1;
  jsonObj["b"] = 1;
  serializeJsonPretty(jsonObj, JSONmessageBuffer);
  server.send(200, "application/json", JSONmessageBuffer);
  ledOff();
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

void ledOn() {
  digitalWrite(BUILTIN_LED, LOW);
}

void ledOff() {
  digitalWrite(BUILTIN_LED, HIGH);
}
