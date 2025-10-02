#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include "WiFiHandler.h"
#include "MQTTHandler.h"
#include "ModbusHandler.h"

// Timer for periodic Modbus polling
unsigned long previousMillis = 0;
const unsigned long interval = 3000; // 3 seconds

void setup() {
  Serial.begin(9600);

  // ----------------- Setup Wi-Fi -----------------
  setupWiFi();                                // Connect to Wi-Fi
  mqttClient.setServer(mqttServer, mqttPort); // MQTT server setup

  // ----------------- Setup Modbus -----------------
  setupModbus(); // RS485 & Modbus config

  // ----------------- Setup OTA -----------------
  ArduinoOTA.begin(); // Enables firmware updates via Wi-Fi

  Serial.println("ESP8266 Modbus RTU Master Started");
}

void loop() {
  // ----------------- Keep Wi-Fi Alive -----------------
  checkWiFi();

  // ----------------- Keep MQTT Alive -----------------
  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop();

  // ----------------- Poll Modbus Every 3s and Publish -----------------
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    String json = readModbusJSON(); // Get temp + humidity
    publishMessage(mqttTopicPub, json.c_str());  // send to MQTT
  }

  // ----------------- Handle OTA Updates -----------------
  if(otaInitialized){
    ArduinoOTA.handle();
  }
}
