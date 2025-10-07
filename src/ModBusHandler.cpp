#include "ModBusHandler.h"
#include "MQTTHandler.h"
#include <Arduino.h>
#include <ArduinoJson.h>

ModbusSlave slaves[MAX_SLAVES];
uint8_t slaveCount = 0;

// RS485 DE/RE pin
#define MAX485_DE 5

#define SLAVE_TIMEOUT_MS 3000  // 5 seconds per slave
#define TOTAL_QUERY_TIMEOUT_MS 30000  // 30 seconds total

// RS485 control
void preTransmission()  { digitalWrite(MAX485_DE, HIGH); }
void postTransmission() { digitalWrite(MAX485_DE, LOW); }

ModbusMaster node;

// Non-blocking query variables
QueryState queryState = Q_IDLE;
uint8_t currentQueryIndex = 0;
unsigned long queryStartTime = 0;
JsonDocument queryData;

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
    Serial.println("✅ Modbus pins initialized");
}

// ----------------- NON-BLOCKING MULTI-SLAVE QUERY -----------------

uint8_t readModbusRegisters(uint8_t slaveID, uint16_t startReg, uint16_t numRegs) {
    static ModbusMaster node;
    static uint8_t lastSlaveID = 0;
    
    // Serial.print("🔧 readModbusRegisters - Slave: ");
    // Serial.print(slaveID);
    // Serial.print(", Reg: ");
    // Serial.print(startReg);
    // Serial.print(", Count: ");
    // Serial.println(numRegs);
    
    // Re-initialize only when slave ID changes
    if (slaveID != lastSlaveID) {
        Serial.print("🔄 Initializing Modbus for Slave ID: ");
        Serial.println(slaveID);
        node.begin(1, Serial);
        node.preTransmission(preTransmission);
        node.postTransmission(postTransmission);
        lastSlaveID = slaveID;
        delay(50); // Increased delay for stabilization
    }
    
    node.clearResponseBuffer();
    
    // Serial.println("   Sending Modbus request...");
    // unsigned long startTime = millis();
    // uint8_t result = node.readInputRegisters(startReg, numRegs);
    // unsigned long endTime = millis();
    
    // Serial.print("   Modbus transaction took ");
    // Serial.print(endTime - startTime);
    // Serial.print("ms, Result: 0x");
    // Serial.println(result, HEX);
    
    // // Enhanced error decoding
    // if (result != node.ku8MBSuccess) {
    //     Serial.print("   ❌ MODBUS ERROR: ");
    //     switch(result) {
    //         case node.ku8MBResponseTimedOut:
    //             Serial.println("Response Timeout (0xE2) - No response from slave");
    //             Serial.println("   → Check: Wiring, Power, Slave ID, Baud rate");
    //             break;
    //         case node.ku8MBInvalidSlaveID:
    //             Serial.println("Invalid Slave ID (0xE0)");
    //             break;
    //         case node.ku8MBIllegalDataAddress:
    //             Serial.println("Illegal Data Address (0x02) - Wrong register");
    //             break;
    //         case node.ku8MBIllegalFunction:
    //             Serial.println("Illegal Function (0x01) - Try holding registers");
    //             break;
    //         case node.ku8MBSlaveDeviceFailure:
    //             Serial.println("Slave Device Failure (0x04)");
    //             break;
    //         case node.ku8MBInvalidCRC:
    //             Serial.println("Invalid CRC (0xE3)");
    //             break;
    //         default:
    //             Serial.print("Unknown error: 0x");
    //             Serial.println(result, HEX);
    //             break;
    //     }
    // } else {
    //     Serial.println("   ✅ Modbus request sent successfully");
    // }
    
    return node.readInputRegisters(0, 2);
}

// Query a single slave and return success status
bool querySingleSlave(const ModbusSlave& slave, JsonObject& resultObj) {

    // Serial.println();
    // Serial.print("🎯 STARTING querySingleSlave for: ");
    // Serial.print(slave.name);
    // Serial.print(" (ID: ");
    // Serial.print(slave.id);
    // Serial.print(")");
    // Serial.print(" - Reading registers ");
    // Serial.print(slave.startReg);
    // Serial.print(" to ");
    // Serial.println(slave.startReg + slave.numRegs - 1);
  
    uint8_t result = readModbusRegisters(slave.id, slave.startReg, slave.numRegs);

    resultObj["id"] = slave.id;
    resultObj["name"] = slave.name;
    resultObj["startReg"] = slave.startReg;
    resultObj["numRegs"] = slave.numRegs;

    if (result == node.ku8MBSuccess) {
        Serial.println("✅ MODBUS SUCCESS - Processing response data:");
        
        // Successfully received data
        if (slave.numRegs >= 1) {
            uint16_t rawTemp = node.getResponseBuffer(0);
            float temperature = convertRegisterToTemperature(rawTemp);
            resultObj["temperature"] = temperature;
            // Serial.print("   Temperature: Raw=0x");
            // Serial.print(rawTemp, HEX);
            // Serial.print(" (");
            // Serial.print(rawTemp);
            // Serial.print(") → ");
            // Serial.print(temperature);
            // Serial.println("°C");
        }
        if (slave.numRegs >= 2) {
            uint16_t rawHum = node.getResponseBuffer(1);
            float humidity = convertRegisterToHumidity(rawHum);
            resultObj["humidity"] = humidity;
            // Serial.print("   Humidity: Raw=0x");
            // Serial.print(rawHum, HEX);
            // Serial.print(" (");
            // Serial.print(rawHum);
            // Serial.print(") → ");
            // Serial.print(humidity);
            // Serial.println("%");
        }
        
        // Add any additional registers
        for (uint16_t i = 2; i < slave.numRegs; i++) {
            String regName = "reg" + String(i);
            uint16_t regValue = node.getResponseBuffer(i);
            resultObj[regName] = regValue;
            // Serial.print("   Register ");
            // Serial.print(i);
            // Serial.print(": 0x");
            // Serial.print(regValue, HEX);
            // Serial.print(" (");
            // Serial.print(regValue);
            // Serial.println(")");
        }
        
        // Serial.print("📊 Slave ");
        // Serial.print(slave.id);
        // Serial.print(" (");
        // Serial.print(slave.name);
        // Serial.println(") completed successfully");
        
        return true;
    } else {
        // Error occurred
        resultObj["error"] = "0x" + String(result, HEX);
        Serial.print("❌ Slave ");
        Serial.print(slave.id);
        Serial.print(" (");
        Serial.print(slave.name);
        Serial.print(") FAILED with error: 0x");
        Serial.println(result, HEX);
        return false;
    }
}

