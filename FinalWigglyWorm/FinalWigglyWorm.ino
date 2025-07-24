#include <Servo.h> 

//Basic functions
float absoluteVal(float x) {return (x < 0)? (float)-x: (float)x;}
float getPositivity(float x) {return (x < 0)? (float)-1.0: (float)1.0;}
float range(float x, float lowerLimit, float higherLimit) {
  if (x > higherLimit) {
    x = higherLimit;
  } 
  if (x < lowerLimit) {
    x = lowerLimit;
  }
  return x;
}
//Stepper motor control variables 
int outPorts[] = {11, 10, 9, 8};
int stepsToDo = 0;
unsigned long lastStepTime = 0;

const unsigned long oneStepInterval = 4;
int ticksFromOrigin = 0;
int STEP_TOLARANCE = 2;

int STEPPER_MOTOR_RANGE = 90;
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
      this -> rate = rate * 0.6;
      this -> initialPos = 0;
      this -> strideLength = 1;
      this -> initialized = true;
    }
    
    ServoPosTransition(int targetPos, int rate, int initialPos) {
      this -> targetPos = targetPos;
      this -> rate = rate * 0.6;
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

//Joystick variables 
int xAxisPin = 0;
int yAxisPin = 1;
// int zAxisPin = 2;

int X_HOME = 516;
int Y_HOME = 501;
int ANALOG_TOLARANCE = 100;
float xVal, yVal;
// int zVal;

void setup() {
  for (int i = 0; i < 4; i++) {
        pinMode(outPorts[i], OUTPUT);
    }

  elbowServo.attach(elbowServoPin);
  ankleServo.attach(ankleServoPin);
  //pinMode(zAxisPin, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  xVal = readX();
  stepsToDo = joyStickXFollower();
  stepperMotorController();

  yVal = readY();
  runServoArm();

  telemetry();
}

int joyStickXFollower() {
    // int receivedInt;
    // if (Serial.available()) {
    //     receivedInt = Serial.parseInt();
    //     Serial.print("received: ");
    //     Serial.print(receivedInt);
    //     return receivedInt;
    // } else {
    //     return stepsToDo;
    // }
    if (xVal != 0) {
        int stepDifference = X_HOME * xVal/STEPPER_MOTOR_RANGE - (ticksFromOrigin * -1);
        return (absoluteVal(stepDifference) >= STEP_TOLARANCE)? stepDifference: 0;
    } else {
        return ticksFromOrigin;
    }
};

void stepperMotorController() {
    if (millis() - lastStepTime >= oneStepInterval) {
        lastStepTime = millis();
        if (stepsToDo != 0) {
            moveOneStep(stepsToDo > 0);
            if (stepsToDo < 0) {
                stepsToDo++;
                ticksFromOrigin++;
            }
            if (stepsToDo > 0) {
                stepsToDo--;
                ticksFromOrigin--;
            }
        } else {
            for (int i = 0; i < 4; i++) {
                digitalWrite(outPorts[i], LOW);
            }
        }
    }
}

void moveOneStep(bool dir) {
    static byte out = 0x01;
    if (dir) {
        out != 0x08? out = out << 1 : out = 0x01;
    } else {
        out != 0x01? out = out >> 1 : out = 0x08;
    }
    for (int i = 0; i < 4; i++) {
        digitalWrite(outPorts[i], (out & (0x01 << i)) ? HIGH : LOW);
    }
}

void runServoArm() {
    if (elbowPosTrans.done && yVal != 0) {
    switch (state) {
    case WiggleStates::RETRACTED:
      currentWiggleStrideLength = yVal;
      state = WiggleStates::START_WIGGLE;
      if (currentWiggleStrideLength > 0) {
        elbowPosTrans = ServoPosTransition(READY_PUSH, 25, NEUTRAL);//1000ms
        anklePosTrans = ServoPosTransition(START_PUSH, 100, MIN);//1000ms
      } else {
        elbowPosTrans = ServoPosTransition(ON_GROUND + 5, 45, NEUTRAL);//1000ms
        anklePosTrans = ServoPosTransition(STRAIGHT, 5, MIN);//1000ms
      }
      break;
    
    case WiggleStates::START_WIGGLE:
      state = WiggleStates::END_WIGGLE;
      if (currentWiggleStrideLength > 0) {
        elbowPosTrans = ServoPosTransition(ON_GROUND, 40, READY_PUSH, absoluteVal(currentWiggleStrideLength));//1900ms
        anklePosTrans = ServoPosTransition(STRAIGHT, 20, START_PUSH, absoluteVal(currentWiggleStrideLength));//1900ms
      } else {
        elbowPosTrans = ServoPosTransition(READY_PUSH, 45, ON_GROUND + 5, absoluteVal(currentWiggleStrideLength));//1000ms
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
}

float readX() {
    int joystickX = analogRead(xAxisPin);
    if (absoluteVal(joystickX - X_HOME) >= ANALOG_TOLARANCE) {
        return (float)(joystickX - X_HOME) / X_HOME * 90;
    } else {
        return 0;
    }
}

float readY() {
  int joystickY = analogRead(yAxisPin);
  if (absoluteVal(joystickY - Y_HOME) >= ANALOG_TOLARANCE) {
      float realY = (float)(joystickY - Y_HOME) / Y_HOME * -1;
      realY = range(realY, -1, 1);
      float processedY = getPositivity(realY) * (realY * realY);
      return realY;
  } else {
      return 0;
  }
}

void telemetry() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastTelemetryTime >= telemetryInterval) {
        // lastTelemetryTime = currentMillis;
        // Serial.print("steps to do: ");
        // Serial.print(stepsToDo);
        // Serial.print(" difference ");
        // Serial.print(512 * xVal/90.0 - (ticksFromOrigin * -1));
        // Serial.print(" ticksFromOrigin: ");
        // Serial.println(ticksFromOrigin);
        Serial.print("xVal ");
        Serial.print(xVal);
        Serial.print(" yVal: ");
        Serial.println(yVal);
    }
}