int led_pins[] = { 13, 12, 14, 27, 26, 25, 33, 32 };

void setup()
{
}

void loop()
{
  for (int i = 0; i < 8; i++) {
    pinMode(led_pins[i], OUTPUT); // this turns off analogWrite() (of previous loop)
  }

  for (int i = 0; i < 8; i++) {
    digitalWrite(led_pins[i], HIGH);
    delay(200);
    digitalWrite(led_pins[i], LOW);
  }

  for (int brightness = 1; brightness <= 4; brightness++) {
    for (int i = 0; i < 8; i++) {
      analogWrite(led_pins[i], 255 * brightness / 4);
      delay(200);
    }
  }
}
