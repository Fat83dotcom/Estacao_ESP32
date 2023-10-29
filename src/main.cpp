#include <Arduino.h>
#include <cstring> 
#include "classesAndFunctions.h"

#define TOPIC_PUBLISH_MAC "ESP32_MAC_BRAIN"
#define TOPIC_PUBLISH_HUMIDITY "ESP32_Sensors_BME280_HUMI"
#define TOPIC_PUBLISH_TEMPERATURE "ESP32_Sensors_BME280_TEMP"
#define TOPIC_PUBLISH_PRESURE "ESP32_Sensors_BME280_PRESS"


#define led 2
#define MSG_BUFFER_SIZE 50
char msg[MSG_BUFFER_SIZE];
String sMac = WiFi.macAddress();
const char *mac = sMac.c_str();

Mean temperatureMean;
Mean pressureMean;
Mean humidityMean;
Counter count;
Counter getDataSensorCounter;

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

  MQTT.publish(TOPIC_PUBLISH_MAC, mac);
  Serial.println(mac);

  Serial.print("Temperature = ");
  Serial.print(temperatureMean.getMean(count.getCounter()));
  Serial.println(" °C");
  sprintf(msg, "%.2f", temperatureMean.getMean(count.getCounter()));
  temperatureMean.resetSum();
  MQTT.publish(TOPIC_PUBLISH_TEMPERATURE, msg);

  Serial.print("Pressure = ");
  Serial.print(pressureMean.getMean(count.getCounter()));
  Serial.println(" hPa");
  sprintf(msg, "%.2f", pressureMean.getMean(count.getCounter()));
  pressureMean.resetSum();
  MQTT.publish(TOPIC_PUBLISH_PRESURE, msg);

  Serial.print("Humidity = ");
  Serial.print(humidityMean.getMean(count.getCounter()));
  Serial.println(" %");
  sprintf(msg, "%.2f", humidityMean.getMean(count.getCounter()));
  humidityMean.resetSum();
  MQTT.publish(TOPIC_PUBLISH_HUMIDITY, msg);
  
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
  checkConnectionsWifiMqtt();
  MQTT.setCallback(mqttCallback);
  getSensorData();
  MQTT.loop();
}
