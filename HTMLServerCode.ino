#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32-AP";
const char* password = "12345678"; // mindestens 8 Zeichen

WebServer server(80);

// HTML-Seite (im Flash-Speicher)
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Access Point</title>
<style>
  body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
  h1 { color: #333; }
  p { font-size: 18px; }
</style>
</head>
<body>
  <h1>Willkommen auf dem ESP32!</h1>
  <p>Du bist erfolgreich mit dem Access Point verbunden.</p>
  <p>IP-Adresse: <strong>192.168.4.1</strong></p>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println("Access Point gestartet");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Webserver läuft...");
}

void loop() {
  server.handleClient();
}
