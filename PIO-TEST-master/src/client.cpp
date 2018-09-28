
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>


//needed for library
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager


#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <NewPingESP8266.h>

#define TRIGGER_PIN  13  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     12  // Arduino pin tied to echo pin on the ultrasonic sensor.
 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define VALVE_PIN 4
#define CM_OFF 80
#define CM_ON 250
#define SWITCH_PIN 5

String host_souyuz = "http://soyuz.teatrtogo.ru/";

unsigned long FULL_TANK_DISTANCE = 20.0;
unsigned long EMPTY_TANK_DISTANCE = 320.0;

unsigned int MAX_DISTANCE = EMPTY_TANK_DISTANCE - FULL_TANK_DISTANCE;

int DELTA_DISTANCE = 5;
unsigned int VALVE_LEVEL_OPEN = 50;
unsigned int VALVE_LEVEL_CLOSE = 80;
int delta;
unsigned int value = 0;
unsigned int distance_cm;
unsigned long percent_full;
unsigned int switch_pin_state = 0;

bool is_open = false;
unsigned long ping_time = 0;
NewPingESP8266 sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPingESP8266 setup of pins and maximum distance.

HTTPClient http;

const uint16_t aport = 23;
WiFiServer TelnetServer(aport);
WiFiClient Telnet;

String last_payload = "";
void report_water_level();
void printDebug();
void close_valve();
void open_valve();
String values_toString();
void printToTelnet();

void setup() {
  delay(500);
  pinMode(SWITCH_PIN, INPUT);
    //Local intialization. Once its business is done, there is no need to keep it around
   

  TelnetServer.begin();
     TelnetServer.setNoDelay(true);
 
  USE_SERIAL.begin(115200);
  // USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  WiFi.mode(WIFI_STA);
   
  WiFi.begin("soyuz", "89626866191");
 

 while (WiFi.status() != WL_CONNECTED) {
    delay(500);
     USE_SERIAL.print(".");
  }

   while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

     // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
//  ArduinoOTA.setHostname("myesp8266");
  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
pinMode(VALVE_PIN, OUTPUT);
close_valve();

  USE_SERIAL.println("");
  USE_SERIAL.print("Connected to ");
  USE_SERIAL.println("soyuz");
  USE_SERIAL.print("IP address: ");
  USE_SERIAL.println(WiFi.localIP());
   WiFi.setAutoReconnect(true);// if (Wifi.isConnected()..setA)
  http.setReuse(true);
  wifi_config_ap();
}



void wifi_config_ap() {
      //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //reset settings - for testing
    //wifiManager.resetSettings();

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifiManager.setTimeout(120);

}
void loop() {
switch_pin_state = digitalRead(SWITCH_PIN);
	ArduinoOTA.handle();
ping_time = sonar.ping_median(5);
distance_cm = sonar.convert_cm(ping_time);
percent_full = map(distance_cm, EMPTY_TANK_DISTANCE, FULL_TANK_DISTANCE, 0.0, 100.0);
delta = abs(value-percent_full);
value = distance_cm; 

 // int new_value = 100*(EMPTY_TANK_DISTANCE-FULL_TANK_DISTANCE-sonar.convert_cm(sonar.ping_median(5,EMPTY_TANK_DISTANCE)))/MAX_DISTANCE;
//if (distance_cm){
//    value = percent_full;
//  printToTelnet();
  //  printToTelnet(values_toString());
    //USE_SERIAL.println(value);
//    report_water_level();
//}


if (distance_cm <= CM_OFF){
	if (is_open){	
		close_valve();
	}

}
if (distance_cm >= CM_ON){
	if (!is_open){
		open_valve();
	}
}
printToTelnet();
  delay(100);
}

//if Telnet.
//String host_opiz = "http://192.168.1.130/";

