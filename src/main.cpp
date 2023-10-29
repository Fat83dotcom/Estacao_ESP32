#include <Arduino.h>
#include <cstring>
#include <ArduinoJson.h>
#include "classesAndFunctions.h"


#define TOPIC_PUBLISH_DATA "ESP32_Sensors_BME280"


#define led 2
#define MSG_BUFFER_SIZE 50
char msg[MSG_BUFFER_SIZE];
String sMac = WiFi.macAddress();
const char *mac = sMac.c_str();
const char *tempJson = "Temperatura";
const char *humiJson = "Umidade";
const char *pressureJson = "Pressao";
const char *macJson = "IDMac";
const char *ID_MQTT_ESP32 = concatChar("ESP32_ID_", mac);

Mean temperatureMean;
Mean pressureMean;
Mean humidityMean;
Counter count;
Counter getDataSensorCounter;
DynamicJsonDocument jsonData(JSON_OBJECT_SIZE(4));

void setup() {
  initSerial();
  // initBME280();
  WifiManager();
  initMqtt();
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
    temperature = 30.1F;
    temperatureMean.sum(temperature);

    pressure = 945.23F;
    pressureMean.sum(pressure);

    humidity = 45.4F;
    humidityMean.sum(humidity);

    count.increaseCounter();
  }

  getDataSensorCounter.increaseCounter();
  ledBlink();
}

void printValues() {
  Serial.println(count.getCounter());
  Serial.println(ID_MQTT_ESP32);

  jsonData[tempJson] = temperatureMean.getMean(count.getCounter());
  temperatureMean.resetSum();

  jsonData[humiJson] = humidityMean.getMean(count.getCounter());
  humidityMean.resetSum();

  jsonData[pressureJson] = pressureMean.getMean(count.getCounter());
  pressureMean.resetSum();

  jsonData[macJson] = mac;

  size_t sizeMsg = measureJson(jsonData) + 1;
  char msg[sizeMsg];
  serializeJson(jsonData, msg, sizeMsg);

  Serial.print("Data = ");
  Serial.println(msg);
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
    Serial.println(cMsg);
  }
}

void loop() { 
  checkConnectionsWifiMqtt(ID_MQTT_ESP32);
  MQTT.setCallback(mqttCallback);
  getSensorData();
  MQTT.loop();
}
