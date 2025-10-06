#include "ModbusHandler.h"
#include "MQTTHandler.h"
#include <Arduino.h>

#define MAX485_DE 5

// RS485 DE/RE control
void preTransmission()  { digitalWrite(MAX485_DE, HIGH); }
void postTransmission() { digitalWrite(MAX485_DE, LOW); }

ModbusMaster node;

// =================== SLAVE LIST ===================
// Start with empty array, WebServer can add/remove later
ModbusDevice slaves[MAX_SLAVES];
size_t slaveCount = 0;

// =================== HELPERS ===================
float convertRegisterToTemperature(uint16_t regVal) {
  int16_t tempInt;
  if (regVal & 0x8000) tempInt = -((0xFFFF - regVal) + 1);
  else tempInt = regVal;
  return tempInt * 0.1;
}

float convertRegisterToHumidity(uint16_t regVal) {
  return regVal * 0.1;
}

// =================== SETUP ===================
void setupModbus() {
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_DE, LOW);

  // Initialize Modbus with dummy slave, will change dynamically
  node.begin(1, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

}

// =================== QUERY ONE SLAVE ===================
String querySlave(const ModbusDevice& sc) {
  node.begin(sc.id, Serial);
  uint8_t result = node.readInputRegisters(sc.regStart, sc.regCount);

  if (result == node.ku8MBSuccess) {
    float temp = convertRegisterToTemperature(node.getResponseBuffer(0));
    float hum  = convertRegisterToHumidity(node.getResponseBuffer(1));

    String payload = "{";
    payload += "\"id\":" + String(sc.id) + ",";
    payload += "\"name\":\"" + sc.name + "\",";
    payload += "\"temperature\":" + String(temp, 1) + ",";
    payload += "\"humidity\":" + String(hum, 1);
    payload += "}";
    return payload;
  } else {
    return "{\"id\":" + String(sc.id) + ",\"error\":\"0x" + String(result, HEX) + "\"}";
  }
}

// =================== QUERY ALL SLAVES ===================
String queryAllSlaves() {
  String allPayload = "[";
  for (size_t i = 0; i < slaveCount; i++) {
    allPayload += querySlave(slaves[i]);
    if (i < slaveCount - 1) allPayload += ",";
  }
  allPayload += "]";
  return allPayload;
}

void updateSlavesFromWeb(JsonArray arr) {
  slaveCount = 0;
  for (JsonObject obj : arr) {
    if (slaveCount < MAX_SLAVES) {
      slaves[slaveCount].id       = obj["id"] | 1;       // default ID=1
      slaves[slaveCount].regStart = obj["regStart"] | 0; // default start
      slaves[slaveCount].regCount = obj["regCount"] | 2; // default 2 regs
      slaves[slaveCount].name     = obj["name"] | "slave";
      slaveCount++;
    }
  }
}
