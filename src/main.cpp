#include <Arduino.h>
#include "FastLED.h"

/* This is a sketch for a clock which has uses random intervals to move the second hand 
   
   Design goals: 
   * Keep it simple
   * Make it second hand feel like it is random
   * Time keeping of the minute hand should be accurate to within +/- 45 seconds 
   * The physical hand can't be pulsed faster than about 8 times per second reliably

 */

unsigned long fiveHzTimer = 0;
unsigned long oneHzTimer = 0;
unsigned long secondsTicked = 120;
unsigned long secondsActual = 120;
int extra_ticks = 0;
int correctionMode = 0; // 0 = normal, 1 = speed up, 2 = slow down
bool no_ticking = false;
bool normal_clock = false; // don't do vetinari and effects. 
int sync_pointer = -1;

#define NUM_LEDS 68
#define LEDS_PIN 2
CRGB leds[NUM_LEDS];

char second_map[60] = {66,64,63,62,61,60,59,58,57,56,55,54,53,52,51, 
                       49,47,46,45,44,43,42,41,40,39,38,37,36,35,34,
                       32,30,29,28,27,26,25,24,23,22,21,20,19,18,17,
                       15,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

char twelve_bar[3] = {67, 66, 65};
char three_bar[3] = {50, 49, 48};
char six_bar[3] = {33, 32, 31};
char nine_bar[3] = {16, 15, 14};

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

  if (!no_ticking) {
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
  } else {
    delay(50);
  }
  digitalWrite(13, LOW);


}

void setup() {

  FastLED.addLeds<WS2811, LEDS_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,500); 
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

void set_bar(char *bar, CRGB color) {
  leds[bar[0]] = color;
  leds[bar[1]] = color;
  leds[bar[2]] = color;  
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
  if (normal_clock) {
    tick();
  }
}

void fiveHzLoop() {
  // this function gets called at 200ms intervals, without drift. 
  long r = random(1000); // 0 - 999
  if (normal_clock) {
    r = 1000; // no ticks from this function. But will draw the leds.
  }

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

  static int led = 0;  

  if ( r < threshold ) {
    for (int i = 0; i< 60; i++) {
      leds[second_map[i]] = CRGB::Red;
    }
    leds[second_map[(secondsTicked+1) % 60]] = CRGB::White;
    FastLED.show();
  } 
  delay(1);

  FastLED.clear();
  set_bar(twelve_bar, CRGB::Red);
  set_bar(three_bar, CRGB::Green);
  set_bar(six_bar, CRGB::Blue);
  set_bar(nine_bar, CRGB::Pink);

  if (!normal_clock) {
    leds[second_map[secondsActual % 60]] = CRGB::DarkGreen;
    if (sync_pointer > -1) {
      leds[second_map[sync_pointer % 60]] = CRGB::DarkViolet;
    }
  }

  led += 1;
  led = led % 60;
  if ( r < threshold ) {
    leds[second_map[(secondsTicked+1) % 60]] = CRGB::White;
  } else {
    leds[second_map[secondsTicked % 60]] = CRGB::White;
  }
  FastLED.show();


  if ( r < threshold ) {
    // 1 in 5 chance of a tick. 
    tick();    
  } else if (extra_ticks > 0) {
    extra_ticks -= 1;
    tick();
  }


}

void syncSecondsHand(int second) {
  int currentSecond = secondsActual % 60;
  int delta = second - currentSecond;
   if (delta < -29) {
     delta = delta + 60;
   } else if (delta > 29) {
     delta = -(60 - delta);  
   }
  secondsTicked = secondsActual + delta;

}

void loop() {
  unsigned long now = millis();
  if (now > oneHzTimer) {
    oneHzTimer += 1000;
    oneHzLoop();
  }
  if (now > fiveHzTimer) {
    fiveHzTimer += 200; 
    fiveHzLoop();
  }
  while (Serial.available()) {
    int byte = Serial.read();
    switch (byte) {
      case 't': // just an extra tick, manual
        extra_ticks += 1;
        break;
      case 'e': // enable mechanical clock
        no_ticking = false;
        sync_pointer = -1;
        break;
      case 'd': // disable mechanical clock
        no_ticking = true;
        break;
      case 'n': // disable mechanical clock
        normal_clock = true;
        break;
      case 'v': // disable mechanical clock
        normal_clock = false;
        break;
      case 's': // step pointer to align with second hand.
        no_ticking = true;
        sync_pointer += 1;
        sync_pointer = sync_pointer % 60;
        break;
      case 'S': // step pointer 5 seconds
        no_ticking = true;
        sync_pointer += 5;
        sync_pointer = sync_pointer % 60;
        break;
      case 'a': // activate 
        syncSecondsHand(sync_pointer);
        no_ticking = false;
        sync_pointer = -1;
        break;
    }

  }
}