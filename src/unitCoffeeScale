#include <HX711.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

// ===== ピン定義 =====
#define HX711_1_D 3
#define HX711_1_SCK 2
#define HX711_2_D 5
#define HX711_2_SCK 4
#define DS18B20_1_PIN 6
#define DS18B20_2_PIN 7

#define RX_PIN 10
#define TX_PIN 11
SoftwareSerial mySerial(RX_PIN, TX_PIN);

// ===== オブジェクト =====
HX711 hx1;
HX711 hx2;

OneWire oneWire1(DS18B20_1_PIN);
OneWire oneWire2(DS18B20_2_PIN);
DallasTemperature ds1(&oneWire1);
DallasTemperature ds2(&oneWire2);

// EEPROM保存値
float w1 = 1.0;
float w2 = 1.0;

// USB入力バッファ
String inputString = "";

// 温度取得管理
unsigned long lastTempRequest = 0;
const unsigned long tempInterval = 750; // ms
bool waitingForTemp = false;
float t1 = -99999;
float t2 = -99999;

// ===== セットアップ =====
void setup() {
  Serial.begin(9600);    
  mySerial.begin(9600);

  hx1.begin(HX711_1_D, HX711_1_SCK);
  hx2.begin(HX711_2_D, HX711_2_SCK);

  ds1.begin();
  ds2.begin();
  ds1.setResolution(11); // 高速化
  ds2.setResolution(11);

  EEPROM.get(0, w1);
  EEPROM.get(sizeof(float), w2);

  Serial.println("Ready. Enter like: w1,1.234");
  showCurrentValues();
}

// ===== メインループ =====
void loop() {
  handleSerialInput();

  unsigned long now = millis();

  // 温度測定管理（非同期）
  if (!waitingForTemp || now - lastTempRequest >= tempInterval) {
    if (!waitingForTemp) {
      ds1.requestTemperatures();
      ds2.requestTemperatures();
      lastTempRequest = now;
      waitingForTemp = true;
    } else {
      float temp1 = ds1.getTempCByIndex(0);
      float temp2 = ds2.getTempCByIndex(0);

      if (temp1 != DEVICE_DISCONNECTED_C) t1 = temp1;
      if (temp2 != DEVICE_DISCONNECTED_C) t2 = temp2;

      waitingForTemp = false;
    }
  }

  sendSensorValues();
  delay(50);
}

// ===== センサー値送信 =====
void sendSensorValues() {
  float val1 = readHX711Direct(hx1, w1);
  float val2 = readHX711Direct(hx2, w2);

  // UART出力
  mySerial.print(val1); mySerial.print(",");
  mySerial.print(val2); mySerial.print(",");
  mySerial.print(t1); mySerial.print(",");
  mySerial.println(t2);

  // // USB出力
  // Serial.print(val1); Serial.print(",");
  // Serial.print(val2); Serial.print(",");
  // Serial.print(t1); Serial.print(",");
  // Serial.println(t2);
}

// ===== HX711順次読み込み（平均化なし） =====
float readHX711Direct(HX711 &hx, float scale) {
  if (hx.is_ready()) {
    long raw = hx.read(); // 平均化なし
    return raw * scale / 8388608.0;
  }
  return -99999;
}

// ===== USB入力処理 =====
void handleSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputString.length() > 0) {
        processInput(inputString);
        inputString = "";
      }
    } else {
      inputString += c;
    }
  }
}

void processInput(String s) {
  int commaIndex = s.indexOf(',');
  if (commaIndex < 0) {
    Serial.println("Invalid format. Use key,value");
    return;
  }

  String key = s.substring(0, commaIndex);
  float value = s.substring(commaIndex + 1).toFloat();

  int address = getAddressForKey(key);
  EEPROM.put(address, value);

  Serial.print("Saved ");
  Serial.print(key);
  Serial.print(" = ");
  Serial.println(value);

  EEPROM.get(0, w1);
  EEPROM.get(sizeof(float), w2);
  showCurrentValues();
}

int getAddressForKey(String key) {
  if (key == "w1") return 0;
  if (key == "w2") return sizeof(float);
  return 100;
}

void showCurrentValues() {
  Serial.print("Current values: ");
  Serial.print("w1=");
  Serial.print(w1, 6);
  Serial.print(", w2=");
  Serial.println(w2, 6);
}