// Start non-blocking query of all slaves
bool startNonBlockingQuery(ModbusSlave* slaves, uint8_t slaveCount) {
  if (queryState == Q_IDLE && slaveCount > 0) {
    queryState = Q_QUERYING;
    currentQueryIndex = 0;
    queryStartTime = millis();
    queryData.clear();
    JsonArray arr = queryData.to<JsonArray>();
    
    Serial.println("🚀 === STARTING NON-BLOCKING SLAVE QUERY ===");
    Serial.print("📋 Number of slaves to query: ");
    Serial.println(slaveCount);
    
    // // Debug: Show all slaves that will be queried
    // for (uint8_t i = 0; i < slaveCount; i++) {
    //     Serial.print("   Slave ");
    //     Serial.print(i);
    //     Serial.print(": ID=");
    //     Serial.print(slaves[i].id);
    //     Serial.print(", Name='");
    //     Serial.print(slaves[i].name);
    //     Serial.print("', Reg=");
    //     Serial.print(slaves[i].startReg);
    //     Serial.print(", Count=");
    //     Serial.println(slaves[i].numRegs);
    // }
    
    return true;
  }
  return false;
}

bool continueNonBlockingQuery(ModbusSlave* slaves, uint8_t slaveCount) {
  if (queryState != Q_QUERYING) return false;
  
  // Overall timeout check
  if (millis() - queryStartTime > TOTAL_QUERY_TIMEOUT_MS) {
    queryState = Q_ERROR;
    Serial.println("⏰ !!! QUERY TIMEOUT - STOPPING EARLY !!!");
    return true;
  }
  
  static unsigned long slaveStartTime = 0;
  static bool slaveInProgress = false;
  
  // START new slave query
  if (!slaveInProgress) {
    if (currentQueryIndex < slaveCount) {
      slaveInProgress = true;
      slaveStartTime = millis();
      
      // Serial.println();
      // Serial.print("🔍 Querying slave [");
      // Serial.print(currentQueryIndex + 1);
      // Serial.print("/");
      // Serial.print(slaveCount);
      // Serial.print("]: ");
      // Serial.print(slaves[currentQueryIndex].name);
      // Serial.print(" (ID: ");
      // Serial.print(slaves[currentQueryIndex].id);
      // Serial.println(")");
      
    } else {
      // All slaves processed
      queryState = Q_COMPLETE;
      Serial.println("🎉 === QUERY COMPLETED ===");
      return true;
    }
  }
  
  // CHECK timeout for current slave
  if (slaveInProgress && (millis() - slaveStartTime > SLAVE_TIMEOUT_MS)) {
    // Serial.print("⏰ !!! Slave ");
    // Serial.print(slaves[currentQueryIndex].id);
    // Serial.print(" (");
    // Serial.print(slaves[currentQueryIndex].name);
    // Serial.println(") timeout - skipping !!!");
    
    JsonObject obj = queryData.add<JsonObject>();
    obj["id"] = slaves[currentQueryIndex].id;
    obj["name"] = slaves[currentQueryIndex].name;
    obj["error"] = "timeout";
    
    slaveInProgress = false;
    currentQueryIndex++;
    
    // Check if this was the last slave
    if (currentQueryIndex >= slaveCount) {
      queryState = Q_COMPLETE;
      return true;
    }
    return false;
  }
  
  // PROCESS current slave (if no timeout)
  if (slaveInProgress && currentQueryIndex < slaveCount) {
    JsonObject obj = queryData.add<JsonObject>();
    bool success = querySingleSlave(slaves[currentQueryIndex], obj);
    
    if (success) {
      Serial.print("✅ Slave ");
      Serial.print(slaves[currentQueryIndex].id);
      Serial.println(" completed successfully");
    } else {
      Serial.print("❌ Slave ");
      Serial.print(slaves[currentQueryIndex].id);
      Serial.println(" failed");
    }
    
    slaveInProgress = false;
    currentQueryIndex++;
    
    // Check if all slaves are done
    if (currentQueryIndex >= slaveCount) {
      queryState = Q_COMPLETE;
      Serial.println("🎉 === NON-BLOCKING QUERY COMPLETED ===");
      Serial.print("📊 Total slaves processed: ");
      Serial.println(slaveCount);
      return true;
    }
  }
  
  return false; // Still processing
}

// Get query results as JSON string
String getQueryResults() {
  String output;
  serializeJson(queryData, output);
  
  Serial.println("📄 === QUERY RESULTS ===");
  serializeJsonPretty(queryData, Serial);
  Serial.println();
  Serial.println("=====================");
  
  return output;
}

// Reset query state for next poll
void resetQueryState() {
  queryState = Q_IDLE;
  currentQueryIndex = 0;
  Serial.println("🔄 Query state reset to Q_IDLE");
}