#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include "WiFiHandler.h"
#include "MQTTHandler.h"
#include "ModbusHandler.h"
#include "WebServerHandler.h"

// ----------------- Global Timing -----------------
unsigned long previousMillis = 0;
const unsigned long interval = 3000; // Poll Modbus every 3s

void setup() {
  Serial.begin(9600);
  Serial.println("\n--- ESP8266 Modbus RTU Master ---");

  // Wi-Fi & MQTT
  setupWiFi();
  mqttClient.setServer(mqttServer, mqttPort);

  // Modbus (RS485)
  setupModbus();

  // Web server (slave config management)
  setupWebServer();

  // OTA (firmware update via Wi-Fi)
  ArduinoOTA.begin();
}

void loop() {
  // Maintain connections
  checkWiFi();
  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop();

  // Handle HTTP requests
  handleWebServer();

  // Poll Modbus slaves periodically and publish results
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    String json = queryAllSlaves();             // Collect sensor data
    publishMessage(mqttTopicPub, json.c_str()); // Send to MQTT
  }

  // Handle OTA updates
  if (otaInitialized) ArduinoOTA.handle();
}
