# ESP8266 RS485 Modbus + MQTT Project

This project uses an **ESP8266** with an RS485 extension, using ModBus language to gather data from EID41-G01S and publish to MQTT.  
It is modularized into separate handler files for easier maintenance and reuse in future projects.

---

## 📂 File Structure  

- **`main.cpp`**  
  Entry point. Initializes all handlers and runs the main loop.  

- **`WiFiHandler.h / .cpp`**  
  Handles Wi-Fi STA/AP connection, reconnection, and OTA (Over-the-Air updates).  

- **`MQTTHandler.h / .cpp`**  
  Handles MQTT client connection, reconnection, subscriptions, and publishing.  

- **`ModbusHandler.h / .cpp`**  
  Handles Modbus RTU master communication via RS485 (temperature & humidity reading)

---


## ⚙️ Setup Instructions  

### 1️⃣ **WiFiHandler**
**In `WiFiHandler.cpp`:**
- Update the file with your preferred **SSID** and **Password** for both **STA** (Station mode) and **AP** (Access Point).  

**In `main.cpp`:**
```cpp
#include "WiFiHandler.h"

void setup() {
    setupWiFi();   // Connect to WiFi
}

void loop() {
    checkWiFi();   // Keep STA connected (auto reconnect if dropped)
}
```

**Notes:**  
- OTA (`ArduinoOTA`) is initialized only after STA is connected, so firmware updates work only with WiFi available.  

---

### 2️⃣ **MQTTHandler**
**In `main.cpp`:**
```cpp
#include "MQTTHandler.h"

void setup() {
    mqttClient.setServer(mqttServer, mqttPort); // Configure MQTT server
}

void loop() {
    if (!mqttClient.connected()) {
        reconnectMQTT();   // Auto reconnect
    }
    if (WiFi.status() == WL_CONNECTED) {
        mqttClient.loop(); // Maintain connection & handle messages
    }
}
```

**To publish from any file:**  
```cpp
publishMessage("your/topic", "Hello World");
```

**Notes:**  
- MQTT requires Wi-Fi STA mode (from WiFiHandler).  
- Topics and broker address are configured in `MQTTHandler.cpp`.  

---

### 3️⃣ **ModbusHandler**
**In `main.cpp`:**
```cpp
#include "ModbusHandler.h"

void setup() {
    setupModbus();   // Initialize Modbus RTU master
}

void loop() {
    readModbusJSON();   // Query slave & publish every 3 seconds
}
```

**Notes:**  
- In main.cpp there is a an internal **3-second timer** to avoid flooding the Modbus slave with `readModbusJSON()`.  
- Queries register `0` (temperature) and `1` (humidity) from the slave.  
- Automatically publishes results as JSON.

- Example JSON payload:
```json
{
  "temperature": 25.3,
  "humidity": 62.1
}
```

---

## 🚀 Workflow Summary  
1. **WiFiHandler** → Connects to Wi-Fi & enables OTA.  
2. **MQTTHandler** → Connects to MQTT broker, provides `publishMessage()`.  
3. **ModbusHandler** → Reads sensor registers via RS485 and publishes JSON data to MQTT.  
4. **main.cpp** → Initializes all handlers and ties everything together.  

---

## 🔧 Future Reuse  
- To start a new project, copy the `Handler` files into your project folder.  
- Update only:  
  - WiFi SSID & password in `WiFiHandler.cpp`.  
  - MQTT broker/server settings in `MQTTHandler.cpp`.  
  - Slave ID / register map in `ModbusHandler.cpp` (if different sensors are used).  
