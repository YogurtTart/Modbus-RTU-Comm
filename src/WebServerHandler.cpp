#include "WebServerHandler.h"
#include <ArduinoJson.h>

ESP8266WebServer server(80);

// ---------------- GET: return current slaves as JSON ----------------
void handleGetSlaves() {
    StaticJsonDocument<1024> doc;
    JsonArray arr = doc.createNestedArray("slaves");

    for (uint8_t i = 0; i < slaveCount; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["id"]       = slaves[i].id;
        obj["regStart"] = slaves[i].regStart;
        obj["regCount"] = slaves[i].regCount;
        obj["name"]     = slaves[i].name;
    }

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

// ---------------- POST: update slaves via JSON ----------------
void handleSetSlaves() {
    if (server.hasArg("plain")) {
        StaticJsonDocument<1024> doc;
        DeserializationError err = deserializeJson(doc, server.arg("plain"));

        if (!err && doc["slaves"].is<JsonArray>()) {
            updateSlavesFromWeb(doc["slaves"].as<JsonArray>());
            server.send(200, "application/json", "{\"status\":\"updated\"}");
        } else {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        }
    } else {
        server.send(400, "application/json", "{\"error\":\"No body\"}");
    }
}

// ---------------- POST: Delete slave by ID ----------------
void handleDeleteSlave() {
    if (server.hasArg("id")) {
        int delId = server.arg("id").toInt();
        bool found = false;

        for (uint8_t i = 0; i < slaveCount; i++) {
            if (slaves[i].id == delId) {
                for (uint8_t j = i; j < slaveCount - 1; j++) {
                    slaves[j] = slaves[j + 1];
                }
                slaveCount--;
                found = true;
                break;
            }
        }

        if (found) {
            server.send(200, "application/json", "{\"status\":\"deleted\"}");
        } else {
            server.send(404, "application/json", "{\"error\":\"ID not found\"}");
        }
    } else {
        server.send(400, "application/json", "{\"error\":\"Missing ID\"}");
    }
}

// ---------------- GET/POST: HTML form for adding/deleting slaves ----------------
void handleAddSlaveForm() {
    if (server.method() == HTTP_GET) {
        String html = R"rawliteral(
            <!DOCTYPE html>
            <html>
            <head><title>Modbus Slave Config</title></head>
            <body>
              <h2>Add Modbus Slave</h2>
              <form id="addForm" action="/addSlave" method="POST">
                ID: <input type="number" name="id"><br>
                Reg Start: <input type="number" name="regStart"><br>
                Reg Count: <input type="number" name="regCount"><br>
                Name: <input type="text" name="name"><br>
                <input type="submit" value="Add Slave">
              </form>

              <h2>Current Slaves</h2>
              <table border="1" id="slaveTable">
                <thead><tr><th>ID</th><th>RegStart</th><th>RegCount</th><th>Name</th><th>Action</th></tr></thead>
                <tbody></tbody>
              </table>

              <script>
                async function refreshSlaves() {
                  const res = await fetch('/slaves');
                  const data = await res.json();
                  const tbody = document.querySelector('#slaveTable tbody');
                  tbody.innerHTML = '';
                  data.slaves.forEach(s => {
                    tbody.innerHTML += `<tr>
                      <td>${s.id}</td>
                      <td>${s.regStart}</td>
                      <td>${s.regCount}</td>
                      <td>${s.name}</td>
                      <td><button onclick="deleteSlave(${s.id})">Delete</button></td>
                    </tr>`;
                  });
                }

                async function deleteSlave(id) {
                  const res = await fetch('/deleteSlave?id=' + id, {method: 'POST'});
                  if (res.ok) refreshSlaves();
                  else alert('Failed to delete');
                }

                // Validate no duplicate ID or Name
                document.getElementById('addForm').addEventListener('submit', async (e) => {
                  e.preventDefault();
                  const form = e.target;
                  const newId = form.id.value;
                  const newName = form.name.value.trim();
                  const res = await fetch('/slaves');
                  const data = await res.json();

                  if (data.slaves.some(s => s.id == newId)) {
                    alert('ID already exists!');
                    return;
                  }
                  if (data.slaves.some(s => s.name == newName)) {
                    alert('Name already exists!');
                    return;
                  }

                  form.submit();
                });

                refreshSlaves();
              </script>
            </body>
            </html>
        )rawliteral";
        server.send(200, "text/html", html);

    } else if (server.method() == HTTP_POST) {
        if (server.hasArg("id") && server.hasArg("regStart") &&
            server.hasArg("regCount") && server.hasArg("name")) {

            int newId = server.arg("id").toInt();
            String newName = server.arg("name");

            // Prevent duplicates
            for (uint8_t i = 0; i < slaveCount; i++) {
                if (slaves[i].id == newId) {
                    server.send(400, "text/plain", "Duplicate ID");
                    return;
                }
                if (slaves[i].name == newName) {
                    server.send(400, "text/plain", "Duplicate Name");
                    return;
                }
            }

            if (slaveCount < MAX_SLAVES) {
                slaves[slaveCount].id       = newId;
                slaves[slaveCount].regStart = server.arg("regStart").toInt();
                slaves[slaveCount].regCount = server.arg("regCount").toInt();
                slaves[slaveCount].name     = newName;
                slaveCount++;
            }

            server.sendHeader("Location", "/");
            server.send(303);
        } else {
            server.send(400, "text/plain", "Missing parameters");
        }
    }
}

// ---------------- Setup server routes ----------------
void setupWebServer() {
    server.on("/", HTTP_GET, handleAddSlaveForm);
    server.on("/addSlave", HTTP_GET, handleAddSlaveForm);
    server.on("/addSlave", HTTP_POST, handleAddSlaveForm);

    server.on("/slaves", HTTP_GET, handleGetSlaves);
    server.on("/slaves", HTTP_POST, handleSetSlaves);

    server.on("/deleteSlave", HTTP_POST, handleDeleteSlave);

    server.begin();
}

// ---------------- Call this in loop ----------------
void handleWebServer() {
    server.handleClient();
}
