#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <Adafruit_NeoPixel.h>
#include <ezButton.h>
#include "LightPatterns.cpp"
#include "Keyboard.h"

#define SW_VER 1.0
//#define DEBUG 1

/* ======================================================================== */
/* ===                           CONSTANTS                              === */
/* ======================================================================== */

//Input
#define PIN_BUTTON_ONE 6
#define PIN_BUTTON_ZERO 7
#define PIN_BUTTON_DONE 8
#define KEY_DEBOUNCE_TIME_MS 5
#define INACTIVE_CLEAR_TIME_MS 30000

//Neopixel
#define NUM_PIXELS 8
#define PIN_NEO_PIXEL 9 //Neopixel pin
#define NEO_PIXEL_BRIGHTNESS 200 //0-255 brightness setting

//Light patterns
#define NUM_LIGHT_PATTERNS 7
#define LIGHT_PTN_NONE 0
#define LIGHT_PTN_RAINBOW_NORMAL 1
#define LIGHT_PTN_SPARKLE 2
#define LIGHT_PTN_RAINBOW_BLOOM 3
#define LIGHT_PTN_HELL 4
#define LIGHT_PTN_RGB_FADE 5
#define LIGHT_PTN_POPO 6 
#define LIGHT_PTN_WEED 7

/* ======================================================================== */
/* ===                           HARDWARE                               === */
/* ======================================================================== */

//Locks
static mutex_t light_lock;
static mutex_t write_lock;
uint32_t write_lock_owner;
uint32_t light_lock_owner;

//Key input
ezButton key_done(PIN_BUTTON_DONE);
ezButton key_zero(PIN_BUTTON_ZERO);
ezButton key_one(PIN_BUTTON_ONE);

/* ======================================================================== */
/* ===                             STATE                                === */
/* ======================================================================== */
//Character
uint8_t current_char = 0x0;
uint8_t current_bit = 0x0; //From the right, MSB
uint8_t current_num_bits = 0x0;
unsigned long last_bit_time = 0x0;

//Lights
uint8_t light_pattern = LIGHT_PTN_NONE;
void PatternComplete(); // Define a callback to tell what to do after the pattern completes
NeoPatterns NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800, &PatternComplete);

//Actions
bool did_cycle_lights = false;

/* ======================================================================== */
/* ===                              INPUT                              ==== */
/* ======================================================================== */
void reset_buffer() {
  /* Resets all character-related state and buffer*/

  current_char = 0x0;
  current_bit = 0x0;
  current_num_bits = 0x0;
}

void done_handler() {
  /* Handles single press of done button */

  if (!mutex_try_enter(&write_lock, &write_lock_owner)) {
    mutex_enter_blocking(&write_lock);
  }

  if(current_char == 0) //If there's no data in the char buffer, treat as a single backspace
  {
    Keyboard.write(KEY_BACKSPACE);
  }
  else { //Otherwise, erase all bits we've input on screen and print the char associated with the current bits in buffer
    for(int i = 0; i < current_num_bits; i++) {
        Keyboard.write(KEY_BACKSPACE);
    }

    Keyboard.write((char) current_char);
    reset_buffer();
  }

  mutex_exit(&write_lock);
}

void bit_handler(bool val) {
  /* Handles press of 0 or 1 button */

  if (!mutex_try_enter(&write_lock, &write_lock_owner)) {
    mutex_enter_blocking(&write_lock);
  }

  uint8_t tmp = 0x0;

  tmp |= val ? 0x80 : 0x0; //If the one key is pressed, set MSB to 1
  tmp >>= current_bit++;
  current_char |= tmp;
  
  if(current_bit > 7) { //Wrap current bit tracker around to 0
    current_bit = 0x0;
  }
  current_num_bits++;

  Keyboard.write(val ? '1' : '0'); //Write bit to screen
  last_bit_time = millis(); //Track last time we entered a bit
  did_cycle_lights = false;

  mutex_exit(&write_lock);
}

/* ======================================================================== */
/* ===                              LIGHTS                              === */
/* ======================================================================== */

void cycle_light_pattern(bool forward) {
  /*Cycles the current light pattern forwards or backwards*/

  if (!mutex_try_enter(&light_lock, &light_lock_owner)) {
    mutex_enter_blocking(&light_lock);
  }
  if(forward) {
    if(light_pattern + 1 > NUM_LIGHT_PATTERNS) {
      light_pattern = 0;
    }
    else {
      light_pattern++;
    }
  }
  else {
    if(light_pattern == 0) {
      light_pattern = NUM_LIGHT_PATTERNS;
    }
    else {
      light_pattern--;
    }
  }
  did_cycle_lights = true;
  mutex_exit(&light_lock);

  #ifdef DEBUG
  Serial.printf("Cycle lights: %d\n", light_pattern);
  #endif
}

