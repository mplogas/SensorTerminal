#include <Arduino.h>

#include <TFT_eSPI.h>
#include <ardukit.h>
#include <AceButton.h>
#include <LinkedList.h>
#include <SparkFunBQ27441.h>
#include <Digital_Light_TSL2561.h>
// #include <Seeed_VEML6070.h>
#include <Multichannel_Gas_GMXXX.h>
#include <Wire.h>
#include <seeed_bme680.h>
using namespace adk;
using namespace ace_button;

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define IIC_ADDR  uint8_t(0x76)

const unsigned int BATTERY_CAPACITY = 650;
// const char *UV_str[]={"low level","moderate level","high_level","very high","extreme"};
const String TITLE_BME = "BME860";
const String TITLE_GAS = "Gas";
const String TITLTE_LIGHT = "Light";
const String TITLE_SOUND = "Sound";
const String TITLE_GYRO = "Gyro";
const String TITLE_BATTERY = "Battery";
const int MODULE_AMOUNT = 6;


//forward references
void printSprite(void *); 
void handleEvent(AceButton*, uint8_t, uint8_t);
void taskBMEData(void *);
void taskGasData(void *);
void taskLightData(void *);
void taskSoundData(void *);
void taskGyroData(void *);
void taskBatteryData(void *);