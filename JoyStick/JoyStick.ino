int xAxisPin = 0;
int yAxisPin = 1;
int zAxisPin = 8;
int xVal, yVal, zVal;

void setup() {
  pinMode(zAxisPin, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  xVal = analogRead(xAxisPin);
  yVal = analogRead(yAxisPin);
  zVal = digitalRead(zAxisPin);

  Serial.print("X: ");
  Serial.print(xVal);
  Serial.print("Y: ");
  Serial.print(yVal);
  Serial.print("  /t Z: ");
  Serial.println(zVal);
  delay(200);
}