void PatternComplete() {
  /* Function that's called when NeoPixel pattern is complete  */

  if(NeoPixel.ActivePattern == RGB_FADE) { //If we are in the rgb fade pattern, cycle through all the colors
    if(NeoPixel.Color1 == NeoPixel.Color(0xFF, 0, 0)) {
      NeoPixel.Color1 = NeoPixel.Color(0, 0xFF, 0);
      NeoPixel.Color2 = NeoPixel.Color(0, 0, 0xFF);
    }
    else if(NeoPixel.Color1 == NeoPixel.Color(0, 0xFF, 0)) {
      NeoPixel.Color1 = NeoPixel.Color(0, 0, 0xFF);
      NeoPixel.Color2 = NeoPixel.Color(0xFF, 0, 0);
    }
    else {
      NeoPixel.Color1 = NeoPixel.Color(0xFF, 0, 0);
      NeoPixel.Color2 = NeoPixel.Color(0, 0xFF, 0);
    }
  }
  else if(NeoPixel.ActivePattern == FADE) { //Loop back the other way
    uint32_t temp = NeoPixel.Color1;
    NeoPixel.Color1 = NeoPixel.Color2;
    NeoPixel.Color2 = temp;
  }

  //Add a delay for those patterns that have a delay between them
  switch(NeoPixel.ActivePattern) {
    case COLOR_WIPE:
      NeoPixel.FinalDelay = 1000;
      break;
  }
}

void core1_entry() {
  /* Handles light pattern display on RP core 1 */

  uint8_t last_light_pattern = LIGHT_PTN_NONE;

  while(true) {
    if (!mutex_try_enter(&light_lock, &light_lock_owner)) {
      mutex_enter_blocking(&light_lock);
    }
    uint8_t current_light_pattern = light_pattern;
    mutex_exit(&light_lock);

    if(last_light_pattern != current_light_pattern) {
      last_light_pattern = current_light_pattern;

      switch(current_light_pattern) {
        case LIGHT_PTN_RAINBOW_NORMAL:
          NeoPixel.RainbowCycle(5);
          break;
        case LIGHT_PTN_RAINBOW_BLOOM:
          NeoPixel.RainbowBloom(75);
          break;
        case LIGHT_PTN_SPARKLE:
          NeoPixel.Sparkle(NeoPixel.Color(128, 0, 128), 100);
          break;
        case LIGHT_PTN_WEED:
          NeoPixel.Scanner(NeoPixel.Color(0, 0xFF, 0), 100);
          break;
        case LIGHT_PTN_HELL:
          NeoPixel.Fade(NeoPixel.Color(0xFF, 0, 0), NeoPixel.Color(0, 0, 0), 50, 25);
          break;
        case LIGHT_PTN_POPO:
          NeoPixel.OhNoPoPo(NeoPixel.Color(0xFF, 0, 0), NeoPixel.Color(0, 0, 0xFF), 100);
          break;
        case LIGHT_PTN_RGB_FADE:
          NeoPixel.RGBFade(NeoPixel.Color(0xFF, 0, 0), NeoPixel.Color(0, 0, 0xFF), 50, 50);
          break;
        case LIGHT_PTN_NONE:
          NeoPixel.ActivePattern = NONE;
          NeoPixel.clear();
          NeoPixel.show();
          break;
      }
    }

    NeoPixel.Update();
    sleep_ms(1);
  }
}

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  #endif

  //Init HID
  Keyboard.begin();

  //Init Neopixel
  NeoPixel.begin();
  NeoPixel.clear();
  NeoPixel.show();

  //Init locks
  mutex_init(&light_lock);
  mutex_init(&write_lock);

  //Init input
  pinMode(PIN_BUTTON_ZERO, INPUT_PULLUP);
  pinMode(PIN_BUTTON_ONE, INPUT_PULLUP);
  pinMode(PIN_BUTTON_DONE, INPUT_PULLUP);

  key_zero.setDebounceTime(KEY_DEBOUNCE_TIME_MS);
  key_one.setDebounceTime(KEY_DEBOUNCE_TIME_MS);
  key_done.setDebounceTime(KEY_DEBOUNCE_TIME_MS);

  //Start core 1 thread
  multicore_launch_core1(core1_entry);
}

void loop() {
  /*Handles key press detection and input on RP core 0*/

  key_zero.loop();
  key_one.loop();
  key_done.loop();

  if(key_zero.isPressed()) {
    if(!digitalRead(PIN_BUTTON_DONE)) { //Done button is also pressed, cycle light pattern backwards
      cycle_light_pattern(false);
    }
    else {
      bit_handler(false); //Only the zero button is pressed, track new 0 bit
    }
  }
  else if(key_one.isPressed()) {
    if(!digitalRead(PIN_BUTTON_DONE)) { //Done button is also pressed, cycle light pattern forwards
      cycle_light_pattern(true);
    }
    else {
      bit_handler(true); //Only the one button is pressed, track new 1 bit
    }
  }

  if(key_done.isReleased()) { //Handle done action only on release of button
    if(!did_cycle_lights) {
      done_handler(); //If we didn't cycle the lights, do something with whatever's in the char buffer
    }
    did_cycle_lights = false;
  }

  if(current_char > 0 && millis() - last_bit_time > INACTIVE_CLEAR_TIME_MS) { //After INACTIVE_CLEAR_TIME_MS, clear whatever's in the buffer
    reset_buffer();

    #ifdef DEBUG
    Serial.println("Reset buffer due to inactivity.");
    #endif
  }
}
