/*
   Прошивка сгенерирована сервисом GreenPL Codio
   для быстрого подключения x-Duino плат к
   платформе для интернета вещей GreenPL.
   Программный код рекомендуется использовать
   исключительно для подключения платы к GreenPL.
   !! ВНИМАНИЕ! Библиотеки, необходимые для работы:
   PubSubClient (http://pubsubclient.knolleary.net/)
   SimpleTimer (http://cdn.greenpl.ru/SimpleTimer.zip)
      Adafruit_Sensor (http://cdn.greenpl.ru/codio/libs/Adafruit_Unified_Sensor.zip)
   Для установки библиотек выполните следующие шаги:
   Скетч -> Подключить библиотеку -> Добавить .ZIP библиотеку
   Выберите zip-архив с загруженной библиотекой.
   Библиотека установится автоматически. В нижней части окна
   вы получите уведомление об успешности операции.
   Перед загрузкой кода убедитесь, что библиотеки
   корретно установлены в среду разработки Arduino IDE.
*/
#define WIFI_SSID "IOTIK"
#define WIFI_PASSWORD "Terminator812"
#include <PubSubClient.h>
#include <SimpleTimer.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <VEML6075.h>
#define TOKEN  "0d790bb6e9be44a9ef0090b8"
#define CLIENT_ID "iotik-0d790bb6e9be44a9ef0090b8"
#include <Adafruit_ADS1015.h>
#include <BH1750FVI.h>        // добавляем библиотеку датчика освещенности // adding Light intensity sensor library  
BH1750FVI LightSensor_1;      // BH1750

#include <Servo.h>                     // конфигурация сервомотора // servo configuration
Servo myservo;
int r,g,b;
int pos = 1;            // начальная позиция сервомотора // servo start position
int prevangle = 1;      // предыдущий угол сервомотора // previous angle of servo
Adafruit_ADS1015 ads(0x48);
const float air_value    = 83900.0;
const float water_value  = 45000.0;
const float moisture_0   = 0.0;
const float moisture_100 = 100.0;
#include <FastLED.h>                   // конфигурация матрицы // LED matrix configuration   
#include <FastLED_GFX.h>
#include <FastLEDMatrix.h>
#define NUM_LEDS 64                    // количество светодиодов в матрице // number of LEDs 
CRGB leds[NUM_LEDS];                   // определяем матрицу (FastLED библиотека) // defining the matrix (fastLED library)
#define LED_PIN             18         // пин к которому подключена матрица // matrix pin
#define COLOR_ORDER         GRB        // порядок цветов матрицы // color order 
#define CHIPSET             WS2812     // тип светодиодов // LED type            
/*
   Путь (топик) до устройства (см. MQTT API на сайте http://docs.greenpl.ru)
*/
String TOPIC =  "/devices/iotik-" + String((uint32_t)(ESP.getEfuseMac()));
char TOPIC_CHAR[100];
WiFiClient client;

#define Relay_mgbot_mgbot_relay_1_16 16
#define Relay_mgbot_mgbot_relay_2_17 17


VEML6075 veml6075;

// Датчик температуры/влажности и атмосферного давления
Adafruit_BME280 mgbot_bme280_21;
PubSubClient GreenPLClient(client);
// Таймер обновления вывода на сервер IoT
long timer_iot = 0;
// Частота вывода данных на сервер IoT
#define IOT_UPDATE_TIME 5000
// Таймер переподключения MQTT
long lastReconnectAttempt = 0;
//таймер
SimpleTimer sendToGreenPLTimer;
// Параметры IoT сервера
char greenplServer[] = "api.greenpl.ru";
void setup() {
  // Инициализация последовательного порта на скорости 115200
  Serial.begin(115200);
  //Ждем 1 секунду (1000 мс)
  delay(1000);
    pinMode(Relay_mgbot_mgbot_relay_1_16, OUTPUT);
  pinMode(Relay_mgbot_mgbot_relay_2_17, OUTPUT);
 digitalWrite(Relay_mgbot_mgbot_relay_1_16, 0);
  digitalWrite(Relay_mgbot_mgbot_relay_2_17, 0);
  delay(2000);
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);    // конфигурация матрицы // matrix configuration
  fill_solid( leds, NUM_LEDS, CRGB(255, 255, 0));        // заполнить всю матрицу желтым цветом
  FastLED.setBrightness(50);                     // устанавливаем яркость по значению 50
  FastLED.show();
  ads.setGain(GAIN_TWOTHIRDS);
  ads.begin();    // включем АЦП // turn the ADC on
  myservo.attach(19);             // пин сервомотора // servo pin

  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ok");
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

