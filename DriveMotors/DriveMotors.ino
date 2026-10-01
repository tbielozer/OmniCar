// Define your two control pins
#define MOTOR1_PWM 32 // Must be a PWM pin (~ on the Arduino)
#define MOTOR1_REV 33 // Can be any digital pin
#define MOTOR2_PWM 25
#define MOTOR2_REV 26
#define MOTOR3_PWM 27
#define MOTOR3_REV 14


void setup() {
  pinMode(MOTOR1_PWM, OUTPUT);
  pinMode(MOTOR1_REV, OUTPUT);
  pinMode(MOTOR2_PWM, OUTPUT);
  pinMode(MOTOR2_REV, OUTPUT);
  pinMode(MOTOR3_PWM, OUTPUT);
  pinMode(MOTOR3_REV, OUTPUT);
  Serial.begin(9600); 
}

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


// int loop_test = 130;
void loop() {
//   if (loop_test < 255)
//   {
//     loop_test = loop_test + 5;
//   }
//   else
//   {
//     loop_test=130;
//   }
//   Serial.println(loop_test);

  // // 1. Move Forward at roughly 70% speed (180 out of 255)
  // driveInward(MOTOR1_PWM, MOTOR1_REV, 180);
  // driveInward(MOTOR2_PWM, MOTOR2_REV, 180);
  // driveInward(MOTOR3_PWM, MOTOR3_REV, 180);
  // delay(3000);

  // // // Stop the motor
  // stop(MOTOR1_PWM, MOTOR1_REV);
  // stop(MOTOR2_PWM, MOTOR2_REV);
  // stop(MOTOR3_PWM, MOTOR3_REV);
  // delay(1000);

  // // 2. Move Backward at roughly 70% speed 
  // // Since MOTOR1_REV is HIGH, we invert the PWM value: 255 - 180 = 75
  // driveOutward(MOTOR1_PWM, MOTOR1_REV, 180); 
  // driveOutward(MOTOR2_PWM, MOTOR2_REV, 180); 
  // driveOutward(MOTOR3_PWM, MOTOR3_REV, 180);
  // delay(3000);

  // // // Stop the motor
  // stop(MOTOR1_PWM, MOTOR1_REV);
  // stop(MOTOR2_PWM, MOTOR2_REV);
  // stop(MOTOR3_PWM, MOTOR3_REV);
  // delay(1000);
  // delay(1000);
  driveRobot(0.5, 0.5);
  delay(3000);
  driveRobot(0,0);
  delay(1000);
}