#include <Servo.h> 
//Servo variables
Servo elbowServo;
Servo ankleServo;

int elbowServoPin = 3;
int elbowServoPos = 90;

int ankleServoPin = 4;
int ankleServoPos = 90;

int elbowServoRate = 1;
int ankleServoRate = 2;
unsigned long servoMoveInterval = 2;
unsigned long lastServoMoveTime = 0;

//Joystick variables 
int xAxisPin = 0;
int yAxisPin = 1;
int zAxisPin = 2;

int Y_HOME = 501;
int ANALOG_TOLARANCE = 100;
float xVal, yVal;
int zVal;

enum ElbowServoPos {
  NEUTRAL = 90,
  READY_PUSH = 105,
  ON_GROUND = 162
};

enum AnkleServoPos {
  MIN = 0,
  START_PUSH = 15,
  STRAIGHT = 135
};

enum WiggleStates {
  RETRACTED,
  START_WIGGLE,
  END_WIGGLE
};

WiggleStates state = RETRACTED;
float currentWiggleStrideLength;
int currentElbowTarget;
int currentAnkleTarget;

int absoluteVal(int x) {
    return (x < 0)? -x: x;
}

class ServoPosTransition {
  public:
    int targetPos;
    int rate;
    int initialPos;
    float strideLength;
    bool initialized = false;

    bool ordered = false;
    bool done = false;
    unsigned long lastServoMoveTime = 0;

    ServoPosTransition(int targetPos, int rate) {
      this -> targetPos = targetPos;
      this -> rate = rate;
      this -> initialPos = 0;
      this -> strideLength = 1;
      this -> initialized = true;
    }
    
    ServoPosTransition(int targetPos, int rate, int initialPos) {
      this -> targetPos = targetPos;
      this -> rate = rate;
      this -> initialPos = initialPos;
      this -> strideLength = 1;
    }

    ServoPosTransition(int targetPos, int rate, int initialPos, float strideLength) {
      this -> strideLength = strideLength;
      this -> rate = rate * absoluteVal(strideLength);
      if (strideLength > 0) {
        this -> targetPos = targetPos > initialPos? absoluteVal(targetPos - initialPos) * absoluteVal(strideLength) + initialPos :absoluteVal(absoluteVal(targetPos - initialPos) * absoluteVal(strideLength) - initialPos);
        this -> initialPos = initialPos;
      } else {
        this -> targetPos = initialPos > targetPos? absoluteVal(initialPos - targetPos) * absoluteVal(strideLength) + targetPos :absoluteVal(absoluteVal(initialPos - targetPos) * absoluteVal(strideLength) - targetPos);
        this -> initialPos = targetPos;
      }
    }

    int run(int servoPos) {
      if (millis() - lastServoMoveTime >= rate) {
        lastServoMoveTime = millis();
        if (initialized == false) {
          initialized = true;
          return initialPos;
        }

        if(targetPos < servoPos) {
          return servoPos -= 1;
        } 
        if(targetPos > servoPos) {
          return servoPos += 1;
        }
        if(targetPos == servoPos) {
          done = true;
          return servoPos;
        }
      ordered = true;
      } else {
        return servoPos;
      }
    }
};

ServoPosTransition elbowPosTrans(NEUTRAL, 50);
ServoPosTransition anklePosTrans(MIN, 50);

//Telemetry variables
unsigned long lastTelemetryTime = 0;
unsigned long telemetryInterval = 100;

void setup() {
  elbowServo.attach(elbowServoPin);
  ankleServo.attach(ankleServoPin);
  pinMode(zAxisPin, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  yVal = readY();
  // yVal = 1;
  if (elbowPosTrans.done && yVal != 0) {
    switch (state) {
    case WiggleStates::RETRACTED:
      currentWiggleStrideLength = yVal;
      state = WiggleStates::START_WIGGLE;
      elbowPosTrans = ServoPosTransition(READY_PUSH, 25, NEUTRAL);//1000ms
      anklePosTrans = ServoPosTransition(START_PUSH, 100, MIN);//1000ms
      break;
    
    case WiggleStates::START_WIGGLE:
      state = WiggleStates::END_WIGGLE;
      elbowPosTrans = ServoPosTransition(ON_GROUND, 40, READY_PUSH, currentWiggleStrideLength);//1900ms
      anklePosTrans = ServoPosTransition(STRAIGHT, 20, START_PUSH, currentWiggleStrideLength);//1900ms
      currentElbowTarget = elbowPosTrans.targetPos;
      currentAnkleTarget = anklePosTrans.targetPos;
      break;

    case WiggleStates::END_WIGGLE:
      state = WiggleStates::RETRACTED;
      elbowPosTrans = ServoPosTransition(NEUTRAL, 25, currentElbowTarget);//1800ms
      anklePosTrans = ServoPosTransition(MIN, 15, currentAnkleTarget);//1800ms
    default:
      state = WiggleStates::RETRACTED;
      elbowPosTrans = ServoPosTransition(NEUTRAL, 25);
      anklePosTrans = ServoPosTransition(MIN, 20);
      break;
    }
  }
  if (yVal == 0) {
    state = WiggleStates::RETRACTED;
      elbowPosTrans = ServoPosTransition(NEUTRAL, 25);
      anklePosTrans = ServoPosTransition(MIN, 20);
  }
  elbowServoPos = elbowPosTrans.run(elbowServoPos);
  ankleServoPos = anklePosTrans.run(ankleServoPos);
  elbowServo.write(elbowServoPos);
  ankleServo.write(ankleServoPos);

  // if (millis() - lastStepTime >= oneStepInterval) {
  //   lastStepTime = millis();

  //   Serial.print("elbow Servo Pos: ");
  //   Serial.print(elbowServoPos);

  //   Serial.print(" ankle Servo Pos: ");
  //   Serial.println(ankleServoPos);
  // }
  telemetry();
}

float readY() {
  int joystickY = analogRead(yAxisPin);
  if (absoluteVal(joystickY - Y_HOME) >= ANALOG_TOLARANCE) {
      return (float)(joystickY - Y_HOME) / Y_HOME;
  } else {
      return 0;
  }
}

void telemetry() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastTelemetryTime >= telemetryInterval) {
        lastTelemetryTime = currentMillis;
        Serial.print("elbow target: ");
        Serial.print(elbowPosTrans.targetPos);
//        Serial.print(elbowServoPos);
        Serial.print(" ankle target: ");
        Serial.print(anklePosTrans.targetPos);
//        Serial.print(ankleServoPos);
        Serial.print(" yVal: ");
        Serial.println(yVal);
    }
}
// int readAnkleY(){ 
//   int receivedInt;
//   if (Serial.available()) {
//     receivedInt = Serial.parseInt();
//     if (receivedInt < 180 || receivedInt > 0) {
//       Serial.print("ankle received: ");
//       Serial.println(receivedInt);
//       return receivedInt;
//     } else {return ankleServoPos;};
//   } else {
//     return ankleServoPos;
//   }
// }
