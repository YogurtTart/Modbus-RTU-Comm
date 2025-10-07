#pragma once
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "ModBusHandler.h"


// Global variables
extern ESP8266WebServer server;
extern bool savePending;
extern bool shouldQuerySlaves;

// Function prototypes
void setupWebServer();
void handleWebServer();
void handleRoot();
void handleGetSlaves();
void handleAddSlave();
void handleDeleteSlave();
void saveSlavesToFS();
void loadSlavesFromFS();
void processPendingSaves();
void requestSaveSlaves();