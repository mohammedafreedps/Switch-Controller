#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>

const char *ssid = "MO";
const char *password = "yespass1234";

const int pin4 = 4;               // GPIO4 (D2)
const int pin5 = 5;               // GPIO5 (D1)
const int INDICATOR_LED_PIN = 13; // D7
const int NUM_LEDS = 1;

const int lightSwitchPin = 14; // D5
const int fanSwitchPin = 12;   // D6
const int onboardLED = 2;

int currentHour = 0;
int currentMin = 0;
int alarmHour = 0;
int alarmMin = 0;

String fanStatus = "off";
String lightStatus = "off";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);

Adafruit_NeoPixel strip(NUM_LEDS, INDICATOR_LED_PIN, NEO_GRB + NEO_KHZ800);
WebSocketsServer webSocket = WebSocketsServer(81);

void updatePins()
{
    digitalWrite(pin4, fanStatus == "on" ? LOW : HIGH);
    digitalWrite(pin5, lightStatus == "on" ? LOW : HIGH);
}

void fadeColorLED(uint8_t r, uint8_t g, uint8_t b, int delayMs = 10)
{
    for (int brightness = 0; brightness <= 255; brightness += 5)
    {
        strip.setBrightness(brightness);
        strip.setPixelColor(0, strip.Color(r, g, b));
        strip.show();
        delay(delayMs);
    }
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

void sendState(int clientNum = -1) {
  JsonDocument res;
  res["type"] = "state";
  res["fan"] = fanStatus;
  res["light"] = lightStatus;
  res["hour"] = currentHour;
  res["minutes"] = currentMin;
  res["alarmHour"] = alarmHour;
  res["alarmMin"] = alarmMin;
  String response;
  serializeJson(res, response);

  if (clientNum == -1) {
    webSocket.broadcastTXT(response); // send to all clients
  } else {
    webSocket.sendTXT(clientNum, response);
  }
}


void sendAlarm(uint8_t num)
{
    JsonDocument res;
    res["type"] = "alarm";
    res["alarmHour"] = alarmHour;
    res["alarmMin"] = alarmMin;
    String response;
    serializeJson(res, response);
    webSocket.sendTXT(num, response);
}

void handleWebSocketMessage(uint8_t num, String message)
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, message);
    if (err)
        return;

    String type = doc["type"];

    if (type == "setState")
    {
        if (doc.containsKey("fan"))
            fanStatus = doc["fan"].as<String>();
        if (doc.containsKey("light"))
            lightStatus = doc["light"].as<String>();
        updatePins();
        sendState(num);
    }
    else if (type == "getState")
    {
        sendState(num);
    }
    else if (type == "setAlarm")
    {
        if (doc.containsKey("hour"))
            alarmHour = doc["hour"];
        if (doc.containsKey("minutes"))
            alarmMin = doc["minutes"];
        sendAlarm(num);
    }
    else if (type == "getAlarm")
    {
        sendAlarm(num);
    }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    if (type == WStype_TEXT)
    {
        handleWebSocketMessage(num, String((char *)payload));
    }
}

void setup()
{
    Serial.begin(9600);
    strip.begin();
    strip.show();
    strip.setBrightness(50);
    strip.setPixelColor(0, strip.Color(255, 0, 0));
    strip.show();

    // OTA setup
    ArduinoOTA.onStart([]()
                       {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
    else type = "filesystem";
    Serial.println("Start updating " + type); });

    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd OTA"); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100))); });

    ArduinoOTA.onError([](ota_error_t error)
                       {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

    ArduinoOTA.begin();
    Serial.println("OTA Ready");

    IPAddress local_IP(192, 168, 1, 101);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress primaryDNS(8, 8, 8, 8);
    WiFi.config(local_IP, gateway, subnet, primaryDNS);

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    pinMode(pin4, OUTPUT);
    pinMode(pin5, OUTPUT);
    pinMode(onboardLED, OUTPUT);
    digitalWrite(onboardLED, HIGH);
    digitalWrite(pin4, HIGH);
    digitalWrite(pin5, HIGH);

    while (WiFi.status() != WL_CONNECTED)
    {
        fadeColorLED(255, 0, 0);
        Serial.print(".");
    }

    Serial.println("\nConnected! IP: ");
    Serial.println(WiFi.localIP());

    strip.setPixelColor(0, strip.Color(0, 255, 0));
    strip.show();

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    timeClient.begin();
    digitalWrite(onboardLED, LOW);
}

bool lastLightSwitchState = digitalRead(lightSwitchPin);
bool lastFanSwitchState = digitalRead(fanSwitchPin);

void loop()
{
    webSocket.loop();
    timeClient.update();
    ArduinoOTA.handle();

    currentHour = timeClient.getHours();
    currentMin = timeClient.getMinutes();

    int ledBrightness = 5;
    if (currentHour >= 6 && currentHour < 9)
        ledBrightness = 100;
    else if (currentHour < 18)
        ledBrightness = 30;
    else if (currentHour < 22)
        ledBrightness = 10;
    else
        ledBrightness = 1;

    if (WiFi.status() != WL_CONNECTED)
    {
        fadeColorLED(255, 0, 0);
        strip.show();
    }
    else
    {
        if (fanStatus == "off" && lightStatus == "off")
        {
            strip.setBrightness(0);
        }
        else
        {
            strip.setPixelColor(0, hexToColor(0xB2C9AD));
            strip.setBrightness(ledBrightness);
        }
        strip.show();
    }

    bool currentLightSwitchState = digitalRead(lightSwitchPin);
    bool currentFanSwitchState = digitalRead(fanSwitchPin);

    if (currentLightSwitchState != lastLightSwitchState)
    {
        lightStatus = (lightStatus == "on") ? "off" : "on";
        digitalWrite(pin5, lightStatus == "on" ? LOW : HIGH);
        lastLightSwitchState = currentLightSwitchState;
        sendState(); // broadcast
    }

    if (currentFanSwitchState != lastFanSwitchState)
    {
        fanStatus = (fanStatus == "on") ? "off" : "on";
        digitalWrite(pin4, fanStatus == "on" ? LOW : HIGH);
        lastFanSwitchState = currentFanSwitchState;
        sendState(); // broadcast
    }

    if (currentHour == alarmHour && currentMin == alarmMin && alarmHour != 0 && alarmMin != 0)
    {
        lightStatus = "on";
        fanStatus = "off";
        updatePins();
    }
}
