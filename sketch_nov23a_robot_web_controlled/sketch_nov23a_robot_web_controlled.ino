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

ESP8266WebServer server(80);   //Web server object. Will be listening in port 80 (default for HTTP)

const char webPage[] = R"=====(
<!DOCTYPE html>
<html>
<head>
<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>
<script>
$(document).ready(function(){
    $("button").click(function(){
        $.get("/drive?x=" + x + "&z=" + z, function(data, status){});
    });
});
</script>
</head>
<body>

<button>Send an HTTP GET request to a page and get the result back</button>
<div id="zone_joystick" style="background-color: green;"></div>
<div id="output">Hello</div>
</body>
<script src="https://cdnjs.cloudflare.com/ajax/libs/nipplejs/0.6.8/nipplejs.min.js"></script>
<script type="text/javascript">
    var options = {
        zone: document.getElementById('zone_joystick'),
        color: 'blue',
        mode: 'static',
        position: {left: '50%', bottom: '50%'}
    };
    var nipple = nipplejs.create(options);
    nipple.on('move', function (evt, data) {
        //Move
        //output.innerHTML = "<pre>" + JSON.stringify(data.instance.options.size, null, 4) + "</pre>"
        z = data.instance.frontPosition.x / (data.instance.options.size / 2)
        x = -data.instance.frontPosition.y / (data.instance.options.size / 2)
        output.innerHTML = "<pre>X: " + x + "  Z: " + z + "</pre>"
        $.get("/drive?x=" + x + "&z=" + z, function(data, status){});
    }); 
    nipple.on('end', function (evt, data) {
        //Stop
        output.innerHTML = "<pre>Stop</pre>"
        x = 0
        z = 0
        $.get("/drive?x=" + x + "&z=" + z, function(data, status){});
    });
    
</script>
</html>
)=====";

/*
 * Convert speed to analog signals
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
  float turnRate = z * 0.25;
  float speed_right = x - turnRate;
  float speed_left = x + turnRate;
  driveMotor(LEFT_FORWARD, LEFT_BACKWARD, speed_left * 1024);
  driveMotor(RIGHT_FORWARD, RIGHT_BACKWARD, speed_right * 1024);
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
  server.on("/", []() { server.send ( 200, "text/html", webPage );  });
  server.on("/drive", motorHandler);
  server.begin();
}

void loop() {
  //Listen on port 80 for web api
  server.handleClient();
}

