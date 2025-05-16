#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

const char *ssid = "MO";
const char *password = "yespass1234";

const int pin4 = 4; // GPIO4 (D2)
const int pin5 = 5; // GPIO5 (D1)

const int lightSwitchPin = 14;  // D5
const int fanSwitchPin = 12;  // D6

ESP8266WebServer server(80);

String fanStatus = "off";
String lightStatus = "off";

// Function to send CORS headers
void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

// Function to update GPIO pins based on fan/light status
void updatePins() {
  digitalWrite(pin4, fanStatus == "on" ? LOW : HIGH);
  digitalWrite(pin5, lightStatus == "on" ? LOW : HIGH);
}

// Handle GET request
void handleGet() {
  sendCorsHeaders();

  JsonDocument doc;  // use JsonDocument instead of DynamicJsonDocument
  doc["lightStatus"] = lightStatus;
  doc["fanStatus"] = fanStatus;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// Handle POST request
void handlePost() {
  sendCorsHeaders();

  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
      return;
    }

    fanStatus = doc["fan"].as<String>();
    lightStatus = doc["light"].as<String>();

    updatePins();

    JsonDocument res;
    res["status"] = "success";
    res["fan"] = fanStatus;
    res["light"] = lightStatus;

    String response;
    serializeJson(res, response);
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No JSON body\"}");
  }
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  pinMode(pin4, OUTPUT);
  pinMode(pin5, OUTPUT);

  digitalWrite(pin4, HIGH);
  digitalWrite(pin5, HIGH);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/data", HTTP_GET, handleGet);
  server.on("/data", HTTP_POST, handlePost);

  // CORS Preflight support (OPTIONS request)
  server.on("/data", HTTP_OPTIONS, []() {
    sendCorsHeaders();
    server.send(204); // No Content
  });

  server.begin();
  Serial.println("HTTP server started");
}

bool lastLightSwitchState = digitalRead(lightSwitchPin);
bool lastFanSwitchState = digitalRead(fanSwitchPin);

void loop() {
  server.handleClient();

  // Read current switch state
  bool currentLightSwitchState = digitalRead(lightSwitchPin);
  bool currentFanSwitchState = digitalRead(fanSwitchPin);

  // If light switch changed position
  if (currentLightSwitchState != lastLightSwitchState) {
    if (lightStatus == "off") {
      lightStatus = "on";
      digitalWrite(pin5, LOW);  // Turn ON
    } else {
      lightStatus = "off";
      digitalWrite(pin5, HIGH); // Turn OFF
    }
    lastLightSwitchState = currentLightSwitchState;
  }

  // If fan switch changed position
  if (currentFanSwitchState != lastFanSwitchState) {
    if (fanStatus == "off") {
      fanStatus = "on";
      digitalWrite(pin4, LOW);  // Turn ON
    } else {
      fanStatus = "off";
      digitalWrite(pin4, HIGH); // Turn OFF
    }
    lastFanSwitchState = currentFanSwitchState;
  }
}