#ifndef WIRE_BEGIN
#define WIRE_BEGIN
  // Инициализация I2C
  Wire.begin();
  Wire.setClock(10000L);
#endif
  // Инициализация датчика BME280
  bool mgbot_bme280_21_status = mgbot_bme280_21.begin();
  if (!mgbot_bme280_21_status)
    Serial.println("Could not find a valid mgbot_bme280_21 sensor, check wiring!");
  sendToGreenPLTimer.setInterval(IOT_UPDATE_TIME, f_mgbot_bme280_21);

  if (!veml6075.begin())
    Serial.println("VEML6075 not found!");
  LightSensor_1.begin();              // запуск датчика освещенности // turn the light intensity sensor on
  LightSensor_1.setMode(Continuously_High_Resolution_Mode);
  /*
     Настраиваем MQTT подключение
  */
  GreenPLClient.setServer( greenplServer, 1883 );
  GreenPLClient.setCallback( mqttCallback );
  TOPIC.toCharArray(TOPIC_CHAR, 100);
  
}
void loop() {
  //переподключаемся, если отключились
  if (!GreenPLClient.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.println("=== MQTT NOT CONNECTED ===");
    // переподключаемся каждые 10 секунд
    unsigned long t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi fail. Reconnecting to network..");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }
  GreenPLClient.loop();
  sendToGreenPLTimer.run();
}
void f_mgbot_bme280_21() {
  float l = LightSensor_1.getAmbientLight();
  GreenPLPublish("light", l);
  delay(10);
  veml6075.poll();
  float uva = veml6075.getUVA();
  float uvb = veml6075.getUVB();
  float uv_index = veml6075.getUVIndex();
  GreenPLPublish("uva", uva);
  delay(10);
  GreenPLPublish("uvb", uvb);
  delay(10);
  GreenPLPublish("uv_index", uv_index);
  delay(10);
  Serial.println("UVA = " + String(uva, 1) + "   ");
  Serial.println("UVB = " + String(uvb, 1) + "   ");
  Serial.println("UV INDEX = " + String(uv_index, 1) + "   ");

  float air_temp = mgbot_bme280_21.readTemperature();
  GreenPLPublish("mgbot_bme280_21_temp", air_temp);
  delay(10);

  float air_hum = mgbot_bme280_21.readHumidity();
  GreenPLPublish("mgbot_bme280_21_hum", air_hum);
  delay(10);
  float air_press = mgbot_bme280_21.readPressure() / 100.0F;
  GreenPLPublish("mgbot_bme280_21_press", air_press);
  delay(10);
  
  float adc0 = (float)ads.readADC_SingleEnded(0) * 6.144 * 16;
  float adc1 = (float)ads.readADC_SingleEnded(1) * 6.144 * 16;
  float t1 = ((adc1 / 1000)); //1023.0 * 5.0) - 0.5) * 100.0;
  float h1 = map(adc0, air_value, water_value, moisture_0, moisture_100);
  GreenPLPublish("soilh", h1);
  GreenPLPublish("soilt", t1);

}
void GreenPLPublish(String variable, float value) {
  String payload = "{\"";
  payload += variable;
  payload += "\" : ";
  payload += String(value);
  payload += "}";
  char sub_handler[payload.length() + 1];
  payload.toCharArray(sub_handler, payload.length() + 1);
  GreenPLClient.publish(TOPIC_CHAR, sub_handler);
  delay(10);
}
boolean mqttConnect()
{
  Serial.print("Connecting to ");
  Serial.print(greenplServer);
  // Проходим аутентификацию в GreenPL MQTT:
  boolean status = GreenPLClient.connect(CLIENT_ID, TOKEN, "1");
  if (status == false) {
    Serial.println(" fail");
    return false;
  }
  Serial.println(" OK");
  //Подписываемся после возникновения стабильного соединения с сервером
  char sub_handler[100];

  (TOPIC + "/Relay_mgbot_mgbot_relay_1_16/lv").toCharArray(sub_handler, 100);
  GreenPLClient.subscribe(sub_handler);;
  (TOPIC + "/Relay_mgbot_mgbot_relay_2_17/lv").toCharArray(sub_handler, 100);
  GreenPLClient.subscribe(sub_handler);
  (TOPIC + "/MATRIX/lv").toCharArray(sub_handler, 100);
  GreenPLClient.subscribe(sub_handler);
  (TOPIC + "/SERV/lv").toCharArray(sub_handler, 100);
  GreenPLClient.subscribe(sub_handler);
  (TOPIC + "/red/lv").toCharArray(sub_handler, 100);
  GreenPLClient.subscribe(sub_handler);
  (TOPIC + "/green/lv").toCharArray(sub_handler, 100);
  GreenPLClient.subscribe(sub_handler);
  (TOPIC + "/blue/lv").toCharArray(sub_handler, 100);
  GreenPLClient.subscribe(sub_handler);
  return GreenPLClient.connected();
 

}
void mqttCallback(char* topic, byte* payload, unsigned int len)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();
  String variable = getVariableFromTopic(String(topic), '/', 3);
  char payload_char[100];
  for (int i = 0; i < len; i++) {
    payload_char[i] = (char)payload[i];
  }
  int value = atoi(payload_char);
  if (variable == "Relay_mgbot_mgbot_relay_1_16") {
    if (value == 1 || value == 0) {
      digitalWrite(Relay_mgbot_mgbot_relay_1_16, value);
    }
  }
  if (variable == "Relay_mgbot_mgbot_relay_2_17") {
    if (value == 1 || value == 0) {
      digitalWrite(Relay_mgbot_mgbot_relay_2_17, value);
    }
  }
  if (variable == "MATRIX") {
    FastLED.setBrightness(value);          // заполнить всю матрицу желтым цветом
    FastLED.show();
  }
   if (variable == "red") {
    r=value;
   fill_solid( leds, NUM_LEDS, CRGB(r, g, b)); 
   FastLED.show();
   }
    if (variable == "green") {
      g=value;
   fill_solid( leds, NUM_LEDS, CRGB(r,g, b)); 
   FastLED.show();
   }
    if (variable == "blue") {
      b=value;
   fill_solid( leds, NUM_LEDS, CRGB(r, g, b)); 
   FastLED.show();
   }
  if (variable == "SERV") {
    if (prevangle < value) {
      for (pos = prevangle; pos <= value; pos += 1)
      {
        myservo.write(pos);
        delay(15);                                        // если угол задан больше предыдущего, то доводим до нужного угла в ++ // if the current angle>previous angle then going clockwise
      }
      prevangle = value;
    }
    else if (prevangle > value) {
      for (pos = prevangle; pos >= value; pos -= 1)
      {
        myservo.write(pos);
        delay(15);                                       // если угол задан меньше предыдущего, то доводим до нужного угла в -- // if the current angle<previous angle then going counter-clockwise
      }
      prevangle = value;
    }
  }
}
//функция для быстрого разделения строки (топика) по символу '/'
String getVariableFromTopic(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
