#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <math.h>

// Define your two control pins
#define MOTOR1_PWM 32 // Must be a PWM pin (~ on the Arduino)
#define MOTOR1_REV 33 // Can be any digital pin
#define MOTOR2_PWM 25
#define MOTOR2_REV 26
#define MOTOR3_PWM 27
#define MOTOR3_REV 14

void snapJoystick(float &right, float &forward)
{
  const float SNAP_THRESHOLD = 0.12;

  // Mostly moving right or left
  if (fabs(right) > 0.5 && fabs(forward) < SNAP_THRESHOLD) {
    forward = 0.0;
  }

  // Mostly moving forward or backward
  if (fabs(forward) > 0.5 && fabs(right) < SNAP_THRESHOLD) {
    right = 0.0;
  }
}

// -------------------------
// Webpage
// -------------------------
WebServer server(80);

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Robot Joystick</title>

  <style>
    body {
      margin: 0;
      background: #202124;
      color: white;
      font-family: Arial, sans-serif;
      text-align: center;
      touch-action: none;
    }

    h1 {
      margin-top: 25px;
    }

    #status {
      margin: 15px;
      color: #ff5555;
    }

    canvas {
      background: #333;
      border: 3px solid white;
      border-radius: 50%;
      touch-action: none;
    }
  </style>
</head>

<body>

  <h1>Robot Joystick</h1>
  <div id="status">Connecting...</div>

  <canvas id="joystick" width="300" height="300"></canvas>

  <script>
  const socket = new WebSocket(`ws://${location.hostname}:82`);

    const status = document.getElementById("status");
    const canvas = document.getElementById("joystick");
    const ctx = canvas.getContext("2d");

    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const maxDistance = 100;
    const knobRadius = 35;

    let right = 0.0;
    let forward = 0.0;
    let joystickActive = false;

    socket.onopen = function() {
      status.textContent = "Connected";
      status.style.color = "#55ff55";
    };

    socket.onclose = function() {
      status.textContent = "Disconnected";
      status.style.color = "#ff5555";
    };

    socket.onerror = function() {
      status.textContent = "Connection error";
      status.style.color = "#ff5555";
    };

    function drawJoystick() {
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      // Outer joystick area
      ctx.beginPath();
      ctx.arc(centerX, centerY, maxDistance, 0, Math.PI * 2);
      ctx.strokeStyle = "white";
      ctx.lineWidth = 3;
      ctx.stroke();

      // Position of knob
      const knobX = centerX + right * maxDistance;
      const knobY = centerY - forward * maxDistance;

      // Knob
      ctx.beginPath();
      ctx.arc(knobX, knobY, knobRadius, 0, Math.PI * 2);
      ctx.fillStyle = "#4285f4";
      ctx.fill();
    }

    function updateJoystick(event) {
      const rect = canvas.getBoundingClientRect();

      let mouseX = event.clientX - rect.left;
      let mouseY = event.clientY - rect.top;

      let dx = mouseX - centerX;
      let dy = mouseY - centerY;

      const distance = Math.sqrt(dx * dx + dy * dy);

      if (distance > maxDistance) {
        dx = dx / distance * maxDistance;
        dy = dy / distance * maxDistance;
      }

      // Horizontal movement: right is positive
      right = dx / maxDistance;

      // Screen Y is reversed: up means forward
      forward = -dy / maxDistance;

      drawJoystick();
    }

    function stopJoystick() {
      joystickActive = false;
      right = 0.0;
      forward = 0.0;
      drawJoystick();
    }

    canvas.addEventListener("pointerdown", function(event) {
      joystickActive = true;
      canvas.setPointerCapture(event.pointerId);
      updateJoystick(event);
    });

    canvas.addEventListener("pointermove", function(event) {
      if (joystickActive) {
        updateJoystick(event);
      }
    });

    canvas.addEventListener("pointerup", stopJoystick);
    canvas.addEventListener("pointercancel", stopJoystick);

    // Send right,forward every 50 milliseconds
    setInterval(function() {
      if (socket.readyState === WebSocket.OPEN) {
        socket.send(`${right.toFixed(3)},${forward.toFixed(3)}`);
      }
    }, 50);

    drawJoystick();
  </script>

</body>
</html>

)rawliteral";

// -------------------------
// Wi-Fi access point
// -------------------------

const char* robotSSID = "Omni-Robot";
const int wifiChannel = 1;

// Use the same port as your working project
WebSocketsServer webSocket = WebSocketsServer(82);

void webSocketEvent(
  uint8_t client,
  WStype_t type,
  uint8_t* payload,
  size_t length
);


// Only one controller at a time
int activeClient = -1;

