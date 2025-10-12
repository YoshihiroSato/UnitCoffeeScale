#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Adafruit_NeoPixel.h>

// ===== OLED設定 =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SDA_PIN 38
#define SCL_PIN 39
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== UART設定 =====
#define RX_PIN 1
#define TX_PIN 2
#define BAUD_RATE 9600

// ===== LED & BUTTON設定 =====
#define BUTTON_PIN 41
#define LED_PIN 35
#define NUM_PIXELS 1

Adafruit_NeoPixel pixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

bool ScaleOn = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// ===== データ格納用 =====
String inputString = "";
float values[4] = { 0, 0, 0, 0 };

float w1ref = -725.0;
float w2ref = 3013.0;

unsigned long startTime = 0;

// ===== 関数宣言 =====
void parseData(String data);
void updateLED();

// ===== セットアップ =====
void setup() {
  Serial.begin(9600);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.setRotation(3);
  display.clearDisplay();
  display.setFont(&FreeSansBold9pt7b);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println("Waiting UART...");
  display.display();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pixel.begin();
  pixel.setBrightness(50);
  pixel.clear();
  pixel.show();

  startTime = millis();
}


float datachk(float data, const char* unit) {
  if (data > -99999.0) {
    display.printf("%.1f %s\n", data, unit);
    return data;
  } else {
    display.println("---");
    return 0; // 「無効値」を明示
  }
}

// ===== ループ =====
void loop() {
  // UART受信処理
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n' || c == '\r') {
      if (inputString.length() > 0) {
        parseData(inputString);
        inputString = "";
      }
    } else {
      inputString += c;
    }
  }

  // ボタン処理
  if (digitalRead(BUTTON_PIN) == LOW && millis() - lastDebounceTime > debounceDelay) {
    ScaleOn = !ScaleOn;
    lastDebounceTime = millis();

    w1ref = values[0];
    w2ref = values[1];
    startTime = millis();

    updateLED();
  }

  // 経過時間の計算
  unsigned long elapsed = 0;
  if (ScaleOn) {
    elapsed = (millis() - startTime) / 1000;
  } else {
    elapsed = 0;
  }
  int minutes = elapsed / 60;
  int seconds = elapsed % 60;

  // 値の処理
  float wo1 = values[0] - w1ref;
  float wo2 = values[1] - w2ref;
  float t1 = values[2];
  float t2 = values[3];

  // OLED更新
  display.clearDisplay();
  display.setCursor(0, 20);
  if (ScaleOn) {
    display.printf("%02d:%02d\n", minutes, seconds);
  } else {
    display.print("--:--\n");
  }

  float totaladd = datachk(wo1 + wo2, "cc");
  float totalbrew = datachk(wo1, "cc");
  float kettletemp = datachk(t2, "C");
  float brewtemp = datachk(t1, "");

  display.display();

  delay(200);
}

// ===== データ解析 =====
void parseData(String data) {
  int lastIndex = 0;
  for (int i = 0; i < 4; i++) {
    int commaIndex = data.indexOf(',', lastIndex);
    String token;
    if (commaIndex == -1) {
      token = data.substring(lastIndex);
    } else {
      token = data.substring(lastIndex, commaIndex);
    }
    values[i] = token.toFloat();
    lastIndex = commaIndex + 1;
  }
}

// ===== LED制御 =====
void updateLED() {
  if (ScaleOn) {
    pixel.setPixelColor(0, pixel.Color(0, 0, 255));  // 青
  } else {
    pixel.setPixelColor(0, pixel.Color(0, 255, 0));  // 緑
  }
  pixel.show();
}
