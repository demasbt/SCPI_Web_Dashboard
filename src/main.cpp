#include <stdlib.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h>
#include <FS.h>
#include <Arduino_JSON.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Replace with your network credentials
const char* ssid     = "ESP32-AP";
const char* password = "123456789";

const char* PARAM_INPUT_1 = "sample_num";
const char* PARAM_INPUT_2 = "scpi_opt";
const char* PARAM_INPUT_3 = "bode_plot";
const char* PARAM_INPUT_4 = "freq_avg";
const char* PARAM_INPUT_5 = "vpp_avg";
const char* PARAM_INPUT_6 = "id_device";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 500;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");
// Json Variable to Hold SCPI Readings
JSONVar SCPIVal;
// Function to read SCPI values and return as JSONVar
String passjsonval(){
  JSONVar SCPIValue;
  String receivedvpp1, receivedvpp2, receivedph, receivedfreq, jsonString;

  // Read SCPI values from Serial2
  Serial2.write(":MEAS:SOUR1 CH1\n:MEAS:PK2P?\n");
  receivedvpp1 = Serial2.readStringUntil('\n');
  SCPIValue["magnitude1"] = atof(receivedvpp1.c_str());

  Serial2.write(":MEASU:MEAS2:SOU2 CH2\n:MEASU:MEAS2:TYP PK2\n:MEASU:MEAS2:STATE ON\n:MEASU:MEAS2:VAL?\n");
  receivedvpp2 = Serial2.readStringUntil('\n');
  SCPIValue["magnitude2"] = atof(receivedvpp2.c_str());

  Serial2.write(":MEAS:FREQ?\n");
  receivedfreq = Serial2.readStringUntil('\n');
  SCPIValue["frequency"] = atof(receivedfreq.c_str());

  Serial2.write(":MEAS:SOUR1 CH1\n:MEAS:SOUR2 CH2\n:MEAS:PHA?\n");
  receivedph = Serial2.readStringUntil('\n');
  SCPIValue["phase"] = atof(receivedph.c_str());
  
  
  jsonString = JSON.stringify(SCPIValue);
  return jsonString;
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    //data[len] = 0;
    //String message = (char*)data;
    // Check if the message is "getReadings"
    //if (strcmp((char*)data, "getReadings") == 0) {
      //if it is, send current sensor readings
      String sensorReadings = passjsonval();
      Serial.print(sensorReadings);
      notifyClients(sensorReadings);
    //}
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

void initOLED(){
  //display initiation
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
}

void initWiFi(){
  //WiFi initiation
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void oled_text(IPAddress IPs, int client){
  //Display static text oled
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("SSID :");
  display.println(ssid);
  display.print("Pass :");
  display.println(password);
  display.print("IP   :");
  display.println(IPs);
  display.print("Connected clients: ");
  display.println(client);
  display.display();
}


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  // Serial port for SCPI communication
  Serial2.begin(19200);

  initOLED();
  initWiFi();
  initLittleFS();
  initWebSocket();

  // Route for highcharts.js library
  server.on("/h.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/highcharts.js", "text/javascript");
  });/*
  server.on("/d.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/dataexport.js", "text/javascript");
  });
  server.on("/i.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/imgexport.js", "text/javascript");
  });*/

  // Route for root / web page1
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/realtime.html", "text/html");
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/get", HTTP_POST, [] (AsyncWebServerRequest *request) {
    String inputOption;
    String inputParam;
    int inputSamplenum;
    String inputParam2;
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1, true)) {
      inputSamplenum = request->getParam(PARAM_INPUT_1, true)->value().toInt();
      inputParam = PARAM_INPUT_1;
      inputOption = request->getParam(PARAM_INPUT_2, true)->value();
      inputParam2 = PARAM_INPUT_2;
    }
    else {
      inputOption = "none";
      inputSamplenum = 0;
      
    }
    /*request->send(200, "text/html", "HTTP POST request sent to your ESP on input field (" 
                                     + inputOption + ") with value: " + inputSamplenum +
                                     "<br><a href=\"/\">Return to Home Page</a>");*/
    
    request->send(LittleFS, "/bode.html", "text/html");
    Serial.println(inputOption);
    Serial.println(inputSamplenum);
  });

  server.on("/readSCPI", HTTP_ANY, [](AsyncWebServerRequest *request){
    String jsonString = passjsonval();
    request->send(200, "application/json", jsonString);
  });

  server.serveStatic("/", LittleFS, "/");
  // Start server
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  int connectedClients = WiFi.softAPgetStationNum();
  IPAddress IP = WiFi.softAPIP();
  oled_text(IP, connectedClients);
  delay(500);

  if ((millis() - lastTime) > timerDelay) {
    String sensorReadings = passjsonval();
    Serial.println(sensorReadings);
    notifyClients(sensorReadings);
    lastTime = millis();
  }
  ws.cleanupClients();
}