#include <ArduinoJson.h>

#include "secret.h"

#include "heltec.h"
#include "WiFi.h"
#include "HTTPClient.h"

#include "math.h"

#include "FastLED.h"
#include "FastLED_RGBW.h"

#define NUM_LEDS 144
#define DATA_PIN 32

CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];

WiFiClient client;

char server[] = "https://raw.githubusercontent.com/djdevine/NeoPixel-Vaccination-Tracker/main/Server/data.json";

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);

  Heltec.display -> clear();
  Heltec.display -> drawString(0, 0, "Connecting To WiFi...");
  Heltec.display -> display();

  WiFiConnect();

  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  FastLED.setBrightness(100);
  FastLED.show();
}

void loop() {
  //rainbowLoop();

  delay(10000);

  updateFigures();
}

void updateFigures(void)
{
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient client;

    client.begin(server);
    int httpCode = client.GET();

    if (httpCode > 0) {
      String jsonRaw = client.getString();
      //Serial.println(jsonRaw);

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
      Serial.println("Timestamp: " + timestamp.substring(0,10));
      Serial.println();

      Heltec.display -> clear();
      Heltec.display -> drawString(0, 0, "First Dose: " + String(firstDose));
      Heltec.display -> drawString(0, 10, "Both Doses: " + String(secondDose));
      Heltec.display -> drawString(0, 20, "Unvaccinated: " + String(unvaccinated));
      Heltec.display -> drawString(0, 30, "Under 18: " + String(under18));
      Heltec.display -> drawString(0, 45, "Timestamp: " + timestamp.substring(0,10));
      Heltec.display -> display();

      plotStats(firstDose, secondDose, unvaccinated, under18);

    }
    else {
      Serial.println("HTTP Request Error");
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
    //leds[i] = CRGBW(0, 255, 255, 0);
  }
  for (int i = norm_first; i < norm_first + norm_both; i++) {
    leds[i] = CHSV(110, 255, 255);
    //leds[i] = CRGBW(0, 255, 0, 0);
  }
  for (int i = norm_first + norm_both; i < norm_first + norm_both + norm_unvac; i++) {
    leds[i] = CHSV(192, 255, 255);
    //leds[i] = CRGBW(255, 0, 255, 0);
  }
  for (int i = norm_first + norm_both + norm_unvac; i < norm_first + norm_both + norm_unvac + norm_underage; i++) {
    leds[i] = CHSV(0, 255, 255);
    //leds[i] = CRGBW(0, 0, 0, 255);
  }

  FastLED.show();
}

void rainbow() {
  static uint8_t hue;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV((i * 256 / NUM_LEDS) + hue, 255, 255);
  }
  FastLED.show();
  hue++;
}

void rainbowLoop() {
  long millisIn = millis();
  long loopTime = 5000; // 5 seconds

  while (millis() < millisIn + loopTime) {
    rainbow();
    delay(5);
  }
}

void WiFiConnect(void)
{
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.begin(WIFI_NAME, WIFI_PASSPHRASE);
  Serial.print("Connecting to WiFi");
  delay(1000);

  byte count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 20)
  {
    count ++;
    delay(500);
    Heltec.display -> drawString(0, 0, "Connecting...");
    Heltec.display -> display();
  }

  Heltec.display -> clear();
  if (WiFi.status() == WL_CONNECTED)
  {
    Heltec.display -> drawString(0, 0, "Connecting...OK.");
    Heltec.display -> display();
    delay(500);
  }
  else
  {
    Heltec.display -> clear();
    Heltec.display -> drawString(0, 0, "Connecting...Failed");
    Heltec.display -> display();
    while(1);
  }
  Heltec.display -> drawString(0, 10, "WIFI Setup done");
  Heltec.display -> display();
  delay(500);
}
