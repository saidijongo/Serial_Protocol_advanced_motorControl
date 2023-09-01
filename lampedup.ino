#include <Arduino.h>

const int CW_PIN = 5;
const int CCW_PIN = 6;
const int RELAY_PIN = 8;
const int RIGHT_SENSOR_PIN = 11;
const int LEFT_SENSOR_PIN = 10;

const float STEP_ANGLE = 0.72;
int STEP_DELAY = 1000; // Initial step delay value

enum class MotorState { STOPPED, MOVING_CW, MOVING_CCW };

MotorState motorState = MotorState::STOPPED;

void setup() {
  pinMode(CW_PIN, OUTPUT);
  pinMode(CCW_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LEFT_SENSOR_PIN, INPUT_PULLUP);
  Serial.begin(115200);
}

void rotateMotor(bool isClockwise, int steps) {
  digitalWrite(isClockwise ? CCW_PIN : CW_PIN, HIGH);
  delayMicroseconds(STEP_DELAY);
  digitalWrite(isClockwise ? CCW_PIN : CW_PIN, LOW);
  delayMicroseconds(STEP_DELAY);
}

void stopMotor() {
  digitalWrite(CW_PIN, LOW);
  digitalWrite(CCW_PIN, LOW);
}

void moveMotor(int targetAngle, int speedPercent) {
  int speed = map(speedPercent, 0, 100, 5000, 1000);
  STEP_DELAY = speed; // Update STEP_DELAY

  int currentAngle = 0;
  int targetSteps = abs(targetAngle) / STEP_ANGLE;
  bool isClockwise = targetAngle >= 0;

  motorState = isClockwise ? MotorState::MOVING_CW : MotorState::MOVING_CCW;

  while (currentAngle < targetSteps) {
    if (isClockwise) {
      if (digitalRead(LEFT_SENSOR_PIN) == LOW) {
        stopMotor();
        delay(1000); // Stop for 1 second
        rotateMotor(false, 8); // Rotate 8 degrees CCW
        delay(1000); // Stop for 1 second
        return;
      }
    } else {
      if (digitalRead(RIGHT_SENSOR_PIN) == LOW) {
        stopMotor();
        delay(1000); // Stop for 1 second
        rotateMotor(true, 8); // Rotate 8 degrees CW
        delay(1000); // Stop for 1 second
        return;
      }
    }

    rotateMotor(isClockwise, 1); // Rotate 1 step
    currentAngle++;
  }

  motorState = MotorState::STOPPED;
}

void initializeMotor() {
  Serial.println("ST,0,INIT,0,0,ED");

  while (digitalRead(RIGHT_SENSOR_PIN) == HIGH) {
    rotateMotor(true, 1); // Rotate CCW until right sensor is interrupted
  }
  delay(1000); // Delay for stability
  rotateMotor(false, 8); // Rotate 8 degrees CW
  delay(1000);

  while (digitalRead(LEFT_SENSOR_PIN) == HIGH) {
    rotateMotor(false, 1); // Rotate CW until left sensor is interrupted
  }
  delay(1000);

  rotateMotor(true, 8); // Rotate 8 degrees CCW

  Serial.println("ST,0,RETINIT,0,0,ED");
  stopMotor();
}

void status() {
  String status;
  int angle = 0;

  switch (motorState) {
    case MotorState::STOPPED:
      status = "Stopped";
      break;
    case MotorState::MOVING_CW:
      status = "Moving CW";
      break;
    case MotorState::MOVING_CCW:
      status = "Moving CCW";
      break;
  }

  Serial.print("ST,");
  Serial.print(status);
  Serial.print(",");
  Serial.print(angle);
  Serial.print(",");
  Serial.print(STEP_DELAY);
  Serial.println(",ED");
}

void ionizerL() {
  if (Serial.available() > 0) {
    String commandStr = Serial.readStringUntil('\n');
    commandStr.trim();

    if (commandStr.startsWith("L,")) {
      String lampState = commandStr.substring(2); // Remove the "L," prefix
      if (lampState == "1") {
        digitalWrite(RELAY_PIN, HIGH);
      } else {
        digitalWrite(RELAY_PIN, LOW);
      }

      Serial.print("ST,0,RETLAMP,");
      Serial.print(lampState);
      Serial.println(",ED");
    }
  }
}

void loop() {
  if (Serial.available() > 0) {
    String commandStr = Serial.readStringUntil('\n');
    commandStr.trim();

    if (commandStr == "I") {
      initializeMotor();
    } else if (commandStr.startsWith("M,")) {
      if (motorState == MotorState::STOPPED) {
        int commaPos = commandStr.indexOf(",");
        if (commaPos != -1) {
          int angle = commandStr.substring(commaPos + 1).toInt();
          moveMotor(angle, 65);
        }
      }
    } else if (commandStr == "S") {
      status();
    } else if (commandStr.startsWith("L,")) {
      ionizerL();
    }
  }
}
