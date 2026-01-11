#include <FastLED.h>

constexpr uint8_t LED_PIN  = 6;
constexpr uint16_t NUM_LEDS = 60;

CRGB leds[NUM_LEDS];

// ----------------- MODE -----------------

enum Mode : uint8_t {
  MODE_STAGE_INIT,
  MODE_STAGE_RUN,
  MODE_PANIC
};

Mode mode = MODE_PANIC;
uint32_t stageStartMs = 0;

// ----------------- PANIC TIMING -----------------

constexpr uint32_t PANIC_MS = 7000;
constexpr uint32_t PANIC_BLACKOUT_MS = 5000; // last 5 seconds

// ----------------- PANIC PARAMS -----------------

constexpr CRGB panic_groundColor = CRGB(200, 30, 0);
constexpr uint8_t panic_groundBrightnessPct = 5;    // 0..100

constexpr CRGB panic_sparkleColor = CRGB(200, 30, 0);
constexpr uint8_t panic_sparkleBrightnessPct = 100; // 0..100

constexpr uint8_t panic_sparklesPerFrame = 5;
constexpr uint8_t panic_fadePerFrame = 30;          // 0..255

// ------------------------------------------------

void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear(true);
  FastLED.setDither(1);

  stageStartMs = millis();
  mode = MODE_PANIC;   // force panic mode for preview
}

// ----------------- PANIC RENDER -----------------

void renderPanic(uint32_t tMs) {

  // ---- BLACKOUT PHASE ----
  if (tMs >= (PANIC_MS - PANIC_BLACKOUT_MS)) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    return;
  }

  // 1) Fade previous frame
  fadeToBlackBy(leds, NUM_LEDS, panic_fadePerFrame);

  // 2) Ground floor (never overwrite brighter sparkles)
  CRGB base = panic_groundColor;
  base.nscale8_video((uint8_t)(panic_groundBrightnessPct * 255UL / 100UL));

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (leds[i].r < base.r) leds[i].r = base.r;
    if (leds[i].g < base.g) leds[i].g = base.g;
    if (leds[i].b < base.b) leds[i].b = base.b;
  }

  // 3) Add sparkles (additive = agitated)
  CRGB sp = panic_sparkleColor;
  sp.nscale8_video((uint8_t)(panic_sparkleBrightnessPct * 255UL / 100UL));

  for (uint8_t k = 0; k < panic_sparklesPerFrame; k++) {
    uint16_t idx = random16(NUM_LEDS);
    leds[idx] += sp;
  }

  FastLED.show();
}

// ----------------- LOOP -----------------

void loop() {
  renderPanic(millis() - stageStartMs);
}
