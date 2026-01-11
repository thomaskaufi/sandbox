// Driver.ino — ESP32-C3 Super Mini (Driver)
// One-way RX bus listener, parses 6-byte packets and runs a simple state machine:
// STAGE_INIT -> STAGE_RUN, with PANIC override for 30s.
// No FastLED yet. No stage animations filled in.

#include <Arduino.h>

// ----------------- Pins / UART -----------------
constexpr int BUS_RX_PIN = 1;   // Driver RX bus
constexpr uint32_t BUS_BAUD = 9600;

// ----------------- Protocol -----------------
constexpr uint8_t PKT_HEAD = 0xAA;
constexpr uint8_t PKT_TAIL = 0x55;

// ----------------- Timing -----------------
constexpr uint32_t INIT_MS  = 1500;   // placeholder init/fade-in duration
constexpr uint32_t PANIC_MS = 30000;  // 30s panic override

// ----------------- State -----------------
struct Packet {
  uint8_t stage; // 1..4
  uint8_t seq;   // 0..255
  uint8_t alarm; // 0/1
};

enum Mode : uint8_t {
  MODE_STAGE_INIT = 0,
  MODE_STAGE_RUN  = 1,
  MODE_PANIC      = 2
};

static Mode mode = MODE_STAGE_INIT;

static uint8_t currentStage = 1;

static bool    hasSeq = false;
static uint8_t lastSeqSeen = 0;

static uint32_t stageStartMs = 0;
static uint32_t panicStartMs = 0;

// ----------------- Minimal parser -----------------
// Reads from Serial1 (RX only), emits valid packets.
// Frame: [0xAA][stage][seq][alarm][chk][0x55]
// chk = stage ^ seq ^ alarm
static bool readPacket(Packet &out) {
  // State machine for 6-byte frame
  static uint8_t idx = 0;
  static uint8_t buf[6];

  while (Serial1.available() > 0) {
    uint8_t b = (uint8_t)Serial1.read();

    if (idx == 0) {
      if (b != PKT_HEAD) continue; // hunt for header
      buf[idx++] = b;
      continue;
    }

    buf[idx++] = b;

    if (idx < 6) continue;

    // We have 6 bytes
    idx = 0;

    // Validate tail
    if (buf[5] != PKT_TAIL) {
      // If this byte could be a header, allow quick resync
      if (buf[5] == PKT_HEAD) {
        buf[0] = PKT_HEAD;
        idx = 1;
      }
      continue;
    }

    uint8_t stage = buf[1];
    uint8_t seq   = buf[2];
    uint8_t alarm = buf[3];
    uint8_t chk   = buf[4];

    // Checksum
    if ((uint8_t)(stage ^ seq ^ alarm) != chk) continue;

    // Basic sanity (optional but cheap)
    if (stage < 1 || stage > 4) continue;
    if (!(alarm == 0 || alarm == 1)) continue;

    out.stage = stage;
    out.seq   = seq;
    out.alarm = alarm;
    return true;
  }

  return false;
}

// ----------------- Stage logic -----------------
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
  // Alarm trigger wins, and panic ignores stages until it ends.
  if (p.alarm == 1) {
    if (mode != MODE_PANIC) {
      enterPanic(now);
    }
    return;
  }

  if (mode == MODE_PANIC) return;

  // Any change in seq is a new stage event (resync point)
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

  // Same seq: do nothing (no INIT restart)
}

static void updateModes(uint32_t now) {
  if (mode == MODE_PANIC) {
    if ((uint32_t)(now - panicStartMs) >= PANIC_MS) {
      // Panic finished. Resume stage rendering with a clean INIT.
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

// ----------------- Rendering placeholders -----------------
static void renderStageInit(uint8_t stage, uint32_t tMs) {
  (void)tMs;
  switch (stage) {
    case 1: /* TODO */ break;
    case 2: /* TODO */ break;
    case 3: /* TODO */ break;
    case 4: /* TODO */ break;
    default: break;
  }
}

static void renderStageRun(uint8_t stage, uint32_t tMs) {
  (void)tMs;
  switch (stage) {
    case 1: /* TODO */ break;
    case 2: /* TODO */ break;
    case 3: /* TODO */ break;
    case 4: /* TODO */ break;
    default: break;
  }
}

static void renderPanic(uint32_t tMs) {
  (void)tMs;
  // TODO: panic animation later
}

// ----------------- Arduino -----------------
void setup() {
  Serial.begin(115200);

  // RX bus on Serial1, RX pin = BUS_RX_PIN, TX unused
  Serial1.begin(BUS_BAUD, SERIAL_8N1, BUS_RX_PIN, -1);

  uint32_t now = millis();
  stageStartMs = now;

  Serial.println("Driver boot");
}

void loop() {
  uint32_t now = millis();

  // Drain all available packets; keep behavior deterministic
  Packet p;
  while (readPacket(p)) {
    // Optional: print each valid packet (can be noisy at 20 Hz)
    // Serial.printf("pkt stage=%u seq=%u alarm=%u\n", p.stage, p.seq, p.alarm);
    onPacket(p, now);
  }

  updateModes(now);

  // Render placeholder
  if (mode == MODE_PANIC) {
    renderPanic(now - panicStartMs);
  } else if (mode == MODE_STAGE_INIT) {
    renderStageInit(currentStage, now - stageStartMs);
  } else { // MODE_STAGE_RUN
    renderStageRun(currentStage, now - stageStartMs);
  }

  // Keep loop tight and deterministic; no delay needed.
}
