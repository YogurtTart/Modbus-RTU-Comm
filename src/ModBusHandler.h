#pragma once
#include <ModbusMaster.h>
#include <ArduinoJson.h>

#define MAX_SLAVES 10

enum QueryState { 
  Q_IDLE, 
  Q_QUERYING, 
  Q_COMPLETE, 
  Q_ERROR 
};

struct ModbusSlave {
  uint8_t id;
  uint16_t startReg;
  uint16_t numRegs;
  String name;
};

extern ModbusSlave slaves[MAX_SLAVES];
extern uint8_t slaveCount;

// Non-blocking query variables
extern QueryState queryState;
extern uint8_t currentQueryIndex;
extern unsigned long queryStartTime;
extern JsonDocument queryData;

// Function declarations
void setupModbus();
bool startNonBlockingQuery(ModbusSlave* slaves, uint8_t slaveCount);
bool continueNonBlockingQuery(ModbusSlave* slaves, uint8_t slaveCount);
String getQueryResults();
void resetQueryState();
bool querySingleSlave(const ModbusSlave& slave, JsonObject& resultObj);
float convertRegisterToTemperature(uint16_t regVal);
float convertRegisterToHumidity(uint16_t regVal);