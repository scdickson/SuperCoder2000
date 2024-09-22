/*
* Original code courtesy of Jeff G (github.com/x9prototype) 
*/

#include <Adafruit_NeoPixel.h>  // Adafruit NeoPixel by Adafruit
#include <stdint.h>

//Light patterns
enum pattern {
  NONE,
  RAINBOW_CYCLE,
  RAINBOW_BLOOM,
  COLOR_WIPE,
  SCANNER,
  FADE,
  RGB_FADE,
  SPARKLE,
  SPARKLE_RANDOM,
  THEATER_CHASE,
  OH_NO_POPO,
  COLOR_STRIPES
};

enum direction {
  FORWARD,
  REVERSE
};

class NeoPatterns: public Adafruit_NeoPixel {
  public:
    pattern ActivePattern;          // Which pattern we are running
    direction Direction;            // Which direction to run the pattern

    unsigned long Delay;            // Delay between each update
    unsigned long lastUpdate;       // Last update time for the pattern
    unsigned long FinalDelay;       // Delay after the program is run before its run again
  
    uint32_t Color1, Color2, Color3;  // Storage for pattern specific colors
    uint16_t TotalSteps;              // The total steps in the pattern
    uint16_t CurrentStep;             // The current step in the pattern

    void (*OnComplete)();           // Callback function when pattern completes

    // Constructor
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
    : Adafruit_NeoPixel(pixels, pin, type) {
      OnComplete = callback;
      FinalDelay = 0;
    }

    // Update the pattern
    void Update() {
      // Check to see if there is a final delay set
      if((millis() - lastUpdate) >= FinalDelay) {
        // We've passed the delay, reset it
        FinalDelay = 0;
      }
      else {
        // Not at the final delay yet, 
        return;
      }

      // Ready to update?
      if((millis() - lastUpdate) > Delay) {
        lastUpdate = millis();

        switch(ActivePattern) {
          case RAINBOW_CYCLE:
            RainbowCycleUpdate();
            break;
          case RAINBOW_BLOOM:
            RainbowBloomUpdate();
            break;
          case THEATER_CHASE:
            TheaterChaseUpdate();
            break;
          case COLOR_WIPE:
            ColorWipeUpdate();
            break;
          case SCANNER:
            ScannerUpdate();
            break;
          case FADE:
            FadeUpdate();
            break;
          case RGB_FADE:
            RGBFadeUpdate();
            break;
          case SPARKLE:
            SparkleUpdate();
            break;
          case SPARKLE_RANDOM:
            SparkleRandomUpdate();
            break;
          case OH_NO_POPO:
            OhNoPoPoUpdate();
            break;
          case COLOR_STRIPES:
            ColorStripesUpdate();
            break;
          default:
            break;
        }
      }
    }

    // Increment the current position based on direction, loop around
    void IncrementPattern(uint8_t incrementAmount = 1) {
      if (Direction == FORWARD) {
        CurrentStep += incrementAmount;
        if(CurrentStep >= TotalSteps) {
          CurrentStep = 0;

          // Since we looped, check if there is a callback function
          if(OnComplete != NULL) {
            OnComplete();
          }
        }
      }
      else {
        CurrentStep -= incrementAmount;
        if(CurrentStep <= 0) {
          CurrentStep = TotalSteps - 1;

          // Since we looped, check if there is a callback function
          if(OnComplete != NULL) {
            OnComplete();
          }
        }
      }
    }

    // Reverse the pattern direction and reset to beginning of pattern
    void ReversePattern() {
      if (Direction == FORWARD) {
        Direction = REVERSE;
        CurrentStep = TotalSteps -1;
      }
      else {
        Direction == FORWARD;
        CurrentStep = 0;
      }
    }

    // ------------------------- Begin Rainbow Cycle -------------------------
    void RainbowCycle(uint8_t delay, direction dir = FORWARD) {
      ActivePattern = RAINBOW_CYCLE;
      Delay = delay;
      TotalSteps = 255;
      CurrentStep = 0;
      Direction = dir;
      FinalDelay = 0;
    }

