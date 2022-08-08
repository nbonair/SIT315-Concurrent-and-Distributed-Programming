// C++ code

const int LED = 12;
const int PIR1 = 2; //PCINT18
const int PIR2 = 3; //19
const int PIR3 = 4; //20
int led_state = LOW;
const int led_builtin = PB5;

const uint16_t t1_load = 0;
const uint16_t t1_comp = 31250;

void setup()
{
  Serial.begin(9600);
  pinMode(PIR1,INPUT);
  pinMode(PIR2,INPUT);
  pinMode(PIR3,INPUT);
  pinMode(LED,OUTPUT);
  
  
  DDRB |= (1<<led_builtin);
  
  // reset time control
  TCCR1A = 0;
  
  //set prescaler of 256
  
  TCCR1B |= (1<<CS12);
  TCCR1B &= ~(1<<CS11);
  TCCR1B &= ~(1<<CS10);
  
  // reset timer 1 n compare value
  TCNT1 = t1_load;
  OCR1A = t1_comp;
  
  //enable compare interrupt
  TIMSK1 = (1 << OCIE1A);
  
  PCMSK2 |= bit (PCINT18); // Pin D02
  PCMSK2 |= bit (PCINT19); // Pin D03
  PCMSK2 |= bit (PCINT20); // Pin D04
  PCIFR  |= bit (PCIF2);   // clear any outstanding interrupts
  PCICR  |= bit (PCIE2);
  
  sei();
}

void loop() {
}

// interrupt routine timer 1
ISR(TIMER1_COMPA_vect) 
{
  TCNT1 = t1_load;
  PORTB ^= (1<<led_builtin);
}
// interupt routine pcint2 (D0-7)
ISR(PCINT2_vect)
{
  led_state = !led_state;
  if(digitalRead(PIR1) == HIGH) {Serial.write("PIR1 interrupt\n");}
  if(digitalRead(PIR2) == HIGH) {Serial.write("PIR2 interrupt\n");}
  if(digitalRead(PIR3) == HIGH) {Serial.write("PIR3 interrupt\n");}
  digitalWrite(LED, led_state);
}