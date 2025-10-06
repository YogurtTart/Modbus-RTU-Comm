
# ESP8266 RS485 Modbus + MQTT Project

This project uses an **ESP8266** with an RS485 extension, using Modbus RTU to gather data from EID41-G01S sensors and publish it to MQTT. It includes a **web interface** to configure Modbus slaves dynamically.
It is modularized into separate handler files for easier maintenance and reuse.

---

## 📂 File Structure

* **`main.cpp`**
  Entry point. Initializes all handlers and runs the main loop.

* **`WiFiHandler.h / .cpp`**
  Handles Wi-Fi STA/AP connection, reconnection, and OTA (Over-the-Air updates).

* **`MQTTHandler.h / .cpp`**
  Handles MQTT client connection, reconnection, subscriptions, and publishing.

* **`ModbusHandler.h / .cpp`**
  Handles Modbus RTU master communication via RS485, converts register data into temperature and humidity, and publishes JSON payloads.

* **`WebServerHandler.h / .cpp`**
  Hosts an HTTP web interface for managing Modbus slave devices. Supports:

  * Viewing current slaves
  * Adding new slaves
  * Deleting slaves
  * Preventing duplicate IDs and names

---

## ⚙️ Setup Instructions

### 1️⃣ WiFiHandler

**In `WiFiHandler.cpp`:**

* Set your **SSID** and **Password** for both **STA** (Station mode) and **AP** (Access Point).

**In `main.cpp`:**

```cpp
#include "WiFiHandler.h"

void setup() {
    setupWiFi();   // Connect to Wi-Fi
}

void loop() {
    checkWiFi();   // Keep STA connected, auto reconnect if dropped
}
```

**Notes:**

* OTA (`ArduinoOTA`) is initialized only after STA is connected. Firmware updates require Wi-Fi.

---

### 2️⃣ MQTTHandler

**In `main.cpp`:**

```cpp
#include "MQTTHandler.h"

void setup() {
    mqttClient.setServer(mqttServer, mqttPort); // Configure MQTT server
}

void loop() {
    if (!mqttClient.connected()) reconnectMQTT();   // Auto reconnect
    mqttClient.loop(); // Maintain connection & handle messages
}
```

**Publishing from anywhere:**

```cpp
publishMessage("your/topic", variable.c_str());
```

**Notes:**

* MQTT requires Wi-Fi STA mode.
* Topics and broker address are configured in `MQTTHandler.cpp`.

---

### 3️⃣ ModbusHandler

**In `main.cpp`:**

```cpp
#include "ModbusHandler.h"

void setup() {
    setupModbus();   // Initialize Modbus RTU master
}

void loop() {
    // Poll all slaves periodically
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        String json = queryAllSlaves();             // Read all slave registers
        publishMessage(mqttTopicPub, json.c_str()); // Publish JSON to MQTT
    }
}
```

**Notes:**

* A 3-second timer avoids flooding Modbus slaves.
* Automatically publishes results as JSON.

**Example JSON payload for all slaves:**

```json
[
  {
    "id": 1,
    "name": "Sensor1",
    "temperature": 25.3,
    "humidity": 62.1
  },
  {
    "id": 2,
    "name": "Sensor2",
    "temperature": 24.8,
    "humidity": 60.7
  }
]
```

---

### 4️⃣ WebServerHandler

**Accessing Web UI:** Open `http://<ESP_IP>/` in your browser.

**Features:**

* Display current Modbus slaves in a table.
* Add new slaves (prevents duplicate IDs and names).
* Delete slaves.
* Live table updates using JavaScript fetch API.

**How it works:**

* `/slaves` endpoint returns all slaves in JSON format.
* `/addSlave` endpoint handles HTML form submission to add slaves.
* `/deleteSlave` endpoint handles deleting a slave by ID.
* All operations update the **global `slaves[]` array** in memory, which is then used by Modbus polling and MQTT publishing.

---

## 🚀 Workflow Summary

1. **WiFiHandler** → Connects to Wi-Fi & enables OTA updates.
2. **MQTTHandler** → Connects to MQTT broker, provides `publishMessage()`.
3. **ModbusHandler** → Reads sensor registers via RS485 and converts to JSON.
4. **WebServerHandler** → Manage slaves dynamically via web browser.
5. **main.cpp** → Initializes all handlers and ties everything together.

---

## 🔧 Future Reuse

* Copy the `Handler` files into a new project folder.
* Update only:

  * Wi-Fi credentials in `WiFiHandler.cpp`
  * MQTT broker/server settings in `MQTTHandler.cpp`
  * Slave ID/register map in `ModbusHandler.cpp` if sensors differ
