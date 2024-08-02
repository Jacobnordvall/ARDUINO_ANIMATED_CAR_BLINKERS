#include <Arduino.h>
#include <FastLED.h>

// Input pins (Pins to trigger..)
#define INDICATOR_PIN 3
#define LOWBRAKE_PIN 2    
#define HIGHBRAKE_PIN 3

// Output pins (Ledstrip data out)
#define INDICATOR_LED_PIN 6
#define BRAKE_LED_PIN 5

// Led counts (Don't forget to tune the animations for your led count)
#define INDICATOR_NUM_LEDS 25
#define BRAKE_NUM_LEDS 58

// Led strip config
#define INDICATOR_LED_TYPE WS2812B // What led strip are you using?
#define INDICATOR_COLOR_ORDER GRB // LED order on your strip
#define BRAKE_LED_TYPE WS2812B   // What led strip are you using?
#define BRAKE_COLOR_ORDER GRB   // LED order on your strip

// Brightness levels
#define BRIGHTNESS_INDICATORS 255  // 1-255
#define BRIGHTNESS_BRAKE_DRL 64   // 1-255
#define BRIGHTNESS_BRAKE_MAX 255 // 1-255

// Indicators anim config
const uint8_t min_cycles = 3; // Minimum number of animation cycles per click
const uint32_t lighting_up_delay = 10; // Delay in milliseconds between lighting up each LED
const uint32_t hold_after_lit_duration = 150; // Duration for which the strip should be held fully lit
const uint8_t fade_speed = 10; // Speed of fading after being fully lit and held
const uint32_t hold_after_fade_duration = 200; // Duration for which the strip should be held dark after fading to black
const uint8_t indicator_logic_delay = 5; // Time between each ledstrip update in ms (lower is faster, but also heavier)

// Brake anim config
#define FILL_SPEED 1   // Time in ms between lighting up each LED (1-20)
#define FLASH_SPEED 200 // Time in ms for flashing in high brake state
const uint8_t brake_logic_delay = 5; // Time between each brake ledstrip update in ms

// Colors
const CRGB indicator_color = CRGB(255, 100, 0); // You want the amber color. Led strips are inaccurate, so tune it by eye.
const CRGB brake_color = CRGB(255, 0, 0); // Brakes are red indeed.

// CONFIG END ==============================================================================================================


// LED arrays
CRGB indicator_leds[INDICATOR_NUM_LEDS];
CRGB brake_leds[BRAKE_NUM_LEDS];

enum INDICATOR_STATE : uint8_t 
{
    INDICATOR_IDLE,
    INDICATOR_LIGHTING_UP,
    INDICATOR_HOLDING,
    INDICATOR_FADING,
    INDICATOR_HOLDING_AFTER_FADE
};

enum BRAKE_STATE : uint8_t
{
    BRAKE_DRL_MODE,
    BRAKE_FILLING,
    BRAKE_HOLDING,
    BRAKE_HIGHBRAKE_FLASHING
};

// Indicator variables
INDICATOR_STATE indicatorState = INDICATOR_IDLE;
uint8_t current_led = 0;
uint32_t last_update = 0;
uint32_t hold_start = 0;
uint32_t hold_start_after_fade = 0;
uint32_t lighting_up_start = 0; 
bool button_held = false;
int animation_cycles = 0;
bool reset_called = false;
uint8_t fade_brightness = BRIGHTNESS_INDICATORS; // Separate brightness for indicators

// Brake variables
BRAKE_STATE brakeState = BRAKE_DRL_MODE;
uint8_t currentLedIndex = 0;
uint32_t lastUpdate = 0;
uint32_t flashLastUpdate = 0;
bool flashState = false;

const float gamma = 2.2; // Gamma correction factor (typically between 2.2 and 2.8 for LEDs)
inline uint8_t gammaCorrection(uint8_t value) 
{ return (uint8_t)(255.0 * pow((value / 255.0), gamma)); }

// Adjust the brightness of a color
inline CRGB adjustBrightness(const CRGB& color, uint8_t brightness) 
{ return CRGB(color.r * brightness / 255, color.g * brightness / 255, color.b * brightness / 255); }

// Indicators ================================================================
void resetAnimation() 
{
    current_led = 0;
    last_update = 0;
    hold_start = 0;
    hold_start_after_fade = 0;
    lighting_up_start = 0;
    indicatorState = INDICATOR_IDLE;
    fade_brightness = BRIGHTNESS_INDICATORS;
    fill_solid(indicator_leds, INDICATOR_NUM_LEDS, CRGB::Black);
    FastLED.show();
    reset_called = true;
    Serial.println(F("Animation reset"));
}

