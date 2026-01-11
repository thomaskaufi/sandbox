// Gople Kostume for Lilibeth Cuenca
// Driver Side - tager som input mode packet

#include <Arduino.h>
#include <FastLED.h>

constexpr int BUS_RX_PIN = 1;
constexpr uint32_t BUS_BAUD = 9600;
constexpr uint16_t NUM_LEDS = 266;  //Opdater til længste strip i klyngen
constexpr uint8_t LED_PIN = 3;
CRGB leds[NUM_LEDS];

constexpr uint8_t PKT_HEAD = 0xAA;
constexpr uint8_t PKT_TAIL = 0x55;

// BRUGER VARIABLE! *************************************************************************************

constexpr uint32_t INIT_MS = 1500;            //Længde på optakt til fase
constexpr uint32_t PANIC_MS = 30000;          //Længde på panic-mode
constexpr uint32_t PANIC_BLACKOUT_MS = 5000;  //Blackout (I halen på panic_MS længde)


// ----------------- Stage 1 params ------------------
constexpr CRGB stage1_groupColor = CRGB(30, 240, 250);
const CHSV stage1_hsv = rgb2hsv_approximate(stage1_groupColor);
constexpr uint8_t stage1_initBrightness = 40;  // 0..100
constexpr uint8_t stage1_brightnessPct = 40;   // 0..100
constexpr uint8_t stage1_basePct = 20;         // 0..100
constexpr uint8_t stage1_groupSize = 10;
constexpr uint16_t stage1_msPerLed = 10;
constexpr uint16_t stage1_trailFade = 80;
constexpr uint16_t stage1_leadRampMs = 1;

// ----------------- Stage 2 params -----------------
constexpr CRGB stage2_groupColor = CRGB(0, 0, 250);
const CHSV stage2_hsv = rgb2hsv_approximate(stage2_groupColor);
constexpr uint8_t stage2_initBrightness = 25;  // 0..100
constexpr uint8_t stage2_brightnessPct = 30;   // 0..100
constexpr uint8_t stage2_basePct = 20;         // 0..100
constexpr uint8_t stage2_groupSize = 20;
constexpr uint16_t stage2_msPerLed = 100;
constexpr uint16_t stage2_trailFade = 50;
constexpr uint16_t stage2_leadRampMs = 80;

// ----------------- Stage 3 params (sparkle) -----------------
constexpr CRGB stage3_groundColor = CRGB(0, 0, 255);
constexpr uint8_t stage3_groundBrightnessPct = 5;  // 0..100

constexpr CRGB stage3_sparkleColor = CRGB(0, 0, 255);
constexpr uint8_t stage3_sparkleBrightnessPct = 30;  // 0..100

constexpr uint8_t stage3_sparklesPerFrame = 1;
constexpr uint8_t stage3_fadePerFrame = 30;  // 0..255

// ----------------- Stage 4 params (breathing) -----------------
constexpr CRGB stage4_color = CRGB(45, 0, 90);
constexpr uint16_t stage4_pulseSpeedMs = 2000;
constexpr uint8_t stage4_minBrightnessPct = 15;  // 0..100
constexpr uint8_t stage4_maxBrightnessPct = 30;  // 0..100

// ----------------- PANIC PARAMS -----------------

constexpr CRGB panic_groundColor = CRGB(200, 30, 0);
constexpr uint8_t panic_groundBrightnessPct = 5;  // 0..100

constexpr CRGB panic_sparkleColor = CRGB(200, 30, 0);
constexpr uint8_t panic_sparkleBrightnessPct = 100;  // 0..100

constexpr uint8_t panic_sparklesPerFrame = 5;
constexpr uint8_t panic_fadePerFrame = 30;  // 0..255

//*******************************************************************************************************


struct Packet {
  uint8_t stage;  // 1..4
  uint8_t seq;    // 0..255
  uint8_t alarm;  // 0/1
};

enum Mode : uint8_t {
  MODE_STAGE_INIT = 0,
  MODE_STAGE_RUN = 1,
  MODE_PANIC = 2
};

static Mode mode = MODE_STAGE_INIT;
static uint8_t currentStage = 1;

static bool hasSeq = false;
static uint8_t lastSeqSeen = 0;

