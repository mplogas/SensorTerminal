#include <Arduino.h>

#include"TFT_eSPI.h"
#include <ardukit.h>
#include <AceButton.h>
#include <LinkedList.h>
#include <Multichannel_Gas_GMXXX.h>
#include <Wire.h>
#include <seeed_bme680.h>
#include <SparkFunBQ27441.h>
#include <Digital_Light_TSL2561.h>
#include <Seeed_VEML6070.h>
using namespace adk;
using namespace ace_button;

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define IIC_ADDR  uint8_t(0x76)

const String TITLE_BME = "BME860";
const String TITLE_GAS = "Gas";
const String TITLTE_LIGHT = "Light";
const String TITLE_SOUND = "Sound";
const String TITLE_GYRO = "Gyro";
const String TITLE_BATTERY = "Battery";
const int MODULE_AMOUNT = 6;


class Entry {
  public:
    String title;
    String unit;
    float value;
};

struct Module {
  String title;
  LinkedList<Entry*> entries;
};

Module modules[MODULE_AMOUNT] = {
  {TITLE_BME, LinkedList<Entry*>()},
  {TITLE_GAS, LinkedList<Entry*>()},
  {TITLTE_LIGHT, LinkedList<Entry*>()},
  {TITLE_SOUND, LinkedList<Entry*>()},
  {TITLE_GYRO, LinkedList<Entry*>()},
  {TITLE_BATTERY, LinkedList<Entry*>()}
};

int pageId = 0;

TFT_eSPI tft;
TFT_eSprite spr = TFT_eSprite(&tft);

Task taskBME, taskGas, taskLight, taskSound, taskGyro, taskBattery, taskUi; 

Seeed_BME680 bme680(IIC_ADDR);
GAS_GMXXX<TwoWire> gas;
const unsigned int BATTERY_CAPACITY = 650; // Set Wio Terminal Battery's Capacity 
// #ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
//   #define SERIAL SerialUSB
// #else
//   #define SERIAL Serial
// #endif
// VEML6070 uv_sensor;


AceButton bFavA(WIO_KEY_A);
AceButton bFavB(WIO_KEY_B);
AceButton bFavC(WIO_KEY_C);
AceButton bNext(WIO_5S_RIGHT);
AceButton bPrev(WIO_5S_LEFT);

//forward references
void printSprite(void *); 
void handleEvent(AceButton*, uint8_t, uint8_t);
void taskBMEData(void *);
void taskGasData(void *);
void taskLightData(void *);
void taskSoundData(void *);
void taskGyroData(void *);
void taskBatteryData(void *);


void setup()
{
    Serial.begin(128000);

    tft.begin();
    tft.setRotation(3);
    spr.createSprite(tft.width(), tft.height());
    spr.setFreeFont(&FreeMono9pt7b);

    pinMode(WIO_KEY_A, INPUT_PULLUP);
    pinMode(WIO_KEY_B, INPUT_PULLUP);
    pinMode(WIO_KEY_C, INPUT_PULLUP);
    pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
    pinMode(WIO_5S_LEFT, INPUT_PULLUP);

    ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
    buttonConfig->setEventHandler(handleEvent);
    buttonConfig->setFeature(ButtonConfig::kFeatureClick);

      // sensor init
    gas.begin(Wire, 0x08); 
    bme680.init();
    TSL2561.init();
    //uv_sensor.init();
    /*threshold is 145 steps*/
    //uv_sensor.set_interrupt(INT_145_STEP,ENABLE);


    if (lipo.begin()) // begin() will return true if communication is successful
    {
      lipo.setCapacity(BATTERY_CAPACITY);
    } else {
      dmsg("Error: Unable to communicate with BQ27441.");
    }

    taskBME.set_interval(1000).start(taskBMEData); 
    taskGas.set_interval(1000).start(taskGasData);
    taskLight.set_interval(1000).start(taskLightData);
    taskSound.set_interval(1000).start(taskSoundData);
    taskGyro.set_interval(500).start(taskGyroData);
    taskBattery.set_interval(1000).start(taskBatteryData);
    taskUi.set_interval(200).start(printSprite);
}