void startAnimation() 
{
    if (indicatorState != INDICATOR_IDLE) return; // Prevent restarting if animation is already running

    current_led = 0;
    indicatorState = INDICATOR_LIGHTING_UP;
    animation_cycles = 0;
    reset_called = false;
    fill_solid(indicator_leds, INDICATOR_NUM_LEDS, CRGB::Black);
    FastLED.show();
    lighting_up_start = millis();
    Serial.println(F("Animation started"));
}

void runIndicatorAnimation() 
{
    uint32_t now = millis();

    if (now - last_update > indicator_logic_delay) 
    {
        last_update = now;

        switch (indicatorState) 
        {
            case INDICATOR_LIGHTING_UP:
                if (now - lighting_up_start >= lighting_up_delay) 
                {
                    indicator_leds[current_led] = adjustBrightness(indicator_color, BRIGHTNESS_INDICATORS);
                    FastLED.show();
                    lighting_up_start = now; // Reset the start time

                    current_led++;
                    if (current_led >= INDICATOR_NUM_LEDS) 
                    {
                        indicatorState = INDICATOR_HOLDING;
                        hold_start = now;
                        Serial.println(F("All LEDs lit. Starting hold..."));
                    }
                }
                break;

            case INDICATOR_HOLDING:
                if (now - hold_start >= hold_after_lit_duration) 
                {
                    indicatorState = INDICATOR_FADING;
                    fade_brightness = BRIGHTNESS_INDICATORS;
                    Serial.println(F("Hold complete. Starting fade..."));
                }
                break;

            case INDICATOR_FADING:
                if (fade_brightness > 0) 
                {
                    fade_brightness = max(0, fade_brightness - fade_speed);
                    uint8_t corrected_brightness = gammaCorrection(fade_brightness);
                    for (int i = 0; i < INDICATOR_NUM_LEDS; i++)
                    { indicator_leds[i] = adjustBrightness(indicator_color, corrected_brightness); }
                    FastLED.show();
                    Serial.print(F("Fading... Brightness: "));
                    Serial.println(corrected_brightness);
                } 
                else 
                {
                    indicatorState = INDICATOR_HOLDING_AFTER_FADE;
                    hold_start_after_fade = now;
                    fill_solid(indicator_leds, INDICATOR_NUM_LEDS, CRGB::Black); // Clear the strip after fading
                    FastLED.show(); // Ensure LEDs are updated
                    Serial.println(F("Fade complete. Starting hold after fade..."));
                }
                break;

            case INDICATOR_HOLDING_AFTER_FADE:
                if (now - hold_start_after_fade >= hold_after_fade_duration) 
                {
                    animation_cycles++;
                    if (animation_cycles < min_cycles || button_held) 
                    {
                        // Continue animation if button is held or if minimum cycles not reached
                        indicatorState = INDICATOR_LIGHTING_UP;
                        current_led = 0; // Reset current_led for the next cycle
                        fill_solid(indicator_leds, INDICATOR_NUM_LEDS, CRGB::Black); // Clear the strip before starting the new cycle
                        lighting_up_start = now; // Reset the start time
                        Serial.println(F("Hold after fade complete. Starting new cycle..."));
                    } 
                    else 
                    {
                        // If minimum cycles reached and button is not held, stop animation
                        indicatorState = INDICATOR_IDLE;
                        Serial.println(F("Desired cycles reached. Animation complete."));
                    }
                }
                break;

            default:
                break;
        }
    }
}

// Brake =====================================================================
void handleLowBrake() 
{
    brakeState = BRAKE_FILLING;
    currentLedIndex = 0;
    lastUpdate = millis();
    Serial.println(F("Entering FILLING state"));
}

void handleHighBrake() 
{
    brakeState = BRAKE_HIGHBRAKE_FLASHING;
    flashState = false;
    flashLastUpdate = millis();
    Serial.println(F("Entering HIGHBRAKE_FLASHING state"));
}

void resetToDRLMode() 
{
    brakeState = BRAKE_DRL_MODE;
    fill_solid(brake_leds, BRAKE_NUM_LEDS, adjustBrightness(brake_color, BRIGHTNESS_BRAKE_DRL));
    FastLED.show();
    Serial.println(F("Reverting to DRL_MODE"));
}

