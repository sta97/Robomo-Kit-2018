#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

enum pins
{
  LED = 16,
  LEFT_FORWARD = 5,
  LEFT_BACKWARD = 4,
  RIGHT_FORWARD = 0,
  RIGHT_BACKWARD = 2
};

const int PORT = 8080;
WiFiUDP net;

int led_value = 1000;
int led_change = 5;

unsigned long last_packet_time;
unsigned long led_update_time;

const int BUFF_LEN = 100;
signed char buff[BUFF_LEN];

void setup()
{
  pinMode(LED, OUTPUT);
  pinMode(LEFT_FORWARD, OUTPUT);
  pinMode(LEFT_BACKWARD, OUTPUT);
  pinMode(RIGHT_FORWARD, OUTPUT);
  pinMode(RIGHT_BACKWARD, OUTPUT);
  WiFi.begin("wifi_network_name", "wifi_network_password");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
  }

  net.begin(PORT);
  last_packet_time = millis();

  Serial.begin(115200);
}

void loop() {
  if (net.parsePacket() >= 2) {
    last_packet_time = millis();
    net.read((unsigned char*)buff, BUFF_LEN);
    const int left = buff[0] * 8;
    const int right = buff[1] * 8;
    Serial.printf("left: %i - right: %i\n", left, right);
    //If requested motor speed is above ~20%, move at requested speed and direction, otherwise stop.
    if (left > 200) {
      analogWrite(LEFT_FORWARD, left < 1023 ? left : 1023);
      analogWrite(LEFT_BACKWARD, 0);
    } else if (left < -200) {
      analogWrite(LEFT_FORWARD, 0);
      analogWrite(LEFT_BACKWARD, left > -1023 ? -left : 1023);
    } else {
      analogWrite(LEFT_FORWARD, 0);
      analogWrite(LEFT_BACKWARD, 0);
    }
    if (right > 200) {
      analogWrite(RIGHT_FORWARD, right < 1023 ? right : 1023);
      analogWrite(RIGHT_BACKWARD, 0);
    } else if (right < -200) {
      analogWrite(RIGHT_FORWARD, 0);
      analogWrite(RIGHT_BACKWARD, right > -1023 ? -right : 1023);
    } else {
      analogWrite(RIGHT_FORWARD, 0);
      analogWrite(RIGHT_BACKWARD, 0);
    }
  } else {
    //Turn off motors if no packet for over 0.5s
    if (millis() - last_packet_time > 500) {
      analogWrite(LEFT_FORWARD, 0);
      analogWrite(LEFT_BACKWARD, 0);
      analogWrite(RIGHT_FORWARD, 0);
      analogWrite(RIGHT_BACKWARD, 0);
    }
  }
  //update led
  analogWrite(LED, led_value);
  if (millis() - led_update_time > 50) {
    led_value += led_change;
    led_update_time = millis();
    if (led_value <= 900 || led_value >= 1023) {
      led_change = -led_change;
    }
  }
}
