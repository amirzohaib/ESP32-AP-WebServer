/*
  ESP32 Access Point + Captive Portal + LittleFS (statische Dateien)
  - SSID: ESP32-AP
  - Passwort: 12345678
  - Feste IP: 192.168.4.1
  - Seiten & Assets werden aus LittleFS geladen:
      /index.html, /info.html, /kontakt.html, /style.css, /logo.svg
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <FS.h>
#include <LittleFS.h>

const char* apSsid = "ESP32-AP";
const char* apPass = "12345678"; // mind. 8 Zeichen

// Netzwerk (statische AP-IP)
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway (192, 168, 4, 1);
IPAddress subnet  (255, 255, 255, 0);

// Server
DNSServer dnsServer;
const byte DNS_PORT = 53;
WebServer server(80);

// ---- Content-Type Erkennung ----
String getContentType(const String& filename) {
  if (filename.endsWith(".htm") || filename.endsWith(".html")) return "text/html; charset=utf-8";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".svg")) return "image/svg+xml";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
  if (filename.endsWith(".gif")) return "image/gif";
  if (filename.endsWith(".ico")) return "image/x-icon";
  if (filename.endsWith(".json")) return "application/json; charset=utf-8";
  if (filename.endsWith(".txt")) return "text/plain; charset=utf-8";
  return "application/octet-stream";
}

// Datei aus LittleFS streamen (liefert true bei Erfolg)
bool handleFileRead(String path) {
  // Standard-Dokument
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);

  if (!LittleFS.exists(path)) {
    // Versuche ohne führenden Slash
    if (path[0] == '/' && LittleFS.exists(path.substring(1))) {
      path = path.substring(1);
    } else {
      return false;
    }
  }

  File file = LittleFS.open(path, "r");
  if (!file) return false;

  // Für statische Assets Caching erlauben (1 Tag)
  if (contentType.startsWith("image/") || contentType == "text/css" || contentType == "application/javascript") {
    server.sendHeader("Cache-Control", "public, max-age=86400");
  }

  // Datei streamen
  server.streamFile(file, contentType);
  file.close();
  return true;
}

// ---- Seiten-Handler ----
void handleRoot() {
  if (!handleFileRead("/index.html")) {
    server.send(500, "text/plain; charset=utf-8", "index.html fehlt im LittleFS");
  }
}

void handleInfo() {
  if (!handleFileRead("/info.html")) {
    server.send(404, "text/plain; charset=utf-8", "info.html nicht gefunden");
  }
}

void handleKontakt() {
  if (!handleFileRead("/kontakt.html")) {
    server.send(404, "text/plain; charset=utf-8", "kontakt.html nicht gefunden");
  }
}

// Captive-Portal: immer Startseite liefern
void handleCaptive() { handleRoot(); }

// Catch-All: unbekannte Pfade -> versuche Datei, sonst Startseite
void handleNotFound() {
  String uri = server.uri();
  if (!handleFileRead(uri)) {
    handleRoot();
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // LittleFS starten
  if (!LittleFS.begin()) {
    Serial.println("LittleFS konnte nicht gemountet werden! Filesystem hochladen?");
  } else {
    Serial.println("LittleFS bereit.");
  }

  // Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  bool ok = WiFi.softAP(apSsid, apPass);
  if (!ok) {
    Serial.println("Fehler: AP konnte nicht gestartet werden.");
  } else {
    Serial.println("Access Point gestartet.");
    Serial.print("SSID: "); Serial.println(apSsid);
    Serial.print("IP:   "); Serial.println(WiFi.softAPIP());
  }

  // DNS: alle Domains -> lokale IP (Captive-Portal-Effekt)
  dnsServer.start(DNS_PORT, "*", local_ip);

  // Routen
  server.on("/", HTTP_GET, handleRoot);
  server.on("/info", HTTP_GET, handleInfo);
  server.on("/kontakt", HTTP_GET, handleKontakt);

  // Captive-Portal-Trigger (verschiedene OS)
  server.on("/generate_204", HTTP_GET, handleCaptive);   // Android
  server.on("/gen_204", HTTP_GET, handleCaptive);        // Android alt
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptive); // Apple
  server.on("/ncsi.txt", HTTP_GET, handleCaptive);       // Windows
  server.on("/connecttest.txt", HTTP_GET, handleCaptive);
  server.on("/redirect", HTTP_GET, handleCaptive);
  server.on("/fwlink", HTTP_GET, handleCaptive);

  // Alle anderen Pfade
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP-Server läuft auf Port 80.");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