static uint32_t stageStartMs = 0;
static uint32_t panicStartMs = 0;

static inline void fillAll(const CRGB &c) {
  fill_solid(leds, NUM_LEDS, c);
}

static inline void set1(uint16_t idx, const CRGB &c) {
  leds[idx] = c;
}


static bool readPacket(Packet &out) {
  //Parsing Funktion! Modtager fra Master ESP
  static uint8_t idx = 0;
  static uint8_t buf[6];

  while (Serial1.available() > 0) {
    uint8_t b = (uint8_t)Serial1.read();

    if (idx == 0) {
      if (b != PKT_HEAD) continue;
      buf[idx++] = b;
      continue;
    }

    buf[idx++] = b;
    if (idx < 6) continue;

    idx = 0;

    if (buf[5] != PKT_TAIL) {
      if (buf[5] == PKT_HEAD) {
        buf[0] = PKT_HEAD;
        idx = 1;
      }
      continue;
    }

    uint8_t stage = buf[1];
    uint8_t seq = buf[2];
    uint8_t alarm = buf[3];
    uint8_t chk = buf[4];

    if ((uint8_t)(stage ^ seq ^ alarm) != chk) continue;
    if (stage < 1 || stage > 4) continue;
    if (!(alarm == 0 || alarm == 1)) continue;

    out.stage = stage;
    out.seq = seq;
    out.alarm = alarm;
    return true;
  }

  return false;
}

static inline void enterStage(uint8_t stage, uint8_t seq, uint32_t now) {
  (void)seq;
  currentStage = stage;
  stageStartMs = now;
  mode = MODE_STAGE_INIT;
  Serial.printf("[STAGE] stage=%u -> INIT\n", currentStage);
}

static inline void enterPanic(uint32_t now) {
  mode = MODE_PANIC;
  panicStartMs = now;
  Serial.println("[PANIC] enter");
}

static void onPacket(const Packet &p, uint32_t now) {
  if (p.alarm == 1) {
    if (mode != MODE_PANIC) enterPanic(now);
    return;
  }

  if (mode == MODE_PANIC) return;

  if (!hasSeq) {
    hasSeq = true;
    lastSeqSeen = p.seq;
    enterStage(p.stage, p.seq, now);
    return;
  }

  if (p.seq != lastSeqSeen) {
    lastSeqSeen = p.seq;
    enterStage(p.stage, p.seq, now);
    return;
  }
}

static void updateModes(uint32_t now) {
  if (mode == MODE_PANIC) {
    if ((uint32_t)(now - panicStartMs) >= PANIC_MS) {
      stageStartMs = now;
      mode = MODE_STAGE_INIT;
      Serial.println("[PANIC] exit -> STAGE_INIT");
    }
    return;
  }

  if (mode == MODE_STAGE_INIT) {
    if ((uint32_t)(now - stageStartMs) >= INIT_MS) {
      mode = MODE_STAGE_RUN;
      Serial.println("[STAGE] INIT -> RUN");
    }
  }
}


static void renderStageInit(uint8_t stage, uint32_t tMs) {
  //Optakt til animationer
  switch (stage) {
    case 1:
      {
        uint8_t v = (uint8_t)(min(tMs, INIT_MS) * 255UL / INIT_MS);
        uint8_t vCap = (uint8_t)(stage1_initBrightness * 255UL / 100UL);
        if (v > vCap) v = vCap;
        fill_solid(leds, NUM_LEDS, CHSV(stage1_hsv.h, stage1_hsv.s, v));
        FastLED.show();
      }
      break;

    case 2:
      {
        uint8_t v = (uint8_t)(min(tMs, INIT_MS) * 255UL / INIT_MS);
        uint8_t vCap = (uint8_t)(stage2_initBrightness * 255UL / 100UL);
        if (v > vCap) v = vCap;
        fill_solid(leds, NUM_LEDS, CHSV(stage2_hsv.h, stage2_hsv.s, v));
        FastLED.show();
      }
      break;

    case 3:
      {
        fillAll(stage3_groundColor);
        uint8_t cap = (uint8_t)(stage3_groundBrightnessPct * 255UL / 100UL);
        nscale8_video(leds, NUM_LEDS, cap);
        FastLED.show();
      }
      break;

    case 4:
      {
        fillAll(stage4_color);
        uint8_t cap = (uint8_t)(stage4_maxBrightnessPct * 255UL / 100UL);
        nscale8_video(leds, NUM_LEDS, cap);
        FastLED.show();
      }
      break;

    default: break;
  }
}

