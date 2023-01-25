#include <Arduino.h>

/* This is a sketch for a clock which has uses random intervals to move the second hand 
   
   Design goals: 
   * Keep it simple
   * Make it second hand feel like it is random
   * Time keeping of the minute hand should be accurate to within +/- 45 seconds 
   * The physical hand can't be pulsed faster than about 8 times per second reliably

 */

unsigned long fiveHzTimer = 0;
unsigned long oneHzTimer = 0;
unsigned long secondsTicked = 0;
unsigned long secondsActual = 0;
int correctionMode = 0; // 0 = normal, 1 = speed up, 2 = slow down

#define NEUTRAL_MODE 0
#define FAST_MODE 1
#define SLOW_MODE 2
#define VERYFAST_MODE 3
#define VERYSLOW_MODE 4

#define CLOCK_PIN_A 4
#define CLOCK_PIN_B 5


void tick() {
  static bool ticktock = false;
  // move the second hand. 

  digitalWrite(13, HIGH);
  secondsTicked += 1;
  if (ticktock) {
    ticktock = false;
    digitalWrite(CLOCK_PIN_A, HIGH);
    delay(20);
    digitalWrite(CLOCK_PIN_A, LOW);

  } else {
    ticktock = true;
    digitalWrite(CLOCK_PIN_B, HIGH);
    delay(20);
    digitalWrite(CLOCK_PIN_B, LOW);
  }

  delay(30);
  digitalWrite(13, LOW);


}

void setup() {
  fiveHzTimer = millis(); 
  oneHzTimer = fiveHzTimer + 1000;
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  pinMode(CLOCK_PIN_A, OUTPUT);
  pinMode(CLOCK_PIN_B, OUTPUT);
  digitalWrite(CLOCK_PIN_A, LOW);
  digitalWrite(CLOCK_PIN_B, LOW);

  randomSeed(analogRead(0));

  correctionMode = FAST_MODE;
}


void oneHzLoop() {
  // this function gets called exactly once per second
  secondsActual += 1;  
  long delta = secondsTicked - secondsActual;
  Serial.println(delta);
  if (correctionMode != SLOW_MODE && delta > 20) {
    correctionMode = SLOW_MODE;
    Serial.println("Slowing down");
  } else if (correctionMode != FAST_MODE && delta < -20) {
    correctionMode = FAST_MODE;
    Serial.println("Speeding up");
  } else if (correctionMode != NEUTRAL_MODE && delta > -5 && delta < 5) {
    correctionMode = NEUTRAL_MODE;
  }
}

void fiveHzLoop() {
  // this function gets called at 200ms intervals, without drift. 
  long r = random(1000); // 0 - 999

  long threshold = 200;
  
  if (correctionMode == VERYFAST_MODE) {
    threshold += 200 ;
  } else if (correctionMode == FAST_MODE) {
    threshold += 20 ;
  } else if (correctionMode == SLOW_MODE) {
    threshold -= 20;
  } else if (correctionMode == VERYSLOW_MODE) {
    threshold -= -100 ;
  }

  if ( r < threshold ) {
    // 1 in 5 chance of a tick. 
    tick();
    
  } 

}


void loop() {
  unsigned long now = millis();
  if (now > fiveHzTimer) {
    fiveHzTimer += 200; 
    fiveHzLoop();
  }
  if (now > oneHzTimer) {
    oneHzTimer += 1000;
    oneHzLoop();
  }
}