#include <Arduino.h>
#include <FastLED.h>

// MCU configuration
#define LED_PIN 6 // This is the pin that you connect to your ledstrips data in.
#define NUM_LEDS 25 // How many leds are your strip? (Dont forget to tune the animation for your led count)
#define BRIGHTNESS 255 // 1-255 How bright should it light up.
#define INDICATOR_PIN 2 // Input pin for the indicator signal or button.
#define LED_TYPE WS2812B // What led strip are you using?
#define COLOR_ORDER GRB // LED order on your strip
CRGB leds[NUM_LEDS];

// Animation configuration
const int min_cycles = 3; // Minimum number of animation cycles per click
const uint32_t hold_duration = 150; // Duration for which the strip should be held fully lit
const uint32_t hold_after_fade_duration = 200; // Duration for which the strip should be held dark after fading to black
const uint8_t fade_speed = 10; // Speed of fading after being fully lit and held
const uint32_t lighting_up_delay = 10; // Delay in milliseconds between lighting up each LED
const uint8_t animation_logic_delay = 5; // time between each ledstrip update in ms (lower is faster, but also heavier)

// Define the color for the animation
CRGB animationColor = CRGB(255, 100, 0); // You want the amber color. led strips are inacurate as f**k so tune it bye eye.

enum AnimationState 
{
    IDLE,
    LIGHTING_UP,
    HOLDING,
    FADING,
    HOLDING_AFTER_FADE
};

AnimationState state = IDLE;
uint8_t current_led = 0;
uint32_t last_update = 0;
uint32_t hold_start = 0;
uint32_t hold_start_after_fade = 0;
uint32_t lighting_up_start = 0; // Start time for lighting up phase
bool button_held = false;
int animation_cycles = 0;
bool reset_called = false;
uint8_t fade_brightness = BRIGHTNESS;

const float gamma = 2.2; // Gamma correction factor (typically between 2.2 and 2.8 for LEDs)
uint8_t gammaCorrection(uint8_t value) 
{ return (uint8_t)(255.0 * pow((value / 255.0), gamma)); }

void resetAnimation() 
{
    current_led = 0;
    last_update = 0;
    hold_start = 0;
    hold_start_after_fade = 0;
    lighting_up_start = 0;
    state = IDLE;
    fade_brightness = BRIGHTNESS;
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
    reset_called = true;
    Serial.println("Animation reset");
}

void startAnimation() 
{
    if (state != IDLE) return; // Prevent restarting if animation is already running

    current_led = 0;
    state = LIGHTING_UP;
    animation_cycles = 0;
    reset_called = false;
    FastLED.clear();
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.show();
    lighting_up_start = millis();
    Serial.println("Animation started");
}

void runIndicatorAnimation() 
{
    uint32_t now = millis();

    if (now - last_update > animation_logic_delay) 
    {
        last_update = now;

        switch (state) 
        {
            case LIGHTING_UP:
                if (now - lighting_up_start >= lighting_up_delay) 
                {
                    leds[current_led] = animationColor;
                    FastLED.show();
                    lighting_up_start = now; // Reset the start time

                    current_led++;
                    if (current_led >= NUM_LEDS) 
                    {
                        state = HOLDING;
                        hold_start = now;
                        Serial.println("All LEDs lit. Starting hold...");
                    }
                }
                break;

            case HOLDING:
                if (now - hold_start >= hold_duration) 
                {
                    state = FADING;
                    fade_brightness = BRIGHTNESS;
                    Serial.println("Hold complete. Starting fade...");
                }
                break;

            case FADING:
                if (fade_brightness > 0) 
                {
                    fade_brightness = max(0, fade_brightness - fade_speed);
                    uint8_t corrected_brightness = gammaCorrection(fade_brightness);
                    FastLED.setBrightness(corrected_brightness);
                    FastLED.show();
                    Serial.println("Fading... Brightness: " + String(corrected_brightness));
                } 
                else 
                {
                    state = HOLDING_AFTER_FADE;
                    hold_start_after_fade = now;
                    FastLED.setBrightness(BRIGHTNESS);
                    FastLED.clear(); // Clear the strip after fading
                    FastLED.show(); // Ensure LEDs are updated
                    Serial.println("Fade complete. Starting hold after fade...");
                }
                break;

            case HOLDING_AFTER_FADE:
                if (now - hold_start_after_fade >= hold_after_fade_duration) 
                {
                    animation_cycles++;
                    if (animation_cycles < min_cycles || button_held) 
                    {
                        // Continue animation if button is held or if minimum cycles not reached
                        state = LIGHTING_UP;
                        current_led = 0; // Reset current_led for the next cycle
                        FastLED.clear(); // Clear the strip before starting the new cycle
                        lighting_up_start = now; // Reset the start time
                        Serial.println("Hold after fade complete. Starting new cycle...");
                    } 
                    else 
                    {
                        // If minimum cycles reached and button is not held, stop animation
                        state = IDLE;
                        Serial.println("Desired cycles reached. Animation complete.");
                    }
                }
                break;

            default:
                break;
        }
    }
}

void setup() 
{
    pinMode(INDICATOR_PIN, INPUT_PULLUP);
    Serial.begin(9600);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
}

void loop() 
{
    static bool previous_state = HIGH;
    bool current_state = digitalRead(INDICATOR_PIN);

    if (previous_state == HIGH && current_state == LOW) 
    {
        startAnimation();
        button_held = true;
        Serial.println("Input Start");
    }

    if (previous_state == LOW && current_state == HIGH) 
    {
        button_held = false;
        Serial.println("Input Stop");
    }

    previous_state = current_state;

    if (state != IDLE) 
    { runIndicatorAnimation(); } 
    else if (!reset_called) 
    {
        resetAnimation();
        Serial.println("Finished and reset");
    }
}
