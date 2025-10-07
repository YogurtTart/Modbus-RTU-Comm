#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include "WiFiHandler.h"
#include "MQTTHandler.h"
#include <LittleFS.h> 
#include "ModbusHandler.h"    // ✅ This defines ModbusSlave struct
#include "WebServerHandler.h" // ✅ This uses the shared struct

// Timer for periodic Modbus polling
unsigned long previousMillis = 0;
const unsigned long interval = 3000; // 3 seconds

void setup() {
  Serial.begin(9600, SERIAL_8N1);

  Serial.println("Mounting LittleFS...");
  if (!LittleFS.begin()) {
    Serial.println("❌ LittleFS mount failed!");
  } else {
    Serial.println("✅ LittleFS mounted successfully");
  }

  // ----------------- Setup Wi-Fi -----------------
  setupWiFi();
  mqttClient.setServer(mqttServer, mqttPort);

  // ----------------- Setup Modbus -----------------
  setupModbus();

  // ----------------- Setup Web Server -----------------
  setupWebServer();  // ✅ Now this will use the shared ModbusSlave struct

  // ----------------- Setup OTA -----------------
  ArduinoOTA.begin();

  Serial.println("ESP8266 Modbus RTU Master with Web Server Started");
}

void loop() {
  // ----------------- Keep Wi-Fi Alive -----------------
  checkWiFi();

  // ----------------- Keep MQTT Alive -----------------
  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop();

  // ----------------- Handle Web Server -----------------
  handleWebServer();

  // ----------------- Process Pending Saves -----------------
  processPendingSaves();

  // ----------------- Handle Manual Queries -----------------
  if (shouldQuerySlaves) {
    shouldQuerySlaves = false;
    // Start non-blocking query using the shared slaves array
    if (startNonBlockingQuery(slaves, slaveCount)) {
      Serial.println("Manual query started");
    }
  }

  // ----------------- Continue Non-Blocking Queries -----------------
  if (queryState == Q_QUERYING) {
    if (continueNonBlockingQuery(slaves, slaveCount)) {
      // Query completed
      String results = getQueryResults();
      publishMessage(mqttTopicPub, results.c_str());
      resetQueryState();
    }
  }

  // ----------------- Periodic Auto Polling -----------------
  // unsigned long currentMillis = millis();
  // if (currentMillis - previousMillis >= interval && slaveCount > 0) {
  //   previousMillis = currentMillis;
    
  //   // Start non-blocking auto-query using the shared slaves array
  //   if (queryState == Q_IDLE) {
  //     startNonBlockingQuery(slaves, slaveCount);
  //   }
  // }

  // ----------------- Handle OTA Updates -----------------
  if(otaInitialized){
    ArduinoOTA.handle();
  }
}