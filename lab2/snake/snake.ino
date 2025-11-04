/* EECE.4520 Microprocessors II & Embedded Systems
Project: Lab 2: Game Control with Joy Stick, Gyro and Accelerometer
Kimson Lam and Nathan Hoang

References:
https://projecthub.arduino.cc/hibit/using-joystick-module-with-arduino-0ffdd4
https://projecthub.arduino.cc/Nicholas_N/how-to-use-the-accelerometer-gyroscope-gy-521-647e65
*/
#include <Wire.h>

//Joystick pins
#define ANALOG_X_PIN A3
#define ANALOG_Y_PIN A4
#define ANALOG_BUTTON_PIN A2

#define ANALOG_X_CORRECTION 128
#define ANALOG_Y_CORRECTION 128

struct Analog {
  short x, y;
} analog;

//Buzzer Pin
const int buzzerPin = 13;

//Gyroscope constants
const int MPU = 0x68;
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;

// Direction enum
enum Direction { UP, DOWN, LEFT, RIGHT };

void setup() {
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  pinMode(ANALOG_BUTTON_PIN, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  // Read gyro sensor
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 12, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();

  // Read joystick
  analog.x = analogRead(ANALOG_X_PIN) / 8 - ANALOG_X_CORRECTION;
  analog.y = analogRead(ANALOG_Y_PIN) / 8 - ANALOG_Y_CORRECTION;

  Direction dir = getDominantDirection();

  // Send direction to Python
  switch (dir) {
    case UP:    Serial.println('w'); break;
    case DOWN:  Serial.println('s'); break;
    case LEFT:  Serial.println('a'); break;
    case RIGHT: Serial.println('d'); break;
  }

  // Check if Python sends score event
  if (Serial.available() > 0) {
    char incomingByte = Serial.read();
    if (incomingByte == 'S') {
      digitalWrite(buzzerPin, HIGH);
      delay(20);
      digitalWrite(buzzerPin, LOW);
    }
  }

  delay(30);
}

Direction getDominantDirection() {
  const int DEADZONE = 40;
  if (abs(analog.x) > DEADZONE || abs(analog.y) > DEADZONE) {
    if (abs(analog.x) > abs(analog.y)) {
      return (analog.x > 0) ? RIGHT : LEFT;
    } else {
      return (analog.y > 0) ? UP : DOWN;
    }
  }

  const int ACCEL_THRESHOLD = 4000;
  if (abs(AcX) > ACCEL_THRESHOLD || abs(AcY) > ACCEL_THRESHOLD) {
    if (abs(AcX) > abs(AcY)) {
      return (AcX > 0) ? RIGHT : LEFT;
    } else {
      return (AcY > 0) ? UP : DOWN;
    }
  }

  return UP;
}