#define MOTOR1_PWM 32
#define MOTOR1_REV 33
#define MOTOR2_PWM 25
#define MOTOR2_REV 26
#define MOTOR3_PWM 19
#define MOTOR3_REV 18

#define ENC1_A 35
#define ENC1_B 34
#define ENC2_A 14
#define ENC2_B 27
#define ENC3_A 17
#define ENC3_B 16

volatile long encoder1Count = 0;
volatile long encoder2Count = 0;
volatile long encoder3Count = 0;

void IRAM_ATTR encoder1ISR()
{
  bool a = digitalRead(ENC1_A);
  bool b = digitalRead(ENC1_B);
  encoder1Count += (a == b) ? 1 : -1;
}

void IRAM_ATTR encoder2ISR()
{
  bool a = digitalRead(ENC2_A);
  bool b = digitalRead(ENC2_B);
  encoder2Count += (a == b) ? 1 : -1;
}

void IRAM_ATTR encoder3ISR()
{
  bool a = digitalRead(ENC3_A);
  bool b = digitalRead(ENC3_B);
  encoder3Count += (a == b) ? 1 : -1;
}

void setMotor(int pwm, int rev, int command)
{
  command = constrain(command, -255, 255);

  if (command >= 0) {
    digitalWrite(rev, LOW);
    analogWrite(pwm, command);
  } else {
    digitalWrite(rev, HIGH);
    analogWrite(pwm, 255 - abs(command));
  }
}

void stopAllMotors()
{
  setMotor(MOTOR1_PWM, MOTOR1_REV, 0);
  setMotor(MOTOR2_PWM, MOTOR2_REV, 0);
  setMotor(MOTOR3_PWM, MOTOR3_REV, 0);
}

void resetEncoderCounts()
{
  noInterrupts();
  encoder1Count = 0;
  encoder2Count = 0;
  encoder3Count = 0;
  interrupts();
}

void readEncoderCounts(long &c1, long &c2, long &c3)
{
  noInterrupts();
  c1 = encoder1Count;
  c2 = encoder2Count;
  c3 = encoder3Count;
  interrupts();
}

void runTopSpeedTest()
{
  Serial.println("Starting motors...");

  // Run all motors at full speed in the positive direction
  setMotor(MOTOR1_PWM, MOTOR1_REV, 255);
  setMotor(MOTOR2_PWM, MOTOR2_REV, 255);
  setMotor(MOTOR3_PWM, MOTOR3_REV, 255);

  // Allow motors to reach steady speed
  delay(1000);

  resetEncoderCounts();

  unsigned long startTime = millis();
  const unsigned long testTime = 2000;

  while (millis() - startTime < testTime) {
    delay(1);
  }

  unsigned long elapsed = millis() - startTime;

  stopAllMotors();

  long c1, c2, c3;
  readEncoderCounts(c1, c2, c3);

  float seconds = elapsed / 1000.0;

  Serial.println();
  Serial.println("Top-speed results:");

  Serial.print("Motor 1: ");
  Serial.print(abs(c1));
  Serial.print(" counts, ");
  Serial.print(abs(c1) / seconds);
  Serial.println(" counts/second");

  Serial.print("Motor 2: ");
  Serial.print(abs(c2));
  Serial.print(" counts, ");
  Serial.print(abs(c2) / seconds);
  Serial.println(" counts/second");

  Serial.print("Motor 3: ");
  Serial.print(abs(c3));
  Serial.print(" counts, ");
  Serial.print(abs(c3) / seconds);
  Serial.println(" counts/second");
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

  pinMode(ENC1_A, INPUT);
  pinMode(ENC1_B, INPUT);
  pinMode(ENC2_A, INPUT);
  pinMode(ENC2_B, INPUT);
  pinMode(ENC3_A, INPUT);
  pinMode(ENC3_B, INPUT);

  attachInterrupt(digitalPinToInterrupt(ENC1_A), encoder1ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC2_A), encoder2ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC3_A), encoder3ISR, CHANGE);

  stopAllMotors();

  delay(2000);
  runTopSpeedTest();
}

void loop()
{
  // Test runs once in setup()
}