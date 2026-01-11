constexpr int ROT_PINS[4] = { 4, 5, 6, 7 };
constexpr int PUSH_PIN = 0;
constexpr int BUS_TX_PIN = 3;

constexpr uint32_t TX_HZ = 20;
constexpr uint32_t TX_PERIOD_MS = 1000 / TX_HZ;

constexpr uint32_t ROT_STABLE_MS = 50;    // Rotary switch "debounce"
constexpr uint32_t BTN_DEBOUNCE_MS = 30;  // Pushbutton debounce
constexpr uint32_t ALARM_HOLD_MS = 1000;  // Alarm signal length

uint8_t stage = 1;
uint8_t seq = 0;

uint8_t rotCandidate = 1;
uint8_t rotStable = 1;
uint32_t rotCandidateSince = 0;

bool btnLast = true;  // true = not pressed (because pullup)
uint32_t btnLastChange = 0;

uint32_t alarmUntilMs = 0;

uint32_t nextTxMs = 0;

// *********** FUNCTIONS:

static uint8_t readRotaryStageRaw() {
  // Returns 1-4 if exactly one pin is LOW, else 0 (invalid)
  int activeIndex = -1;
  for (int i = 0; i < 4; i++) {
    if (digitalRead(ROT_PINS[i]) == LOW) {
      if (activeIndex != -1) return 0;  // multiple active
      activeIndex = i;
    }
  }
  if (activeIndex == -1) return 0;  // none active
  return (uint8_t)(activeIndex + 1);
}

static void sendPacket(uint8_t st, uint8_t sq, uint8_t alarm) {
  uint8_t chk = st ^ sq ^ alarm;

  Serial1.write((uint8_t)0xAA);
  Serial1.write(st);
  Serial1.write(sq);
  Serial1.write(alarm);
  Serial1.write(chk);
  Serial1.write((uint8_t)0x55);
}

// ********** END OF FUNCTIONS

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) pinMode(ROT_PINS[i], INPUT_PULLUP);
  pinMode(PUSH_PIN, INPUT_PULLUP);

  // Serial1 is TX bus - USB Serial stays free for debug
  Serial1.begin(9600, SERIAL_8N1, -1, BUS_TX_PIN);

  // Initialize stage from rotary if valid
  uint8_t r = readRotaryStageRaw();
  if (r >= 1 && r <= 4) {
    stage = r;
    rotCandidate = r;
    rotStable = r;
    rotCandidateSince = millis();
  }

  nextTxMs = millis();
}

void loop() {
  uint32_t now = millis();

  // --- Rotary: update stage + seq (with stabilization) ---
  uint8_t raw = readRotaryStageRaw();
  if (raw >= 1 && raw <= 4) {
    if (raw != rotCandidate) {
      rotCandidate = raw;
      rotCandidateSince = now;
    } else {
      if ((now - rotCandidateSince) >= ROT_STABLE_MS && rotStable != rotCandidate) {
        rotStable = rotCandidate;

        if (stage != rotStable) {
          stage = rotStable;
          seq = (uint8_t)(seq + 1);  // wraps 255->0 automatically
        }
      }
    }
  }

  // --- Pushbutton: trigger alarm pulse ---
  bool btnNow = (digitalRead(PUSH_PIN) == HIGH);  // true = not pressed
  if (btnNow != btnLast && (now - btnLastChange) >= BTN_DEBOUNCE_MS) {
    btnLastChange = now;
    btnLast = btnNow;

    if (btnNow == false) { // pressed (LOW)
      alarmUntilMs = now + ALARM_HOLD_MS; // hold alarm=1 for ~1s
    }
  }

  uint8_t alarm = (now < alarmUntilMs) ? 1 : 0;

  // --- Periodic broadcast on Serial1 ---
  if ((int32_t)(now - nextTxMs) >= 0) {
    nextTxMs += TX_PERIOD_MS;
    sendPacket(stage, seq, alarm);
  }

  // Debug
  Serial.printf("stage=%u seq=%u alarm=%u\n", stage, seq, alarm);
}
