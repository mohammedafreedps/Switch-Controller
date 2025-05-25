// #include <Arduino.h>
// #include <nvs_flash.h>

// void factoryReset() {
//   Serial.println("🧨 Erasing NVS (Flash)...");

//   // De-initialize NVS first
//   nvs_flash_deinit();
  
//   // Erase entire flash partition
//   esp_err_t result = nvs_flash_erase();
  
//   if (result == ESP_OK) {
//     Serial.println("✅ NVS erased successfully!");
//   } else {
//     Serial.print("❌ NVS erase failed: ");
//     Serial.println(result);
//   }

//   delay(1000);
//   Serial.println("🔁 Rebooting...");
//   ESP.restart();  // Reboot device (RAM cleared as well)
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);
//   factoryReset();  // ⚠️ This will wipe memory and reboot immediately
// }

// void loop() {
//   // Nothing here - will reboot after reset
// }
