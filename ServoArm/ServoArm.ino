#include <Servo.h> 
unsigned long lastStepTime;
unsigned long oneStepInterval = 250;

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

int Y_HOME = 500;
int ANALOG_TOLARANCE = 50;
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
int currentWiggleStride;

int absoluteVal(int x) {
    return (x < 0)? -x: x;
}

class ServoPosTransition {
  public:
    int targetPos;
    int rate;
    int initialPos;
    int strideLength;
    bool ordered = false;
    bool done = false;
    bool initialized = false;

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

    ServoPosTransition(int targetPos, int rate, int initialPos, int strideLength) {
      this -> rate = rate;
      if (strideLength > 0) {
        this -> targetPos = targetPos * absoluteVal(strideLength);
        this -> initialPos = initialPos;
      } else {
        this -> targetPos = initialPos * absoluteVal(strideLength);
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

void setup() {
  elbowServo.attach(elbowServoPin);
  ankleServo.attach(ankleServoPin);
  pinMode(zAxisPin, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  yVal = readY();
  if (elbowPosTrans.done) {
    switch (state) {
    case WiggleStates::RETRACTED:
      currentWiggleStride = yVal;
      state = WiggleStates::START_WIGGLE;
      elbowPosTrans = ServoPosTransition(READY_PUSH, 25, NEUTRAL);//1000ms
      anklePosTrans = ServoPosTransition(START_PUSH, 100, MIN);//1000ms
      break;
    
    case WiggleStates::START_WIGGLE:
      state = WiggleStates::END_WIGGLE;
      elbowPosTrans = ServoPosTransition(ON_GROUND, 40, READY_PUSH);//1900ms
      anklePosTrans = ServoPosTransition(STRAIGHT, 20, START_PUSH);//1900ms
      break;

    case WiggleStates::END_WIGGLE:
      state = WiggleStates::RETRACTED;
      elbowPosTrans = ServoPosTransition(NEUTRAL, 25, ON_GROUND);//1800ms
      anklePosTrans = ServoPosTransition(MIN, 15, STRAIGHT);//1800ms
    default:
      state = WiggleStates::RETRACTED;
      elbowPosTrans = ServoPosTransition(NEUTRAL, 25);
      ServoPosTransition anklePosTrans(MIN, 20);
      break;
    }
  }
  elbowServoPos = elbowPosTrans.run(elbowServoPos);
  ankleServoPos = anklePosTrans.run(ankleServoPos);
  elbowServo.write(elbowServoPos);
  ankleServo.write(ankleServoPos);

  if (millis() - lastStepTime >= oneStepInterval) {
    lastStepTime = millis();

    Serial.print("elbow Servo Pos: ");
    Serial.print(elbowServoPos);

    Serial.print(" ankle Servo Pos: ");
    Serial.println(ankleServoPos);
  }
}

float readY() {
  int joystickY = analogRead(yAxisPin);
  if (absoluteVal(joystickY - Y_HOME) >= ANALOG_TOLARANCE) {
      return (float)(joystickY - Y_HOME) / Y_HOME;
  } else {
      return 0;
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
