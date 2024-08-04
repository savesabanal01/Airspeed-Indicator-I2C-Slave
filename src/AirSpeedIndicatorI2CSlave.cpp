#include <Arduino.h>
#include "I2C_slave.h"
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <main_gauge.h>
#include <asi_needle.h>
#include <asi_info.h>
#include <tas_info.h>
#include <altitude_info.h>
#include <vsi_info.h>
#include <baro_info.h>
#include <ssFont.h>
#include <NotoSansBold15.h>
#include <NotoSansBold36.h>
#include <instrument_bezel.h>
#include <logo.h>

#define USE_HSPI_PORT
#define I2C_MOBIFLIGHT_ADDR 0x27
#define digits NotoSansBold36
#define PANEL_COLOR 0x7BEE

float startTime;
float endTime;
float airSpeed = 0;          // Indicated AirSpeed from Sim
float ASIneedleRotation = 0; // angle of rotation of needle based on the Indicated AirSpeed
float instrumentBrightnessRatio = 1;
int instrumentBrightness = 255;
float prevInstrumentBrightnessRatio = 0; // previous value of instrument brightness. If no change do not set instrument brightness to avoid flickers
float fps = 0;                           // frames per second for testing

int16_t messageID = 0;            // will be set to the messageID coming from the connector
char message[MAX_LENGTH_MESSAGE]; // contains the message which belongs to the messageID
I2C_slave i2c_slave;
// Function Declaration
void setAirspeed(float value);
void setInstrumentBrightnessRatio(float ratio);
void checkI2CMesage();
float scaleValue(float x, float in_min, float in_max, float out_min, float out_max);

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

TFT_eSprite mainSpr = TFT_eSprite(&tft);         // Main Sprite
TFT_eSprite ASIneedleSpr = TFT_eSprite(&tft);    // Airspeed Indicator Gauge Sprite
TFT_eSprite asiInfoSpr = TFT_eSprite(&tft);      // Sprite to hold Air Speed Value
TFT_eSprite tasInfoSpr = TFT_eSprite(&tft);      // Sprite to hold True Air Speed Value
TFT_eSprite altitudeInfoSpr = TFT_eSprite(&tft); // Sprite to hold Altitude
TFT_eSprite vsiInfoSpr = TFT_eSprite(&tft);      // Sprite to hold Vertical Speed Info
TFT_eSprite baroInfoSpr = TFT_eSprite(&tft);     // Sprite to hold Barometric Pressure Info

void setup()
{
  Serial.begin(115200);

  // init I2C busses
  i2c_slave.init(I2C_MOBIFLIGHT_ADDR, 400000);
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); // built in LED on arduino board will turn on and off with the status of the beacon light
  digitalWrite(LED_BUILTIN, HIGH);

  // delay(1000); // wait for serial monitor to open
  digitalWrite(LED_BUILTIN, LOW);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(PANEL_COLOR);
  tft.setPivot(320, 160);
  tft.setSwapBytes(true);
  tft.pushImage(160, 80, 160, 160, logo, TFT_BLACK);
  delay(3000);
  tft.fillScreen(PANEL_COLOR);
  tft.fillCircle(240, 160, 160, TFT_BLACK);

  tft.setSwapBytes(true);
  tft.pushImage(80, 0, 320, 320, instrument_bezel, TFT_BLACK);
  mainSpr.setColorDepth(16);
  mainSpr.createSprite(320, 320);
  mainSpr.setSwapBytes(true);
  mainSpr.fillSprite(TFT_BLACK);
  mainSpr.pushImage(0, 0, 320, 320, main_gauge);
  mainSpr.setPivot(160, 160);

  ASIneedleSpr.createSprite(asi_needle_width, asi_needle_height);
  ASIneedleSpr.fillSprite(TFT_BLACK);
  ASIneedleSpr.pushImage(0, 0, asi_needle_width, asi_needle_height, asi_needle);
  ASIneedleSpr.setPivot(asi_needle_width / 2, 133);

  asiInfoSpr.createSprite(150, 50);
  asiInfoSpr.loadFont(digits);
  asiInfoSpr.setTextDatum(TC_DATUM);

  tasInfoSpr.createSprite(150, 50);
  tasInfoSpr.loadFont(digits);
  tasInfoSpr.setTextDatum(TC_DATUM);

  altitudeInfoSpr.createSprite(150, 50);
  altitudeInfoSpr.loadFont(digits);
  altitudeInfoSpr.setTextDatum(TC_DATUM);

  vsiInfoSpr.createSprite(150, 50);
  vsiInfoSpr.loadFont(digits);
  vsiInfoSpr.setTextDatum(TC_DATUM);

  baroInfoSpr.createSprite(150, 50);
  baroInfoSpr.loadFont(digits);
  baroInfoSpr.setTextDatum(TC_DATUM);
}

void loop()
{
  // Check if messages are coming in
  checkI2CMesage();
  startTime = millis();

  if (airSpeed <= 40)
    ASIneedleRotation = scaleValue(airSpeed, 0, 40, 0, 20);
  else if (airSpeed > 40 && airSpeed <= 200)
    ASIneedleRotation = scaleValue(airSpeed, 41, 200, 21, 321);
  else if (airSpeed > 200)
    airSpeed = 200;

  mainSpr.pushImage(0, 0, 320, 320, main_gauge);
  ASIneedleSpr.setSwapBytes(true);
  ASIneedleSpr.pushImage(0, 0, asi_needle_width, asi_needle_height, asi_needle);

  ASIneedleSpr.pushRotated(&mainSpr, ASIneedleRotation, TFT_BLACK);
  ASIneedleSpr.setSwapBytes(true);

  mainSpr.pushSprite(80, 0, TFT_BLACK);

  endTime = millis();

  fps = 1000 / (endTime - startTime);

  Serial.println(1000 / (endTime - startTime));

  if (prevInstrumentBrightnessRatio != instrumentBrightnessRatio) // there is a change in brighness, execute code
  {
    analogWrite(TFT_BL, instrumentBrightness);
    prevInstrumentBrightnessRatio = instrumentBrightnessRatio;
  }

}

void checkI2CMesage()
{
  if (i2c_slave.message_available())
  {
    messageID = i2c_slave.getMessage(message);
    switch (messageID)
    {
    case -1:
      // received messageID is 0
      // data is a string in message[] and 0x00 terminated
      // do something with your received data
      // Serial.print("MessageID is -1 and Payload is: "); Serial.println(message);
      break;

    case 0:
      // received messageID is 0
      // data is a string in message[] and 0x00 terminated
      // do something with your received data
      // Serial.print("MessageID is 0 and Payload is: "); Serial.println(message);
      setAirspeed(atof(message));
      break;

    case 1:
      // received messageID is 1
      // data is a string in message[] and 0x00 terminated
      // do something with your received data
      // Serial.print("MessageID is 1 and Payload is: "); Serial.println(message);
      setInstrumentBrightnessRatio(atof(message));
      break;

      // case 2:
      //   // received messageID is 2
      //   // data is a string in message[] and 0x00 terminated
      //   // do something with your received data
      //   Serial.print("MessageID is 2 and Payload is: "); Serial.println(message);
      //   break;

    default:
      break;
    }
  }
}

void setAirspeed(float value)
{
  airSpeed = value;
}

void setInstrumentBrightnessRatio(float ratio)
{
  instrumentBrightnessRatio = ratio;
  instrumentBrightness = round(scaleValue(instrumentBrightnessRatio, 0.15, 1, 0, 255));
}

float scaleValue(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
