#include "ModbusHandler.h"
#include "MQTTHandler.h"
#include <Arduino.h>
#include <ArduinoJson.h>

#define MAX485_DE 5

// ---------------- RS485 DE/RE Control ----------------
void preTransmission()  { digitalWrite(MAX485_DE, HIGH); }
void postTransmission() { digitalWrite(MAX485_DE, LOW); }

ModbusMaster node;

// ---------------- SLAVE LIST ----------------
ModbusDevice slaves[MAX_SLAVES];
size_t slaveCount = 0;

// ---------------- HELPERS ----------------
float convertRegisterToTemperature(uint16_t regVal) {
    int16_t tempInt = (regVal & 0x8000) ? -((0xFFFF - regVal) + 1) : regVal;
    return tempInt * 0.1;
}

float convertRegisterToHumidity(uint16_t regVal) {
    return regVal * 0.1;
}

// ---------------- SETUP ----------------
void setupModbus() {
    pinMode(MAX485_DE, OUTPUT);
    digitalWrite(MAX485_DE, LOW);

    // Initialize Modbus with dummy slave
    node.begin(1, Serial);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
}

// ---------------- QUERY ONE SLAVE ----------------
JsonObject querySlaveJson(const ModbusDevice& sc, JsonDocument& doc) {
    node.begin(sc.id, Serial);
    uint8_t result = node.readInputRegisters(sc.regStart, sc.regCount);

    JsonObject obj = doc.to<JsonObject>();
    obj["id"] = sc.id;
    obj["name"] = sc.name;

    if (result == node.ku8MBSuccess) {
        obj["temperature"] = convertRegisterToTemperature(node.getResponseBuffer(0));
        obj["humidity"]    = convertRegisterToHumidity(node.getResponseBuffer(1));
    } else {
        obj["error"] = "0x" + String(result, HEX);
    }
    return obj;
}

// ---------------- QUERY ALL SLAVES ----------------
String queryAllSlaves() {
    StaticJsonDocument<1024> doc;
    JsonArray arr = doc.to<JsonArray>();

    for (size_t i = 0; i < slaveCount; i++) {
        StaticJsonDocument<128> tmp;
        JsonObject slaveObj = querySlaveJson(slaves[i], tmp);
        arr.add(slaveObj);
    }

    String output;
    serializeJson(doc, output);
    return output;
}

// ---------------- UPDATE SLAVES FROM WEB ----------------
void updateSlavesFromWeb(JsonArray arr) {
    slaveCount = 0;
    for (JsonObject obj : arr) {
        if (slaveCount < MAX_SLAVES) {
            slaves[slaveCount].id       = obj["id"] | 1;
            slaves[slaveCount].regStart = obj["regStart"] | 0;
            slaves[slaveCount].regCount = obj["regCount"] | 2;
            slaves[slaveCount].name     = obj["name"] | "slave";
            slaveCount++;
        }
    }
}
