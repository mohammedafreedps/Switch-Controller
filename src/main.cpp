#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <Adafruit_NeoPixel.h>

const char *ssid = "MO";
const char *password = "yespass1234";

const int pin4 = 4;               // GPIO4 (D2)
const int pin5 = 5;               // GPIO5 (D1)
const int INDICATOR_LED_PIN = 13; // GPIO13 (D7)
const int NUM_LEDS = 1;           // Number of LEDs

const int lightSwitchPin = 14; // D5
const int fanSwitchPin = 12;   // D6

const int onboardLED = 2;

int currentHour = 0;
int currentminuts = 0;

int alarmHour = 0;
int alarmMinuts = 0;

ESP8266WebServer server(80);

String fanStatus = "off";
String lightStatus = "off";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);

Adafruit_NeoPixel strip(NUM_LEDS, INDICATOR_LED_PIN, NEO_GRB + NEO_KHZ800);

// Function to send CORS headers
void sendCorsHeaders()
{
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

// Function to update GPIO pins based on fan/light status
void updatePins()
{
  digitalWrite(pin4, fanStatus == "on" ? LOW : HIGH);
  digitalWrite(pin5, lightStatus == "on" ? LOW : HIGH);
}

void handleFanOn()
{
  fanStatus = "on";

  JsonDocument res;
  res["status"] = "sucsuss";
  String response;
  serializeJson(res, response);
  server.send(200, "application/json", response);
  updatePins();
}

void handleFanOff()
{
  fanStatus = "off";
  JsonDocument res;
  res["status"] = "sucsuss";
  String response;
  serializeJson(res, response);
  server.send(200, "application/json", response);
  updatePins();
}

void handleLightOn()
{
  lightStatus = "on";
  JsonDocument res;
  res["status"] = "sucsuss";
  String response;
  serializeJson(res, response);
  server.send(200, "application/json", response);
  updatePins();
}

void handleLightOff()
{
  lightStatus = "off";
  JsonDocument res;
  res["status"] = "sucsuss";
  String response;
  serializeJson(res, response);
  server.send(200, "application/json", response);
  updatePins();
}

void handleAlarmDetail()
{
  JsonDocument res;
  res["hour"] = alarmHour;
  res["minnuts"] = alarmMinuts;
  String response;
  serializeJson(res, response);
  server.send(200, "application/json", response);
}

void handleSetAlarmDetail()
{
  sendCorsHeaders();

  if (server.hasArg("plain"))
  {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error)
    {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
      return;
    }

    // Extract alarm details from JSON
    if (doc["hour"].is<int>() && doc["minuts"].is<int>())
    {
      alarmHour = doc["hour"];
      alarmMinuts = doc["minuts"];

      JsonDocument res;
      res["status"] = "success";
      res["alarmHour"] = alarmHour;
      res["alarmMinuts"] = alarmMinuts;
      String response;
      serializeJson(res, response);
      server.send(200, "application/json", response);
    }
    else
    {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing hour or minutes\"}");
    }
  }
  else
  {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No JSON body\"}");
  }
}

// Handle GET request
void handleGet()
{
  sendCorsHeaders();

  JsonDocument doc; // use JsonDocument instead of DynamicJsonDocument
  doc["status"] = "succuss";
  doc["lightStatus"] = lightStatus;
  doc["fanStatus"] = fanStatus;
  doc["Hour"] = currentHour;
  doc["Minuts"] = currentminuts;
  doc["alarmHour"] = alarmHour;
  doc["alrmMinuts"] = alarmMinuts;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Handle POST request
void handlePost()
{
  sendCorsHeaders();

  if (server.hasArg("plain"))
  {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error)
    {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
      return;
    }

    fanStatus = doc["fan"].as<String>();
    lightStatus = doc["light"].as<String>();
    alarmHour = doc["h"];
    alarmMinuts = doc["m"];

    updatePins();

    JsonDocument res;
    res["status"] = "success";
    res["fan"] = fanStatus;
    res["light"] = lightStatus;
    res["alarm hour"] = alarmHour;
    res["alarm minuts"] = alarmMinuts;

    String response;
    serializeJson(res, response);
    server.send(200, "application/json", response);
  }
  else
  {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No JSON body\"}");
  }
}

void fadeColorLED(uint8_t r, uint8_t g, uint8_t b, int delayMs = 10)
{
  // Fade in
  for (int brightness = 0; brightness <= 255; brightness += 5)
  {
    strip.setBrightness(brightness);
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
    delay(delayMs);
  }

  // Fade out
  for (int brightness = 255; brightness >= 0; brightness -= 5)
  {
    strip.setBrightness(brightness);
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
    delay(delayMs);
  }
}

