#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiUdp.h>              //Used for direct UDP control
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

//Pins for TB6612 motor driver
enum pins
{
  AIN_1 = 0,
  AIN_2 = 2,
  APWM = 12,
  BIN_1 = 4,
  BIN_2 = 5,
  BPWM = 14,
  STANDBY = 13
};

ESP8266WebServer server(80);   //Web server object. Will be listening in port 80 (default for HTTP)

const char webPage[] = R"=====(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <title>robot remote control</title>
    <style>
        #stick_box {
            width: 90vmin;
            height: 90vmin;
            border-width: thick;
            border: solid black;
        }
        
        #thumbstick {
            width: 20vmin;
            height: 20vmin;
            background-color: red;
        }
    </style>
    <script defer>
        var robotX = 0.0;
        var robotY = 0.0;
        setInterval(sendToRobot, 50);

        function f(event) {
            if (event.type == "touchmove")
                event = event.touches[0];

            var x = event.clientX;
            var y = event.clientY;

            var stick = document.getElementById("thumbstick");
            var box = document.getElementById("stick_box");

            var offset = stick.clientWidth / 2;

            var boxSize = box.getBoundingClientRect();

            var leftLim = boxSize.left + offset;
            var rightLim = boxSize.right - offset;
            var topLim = boxSize.top + offset;
            var bottomLim = boxSize.bottom - offset;

            if (x < leftLim)
                x = leftLim;

            if (x > rightLim)
                x = rightLim;

            if (y < topLim)
                y = topLim;

            if (y > bottomLim)
                y = bottomLim;

            stick.style.left = x - offset + "px";
            stick.style.top = y - offset + "px";
            stick.style.position = "absolute";

            var centerX = (boxSize.left + boxSize.right) / 2;
            var centerY = (boxSize.top + boxSize.bottom) / 2;

            var joyX;
            var joyY;

            if (x > centerX)
                joyX = percent(x, rightLim, centerX);
            else
                joyX = -percent(x, leftLim, centerX);

            if (y > centerY)
                joyY = -percent(y, bottomLim, centerY);
            else
                joyY = percent(y, topLim, centerY);

            robotX = joyX;
            robotY = joyY;
        }

        function percent(x, hi, low) {
            return (x - low) / (hi - low);
        }

        function sendToRobot() {
            var getRequest = "http://" + window.location.hostname + "/drive?x=" + robotX + "&z=" + robotY;
            var debug = document.getElementById("debug");
            if (debug != null)
                debug.innerHTML = getRequest;
            else
                console.log("debug is null!");
            var robot = new XMLHttpRequest()
            robot.open("GET", getRequest);
            robot.send();
        }

        function center() {
            var stick = document.getElementById("thumbstick");
            var box = document.getElementById("stick_box");

            var offset = stick.clientWidth / 2;

            var boxSize = box.getBoundingClientRect();

            var x = (boxSize.left + boxSize.right) / 2;
            var y = (boxSize.top + boxSize.bottom) / 2;

            stick.style.left = x - offset + "px";
            stick.style.top = y - offset + "px";
            stick.style.position = "absolute";

            robotX = 0.0;
            robotY = 0.0;
        }
    </script>
</head>

<body>
    <div id="stick_box" onmousemove="f(event)" onmouseleave="center()" ontouchmove="f(event)" ontouchend="center()">
        <div id="thumbstick"></div>
    </div>
    <p id="debug">no data</p>
</body>

</html>
)=====";

/*
 * Convert speed to analog signals
 */
void driveMotor(int pwmPin, int forwardPin, int backwardPin, int motorSpeed){
  //Set direction
  if(motorSpeed > 0){
    digitalWrite(forwardPin, HIGH);
    digitalWrite(backwardPin, LOW);
  } else {
    digitalWrite(forwardPin, LOW);
    digitalWrite(backwardPin, HIGH);
  }
  //Set speed
  analogWrite(pwmPin, motorSpeed);
}

/*
 * x - Forward speed 
 * z - Angular speed
 */
void driveMotors(float x, float z){
  analogWrite(LED_BUILTIN, x*-1024);
  Serial.println(x);
  
  float turnRate = z * 0.25;
  float speed_right = x + turnRate;
  float speed_left = x - turnRate;
  driveMotor(APWM, AIN_1, AIN_2, speed_left * 1024);
  driveMotor(BPWM, BIN_1, BIN_2, speed_right * 1024);
}

void motorHandler(){
  //Use server x and z to drive motors - http://esp8266_ip_address/?x=1.0&z=-1.0
  driveMotors(server.arg("x").toFloat(), server.arg("z").toFloat());
  //Just echo back inputs and send a 200 OK
  server.send(200, "text/plain", server.arg("x") + " " + server.arg("z"));  
}

void rootPageHandler(){
  server.send(200, "text/plain", webPage);
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(9600);
  delay(100);
  //Setup up pins for motor controller
  pinMode(AIN_1, OUTPUT);
  pinMode(AIN_2, OUTPUT);
  pinMode(BIN_1, OUTPUT);
  pinMode(BIN_2, OUTPUT);
  pinMode(APWM, OUTPUT);
  pinMode(BPWM, OUTPUT);
  pinMode(STANDBY, OUTPUT);
  digitalWrite(STANDBY, HIGH);
  //Make sure motors are stopped
  driveMotors(0.0,0.0);

  WiFiManager wifiManager;
  wifiManager.autoConnect();

  //Listen for web api calls
  server.on("/", []() { server.send ( 200, "text/html", webPage );  });
  server.on("/drive", motorHandler);
  server.begin();
}

void loop() {
  //Listen on port 80 for web api
  server.handleClient();
}
