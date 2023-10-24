#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

#define TOPIC_SUBSCRIBE "ESP32_Receive_Information"
#define TOPIC_PUBLISH_HUMIDITY "ESP32_Sensors_BME280_HUMI"
#define TOPIC_PUBLISH_TEMPERATURE "ESP32_Sensors_BME280_TEMP"
#define TOPIC_PUBLISH_PRESURE "ESP32_Sensors_BME280_PRESS"
#define ID_MQTT "ESP32_MQTT_BRANISTORMTECH"
#define led 2

#define MSG_BUFFER_SIZE 50
char msg[MSG_BUFFER_SIZE];

const char *BROKER_MQTT = "broker.hivemq.com";
int BROKER_PORT = 1883;

WiFiClient espClient;
PubSubClient MQTT(espClient);

Adafruit_BME280 bme;
unsigned long delayTime;
WiFiManager wfm;

void reconnectWifi(){
  while (1){
    wfm.autoConnect();
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

void WifiManager(){
  wfm.setAPCallback(configModeCallback);
  wfm.setSaveConfigCallback(saveConfigCallback);
  reconnectWifi();
}

void initBME280(){
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

void initSerial(){
  Serial.begin(9600);
  while(!Serial);    // time to get serial running
}

void initWifi(void){
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");
  reconnectWifi();
}

void reconnectMqtt(void){
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

void mqttCallback(char* topic, byte* payload, unsigned int length){
  String msg;

  //obtem a string do payload recebido
  for(int i = 0; i < length; i++){
    char c = (char)payload[i];
    msg += c;
  }
  Serial.print("[MQTT] Mensagem recebida: ");
  Serial.println(msg);     
}

void init_mqtt(void){
  /* informa a qual broker e porta deve ser conectado */
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); 
  /* atribui função de callback (função chamada quando qualquer informação do 
  tópico subescrito chega) */
  MQTT.setCallback(mqtt_callback);            
}

void verifica_conexoes_wifi_mqtt(void){
  /* se não há conexão com o WiFI, a conexão é refeita */
  reconnect_wifi(); 
  /* se não há conexão com o Broker, a conexão é refeita */
  if (!MQTT.connected()) 
    reconnect_mqtt(); 
} 

void setup() {
  Serial.begin(9600);
  init_wifi();
  init_mqtt();
  pinMode(led, OUTPUT);
  while(!Serial);    // time to get serial running
  Serial.println(F("BME280 test"));

  unsigned status;
  
  status = bme.begin(0x76);
 
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }
  
  Serial.println("-- Default Test --");
  delayTime = 1000;

  Serial.println();
}

void printValues() {
  static unsigned long currentTime = millis();
  static unsigned long currentTimeLed = millis();
  if ((millis() - currentTime) < 1){
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" °C");

    Serial.print("Pressure = ");
    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

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

void loop() {
  /* garante funcionamento das conexões WiFi e ao broker MQTT */
  verifica_conexoes_wifi_mqtt();
  /* Envia frase ao broker MQTT */
  MQTT.publish(TOPIC_PUBLISH, "ESP32 se comunicando com MQTT");

  /* keep-alive da comunicação com broker MQTT */    
  MQTT.loop();
  /* Agurda 1 segundo para próximo envio */
  delay(1000);
}