void runBrakeAnimation() 
{
    uint32_t now = millis();

    switch (brakeState) 
    {
    case BRAKE_FILLING:
        // Use FILL_SPEED for timing each LED lighting
        if (now - lastUpdate >= FILL_SPEED) 
        {
            lastUpdate = now; // Update lastUpdate to the current time
            if (currentLedIndex <= BRAKE_NUM_LEDS / 2) 
            {
                int mid = BRAKE_NUM_LEDS / 2;

                // Update the edges
                brake_leds[mid - currentLedIndex] = adjustBrightness(brake_color, BRIGHTNESS_BRAKE_MAX);
                brake_leds[mid + currentLedIndex] = adjustBrightness(brake_color, BRIGHTNESS_BRAKE_MAX);

                // Fill in between the edges with DRL brightness
                for (int i = 0; i < BRAKE_NUM_LEDS; i++) 
                {
                    if (i < mid - currentLedIndex || i > mid + currentLedIndex) 
                    {
                        brake_leds[i] = adjustBrightness(brake_color, BRIGHTNESS_BRAKE_DRL);
                    }
                }

                FastLED.show();
                currentLedIndex++;
            } 
            else 
            {
                // Transition to HOLDING state once all LEDs are lit
                brakeState = BRAKE_HOLDING;
                currentLedIndex = 0; // Reset for potential future use
                Serial.println(F("Entering HOLDING state"));
            }
        }
        break;

    case BRAKE_HIGHBRAKE_FLASHING:
        if (now - flashLastUpdate >= FLASH_SPEED) 
        {
            flashLastUpdate = now;
            flashState = !flashState;
            if (flashState) 
            { fill_solid(brake_leds, BRAKE_NUM_LEDS, brake_color); } 
            else 
            { fill_solid(brake_leds, BRAKE_NUM_LEDS, CRGB::Black); }

            FastLED.show();
            Serial.println(F("Flashing in HIGHBRAKE_FLASHING state"));
        }
        break;

    default:
        break;
    }
}


// Setup and loop ============================================================
void setup() 
{
    pinMode(INDICATOR_PIN, INPUT_PULLUP);
    pinMode(LOWBRAKE_PIN, INPUT_PULLUP);
    pinMode(HIGHBRAKE_PIN, INPUT_PULLUP);
    Serial.begin(9600);

    // Initialize indicator LED strip
    FastLED.addLeds<INDICATOR_LED_TYPE, INDICATOR_LED_PIN, INDICATOR_COLOR_ORDER>(indicator_leds, INDICATOR_NUM_LEDS).setCorrection(TypicalLEDStrip);
    fill_solid(indicator_leds, INDICATOR_NUM_LEDS, CRGB::Black); // Ensure all LEDs are off initially
    FastLED.show();

    // Initialize brake LED strip
    FastLED.addLeds<BRAKE_LED_TYPE, BRAKE_LED_PIN, BRAKE_COLOR_ORDER>(brake_leds, BRAKE_NUM_LEDS).setCorrection(TypicalLEDStrip);
    fill_solid(brake_leds, BRAKE_NUM_LEDS, adjustBrightness(brake_color, BRIGHTNESS_BRAKE_DRL)); // Set initial DRL brightness
    FastLED.show();
}

void loop() 
{
    // Indicators =============================================

    static bool previous_state = HIGH; // Stores the previous state of the indicator button
    bool current_state = digitalRead(INDICATOR_PIN); // Reads the current state of the indicator button

    if (previous_state == HIGH && current_state == LOW) 
    {
        startAnimation();
        button_held = true;
        Serial.println(F("Input Start"));
    }

    if (previous_state == LOW && current_state == HIGH) 
    {
        button_held = false;
        Serial.println(F("Input Stop"));
    }

    previous_state = current_state;

    if (indicatorState != INDICATOR_IDLE) 
    { runIndicatorAnimation(); } 
    else if (!reset_called) 
    {
        resetAnimation();
        Serial.println(F("Finished and reset"));
    }

    // Brake ==================================================
    
    bool lowBrakePressed = !digitalRead(LOWBRAKE_PIN);
    bool highBrakePressed = !digitalRead(HIGHBRAKE_PIN);

    if (brakeState == BRAKE_DRL_MODE)
    {
        if (lowBrakePressed)
        { handleLowBrake(); }
    }
    else if (brakeState == BRAKE_HOLDING)
    {
        if (highBrakePressed)
        { handleHighBrake(); }
        else if (!lowBrakePressed)
        { resetToDRLMode(); }
    }
    else if (brakeState == BRAKE_HIGHBRAKE_FLASHING && !lowBrakePressed && !highBrakePressed)
    { resetToDRLMode(); }

    if (brakeState != BRAKE_DRL_MODE)
    { runBrakeAnimation(); }
}