void report_water_level(){
    
   
    if (WiFi.isConnected()) {

    USE_SERIAL.print("[HTTP] begin...\n");
    String g = host_souyuz;
    g+=  "api/soyuz/update.php?level=";
    g+=value;
    g+="&station=US_LEVEL"; //HTTP
    g+="&ping=";
    g+=ping_time;
    g+="&delta=";
    g+=delta;
    g+="&is_open=";
    g+=is_open;
    g+="&local_IP=";
    g+=WiFi.localIP().toString();


    http.begin(g);
    USE_SERIAL.print("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        //USE_SERIAL.println(payload);
	
	last_payload += value;
	last_payload += " LED:";
	if (digitalRead(2) == LOW){
		last_payload += "ON";
	}
	else 
	{
        	last_payload+= "OFF";
	}
  last_payload += " is_open=";
  last_payload += is_open;
	last_payload += "\n";
	last_payload += payload;
	printDebug();
	    		     
}
    } else {
      USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
   	
 }
    http.end();
  }
    else 
    {
	USE_SERIAL.println("REBOOT");
 	ESP.restart();
    }
  
}

void open_valve() {
digitalWrite(VALVE_PIN, HIGH);
is_open = true;
last_payload = "VALVE - OPENED";
printDebug();
}

void close_valve(){
digitalWrite(VALVE_PIN, LOW);
is_open = false;
last_payload = "VALVE - CLOSED";
printDebug();
}

void printDebug(){
//Prints to telnet if connected
 // Serial.println(last_payload);
 if (!Telnet) {  // otherwise it works only once
        Telnet = TelnetServer.available();
 }
       	if (Telnet.connected()) {
    Telnet.println(last_payload);
  last_payload = "";
	}  

}

String values_toString(){
    String g = host_souyuz;
    g+=  "api/soyuz/update.php?\n";
    g+= "level=";
    g+=value;
    g+="&station=US_LEVEL"; //HTTP
    g+="&ping=";
    g+=ping_time;
    g+="&delta=";
    g+=delta;
    g+="&is_open=";
    g+=is_open;
    g+="&local_IP=";
    g+=WiFi.localIP().toString();
    g+= "\nFULL_TANK_DISTANCE ";
    g+=FULL_TANK_DISTANCE;// = 20.0;
    g+= "\nEMPTY_TANK_DISTANCE ";
g+= EMPTY_TANK_DISTANCE;
g+= "\nDELTA_DISTANCE ";
g+= DELTA_DISTANCE;
g+= "\nVALVE_LEVEL_OPEN ";
g+= VALVE_LEVEL_OPEN;
g+= "\nVALVE_LEVEL_CLOSE ";
g+= VALVE_LEVEL_CLOSE;
return g;
}
  

void printToTelnet(){
  String g = "";
 
  
long ddm = ping_time;//distance_cm;//sonar.ping_median(5, MAX_DISTANCE);
g+= ddm; 
g+= " = ";
 g+= distance_cm;
g+= " - ";
g+= switch_pin_state;
 g+="\n";

       //String g = host_souyuz;
   // g+=  "api/soyuz/update.php?\n";
    //g+= "level=";
    //g+=value;
    //g+="&station=US_LEVEL"; //HTTP
    //g+="&ping=";
    //g+=ping_time;
    //g+="&delta=";
    //g+=delta;
    //g+="&is_open=";
    //g+=is_open;
    //g+="&local_IP=";
    //g+=WiFi.localIP().toString();
   // g+= "\nFULL_TANK_DISTANCE ";
   // g+=FULL_TANK_DISTANCE;// = 20.0;
   // g+= "\nEMPTY_TANK_DISTANCE ";
//g+= EMPTY_TANK_DISTANCE;
//g+= "\nDELTA_DISTANCE ";
//g+= DELTA_DISTANCE;
//g+= "\nVALVE_LEVEL_OPEN ";
//g+= VALVE_LEVEL_OPEN;
//g+= "\nVALVE_LEVEL_CLOSE ";
//g+= VALVE_LEVEL_CLOSE;

 if (!Telnet) {  // otherwise it works only once
        Telnet = TelnetServer.available();
 }
       	if (Telnet.connected()) {
    Telnet.println(g);
	}  

}
