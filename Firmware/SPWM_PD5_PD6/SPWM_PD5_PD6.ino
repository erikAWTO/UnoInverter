#include <avr/io.h>
#include <avr/interrupt.h>

#define LookupEntries (512)

static int microMHz = 16;   // clock frequency in MHz
static int freq, amp = 1024;// Sinusoidal frequency
static long int period;     // Period of PWM in clock cycles. 1600 gives 10KHz.
static unsigned int lookUp[LookupEntries];
static char theTCCR1A = 0b10000010; //varible for TCCR1A
static unsigned long int phaseinc, switchFreq;
static double phaseincMult;

int setFreq(int freq);
int setSwitchFreq(int sfreq);
int setAmp(float _amp);
void makeLookUp(void);
void registerInit(void);

void setup(){ 
  Serial.begin(9600);
  makeLookUp();
  setSwitchFreq(10000);  
  setFreq(50);
  setAmp(100);
  registerInit();
}

void loop(){
  static int ampVal, freqVal, anologVal;

  anologVal = analogRead(A0);
  if(anologVal > freqVal*1.01 || anologVal < freqVal*0.99){
    freqVal = anologVal;
    setFreq(map(freqVal, 0, 1023, 5, 300));
    Serial.println("phaseinc");
    Serial.print(phaseinc>>23);
    Serial.print(".");
    Serial.print(phaseinc&0x007FFFFF);
    Serial.print("\n"); 
  }
  
  anologVal = analogRead(A1);
  if(anologVal > ampVal*1.01 || anologVal < ampVal*0.99){
    ampVal = anologVal;
    setAmp(map(ampVal, 0, 1023, 0, 100));
    Serial.println("amplitude");    
    Serial.println(amp);
  }
  
  delay(20);
  static char cnt = 0;
  cnt++;
  if(cnt == 100){
    setSwitchFreq(15000);
    cnt = 0;
  } else if(cnt == 50){
    setSwitchFreq(5000);
  }
}

ISR(TIMER1_OVF_vect){
  static unsigned long int phase, lastphase;
  static char delay1, trig = LOW;

  phase += phaseinc;

  if(delay1 == 1){
    theTCCR1A ^= 0b10100000;// Toggle connect and disconnect of compare output A and B.
    TCCR1A = theTCCR1A;
    delay1 = 0;  
  } 
  else if((phase>>31 != lastphase>>31) && !(phase>>31)){
    delay1++;      
    trig = !trig;
    digitalWrite(13,trig);
  }
  
  lastphase = phase;
  OCR0A = OCR0B = ((lookUp[phase >> 23]*period) >> 12)*amp >> 10;
}

// Rest of the functions remain the same

void registerInit(void){
  // Register initialization for Timer/Counter1
  TCCR1A = theTCCR1A;
  TCCR1B = 0b00011001;
  TIMSK1 = 0b00000001;
  sei();             // Enable global interrupts

  // Configure Timer/Counter0 for Fast PWM mode on PD5 and PD6
  TCCR0A = 0b10100011; // Fast PWM, Clear OC0A/OC0B on compare match, set at BOTTOM
  TCCR0B = 0b00000001; // No prescaler
  DDRD |= 0b01100000;  // Set PD5 and PD6 as outputs
  
  pinMode(13, OUTPUT); // Set trigger pin to output
}