
#include <Arduino.h>
//#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
//needed for library
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#define USE_SERIAL Serial

//ESP8266WiFiMulti WiFiMulti;
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <RunningMedian.h>

#include <NewPingESP8266.h>

#define TRIGGER_PIN  13  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     12  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define VALVE_PIN 4
#define CM_OFF 80
#define CM_ON 200
#define SWITCH_PIN 5
#define MAX_DELTA 10
#define LED_PIN 2
#define MEDIAN_MAX_SIZE 49
String host_souyuz = "http://soyuz.teatrtogo.ru/";

unsigned long FULL_TANK_DISTANCE = 20.0;
unsigned long EMPTY_TANK_DISTANCE = 320.0;

unsigned int MAX_DISTANCE = EMPTY_TANK_DISTANCE - FULL_TANK_DISTANCE;
  unsigned long report_interval = 10000;
   unsigned long ping_interval = 100;
  unsigned long last_ping_time = 0;
   unsigned long last_report_time = 0;
 
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
    
NewPingESP8266 sonar(TRIGGER_PIN, ECHO_PIN, EMPTY_TANK_DISTANCE); // NewPingESP8266 setup of pins and maximum distance.
RunningMedian samples = RunningMedian(MEDIAN_MAX_SIZE);
HTTPClient http;
long pcm = 0;
const uint16_t aport = 23;
WiFiServer TelnetServer(aport);
WiFiClient Telnet;
WiFiManager wifiManager;
String local_ip ="1.1.1.1";
String last_payload = "";
String payload = "";
bool calibrated = false;
const int DELTA_OUTLIERS = 30; 

void report_water_level();
void printDebug();
void close_valve();
void open_valve();
void ota_setup();
String values_toString();
void printToTelnet();
void wifi_config_ap();
void pinMode_setup();
void read_switch_pin();
void valve_control();

void setup() {
  delay(500);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
pinMode(VALVE_PIN, OUTPUT);
close_valve();


  wifi_config_ap();
  TelnetServer.begin();
  TelnetServer.setNoDelay(true);

  ota_setup();

  //WiFi.setAutoReconnect(true);// if (Wifi.isConnected()..setA)
  http.setReuse(true);
local_ip=WiFi.localIP().toString();	 

}

