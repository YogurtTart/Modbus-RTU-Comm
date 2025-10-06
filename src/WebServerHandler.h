#pragma once
#include <ESP8266WebServer.h>
#include "ModbusHandler.h"

#define MAX_SLAVES 10

extern ESP8266WebServer server;

void setupWebServer();
void handleWebServer();
void handleGetSlaves();
void handleSetSlaves();
void handleAddSlaveForm();
