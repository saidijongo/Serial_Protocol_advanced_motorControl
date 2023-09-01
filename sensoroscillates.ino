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
bool isClockwise = true;

int currentAngle = 0;
int totalStepsMoved = 0; // Variable to track total steps moved

enum class Command { NONE, INITIALIZE, MOVE, STATUS };

Command parseCommand(const String &commandStr) {
  if (commandStr == "I") return Command::INITIALIZE;
  if (commandStr.startsWith("M,")) return Command::MOVE;
  if (commandStr == "S") return Command::STATUS;
  return Command::NONE;
}

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
    switch (isClockwise) {
      case true:
        if (digitalRead(LEFT_SENSOR_PIN) == LOW) {
          stopMotor();
          delay(1000); // Stop for 1 second
          rotateMotor(8, false); // Rotate 8 degrees CCW
          delay(1000); // Stop for 1 second
          return;
        }
        break;

      case false:
        if (digitalRead(RIGHT_SENSOR_PIN) == LOW) {
          stopMotor();
          delay(1000); // Stop for 1 second
          rotateMotor(8, true); // Rotate 8 degrees CW
          delay(1000); // Stop for 1 second
          return;
        }
        break;
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
  rotateMotor(8, false); // Rotate 8 degrees CW
  delay(1000);

  while (digitalRead(LEFT_SENSOR_PIN) == HIGH) {
    rotateMotor(1, false); // Rotate CW until left sensor is interrupted
  }
  rotateMotor(8, true); // Rotate 8 degrees CCW

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
    String commandStr = Serial.readStringUntil('\n');
    commandStr.trim();
    Command command = parseCommand(commandStr);

    if (!resetInProgress && !moveInProgress) {
      switch (command) {
        case Command::INITIALIZE:
          resetInProgress = true;
          motorStopped = false;
          initializeMotor();
          resetInProgress = false;
          motorStopped = true;
          break;

        case Command::MOVE: {
          int commaPos = commandStr.indexOf(",");
          if (commaPos != -1) {
            int angle = commandStr.substring(commaPos + 1).toInt();
            moveMotor(angle, 80); // Assuming speedPercent is 80
          }
          break;
        }

        case Command::STATUS:
          status();
          break;

        default:
          break;
      }
    }
  }
}
