/*
  ESP32 Access Point mit Captive Portal und mehreren Unterseiten
  - SSID: ESP32-AP
  - Passwort: 12345678
  - IP: 192.168.4.1
  - Seiten: "/", "/info", "/kontakt"
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

const char* apSsid = "ESP32-AP";
const char* apPass = "12345678"; // mind. 8 Zeichen

// Feste IP (optional, aber empfehlenswert)
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway (192, 168, 4, 1);
IPAddress subnet  (255, 255, 255, 0);

// DNS-Server (für Captive Portal: wildcard-DNS -> zeigt immer auf ESP32-IP)
DNSServer dnsServer;
const byte DNS_PORT = 53;

// HTTP-Server
WebServer server(80);

// --------- Hilfsfunktionen für HTML-Ausgabe ---------

String htmlPage(const String& title, const String& content) {
  // Minimalistisches Layout mit Navigation
  String html = F(
    "<!DOCTYPE html><html lang='de'><head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>"
  );
  html += title;
  html += F(
    "</title>"
    "<style>"
    "body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;margin:1.5rem;line-height:1.5}"
    "header{margin-bottom:1rem}"
    "nav a{margin-right:.8rem;text-decoration:none;color:#0a66c2;font-weight:600}"
    "nav a:hover{text-decoration:underline}"
    ".card{max-width:800px;border:1px solid #ddd;border-radius:12px;padding:1rem}"
    "h1{margin:.2rem 0 1rem;font-size:1.4rem}"
    "footer{margin-top:2rem;color:#666;font-size:.9rem}"
    "</style>"
    "</head><body>"
    "<header><nav>"
    "<a href='/'>Start</a>"
    "<a href='/info'>Info</a>"
    "<a href='/kontakt'>Kontakt</a>"
    "</nav></header>"
    "<main class='card'><h1>"
  );
  html += title;
  html += F("</h1>");
  html += content;
  html += F("</main>"
            "<footer>ESP32 Captive Portal • IP: 192.168.4.1</footer>"
            "</body></html>");
  return html;
}

// --------- Seiten-Handler ---------

void handleHome() {
  String content =
    F("<p>Willkommen! Du bist mit dem ESP32 Access Point verbunden.</p>"
      "<p>Diese Seite wird lokal vom ESP32 bereitgestellt.</p>"
      "<p><strong>Tipp:</strong> Du kannst auch die Seiten <code>/info</code> und <code>/kontakt</code> aufrufen.</p>");
  server.send(200, "text/html; charset=utf-8", htmlPage("Start", content));
}

void handleInfo() {
  String content =
    F("<p><strong>Info</strong></p>"
      "<ul>"
      "<li>SSID: ESP32-AP</li>"
      "<li>Gateway & Server-IP: 192.168.4.1</li>"
      "<li>Subnetz: 255.255.255.0</li>"
      "<li>Modus: Access Point + Captive Portal</li>"
      "</ul>");
  server.send(200, "text/html; charset=utf-8", htmlPage("Info", content));
}

void handleKontakt() {
  String content =
    F("<p>Kontakt / Impressum (Beispielinhalte):</p>"
      "<p>Administrator: <em>Dein Name</em></p>"
      "<p>E-Mail: <em>example@local</em></p>");
  server.send(200, "text/html; charset=utf-8", htmlPage("Kontakt", content));
}

// Common captive portal handler: liefert Startseite zurück
void handleCaptive() {
  // Viele Betriebssysteme erwarten hier eine 200-Antwort, damit das Captive-Portal-Fenster aufgeht.
  handleHome();
}

// Catch-All für unbekannte Pfade: leite auf Start weiter (Captive-Portal-Verhalten)
void handleNotFound() {
  // Option 1: 302-Redirect auf "/"
  // server.sendHeader("Location", "/", true);
  // server.send(302, "text/plain", "");

  // Option 2: Direkt die Startseite ausliefern (meist angenehmer)
  handleHome();
}

// --------- Setup / Loop ---------

void setup() {
  Serial.begin(115200);
  delay(200);

  // Access Point mit fester IP starten
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

  // DNS-Server: Alle (Wildcard) Domains -> unsere lokale IP
  dnsServer.start(DNS_PORT, "*", local_ip);

  // Routen
  server.on("/", HTTP_GET, handleHome);
  server.on("/info", HTTP_GET, handleInfo);
  server.on("/kontakt", HTTP_GET, handleKontakt);

  // Captive-Portal-Trigger für gängige OS-Checks (je nach OS variieren die Pfade)
  // Android:
  server.on("/generate_204", HTTP_GET, handleCaptive);
  server.on("/gen_204", HTTP_GET, handleCaptive);
  // Apple/iOS/macOS:
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptive);
  // Windows:
  server.on("/ncsi.txt", HTTP_GET, handleCaptive);
  server.on("/connecttest.txt", HTTP_GET, handleCaptive);
  server.on("/redirect", HTTP_GET, handleCaptive);
  server.on("/fwlink", HTTP_GET, handleCaptive);

  // Catch-All
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP-Server läuft auf Port 80.");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
