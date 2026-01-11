/*
DRIVER RX sanity test (ESP32-C3)

- Reads packets from Serial1 (RX bus) on GPIO0
- Parses: [0xAA][stage][seq][alarm][chk][0x55]
- chk = stage ^ seq ^ alarm
- Prints only valid packets

Wiring:
- Master GPIO3 (TX) -> Driver GPIO0 (RX)
- GND -> GND
*/

#define BUS_RX_PIN 1
#define BUS_BAUD   9600

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial1.begin(BUS_BAUD, SERIAL_8N1, BUS_RX_PIN, -1); // RX=GPIO0, TX unused
  Serial.println("Driver RX parser started");
}

void loop() {
  static uint8_t buf[6];
  static uint8_t idx = 0;

  while (Serial1.available()) {
    uint8_t b = (uint8_t)Serial1.read();

    // State machine: wait for start byte 0xAA
    if (idx == 0) {
      if (b != 0xAA) continue;
      buf[idx++] = b;
      continue;
    }

    // Collect the rest of the packet (6 bytes total)
    buf[idx++] = b;

    if (idx >= 6) {
      // buf: [0]=AA [1]=stage [2]=seq [3]=alarm [4]=chk [5]=55
      uint8_t stage = buf[1];
      uint8_t seq   = buf[2];
      uint8_t alarm = buf[3];
      uint8_t chk   = buf[4];
      uint8_t endb  = buf[5];

      bool ok_end = (endb == 0x55);
      bool ok_chk = ((uint8_t)(stage ^ seq ^ alarm) == chk);
      bool ok_stage = (stage >= 1 && stage <= 4);
      bool ok_alarm = (alarm == 0 || alarm == 1);

      if (ok_end && ok_chk && ok_stage && ok_alarm) {
        Serial.print("OK  stage=");
        Serial.print(stage);
        Serial.print(" seq=");
        Serial.print(seq);
        Serial.print(" alarm=");
        Serial.println(alarm);
      } else {
        Serial.print("BAD ");
        Serial.print(" stage=0x"); Serial.print(stage, HEX);
        Serial.print(" seq=0x");   Serial.print(seq, HEX);
        Serial.print(" alarm=0x"); Serial.print(alarm, HEX);
        Serial.print(" chk=0x");   Serial.print(chk, HEX);
        Serial.print(" end=0x");   Serial.println(endb, HEX);
      }

      // Reset and look for next packet start
      idx = 0;
    }
  }
}
