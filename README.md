# ESP32 e Sensor BME280
## Captação de dados
* A finalidade da estação meteorológica BrainStorm é captar dados através de sensores distribuídos por diferentes localidades, mantendo esses dados em um banco com alta disponibilidade e performance além de proporcionar uma interface para a visualização dos dados de forma simples e intuitiva.
* A escolha do ESP32 para compor o dispositivo de captação vem da sua facilidade de conexão com redes domésticas, computação performática e preço acessível.
* Compondo o dispositivo de captação, o sensor BME280 tem a vantagem de se obter 3 grandezas simultaneamente, temperatura, umidade e pressão atmosférica no mesmo chip, contando com um pré-processamento no próprio chip. Conectando-se com o ESP32 através de uma conexão I2C, permitindo assim a possibilidade de usar outros dispositivos no mesmo barramento I2C, caso isso venha ser necessário no futuro, auxiliando na escalabilidade do projeto além de facilitar a ligação entre os dispositivos.
* A precisão do sensor é especificada como umidade com precisão de ±3%, pressão barométrica com precisão absoluta de ±1 hPa e temperatura com precisão de ±1,0°C. Mais informações e especificações técnicas podem ser encontradas [Aqui](https://www.mouser.com/datasheet/2/783/BST-BME280-DS002-1509607.pdf) e [Aqui](https://www.bosch-sensortec.com/media/boschsensortec/downloads/product_flyer/bst-bme280-fl000.pdf).
* A API do sensor é bastante simples e trivial para o uso neste caso, sendo que primeiramente o sensor deve ser iniciado da seguinte forma:

```
void initBME280() {
  // Serial.println("BME280 test");
  unsigned status;
  status = bme.begin(0x76); // Inicia o sensor no endereço 0x76
 
  if (!status) {
    // Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    // Serial.print("ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    // Serial.print("ID of 0x56-0x58 represents a BMP 280,\n");
    // Serial.print("ID of 0x60 represents a BME 280.\n");
    // Serial.print("ID of 0x61 represents a BME 680.\n");
    while (1) {
      // Serial.printf("Verifique o sensor BME280...");
      delay(10);
      // Se nenhum sensor for detectado na inicialização, o programa ficará preso neste loop, e nunca será inicializado. Caso isto aconteça, verifique as ligações do esquema eletrônico.
    } 
  }

  Serial.println("-- BME280 Test OK --");
  Serial.print("SensorID was: 0x"); 
  Serial.println(bme.sensorID(),16);
  Serial.println();
}
```
* Implementei procedimentos para a inicialização de todo o programa para facilitar a manipulação e limpeza.
``` 
void setup() {
  initSerial();
  initBME280(); // O sensor é inicializado aqui
  WifiManager();
  initMqtt();
  ntpInit();
  pinMode(led, OUTPUT);
}
```
* A biblioteca usada foi a [Adafruit_BME280.h](https://github.com/adafruit/Adafruit_BME280_Library?tab=readme-ov-file) e sua dependência [Adafruit_Sensor.h](https://github.com/adafruit/Adafruit_Sensor).

### Como o algoritmo funciona.
* O propósito deste README não é de um tutorial mas sim de apontar algumas soluções e adaptações que desenvolvi partindo da própria documentação das bibliotecas e recursos usados, lembrando que a maioria dos recursos usados são disponibilizados pela comunidade open source e meu agradecimento é grande a todos que disponibilizam seus conhecimentos de forma gratuita.
* Antes de mais nada, vou deixar definido aqui um escopo para que isso ajude a justificar minhas decisões de projeto.
1. Os sensores devem ser distribuídos por diferentes localidades, têm identidade única e totalmente independentes uns dos outros, ou seja, é um projeto internet das coisas (IOT).
2. O protocolo deve ser do tipo publish/subscribe, para que haja uma sincronização entre eles a partir de uma central controladora (cliente remoto MQTT).
3. Os dispositivos captadores devem ser de baixo custo, de fácil instalação e manutenção, seus operadores devem ter noções básicas de programação em linguagem C ou C++, conhecimentos básicos para instalação de programas e noções básicas de eletrônica.
* Para solucionar o item 2, decidi usar o protocolo MQTT por razões óbvias; é um protocolo amplamente usado e consolidado na comunidade IOT; é leve e otimizado para redes de baixa largura de banda, alta latência e não confiáveis; tem uma vasta documentação e bibliotecas cliente em várias linguagens populares.
* A API utilizada nos sensores foi a [PubSubClient.h](https://github.com/knolleary/pubsubclient), que é muito utilizada e tem uma [interface](https://pubsubclient.knolleary.net/api) fácil de usar.
#### O algoritmo é simples e trivial:
```
    void loop() {
    checkConnectionsWifiMqtt(ID_MQTT_ESP32, mqttUsername, mqttPassword);
    MQTT.setCallback(mqttCallback);
    getSensorData();
    MQTT.loop();
    }
```
* `checkConnectionsWifiMqtt(ID_MQTT_ESP32, mqttUsername, mqttPassword);` gerencia a conexão com o broker.
* `MQTT.setCallback(mqttCallback);` um ponteiro para uma função de retorno de chamada de mensagem, chamada quando uma mensagem chega para uma assinatura criada por este cliente.
* `getSensorData();` efetua a média das leituras dos sensores até que seja solicitada pelo callback.
* `MQTT.loop();` Deve ser chamado regularmente para permitir que o cliente processe as mensagens recebidas e mantenha sua conexão com o servidor. Retorna true se conectado e false se não.
#### Estrutura de dados:
* O callback aguarda uma mensagem do cliente remoto MQTT, que é disparada a cada minuto:
 ```
    for(int i = 0; i < length; i++){
        char c = (char)payload[i];
        msg += c;
    }

    if (msg == "return") {
        const char *cMsg = msg.c_str();
        ...
    }
```
* Dessa forma, a função responsável por enviar os dados ao cliente remoto MQTT é acionada:
```
    if (msg == "return") {
        const char *cMsg = msg.c_str();
        printValues();
    }
```

```
void printValues() {
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

    MQTT.publish(TOPIC_PUBLISH_DATA, msg);
    
    count.resetCounter();
    getDataSensorCounter.resetCounter();
}
```
* O procedimento `void printValues()` envia os dados em um Json ao cliente remoto MQTT chamando `MQTT.publish(TOPIC_PUBLISH_DATA, msg);` e passando os dados através de `msg`.
* A estrutura escolhida foi o formato Json, por ser um formato consolidado na WEB e por estratégia, facilitar o envio dos dados a outros clientes, por exemplo, diretamente a uma página de cliente WEB ou outra estrutura que por ventura venha ser projetada futuramente.
* O formato da estrutura recebida pelo cliente remoto MQTT é a seguinte:
`{"dataHora":1711850377,"Temperatura":28.00083923,"Umidade":74.27648163,"Pressao":939.9761353,"IDMac":"xx:xx:xx:xx:xx:xx"}`
* O campo `"dataHora":1711850377` é obtido por um servidor NTP (Network Time Protocol), usado para sincronizar os horários do relógio do computador em uma rede. Se esse recurso falhar, a hora será definida pelo cliente remoto MQTT, sendo -1 retornado pelo campo. O formato Unix Time Stamp foi mantido para simplificação do código e por ser facilmente tratado em qualquer sistema.
* O campo `"IDMac":"xx:xx:xx:xx:xx:xx"` é a forma de identificação de cada sensor no banco de dados.
* A biblioteca usada para conversão Json foi a [ArduinoJson.h](https://github.com/bblanchon/ArduinoJson?tab=readme-ov-file) e a documentação da API pode ser encontrada [Aqui](https://arduinojson.org/).
* Para a aplicação desta API é necessário conhecimento do formato Json e C++.

#### Operação do dispositivo:
* Entenda a partir de agora 'dispositivo' como o conjunto ESP32 com o sensor BME280.
* A conexão do dispositivo com um rede se dá de uma maneira muito simples, que pode ser feita por qualquer pessoa com um smartphone Android (não testei em dispositivos iOS, mas acredito que o procedimento será o mesmo).
* A biblioteca usada foi [WiFiManager.h](https://github.com/tzapu/WiFiManager).
* Não fiz nenhuma alteração no código, a não ser o nome da estação:

```
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
        if (!wfm.startConfigPortal("BrainStorm Tecnologia - IOT")){
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
```
* Essa solução funciona muito bem e é amplamente usada pela comunidade, sendo assim, deixo [Aqui](https://github.com/tzapu/WiFiManager) a documentação, com códigos e exemplos de uso para eventuais consultas.
### Considerações finais:
* Este projeto pode ser adaptado para diversas finalidades onde seja requerido a captação de dados atmosféricos ou não, que envolvam temperatura, umidade e pressão.
* Este documento tem como finalidade apresentar o projeto de maneira simplificada, demonstrando os principais aspectos e suas respectivas soluções.
* As plataformas de desenvolvimento usadas neste projeto foram:
* [ESP32 DevKit V1](https://www.circuitstate.com/pinouts/doit-esp32-devkit-v1-wifi-development-board-pinout-diagram-and-reference/)
* [BME280](https://makerselectronics.com/product/bme280-i2c-pressure-humidity-temperature-sensor-module)
* [Esquema de ligação](https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2019/06/ESP32-bme280_schematic.jpg?w=768&quality=100&strip=all&ssl=1)

## Referências bibliográficas:
* [Protocolo MQTT](https://mqtt.org/)
* [Protocolo HTTP](https://developer.mozilla.org/pt-BR/docs/Web/HTTP/Overview)
* [Protocolo NTP](https://ntp.br/conteudo/ntp/)
* [Protocolo I2C](https://www.nxp.com/docs/en/user-guide/UM10204.pdf)
* [Formato Json](https://www.json.org/json-pt.html#:~:text=JSON%20%C3%A9%20em%20formato%20texto,ideal%20de%20troca%20de%20dados.)
* [ESP32](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html)
* [BME280](https://cdn-learn.adafruit.com/downloads/pdf/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout.pdf)
* [Referência pinos ESP32 DevKit V1](https://www.circuitstate.com/pinouts/doit-esp32-devkit-v1-wifi-development-board-pinout-diagram-and-reference/)
* [API Arduino](https://www.arduino.cc/)
* Consultados em 31/03/2024 às 00:30.