void loop()
{
    bFavA.check();
    bFavB.check();
    bFavC.check();
    bNext.check();
    bPrev.check();
    adk::run();
}

void drawTitle(String title, int x, int y) {
  spr.setTextColor(TFT_ORANGE);
  spr.setTextSize(2);
  spr.drawString(title, x, y);
  for(int8_t line_index = 0;line_index < 5 ; line_index++)
  {
    spr.drawLine(0, x + line_index, tft.width(), x + line_index, TFT_ORANGE);
  }
}

void drawEntry(Entry *entry, int x, int y) {

  String tmp = entry->title;
  tmp.concat(": ");
  tmp.concat(entry->value);
  tmp.concat(" ");
  tmp.concat(entry->unit);
  
  spr.setTextColor(TFT_WHITE);
  spr.setTextSize(1);
  spr.drawString(tmp, x, y);
}

void printSprite(void *) {
  const int titleHeight = 50;
  spr.fillSprite(TFT_BLACK);
  drawTitle(modules[pageId].title, titleHeight, 10);
  
  for(int i = 0; i < modules[pageId].entries.size(); i++) {
    drawEntry(modules[pageId].entries.get(i), 20, titleHeight + (i + 1)*20);
  }

  spr.pushSprite(0, 0);
}

void handleEvent(AceButton* button, uint8_t eventType, uint8_t buttonState)  {
  if(eventType == AceButton::kEventPressed) {
    uint8_t buttonId = button->getPin();

    switch(buttonId) {
      case WIO_KEY_A:
        pageId = 5;
        break;
      case WIO_KEY_B:
        pageId = 1;
        break;
      case WIO_KEY_C:
        pageId = 0;
        break;
      case WIO_5S_RIGHT:
        if(pageId < MODULE_AMOUNT - 1) {
          pageId++;
        } else {
          pageId = 0;
        }
        break;
      case WIO_5S_LEFT:
        if(pageId > 0 ) {
          pageId--;
        } else {
          pageId = MODULE_AMOUNT - 1;
        }
        break; 
      default:
        pageId = 0;
        break;   
    }

    dmsg("pageId: %d", pageId);
  }
  
}
void taskBMEData(void *) {
  const int id = 0; 
  bme680.read_sensor_data();
  float currentTemp = bme680.sensor_result_value.temperature * 0.89;
  float currentHumi = float(bme680.sensor_result_value.humidity) * 1.07;
  float currentPres = bme680.sensor_result_value.pressure/100;
  float currentIaQ = log(bme680.sensor_result_value.gas) + 0.04/currentHumi;

  if(modules[id].entries.size() == 0) {
    Entry *temp = new Entry();
    temp->title = "Temperature";
    temp->unit = "°C";
    temp->value = currentTemp;
    modules[id].entries.add(temp);

    Entry *humi = new Entry();
    humi->title = "Humidity";
    humi->unit = "%";
    humi->value = currentHumi;
    modules[id].entries.add(humi);

    Entry *pres = new Entry();
    pres->title = "Air Pressure";
    pres->unit = "mBar";
    pres->value = currentPres;
    modules[id].entries.add(pres);

    Entry *iaq = new Entry();
    iaq->title = "Air Quality";
    iaq->unit = "%";
    iaq->value = currentIaQ;
    modules[id].entries.add(iaq);
  } else {
    modules[id].entries.get(0)->value = currentTemp;
    modules[id].entries.get(1)->value = currentHumi;
    modules[id].entries.get(2)->value = currentPres;
    modules[id].entries.get(3)->value = currentIaQ;
  }
}

