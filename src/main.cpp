#include <Arduino.h>
#include <cstring>
#include <string.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include "classesAndFunctions.h"


#define TOPIC_PUBLISH_DATA "ESP32_Sensors_BME280"


#define led 2
#define CHAR_BUFFER_SIZE 50
char msg[CHAR_BUFFER_SIZE];
String sMac = WiFi.macAddress();
const char *mac = sMac.c_str();

DynamicJsonDocument jsonData(JSON_OBJECT_SIZE(1024));
unsigned long dateHour = jsonData["dataHora"];
float tempJson = jsonData["Temperatura"];
float humiJson = jsonData["Umidade"];
float pressureJson = jsonData["Pressao"];
const char *macJson = jsonData["IDMac"];
const char *ID_MQTT_ESP32 = concatChar("ESP32_ID_", mac);

WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP, "a.st1.ntp.br");
Mean temperatureMean;
Mean pressureMean;
Mean humidityMean;
Counter count;
Counter getDataSensorCounter;

void ntpInit() {
  ntp.begin();
  ntp.forceUpdate();
}

void setup() {
  // initSerial();
  // initBME280();
  WifiManager();
  initMqtt();
  ntpInit();
  pinMode(led, OUTPUT);
}

void ledBlink() {
  static unsigned long currentTimeLed = millis();
  if ((millis() - currentTimeLed) < 500) {
    digitalWrite(led, 1);
  }

  else {
    digitalWrite(led, 0);
  }

  if ((millis() - currentTimeLed) > 1000){
    currentTimeLed = millis();
  }
}

void getSensorData() {
  float temperature, humidity, pressure;
  if (int(getDataSensorCounter.getCounter()) % 7000 == 0) {
    temperature = float(random(24, 35));
    temperatureMean.sum(temperature);

    pressure = float(random(930, 945));
    pressureMean.sum(pressure);

    humidity = float(random(30, 90));
    humidityMean.sum(humidity);

    count.increaseCounter();
  }

  getDataSensorCounter.increaseCounter();
  ledBlink();
}

void printValues() {
  // Serial.println(count.getCounter());
  // Serial.println(ID_MQTT_ESP32);

  if (ntp.forceUpdate()) {
    unsigned long date = ntp.getEpochTime();
    jsonData["dataHora"] = date;
  }
  else {
    jsonData["dataHora"] = -1 ;
  }
  
  jsonData["Temperatura"] = temperatureMean.getMean(count.getCounter());
  temperatureMean.resetSum();

  jsonData["Umidade"] = humidityMean.getMean(count.getCounter());
  humidityMean.resetSum();

  jsonData["Pressao"] = pressureMean.getMean(count.getCounter());
  pressureMean.resetSum();

  jsonData["IDMac"] = mac;

  size_t sizeMsg = measureJson(jsonData) + 1;
  char msg[sizeMsg];
  serializeJson(jsonData, msg, sizeMsg);

  // Serial.print("Data = ");
  // Serial.println(msg);
  MQTT.publish(TOPIC_PUBLISH_DATA, msg);
  
  count.resetCounter();
  getDataSensorCounter.resetCounter();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;

  for(int i = 0; i < length; i++){
    char c = (char)payload[i];
    msg += c;
  }
  if (msg == "return") {
    const char *cMsg = msg.c_str();
    printValues();
    // Serial.println(cMsg);
  }
}

void loop() { 
  checkConnectionsWifiMqtt(ID_MQTT_ESP32);
  MQTT.setCallback(mqttCallback);
  getSensorData();
  MQTT.loop();
}
