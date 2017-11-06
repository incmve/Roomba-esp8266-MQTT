

/*
  https://github.com/incmve/roomba-esp8266/wiki
*/

// Includes
#include <Time.h>
#include <TimeLib.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <FS.h>
#include <MQTT.h>
#include <PubSubClient.h>

// webserver
ESP8266WebServer  server(80);
MDNSResponder   mdns;
WiFiClient client;

String roombotVersion = "0.6.0";

// WIFI
String ssid    = "SSID";
String password = "password";
String espName    = "Roombot";
// AP mode when WIFI not available
const char *APssid = "Roombot";
const char *APpassword = "thereisnospoon";
String ClientIP;

// MQTT
const char* mqttServer = "MQTTBroker";
const char* mqttUsername = "user";
const char* mqttPassword = "password";
const char* mqttClientId = "Roomba"; // Must be unique on the MQTT network
PubSubClient mqttclient(client);

// MQTT Topics
const char* commandtopic = "roomba/command";
const char* responsetopic = "roomba/response";

// Pimatic settings

long sendInterval  = 600000; //in millis
long lastInterval  = 0;
String WMode = "1";

#define SERIAL_RX     D5  // pin for SoftwareSerial RX
#define SERIAL_TX     D6  // pin for SoftwareSerial TX
SoftwareSerial mySerial(SERIAL_RX, SERIAL_TX); // (RX, TX. inverted, buffer)


// Div
File UploadFile;
String fileName;
String  BSlocal = "0";
int FSTotal;
int FSUsed;
String state;

//-------------- FSBrowser application -----------
//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}







// HTML
String header       =  "<html lang='en'><head><title>Roombot control panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='http://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='http://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";
String navbar       =  "<nav class='navbar navbar-default'><div class='container-fluid'><div class='navbar-header'><a class='navbar-brand' href='/'>Roombot control panel</a></div><div><ul class='nav navbar-nav'><li><a href='/'><span class='glyphicon glyphicon-info-sign'></span> Status</a></li><li class='dropdown'><a class='dropdown-toggle' data-toggle='dropdown' href='#'><span class='glyphicon glyphicon-cog'></span> Tools<span class='caret'></span></a><ul class='dropdown-menu'><li><a href='/updatefwm'><span class='glyphicon glyphicon-upload'></span> Firmware</a></li></ul></li><li><a href='https://github.com/incmve/roomba-eps8266/wiki' target='_blank'><span class='glyphicon glyphicon-question-sign'></span> Help</a></li></ul></div></div></nav>";
String containerStart   =  "<div class='container'><div class='row'>";
String containerEnd     =  "<div class='clearfix visible-lg'></div></div></div>";
String siteEnd        =  "</body></html>";

String panelHeaderName    =  "<div class='col-md-4'><div class='page-header'><h1>";
String panelHeaderEnd   =  "</h1></div>";
String panelEnd       =  "</div>";

String panelBodySymbol    =  "<div class='panel panel-default'><div class='panel-body'><span class='glyphicon glyphicon-";
String panelBodyName    =  "'></span> ";
String panelBodyValue   =  "<span class='pull-right'>";
String panelcenter   =  "<div class='row'><div class='span6' style='text-align:center'>";
String panelBodyEnd     =  "</span></div></div>";

String inputBodyStart   =  "<form action='' method='POST'><div class='panel panel-default'><div class='panel-body'>";
String inputBodyName    =  "<div class='form-group'><div class='input-group'><span class='input-group-addon' id='basic-addon1'>";
String inputBodyPOST    =  "</span><input type='text' name='";
String inputBodyClose   =  "' class='form-control' aria-describedby='basic-addon1'></div></div>";
String roombacontrol     =  "<a href='/roombastart'<button type='button' class='btn btn-default'><span class='glyphicon glyphicon-play' aria-hidden='true'></span> Start</button></a><a href='/roombadock'<button type='button' class='btn btn-default'><span class='glyphicon glyphicon-home' aria-hidden='true'></span> Dock</button></a><br><a href='/restart'<button type='button' class='btn btn-default'><span class='glyphicon glyphicon-sort' aria-hidden='true'></span> Reset roombot</button></a></div>";