void taskGasData(void *) {
  const int id = 1;

  float currentNO2 = float(gas.getGM102B());
  float currentC2H5CH = float(gas.getGM302B());
  float currentVOC = float(gas.getGM502B());
  float currentCO = float(gas.getGM702B());

  if(modules[id].entries.size() == 0) {
    Entry *no2 = new Entry();
    no2->title = "NO2";
    no2->unit = "ppm";
    no2->value = currentNO2;
    modules[id].entries.add(no2);

    Entry *c2h2ch = new Entry();
    c2h2ch->title = "C2H5CH";
    c2h2ch->unit = "ppm";
    c2h2ch->value = currentC2H5CH;
    modules[id].entries.add(c2h2ch);

    Entry *voc = new Entry();
    voc->title = "vOC";
    voc->unit = "ppm";
    voc->value = currentVOC;
    modules[id].entries.add(voc);

    Entry *co = new Entry();
    co->title = "CO";
    co->unit = "ppm";
    co->value = currentCO;
    modules[id].entries.add(co);
   } else {
    modules[id].entries.get(0)->value = currentNO2;
    modules[id].entries.get(1)->value = currentC2H5CH;
    modules[id].entries.get(2)->value = currentVOC;
    modules[id].entries.get(3)->value = currentCO;
   }
}

void taskLightData(void *) {
  const int id = 2;

  float currentLux = TSL2561.readVisibleLux();
  // u16 step;
  // uv_sensor.wait_for_ready();
  // uv_sensor.read_step(step);
  //char *UV_str[]={"low level","moderate level","high_level","very high","extreme"};
  //RISK_LEVEL currentUvLevel=uv_sensor.convert_to_risk_level(step);
  //float currentUV = float(step);

  if(modules[id].entries.size() == 0) {
    Entry *lux = new Entry();
    lux->title = "Light";
    lux->unit = "lux";
    lux->value = currentLux;
    modules[id].entries.add(lux);

    // Entry *uv = new Entry();
    // uv->title = "UV";
    // uv->unit = "";//UV_str[currentUvLevel];
    // uv->value = currentUV;
    // modules[id].entries.add(uv);    
  } else {
    modules[id].entries.get(0)->value = currentLux;
    //modules[id].entries.get(1)->value = currentUV;
    //modules[id].entries.get(1)->unit = UV_str[currentUvLevel];
  } 
}
void taskSoundData(void *) {}
void taskGyroData(void *) {}

void taskBatteryData(void *) {
  const int id = 5;

  float currentSOC = float(lipo.soc()); 
  float currentVolt = float(lipo.voltage()/1000.00);
  float currentCurr = float(lipo.current(AVG));
  float currentCap = float(lipo.capacity(REMAIN));
  float currentSOH = float(100 - lipo.soh());
  float currentDraw = lipo.power();

  if(modules[id].entries.size() == 0) {
    Entry *soc = new Entry();
    soc->title = "State of Charge";
    soc->unit = "%";
    soc->value = currentSOC;
    modules[id].entries.add(soc);

    Entry *draw = new Entry();
    draw->title = "Avg. Draw";
    draw->unit = "mW";
    draw->value = currentDraw;
    modules[id].entries.add(draw);

    Entry *volt = new Entry();
    volt->title = "Voltage";
    volt->unit = "V";
    volt->value = currentVolt;
    modules[id].entries.add(volt);

    Entry *curr = new Entry();
    curr->title = "Avg. Current";
    curr->unit = "mA";
    curr->value = currentCurr;
    modules[id].entries.add(curr);

    Entry *cap = new Entry();
    cap->title = "Remaining Capacity";
    cap->unit = "mA";
    cap->value = currentCap;
    modules[id].entries.add(cap);

    Entry *soh = new Entry();
    soh->title = "Health";
    soh->unit = "%";
    soh->value = currentSOH;
    modules[id].entries.add(soh);
  } else {
    modules[id].entries.get(0)->value = currentSOC;
    modules[id].entries.get(1)->value = currentDraw;
    modules[id].entries.get(2)->value = currentVolt;
    modules[id].entries.get(3)->value = currentCurr;
    modules[id].entries.get(3)->value = currentCap;
    modules[id].entries.get(3)->value = currentSOH;
  }
}
