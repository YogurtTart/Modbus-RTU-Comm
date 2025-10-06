#pragma once
#include <Arduino.h>
#include <ModbusMaster.h>
#include <ArduinoJson.h>


#define MAX_SLAVES 10

// Unified struct for Modbus slave devices
struct ModbusDevice {
  uint8_t id;        // Slave ID
  uint16_t regStart; // Starting register
  uint16_t regCount; // Number of registers
  String name;       // Identifier for JSON
};

// Exposed globals
extern ModbusDevice slaves[MAX_SLAVES];
extern size_t slaveCount;

// Function prototypes
void setupModbus();                            
String querySlave(const ModbusDevice& slave);  
String queryAllSlaves();                       
void updateSlavesFromWeb(JsonArray arr);       // <-- add this here
