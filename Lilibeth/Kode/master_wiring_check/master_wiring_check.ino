void setup() {
  Serial.begin(115200);

  pinMode(0, INPUT_PULLUP);

  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
}

void loop() {

  int but = digitalRead(0);

  int a = digitalRead(4);
  int b = digitalRead(5);
  int c = digitalRead(6);
  int d = digitalRead(7);

  Serial.print("rotary states: ");
  Serial.print(a);
  Serial.print(" ");
  Serial.print(b);
  Serial.print(" ");
  Serial.print(c);
  Serial.print(" ");
  Serial.print(d);
  Serial.print("    Button:");
  Serial.println(but);

  delay(200);
}
