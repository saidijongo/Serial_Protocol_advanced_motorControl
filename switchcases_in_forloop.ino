#include <Arduino.h>

const int CW_PIN = 5;
const int CCW_PIN = 6;
const int RIGHT_SENSOR_PIN = 11;
const int LEFT_SENSOR_PIN = 10;

const float STEP_ANGLE = 0.72;
int STEP_DELAY = 1000; // Initial step delay value

bool motorStopped = true;
bool resetInProgress = false;
bool moveInProgress = false;
bool leftSensorInterrupted = false;
bool rightSensorInterrupted = false;
bool isClockwise = true; // Assuming initial direction is clockwise

int currentAngle = 0;
int totalStepsMoved = 0; // Variable to track total steps moved

void setup() {
  pinMode(CW_PIN, OUTPUT);
  pinMode(CCW_PIN, OUTPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LEFT_SENSOR_PIN, INPUT_PULLUP);
  Serial.begin(115200);
}
void stopMotor() {
  digitalWrite(CW_PIN, LOW);
  digitalWrite(CCW_PIN, LOW);
  motorStopped = true;
  Serial.println("Motor Stopped");
}

void rotateMotor(int angle, bool isClockwise = true) {
  int steps = angle / STEP_ANGLE;
  for (int i = 0; i < steps; i++) {
    if ((isClockwise && digitalRead(LEFT_SENSOR_PIN) == LOW) || (!isClockwise && digitalRead(RIGHT_SENSOR_PIN) == LOW)) {
      stopMotor();

      switch (isClockwise) {
        case true:
          if (resetInProgress) {
            rotateMotor(8, false); // Rotate 8 degrees CCW
          } else if (leftSensorInterrupted) {
            rotateMotor(8, false); // Rotate 8 degrees CCW
          }
          break;
        case false:
          if (resetInProgress) {
            rotateMotor(8, true); // Rotate 8 degrees CW
          } else if (rightSensorInterrupted) {
            rotateMotor(8, true); // Rotate 8 degrees CW
          }
          break;
      }
    
      delay(1000); // Stop for 1 second

      return;
    }

    totalStepsMoved++;
    currentAngle = isClockwise ? totalStepsMoved : -totalStepsMoved;

    digitalWrite(isClockwise ? CCW_PIN : CW_PIN, HIGH);
    delayMicroseconds(STEP_DELAY);
    digitalWrite(isClockwise ? CCW_PIN : CW_PIN, LOW);
    delayMicroseconds(STEP_DELAY);
  }
}

void initializeMotor() {
  Serial.println("ST,0,INIT,0,0,ED");

  while (digitalRead(RIGHT_SENSOR_PIN) == HIGH) {
    rotateMotor(1, true); // Rotate CCW until right sensor is interrupted
  }
  delay(1000); // Delay for stability

  if (digitalRead(RIGHT_SENSOR_PIN) == LOW) {
    rotateMotor(8, false); // Rotate 8 degrees CW
    delay(1000);
  }

  while (digitalRead(LEFT_SENSOR_PIN) == HIGH) {
    rotateMotor(1, false); // Rotate CW until left sensor is interrupted
  }

  if (digitalRead(LEFT_SENSOR_PIN) == LOW) {
    rotateMotor(8, true); // Rotate 8 degrees CCW
    Serial.println("Home Base");
  }

  Serial.println("ST,0,RETINIT,0,0,ED");
}



void moveMotor(int angle, int speedPercent) {
  int speed = map(speedPercent, 0, 100, 5000, 1000);
  STEP_DELAY = speed; // Update STEP_DELAY

  Serial.print("ST,0,MOVE,0,");
  Serial.print(angle);
  Serial.print(",");
  Serial.print(speed);
  Serial.println(",ED");

  int targetSteps = abs(angle) / STEP_ANGLE;
  rotateMotor(targetSteps, angle >= 0);
}

void status() {
  String status = motorStopped ? "Stopped" : "Moving";
  int angle = currentAngle;
  int speed = STEP_DELAY;

  Serial.print("ST,");
  Serial.print(status);
  Serial.print(",");
  Serial.print(angle);
  Serial.print(",");
  Serial.print(speed);
  Serial.println(",ED");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (!resetInProgress && !moveInProgress) {
      if (command == "I") {
        resetInProgress = true;
        motorStopped = false;
        initializeMotor();
        resetInProgress = false;
        motorStopped = true;
      } else if (command == "S") {
        status();
      } else {
        int angle = command.toInt();
        bool clockwise = angle <= 0;
        moveMotor(angle, 80); // Assuming speedPercent is 50
      }
    }
  }
}
