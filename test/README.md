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

- **main.cpp**  
  The entry point of the program.  
  - Initializes WiFi, MQTT, and Modbus.  
  - Runs the main loop for polling Modbus sensors, handling MQTT, and OTA updates.


- **WiFiHandler.h / WiFiHandler.cpp**  
  Responsible for connecting the ESP8266 to WiFi.  
  - Handles WiFi setup in **STA & AP mode** (client).  
  - Provides reconnection logic if WiFi drops.  
  - Contains SSID & password configuration.

 **In `To SetUp in main.cpp`:**
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
- OTA (`ArduinoOTA`) is initialized only after STA is connected at WiFiHandler.cpp, so firmware updates work only with WiFi available.  



- **MQTTHandler.h / MQTTHandler.cpp**  
  Handles MQTT communication.  
  - Connects to the broker.  
  - Defines publish/subscribe topics.  
  - Provides `publishMessage()` function to send sensor JSON data.  
  - Includes auto-reconnect if connection is lost.

**To publish from any file:**  
```cpp
#include "MQTTHandler.h"

publishMessage("your/topic", "variable".c_str());
```


  **In `To SetUp in main.cpp`:**
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

**Notes:**  
- MQTT requires Wi-Fi STA mode (from WiFiHandler).  
- Topics and broker address are configured in `MQTTHandler.cpp`.  


- **ModbusHandler.h / ModbusHandler.cpp**  
  Handles Modbus RTU communication over RS485.  
  - Configures RS485 DE/RE pins for transmission.  
  - Defines functions to read registers from the sensor.  
  - Converts raw register values into **temperature & humidity**.   


  **In `To SetUp in main.cpp`:**
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
- `readModbusJSON()` uses an internal **3-second timer** to avoid flooding the Modbus slave.  
- Queries register `0` (temperature) and `1` (humidity) from the slave.  
- Automatically publishes results as JSON to MQTT if connected, otherwise logs to Serial.  
- Example JSON payload:
```json
{
  "temperature": 25.3,
  "humidity": 62.1
}
```



---

## ⚡ Setup Instructions

### 1. Hardware
- ESP8266 
- RS485 transceiver module (MAX485).  
- Modbus RTU sensor (e.g., EID041-G01S).  
- Connect `RO/TX`, `DI/RX`, and `DE/RE` to ESP pins.  
- Power from 5V or 3.3V depending on module.

### 2. Software
- Install PlatformIO or Arduino IDE.  
- Required libraries:
  - `ModbusMaster`
  - `PubSubClient`
  - `ArduinoJson`
  - `ArduinoOTA`

### 3. Configuration
- **WiFiHandler.h** → set your WiFi SSID & password.  
- **MQTTHandler.h** → set broker IP, port, and topics.  
- **ModbusHandler.cpp** → set:
  - Slave ID (`node.begin(slaveID, Serial)`)  
  - Register addresses (`START_ADDRESS`, `NUM_REGISTERS`)  

### 4. Upload
- Connect ESP8266 via USB.  
- Upload via PlatformIO / Arduino IDE.  
- Check Serial Monitor for logs.

---


## 🔄 How to Reuse for Future Projects

- **Need a different sensor?**  
  Update register addresses and conversion logic in **ModbusHandler.cpp**.  

- **Need a different MQTT topic or broker?**  
  Update **MQTTHandler.h**.  

- **New WiFi credentials?**  
  Update **WiFiHandler.h**.  

- **Want to add another protocol (LoRa, HTTP, etc.)?**  
  Create a new `Handler.h/.cpp` file and call it from `main.cpp`.  

This modular structure allows you to quickly adapt the project without rewriting everything.

---

## 🛠 Example Workflow
1. ESP boots up → connects to WiFi & MQTT.  
2. Every 3s → reads Modbus registers (temperature & humidity).  
3. Publishes JSON to MQTT topic:  

```json
{
  "temperature": 23.5,
  "humidity": 55.2
}
