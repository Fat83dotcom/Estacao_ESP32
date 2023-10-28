#include <Arduino.h>
#include "classesAndFunctions.h"

#define TOPIC_SUBSCRIBE "ESP32_Receive_Information"
#define TOPIC_PUBLISH_HUMIDITY "ESP32_Sensors_BME280_HUMI"
#define TOPIC_PUBLISH_TEMPERATURE "ESP32_Sensors_BME280_TEMP"
#define TOPIC_PUBLISH_PRESURE "ESP32_Sensors_BME280_PRESS"
#define ID_MQTT "ESP32_MQTT_BRANISTORMTECH"

#define led 2
#define MSG_BUFFER_SIZE 50
char msg[MSG_BUFFER_SIZE];

Mean temperatureMean;
Mean pressureMean;
Mean humidityMean;

void printValues() {
  static float count = 0;
  static unsigned long currentTime = millis();
  static unsigned long currentTimeLed = millis();
  float temperature, humidity, pressure;

  if ((millis() - currentTime) < 1){
    temperature = 30.1F;
    temperatureMean.sum(temperature);

    pressure = 945.23F;
    pressureMean.sum(pressure);

    humidity = 45.4F;
    humidityMean.sum(humidity);

    count++;
  }

  if (count > 59){
    Serial.print("Temperature = ");
    Serial.print(temperatureMean.getMean(count));
    Serial.println(" °C");
    sprintf(msg, "%.2f", temperatureMean.getMean(count));
    temperatureMean.resetSum();
    MQTT.publish(TOPIC_PUBLISH_TEMPERATURE, msg);

    Serial.print("Pressure = ");
    Serial.print(pressureMean.getMean(count));
    Serial.println(" hPa");
    sprintf(msg, "%.2f", pressureMean.getMean(count));
    pressureMean.resetSum();
    MQTT.publish(TOPIC_PUBLISH_PRESURE, msg);

    Serial.print("Humidity = ");
    Serial.print(humidityMean.getMean(count));
    Serial.println(" %");
    sprintf(msg, "%.2f", humidityMean.getMean(count));
    MQTT.publish(TOPIC_PUBLISH_HUMIDITY, msg);
    humidityMean.resetSum();
    count = 0;

    Serial.println();
  }
  
  if ((millis() - currentTime) > 999){
    currentTime = millis();
  }

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

void setup() {
  initSerial();
  // initBME280();
  WifiManager();
  initMqtt();
  pinMode(led, OUTPUT);
}

void loop() { 
  checkConnectionsWifiMqtt();
  printValues();
  /* keep-alive da comunicação com broker MQTT */    
  MQTT.loop();
}
