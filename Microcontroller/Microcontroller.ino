
#include "math.h"

// WiFi Creds
#include "secret.h"

// Board specific features
#include "heltec.h"

// WiFi Features
#include "WiFi.h"
#include "HTTPClient.h"
#include <ArduinoJson.h>
char server[] = "https://raw.githubusercontent.com/djdevine/NeoPixel-Vaccination-Tracker/main/Server/data.json";
WiFiClient client;

// For NeoPixels
#include "FastLED.h"
#include "FastLED_RGBW.h"
#define NUM_LEDS 144
#define DATA_PIN 32
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];

unsigned long previousMillis = 0;
unsigned long interval = 30000;

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);

  Heltec.display -> clear();
  Heltec.display -> drawString(0, 0, "Disconnected from WiFi access point");
  Heltec.display -> drawString(0, 10, "Reason: " + info.disconnected.reason);
  Heltec.display -> display();
  
  delay(5000);
  Serial.println("Trying to Reconnect");
  Heltec.display -> drawString(0, 30, "Trying to reconnect...");
  Heltec.display -> display();
  WiFi.begin(WIFI_NAME, WIFI_PASSPHRASE);
}

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);

  Heltec.display -> clear();
  Heltec.display -> drawString(0, 0, "Starting");
  Heltec.display -> display();

  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  FastLED.setBrightness(20);
  FastLED.show();

  WiFi.disconnect(true);

  delay(1000);

  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);

  WiFi.begin(WIFI_NAME, WIFI_PASSPHRASE);
    
  Serial.println();
  Serial.println();
  Serial.println("Waiting for WiFi... ");
  Heltec.display -> drawString(0, 10, "Waiting for WiFi...");
  Heltec.display -> display();
}

void loop() {
  // Fetch new figures every 30s
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    updateFigures();
    previousMillis = currentMillis;
  }
}

void updateFigures(void)
{
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient client;

    client.begin(server);
    int httpCode = client.GET();

    if (httpCode > 0) {
      String jsonRaw = client.getString();

      char json[500];
      jsonRaw.replace(" ", "");
      jsonRaw.replace("\n", "");
      jsonRaw.trim();
      //jsonRaw.remove(0,1);
      jsonRaw.toCharArray(json, 1000);

      StaticJsonDocument<500> doc;
      deserializeJson(doc, json);

      int firstDose = doc["firstDose"];
      int secondDose = doc["bothDoses"];
      int unvaccinated = doc["unvaccinated"];
      int under18 = doc["under18"];
      String timestamp = doc["updated"];

      client.end();

      Serial.println("JSON Deserialised");
      Serial.println("First Dose: " + String(firstDose));
      Serial.println("Both Doses: " + String(secondDose));
      Serial.println("Unvaccinated: " + String(unvaccinated));
      Serial.println("Under 18: " + String(under18));
      Serial.println("Timestamp: " + timestamp.substring(0, 10));
      Serial.println();

      Heltec.display -> clear();
      Heltec.display -> drawString(0, 0, "First Dose: " + String(firstDose));
      Heltec.display -> drawString(0, 10, "Both Doses: " + String(secondDose));
      Heltec.display -> drawString(0, 20, "Unvaccinated: " + String(unvaccinated));
      Heltec.display -> drawString(0, 30, "Under 18: " + String(under18));
      Heltec.display -> drawString(0, 45, "Timestamp: " + timestamp.substring(0, 10));
      Heltec.display -> display();

      plotStats(firstDose, secondDose, unvaccinated, under18);

    }
    else {
      Serial.println("HTTP Request Error");
      Heltec.display -> drawString(0, 0, "HTTP Request Error, retry in 30s");
      Heltec.display -> display();
    }
  }
}

void plotStats(int first, int both, int unvac, int underage)
{
  // normalise stats
  int n = first + both + unvac + underage;
  int norm_first = map(first, 0, n, 0, NUM_LEDS);
  int norm_both = map(both, 0, n, 0, NUM_LEDS);
  int norm_unvac = map(unvac, 0, n, 0, NUM_LEDS);
  int norm_underage = map(underage, 0, n, 0, NUM_LEDS);

  Serial.println("Normalised Data");
  Serial.println("First Dose: " + String(norm_first));
  Serial.println("Both Doses: " + String(norm_both));
  Serial.println("Unvaccinated: " + String(norm_unvac));
  Serial.println("Under 18: " + String(norm_underage));
  Serial.println();

  // set LEDS
  for (int i = 0; i < norm_first; i++) {
    leds[i] = CHSV(64, 255, 255);
  }
  for (int i = norm_first; i < norm_first + norm_both; i++) {
    leds[i] = CHSV(110, 255, 255);
  }
  for (int i = norm_first + norm_both; i < norm_first + norm_both + norm_unvac; i++) {
    leds[i] = CHSV(192, 255, 255);
  }
  for (int i = norm_first + norm_both + norm_unvac; i < norm_first + norm_both + norm_unvac + norm_underage; i++) {
    leds[i] = CHSV(0, 255, 255);
  }

  FastLED.show();
}
