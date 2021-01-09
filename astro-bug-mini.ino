#include "MillisTimer.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include <WiFiMulti.h>

MillisTimer T1S = MillisTimer(1000);
char screen[256];

Adafruit_BME280 bme280;

IPAddress server(52,91,115,13);
//char ssid[] = "NETGEAR06"; char password[] = "gentlegiant323";
//char ssid[] = "SPRING"; char password[] = "2124563829";
char ssid[] = "LILY"; 
char password[] = "2124563829";
char my_name[] = "1st Floor";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
const long utcOffsetInSeconds = -4 * 60 * 60;

unsigned long waitCount = 0;                 

float Temperature, Humidity, Pressure;

boolean timeSet = false;

////////////////////////////////////////////////////////////////////////////////

void get_network_time() {    
  update_status("NTP UPDATE");
  if(!timeClient.update()) {
    timeClient.forceUpdate();
  } else {
    timeSet = true;
  }
  if(timeSet) {
    Serial.print("NTP TIME ");
    Serial.println(timeClient.getFormattedTime());
  } else {
    Serial.println("NTP TIME NOT SET");
  }
}

void update_status(char* msg) {
  Serial.print("STATUS |");
  Serial.print(msg);
  Serial.println("|");
}

void update(MillisTimer &mt) {
  check_connection();
  Temperature = bme280.readTemperature()*1.8+32;
  Pressure = bme280.readPressure();
  Humidity = bme280.readHumidity();
  String st = timeClient.getFormattedTime();
  sprintf(screen,"{\"SU\":[0,0,0,%.2f,%.0f,%.0f,\"%s\",\"%s\"]}", Temperature, Humidity, Pressure, st.c_str(), my_name);
  Serial.println(screen);
  mqttClient.publish(my_name, screen);
}

void setup() {

  Serial.begin(115200);
  delay(2000);
  
  update_status("BME280");
  if(!bme280.begin(0x76)){ Serial.println("FAIL"); }
  delay(100);
 
  //wifiMulti.addAP("NETGEAR06", "gentlegiant323");
  //wifiMulti.addAP("SPRING", "2124563829");
  
  update_status("WIFI"); 
  WiFi.begin(ssid, password);
  update_status(ssid); 
  delay(5000);
 
  update_status("NTP");
  timeClient.begin();
  timeClient.setTimeOffset(utcOffsetInSeconds);
  get_network_time();
  delay(1000);
  
  update_status("MQTT");
  mqttClient.setServer(server, 1883);
  boolean rc = mqttClient.connect(my_name, "ubuntu", "vul[an");
  delay(1000);
  
  T1S.setInterval(2 * 60 * 1000);
  T1S.expiredHandler(update);
  T1S.setRepeats(0);
  T1S.start();

  update_status("GO");
}

void check_connection() {
  waitCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    update_status("WIFI DOWN");
    WiFi.begin(ssid);
    waitCount++;
    if( waitCount >= 12 ) { ESP.restart(); }
    delay(5000);
  }
  waitCount = 0;
  while (!mqttClient.connected()) {
    update_status("MQTT DOWN");
    boolean rc = mqttClient.connect(my_name, "ubuntu", "vul[an");
    waitCount++;
    if( waitCount >= 12 ) { ESP.restart(); }
    delay(5000);
  }
  if(!timeSet) get_network_time();
}

 
void loop() {
  T1S.run();
  mqttClient.loop();
}
