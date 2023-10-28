#ifndef CLASSES_AND_FUNCTIONS_H
#define CLASSES_AND_FUNCTIONS_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define TOPIC_SUBSCRIBE "ESP32_Receive_Information"
#define TOPIC_PUBLISH_HUMIDITY "ESP32_Sensors_BME280_HUMI"
#define TOPIC_PUBLISH_TEMPERATURE "ESP32_Sensors_BME280_TEMP"
#define TOPIC_PUBLISH_PRESURE "ESP32_Sensors_BME280_PRESS"
#define ID_MQTT "ESP32_MQTT_BRANISTORMTECH"

const char *BROKER_MQTT = "broker.hivemq.com";
int BROKER_PORT = 1883;

WiFiClient espClient;
PubSubClient MQTT(espClient);
Adafruit_BME280 bme;
unsigned long delayTime;
WiFiManager wfm;

class Mean;
void reconnectWifi();
void configModeCallback(WiFiManager *myWiFiManager);
void saveConfigCallback();
void WifiManager();
void initBME280();
void initSerial();
void reconnectMqtt(void);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void initMqtt(void);
void checkConnectionsWifiMqtt(void);

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
  Serial.println(F("BME280 test"));
  unsigned status;
  status = bme.begin(0x76);
 
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("ID of 0x60 represents a BME 280.\n");
    Serial.print("ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  Serial.println("-- Default Test --");
  Serial.print("SensorID was: 0x"); 
  Serial.println(bme.sensorID(),16);
  Serial.println();
}

void initSerial() {
  Serial.begin(9600);
  while(!Serial);    // time to get serial running
}

void reconnectMqtt(void) {
  while (!MQTT.connected()){
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)){
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPIC_SUBSCRIBE); 
    } 
    else{
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentatica de conexao em 2s");
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;

  //obtem a string do payload recebido
  for(int i = 0; i < length; i++){
    char c = (char)payload[i];
    msg += c;
  }
  Serial.print("[MQTT] Mensagem recebida: ");
  Serial.println(msg);     
}

void initMqtt(void) {
  /* informa a qual broker e porta deve ser conectado */
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); 
  /* atribui função de callback (função chamada quando qualquer informação do 
  tópico subescrito chega) */
  MQTT.setCallback(mqttCallback);            
}

void checkConnectionsWifiMqtt(void) {
  /* se não há conexão com o WiFI, a conexão é refeita */
  if (WiFi.status() != WL_CONNECTED){
    reconnectWifi(); 
  }
  /* se não há conexão com o Broker, a conexão é refeita */
  if (!MQTT.connected()){
    reconnectMqtt(); 
  }
} 

#endif