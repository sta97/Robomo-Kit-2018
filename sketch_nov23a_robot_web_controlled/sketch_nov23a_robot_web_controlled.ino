#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

//Distance between wheels in cm
#define WHEEL_DIST 8

enum pins
{
  LED = 16,
  LEFT_FORWARD = 5,
  LEFT_BACKWARD = 4,
  RIGHT_FORWARD = 0,
  RIGHT_BACKWARD = 2
};

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
  driveMotors(server.arg("x").toFloat(), server.arg("z").toFloat());
  server.send(200, "text/plain", server.arg("x") + " " + server.arg("z"));  
}

void setup()
{
  Serial.begin(115200);

  pinMode(LEFT_FORWARD, OUTPUT);
  pinMode(LEFT_BACKWARD, OUTPUT);
  pinMode(RIGHT_FORWARD, OUTPUT);
  pinMode(RIGHT_BACKWARD, OUTPUT);
  
  analogWrite(LEFT_FORWARD, 0);
  analogWrite(LEFT_BACKWARD, 0);
  analogWrite(RIGHT_FORWARD, 0);
  analogWrite(RIGHT_BACKWARD, 0);  
  
  WiFiManager wifiManager;
  wifiManager.autoConnect();

  server.on("/", motorHandler);
  server.begin();
}

void loop() {
  server.handleClient();    //Handling of incoming requests
}
