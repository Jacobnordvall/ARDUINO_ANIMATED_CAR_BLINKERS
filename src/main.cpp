#include <Arduino.h>
#include <FastLED.h>

//?     ___     _   __ ____ __  ___ ___   ______ ______ ____        ______ ___     ____         __     ____ ______ __  __ ______ ____ _   __ ______
//?    /   |   / | / //  _//  |/  //   | /_  __// ____// __ \      / ____//   |   / __ \       / /    /  _// ____// / / //_  __//  _// | / // ____/
//?   / /| |  /  |/ / / / / /|_/ // /| |  / /  / __/  / / / /     / /    / /| |  / /_/ /      / /     / / / / __ / /_/ /  / /   / / /  |/ // / __  
//?  / ___ | / /|  /_/ / / /  / // ___ | / /  / /___ / /_/ /     / /___ / ___ | / _, _/      / /___ _/ / / /_/ // __  /  / /  _/ / / /|  // /_/ /  
//? /_/ _|_|/_/ |_//___//_/  /_//_/  |_|/_/  /_____//_____/______\____//_/  |_|/_/_|_|______/_____//___/ \____//_/ /_/ _/_/_ /___//_/ |_/ \____/                                                  
//?  _                      _____                     _           __                  _               _  _                                
//? | |__   _   _           \__  \  __ _   ___  ___  | |__     /\ \ \  ___   _ __  __| |__   __ __ _ | || |                               
//? | '_ \ | | | |             \ \ / _` | / __|/ _ \ | '_ \   /  \/ / / _ \ | '__|/ _` |\ \ / // _` || || |                               
//? | |_) || |_| |          /\_/ /| (_| || (__||(_)| | |_)|  |/ /\ /  |(_)| | |  | (_| | \ V /| (_| || || |                               
//? |_.__/  \__, |          \___/  \__,_| \___|\___/ |_.__/  \_\ \/   \___/ |_|   \__,_|  \_/  \__,_||_||_|                               
//?         |___/                                                                                                                        
//?                                                                                        

//* Input pins (Pins to trigger..)
#define INDICATOR_PIN 7
#define LOWBRAKE_PIN 2    
#define HIGHBRAKE_PIN 3
#define REVERSE_PIN 4

//* Output pins (Ledstrip data out)
#define INDICATOR_LED_PIN 9
#define BRAKE_LED_PIN 5
#define FRONTDRL_LED_PIN 6
#define REVERSE_LED_PIN 8

//* Led counts (Don't forget to tune the animations for your led count)
#define INDICATOR_NUM_LEDS 25  // Can be whatever
#define BRAKE_NUM_LEDS 58     // Preferably uneven for a centered anim. 
#define FRONTDRL_NUM_LEDS 25 // Can be whatever
#define REVERSE_NUM_LEDS 15 // Should be uneven.

//* Led strip config
#define INDICATOR_LED_TYPE WS2812B // What led strip are you using?
#define INDICATOR_COLOR_ORDER GRB // LED order on your strip
#define BRAKE_LED_TYPE WS2812B
#define BRAKE_COLOR_ORDER GRB
#define FRONTDRL_LED_TYPE WS2812B
#define FRONTDRL_COLOR_ORDER GRB
#define REVERSE_LED_TYPE WS2812B
#define REVERSE_COLOR_ORDER GRB

//* Brightness levels
#define BRIGHTNESS_INDICATORS 255  // 1-255
#define BRIGHTNESS_BRAKE_DRL 64   // 1-255
#define BRIGHTNESS_BRAKE_MAX 255 // 1-255
#define BRIGHTNESS_FRONTDRL 255 // 1-255
#define BRIGHTNESS_REVERSE 255 // 1-255 

//* Indicators anim config
const uint8_t min_cycles = 3; // Minimum number of animation cycles per click
const uint8_t lighting_up_delay = 10; // Delay in ms between lighting up each LED
const uint16_t hold_after_lit_duration = 150; // Duration for which the strip should be held fully lit
const uint8_t fade_speed = 10; // Speed of fading after being fully lit and held
const uint16_t hold_after_fade_duration = 200; // Duration for which the strip should be held dark after fading to black
const uint8_t indicator_logic_delay = 5; // Time between each ledstrip update in ms (5ms = 200fps)

//* Brake anim config
#define FILL_SPEED 1   // Time in ms between lighting up each LED (1-20)
#define FLASH_SPEED 200 // Time in ms for flashing in high brake state
const uint8_t brake_logic_delay = 5; // Time between each brake ledstrip update in ms (5ms = 200fps)

