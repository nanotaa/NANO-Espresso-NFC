/*********
Many thanks to James Coxon for websocket on yapraiwallet.space and for code on: https://github.com/jamescoxon/Nano_ESP8266_Trigger/blob/master/nano_esp8266_trig ger.ino 
Manuel Voltolini for integrating and customizing the code.
Paolo Saggin and Flavio Giongo for support and testing phase - admin@nanotaa.com  ********/
#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson
#include <WebSocketsClient.h>
#define ParamReset false
#define LED_PIN 16
#define MinAmount 1000000
#define AutoConnectSSID "NanoEspresso"
#define TempoErogazione 500 //in ms WebSocketsClient webSocket;
// Set web server port number to 80 WiFiServer server(80);
// Variable to store the HTTP request
String header;
// Auxiliar variables to store the current output state
String outputState = "off";
// Assign output variables to TRACKING_ADDRESS char output[65] = "";
//flag for saving data bool shouldSaveConfig = false;
//callback notifying us of the need to save config void saveConfigCallback () {
  Serial.println("Should save config");   shouldSaveConfig = true;
}
void setup() {
 
  // put your setup code here, to run once:
  Serial.begin(115200);
 
  //clean FS, for testing
  //SPIFFS.format();
  //read configuration from FS json   Serial.println("mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");     if (SPIFFS.exists("/config.json")) {       //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");       if (configFile) {
        Serial.println("opened config file");         size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.         std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());         json.printTo(Serial);         if (json.success()) {
          Serial.println("\nparsed json");           strcpy(output, json["output"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
 
  WiFiManagerParameter custom_output("output", "Nano Adress", output, 65);
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);  
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  //add all your parameters here
  wifiManager.addParameter(&custom_output);
 
  // Uncomment and run it once, if you want to erase all the stored information
 
  if (ParamReset) {     wifiManager.resetSettings();     SPIFFS.remove("/config.json");   }  
  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
 
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);
  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration   wifiManager.autoConnect(AutoConnectSSID);
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
 
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
 
  strcpy(output, custom_output.getValue());
  //save the custom parameters to FS   if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();     json["output"] = output;
    File configFile = SPIFFS.open("/config.json", "w");     if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    json.printTo(Serial);     json.printTo(configFile);     configFile.close();
    //end save
  }
  // Initialize the output variables as outputs   pinMode(LED_PIN, OUTPUT);   // Set outputs to LOW   digitalWrite(LED_PIN, LOW);
  //WiFi.disconnect();
  /*while(WiFiMulti.run() != WL_CONNECTED) {     delay(100);   }*/
  // server address, port and URL
  webSocket.begin("yapraiwallet.space", 80, "/call");
  // event handler
  webSocket.onEvent(webSocketEvent);
  // try ever 5000 again if connection has failed   webSocket.setReconnectInterval(5000);
 
  server.begin();
}
void loop() {
  // put your main code here, to run repeatedly:
  webSocket.loop();
} void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  //#define TRACKING_ADDRESS 
"nano_1wcxcjbwnnsdpee3d9i365e8bcj1uuyoqg9he5zjpt3r57dnjqe3gdc184ck";
   char TRACKING_ADDRESS[65] = "";
  DynamicJsonBuffer jsonBuffer;
  JsonObject& doc = jsonBuffer.createObject();
  File configFile = SPIFFS.open("/config.json", "r");   if (configFile) {
    size_t size = configFile.size();      std::unique_ptr<char[]> buf(new char[size]);     configFile.readBytes(buf.get(), size);
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(buf.get());     strcpy(TRACKING_ADDRESS, json["output"]);
    //Serial.printf("[WSc] Letto da file %s\n", TRACKING_ADDRESS);
  }      switch(type) {     case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");       break;     case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      // send message to server when Connected
           
      doc["address"] = TRACKING_ADDRESS;       doc["api_key"] = "0";       char output[512];
      //serializeJson(doc, output);
      doc.printTo(output);
     
      Serial.println(output);
      webSocket.sendTXT(output);       break;          case WStype_TEXT:
    {
      Serial.printf("[WSc] get text: %s\n", payload);       //deserializeJson(rx_doc, payload);
      JsonObject& rx_doc = jsonBuffer.parseObject(payload);
     
      String block_amount = rx_doc["amount"];
     
      long int noughtPointone = 100000;       long int noughtPointtwo = 200000;       Serial.println(block_amount);
      //Convert to nano
      int lastIndex = block_amount.length() - 24;       block_amount.remove(lastIndex);
      Serial.println(block_amount);
           if (block_amount.toInt() >= MinAmount) {
        Serial.println("Pin aperto");
       
        if (LED_PIN == 16) {           //Serial.println("1");           digitalWrite(LED_PIN,HIGH);           delay(TempoErogazione);           digitalWrite(LED_PIN, LOW);
        }         else {
          //Serial.println("2");           digitalWrite(LED_PIN,HIGH);           delay(2000);
          digitalWrite(LED_PIN, LOW);
        }
      }
      // send message to server
      // webSocket.sendTXT("message here");       break;     }     case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);       hexdump(payload, length);
      // send data to server
      // webSocket.sendBIN(payload, length);       break;     case WStype_ERROR:      
    case WStype_FRAGMENT_TEXT_START:     case WStype_FRAGMENT_BIN_START:     case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;   }
}
