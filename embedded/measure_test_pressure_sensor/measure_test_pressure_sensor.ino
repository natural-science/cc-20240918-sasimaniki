//

int sensor_pin = 36;

void setup()
{
  Serial.begin(115200);

  Serial.println();
  Serial.println("@@@ measure_test_pressure_sensor");

  pinMode(sensor_pin, INPUT);
}

void loop()
{
  int adcValue = analogRead(sensor_pin);
  float voltage = adcValue * 3.3f / 4095.0f;
  float resitance = 10e3f * voltage / (3.3f - voltage);
  // 3.3 * r / (r + 10k) = v
  Serial.printf("ADC Value: %d, Voltage: %.2f V, Resistance: %.2f kOhm\n", adcValue, voltage, resitance * 1e-3f);

  delay(500);
}
