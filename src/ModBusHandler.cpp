#include "ModbusHandler.h"
#include "MQTTHandler.h"   // publish
#include <Arduino.h>

// RS485 DE/RE pin
#define MAX485_DE 5

// RS485 control
void preTransmission()  { digitalWrite(MAX485_DE, HIGH); }
void postTransmission() { digitalWrite(MAX485_DE, LOW); }

ModbusMaster node;

// Convert Modbus register to signed temperature
float convertRegisterToTemperature(uint16_t regVal) {
  int16_t tempInt;
  if (regVal & 0x8000) tempInt = -((0xFFFF - regVal) + 1);
  else tempInt = regVal;
  return tempInt * 0.1;
}

// Convert Modbus register to humidity
float convertRegisterToHumidity(uint16_t regVal) {
  return regVal * 0.1;
}

void setupModbus() {
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_DE, LOW);

  node.begin(1, Serial); // Slave ID = 1
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

// ----------------- READING ONLY -----------------
String readModbusJSON() {
  const uint16_t START_ADDRESS = 0;
  const uint16_t NUM_REGISTERS = 2;

  // Query Modbus (function code 04: read input registers)
  uint8_t result = node.readInputRegisters(START_ADDRESS, NUM_REGISTERS);

  if (result == node.ku8MBSuccess) {
    // ✅ Successfully received data
    float temperature = convertRegisterToTemperature(node.getResponseBuffer(0));
    float humidity    = convertRegisterToHumidity(node.getResponseBuffer(1));

    // Build JSON
    String payload = "{";
    payload += "\"temperature\":" + String(temperature, 1) + ",";
    payload += "\"humidity\":" + String(humidity, 1);
    payload += "}";

    Serial.println("JSON Built: " + payload);
    return payload;

  } else {
    // ❌ Error occurred
    String err = "{\"error\":\"Modbus error: 0x" + String(result, HEX) + "\"}";
    Serial.println(err);
    return err;
  }
}
