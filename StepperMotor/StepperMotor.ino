int outPorts[] = {11, 10, 9, 8};

//motor control variables 
int stepsToDo = 0;
unsigned long lastStepTime = 0;

const unsigned long oneStepInterval = 4;
int ticksFromOrigin = 0;

//joystick variables

//telemetry variables
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 250;
void setup() {
    for (int i = 0; i < 4; i++) {
        pinMode(outPorts[i], OUTPUT);
    }
    Serial.begin(9600);
}

void loop() {
    stepsToDo = joyStickFollower();
    stepperMotorController();
    telemetry();
}

int joyStickFollower() {
    int receivedInt;
    if (Serial.available()) {
        receivedInt = Serial.parseInt();
        Serial.print("received: ");
        Serial.print(receivedInt);
        return receivedInt;
    } else {
        return stepsToDo;
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
        Serial.println(stepsToDo);
    }
}