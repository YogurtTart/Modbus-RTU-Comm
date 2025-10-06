#include "WebServerHandler.h"
#include <ArduinoJson.h>

ESP8266WebServer server(80);

// ---------------- GET: return current slaves as JSON ----------------
void handleGetSlaves() {
    DynamicJsonDocument doc(1024);
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
        DynamicJsonDocument doc(1024);
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

// ---------------- GET/POST: HTML form for adding a new slave ----------------
void handleAddSlaveForm() {
    if (server.method() == HTTP_GET) {
        String html = R"rawliteral(
            <!DOCTYPE html>
            <html>
            <head><title>Modbus Slave Config</title></head>
            <body>
              <h2>Add / Edit Modbus Slaves</h2>
              <form action="/addSlave" method="POST">
                ID: <input type="number" name="id"><br>
                Reg Start: <input type="number" name="regStart"><br>
                Reg Count: <input type="number" name="regCount"><br>
                Name: <input type="text" name="name"><br>
                <input type="submit" value="Add Slave">
              </form>
              <hr>
              <h3>Current Slaves</h3>
              <pre id="slaveList"></pre>
              <script>
                fetch('/slaves').then(r => r.json()).then(d => {
                  document.getElementById('slaveList').textContent = JSON.stringify(d, null, 2);
                });
              </script>
            </body>
            </html>
        )rawliteral";
        server.send(200, "text/html", html);
    } else if (server.method() == HTTP_POST) {
        if (server.hasArg("id") && server.hasArg("regStart") &&
            server.hasArg("regCount") && server.hasArg("name")) {

            if (slaveCount < MAX_SLAVES) {
                slaves[slaveCount].id       = server.arg("id").toInt();
                slaves[slaveCount].regStart = server.arg("regStart").toInt();
                slaves[slaveCount].regCount = server.arg("regCount").toInt();
                slaves[slaveCount].name     = server.arg("name");
                slaveCount++;
            }

            server.sendHeader("Location", "/"); // Redirect back to form
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

    server.begin();
}

// ---------------- Call this in loop ----------------
void handleWebServer() {
    server.handleClient();
}
