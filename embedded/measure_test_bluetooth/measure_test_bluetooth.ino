#include "BluetoothSerial.h"

String const deviceName = "saniki measure 6"; // 自分。末尾の数字は nodeId。
String const targetDeviceName = "saniki TJH"; // 表示部

//

int nodeId = -1; // 自分のノード ID (0 から 7)。setup() 内で deviceName から計算して上書きされるので、ここで設定しても無駄。
BluetoothSerial BT_serialBT;

void setup()
{
  nodeId = deviceName[deviceName.length() - 1] - '0';

  Serial.begin(115200);

  Serial.println();
  Serial.printf("@@@ measure_test_bluetooth nodeId = %d\n", nodeId);
  BT_serialBT.begin(deviceName, true);
}

void loop()
{
  Serial.printf("connecting to BT device '%s'...\n", targetDeviceName);
  if (!BT_serialBT.connect(targetDeviceName)) {
    Serial.println("failed to connect to BT device");
  } else {
    float weight_kg = random(1000) / 1000.0f;
    Serial.printf("sending weight: %f kg\n", weight_kg);
    BT_serialBT.printf("%d %f\n", nodeId, weight_kg);
    BT_serialBT.disconnect();
    Serial.println("disconnected");
  }

  delay(5000);
}
