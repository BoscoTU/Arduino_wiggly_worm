int outPorts[] = {11, 10, 9, 8};

//motor control variables 
int stepsToDo = 0;
unsigned long lastStepTime = 0;

const unsigned long oneStepInterval = 4;
int ticksFromOrigin = 0;

//joystick variables
int xAxisPin = 0;
int yAxisPin = 1;
int zAxisPin = 2;

int X_HOME = 516;
int ANALOG_TOLARANCE = 50;
int STEP_TOLARANCE = 20;
float xVal, yVal;
int zVal;

//telemetry variables
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 250;
void setup() {
    for (int i = 0; i < 4; i++) {
        pinMode(outPorts[i], OUTPUT);
    }
    pinMode(zAxisPin, INPUT_PULLUP);
    Serial.begin(9600);
}

void loop() {
    xVal = readX();
    stepsToDo = joyStickXFollower();
    stepperMotorController();
    telemetry();
}

long readX() {
    int joystickX = analogRead(xAxisPin);
    if (absoluteVal(joystickX - X_HOME) >= ANALOG_TOLARANCE) {
        return (float)(joystickX - X_HOME) / X_HOME * 90;
    } else {
        return 0;
    }
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
        int stepDifference = 512 * xVal/90.0 - (ticksFromOrigin * -1);
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

void telemetry() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastTelemetryTime >= telemetryInterval) {
        lastTelemetryTime = currentMillis;
        // Serial.println(analogRead(xAxisPin));
        Serial.print("steps to do: ");
        Serial.print(stepsToDo);
        Serial.print(" difference ");
        Serial.print(512 * xVal/90.0 - (ticksFromOrigin * -1));
        Serial.print(" ticksFromOrigin: ");
        Serial.println(ticksFromOrigin);
    }
}

int absoluteVal(int x) {
    return (x < 0)? -x: x;
}