void ota_setup(){  ArduinoOTA.onStart([]() {

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
}

void wifi_config_ap() {
  //start-block2
 // IPAddress _ip = IPAddress(10, 0, 1, 78);
  //IPAddress _gw = IPAddress(10, 0, 1, 1);
 // IPAddress _sn = IPAddress(255, 255, 255, 0);
  //end-block2
  
 // wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
wifiManager.setConfigPortalTimeout(180);
  if (!wifiManager.autoConnect("zavod_h2o", "")){
  Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }


}

int DebouncePin(){
    if (digitalRead(SWITCH_PIN) == HIGH){
        delay(25);
    if (digitalRead(SWITCH_PIN) == HIGH){
    return HIGH;
}

    }
    return LOW;
}

void loop() {
    
  	ArduinoOTA.handle();
  	read_switch_pin();

//  	
//if (calibrated==false){
//    RunningMedian first_value = RunningMedian(5);
//    for (int i = 0; i<5; i++){
//        first_value.add(sonar.ping_cm(EMPTY_TANK_DISTANCE));
//        delay(50);
//    }
//    value = first_value.getHighest();
//    calibrated = true;
//    printToTelnet();
//    report_water_level();
//}  	
////ping_time = sonar.ping_median(5,EMPTY_TANK_DISTANCE);
//else 
//{
    
   

unsigned long current_ping_time = millis();
if (ping_interval < (current_ping_time - last_ping_time) )
{
    last_ping_time = current_ping_time;
     pcm = sonar.ping_cm(EMPTY_TANK_DISTANCE);
if (abs(value-pcm)<DELTA_OUTLIERS);
     samples.add(pcm);
}

    //distance_cm = sonar.convert_cm(ping_time);
    if (samples.getCount() == MEDIAN_MAX_SIZE)
        { distance_cm = samples.getAverage(5);
          delta = abs(value-distance_cm);
	
	if (!calibrated){
	 value = samples.getHighest();
    printToTelnet();
    report_water_level();
calibrated = true;

}
else{
if (delta>MAX_DELTA || report_interval < (current_ping_time - last_report_time)){
    last_report_time = current_ping_time;
    value = distance_cm;
    printToTelnet();
    report_water_level();
}

}
samples.clear();
valve_control();
}//delay(100);

}

void read_switch_pin(){
    switch_pin_state = digitalRead(SWITCH_PIN);
    
   // if (switch_pin_state == HIGH){
   // digitalWrite(LED_PIN, LOW);
//}

//else 
// {    
//    digitalWrite(LED_PIN, HIGH);    
// }

}

void valve_control(){
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
 
}

//if Telnet.
//String host_opiz = "http://192.168.1.130/";

void report_water_level(){
    
   
    if (WiFi.isConnected()) {

    USE_SERIAL.print("[HTTP] begin...\n");
    String g = host_souyuz;
    g+=  "api/soyuz/update.php?level=";
    g+=value;
    g+="&station="; //HTTP
    g+=local_ip;
    g+="&ping=";
    g+=ping_time;
    g+="&delta=";
    g+=delta;
    g+="&is_open=";
    g+=is_open;
    g+="&local_IP=";
    g+=local_ip;
    g+="&GPIO_5=";
    g+=switch_pin_state;
    g+="&GPIO_4=";
    g+=digitalRead(VALVE_PIN);
    g+="&LED_PIN=";
    g+=digitalRead(LED_PIN);
   g+="&FULL_TANK_DISTANCE=";
g+=FULL_TANK_DISTANCE;
g+="&EMPTY_TANK_DISTANCE=";
g+= EMPTY_TANK_DISTANCE;
g+= "&DELTA_DISTANCE=";
g+= DELTA_DISTANCE;
g+= "&VALVE_LEVEL_OPEN=";
g+= VALVE_LEVEL_OPEN;
g+= "&VALVE_LEVEL_CLOSE=";
g+= VALVE_LEVEL_CLOSE;

    http.begin(g);

    USE_SERIAL.print("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
         payload = http.getString();
        USE_SERIAL.println(payload);
	
//	last_payload += value;
//	last_payload += " LED:";
//	if (digitalRead(2) == LOW){
//		last_payload += "ON";
//	}
//	else 
//	{
//        	last_payload+= "OFF";
//	}
//  last_payload += " is_open=";
//  last_payload += is_open;
//	last_payload += "\n";
//	last_payload += payload;
//	printDebug();
	    		     
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
//last_payload = "VALVE - OPENED";
//printDebug();
}

void close_valve(){
digitalWrite(VALVE_PIN, LOW);
is_open = false;
//last_payload = "VALVE - CLOSED";
//printDebug();
}

void printDebug(){
//Prints to telnet if connected
 // Serial.println(last_payload);
// if (!Telnet) {  // otherwise it works only once
 //       Telnet = TelnetServer.available();
// }
//       	if (Telnet.connected()) {
//    Telnet.println(last_payload);
//  last_payload = "";
//	}  

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
    g+=local_ip;
    g+= "\nFULL_TANK_DISTANCE ";
    g+=FULL_TANK_DISTANCE;
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
//  String g = "";

//long ddm = ping_time;//distance_cm;//sonar.ping_median(5, MAX_DISTANCE);
//g+= "echo_time: ";
//g+=ddm; 
//g+= "  distance_cm: ";
//g+= distance_cm;
//g+= " pcm: ";
//g+= pcm;
//g+= " switch_pin_state: ";
//g+= switch_pin_state;
//g+= " value: ";
//g+= value;
//g+= " gpio_5: ";
//g+= switch_pin_stat
//e;
//g+="\n";
//String m=value;
//m+= ": ";m += samples.getLowest();m +=   " ";m += samples.getMedian();m +=   " ";m +=    samples.getHighest();
 if (!Telnet) {  // otherwise it works only once
        Telnet = TelnetServer.available();
 }
       	if (Telnet.connected()) {
      Telnet.println(value);

    Telnet.println(samples.getLowest());
      Telnet.println(samples.getMedian());
  Telnet.println(samples.getHighest());
  Telnet.println("----");

	}  

}