// ROOT page
void handle_root()
{

  // get IP
  IPAddress ip = WiFi.localIP();
  ClientIP = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  delay(500);

  String title1     = panelHeaderName + String("Roombot Settings") + panelHeaderEnd;
  String IPAddClient    = panelBodySymbol + String("globe") + panelBodyName + String("IP Address") + panelBodyValue + ClientIP + panelBodyEnd;
  String ClientName   = panelBodySymbol + String("tag") + panelBodyName + String("Client Name") + panelBodyValue + espName + panelBodyEnd;
  String Version     = panelBodySymbol + String("info-sign") + panelBodyName + String("Roombot Version") + panelBodyValue + roombotVersion + panelBodyEnd;
  String Uptime     = panelBodySymbol + String("time") + panelBodyName + String("Uptime") + panelBodyValue + hour() + String(" h ") + minute() + String(" min ") + second() + String(" sec") + panelBodyEnd + panelEnd;


  String title2     = panelHeaderName + String("Pimatic server") + panelHeaderEnd;
  String IPAddServ    = panelBodySymbol + String("globe") + panelBodyName + String("IP Address") + panelBodyValue + panelBodyEnd;
  //  String User     = panelBodySymbol + String("user") + panelBodyName + String("Username") + panelBodyValue + Username + panelBodyEnd + panelEnd;


  String title3 = panelHeaderName + String("Commands") + panelHeaderEnd;
  String commands = panelBodySymbol + panelBodyName + panelcenter + roombacontrol + panelBodyEnd;


  server.send ( 200, "text/html", header + navbar + containerStart + title1 + IPAddClient + ClientName + Version + Uptime + title3 + commands + containerEnd + siteEnd);
}




// Setup
void setup(void)
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  mySerial.begin(115200);
  pinMode(SERIAL_RX, INPUT);
  pinMode(SERIAL_TX, OUTPUT);
  // Check if SPIFFS is OK
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS failed, needs formatting");
    handleFormat();
    delay(500);
    ESP.restart();
  }
  else
  {
    FSInfo fs_info;
    if (!SPIFFS.info(fs_info))
    {
      Serial.println("fs_info failed");
    }
    else
    {
      FSTotal = fs_info.totalBytes;
      FSUsed = fs_info.usedBytes;
    }
  }
  WiFi.hostname(espName);
  WiFi.begin(ssid.c_str(), password.c_str());
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 31)
  {
    delay(1000);
    Serial.print(".");
    ++i;
  }
  if (WiFi.status() != WL_CONNECTED && i >= 30)
  {
    WiFi.disconnect();
    delay(1000);
    Serial.println("");
    Serial.println("Couldn't connect to network :( ");
    Serial.println("Setting up access point");
    Serial.println("SSID: ");
    Serial.println(APssid);
    Serial.println("password: ");
    Serial.println(APpassword);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(APssid, APpassword);
    WMode = "AP";
    Serial.print("Connected to ");
    Serial.println(APssid);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("IP address: ");
    Serial.println(myIP);

  }
  else
  {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(espName);

  }

  server.on ( "/format", handleFormat );
  server.on("/", handle_root);
  server.on("/", handle_fupload_html);
  server.on("/api", handle_api);
  server.on("/updatefwm", handle_updatefwm_html);
  server.on("/fupload", handle_fupload_html);
  server.on("/roombastart", handle_roomba_start);
  server.on("/roombadock", handle_roomba_dock);
  server.on("/restart", handle_esp_restart);



  // Upload firmware:
  server.on("/updatefw2", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []()
  {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
      fileName = upload.filename;
      Serial.setDebugOutput(true);
      Serial.printf("Update: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
      {
        Update.printError(Serial);
      }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      if (Update.end(true)) //true to set the size to the current progress
      {
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      }
      else
      {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);

    }
    yield();
  });
  // upload file to SPIFFS
  server.on("/fupload2", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
      fileName = upload.filename;
      Serial.setDebugOutput(true);
      //fileName = upload.filename;
      Serial.println("Upload Name: " + fileName);
      String path;
      if (fileName.indexOf(".css") >= 0)
      {
        path = "/css/" + fileName;
      }
      else if (fileName.indexOf(".js") >= 0)
      {
        path = "/js/" + fileName;
      }
      else if (fileName.indexOf(".otf") >= 0 || fileName.indexOf(".eot") >= 0 || fileName.indexOf(".svg") >= 0 || fileName.indexOf(".ttf") >= 0 || fileName.indexOf(".woff") >= 0 || fileName.indexOf(".woff2") >= 0)
      {
        path = "/fonts/" + fileName;
      }
      else
      {
        path = "/" + fileName;
      }
      UploadFile = SPIFFS.open(path, "w");
      // already existing file will be overwritten!
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      if (UploadFile)
        UploadFile.write(upload.buf, upload.currentSize);
      Serial.println(fileName + " size: " + upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      Serial.print("Upload Size: ");
      Serial.println(upload.totalSize);  // need 2 commands to work!
      if (UploadFile)
        UploadFile.close();
    }
    yield();
  });


  if (!mdns.begin(espName.c_str(), WiFi.localIP())) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  server.begin();
  Serial.println("HTTP server started");
  //MQTT
  mqttclient.setServer(mqttServer, 1883);
  mqttclient.setCallback(callback);
}




