// //working code


// #include <Arduino.h>
// #include <WiFi.h>
// #include <HomeSpan.h>
// #include <ArduinoOTA.h>
// #include <Adafruit_NeoPixel.h>


// // WiFi credentials
// const char *ssid = "MO";
// const char *password = "yespass1234";

// // Pins
// const int lightRelayPin = 22;
// const int fanRelayPin = 23;
// const int lightSwitchPin = 18;
// const int fanSwitchPin = 19;
// const int statusLEDPin = 2;

// // State tracking
// String lightStatus = "off";
// String fanStatus = "off";
// bool lastLightSwitchState;
// bool lastFanSwitchState;

// Adafruit_NeoPixel strip(1, 5, NEO_GRB + NEO_KHZ800);

// // HomeSpan characteristic pointers
// Characteristic::On *lightPower;
// Characteristic::On *fanPower;

// // ---- Accessory Info ----
// struct CombinedInfoService : Service::AccessoryInformation {
//   CombinedInfoService() {
//     new Characteristic::Name("ESP32 Light & Fan");
//     new Characteristic::Manufacturer("Your Name");
//     new Characteristic::SerialNumber("001");
//     new Characteristic::Model("ESP32-LF");
//     new Characteristic::FirmwareRevision("1.0");
//     new Characteristic::Identify();
//   }
// };

// // ---- Light Switch ----
// struct LightSwitchService : Service::Switch {
//   LightSwitchService() {
//     pinMode(lightRelayPin, OUTPUT);
//     lightPower = new Characteristic::On(false);
//   }

//   boolean update() override {
//     bool newState = lightPower->getNewVal();
//     lightStatus = newState ? "on" : "off";
//     digitalWrite(lightRelayPin, newState ? LOW : HIGH);  // active LOW relay
//     return true;
//   }
// };

// // ---- Fan Switch ----
// struct FanSwitchService : Service::Switch {
//   FanSwitchService() {
//     pinMode(fanRelayPin, OUTPUT);
//     fanPower = new Characteristic::On(false);
//   }

//   boolean update() override {
//     bool newState = fanPower->getNewVal();
//     fanStatus = newState ? "on" : "off";
//     digitalWrite(fanRelayPin, newState ? LOW : HIGH);
//     return true;
//   }
// };

// void fadeColorLED(uint8_t r, uint8_t g, uint8_t b, int delayMs = 10)
// {
//   // Fade in
//   for (int brightness = 0; brightness <= 255; brightness += 5)
//   {
//     strip.setBrightness(brightness);
//     strip.setPixelColor(0, strip.Color(r, g, b));
//     strip.show();
//     delay(delayMs);
//   }

//   // Fade out
//   for (int brightness = 255; brightness >= 0; brightness -= 5)
//   {
//     strip.setBrightness(brightness);
//     strip.setPixelColor(0, strip.Color(r, g, b));
//     strip.show();
//     delay(delayMs);
//   }
// }
// uint32_t hexToColor(uint32_t hex) {
//   uint8_t r = (hex >> 16) & 0xFF;
//   uint8_t g = (hex >> 8) & 0xFF;
//   uint8_t b = hex & 0xFF;
//   return strip.Color(r, g, b);
// }
// void setup() {
//   Serial.begin(9600);
//   strip.begin();
//   strip.show();
//   strip.setBrightness(50);
//   strip.setPixelColor(0, strip.Color(255, 0, 0));
//   strip.show();

//   pinMode(lightSwitchPin, INPUT_PULLUP);
//   pinMode(fanSwitchPin, INPUT_PULLUP);
//   pinMode(statusLEDPin, OUTPUT);
//   digitalWrite(statusLEDPin, HIGH);

//   lastLightSwitchState = digitalRead(lightSwitchPin);
//   lastFanSwitchState = digitalRead(fanSwitchPin);

//   IPAddress local_IP(192, 168, 1, 102);
//   IPAddress gateway(192, 168, 1, 1);
//   IPAddress subnet(255, 255, 255, 0);
//   IPAddress primaryDNS(8, 8, 8, 8);
//   IPAddress secondaryDNS(8, 8, 4, 4);

