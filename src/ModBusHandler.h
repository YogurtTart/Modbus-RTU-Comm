#pragma once

#include <ModbusMaster.h>

extern float latestTemperature;
extern float latestHumidity;
extern bool modbusOK;

// Function prototypes
void setupModbus();       // Configure RS485 + Modbus
String readModbusJSON();    // Read registers and send to MQTT