// LOOP
void loop(void)
{
  if (mySerial.available()) {
    Serial.print(mySerial.read());
  }
  server.handleClient();
  /*
    if (millis() - lastInterval > sendInterval) //update pimatic
    {
    handle_charging_state();
    handle_voltage();
    handle_charging_sources();
    lastInterval = millis();
    }
  */
  if (!mqttclient.connected()) {
    reconnect();
  }
  mqttclient.loop();
}


// handles

void handle_api()
{
  // Get vars for all commands
  String action = server.arg("action");
  String value = server.arg("value");
  String api = server.arg("api");

  if (action == "clean" && value == "start")
  {
    handle_roomba_start();

  }

  if (action == "dock" && value == "home")
  {
    handle_roomba_dock();
  }
  if (action == "reset" && value == "true")
  {
    server.send ( 200, "text/html", "Reset ESP OK");
    delay(500);
    Serial.println("RESET");
    ESP.restart();
  }
  if (action == "charge")
  {
    handle_charging_state();
  }
  if (action == "voltage")
  {
    handle_voltage();
  }
  if (action == "chargesource")
  {
    handle_charging_sources();
  }
}

void handle_updatefwm_html()
{
  server.send ( 200, "text/html", "<form method='POST' action='/updatefw2' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form><br<b>For firmware only!!</b>");
}


void handle_fupload_html()
{
  String HTML = "<br>Files on flash:<br>";
  Dir dir = SPIFFS.openDir("/");
  while (dir.next())
  {
    fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    HTML += fileName.c_str();
    HTML += " ";
    HTML += formatBytes(fileSize).c_str();
    HTML += " , ";
    HTML += fileSize;
    HTML += "<br>";
    //Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }

  server.send ( 200, "text/html", "<form method='POST' action='/fupload2' enctype='multipart/form-data'><input type='file' name='update' multiple><input type='submit' value='Update'></form><br<b>For webfiles only!!</b>Multiple files possible<br>" + HTML);
}



void handle_update_upload()
{
  if (server.uri() != "/update2") return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.setDebugOutput(true);
    Serial.printf("Update: %s\n", upload.filename.c_str());
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) { //start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { //true to set the size to the current progress
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
    Serial.setDebugOutput(false);
  }
  yield();
}
void handle_update_html2()
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  ESP.restart();
  
  handle_root();
}

void handleFormat()
{
  server.send ( 200, "text/html", "OK");
  Serial.println("Format SPIFFS");
  if (SPIFFS.format())
  {
    if (!SPIFFS.begin())
    {
      Serial.println("Format SPIFFS failed");
    }
  }
  else
  {
    Serial.println("Format SPIFFS failed");
  }
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS failed, needs formatting");
  }
  else
  {
    Serial.println("SPIFFS mounted");
  }
}


