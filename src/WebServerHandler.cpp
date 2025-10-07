#include "WebServerHandler.h"
#include <LittleFS.h>
#include "MQTTHandler.h"
#include "ModBusHandler.h"
#include <Arduino.h>

ESP8266WebServer server(80);
bool savePending = false;
bool shouldQuerySlaves = false;

// Serve the main configuration page
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Modbus Slave Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input, button { width: 100%; padding: 8px; margin: 5px 0; border: 1px solid #ddd; border-radius: 4px; }
        button { background: #007cba; color: white; border: none; cursor: pointer; }
        button:hover { background: #005a87; }
        button.delete { background: #dc3545; }
        button.delete:hover { background: #c82333; }
        table { width: 100%; border-collapse: collapse; margin-top: 20px; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background: #f8f9fa; }
        .section { margin-bottom: 30px; padding: 15px; border: 1px solid #e9ecef; border-radius: 5px; }
        .status { padding: 10px; margin: 10px 0; border-radius: 4px; }
        .success { background: #d4edda; color: #155724; }
        .error { background: #f8d7da; color: #721c24; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Modbus Slave Configuration</h1>
        
        <div class="section">
            <h2>Add New Slave</h2>
            <form id="addForm">
                <div class="form-group">
                    <label>Slave ID:</label>
                    <input type="number" name="id" min="1" max="247" required>
                </div>
                <div class="form-group">
                    <label>Start Register:</label>
                    <input type="number" name="startReg" value="0" required>
                </div>
                <div class="form-group">
                    <label>Number of Registers:</label>
                    <input type="number" name="numRegs" value="2" required>
                </div>
                <div class="form-group">
                    <label>Name:</label>
                    <input type="text" name="name" placeholder="sensor1" required>
                </div>
                <button type="submit">Add Slave</button>
            </form>
        </div>

        <div class="section">
            <h2>Current Slaves</h2>
            <div id="status"></div>
            <table>
                <thead>
                    <tr>
                        <th>ID</th>
                        <th>Name</th>
                        <th>Start Reg</th>
                        <th>Num Regs</th>
                        <th>Actions</th>
                    </tr>
                </thead>
                <tbody id="slavesTable"></tbody>
            </table>
        </div>

        <div class="section">
            <h2>Actions</h2>
            <button onclick="queryAllSlaves()">Query All Slaves Now</button>
            <button onclick="saveConfiguration()" style="background: #28a745;">Save Configuration</button>
            <button onclick="loadConfiguration()" style="background: #17a2b8;">Load Configuration</button>
        </div>
    </div>

    <script>
        let currentSlaves = [];

        // Load slaves on page load
        window.onload = loadSlaves;

        async function loadSlaves() {
            try {
                const response = await fetch('/slaves');
                currentSlaves = await response.json();
                updateSlavesTable();
            } catch (error) {
                showStatus('Error loading slaves: ' + error, 'error');
            }
        }

        function updateSlavesTable() {
            const tbody = document.getElementById('slavesTable');
            tbody.innerHTML = '';
            
            currentSlaves.forEach(slave => {
                const row = tbody.insertRow();
                row.innerHTML = `
                    <td>${slave.id}</td>
                    <td>${slave.name}</td>
                    <td>${slave.startReg}</td>
                    <td>${slave.numRegs}</td>
                    <td>
                        <button class="delete" onclick="deleteSlave(${slave.id})">Delete</button>
                    </td>
                `;
            });
        }

        // Add slave form handler
        document.getElementById('addForm').addEventListener('submit', async function(e) {
            e.preventDefault();
            
            const formData = new FormData(this);
            const newSlave = {
                id: parseInt(formData.get('id')),
                startReg: parseInt(formData.get('startReg')),
                numRegs: parseInt(formData.get('numRegs')),
                name: formData.get('name')
            };

            // Validate no duplicate ID
            if (currentSlaves.some(s => s.id === newSlave.id)) {
                showStatus('Error: Slave ID already exists!', 'error');
                return;
            }

            // Validate no duplicate name
            if (currentSlaves.some(s => s.name === newSlave.name)) {
                showStatus('Error: Slave name already exists!', 'error');
                return;
            }

            try {
                const response = await fetch('/addSlave', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(newSlave)
                });

                if (response.ok) {
                    showStatus('Slave added successfully!', 'success');
                    this.reset();
                    await loadSlaves();
                } else {
                    showStatus('Error adding slave', 'error');
                }
            } catch (error) {
                showStatus('Error: ' + error, 'error');
            }
        });

        async function deleteSlave(id) {
            if (!confirm('Are you sure you want to delete slave ' + id + '?')) {
                return;
            }

            try {
                const response = await fetch('/deleteSlave?id=' + id, {
                    method: 'POST'
                });

                if (response.ok) {
                    showStatus('Slave deleted successfully!', 'success');
                    await loadSlaves();
                } else {
                    showStatus('Error deleting slave', 'error');
                }
            } catch (error) {
                showStatus('Error: ' + error, 'error');
            }
        }

        async function queryAllSlaves() {
            try {
                showStatus('Querying slaves...', 'success');
                const response = await fetch('/querySlaves', { method: 'POST' });
                if (response.ok) {
                    showStatus('Slaves queried and data sent to MQTT!', 'success');
                } else {
                    showStatus('Error querying slaves', 'error');
                }
            } catch (error) {
                showStatus('Error: ' + error, 'error');
            }
        }

        async function saveConfiguration() {
            try {
                const response = await fetch('/saveSlaves', { method: 'POST' });
                if (response.ok) {
                    showStatus('Configuration saved to flash!', 'success');
                } else {
                    showStatus('Error saving configuration', 'error');
                }
            } catch (error) {
                showStatus('Error: ' + error, 'error');
            }
        }

        async function loadConfiguration() {
            try {
                const response = await fetch('/loadSlaves', { method: 'POST' });
                if (response.ok) {
                    showStatus('Configuration loaded from flash!', 'success');
                    await loadSlaves();
                } else {
                    showStatus('Error loading configuration', 'error');
                }
            } catch (error) {
                showStatus('Error: ' + error, 'error');
            }
        }

        function showStatus(message, type) {
            const statusDiv = document.getElementById('status');
            statusDiv.innerHTML = `<div class="status ${type}">${message}</div>`;
            setTimeout(() => statusDiv.innerHTML = '', 5000);
        }
    </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

// Get slaves as JSON (NON-BLOCKING)
void handleGetSlaves() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  
  for (uint8_t i = 0; i < slaveCount; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["id"] = slaves[i].id;
    obj["name"] = slaves[i].name;
    obj["startReg"] = slaves[i].startReg;
    obj["numRegs"] = slaves[i].numRegs;
  }
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// Add new slave (NON-BLOCKING)
void handleAddSlave() {
  if (server.hasArg("plain")) {
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    
    if (slaveCount < MAX_SLAVES) {
      uint8_t newId = doc["id"];
      String newName = doc["name"].as<String>();
      
      // Check for duplicates
      for (uint8_t i = 0; i < slaveCount; i++) {
        if (slaves[i].id == newId) {
          server.send(400, "application/json", "{\"error\":\"Duplicate ID\"}");
          return;
        }
        if (slaves[i].name == newName) {
          server.send(400, "application/json", "{\"error\":\"Duplicate name\"}");
          return;
        }
      }
      
      slaves[slaveCount].id = newId;
      slaves[slaveCount].startReg = doc["startReg"];
      slaves[slaveCount].numRegs = doc["numRegs"];
      slaves[slaveCount].name = newName;
      slaveCount++;
      
      //requestSaveSlaves(); // Schedule async save
      server.send(200, "application/json", "{\"status\":\"added\"}");
    } else {
      server.send(400, "application/json", "{\"error\":\"Max slaves reached\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

// Delete slave (NON-BLOCKING)
void handleDeleteSlave() {
  if (server.hasArg("id")) {
    uint8_t delId = server.arg("id").toInt();
    bool found = false;
    
    for (uint8_t i = 0; i < slaveCount; i++) {
      if (slaves[i].id == delId) {
        // Shift array
        for (uint8_t j = i; j < slaveCount - 1; j++) {
          slaves[j] = slaves[j + 1];
        }
        slaveCount--;
        found = true;
        break;
      }
    }
    
    if (found) {
      //requestSaveSlaves(); // Schedule async save
      server.send(200, "application/json", "{\"status\":\"deleted\"}");
    } else {
      server.send(404, "application/json", "{\"error\":\"ID not found\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing ID\"}");
  }
}

// Trigger slave query (NON-BLOCKING)
void handleQuerySlaves() {
  shouldQuerySlaves = true;
  server.send(200, "application/json", "{\"status\":\"query_started\"}");
}

// Save slaves to LittleFS (called async)
void handleSaveSlaves() {
  requestSaveSlaves();
  server.send(200, "application/json", "{\"status\":\"save_scheduled\"}");
}

// Load slaves from LittleFS (NON-BLOCKING)
void handleLoadSlaves() {
  loadSlavesFromFS();
  server.send(200, "application/json", "{\"status\":\"loaded\"}");
}


// Schedule save for main loop (NON-BLOCKING)
void requestSaveSlaves() {
  savePending = true;
}

// Process pending saves in main loop
void processPendingSaves() {
  if (savePending) {
    saveSlavesToFS();
    savePending = false;
  }
}

void setupWebServer() {
  // ✅ LITTLEFS INITIALIZATION (already done in main, but safe to call again)
  LittleFS.begin();
  
  // Load existing configuration
  loadSlavesFromFS();
  
  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/slaves", HTTP_GET, handleGetSlaves);
  server.on("/addSlave", HTTP_POST, handleAddSlave);
  server.on("/deleteSlave", HTTP_POST, handleDeleteSlave);
  server.on("/querySlaves", HTTP_POST, handleQuerySlaves);
  server.on("/saveSlaves", HTTP_POST, handleSaveSlaves);
  server.on("/loadSlaves", HTTP_POST, handleLoadSlaves);
  
  server.begin();
  Serial.println("✅ HTTP server started");
}

// Handle web client in main loop
void handleWebServer() {
  server.handleClient();
}

// Save slaves configuration to LittleFS
void saveSlavesToFS() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  
  for (uint8_t i = 0; i < slaveCount; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["id"] = slaves[i].id;
    obj["startReg"] = slaves[i].startReg;
    obj["numRegs"] = slaves[i].numRegs;
    obj["name"] = slaves[i].name;
  }
  
  // ✅ LITTLEFS WRITE OPERATION
  File file = LittleFS.open("/slaves.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("✅ Slaves saved to LittleFS");
  } else {
    Serial.println("❌ Failed to save slaves to LittleFS");
  }
}

// Load slaves configuration from LittleFS
void loadSlavesFromFS() {
  // ✅ LITTLEFS READ OPERATION
  if (LittleFS.exists("/slaves.json")) {
    File file = LittleFS.open("/slaves.json", "r");
    if (file) {
      JsonDocument doc;
      deserializeJson(doc, file);
      file.close();
      
      slaveCount = 0;
      JsonArray arr = doc.as<JsonArray>();
      for (JsonObject obj : arr) {
        if (slaveCount < MAX_SLAVES) {
          slaves[slaveCount].id = obj["id"];
          slaves[slaveCount].startReg = obj["startReg"];
          slaves[slaveCount].numRegs = obj["numRegs"];
          slaves[slaveCount].name = obj["name"].as<String>();
          slaveCount++;
        }
      }
      Serial.println("✅ Slaves loaded from LittleFS");
      Serial.print("Loaded ");
      Serial.print(slaveCount);
      Serial.println(" slaves");
    }
  } else {
    Serial.println("⚠️ No saved slaves configuration found");
  }
}