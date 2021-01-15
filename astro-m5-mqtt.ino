#include <M5StickC.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#include "DHT12.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_BMP280.h"
#include "bmm150.h"
#include "bmm150_defs.h"

BMM150 bmm = BMM150();
bmm150_mag_data value_offset;

DHT12 dht12;
Adafruit_BMP280 bme;


IPAddress server(192, 168, 1, 30);

char password[] = "";
char ssid[] = "";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

long tLastMsg = 0;
long tLastScreen = 0;
long tLastConn = 0;

const long MSG_INTERVAL =     1 * 60 * 1000; // 1 minute
const long SCREEN_INTERVAL =       1 * 1000; // 1 sec
const long CONN_INTERVAL =        15 * 1000; // 15 sec

float aX = 0.0; float aY = 0.0; float aZ = 0.0;
float dX = 0.0; float dY = 0.0; float dZ = 0.0;

float angle;
float temperature;
float humidity;
float pressure; 

char buff[128];
char screen[64];

unsigned long waitCount = 0;                 
uint8_t conn_stat = 0; 
                      
// Connection status for WiFi and MQTT
//
// status |   WiFi   |    MQTT
// -------+----------+------------
//      0 |   down   |    down
//      1 | starting |    down
//      2 |    up    |    down
//      3 |    up    |  starting
//      4 |    up    | finalising
//      5 |    up    |     up

////////////////////////////////////////////////////

void imu_offset() {
  delay(100);
  M5.IMU.getAccelData(&aX, &aY, &aZ);
  dX = dX+aX; dY = dY+aY; dZ = dZ+aZ;
  delay(100);
  M5.IMU.getAccelData(&aX, &aY, &aZ);
  dX = dX+aX; dY = dY+aY; dZ = dZ+aZ;
  delay(100);
  M5.IMU.getAccelData(&aX, &aY, &aZ);
  dX = dX+aX; dY = dY+aY; dZ = dZ+aZ;
  dX = dX/3.0; dY = dY/3.0; dZ = dZ/3.0; 
}

void read_sensors() {

  //M5.Rtc.GetBm8563Time();
  //M5.Rtc.Hour, M5.Rtc.Minute, M5.Rtc.Second
  
  temperature = dht12.readTemperature(FAHRENHEIT);
  humidity = dht12.readHumidity();
  pressure = bme.readPressure();
  M5.IMU.getAccelData(&aX, &aY, &aZ);
  aX = aX - dX; aY = aY - dY; aZ = aZ - dZ;
  bmm150_mag_data value;
  bmm.read_mag_data();
  value.x = bmm.raw_mag_data.raw_datax - value_offset.x;
  value.y = bmm.raw_mag_data.raw_datay - value_offset.y;
  value.z = bmm.raw_mag_data.raw_dataz - value_offset.z;
  float xyH = atan2(value.x, value.y); 
  float H = xyH;
  H = ( H < 0 ) ? H + 2*PI : H;
  H = ( H > 2*PI ) ? H - 2*PI : H;
  angle = H * 180.0 / M_PI; 
  sprintf(buff, "{\"t\":\"%.2f\",\"h\":\"%.2f\",\"m\":\"%.2f\",\"p\":\"%.4f\",\"x\":\"%.4f\",\"y\":\"%.4f\",\"z\":\"%.4f\"}", temperature, humidity, angle, pressure, aX*1000, aY*1000, aZ*1000);
}

void update_screen() {
  sprintf(screen, "T %4.1f\nH %4.1f\n%6.0f%\nX%5.0f\nY%5.0f\nZ%5.0f\nA%5.0f", temperature, humidity, pressure, aX*1000, aY*1000, aZ*1000, angle);
  M5.Lcd.setCursor(0, 8);
  M5.Lcd.printf(screen);
}

void update_status(char* msg, unsigned int color) {
  M5.Lcd.fillRect(0, 140, 80, 16, BLACK);  
  M5.Lcd.setTextColor(color, BLACK); 
  M5.Lcd.setCursor(0, 140); 
  M5.Lcd.printf(msg); 
  M5.Lcd.setTextColor(GREEN, BLACK);
}

