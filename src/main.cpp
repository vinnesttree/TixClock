#include <FastLED.h>
#include <Wire.h>
#include <cstdint>
#include <I2C_RTC.h>
#include <arduino.h>
#include "hardware/structs/sio.h"
#include "hardware/structs/iobank0.h"
#include "hardware/regs/addressmap.h"
#include "hardware/timer.h"

//Definitions
#define LED_PIN 2 //WS2812B pin
#define NUM_LEDS 27
#define SDA_PIN 4
#define SCL_PIN 5
#define Button1_PIN 6 //Set pin
#define Button2_PIN 7 //Minute pin
#define Button3_PIN 8 //Hour pin
#define Charge_PIN 9
#define Standby_PIN 10
#define Debug_PIN 25 //Debug LED pin

#define Clock_Address 0xD0

//Defines for compile time clock setting
#define CHAR_TO_INT(c) ((c) - '0')

#define HOUR   (CHAR_TO_INT(__TIME__[0]) * 10 + CHAR_TO_INT(__TIME__[1]))
#define MINUTE (CHAR_TO_INT(__TIME__[3]) * 10 + CHAR_TO_INT(__TIME__[4]))
#define SECOND (CHAR_TO_INT(__TIME__[6]) * 10 + CHAR_TO_INT(__TIME__[7]))

//Global Variables
static PCF8523 RTC;
CRGB leds[NUM_LEDS];
uint8_t hour = 0;
uint8_t minute = 0;
uint32_t offset = 0;
uint32_t blinkTime = 0;
uint8_t setButtonPrev = 0;

void ReadClockData(uint8_t *hours, uint8_t *minutes){
  *hours = RTC.getHours();
  *minutes = RTC.getMinutes();
  /*
  Serial.print(*hours);
  Serial.print(":");
  Serial.println(*minutes);
  */
}

void SetClockData(uint8_t hour, uint8_t minute){

}

uint16_t generate_pattern(uint32_t rand_val, uint8_t on_bits, uint8_t total_bits) {
    if (total_bits > 16) total_bits = 16;
    if (on_bits > total_bits) on_bits = total_bits;

    uint16_t pattern = 0;
    uint8_t count = 0;

    // Fill the rightmost 'total_bits' using bits from rand_val
    for (uint8_t i = 0; i < total_bits && count < on_bits; i++) {
        if ((rand_val >> i) & 1U) {
            pattern |= (1U << i);
            count++;
        }
    }

    // If we didn’t get enough ON bits, fill remaining from LSB upwards
    for (uint8_t i = 0; i < total_bits && count < on_bits; i++) {
        if (!(pattern & (1U << i))) {
            pattern |= (1U << i);
            count++;
        }
    }

    // Mask to ensure only 'total_bits' are used and pattern is right-aligned
    pattern &= (1U << total_bits) - 1;

    return pattern;
}


void SetLEDs(uint8_t hours, uint8_t minutes){
  //Clear all LEDs
  FastLED.clear();

  uint32_t random_value = get_rand_32();

  //Set hours LEDs
  int hour10 = hours / 10;
  int hour1 = hours % 10;
  uint16_t hour10_pattern = generate_pattern(random_value, hour10, 3);
  uint16_t hour1_pattern = generate_pattern(random_value >> 8, hour1, 9);
  for(uint8_t i=0; i<3; i++){
    if(hour10_pattern & (1 << i)){
      leds[i] = CRGB::Red;
    } else {
      leds[i] = CRGB::Black;
    }
  }
  for(uint8_t i=0; i<9; i++){
    if(hour1_pattern & (1 << i)){
      leds[i+3] = CRGB::Green;
    } else {
      leds[i+3] = CRGB::Black;
    }
  }

 //Set minutes LEDs
  int minute10 = minutes / 10;
  int minute1 = minutes % 10;
  uint16_t minute10_pattern = generate_pattern(random_value >> 16, minute10, 6);
  uint16_t minute1_pattern = generate_pattern(random_value >> 24, minute1, 9);
  for(uint8_t i=0; i<6; i++){
    if(minute10_pattern & (1 << i)){
      leds[i+12] = CRGB::Blue;
    } else {
      leds[i+12] = CRGB::Black;
    }
  }
  for(uint8_t i=0; i<9; i++){
    if(minute1_pattern & (1 << i)){
      leds[i+18] = CRGB::Yellow;
    } else {
      leds[i+18] = CRGB::Black;
    }
  }

  FastLED.show();
}