//* Reverse anim config
const uint8_t reverse_animation_delay = 15;  // Time in ms between each LED in animation
const uint8_t reverse_logic_delay = 5;  // Time between each ledstrip update in ms  (5ms = 200fps)

//* Colors
const CRGB indicator_color = CRGB(255, 100, 0); // You want the amber color. Led strips are inaccurate, so tune it by eye.
const CRGB brake_color = CRGB(255, 0, 0);      // Brakes are red indeed.
const CRGB frontDRL_color = CRGB::White;      // Color for the front DRL strip
const CRGB reverse_color = CRGB::White;      // Color for the reverse light

//* Bootup anim configs
#define ENABLE_BRAKE_BOOTUP true // Set to true to enable bootup animation, false to disable
#define BRAKE_BOOTUP_SPEED 15 // Delay to make it run slower (ms)
#define BRAKE_NUM_TRAVELING_LEDS 3 // Set the number of LEDs traveling on each side
#define ENABLE_FRONTDRL_BOOTUP true // Set to true to enable bootup animation, false to disable
#define FRONTDRL_BOOTUP_SPEED 20 // Delay to make it run slower (ms)

//! CONFIG END ==============================================================================================================
//! =========================================================================================================================


//* State machines ===========================================================================================================
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
    BRAKE_BOOTUP_STAGE_1,
    BRAKE_BOOTUP_STAGE_2,
    BRAKE_BOOTUP_STAGE_3,
    BRAKE_DRL_MODE,
    BRAKE_FILLING,
    BRAKE_HOLDING,
    BRAKE_HIGHBRAKE_FLASHING
};

enum FRONTDRL_STATE : uint8_t 
{
    FRONTDRL_BOOTUP_STAGE_1,
    FRONTDRL_BOOTUP_STAGE_2,
    FRONTDRL_ON
};

enum REVERSE_STATE : uint8_t 
{
    REVERSE_IDLE,
    REVERSE_LIGHTING_UP,
    REVERSE_HOLDING,
    REVERSE_TURNING_OFF
};

//* Non config variables ======================================================
// LED arrays
CRGB indicator_leds[INDICATOR_NUM_LEDS];
CRGB brake_leds[BRAKE_NUM_LEDS];
CRGB frontDRL_leds[FRONTDRL_NUM_LEDS];
CRGB reverse_leds[REVERSE_NUM_LEDS];

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
uint8_t bootup_led_index = 0;
uint8_t bootup_stage = 1;
uint32_t bootup_lastUpdate = 0;

// Front DRL variables
FRONTDRL_STATE frontDRLState = FRONTDRL_BOOTUP_STAGE_1;
uint8_t frontDRLCurrentLedIndex = 0;
uint32_t frontDRLLastUpdate = 0;
uint8_t frontDRLBootupStage = 1;  // 1 for first half brightness, 2 for full brightness

// Reverse light variables
REVERSE_STATE reverseState = REVERSE_IDLE;
uint8_t current_reverse_led = 0;
uint32_t reverse_last_update = 0;
uint32_t reverse_lighting_up_start = 0;
bool reverse_input_active = false;

//* Helper functions ==========================================================
const float gamma = 2.2; // Gamma correction factor (typically between 2.2 and 2.8 for LEDs)
inline uint8_t gammaCorrection(uint8_t value) 
{ return (uint8_t)(255.0 * pow((value / 255.0), gamma)); }

// Adjust the brightness of a color
inline CRGB adjustBrightness(const CRGB& color, uint8_t brightness) 
{ return CRGB(color.r * brightness / 255, color.g * brightness / 255, color.b * brightness / 255); }

//* Indicators ================================================================
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

//* Brake =====================================================================
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

