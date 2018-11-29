#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <WiFiUdp.h>              //Used for direct UDP control

//Pins for DRV8833 motor driver
enum pins
{
  LEFT_FORWARD = 5,
  LEFT_BACKWARD = 4,
  RIGHT_FORWARD = 0,
  RIGHT_BACKWARD = 2
};

const int PORT = 8080;
WiFiUDP net;
const int BUFF_LEN = 100;
signed char buff[BUFF_LEN];
unsigned long last_packet_time;

ESP8266WebServer server(80);   //Web server object. Will be listening in port 80 (default for HTTP)

/*
 * Convert speed to analog singnals
 */
void driveMotor(int forwardPin, int backwardPin, int motorSpeed){
  if(motorSpeed > 0){
    analogWrite(forwardPin, motorSpeed);
    analogWrite(backwardPin, 0);
  } else {
    analogWrite(forwardPin, 0);
    analogWrite(backwardPin, -motorSpeed);
  }
}

/*
 * x - Forward speed 
 * z - Angular speed
 */
void driveMotors(float x, float z){
  float speed_right = z / 2 + x;
  float speed_left = x * 2 - speed_right;
  driveMotor(LEFT_FORWARD, LEFT_BACKWARD, speed_left * 1024);
  driveMotor(RIGHT_FORWARD, RIGHT_BACKWARD, speed_right * 1024);
}

void motorHandler(){
  //Use server x and z to drive motors - http://esp8266_ip_address/?x=1.0&z=-1.0
  driveMotors(server.arg("x").toFloat(), server.arg("z").toFloat());
  //Just echo back inputs and send a 200 OK
  server.send(200, "text/plain", server.arg("x") + " " + server.arg("z"));  
}

void setup()
{
  Serial.begin(115200);
  //Setup up pins for motor controller
  pinMode(LEFT_FORWARD, OUTPUT);
  pinMode(LEFT_BACKWARD, OUTPUT);
  pinMode(RIGHT_FORWARD, OUTPUT);
  pinMode(RIGHT_BACKWARD, OUTPUT);
  //Make sure motors are stopped
  driveMotors(0,0);

  //Setup wifi
  WiFiManager wifiManager;
  wifiManager.autoConnect();

  //Listen for web api calls
  server.on("/", motorHandler);
  server.begin();

  //Listen for UDP controll
  net.begin(PORT);
  last_packet_time = millis();
}

void loop() {
  //Listen on port 80 for web api
  server.handleClient();
  //List on port 8080 for UPD controll
  handleUdp();
}

void handleUdp(){
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
    }
  
}
