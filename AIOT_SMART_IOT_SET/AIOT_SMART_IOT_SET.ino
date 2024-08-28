#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "nRF_SSD1306Wire.h"
#include <PCF8574.h>

#define KEY_S1 23  // Button S1
#define KEY_S2 19  // Button S2

#define PINOUT1 27
#define PINOUT2 21
#define PINOUT3 14

#define O1 17
#define O2 26
#define IN1 35
#define IN2 34
#define IN3 37
#define IN4 38

#define WIFI_LED 2  // Wifi LED
#define IOT_LED 12

#define SOUND_PIN 33
#define LIGHT_PIN 36

#define motor_AIN1 15
#define motor_AIN2 18

#define motor_BIN1 17
#define motor_BIN2 26

#define LED_BUILTIN 0
#define NUMPIXELS 6
#define DELAYVAL 200

float lightSensor = 0;
bool Val_S1 = 0;
bool Val_S2 = 0;
bool inputState1 = 0, inputState2 = 0, inputState3 = 0, inputState4 = 0;
int sw1 = 0, sw2 = 0, sw3 = 0, sw4 = 0;
char msg[200];
Adafruit_NeoPixel pixels(NUMPIXELS, LED_BUILTIN, NEO_GRB + NEO_KHZ800);  // On-board 6 LEDs
PCF8574 pcf20(0x20);
SSD1306Wire display(0x3C, 4, 5);  //OLED display

/////////////////////////////////////////////// ตั้งค่า WiFi ที่นี่ //////////////////////////////////////////////////////////
const char *ssid = "";     //WiFi network name
const char *password = "";  //WiFi network password

////////////////////////////////////////////////////////// ตั้งค่า NETPIE ที่นี่ ////////////////////////////////////////////////////////////////////
const char *mqttServer = "mqtt.netpie.io";
const int mqtt_port = 1883;
const char *mqtt_Client = "";    //get from NETPIE website
const char *mqtt_username = "";  //get from NETPIE website
const char *mqtt_password = "";  //get from NETPIE website

WiFiClient espClient;
PubSubClient client(espClient);

const byte PcfButtonLedPin0 = 0;
const byte PcfButtonLedPin1 = 1;
const byte PcfButtonLedPin2 = 2;
const byte PcfButtonLedPin3 = 3;
const byte PcfButtonLedPin6 = 6;
const byte PcfButtonLedPin7 = 7;

void mqttReconnect() {
  while (!client.connected()) {
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
      client.subscribe("@msg/#");
    } else
      delay(5000);
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }
  Serial.println(message);
  if (String(topic) == "@msg/LEDONBOARD") {
    Serial.println("LED checking");
    if (message == "Val_S1_ON") {
      Val_S1 = 0;
      onboardLED(0);
    } else if (message == "Val_S1_OFF") {
      Val_S1 = 1;
      onboardLED(1);
    }
    String data = "{\"data\":{\"Val_S1\":" + String(Val_S1) + "}}";
    updateShadow(data);
  } else if (String(topic) == "@msg/LED_ROOM1") {
    if (message == "LED1_ON") {
      inputState2 = (inputState2 + 1) % 2;
      pcf20.write(PcfButtonLedPin6, HIGH);
    } else if (message == "LED1_OFF") {
      inputState2 = (inputState2 + 1) % 2;
      pcf20.write(PcfButtonLedPin6, LOW);
    }
    String data = "{\"data\":{\"Light1\":" + String(inputState2) + "}}";
    updateShadow(data);
  } else if (String(topic) == "@msg/LED_ROOM2") {
    if (message == "LED2_ON") {
      inputState3 = (inputState3 + 1) % 2;
      pcf20.write(PcfButtonLedPin7, HIGH);
    } else if (message == "LED2_OFF") {
      inputState3 = (inputState3 + 1) % 2;
      pcf20.write(PcfButtonLedPin7, LOW);
    }
    String data = "{\"data\":{\"Light2\":" + String(inputState3) + "}}";
    updateShadow(data);
  } else if (String(topic) == "@msg/MOTOR1") {
    if (message == "M_ON") {
      inputState1 = 1;
      digitalWrite(O1, HIGH);
    } else if (message == "M_OFF") {
      inputState1 = 0;
      digitalWrite(O1, LOW);
    }
    String data = "{\"data\":{\"Motor1\":" + String(inputState1) + "}}";
    updateShadow(data);
  } else if (String(topic) == "@msg/MOTOR2") {
    if (message == "M_ON") {
      inputState4 = 1;
      digitalWrite(O2, HIGH);
    } else if (message == "M_OFF") {
      inputState4 = 0;
      digitalWrite(O2, LOW);
    }
    String data = "{\"data\":{\"Motor2\":" + String(inputState4) + "}}";
    updateShadow(data);
  } else {
    // Do something.
  }
}

void updateShadow(String updateData) {
  Serial.print("Update Shadow: ");
  Serial.println(updateData);
  updateData.toCharArray(msg, (updateData.length() + 1));
  client.publish("@shadow/data/update", msg);
}


void onboardLED(int val) {
  if (val == 0) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(150, 0, 0));
      pixels.show();
      delay(DELAYVAL);
    }
  } else {
    pixels.clear();
    pixels.show();
  }
}