    void RainbowCycleUpdate() {
      for (int i = 0; i < numPixels(); i++) {
        setPixelColor(i, Wheel(((i * 256 / numPixels()) + CurrentStep) & 255));
      }
      show();
      IncrementPattern();
    }
    // -------------------------- End Rainbow Cycle --------------------------

    // ------------------------- Begin Theater Chase -------------------------
    void TheaterChase(uint32_t color1, uint32_t color2, uint8_t delay, direction dir = FORWARD) {
      ActivePattern = THEATER_CHASE;
      Delay = delay;
      CurrentStep = 0;
      TotalSteps = numPixels();
      Color1 = color1;
      Color2 = color2;
      Direction = dir;
      FinalDelay = 0;
    }

    void TheaterChaseUpdate() {
      for(int i = 0; i < numPixels(); i++) {
        if ((i + CurrentStep) % 3 == 0) {
          setPixelColor(i, Color1);
        }
        else {
          setPixelColor(i, Color2);
        }
      }
      show();
      IncrementPattern();
    }
    // -------------------------- End Theater Chase --------------------------

    // -------------------------- Begin Color Wipe ---------------------------
    void ColorWipe(uint32_t color, uint8_t delay, direction dir = FORWARD) {
      ActivePattern = COLOR_WIPE;
      Delay = delay;
      CurrentStep = 0;
      TotalSteps = numPixels() * 2;
      Color1 = color;
      Direction = dir;
      FinalDelay = 0;
    }

    void ColorWipeUpdate() {
      if(CurrentStep >= numPixels()) {
        setPixelColor(CurrentStep - numPixels(), Color(0, 0, 0));
      }
      else {
        setPixelColor(CurrentStep, Color1);
      }
      show();
      IncrementPattern();
    }
    // --------------------------- End Color Wipe ----------------------------

    // ---------------------------- Begin Scanner ----------------------------
    void Scanner(uint32_t color, uint8_t delay) {
      ActivePattern = SCANNER;
      Delay = delay;
      CurrentStep = 0;
      TotalSteps = numPixels();
      Color1 = color;
      FinalDelay = 0;
    }

    void ScannerUpdate() {
      int8_t dir_modifier = 1;
      if(Direction == REVERSE) {
        dir_modifier = -1;
      }

      clear();
      for(int i = 0; i < numPixels(); i++) {
        int8_t pixel = CurrentStep;
        int8_t pixelb1 = (CurrentStep - (1 * dir_modifier)) % numPixels();
        int8_t pixelb2 = (CurrentStep - (2 * dir_modifier)) % numPixels();
        int8_t pixelb3 = (CurrentStep - (3 * dir_modifier)) % numPixels();

        // Clamp the pixels to wrap around
        if (pixelb1 < 0) { pixelb1 += numPixels(); }
        if (pixelb2 < 0) { pixelb2 += numPixels(); }
        if (pixelb3 < 0) { pixelb3 += numPixels(); }

        // Scan pixel
        if(i == pixel) {
          setPixelColor(i, Color1);
        }
        else if (i == pixelb1) {
          // Scan pixel one behind it
          setPixelColor(i, DimColor(Color1));
        }
        else if (i == pixelb2) {
          // Fading trail
          setPixelColor(i, DimColor(Color1, 2));
        }
        else if (i == pixelb3) {
          // Fading trail
          setPixelColor(i, DimColor(Color1, 4));
        }
      }
      show();
      IncrementPattern();
    }
    // ----------------------------- End Scanner -----------------------------

