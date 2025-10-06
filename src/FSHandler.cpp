#include "FSHandler.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

#define SLAVES_FILE "/slaves.json"

void initFS() {
    LittleFS.begin();
       
    loadSlavesFromFS();
}

void saveSlavesToFS() {
    StaticJsonDocument<1024> doc;
    JsonArray arr = doc.to<JsonArray>();

    for (uint8_t i = 0; i < slaveCount; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["id"]       = slaves[i].id;
        obj["regStart"] = slaves[i].regStart;
        obj["regCount"] = slaves[i].regCount;
        obj["name"]     = slaves[i].name;
    }

    File file = LittleFS.open(SLAVES_FILE, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    serializeJson(doc, file);
    file.close();
}

void loadSlavesFromFS() {
    if (!LittleFS.exists(SLAVES_FILE)) return;

    File file = LittleFS.open(SLAVES_FILE, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
        Serial.println("Failed to parse JSON");
        return;
    }

    JsonArray arr = doc.as<JsonArray>();
    slaveCount = 0;
    for (JsonObject obj : arr) {
        if (slaveCount >= MAX_SLAVES) break;
        slaves[slaveCount].id       = obj["id"];
        slaves[slaveCount].regStart = obj["regStart"];
        slaves[slaveCount].regCount = obj["regCount"];
        slaves[slaveCount].name     = String(obj["name"].as<const char*>());
        slaveCount++;
    }
}
