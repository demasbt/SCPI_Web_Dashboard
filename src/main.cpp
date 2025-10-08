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

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
JSONVar SCPIVal;

JSONVar passjsonval(){
  JSONVar SCPIValue;
  String receivedvpp, receivedfreq;
  float vppval, freqval;
  Serial2.write(":MEAS:SOUR 1;:MEAS:VPP?\n");
  receivedvpp = Serial2.readStringUntil('\n');
  vppval = atof(receivedvpp.c_str());
  Serial2.write(":MEAS:SOUR 1;:MEAS:FREQ?\n");
  receivedfreq = Serial2.readStringUntil('\n');
  freqval = atof(receivedfreq.c_str());
  SCPIValue["magnitude"] = vppval;
  SCPIValue["frequency"] = receivedfreq;
  return SCPIValue;
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
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

float SCPI_Magnitude(){
  String receivedvpp, receivedfreq;
  Serial2.write(":MEAS:SOUR 1;:MEAS:VPP?\n");
  receivedvpp = Serial2.readStringUntil('\n');
  Serial2.write(":MEAS:SOUR 1;:MEAS:FREQ?\n");
  receivedfreq = Serial2.readStringUntil('\n');
  return atof(receivedfreq.c_str());
}

float SCPI_Frequency(){
  String receivedChar;
  Serial2.write(":MEAS:SOUR 1;:MEAS:FREQ?\n");
  receivedChar = Serial2.readString();
  float my_float_value = atof(receivedChar.c_str());
  return my_float_value;
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial2.begin(19200);

  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  //display initiation
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  
 // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

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
  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/bode.html", "text/html");
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
                                     "<br><a href=\"/\">Return to Home Page</a>");
    */
    request->send(LittleFS, "/bode.html", "text/html");
    Serial.println(inputOption);
    Serial.println(inputSamplenum);
  });

  server.on("/readSCPI", HTTP_ANY, [](AsyncWebServerRequest *request){
    //SCPIVal = passjsonval();
    SCPIVal["frequency"] = random(0,1000);
    SCPIVal["magnitude1"] = random(0,10);
    SCPIVal["magnitude2"] = random(0,10);
    SCPIVal["phase"] = random(-90,90);
    String jsonString = JSON.stringify(SCPIVal);
    request->send(200, "application/json", jsonString);
  });

  // Start server
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  int connectedClients = WiFi.softAPgetStationNum();
  IPAddress IP = WiFi.softAPIP();
  oled_text(IP, connectedClients);
  Serial.println(passjsonval());
  delay(500);
}