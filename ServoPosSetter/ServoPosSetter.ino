#include <Servo.h>
Servo myservo;

int servoPin = 3;
int servoPos = 0;

void setup() {
  Serial.begin(9600);
  myservo.attach(servoPin);
}

void loop() {
  servoPos = readPos();
  myservo.write(servoPos);
}

int readPos(){ 
  int receivedInt;
  if (Serial.available()) {
    receivedInt = Serial.parseInt();
    if (receivedInt < 180 || receivedInt > 0) {
      Serial.print("pos received: ");
      Serial.println(receivedInt);
      return receivedInt;
    } else {return servoPos;};
  } else {
    return servoPos;
  }
}