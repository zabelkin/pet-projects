#define CYCLES 3 // cycles to skip for blinking LEDs

int sensorPin1 = A0;    // select the input pin for the potentiometer
int sensorValue1;  // variable to store the value coming from the sensor
int mappedValue1;
int LED1 = 2;

int sensorPin2 = A1;    // select the input pin for the potentiometer
int sensorValue2;  // variable to store the value coming from the sensor
int mappedValue2;
int LED2 = 3;

int delayValue;
int skip_cycles1 = CYCLES;
int skip_cycles2 = CYCLES;


void setup() {
  Serial.begin(9600);
}

void loop() {
  delayValue = 5000;
  sensorValue1 = analogRead(sensorPin1);
  mappedValue1 = map(sensorValue1, 0, 1023, 0, 10);
  sensorValue2 = analogRead(sensorPin2);
  mappedValue2 = map(sensorValue2, 0, 1023, 0, 10);

  Serial.print(mappedValue1);
  Serial.print(":");
  Serial.println(mappedValue2);

  if (mappedValue1>3 and skip_cycles1==0)
  {
    blink_twice(LED1);
    delayValue -= 1100;
    skip_cycles1 = CYCLES;
  }
  if (mappedValue2>3 and skip_cycles2==0)
  {
    blink_twice(LED2);
    delayValue -= 1100;
    skip_cycles2 = CYCLES;
  }
  delay(delayValue);
  skip_cycles1 -=1;
  skip_cycles2 -=1;
}

void blink_twice(int dport) {
  digitalWrite(dport, HIGH);
  delay(150);
  digitalWrite(dport, LOW);
  delay(300);
  digitalWrite(dport, HIGH);
  delay(150);
  digitalWrite(dport, LOW);
  delay(300);
}
