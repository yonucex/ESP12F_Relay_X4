//#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <BlynkSimpleEsp8266.h>

#include <SimpleTimer.h>
#include <ModbusMaster.h>

#define BLYNK_PRINT Serial
#define RX2 05
#define TX2 04

#ifndef STASSID
#define STASSID "ได้หมดถ้าสดชื่น"
#define STAPSK "ได้หมดถ้าสดชื่น"
#endif

const char* softwareVersion = "1.0.4";
const char* host = "esp8266-webupdate-esp12f-2004";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "ได้หมดถ้าสดชื่น";
const char* ssid = STASSID;
const char* password = STAPSK;
const char* hotspot_ssid = "esp2004"; // ชื่อ SSID สำหรับ Hotspot
const char* hotspot_password = "ได้หมดถ้าสดชื่น"; // รหัสผ่านสำหรับ Hotspot
char auth[] = "ได้หมดถ้าสดชื่น";


ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

#define relay1 16
#define relay2 14
#define relay3 12
#define relay4 13

bool relayStates[4] = {false, false, false, false};
WidgetTerminal terminal(V0);
SimpleTimer timer;


void setupWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(500);
    Serial.println("WiFi failed, retrying.");
    WiFi.begin(ssid, password);
  }
  WiFi.softAP(hotspot_ssid, hotspot_password);
  Serial.print("Hotspot SSID: ");
  Serial.println(hotspot_ssid);
  Serial.print("Hotspot IP Address: ");
  Serial.println(WiFi.softAPIP());
}


void setupRelays() {
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, LOW);
  digitalWrite(relay4, LOW);
}

void toggleRelay(int relay, bool &state) {
  state = !state;
  digitalWrite(relay, state ? HIGH : LOW);
  httpServer.sendHeader("Location", "/relay");
  httpServer.send(303);
}

void setupWebServerRoutes() {
  httpServer.on("/", HTTP_GET, []() {
    String htmlResponse = "<html><head><style>"
                          "body { font-family: Arial; text-align: center; }"
                          "h1 { color: #0f6fc6; }"
                          ".button { background-color: #4CAF50; border: none; color: white; padding: 15px 32px;"
                          "text-align: center; text-decoration: none; display: inline-block; font-size: 16px;"
                          "margin: 4px 2px; cursor: pointer; border-radius: 12px; }"
                          ".button-off { background-color: #f44336; }"
                          "</style></head><body>"
                          "<h1>ESP8266 Web Update</h1>"
                          "<p>HOST: " + String(host) + "</p>"
                          "<p>IP Address: " + WiFi.localIP().toString() + "</p>"
                          "<p>Software Version: " + String(softwareVersion) + "</p>"
                          "<p><a href=\"/relay\">Control Relays</a></p>"
                          "</body></html>";
    httpServer.send(200, "text/html", htmlResponse);
  });

  httpServer.on("/relay", HTTP_GET, []() {
    String message = "<html><head><style>"
                     "body { font-family: Arial; text-align: center; }"
                     "h1 { color: #0f6fc6; }"
                     ".switch { position: relative; display: inline-block; width: 60px; height: 34px; }"
                     ".switch input { opacity: 0; width: 0; height: 0; }"
                     ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0;"
                     "background-color: #ccc; -webkit-transition: .4s; transition: .4s; }"
                     ".slider:before { position: absolute; content: ''; height: 26px; width: 26px; left: 4px;"
                     "bottom: 4px; background-color: white; -webkit-transition: .4s; transition: .4s; }"
                     "input:checked + .slider { background-color: #2196F3; }"
                     "input:focus + .slider { box-shadow: 0 0 1px #2196F3; }"
                     "input:checked + .slider:before { -webkit-transform: translateX(26px);"
                     "-ms-transform: translateX(26px); transform: translateX(26px); }"
                     ".slider.round { border-radius: 34px; }"
                     ".slider.round:before { border-radius: 50%; }"
                     "</style></head><body>"
                     "<h1>ESP8266 Relay Control</h1>";

    for (int i = 0; i < 4; i++) {
      message += "<label class=\"switch\"><input type=\"checkbox\" onclick=\"location.href='/relay/toggle" + String(i + 1) + "'\"" + (relayStates[i] ? " checked" : "") + "><span class=\"slider round\"></span></label>"
                 "<p>Relay " + String(i + 1) + "</p>";
    }

    message += "</body></html>";
    httpServer.send(200, "text/html", message);
  });

  httpServer.on("/relay/toggle1", HTTP_GET, []() { toggleRelay(relay1, relayStates[0]); });
  httpServer.on("/relay/toggle2", HTTP_GET, []() { toggleRelay(relay2, relayStates[1]); });
  httpServer.on("/relay/toggle3", HTTP_GET, []() { toggleRelay(relay3, relayStates[2]); });
  httpServer.on("/relay/toggle4", HTTP_GET, []() { toggleRelay(relay4, relayStates[3]); });
}

