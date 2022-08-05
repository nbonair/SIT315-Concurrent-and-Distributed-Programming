// C++ code

int LED1 = 12;
int LED2 = 8;
int PIR1 = 3;
int PIR2 = 2;
void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIR1,INPUT);
  pinMode(LED1,OUTPUT);
  
  pinMode(PIR2,INPUT);
  pinMode(LED2,OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PIR1),motion_detect_1,CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIR2),motion_detect_2,CHANGE);

}

void loop()
{
}

void motion_detect_1()
{
  int motion = digitalRead(PIR1);
  digitalWrite(LED1,motion);
  if (motion == HIGH)
  {
    Serial.write("Motion detected by sensor 1\n");
  }
}

void motion_detect_2()
{
  int motion = digitalRead(PIR2);
  digitalWrite(LED2,motion);
  if (motion == HIGH)
  {
    Serial.write("Motion detected by sensor 2\n");
  }
}