void get_network_time() {
  while(!timeClient.update()) {
    Serial.println("NTP UPDATE");
    update_status("NTP", RED);
    timeClient.forceUpdate();
    delay(500);
  }
  Serial.print("NTP TIME ");
  Serial.println(timeClient.getFormattedTime());
  update_status("NTP", GREEN);

  RTC_TimeTypeDef TS;
  TS.Hours   = timeClient.getHours();TS.Minutes = timeClient.getMinutes(); TS.Seconds = timeClient.getSeconds();
  M5.Rtc.SetTime(&TS);
  
  //RTC_DateTypeDef DateStruct;
  //DateStruct.WeekDay = 3;
  //DateStruct.Month = 3;
 //DateStruct.Date = 22;
  //DateStruct.Year = 2019;
  //M5.Rtc.SetData(&DateStruct);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  char cc;
  Serial.print("[");
  String st = String(topic);
  st.replace("nugget","");
  Serial.print(st);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    cc = (char)payload[i];
    if(cc != 34)
      Serial.print(cc);
  }
  Serial.println();
  update_status("MSG", YELLOW);
}

void setup() {
  M5.begin();
  M5.Lcd.setRotation(0);
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Axp.ScreenBreath(8);
  M5.Lcd.setTextDatum(TR_DATUM);
  
  Wire.begin(0, 26);
  delay(250);
  
  Serial.begin(115200);
  while(!Serial) { Serial.print('.'); }
  
  Serial.println("BMP280");
  if (!bme.begin(0x76)){ Serial.println("FAIL"); }
  
  Serial.println("BMM150");
  if(bmm.initialize() == BMM150_E_ID_NOT_CONFORM) { Serial.println("FAIL"); }
  
  Serial.println("IMU OFFSET");
  M5.IMU.Init();
  imu_offset();
  
  Serial.print("WIFI CONNECTING ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  delay(1000);
  
  Serial.println("MQTT");
  mqttClient.setServer(server, 1883);
  mqttClient.setCallback(mqtt_callback);
  
  Serial.println("NTP");
  timeClient.begin();
  timeClient.setTimeOffset(5 * 60 * 60);
  get_network_time();
  Serial.println("GO");
  update_status("GO", YELLOW);
}

void check_connection() {
  if ((WiFi.status() != WL_CONNECTED) && (conn_stat != 1)) { conn_stat = 0; }
  if ((WiFi.status() == WL_CONNECTED) && !mqttClient.connected() && (conn_stat != 3))  { conn_stat = 2; }
  if ((WiFi.status() == WL_CONNECTED) && mqttClient.connected() && (conn_stat != 5)) { conn_stat = 4;}
  switch (conn_stat) {
    case 0:
      Serial.println("MQTT/WIFI DOWN");
      Serial.println("WIFI START");
      update_status("WIFI", RED);
      WiFi.begin(ssid, password);
      conn_stat = 1;
      break;
    case 1:
      if(waitCount == 0 || waitCount % 1000 == 0) Serial.println("WIFI STARTING "+ String(waitCount));
      waitCount++;
      break;
    case 2:
      Serial.print("WIFI UP ");
      Serial.println(WiFi.localIP());
      Serial.println("MQTT DOWN");
      Serial.println("MQTT START");
      update_status("WIFI", GREEN);
      mqttClient.connect("stick");
      conn_stat = 3;
      waitCount = 0;
      break;
    case 3:
      Serial.println("MQTT STARTING "+ String(waitCount));
      update_status("MQTT", RED);
      waitCount++;
      break;
    case 4:
      Serial.println("MQTT UP");
      update_status("MQTT", GREEN);
      mqttClient.subscribe("nugget");
      conn_stat = 5;
      tLastMsg = 0;               
      break;
  }
}


void loop() {
  mqttClient.loop();
  
  // Reset
  if (M5.BtnA.wasPressed()){
    imu_offset();
  }
  
  long zeit = millis();
  
  // Try internet connection
  check_connection();
  
  // Screen
  if (zeit - tLastScreen > SCREEN_INTERVAL ) {
    tLastScreen = zeit;
    read_sensors();
    update_screen();
  }

  // Message
  if (conn_stat == 5 && zeit - tLastMsg > MSG_INTERVAL ) {
    tLastMsg = zeit;
    read_sensors();
    mqttClient.publish("nugget", buff);
  }
}
