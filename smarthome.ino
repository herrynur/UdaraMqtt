#include "DHT.h"
#include <GyverOLED.h>
#include <WiFi.h>
#include <PubSubClient.h>
GyverOLED<SSH1106_128x64> oled;

#define DHTPIN 2
#define DHTTYPE DHT21   // AM2301  
DHT dht(DHTPIN, DHTTYPE);

const int buzzer =  33;
int buzzerState = LOW;
unsigned long previousMillis = 0;
const long interval = 1000;

int smokeA0 = 39;
int data_real = 0;
// Your threshold value
int sensorThres = 160;
int analogSensor = 0;
int a, b = 0;

/*IoT
Setting pass dan ssid wifi disini
*****/
const char *ssid = "Asus_X01BDA";
const char *password = "heri1234567";

// mqtt
const char *mqtt_server = "broker.hivemq.com";
#define MQTT_USERNAME ""
#define MQTT_KEY ""

//Setting id disini, id disini harus sama dengan yang di Android
String id = "kamar2";
//id utama, jangan dirubah!
String idUtama = "SmartGR/";
String topic;
String topicR1;
String topicR2;

//wifi client
WiFiClient espClient;
PubSubClient client(espClient);
void setup_wifi();
void reconnect();
void callback(char *topic, byte *payload, unsigned int length);

//data
float h;
float t;

//data dummy
//float t = 25.5;
//int h = 50;
//int co = 350;
String status = "Aman";
String data;

//Delay data pengiriman ke mqtt
unsigned long lastMsg = 0;
const long intervalMsg = 1000;

void setup() {
  //setup iot
  pinMode(2, OUTPUT);
  topic = idUtama + id;
  topicR1 = topic + "/B1";
  topicR2 = topic + "/B2";
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.begin(115200);
  oled.init();
  dht.begin();
  pinMode(buzzer, OUTPUT);
  pinMode(smokeA0, INPUT);
  oled.update();
  delay(2);
  oled.clear();
  oled.setScale(1);
  oled.setCursorXY(0, 5);
  oled.print(" Bissmillah !!!!!!");
  delay(1000);
  oled.update();
  oled.clear();
}

void loop() {
  data_real = analogRead(smokeA0);
  if (data_real < 60) {
    analogSensor = analogRead(smokeA0);
  }
  else if (data_real > 60) {
    analogSensor = (analogRead(smokeA0) * 0.6) + analogRead(smokeA0);
  }
  oled.setCursorXY(0, 3);
  //if (analogSensor <=60){analogSensor = 60;}
  oled.print("CO : " + String(analogSensor));
  oled.print("     ");
  oled.setCursorXY(80, 3);
  oled.print("PPM    ");
  oled.update();
  // Checks if it has reached the threshold value
  if (analogSensor > sensorThres)
  {
    oled.setCursorXY(0, 54);
    oled.print("Udara Tidak Aman");
    oled.update();
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      if (buzzerState == LOW) {
        buzzerState = HIGH;
      }
      else {
        buzzerState = LOW;
      }
    }
  }
  else
  {
    oled.setCursorXY(0, 54);
    oled.print("Udara Aman      ");
    oled.update();
    buzzerState = LOW;
    Serial.println("BUZZ OFF");
  }
  digitalWrite(buzzer, buzzerState);
  delay(100);
  a++;
  if (a <= 10) {
    baca_temp();
    a = 0;
  }
  //oled.clear();

  //upload and setting mqtt
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  //upload ke mqtt
  unsigned long currentT = millis();
  if (currentT - lastMsg >= intervalMsg) {
    lastMsg = currentT;

    data = String(analogSensor) + "," + String(t) + "," + String(h) + "," + status;

    Serial.print("Publish message: ");
    Serial.println(data);
    client.publish((char*) topic.c_str(), (char*) data.c_str());
  }
}

void baca_temp()
{
  h = dht.readHumidity();
  t = dht.readTemperature();
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h))
  {
    oled.setCursorXY(0, 18);
    oled.print("Failed to read from DHT");

  }
  else
  {
    oled.setCursorXY(0, 18);
    oled.print("Hum: ");
    oled.print(h);
    oled.print("  ");
    oled.setCursorXY(80, 18);
    oled.print("%");
    oled.setCursorXY(0, 34);
    oled.print("Temp: ");
    oled.print(t);
    oled.print("  ");
    oled.setCursorXY(80, 34);
    oled.print("*C");
    oled.update();
  }
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//Reconnect saat putus
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      client.subscribe((char*) topicR1.c_str(), 1);
      client.subscribe((char*) topicR2.c_str(), 1);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//Program utama saat menerima data dari mqtt
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Setting untuk Relay1 dan Relay2
  //Relay 1
  if (strcmp(topic, (char*) topicR1.c_str()) == 0)
  {
    if ((char)payload[0] == '1') {
      digitalWrite(2, LOW);
    }
    else
    {
      digitalWrite(2, HIGH);
    }
  }
  //Relay2
  else if (strcmp(topic, (char*) topicR2.c_str()) == 0)
  {
    if ((char)payload[0] == '1') {
      digitalWrite(2, LOW);
    }
    else
    {
      digitalWrite(2, HIGH);
    }
  }
}