static void renderStageRun(uint8_t stage, uint32_t tMs) {
  switch (stage) {
    case 1:
      {
        CRGB base = stage1_groupColor;
        base.nscale8((uint8_t)(stage1_basePct * 255UL / 100UL));
        fillAll(base);

        uint16_t groupSize = min((uint16_t)stage1_groupSize, (uint16_t)NUM_LEDS);
        uint16_t trailLen = min((uint16_t)stage1_trailFade, (uint16_t)NUM_LEDS);

        uint32_t tRun = (tMs > INIT_MS) ? (tMs - INIT_MS) : 0;

        uint16_t head = (stage1_msPerLed > 0)
                          ? (uint16_t)((tRun / stage1_msPerLed) % NUM_LEDS)
                          : 0;

        for (uint16_t i = 0; i < groupSize; i++) {
          set1((head + i) % NUM_LEDS, stage1_groupColor);
        }

        if (stage1_leadRampMs > 0 && stage1_msPerLed > 0) {
          uint16_t inStep = (uint16_t)(tRun % stage1_msPerLed);

          uint16_t denom = (stage1_leadRampMs < stage1_msPerLed)
                             ? stage1_leadRampMs
                             : stage1_msPerLed;

          uint16_t step = (inStep < denom) ? inStep : denom;
          uint8_t a = (uint8_t)(step * 255UL / denom);

          uint16_t leadIdx = (head + groupSize) % NUM_LEDS;
          set1(leadIdx, blend(base, stage1_groupColor, a));
        }

        bool firstLap = (tRun < (uint32_t)NUM_LEDS * stage1_msPerLed);

        for (uint16_t j = 0; j < trailLen; j++) {
          int32_t idx = (int32_t)head - 1 - (int32_t)j;

          if (firstLap) {
            if (idx < 0) break;
          } else {
            while (idx < 0) idx += NUM_LEDS;
          }

          uint8_t mix = (trailLen <= 1) ? 255
                                        : (uint8_t)((j * 255UL) / (trailLen - 1));
          set1((uint16_t)idx, blend(stage1_groupColor, base, mix));
        }

        uint8_t cap = (uint8_t)(stage1_brightnessPct * 255UL / 100UL);
        nscale8_video(leds, NUM_LEDS, cap);

        FastLED.show();
      }
      break;

    case 2:
      {
        CRGB base = stage2_groupColor;
        base.nscale8((uint8_t)(stage2_basePct * 255UL / 100UL));
        fillAll(base);

        uint16_t groupSize = min((uint16_t)stage2_groupSize, (uint16_t)NUM_LEDS);
        uint16_t trailLen = min((uint16_t)stage2_trailFade, (uint16_t)NUM_LEDS);

        uint32_t tRun = (tMs > INIT_MS) ? (tMs - INIT_MS) : 0;

        uint16_t head = (stage2_msPerLed > 0)
                          ? (uint16_t)((tRun / stage2_msPerLed) % NUM_LEDS)
                          : 0;

        for (uint16_t i = 0; i < groupSize; i++) {
          set1((head + i) % NUM_LEDS, stage2_groupColor);
        }

        if (stage2_leadRampMs > 0 && stage2_msPerLed > 0) {
          uint16_t inStep = (uint16_t)(tRun % stage2_msPerLed);

          uint16_t denom = (stage2_leadRampMs < stage2_msPerLed)
                             ? stage2_leadRampMs
                             : stage2_msPerLed;

          uint16_t step = (inStep < denom) ? inStep : denom;
          uint8_t a = (uint8_t)(step * 255UL / denom);

          uint16_t leadIdx = (head + groupSize) % NUM_LEDS;
          set1(leadIdx, blend(base, stage2_groupColor, a));
        }

        bool firstLap = (tRun < (uint32_t)NUM_LEDS * stage2_msPerLed);

        for (uint16_t j = 0; j < trailLen; j++) {
          int32_t idx = (int32_t)head - 1 - (int32_t)j;

          if (firstLap) {
            if (idx < 0) break;
          } else {
            while (idx < 0) idx += NUM_LEDS;
          }

          uint8_t mix = (trailLen <= 1) ? 255
                                        : (uint8_t)((j * 255UL) / (trailLen - 1));
          set1((uint16_t)idx, blend(stage2_groupColor, base, mix));
        }

        uint8_t cap = (uint8_t)(stage2_brightnessPct * 255UL / 100UL);
        nscale8_video(leds, NUM_LEDS, cap);

        FastLED.show();
      }
      break;

    case 3:
      {
        fadeToBlackBy(leds, NUM_LEDS, stage3_fadePerFrame);

        CRGB base = stage3_groundColor;
        base.nscale8_video((uint8_t)(stage3_groundBrightnessPct * 255UL / 100UL));

        for (uint16_t i = 0; i < NUM_LEDS; i++) {
          if (leds[i].r < base.r) leds[i].r = base.r;
          if (leds[i].g < base.g) leds[i].g = base.g;
          if (leds[i].b < base.b) leds[i].b = base.b;
        }

        CRGB sp = stage3_sparkleColor;
        sp.nscale8_video((uint8_t)(stage3_sparkleBrightnessPct * 255UL / 100UL));

        for (uint8_t k = 0; k < stage3_sparklesPerFrame; k++) {
          uint16_t idx = random16(NUM_LEDS);
          leds[idx] += sp;
        }

        FastLED.show();
      }
      break;

    case 4:
      {
        fillAll(stage4_color);

        uint32_t tRun = (tMs > INIT_MS) ? (tMs - INIT_MS) : 0;
        uint16_t period = (stage4_pulseSpeedMs == 0) ? 1 : stage4_pulseSpeedMs;

        uint8_t phase = (uint8_t)((tRun % period) * 255UL / period);
        uint8_t wave = sin8(phase);

        uint8_t bMin = (uint8_t)(stage4_minBrightnessPct * 255UL / 100UL);
        uint8_t bMax = (uint8_t)(stage4_maxBrightnessPct * 255UL / 100UL);
        if (bMax < bMin) {
          uint8_t tmp = bMax;
          bMax = bMin;
          bMin = tmp;
        }

        uint8_t b = (uint8_t)(bMin + ((uint16_t)(bMax - bMin) * wave) / 255U);
        nscale8_video(leds, NUM_LEDS, b);

        FastLED.show();
      }
      break;

    default: break;
  }
}

