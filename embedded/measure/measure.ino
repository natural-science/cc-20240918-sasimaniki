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
 *   - deviceName と targetDeviceName
 *   - servoPositions
 *   - weightConversionTables
 *
 * 他のノードの情報が混じっているのが気持ち悪いけど、実装が簡単なので
 * こうしている。
 */

#include "BluetoothSerial.h"
#include <ESP32Servo.h>

int nodeId = -1; // 自分のノード ID (0 から 7)。setup() 内で deviceName から計算して上書きされるので、ここで設定しても無駄。
String const deviceName = "saniki measure 6"; // 自分。末尾の数字は nodeId。
String const targetDeviceName = "saniki TJH";       // 表示部

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

void BT_init()
{
  BT_serialBT.begin(deviceName, true);
}

bool BT_connect()
{
  return BT_serialBT.connect(targetDeviceName);
}

bool BT_connected(int timeout = 0)
{
  return BT_serialBT.connected(timeout);
}

void BT_sendWeight(float weight_kg)
{
  BT_serialBT.printf("%d %f\n", nodeId, weight_kg);
}

void BT_disconnect()
{
  BT_serialBT.disconnect();
}



//

void setup()
{
  nodeId = deviceName[deviceName.length() - 1] - '0';

  Serial.begin(115200);

  Serial.println();
  Serial.printf("@@@ measure nodeId = %d\n", nodeId);
  if (nodeId < 0 || nodeId > 7) {
    Serial.println("got invalid nodeId, aborting");
    abort();
  }

  LockingBar_init(16, // pin
                  servoPositions[nodeId].unlockPosition,
                  servoPositions[nodeId].lockPosition);
  Sensor_init(36);
  BT_init();
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

void loop_production()
{
  LockingBar_unlock();
  float resistance = Sensor_measure();
  float weight_kg = resistanceToWeight(resistance);
  LockingBar_lock();

  if (!BT_connect()) {
    Serial.println("failed to connect to BT device");
  } else {
    BT_sendWeight(weight_kg);
    BT_disconnect();
  }

  delay(1000 * 60);
}

void loop_test_00()
{
  if (!BT_connect()) {
    Serial.println("failed to connect to BT device");
  } else {
    float weight_kg = random(1000) / 1000.0f;
    Serial.printf("sending weight: %f kg\n", weight_kg);
    BT_sendWeight(weight_kg);
    BT_disconnect();
  }

  delay(5000);
}


void loop()
{
  loop_production();
  //loop_test_00();
}