unsigned long lastCommandTime = 0;
const unsigned long COMMAND_TIMEOUT = 300;

// -------------------------
// Drive Functions
// -------------------------
void driveInward(int pwm, int rev, int speed)
{
  digitalWrite(rev, LOW);
  analogWrite(pwm, speed);
}

void driveOutward(int pwm, int rev, int speed)
{
  int speed_rev = 255-speed;
  digitalWrite(rev, HIGH);
  analogWrite(pwm, speed_rev);
}

void stop(int pwm, int rev)
{
  analogWrite(pwm, 0);
  digitalWrite(rev, LOW);
}

void setMotor(int pwm, int rev, int command)
{
  command = constrain(command, -255, 255);
  if (command > 0) {
    driveInward(pwm, rev, command);
  }
  else if (command < 0) {
    driveOutward(pwm, rev, -command);
  }
  else {
    stop(pwm, rev);
  }
}


void driveRobot(float forward, float right)
{
  float motor1 = -forward;
  float motor2 =  0.5 * forward - 0.8660254 * right;
  float motor3 =  0.5 * forward + 0.8660254 * right;

  // Keep all commands within -1.0 to 1.0
  float maximum = max(abs(motor1), max(abs(motor2), abs(motor3)));

  if (maximum > 1.0) {
    motor1 /= maximum;
    motor2 /= maximum;
    motor3 /= maximum;
  }

  setMotor(MOTOR1_PWM, MOTOR1_REV, motor1 * 255);
  setMotor(MOTOR2_PWM, MOTOR2_REV, motor2 * 255);
  setMotor(MOTOR3_PWM, MOTOR3_REV, motor3 * 255);
}


void setup()
{
  Serial.begin(115200);

  pinMode(MOTOR1_PWM, OUTPUT);
  pinMode(MOTOR1_REV, OUTPUT);

  pinMode(MOTOR2_PWM, OUTPUT);
  pinMode(MOTOR2_REV, OUTPUT);

  pinMode(MOTOR3_PWM, OUTPUT);
  pinMode(MOTOR3_REV, OUTPUT);

  // // ESP32 Core 3.x PWM setup
  // ledcAttach(MOTOR1_PWM, 1000, 8);
  // ledcAttach(MOTOR2_PWM, 1000, 8);
  // ledcAttach(MOTOR3_PWM, 1000, 8);

  driveRobot(0.0, 0.0);

  // Create an open Wi-Fi network
  WiFi.mode(WIFI_AP);
  WiFi.softAP(robotSSID, NULL, wifiChannel);

  Serial.println();
  Serial.println("Robot Wi-Fi started");
  Serial.print("Network name: ");
  Serial.println(robotSSID);
  Serial.print("Robot IP address: ");
  Serial.println(WiFi.softAPIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/", HTTP_GET, []() {
  server.send_P(200, "text/html", INDEX_HTML);
  });

  server.begin();

  lastCommandTime = millis();
}

void webSocketEvent(
  uint8_t client,
  WStype_t type,
  uint8_t* payload,
  size_t length
) {
  if (type == WStype_CONNECTED) {
    if (activeClient == -1) {
      activeClient = client;
      webSocket.sendTXT(client, "CONNECTED");
    }
    else {
      webSocket.sendTXT(
        client,
        "ERROR: Robot is already being controlled"
      );

      webSocket.disconnect(client);
    }

    return;
  }

  if (type == WStype_DISCONNECTED) {
    if (client == activeClient) {
      activeClient = -1;
      driveRobot(0.0, 0.0);
    }

    return;
  }

  if (type == WStype_TEXT) {
    // Ignore commands from non-controller clients
    if (client != activeClient) {
      return;
    }

    char message[40];

    size_t copyLength = length;

    if (copyLength >= sizeof(message)) {
      copyLength = sizeof(message) - 1;
    }

    memcpy(message, payload, copyLength);
    message[copyLength] = '\0';

    float right;
    float forward;

    // Expected format: right,forward
    // Example: 0.5,0.8
    if (sscanf(message, "%f,%f", &right, &forward) == 2) {
      right = constrain(right, -1.0, 1.0);
      forward = constrain(forward, -1.0, 1.0);
      snapJoystick(right, forward);
      Serial.print("Right: ");
      Serial.println(right);
      Serial.print("Forward: ");
      Serial.println(forward);
      driveRobot(forward, right);
      lastCommandTime = millis();
    }
  }
}

void loop()
{
  webSocket.loop();
  server.handleClient();

  // Stop if the joystick stops sending commands
  if (millis() - lastCommandTime > COMMAND_TIMEOUT) {
    driveRobot(0.0, 0.0);
  }
}