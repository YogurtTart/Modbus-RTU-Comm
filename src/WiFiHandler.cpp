#include "WiFiHandler.h"
#include <ArduinoOTA.h>

const char* ssidSTA = "Tanand_Hardware";
const char* passwordSTA = "202040406060808010102020";

const char* ssidAP = "ESP8266_AP";
const char* passwordAP = "12345678";

bool otaInitialized = false;
unsigned long previousWiFiCheck = 0;
const unsigned long wifiCheckInterval = 10000;

void setupWiFi() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssidSTA, passwordSTA);
    WiFi.softAP(ssidAP, passwordAP);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
    Serial.print("STA IP: "); Serial.println(WiFi.localIP());
}

void checkWiFi() {
    unsigned long now = millis();
    if (now - previousWiFiCheck >= wifiCheckInterval) {
        previousWiFiCheck = now;
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Reconnecting STA...");
            WiFi.begin(ssidSTA, passwordSTA);
            otaInitialized = false;
        } else if (!otaInitialized) {
            ArduinoOTA.begin();
            otaInitialized = true;
            Serial.print("STA connected, IP: ");
            Serial.println(WiFi.localIP());
        }
    }
}
