#include <M5StickC.h>

#include "MillisTimer.h"
#include "Seeed_BME280.h"

#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include <WiFiMulti.h>

//WiFiMulti wifiMulti;


MillisTimer T2M = MillisTimer(60 * 2 * 1000);
char screen[256];

BME280 bme280;

IPAddress server();
char ssid[] = ""; char password[] = "";
//char ssid[] = ""; char password[] = "";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

unsigned long waitCount = 0;                 

int16_t ax, ay, az;
int16_t dax, day, daz;
float Temperature, Humidity,Pressure;

boolean timeSet = false;

////////////////////////////////////////////////////////////////////////////////

void get_network_time() {    
  update_status("NTP UPDATE", YELLOW);
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


void imu_offset() {
  delay(100);
  M5.IMU.getAccelAdc(&ax,&ay,&az);
  dax = dax+ax; day = day+ay; daz = daz+az;
  delay(100);
  M5.IMU.getAccelAdc(&ax,&ay,&az);
  dax = dax+ax; day = day+ay; daz = daz+az;
  delay(100);
  M5.IMU.getAccelAdc(&ax,&ay,&az);
  dax = dax+ax; day = day+ay; daz = daz+az;
  dax = dax/3.0; day = day/3.0; daz = daz/3.0; 
}


void update_status(char* msg, unsigned int color) {
  Serial.print("STATUS |");
  Serial.print(msg);
  Serial.println("|");
  M5.Lcd.fillRect(0, 0, 160, 80, BLACK); 
  M5.Lcd.setTextColor(color, BLACK); 
  M5.Lcd.setCursor(8, 32); 
  M5.Lcd.printf(msg); 
}


void update(MillisTimer &mt) {

  check_connection();
  
  M5.IMU.getAccelAdc(&ax,&ay,&az);
  ax = ax - dax; ay = ay - day; az = az - daz;  

  Temperature = bme280.getTemperature()*1.8+32;
  Pressure = bme280.getPressure();
  Humidity = bme280.getHumidity();
  
  M5.Lcd.fillRect(0, 0, 160, 80, BLACK); 
  M5.Lcd.setTextColor(GREEN, BLACK);
  sprintf(screen,"%d",(int)Temperature);   M5.Lcd.setCursor(  8, 8); M5.Lcd.printf("%s",screen);
  sprintf(screen,"%d",(int)Humidity);      M5.Lcd.setCursor( 48, 8); M5.Lcd.printf("%s",screen); 
  sprintf(screen,"%.2f",(Pressure/1000));  M5.Lcd.setCursor( 88, 8); M5.Lcd.printf("%s",screen);
  sprintf(screen,"%d",(int)(ax));  M5.Lcd.setCursor( 12,32); M5.Lcd.printf("%s",screen);
  sprintf(screen,"%d",(int)(ay));  M5.Lcd.setCursor( 56,32); M5.Lcd.printf("%s",screen); 
  sprintf(screen,"%d",(int)(az));  M5.Lcd.setCursor(120,32); M5.Lcd.printf("%s",screen);

  String st = timeClient.getFormattedTime();
  M5.Lcd.setTextColor(YELLOW, BLACK);
  sprintf(screen,"%s", st.substring(0,5)); M5.Lcd.setCursor( 8,56); M5.Lcd.printf("%s",screen);  

  sprintf(screen,"{\"SU\":[%d,%d,%d,%.2f,%.0f,%.0f,\"%s\",\"STICK\"]}", ax,ay,az, Temperature, Humidity, Pressure, st);
  Serial.println(screen);
  mqttClient.publish("astro-bug-stick", screen);

}

void setup() {
 
  M5.begin();
  delay(100);
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Axp.ScreenBreath(8);
  M5.Lcd.setTextDatum(TR_DATUM);

  Wire.begin(32, 33);
  delay(100);
   
  Serial.begin(115200); delay(100); while(!Serial) { Serial.print('.'); }
  
  update_status("BME280", YELLOW);
  if(!bme280.init()){ Serial.println("FAIL"); }
  delay(100);
 
  //wifiMulti.addAP("NETGEAR06", "gentlegiant323");
  //wifiMulti.addAP("SPRING", "2124563829");

  update_status("IMU", YELLOW);
  M5.IMU.Init();
  M5.IMU.getAres();
  sprintf(screen,"%.8f", M5.IMU.aRes); 
  Serial.println(screen);
  delay(500);
  imu_offset();
  delay(500);
  
  update_status("WIFI", YELLOW); 
  WiFi.begin(ssid, password);
  update_status(ssid, GREEN); 
  delay(5000);
 
  update_status("NTP", YELLOW);
  timeClient.begin();
  timeClient.setTimeOffset(-4 * 36000);
  get_network_time();
  delay(1000);
  
  update_status("MQTT", YELLOW);
  mqttClient.setServer(server, 1883);
  boolean rc = mqttClient.connect("stick", "ubuntu", "vul[an");
  delay(1000);
  
  T2M.setInterval(60 * 2000);
  T2M.expiredHandler(update);
  T2M.setRepeats(0);
  T2M.start();

  update_status("GO", YELLOW);
}

void check_connection() {

  if (WiFi.status() != WL_CONNECTED) {
    update_status("WIFI DOWN", RED);
    waitCount = 0;
    WiFi.begin(ssid);
    delay(1000);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    waitCount++;
    if( waitCount>=60 ){ ESP.restart();}
    delay(1000);
  }
  //update_status("WIFI UP", GREEN);
  
  if (!mqttClient.connected()) {
    update_status("MQTT DOWN", RED);
    boolean rc = mqttClient.connect("stick", "ubuntu", "vul[an");
    delay(1000);
  }
  while(!mqttClient.connected()) {
    Serial.print(".");
    waitCount++;
    delay(1000);
  }
  //update_status("MQTT UP", GREEN);

  if(!timeSet) get_network_time();
}

 
void loop() {
  T2M.run();
  mqttClient.loop();
  
  if (M5.BtnA.wasPressed()){
    imu_offset();
  }
}