void setup() {
  Serial.begin(9600);
  setupWiFi();
  //timer.setInterval(60000L, setupWiFi); // ตั้งเวลาเชื่อมต่อ WiFi ทุก 1 นาที
  Serial.println("\nWiFi connected");
  setupRelays();
  setupWebServerRoutes();
  Blynk.config(auth, "ได้หมดถ้าสดชื่น",8980);

    if (Blynk.connected()) {
    Serial.println("Connected to Blynk server");
  } else {
    Serial.println("Failed to connect to Blynk server");
  }

  MDNS.begin(host);
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser\n", host, update_path);
  
  terminal.clear();

  // This will print Blynk Software version to the Terminal Widget when
  // your hardware gets connected to Blynk Server
  terminal.println(F("Blynk v" BLYNK_VERSION ": Device started"));
  terminal.println(F("-------------"));
  terminal.println(F("Type 'Marco' and get a reply, or type"));
  terminal.println(F("anything else and get it printed back."));
  terminal.flush();
}

void loop() {
  httpServer.handleClient();
  MDNS.update();
  if (WiFi.status() == WL_CONNECTED) {
    if (!Blynk.connected()) {
      Blynk.connect();
    }}

  Blynk.run();
  timer.run();
  if (Serial.available() > 0) {
    // อ่านข้อมูลจาก Serial
    String input = Serial.readStringUntil('\n');
    input.trim(); // ลบช่องว่างหรือขึ้นบรรทัดใหม่ที่อาจมีอยู่
    Serial.print("I received: ");
    Serial.println(input);
    
    // ตรวจสอบคำสั่ง
    if (input == "relay1=HIGH") {
      digitalWrite(relay1, HIGH); // ตั้งค่า relay1 เป็น HIGH
      Serial.println("Relay 1 set to HIGH");
    } else if (input == "relay2=HIGH") {
      digitalWrite(relay2, HIGH); // ตั้งค่า relay2 เป็น HIGH
      Serial.println("Relay 2 set to HIGH");
    } else if (input == "relay3=HIGH") {
      digitalWrite(relay3, HIGH); // ตั้งค่า relay3 เป็น HIGH
      Serial.println("Relay 3 set to HIGH");
    } else if (input == "relay4=HIGH") {
      digitalWrite(relay4, HIGH); // ตั้งค่า relay4 เป็น HIGH
      Serial.println("Relay 4 set to HIGH");
    }
  }

}

// ฟังก์ชัน Blynk
BLYNK_WRITE(V0) {

  // if you type "Marco" into Terminal Widget - it will respond: "Polo:"
  if (String("Marco") == param.asStr()) {
    terminal.println("You said: 'Marco'") ;
    terminal.println("I said: 'Polo'") ;
  } else {

    // Send it back
    terminal.print("You said:");
    terminal.write(param.getBuffer(), param.getLength());
    terminal.println();
  }

  // Ensure everything is sent
  terminal.flush();
}

BLYNK_WRITE(V1) {
  relayStates[0] = param.asInt();
  digitalWrite(relay1, relayStates[0] ? HIGH : LOW);
}

BLYNK_WRITE(V2) {
  relayStates[1] = param.asInt();
  digitalWrite(relay2, relayStates[1] ? HIGH : LOW);
}

BLYNK_WRITE(V3) {
  relayStates[2] = param.asInt();
  digitalWrite(relay3, relayStates[2] ? HIGH : LOW);
}

BLYNK_WRITE(V4) {
  relayStates[3] = param.asInt();
  digitalWrite(relay4, relayStates[3] ? HIGH : LOW);
}

BLYNK_WRITE(V5) {
  relayStates[0] = param.asInt();
  digitalWrite(relay1, relayStates[0] ? HIGH : LOW);
}

BLYNK_WRITE(V6) {
  relayStates[1] = param.asInt();
  digitalWrite(relay2, relayStates[1] ? HIGH : LOW);
}