void runBrakeBootupAnimation() 
{
    uint32_t now = millis();

    // Stage 1: Pairs of LEDs moving towards the center from each side
    if (bootup_stage == 1) 
    {
        if (now - bootup_lastUpdate >= BRAKE_BOOTUP_SPEED) 
        {
            bootup_lastUpdate = now;
            fill_solid(brake_leds, BRAKE_NUM_LEDS, CRGB::Black); // Keep background black
            int mid = BRAKE_NUM_LEDS / 2;

            // Left side LEDs moving towards center
            for (int i = 0; i < BRAKE_NUM_TRAVELING_LEDS; i++) 
            {
                if (bootup_led_index + i < mid) 
                { brake_leds[bootup_led_index + i] = adjustBrightness(brake_color, BRIGHTNESS_BRAKE_DRL / 2); }
            }

            // Right side LEDs moving towards center
            for (int i = 0; i < BRAKE_NUM_TRAVELING_LEDS; i++) 
            {
                if (BRAKE_NUM_LEDS - 1 - bootup_led_index - i >= mid) 
                { brake_leds[BRAKE_NUM_LEDS - 1 - bootup_led_index - i] = adjustBrightness(brake_color, BRIGHTNESS_BRAKE_DRL / 2); }
            }

            FastLED.show();
            bootup_led_index++;

            // Check if the LEDs have reached the center
            if (bootup_led_index >= mid - BRAKE_NUM_TRAVELING_LEDS) 
            {
                bootup_stage = 2;
                bootup_led_index = 0;
                Serial.println(F("Transition to Bootup Stage 2"));
            }
        }
    } 
    // Stage 2: Fill outwards from the middle at half DRL brightness
    else if (bootup_stage == 2) 
    {
        if (now - bootup_lastUpdate >= BRAKE_BOOTUP_SPEED) 
        {
            bootup_lastUpdate = now;
            int mid = BRAKE_NUM_LEDS / 2;

            if (bootup_led_index <= mid) 
            {
                brake_leds[mid - bootup_led_index] = adjustBrightness(brake_color, BRIGHTNESS_BRAKE_DRL / 2);
                brake_leds[mid + bootup_led_index] = adjustBrightness(brake_color, BRIGHTNESS_BRAKE_DRL / 2);
                bootup_led_index++;
                FastLED.show();
            } 
            else 
            {
                bootup_stage = 3;
                bootup_led_index = 0;
                Serial.println(F("Transition to Bootup Stage 3"));
            }
        }
    } 
    // Stage 3: Fill outwards from the middle at full DRL brightness over the half brightness
    else if (bootup_stage == 3) 
    {
        if (now - bootup_lastUpdate >= BRAKE_BOOTUP_SPEED) 
        {
            bootup_lastUpdate = now;
            int mid = BRAKE_NUM_LEDS / 2;

            if (bootup_led_index <= mid) 
            {
                brake_leds[mid - bootup_led_index] = adjustBrightness(brake_color, BRIGHTNESS_BRAKE_DRL);
                brake_leds[mid + bootup_led_index] = adjustBrightness(brake_color, BRIGHTNESS_BRAKE_DRL);
                bootup_led_index++;
                FastLED.show();
            } 
            else 
            {
                // Animation complete, move to DRL mode
                brakeState = BRAKE_DRL_MODE;
                bootup_stage = 0; // Reset stage
                bootup_led_index = 0;
                Serial.println(F("Bootup animation complete. Entering DRL_MODE"));
            }
        }
    }
}

//* Front DRLs ================================================================
void runFrontDRLBootupAnimation() 
{
    uint32_t now = millis();

    if (frontDRLBootupStage == 1) 
    {
        if (now - frontDRLLastUpdate >= FRONTDRL_BOOTUP_SPEED) 
        {
            frontDRLLastUpdate = now;

            if (frontDRLCurrentLedIndex < FRONTDRL_NUM_LEDS) 
            {
                frontDRL_leds[FRONTDRL_NUM_LEDS - 1 - frontDRLCurrentLedIndex] = adjustBrightness(frontDRL_color, BRIGHTNESS_FRONTDRL / 2);
                FastLED.show();
                frontDRLCurrentLedIndex++;
            } 
            else 
            {
                frontDRLBootupStage = 2;
                frontDRLCurrentLedIndex = 0;
                Serial.println(F("Front DRL Bootup Transition to Stage 2"));
            }
        }
    } 
    else if (frontDRLBootupStage == 2) 
    {
        if (now - frontDRLLastUpdate >= FRONTDRL_BOOTUP_SPEED) 
        {
            frontDRLLastUpdate = now;

            if (frontDRLCurrentLedIndex < FRONTDRL_NUM_LEDS) 
            {
                frontDRL_leds[FRONTDRL_NUM_LEDS - 1 - frontDRLCurrentLedIndex] = adjustBrightness(frontDRL_color, BRIGHTNESS_FRONTDRL);
                FastLED.show();
                frontDRLCurrentLedIndex++;
            } 
            else 
            {
                frontDRLBootupStage = 0;
                frontDRLCurrentLedIndex = 0;
                frontDRLState = FRONTDRL_ON; // Correctly set the state to ON
                fill_solid(frontDRL_leds, FRONTDRL_NUM_LEDS, adjustBrightness(frontDRL_color, BRIGHTNESS_FRONTDRL)); // Ensure the strip stays lit at full brightness
                FastLED.show();
                Serial.println(F("Front DRL Bootup Animation Complete. Entering ON state"));
            }
        }
    }
}

