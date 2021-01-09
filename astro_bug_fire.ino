#include <M5Stack.h>

#include "MillisTimer.h"
#include "Seeed_BME280.h"
#include <Wire.h>

#define GFXFF 1
#define CF_OL24 &Orbitron_Light_24
#define CF_OL32 &Orbitron_Light_32

MillisTimer T = MillisTimer(60000);
char screen[64];

BME280 bme280;

void updateScreen(MillisTimer &mt) {
  int t, h, p;
  t = (int)round(bme280.getTemperature()*1.8+32);
  p = (int)bme280.getPressure();
  h = (int)bme280.getHumidity();

  Serial.print(t); 
  Serial.print('\t');
  Serial.print(h); 
  Serial.print('\t');
  Serial.print(p); 
  Serial.println();
  
  M5.Lcd.fillRect(38, 10, 140, 122, BLACK); 
  sprintf(screen,"%d", t); M5.Lcd.setCursor( 20,  40); M5.Lcd.printf("%s",screen);
  sprintf(screen,"%d", h); M5.Lcd.setCursor(100,  40); M5.Lcd.printf("%s",screen); 
  sprintf(screen,"%d", p); M5.Lcd.setCursor( 20,  80); M5.Lcd.printf("%s",screen);
}

void setup() {
  M5.begin();
  
  Serial.begin(9600);
  delay(500);

  Serial.println("BME280");
  if(!bme280.init()){ Serial.println("FAIL"); }

  M5.Lcd.setFreeFont(CF_OL32);
  M5.Lcd.setTextColor(TFT_GREEN, TFT_WHITE);
  M5.Lcd.setBrightness(50);
  
  T.setInterval(60000);
  T.expiredHandler(updateScreen);
  T.setRepeats(0);
  T.start();

  //updateScreen(T);
}



 
void loop() {
  T.run();
}
