/*
 * サーボの一つを最大範囲往復させる。
 * どのサーボかは、setup() 内で指定している。
 * 空転できる状態で実行すること。
 * でないと、サーボが過負荷で破損する。
 */

// ESP32 では Arduino 標準の Servo は使えない。
// 代わりに、外部のライブラリ "ESP32Servo" を使う (Arduino IDE のライ
// ブラリマネージャからインストールできる)。
#include <ESP32Servo.h> // include すると Servo が ESP32Servo のもので置き換わる。使い方は標準のとほぼ同じ。

Servo servo;

void setup()
{
  Serial.begin(115200);

  Serial.println();
  Serial.println("@@@ measure_test_servo_simple");

  servo.attach(16); // 接続しているサーボ信号ピンを指定する。
}

void loop()
{
  // データシート上のパルス幅の範囲は 498〜2400 [micro seconds]
  uint32_t const min_point = 498; // [micro seconds]。最小位置のパルス幅。
  uint32_t const max_point = 2400; // [micro seconds]。最大位置のパルス幅。
  uint32_t const mid_point = (min_point + max_point) / 2;

  Serial.println("@@@@ ---- ----");
  servo.writeMicroseconds(min_point);
  delay(1000);

  Serial.println("---- @@@@ ----");
  servo.writeMicroseconds(mid_point);
  delay(1000);

  Serial.println("---- ---- @@@@");
  servo.writeMicroseconds(max_point);
  delay(1000);

  Serial.println("---- @@@@ ----");
  servo.writeMicroseconds(mid_point);
  delay(1000);
}