//* Reverse light =============================================================
void startReverseAnimation() 
{
    if (reverseState != REVERSE_IDLE)
    return;

    reverseState = REVERSE_LIGHTING_UP;
    current_reverse_led = 0;
    fill_solid(reverse_leds, REVERSE_NUM_LEDS, CRGB::Black); // Ensure all LEDs are off initially
    FastLED.show();
    reverse_lighting_up_start = millis();
    Serial.println(F("Reverse Animation started"));
}

void runReverseAnimation() 
{
    uint32_t now = millis();

    if (now - reverse_last_update > reverse_logic_delay) 
    {
        reverse_last_update = now;

        switch (reverseState) 
        {
            case REVERSE_LIGHTING_UP:
                if (now - reverse_lighting_up_start >= reverse_animation_delay) 
                {
                    int mid = REVERSE_NUM_LEDS / 2;
                    if (current_reverse_led < mid) 
                    {
                        reverse_leds[mid - current_reverse_led] = adjustBrightness(reverse_color, BRIGHTNESS_REVERSE);
                        reverse_leds[mid + current_reverse_led] = adjustBrightness(reverse_color, BRIGHTNESS_REVERSE);
                        FastLED.show();
                        reverse_lighting_up_start = now; // Reset the start time
                        current_reverse_led++;
                    } 
                    else 
                    {
                        // Ensure the last LEDs are lit
                        reverse_leds[0] = adjustBrightness(reverse_color, BRIGHTNESS_REVERSE);
                        reverse_leds[REVERSE_NUM_LEDS - 1] = adjustBrightness(reverse_color, BRIGHTNESS_REVERSE);
                        FastLED.show();
                        reverseState = REVERSE_HOLDING;
                        Serial.println(F("All reverse LEDs lit. Holding..."));
                    }
                }
                break;

            case REVERSE_HOLDING:
                if (!reverse_input_active) 
                {
                    reverseState = REVERSE_TURNING_OFF;
                    current_reverse_led = 0;  // Reset for turning off
                    reverse_lighting_up_start = now; // Reset the start time
                    Serial.println(F("Reverse input stopped. Turning off..."));
                }
                break;

            case REVERSE_TURNING_OFF:
                if (now - reverse_lighting_up_start >= reverse_animation_delay) 
                {
                    int mid = REVERSE_NUM_LEDS / 2;
                    if (current_reverse_led < mid) 
                    {
                        reverse_leds[mid - current_reverse_led] = CRGB::Black;
                        reverse_leds[mid + current_reverse_led] = CRGB::Black;
                        FastLED.show();
                        reverse_lighting_up_start = now; // Reset the start time
                        current_reverse_led++;
                    } 
                    else 
                    {
                        // Ensure the last LEDs are turned off
                        reverse_leds[0] = CRGB::Black;
                        reverse_leds[REVERSE_NUM_LEDS - 1] = CRGB::Black;
                        FastLED.show();
                        reverseState = REVERSE_IDLE;
                        Serial.println(F("Reverse animation complete. All LEDs off."));
                    }
                }
                break;

            default:
                break;
        }
    }
}



