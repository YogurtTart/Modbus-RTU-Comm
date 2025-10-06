#pragma once

#include <Arduino.h>
#include "ModbusHandler.h"

// Initialize filesystem and load slaves
void initFS();

// Save slaves[] to LittleFS
void saveSlavesToFS();

// Load slaves[] from LittleFS
void loadSlavesFromFS();

