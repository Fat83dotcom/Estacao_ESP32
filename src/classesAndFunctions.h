#ifndef CLASSES_AND_FUNCTIONS_H
#define CLASSES_AND_FUNCTIONS_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "passwords.h"

#define TOPIC_SUBSCRIBE "Require_Data"
#define TOPIC_PUBLISH_DATA "ESP32_Sensors_BME280"

const char* BROKER_MQTT = "brokermqtt-estacao.brainstormtech.com.br";
const char* mqttUsername = user; // replace with your Username
const char* mqttPassword = password; // replace with your Password
int BROKER_PORT = 1883;

WiFiClient espClient;
PubSubClient MQTT(espClient);
Adafruit_BME280 bme;
WiFiManager wfm;

class TempData;
class FilterNaN;
class Mean;
class Counter;
void reconnectWifi();
void configModeCallback(WiFiManager *myWiFiManager);
void saveConfigCallback();
void WifiManager();
void initBME280();
void initSerial();
void reconnectMqtt(const char *IDMqtt, const char* user, const char* password);
void initMqtt(void);
void checkConnectionsWifiMqtt(const char *IDMqtt, const char* user, const char* password);
const char* concatChar(const char *preFix, const char *mac);

class Counter {
  private:
    float __count = 0;
  public:
    Counter(){}
    ~Counter(){}

    void increaseCounter() {
      this->__count++;
    }

    void resetCounter() {
      this->__count = 0;
    }

    float getCounter() {
      return this->__count;
    }
};

class Mean {
  private:
    float __sum = 0;
  
  public:
    Mean(){}
    ~Mean(){}

    void sum(float data) {
      this->__sum += data;
    }

    void resetSum() {
      this->__sum = 0;
    }

    float getMean(float divisor) {
      return (this->__sum / divisor);
    }
};

void reconnectWifi(){
  while (1){
    wfm.autoConnect("BrainStorm Tecnologia - IOT");
    if (WiFi.status() == WL_CONNECTED){
      Serial.println(WiFi.SSID());
      Serial.println(WiFi.localIP());
      break;
    }
    else{
      wfm.resetSettings();
      if (!wfm.startConfigPortal("BrainStorm Tecnologia - IOT", "12345678")){                                      // Nome da Rede e Senha gerada pela ESP
        Serial.println("Falha ao conectar");
        delay(2000);
        ESP.restart();
      }
      else{
        Serial.println("Conectado na Rede!!!");
        break;
      }
    }
  }  
}

// callback que indica que o ESP entrou no modo AP
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entrou no modo de configuração");
  Serial.println(WiFi.softAPIP());                      // imprime o IP do AP
  Serial.println(myWiFiManager->getConfigPortalSSID()); // imprime o SSID criado da rede
}

// Callback que indica que salvamos uma nova rede para se conectar (modo estação)
void saveConfigCallback() {
  Serial.println("Configuração salva");
}

void WifiManager() {
  wfm.setAPCallback(configModeCallback);
  wfm.setSaveConfigCallback(saveConfigCallback);
  reconnectWifi();
}

void initBME280() {
  // Serial.println("BME280 test");
  unsigned status;
  status = bme.begin(0x76);
 
  if (!status) {
    // Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    // Serial.print("ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    // Serial.print("ID of 0x56-0x58 represents a BMP 280,\n");
    // Serial.print("ID of 0x60 represents a BME 280.\n");
    // Serial.print("ID of 0x61 represents a BME 680.\n");
    while (1) {
      // Serial.printf("Verifique o sensor BME280...");
      delay(10);
    } 
  }

  Serial.println("-- BME280 Test OK --");
  Serial.print("SensorID was: 0x"); 
  Serial.println(bme.sensorID(),16);
  Serial.println();
}

void initSerial() {
  Serial.begin(9600);
  while(!Serial);    // time to get serial running
}

void reconnectMqtt(const char *IDMqtt, const char* user, const char* password) {
  while (!MQTT.connected()){
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(IDMqtt, user, password)){
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPIC_SUBSCRIBE); 
    } 
    else{
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentatica de conexao em 2s");
    }
  }
}

// void mqttCallback(char* topic, byte* payload, unsigned int length) {
//   String msg;

//   //obtem a string do payload recebido
//   for(int i = 0; i < length; i++){
//     char c = (char)payload[i];
//     msg += c;
//   }
//   Serial.print("[MQTT] Mensagem recebida: ");
//   Serial.println(msg);     
// }

void initMqtt(void) {
  /* informa a qual broker e porta deve ser conectado */
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);   
}

void checkConnectionsWifiMqtt(const char *IDMqtt, const char* user, const char* password) {
  /* se não há conexão com o WiFI, a conexão é refeita */
  while (WiFi.status() != WL_CONNECTED){
    reconnectWifi(); 
  }
  /* se não há conexão com o Broker, a conexão é refeita */
  while (!MQTT.connected()){
    reconnectMqtt(IDMqtt, user, password); 
  }
}

const char* concatChar(const char *preFix, const char *mac) {
  int bufferSize = strlen_P(preFix) + strlen_P(mac) + 1;
  char *ID = new char[bufferSize];
  strcpy_P(ID, preFix);
  strcat_P(ID, mac);
  return ID;
}

class TempData{
  private:
  double t_Umi;
  double t_Temp;
  double t_Press;

  public:
  double *pt_U = &t_Umi;
  double *pt_T = &t_Temp;
  double *pt_P = &t_Temp;
};

class FilterNaN{
  private:
  double _Umidity;
  double _Temperature;
  double _Press;
  int counter;
  int laps = 1000;

  public:
  double umi_NaN (double umidity, double *pUmidity) {
    _Umidity = umidity;
    if (!isnan(_Umidity)) {
      *pUmidity = _Umidity;
    }
    counter = 0;
    while (isnan(_Umidity) && counter < laps){
      _Umidity = umidity;
      counter++;
    }
    return (counter == laps) ? *pUmidity : _Umidity;
  }

  double temp_NaN (double temperature, double *pTemperature) {
    _Temperature = temperature;
    if (!isnan(_Temperature)) {
      *pTemperature = _Temperature;
    }
    counter = 0;
    while (isnan(_Temperature) && counter < laps) {
      _Temperature = temperature;
      counter++;
    }
    return (counter == laps) ? *pTemperature : _Temperature; 
  }

  double press_NaN (double pressure, double *pPressure) {
    _Press = pressure;
    if (!isnan(_Press)) {
      *pPressure = _Press;
    }
    counter = 0;
    while (isnan(_Press) && counter < laps) {
      _Press = pressure;
      counter++;
    }
    return (counter == laps) ? *pPressure : _Press;
  }
};

#endif