//   if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
//     Serial.println("⚠️ Failed to configure static IP");
//   }

//   WiFi.begin(ssid, password);
//   Serial.print("Connecting to WiFi ");
//   while (WiFi.status() != WL_CONNECTED) {
//     fadeColorLED(255,0,0);
//   }

//   Serial.println("\nWiFi connected!");
//   Serial.print("IP address: ");
//   Serial.println(WiFi.localIP());

//   ArduinoOTA.setHostname("ESP32-OTA");
//   ArduinoOTA.onStart([]() { Serial.println("\nOTA Update Started"); });
//   ArduinoOTA.onEnd([]() { Serial.println("\nOTA Update Finished"); });
//   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//     Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
//   });
//   ArduinoOTA.onError([](ota_error_t error) {
//     Serial.printf("\nOTA Error[%u]: ", error);
//     if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
//     else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
//     else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
//     else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
//     else if (error == OTA_END_ERROR) Serial.println("End Failed");
//   });

//   ArduinoOTA.begin();
//   Serial.println("OTA Ready - Waiting for updates...");
//   digitalWrite(statusLEDPin, LOW);  // LED ON = ready

//   // resetPairing();  // Uncomment to reset pairing

//   homeSpan.setPairingCode("48126347");
//   homeSpan.setLogLevel(1);
//   homeSpan.begin(Category::Bridges, "ESP32 Combo Device");

//   new SpanAccessory();
//   new CombinedInfoService();
//   new LightSwitchService();
//   new FanSwitchService();
// }

// // Global variables (add these at the top of your code)
// unsigned long previousBlinkMillis = 0;
// bool ledState = false;
// bool wasConnected = true;  // to detect WiFi state changes
// const unsigned long blinkInterval = 500;  // 500ms blink

// void loop() {
//   homeSpan.poll();
//   ArduinoOTA.handle();  // Always poll OTA too

//   // --- LIGHT SWITCH HANDLING ---
//   bool currentLightSwitch = digitalRead(lightSwitchPin);
//   if (currentLightSwitch != lastLightSwitchState) {
//     lastLightSwitchState = currentLightSwitch;
//     bool newState = !lightPower->getVal();
//     lightPower->setVal(newState, true);
//     digitalWrite(lightRelayPin, newState ? LOW : HIGH);
//     Serial.print("Light toggled from switch. New state: ");
//     Serial.println(newState ? "ON" : "OFF");
//   }

//   // --- FAN SWITCH HANDLING ---
//   bool currentFanSwitch = digitalRead(fanSwitchPin);
//   if (currentFanSwitch != lastFanSwitchState) {
//     lastFanSwitchState = currentFanSwitch;
//     bool newState = !fanPower->getVal();
//     fanPower->setVal(newState, true);
//     digitalWrite(fanRelayPin, newState ? LOW : HIGH);
//     Serial.print("Fan toggled from switch. New state: ");
//     Serial.println(newState ? "ON" : "OFF");
//   }

//   // --- STATUS LED HANDLING ---
//   bool isConnected = WiFi.status() == WL_CONNECTED;

//   if (!isConnected) {
//     // Non-blocking blinking red
//     unsigned long currentMillis = millis();
//     if (currentMillis - previousBlinkMillis >= blinkInterval) {
//       previousBlinkMillis = currentMillis;
//       ledState = !ledState;
//       strip.setPixelColor(0, ledState ? strip.Color(255, 0, 0) : 0);
//       strip.setBrightness(30);  // moderate brightness
//       strip.show();
//     }
//     wasConnected = false;
//   } else {
//     if (!wasConnected) {
//       // Just reconnected
//       wasConnected = true;
//       strip.setPixelColor(0, hexToColor(0xB2C9AD));
//       strip.setBrightness(1);
//       strip.show();
//     }

//     // --- Fade OFF if both relays are OFF ---
//     if (digitalRead(fanRelayPin) == HIGH && digitalRead(lightRelayPin) == HIGH) {
//       strip.setBrightness(100);
//       strip.show();
//     }
//   }
// }