int getLightSensor() {
  lightSensor = analogRead(LIGHT_PIN);
  return map(lightSensor, 0, 4095, 100, 0);  // map the value from 0, 4096 to 0, 100
}

void setup() {
  Serial.begin(115200);
  pinMode(WIFI_LED, OUTPUT);
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(WIFI_LED, LOW);
    delay(500);
  }
  digitalWrite(WIFI_LED, HIGH);
  // Setup pinMode
  pinMode(IOT_LED, OUTPUT);
  pinMode(KEY_S1, INPUT_PULLUP);
  pinMode(KEY_S2, INPUT_PULLUP);
  pinMode(LIGHT_PIN, INPUT);
  pinMode(PINOUT1, OUTPUT);
  pinMode(PINOUT2, OUTPUT);
  pinMode(PINOUT3, OUTPUT);
  pinMode(O1, OUTPUT);
  pinMode(O2, OUTPUT);

  Wire.begin(4, 5);
  pcf20.begin();
  pixels.begin();  //set up 6 on-board LEDs

  //set up OLED display
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  pcf20.write(PcfButtonLedPin6, LOW);  // light 1
  pcf20.write(PcfButtonLedPin7, LOW);  //Light 2

  client.setServer(mqttServer, mqtt_port);
  client.setCallback(callback);
}
void loop() {
  if (!client.connected()) {  // Re-connect to netpie
    digitalWrite(IOT_LED, LOW);
    mqttReconnect();
  } else digitalWrite(IOT_LED, HIGH);

  // Button S1,S2
  Val_S1 = digitalRead(KEY_S1);
  Val_S2 = digitalRead(KEY_S2);
  Serial.println("Val_S1 = " + String(Val_S1));
  Serial.println("Val_S2 = " + String(Val_S2));

  int light = getLightSensor();
  //Serial.println("LIGHT SENSOR: " + String(light));

  if (Val_S1 == 0 || Val_S2 == 0) {
    String data = "{\"data\":{\"Val_S1\":" + String(Val_S1) + ",\"Val_S2\":" + String(Val_S2) + ",\"Light_Sensor\":" + String(light) + "}}";
    updateShadow(data);
  }

  if (Val_S1 == 0) {
    //LED on
    onboardLED(0);
  }
  if (Val_S2 == 0) {
    //LED off
    onboardLED(1);
  }

  int in1 = !pcf20.readButton(PcfButtonLedPin0);
  int in2 = !pcf20.readButton(PcfButtonLedPin1);
  int in3 = !pcf20.readButton(PcfButtonLedPin2);
  int in4 = !pcf20.readButton(PcfButtonLedPin3);

  if (in1 == 1) {  //fan1
    delay(1000);
    inputState1 = (inputState1 + 1) % 2;
    (inputState1 == 1) ? digitalWrite(O1, HIGH) : digitalWrite(O1, LOW);
    String data = "{\"data\":{\"Motor1\":" + String(inputState1) + "}}";
    updateShadow(data);
    delay(500);
  } else if (in2 == 1) {  //light1
    delay(1000);
    inputState2 = (inputState2 + 1) % 2;
    (inputState2 == 1) ? pcf20.write(PcfButtonLedPin6, HIGH) : pcf20.write(PcfButtonLedPin6, LOW);
    String data = "{\"data\":{\"Light1\":" + String(inputState2) + "}}";
    updateShadow(data);
    delay(500);

  } else if (in3 == 1) {  //light2
    delay(1000);
    inputState3 = (inputState3 + 1) % 2;
    (inputState3 == 1) ? pcf20.write(PcfButtonLedPin7, HIGH) : pcf20.write(PcfButtonLedPin7, LOW);
    String data = "{\"data\":{\"Light2\":" + String(inputState3) + "}}";
    updateShadow(data);
    delay(500);

  } else if (in4 == 1) {  //fan2
    delay(1000);
    inputState4 = (inputState4 + 1) % 2;
    (inputState4 == 1) ? digitalWrite(O2, HIGH) : digitalWrite(O2, LOW);
    String data = "{\"data\":{\"Motor2\":" + String(inputState4) + "}}";
    updateShadow(data);
    delay(500);
  }


  //display
  display.clear();
  display.drawString(0, 0, String("HELLO AIOT BOARD"));  // line 1
  //display.drawString(0, 9, String("HELLO AIOT BOARD"));  // line 2
  String line3 = "Fan 1: " + String(inputState1 ? "On" : "Off");
  display.drawString(0, 18, line3);  // line 3
  String line4 = "Fan 2: " + String(inputState4 ? "On" : "Off");
  display.drawString(0, 27, line4);  // line 4
  String line5 = "Light 1: " + String(inputState2 ? "On" : "Off");
  display.drawString(0, 36, line5);  // line 5
  String line6 = "Light 2: " + String(inputState3 ? "On" : "Off");
  display.drawString(0, 45, line6);  // line 6
  //display.drawString(0, 54, String("HELLO AIOT BOARD"));  // line 7
  display.display();

  client.loop();
  Serial.println("MEM left = " + String(esp_get_free_heap_size()));
  delay(500);
}
