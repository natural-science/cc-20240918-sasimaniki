/*
 * 測定部
 *
 * ロックを解除して測定、再ロック、表示部に接続してデータ送信、切断、待機。
 * テストしやすくするため待機時間を短かくしている (60 秒)。
 *
 * 送るデータは "[nodeId] [weight_kg]\n" な形式。
 * 例: "0 0.256789\n"
 *
 * 使用前に書き換えが必要な場所:
 *
 *   - nodeId と deviceId
 *   - servoPositions
 *   - weightConversionTables
 *
 * 他のノードの情報が混じっているのが気持ち悪いけど、実装が簡単なので
 * こうしている。
 */

#include "BluetoothSerial.h"
#include <ESP32Servo.h>

int const nodeId = 0;                         // 自分
String const deviceName = "saniki measure 0"; // 自分。末尾の数字は nodeId。
String const targetDeviceName = "saniki display"; // 表示部

struct ServoPosition {
  uint32_t unlockPosition;
  uint32_t lockPosition;
};

ServoPosition servoPositions[] = {
  { 1000, 1900 }, // node 0
  { 1000, 1900 }, // node 1
  { 1000, 1900 }, // node 2
  { 1000, 1900 }, // node 3
  { 1000, 1900 }, // node 4
  { 1000, 1900 }, // node 5
  { 1000, 1900 }, // node 6
  { 1000, 1900 }, // node 7
};

struct DataPoint {
  float resistance;
  float weight_kg;
};

static const DataPoint weightConversionTables[][2] = {
  {{ 0.0f, 1.0f }, { 1e6f, 0.0f }}, // node 0
  {{ 0.0f, 1.0f }, { 1e6f, 0.0f }}, // node 1
  {{ 0.0f, 1.0f }, { 1e6f, 0.0f }}, // node 2
  {{ 0.0f, 1.0f }, { 1e6f, 0.0f }}, // node 3
  {{ 0.0f, 1.0f }, { 1e6f, 0.0f }}, // node 4
  {{ 0.0f, 1.0f }, { 1e6f, 0.0f }}, // node 5
  {{ 0.0f, 1.0f }, { 1e6f, 0.0f }}, // node 6
  {{ 0.0f, 1.0f }, { 1e6f, 0.0f }}, // node 7
};


// LockingBar

Servo LockingBar_servo;
int LockingBar_servoPin;
uint32_t LockingBar_unlockPosition;
uint32_t LockingBar_lockPosition;

void LockingBar_init(int pin, uint32_t unlockPosition, uint32_t lockPosition)
{
  LockingBar_servoPin = pin;
  LockingBar_unlockPosition = unlockPosition;
  LockingBar_lockPosition = lockPosition;

  LockingBar_servo.attach(LockingBar_servoPin);
}

void LockingBar_unlock()
{
  LockingBar_servo.writeMicroseconds(LockingBar_unlockPosition);
}

void LockingBar_lock()
{
  LockingBar_servo.writeMicroseconds(LockingBar_lockPosition);
}



// Sensor

int Sensor_pin;

void Sensor_init(int pin)
{
  Sensor_pin = pin;
  pinMode(Sensor_pin, INPUT);
}

float Sensor_measure()
{
  // measure multiple times and calculate average
  int N = 256;
  float sum = 0.0f;
  for (int i = 0; i < N; ++i) {
    int adcValue = analogRead(Sensor_pin);
    float voltage = adcValue * 3.3f / 4095.0f;
    sum += voltage;;
    delay(10);
  }
  float voltage = sum / N;
  float resitance = 10e3f * voltage / (3.3f - voltage);
  // 3.3 * r / (r + 10k) = v
  Serial.printf("Voltage: %.2f V, Resistance: %.2f kOhm\n", voltage, resitance * 1e-3f);
}


// BT

BluetoothSerial BT_serialBT;

void BT_connect()
{
}

void BT_sendWeight(float weight_kg)
{
  BT_serialBT.printf("%d %f\n", nodeId, weight_kg);
}

void BT_disconnect()
{
}



//

void setup()
{
  Serial.begin(115200);

  Serial.println();
  Serial.println("@@@ measure");

  LockingBar_init(16, servoPositions[nodeId].unlockPosition, servoPositions[nodeId].lockPosition);
  Sensor_init(36);
}

float resistanceToWeight(float resistance)
{
  DataPoint const *t = weightConversionTables[nodeId];
  int numPoints = sizeof(weightConversionTables[0]) / sizeof(weightConversionTables[0][0]);
  for (int i1 = 1; i1 < numPoints; i1++) {
    if (t[i1].resistance < resistance)
      continue;

    int i0 = i1 - 1;
    // linear interpolate in [i0..i1]
    float r0 = t[i0].resistance;
    float r1 = t[i1].resistance;
    float w0 = t[i0].weight_kg;
    float w1 = t[i1].weight_kg;
    float ratio = (resistance - r0) / (r1 - r0);
    float weight_kg = w0 + ratio * (w1 - w0);
    return weight_kg;
  }

  // resistance is too high
  return t[numPoints - 1].weight_kg;
}

void loop()
{
  LockingBar_unlock();
  float resistance = Sensor_measure();
  float weight_kg = resistanceToWeight(resistance);
  LockingBar_lock();

  BT_connect();
  BT_sendWeight(weight_kg);
  BT_disconnect();

  delay(1000 * 60);

}