    // ----------------------------- Begin Fade ------------------------------
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t delay, direction dir = FORWARD) {
      ActivePattern = FADE;
      Delay = delay;
      CurrentStep = 0;
      TotalSteps = steps;
      Color1 = color1;
      Color2 = color2;
      Direction = dir;
      FinalDelay = 0;
    }

    void FadeUpdate() {
      // Calculate the fade color bettwen color 1 and color 2
      // Optimization from adafruit to minimize truncation errors
      uint8_t red = ((Red(Color1) * (TotalSteps - CurrentStep)) + (Red(Color2) * CurrentStep)) / TotalSteps;
      uint8_t green = ((Green(Color1) * (TotalSteps - CurrentStep)) + (Green(Color2) * CurrentStep)) / TotalSteps;
      uint8_t blue = ((Blue(Color1) * (TotalSteps - CurrentStep)) + (Blue(Color2) * CurrentStep)) / TotalSteps;

      SetAllPixelsToColor(Color(red, green, blue));
      show();
      IncrementPattern();
    }
    // ------------------------------ End Fade -------------------------------

    // ----------------------------- Begin Fade ------------------------------
    void RGBFade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t delay, direction dir = FORWARD) {
      ActivePattern = RGB_FADE;
      Delay = delay;
      CurrentStep = 0;
      TotalSteps = steps;
      Color1 = color1;
      Color2 = color2;
      Direction = dir;
      FinalDelay = 0;
    }

    void RGBFadeUpdate() {
      // Calculate the fade color bettwen color 1 and color 2
      // Optimization from adafruit to minimize truncation errors
      uint8_t red = ((Red(Color1) * (TotalSteps - CurrentStep)) + (Red(Color2) * CurrentStep)) / TotalSteps;
      uint8_t green = ((Green(Color1) * (TotalSteps - CurrentStep)) + (Green(Color2) * CurrentStep)) / TotalSteps;
      uint8_t blue = ((Blue(Color1) * (TotalSteps - CurrentStep)) + (Blue(Color2) * CurrentStep)) / TotalSteps;

      SetAllPixelsToColor(Color(red, green, blue));
      show();
      IncrementPattern();
    }
    // ------------------------------ End Fade -------------------------------

    // ------------------------ Begin Wizard Sparkles ------------------------
    const uint8_t SparkleArray[8] = {0,4,7,3,1,5,2,6};

    void Sparkle(uint32_t color, uint8_t delay, direction dir = FORWARD) {
      ActivePattern = SPARKLE;
      Delay = delay;
      CurrentStep = 0;
      TotalSteps = 8;
      Color1 = color;
      Direction = dir;
      FinalDelay = 0;
    }
    
    void SparkleUpdate() {
      clear();
      // Last one will be cleared to support in-between pattern delays
      if (CurrentStep < sizeof(SparkleArray)) {
        setPixelColor(SparkleArray[CurrentStep], Color1);
      }
      show();
      IncrementPattern();
    }

    void SparkleRandom(uint8_t delay, direction dir = FORWARD) {
      Sparkle(0, delay, dir);
      ActivePattern = SPARKLE_RANDOM;
    }

    void SparkleRandomUpdate() {
      byte r = random(255);
      byte g = random(255);
      byte b = random(255);
      Color1 = Color(r, g, b);
      SparkleUpdate();
    }
    // ------------------------- End Wizard Sparkles -------------------------

    // ------------------------- Begin Rainbow Bloom -------------------------
    // Pattern is [5] => [2, 3, 4, 6, 7] => [0, 1]

    void RainbowBloom(uint8_t delay, direction dir = FORWARD) {
      ActivePattern = RAINBOW_BLOOM;
      Delay = delay;
      CurrentStep = 0;
      TotalSteps = 255;
      Color1 = Color(0xFF, 0, 0);
      Direction = dir;
      FinalDelay = 0;
    }

    void RainbowBloomUpdate() {
      int8_t direction_multiplier = 1;
      const uint8_t incrementAmount = 16;

      if(Direction == REVERSE) {
        direction_multiplier = -1;
      }

      // Set the middle pixel
      setPixelColor(0, Wheel((CurrentStep) % 255));

      // Set the explosion ring
      setPixelColor(1, Wheel((CurrentStep + (incrementAmount * direction_multiplier)) % 255));
      setPixelColor(3, Wheel((CurrentStep + (incrementAmount * direction_multiplier)) % 255));
      setPixelColor(5, Wheel((CurrentStep + (incrementAmount * direction_multiplier)) % 255));
      setPixelColor(7, Wheel((CurrentStep + (incrementAmount * direction_multiplier)) % 255));

      // Set the outer ring
      setPixelColor(2, Wheel((CurrentStep + (incrementAmount * 2 * direction_multiplier)) % 255));
      setPixelColor(4, Wheel((CurrentStep + (incrementAmount * 2 * direction_multiplier)) % 255));
      setPixelColor(6, Wheel((CurrentStep + (incrementAmount * 2 * direction_multiplier)) % 255));

      show();
      IncrementPattern(incrementAmount);
    }

    // -------------------------- End Rainbow Bloom --------------------------

    // ------------------------- Begin Police Sirens -------------------------
    const uint8_t PoPoArray1[4] = {0, 7, 4, 5};
    const uint8_t PoPoArray2[4] = {2, 3, 6, 1};

    void OhNoPoPo(uint32_t color1, uint32_t color2, uint8_t delay, direction dir = FORWARD) {
      ActivePattern = OH_NO_POPO;
      Delay = delay;
      CurrentStep = 0;
      TotalSteps = 6;
      Color1 = color1;
      Color2 = color2;
      Direction = dir;
      FinalDelay = 0;
    }

    void OhNoPoPoUpdate() {
      clear();
      setPixelColor(PoPoArray1[CurrentStep], Color1);
      setPixelColor(PoPoArray2[CurrentStep], Color2);
      show();
      IncrementPattern();
    }

    // -------------------------- End Police Sirens --------------------------

    // ------------------------- Begin Color Stripes -------------------------
    const uint8_t ColorStripe1[3] = {7, 0, 6};
    const uint8_t ColorStripe2[2] = {1, 5};
    const uint8_t ColorStripe3[3] = {2, 4, 3};

    void ColorStripes(uint32_t color1, uint32_t color2, uint32_t color3, uint8_t delay, direction dir = FORWARD) {
      ActivePattern = COLOR_STRIPES;
      Delay = delay;
      CurrentStep = 0;
      TotalSteps = 3;
      Color1 = color1;
      Color2 = color2;
      Color3 = color3;
      Direction = dir;
      FinalDelay = 0;
    }

    void ColorStripesUpdate() {
      clear();

      uint32_t col1 = Color1;
      uint32_t col2 = Color2;
      uint32_t col3 = Color3;

      if (CurrentStep == 1) {
        col1 = Color3;
        col2 = Color1;
        col3 = Color2;
      }
      else if(CurrentStep == 2) {
        col1 = Color2;
        col2 = Color3;
        col3 = Color1;
      }

      for(int i = 0; i < sizeof(ColorStripe1); i++) {
        setPixelColor(ColorStripe1[i], col1);
      }

      for(int i = 0; i < sizeof(ColorStripe2); i++) {
        setPixelColor(ColorStripe2[i], col2);
      }

      for(int i = 0; i < sizeof(ColorStripe3); i++) {
        setPixelColor(ColorStripe3[i], col3);
      }

      show();
      IncrementPattern();
    }

    // -------------------------- End Color Stripes --------------------------

    // --------------------------- Begin Utilities ---------------------------
    // Calculate 50% dimmed version of a color (for Scanner)
    // Will take 8 shifts from pure white to pure black
    uint32_t DimColor(uint32_t color, uint8_t amount = 1) {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> amount, Green(color) >> amount, Blue(color) >> amount);
        return dimColor;
    }

    // Set all pixels to the given color
    void SetAllPixelsToColor(uint32_t color) {
      for(int i = 0; i < numPixels(); i++) {
        setPixelColor(i, color);
      }
      show();
    }

    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }

    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }

    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }

    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos) {
      WheelPos = 255 - WheelPos;
      if(WheelPos < 85) {
        return Color(255 - WheelPos * 3, 0, WheelPos * 3);
      }
      if(WheelPos < 170) {
        WheelPos -= 85;
        return Color(0, WheelPos * 3, 255 - WheelPos * 3);
      }
      WheelPos -= 170;
      return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
};