void handle_roomba_start()
{
  Serial.println("Starting");
  mySerial.write(128);
  delay(50);
  mySerial.write(131);
  delay(50);
  mySerial.write(135);
  Serial.println("I will clean master");
  handle_root();
  mqttclient.publish(responsetopic, String("Start cleaning").c_str());
}

void handle_roomba_dock()
{
  mySerial.write(128);
  delay(50);
  mySerial.write(131);
  delay(50);
  mySerial.write(143);
  Serial.println("Thank you for letting me rest, going home master");
  handle_root();
  mqttclient.publish(responsetopic, String("Dock").c_str());
}


void handle_esp_restart() {
  ESP.restart();
  handle_root();
}

void handle_charging_state() {
  //
  int charge = 0;
  // int data;
  Serial.println("Start handle_charging_state");
  mySerial.write(142);
  delay(50);
  mySerial.write(21);
  delay(50);
  if (mySerial.available()) {
    charge = Serial.read();
    Serial.println("..");
    Serial.print(charge);
    switch (charge) {
      case 0: {
          String data = "Not Charging";
          mqttclient.publish(responsetopic, String(data).c_str());

          break;
        }
      case 1: {
          String data = "Reconditioning";
          mqttclient.publish(responsetopic, String(data).c_str());
          break;
        }
      case 2: {
          String data = "Full Charging";
          mqttclient.publish(responsetopic, String(data).c_str());
          break;
        }
      case 3: {
          String data = "Trickle Charging";
          mqttclient.publish(responsetopic, String(data).c_str());
          break;
        }
      case 4: {
          String data = "Waiting";
          mqttclient.publish(responsetopic, String(data).c_str());
          break;
        }
      case 5: {

          String data = "Charging Fault Condition";
          String variable = String(charge);
          mqttclient.publish(responsetopic, String(data).c_str());

          break;
        }
      default:
        // if nothing else matches, do the default
        // default is optional
        mqttclient.publish(responsetopic, String("no info").c_str());
        break;
    }

  }
    else {
      mqttclient.publish(responsetopic, String("no info").c_str());
    }
}

void handle_voltage() {
  Serial.println("Voltage");
  String voltage = "0";
  mySerial.write(142);
  delay(50);
  mySerial.write(22);
  delay(50);
  if (mySerial.available()) {
    Serial.println("..");
    Serial.print(Serial.read());
    String data = String(Serial.read());
    mqttclient.publish(responsetopic, String(data).c_str());
  }
  else {
    mqttclient.publish(responsetopic, String("no info").c_str());
  }

}


void handle_charging_sources() {
  Serial.println("Charging source");
  int source;
  mySerial.write(142);
  delay(50);
  mySerial.write(34);
  delay(50);
  if (mySerial.available()) {
    source = Serial.read();
    Serial.println("..");
    Serial.print(source);
    switch (source) {
      case 0: {
          String data = "Not Charging";

          mqttclient.publish(responsetopic, String(data).c_str());
          break;
        }
      case 1: {
          String data = "Reconditioning";
          mqttclient.publish(responsetopic, String(data).c_str());
          break;
        }
      default:
        // if nothing else matches, do the default
        // default is optional
        mqttclient.publish(responsetopic, String("no info").c_str());
        break;
    }
  }

}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  Serial.println("payload");
  payload[length] = '\0';
  String strPayload = String((char*)payload);
  Serial.println(strPayload);

  if (strPayload == "clean") {
    handle_roomba_start();
  }
  else if (strPayload == "dock") {
    handle_roomba_dock();
  }
  else if (strPayload == "charge") {
    handle_charging_state();
  }
  else if (strPayload == "voltage") {
    handle_voltage();
  }
  else if (strPayload == "chargesource") {
    handle_charging_sources();
  }
  else if (strPayload == "reset") {
    ESP.restart();
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttclient.connect(mqttClientId, mqttUsername, mqttPassword)) {
      Serial.println("connected");
      // subscribe
      mqttclient.subscribe(commandtopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