uint32_t hexToColor(uint32_t hex)
{
  uint8_t r = (hex >> 16) & 0xFF;
  uint8_t g = (hex >> 8) & 0xFF;
  uint8_t b = hex & 0xFF;
  return strip.Color(r, g, b);
}

void setup()
{
  Serial.begin(9600);
  // Static IP configuration
  strip.begin();
  strip.show();
  strip.setBrightness(50);
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();

  IPAddress local_IP(192, 168, 1, 101); // Your desired static IP
  IPAddress gateway(192, 168, 1, 1);    // Your router's gateway IP
  IPAddress subnet(255, 255, 255, 0);   // Subnet mask
  IPAddress primaryDNS(8, 8, 8, 8);     // Google DNS (or your choice)

  // Attempt to configure static IP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS))
  {
    Serial.println("Static IP configuration failed!");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  pinMode(pin4, OUTPUT);
  pinMode(pin5, OUTPUT);

  digitalWrite(pin4, HIGH);
  digitalWrite(pin5, HIGH);

  pinMode(onboardLED, OUTPUT);
  digitalWrite(onboardLED, HIGH);

  while (WiFi.status() != WL_CONNECTED)
  {
    fadeColorLED(255, 0, 0);
    Serial.print(".");
  }

  ArduinoOTA.setHostname("esp8266-ota");
  ArduinoOTA.begin();

  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();

  server.on("/data", HTTP_GET, handleGet);
  server.on("/data", HTTP_POST, handlePost);
  server.on("/fanOn", HTTP_GET, handleFanOn);
  server.on("/fanOff", HTTP_GET, handleFanOff);
  server.on("/lightOn", HTTP_GET, handleLightOn);
  server.on("/lightOff", HTTP_GET, handleLightOff);
  server.on("/getAlarmDetails", HTTP_GET, handleAlarmDetail);
  server.on("/setAlarmDetails", HTTP_POST, handleSetAlarmDetail);

  // CORS Preflight support (OPTIONS request)
  server.on("/data", HTTP_OPTIONS, []()
            {
              sendCorsHeaders();
              server.send(204); // No Content
            });
  server.begin();
  digitalWrite(onboardLED, LOW);
  Serial.println("HTTP server started");
  timeClient.begin();
}

bool lastLightSwitchState = digitalRead(lightSwitchPin);
bool lastFanSwitchState = digitalRead(fanSwitchPin);

void loop()
{
  server.handleClient();
  ArduinoOTA.handle();
  timeClient.update();
  // Read current switch state
  bool currentLightSwitchState = digitalRead(lightSwitchPin);
  bool currentFanSwitchState = digitalRead(fanSwitchPin);

  int ledBrightness = 5; // default

  if (currentHour >= 6 && currentHour < 9)
  {
    ledBrightness = 100; // Morning boost
  }
  else if (currentHour >= 9 && currentHour < 18)
  {
    ledBrightness = 30; // Daytime
  }
  else if (currentHour >= 18 && currentHour < 22)
  {
    ledBrightness = 10; // Evening
  }
  else
  {
    ledBrightness = 1; // Night / Bedtime
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    // WiFi disconnected → turn LED red
    fadeColorLED(255, 0, 0);
    strip.show();
  }
  else
  {
    strip.setPixelColor(0, hexToColor(0xB2C9AD));
    strip.setBrightness(ledBrightness);
    strip.show();
  }

  // If light switch changed position
  if (currentLightSwitchState != lastLightSwitchState)
  {
    if (lightStatus == "off")
    {
      lightStatus = "on";
      digitalWrite(pin5, LOW); // Turn ON
    }
    else
    {
      lightStatus = "off";
      digitalWrite(pin5, HIGH); // Turn OFF
    }
    lastLightSwitchState = currentLightSwitchState;
  }

  // If fan switch changed position
  if (currentFanSwitchState != lastFanSwitchState)
  {
    if (fanStatus == "off")
    {
      fanStatus = "on";
      digitalWrite(pin4, LOW); // Turn ON
    }
    else
    {
      fanStatus = "off";
      digitalWrite(pin4, HIGH); // Turn OFF
    }
    lastFanSwitchState = currentFanSwitchState;
  }

  currentHour = timeClient.getHours();

  currentminuts = timeClient.getMinutes();
  if (currentHour == alarmHour && currentminuts == alarmMinuts && alarmHour != 0 && alarmMinuts != 0)
  {
    // Set light ON
    lightStatus = "on";
    digitalWrite(pin5, LOW);

    // Set fan OFF
    fanStatus = "off";
    digitalWrite(pin4, HIGH);
  }
}