//* Setup =====================================================================
void setup() 
{
    pinMode(INDICATOR_PIN, INPUT_PULLUP);
    pinMode(LOWBRAKE_PIN, INPUT_PULLUP);
    pinMode(HIGHBRAKE_PIN, INPUT_PULLUP);
    pinMode(REVERSE_PIN, INPUT_PULLUP);
    Serial.begin(9600);

    // Initialize indicator LED strip
    FastLED.addLeds<INDICATOR_LED_TYPE, INDICATOR_LED_PIN, INDICATOR_COLOR_ORDER>(indicator_leds, INDICATOR_NUM_LEDS).setCorrection(TypicalLEDStrip);
    fill_solid(indicator_leds, INDICATOR_NUM_LEDS, CRGB::Black); // Ensure all LEDs are off initially
    FastLED.show();

    // Initialize brake LED strip
    FastLED.addLeds<BRAKE_LED_TYPE, BRAKE_LED_PIN, BRAKE_COLOR_ORDER>(brake_leds, BRAKE_NUM_LEDS).setCorrection(TypicalLEDStrip);

    if (ENABLE_BRAKE_BOOTUP) 
    {
        bootup_stage = 1;
        bootup_led_index = 0;
        bootup_lastUpdate = millis();
        Serial.println(F("Starting Brake Bootup Animation"));
    } 
    else 
    {
        fill_solid(brake_leds, BRAKE_NUM_LEDS, adjustBrightness(brake_color, BRIGHTNESS_BRAKE_DRL)); // Set initial DRL brightness
        FastLED.show();
        brakeState = BRAKE_DRL_MODE;
        Serial.println(F("Brake Bootup Animation Disabled. Entering DRL_MODE"));
    }

    // Initialize front DRL LED strip
    FastLED.addLeds<FRONTDRL_LED_TYPE, FRONTDRL_LED_PIN, FRONTDRL_COLOR_ORDER>(frontDRL_leds, FRONTDRL_NUM_LEDS).setCorrection(TypicalLEDStrip);

    if (ENABLE_FRONTDRL_BOOTUP)
    {
        frontDRLBootupStage = 1;
        frontDRLCurrentLedIndex = 0;
        frontDRLLastUpdate = millis();
        Serial.println(F("Starting Front DRL Bootup Animation"));
    }
    else
    {
        fill_solid(frontDRL_leds, FRONTDRL_NUM_LEDS, CRGB::White); // Set initial DRL brightness
        FastLED.show();
        frontDRLState = FRONTDRL_ON;
        Serial.println(F("Front DRL Bootup Animation Disabled. Entering ON state"));
    }

    // Initialize reverse light LED strip
    FastLED.addLeds<REVERSE_LED_TYPE, REVERSE_LED_PIN, REVERSE_COLOR_ORDER>(reverse_leds, REVERSE_NUM_LEDS).setCorrection(TypicalLEDStrip);
    fill_solid(reverse_leds, REVERSE_NUM_LEDS, CRGB::Black); // Ensure all LEDs are off initially
    FastLED.show();
}

//* Loop ======================================================================
bool isBootupComplete = false; // Flag to indicate bootup completion
void loop() 
{
    // Bootup Animation
    if (!isBootupComplete) 
    {
        // Run brake bootup animation if enabled and not yet complete
        if (ENABLE_BRAKE_BOOTUP && bootup_stage > 0) 
        { runBrakeBootupAnimation(); }

        // Run front DRL bootup animation if enabled and not yet complete
        if (ENABLE_FRONTDRL_BOOTUP && (frontDRLState == FRONTDRL_BOOTUP_STAGE_1 || frontDRLState == FRONTDRL_BOOTUP_STAGE_2)) 
        { runFrontDRLBootupAnimation(); }

        // Check if both bootup animations have completed
        if ((!ENABLE_BRAKE_BOOTUP || bootup_stage == 0) &&
            (!ENABLE_FRONTDRL_BOOTUP || frontDRLState == FRONTDRL_ON)) 
        {
            isBootupComplete = true; // All bootup animations are complete
            Serial.println(F("All bootup animations complete. Entering normal operation."));
        }
    }
    else
    {
        // If the bootup animation is complete or disabled, proceed with regular operation

        //* Indicators =============================================
        static bool previous_state = HIGH;               // Stores the previous state of the indicator button
        bool current_state = digitalRead(INDICATOR_PIN); // Reads the current state of the indicator button

        if (previous_state == HIGH && current_state == LOW)
        {
            startAnimation();
            button_held = true;
            Serial.println(F("Indicators Input Start"));
        }

        if (previous_state == LOW && current_state == HIGH)
        {
            button_held = false;
            Serial.println(F("Indicators Input Stop"));
        }

        previous_state = current_state;

        if (indicatorState != INDICATOR_IDLE)
        { runIndicatorAnimation(); }
        else if (!reset_called)
        {
            resetAnimation();
            Serial.println(F("Indicators Finished and reset"));
        }

        //* Brake ==================================================
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

        //* Reverse light ===============================================        
        static bool previous_reverse_state = HIGH;             // Stores the previous state of the reverse button
        bool current_reverse_state = digitalRead(REVERSE_PIN); // Reads the current state of the reverse button

        if (previous_reverse_state == HIGH && current_reverse_state == LOW)
        {
            startReverseAnimation();
            reverse_input_active = true;
        }

        if (previous_reverse_state == LOW && current_reverse_state == HIGH)
        { reverse_input_active = false; }

        previous_reverse_state = current_reverse_state;

        if (reverseState != REVERSE_IDLE)
        { runReverseAnimation(); }
    }
}
