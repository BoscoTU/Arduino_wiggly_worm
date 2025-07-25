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
  NEUTRAL = 60,
  READY_PUSH = 105,
  ON_GROUND = 165
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

float absoluteVal(float x) {
    return (x < 0)? (float)-x: (float)x;
}

float getPositivity(float x) {
  return (x < 0)? (float)-1.0: (float)1.0;
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
      this -> rate = rate * 0.6;
      if (strideLength > 0) {
        this -> targetPos = targetPos > initialPos? (float)absoluteVal(targetPos - initialPos) * (float)absoluteVal(strideLength) + (float)initialPos :(float)absoluteVal((float)absoluteVal(targetPos - initialPos) * (float)absoluteVal(strideLength) - (float)initialPos);
        this -> initialPos = initialPos;
      } else {
        this -> targetPos = initialPos > targetPos? (float)absoluteVal(initialPos - targetPos) * (float)absoluteVal(strideLength) + (float)targetPos :(float)absoluteVal((float)absoluteVal(initialPos - targetPos) * (float)absoluteVal(strideLength) - (float)targetPos);
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
  //pinMode(zAxisPin, INPUT_PULLUP);
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
      if (currentWiggleStrideLength > 0) {
        elbowPosTrans = ServoPosTransition(READY_PUSH, 25, NEUTRAL);//1000ms
        anklePosTrans = ServoPosTransition(START_PUSH, 100, MIN);//1000ms
      } else {
        elbowPosTrans = ServoPosTransition(ON_GROUND + 5, 20, NEUTRAL);//1000ms
        anklePosTrans = ServoPosTransition(STRAIGHT, 10, MIN);//1000ms
      }
      break;
    
    case WiggleStates::START_WIGGLE:
      state = WiggleStates::END_WIGGLE;
      if (currentWiggleStrideLength > 0) {
        elbowPosTrans = ServoPosTransition(ON_GROUND, 40, READY_PUSH, absoluteVal(currentWiggleStrideLength));//1900ms
        anklePosTrans = ServoPosTransition(STRAIGHT, 20, START_PUSH, absoluteVal(currentWiggleStrideLength));//1900ms
      } else {
        elbowPosTrans = ServoPosTransition(READY_PUSH, 40, ON_GROUND + 5, absoluteVal(currentWiggleStrideLength));//1000ms
        anklePosTrans = ServoPosTransition(START_PUSH, 20, STRAIGHT, absoluteVal(currentWiggleStrideLength));//1000ms
      }
      currentElbowTarget = elbowPosTrans.targetPos;
      currentAnkleTarget = anklePosTrans.targetPos;
      break;

    case WiggleStates::END_WIGGLE:
      state = WiggleStates::RETRACTED;
      elbowPosTrans = ServoPosTransition(NEUTRAL, 25, currentElbowTarget, getPositivity(currentWiggleStrideLength));//1800ms
      anklePosTrans = ServoPosTransition(MIN, 15, currentAnkleTarget, getPositivity(currentWiggleStrideLength));//1800ms
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