void SetLEDsNRAND(uint8_t hours, uint8_t minutes){ //Set LEDs without randomness for easier setting
  //Clear all LEDs
  FastLED.clear();


  //Set hours LEDs
  int hour10 = hours / 10;
  int hour1 = hours % 10;
  for(uint8_t i=0; i<hour10; i++){
    leds[i] = CRGB::Red;
  }
  for(uint8_t i=0; i<hour1; i++){
    leds[i+3] = CRGB::Green;
  }

 //Set minutes LEDs
  int minute10 = minutes / 10;
  int minute1 = minutes % 10;
  for(uint8_t i=0; i<minute10; i++){
    leds[i+12] = CRGB::Blue;
  }
  for(uint8_t i=0; i<minute1; i++){
    leds[i+18] = CRGB::Yellow;
  }

  FastLED.show();

}

void setup(){
  //Initialize FastLED and RTC Communication
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(25);

  //Serial.begin(115200);
  //RTC.begin();

  

  pinMode(Button1_PIN, INPUT);
  pinMode(Button2_PIN, INPUT);
  pinMode(Button3_PIN, INPUT);
  pinMode(Charge_PIN, INPUT);
  pinMode(Standby_PIN, INPUT);

  pinMode(Debug_PIN, OUTPUT);


  //RTC.setHourMode(CLOCK_H24);
  hour = HOUR;
  minute = MINUTE;
  offset = (60 - SECOND) * 1000; //offset time in ms (time past top of the minute)
  //offset = 60 * 1000 - (RTC.getSeconds() * 1000); //calculate offset to next minute

  //ReadClockData(&hour, &minute);

  SetLEDs(hour, minute);

}

void loop(){
  uint32_t currentTime = millis();

  // Poll if a minute has passed
  if(currentTime >= offset){
    offset += 60 * 1000; //set next offset
    minute++;
    if(minute >= 60){
      minute = 0;
      hour++;
      if(hour >= 24){
        hour = 0;
      }
    }
    SetLEDs(hour, minute);
    //minute++;
  }

  if(currentTime >= blinkTime){
    blinkTime += 500; //set next blink
    digitalWrite(Debug_PIN, !digitalRead(Debug_PIN));
  }

  while(digitalRead(Button1_PIN) == LOW){ //Holding button 1 acts as a set
    delay(20); //debounce
    if(digitalRead(Button1_PIN) == LOW){
      SetLEDsNRAND(hour, minute);
      setButtonPrev = LOW;
      if(digitalRead(Button2_PIN) == LOW){
        delay(20); //debounce
        if(digitalRead(Button2_PIN) == LOW){
          minute++;
          if(minute >= 60){
            minute = 0;
            hour++;
            if(hour >= 24){
              hour = 0;
            }
          }
          SetLEDsNRAND(hour, minute);
          while(digitalRead(Button2_PIN) == LOW); //wait for release
        }
      }
      if(digitalRead(Button3_PIN) == LOW){
        delay(20); //debounce
        if(digitalRead(Button3_PIN) == LOW){
          hour++;
          if(hour >= 24){
            hour = 0;
          }
          SetLEDsNRAND(hour, minute);
          while(digitalRead(Button3_PIN) == LOW); //wait for release
        }
      }
    }
  }

  if(setButtonPrev == LOW){ //Re-display time with the random effect only when button is released
    //SetLEDs(hour, minute);
    SetClockData(hour, minute); //set RTC after setting time
    offset = currentTime; //reset offset (seconds past top of minute)
    minute--; //account for trigger to not increment time
    setButtonPrev = HIGH;
  }

}