void renderPanic(uint32_t tMs) {

  if (tMs >= (PANIC_MS - PANIC_BLACKOUT_MS)) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    return;
  }

  fadeToBlackBy(leds, NUM_LEDS, panic_fadePerFrame);

  CRGB base = panic_groundColor;
  base.nscale8_video((uint8_t)(panic_groundBrightnessPct * 255UL / 100UL));

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (leds[i].r < base.r) leds[i].r = base.r;
    if (leds[i].g < base.g) leds[i].g = base.g;
    if (leds[i].b < base.b) leds[i].b = base.b;
  }

  CRGB sp = panic_sparkleColor;
  sp.nscale8_video((uint8_t)(panic_sparkleBrightnessPct * 255UL / 100UL));

  for (uint8_t k = 0; k < panic_sparklesPerFrame; k++) {
    uint16_t idx = random16(NUM_LEDS);
    leds[idx] += sp;
  }

  FastLED.show();
}



void setup() {
  Serial.begin(115200);

  Serial1.begin(BUS_BAUD, SERIAL_8N1, BUS_RX_PIN, -1);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear(true);
  FastLED.setDither(1);

  stageStartMs = millis();
  Serial.println("Driver boot");
}

void loop() {
  uint32_t now = millis();

  Packet p;
  while (readPacket(p)) onPacket(p, now);

  updateModes(now);

  if (mode == MODE_PANIC) {
    renderPanic(now - panicStartMs);
  } else if (mode == MODE_STAGE_INIT) {
    renderStageInit(currentStage, now - stageStartMs);
  } else {
    renderStageRun(currentStage, now - stageStartMs);
  }
}
