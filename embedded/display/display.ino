/*
 * 表示部
 *
 * 測定部は connect 後データを write してすぐに disconnect する。
 * 送ってくるデータは "[nodeId] [weight_kg]\n" な形式。
 * 例: "0 0.256789\n"
 *
 * AC アダプタからの給電を前提にしていて、省電力化はまったく無し。
 */

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;
String const deviceName = "saniki display";

struct Node {
  unsigned long lastSeenAt_ms;
  float weight_kg;
  // TODO: 容器の重さとか?  閾値とか?
  // TODO: 警告が必要かどうかとか?
};

Node nodes[8];
int const numNodes = sizeof(nodes) / sizeof(*nodes);

int const ledPins[] = { 13, 12, 14, 27, 26, 25, 33, 32 }; // 回路に合わせて変えなきゃだめ
struct LED
{
  enum Mode {
    mode_blink,
    mode_fade,
  };

  int pin;
  Mode mode;
  int intensity;
  int onLength_ms;
  int offLength_ms;
  int phase_ms;
};
LED leds[numNodes];

void setup()
{
  Serial.begin(115200);
  SerialBT.begin(deviceName);

  for (int i = 0; i < numNodes; ++i) {
    nodes[i].lastSeenAt_ms = static_cast<unsigned long>(-1) >> 1;
    nodes[i].weight_kg = 0.0f;
  }

  for (int i = 0; i < numNodes; ++i) {
    pinMode(ledPins[i], OUTPUT);
    leds[i].pin = ledPins[i];
    leds[i].mode = LED::Mode::mode_blink;
    leds[i].intensity = 255;
    leds[i].onLength_ms = 200;
    leds[i].offLength_ms = 800;
    leds[i].phase_ms = random(0x7fff);
  }
}

void bt_nextTick(unsigned long now_ms)
{
  if (!SerialBT.available()) {
    return;
  }

  String data = SerialBT.readString();
  Serial.print("Received: ");
  Serial.println(data);

  // expected format: "[nodeId] [weight_kg]\n"
  // e.g. "0 0.256789\n"

  int sep = data.indexOf(' ');
  if (sep <= 0)
    return;

  int nodeId = data.toInt();
  float weight_kg = data.substring(sep + 1).toFloat();
  // Serial.print("nodeId = ");
  // Serial.print(nodeId);
  // Serial.print(", weight_kg = ");
  // Serial.print(weight_kg);
  // Serial.println();
  if (nodeId < 0 || nodeId >= numNodes)
    return;

  nodes[nodeId].lastSeenAt_ms = now_ms;
  nodes[nodeId].weight_kg = weight_kg;
}

void led_nextTick(LED &led, int deltaTime_ms)
{
  led.phase_ms += deltaTime_ms;
  if (led.phase_ms >= led.onLength_ms + led.offLength_ms) {
    led.phase_ms %= led.onLength_ms + led.offLength_ms;
  }
  switch (led.mode) {
  case LED::Mode::mode_blink:
    if (led.phase_ms < led.onLength_ms) {
      analogWrite(led.pin, led.intensity);
    } else {
      analogWrite(led.pin, 0);
    }
    break;
  case LED::Mode::mode_fade:
    if (led.phase_ms < led.onLength_ms) {
      analogWrite(led.pin, led.intensity * led.phase_ms / led.onLength_ms);
    } else {
      int phase = led.offLength_ms - (led.phase_ms - led.onLength_ms);
      analogWrite(led.pin, led.intensity * phase / led.offLength_ms);
    }
    break;
  }
}

void leds_nextTick(int deltaTime_ms)
{
  // turn LEDs on/off based on their phase_ms and given on/off lengths

  for (int i = 0; i < numNodes; ++i) {
    led_nextTick(leds[i], deltaTime_ms);
  }
}

unsigned long oldNow_ms = 0ul;

void loop()
{
  delay(10);
  unsigned long now_ms = millis();
  int deltaTime_ms = now_ms - oldNow_ms;
  oldNow_ms = now_ms;

  // データ受信 & ノードの状態更新
  bt_nextTick(now_ms);

  // 各ノードの状態に合わせて点滅周期を更新する
  for (int i = 0; i < numNodes; ++i) {
    unsigned long tooLong_ms = 120000ul;
    if (now_ms - nodes[i].lastSeenAt_ms > tooLong_ms) {
      // 120 秒間有効なデータ受信無し
      nodes[i].lastSeenAt_ms = now_ms - tooLong_ms; // this prevents possible overflow
      leds[i].mode = LED::Mode::mode_fade;
      leds[i].intensity = 64;
      leds[i].onLength_ms = 5000;
      leds[i].offLength_ms = 5000;
    } else if (nodes[i].weight_kg < 0.300f) {
      // 残量かなり少ない
      leds[i].mode = LED::Mode::mode_blink;
      leds[i].intensity = 255;
      leds[i].onLength_ms = 900;
      leds[i].offLength_ms = 100;
    } else if (nodes[i].weight_kg < 0.500f) {
      // 残量やや減り
      leds[i].mode = LED::Mode::mode_blink;
      leds[i].intensity = 255;
      leds[i].onLength_ms = 100;
      leds[i].offLength_ms = 900;
    } else {
      // たっぷり
      leds[i].mode = LED::Mode::mode_blink;
      leds[i].intensity = 255;
      leds[i].onLength_ms = 0;
      leds[i].offLength_ms = 1000;
    }
  }

  // LED を点滅させる
  leds_nextTick(deltaTime_ms);
}
