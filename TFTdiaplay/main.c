#include <LovyanGFX.hpp>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

#define TFT_BL 21
#define TOUCH_CS 15

#define PIN_SWITCH 47


// LGFX初期化
class MyLGFX : public lgfx::LGFX_Device {
  lgfx::Bus_SPI bus_spi;
  lgfx::Panel_ILI9341 panel;

public:
  MyLGFX() {
    auto cfg = bus_spi.config();
    cfg.spi_host = SPI3_HOST;
    cfg.freq_write = 40000000;
    cfg.freq_read = 16000000;
    cfg.pin_mosi = 18;
    cfg.pin_miso = 8;
    cfg.pin_sclk = 17;
    cfg.pin_dc = 6;
    bus_spi.config(cfg);
    panel.setBus(&bus_spi);

    auto pcfg = panel.config();
    pcfg.pin_cs = 5;
    pcfg.pin_rst = 7;
    pcfg.panel_width = 240;
    pcfg.panel_height = 320;
    panel.config(pcfg);
    setPanel(&panel);
  }
};

MyLGFX tft;

volatile bool switchChanged = false;
volatile bool currentState = HIGH;

bool ScaleOn = false;

unsigned long startTime = 0;

// タッチライブラリ
XPT2046_Touchscreen ts(TOUCH_CS);

// 色配列
uint16_t colors[] = { TFT_RED, TFT_GREEN, TFT_BLUE };
uint16_t colorIndex = 0;


void IRAM_ATTR handleSwitch() {
  currentState = digitalRead(PIN_SWITCH);
  switchChanged = true;
}



void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);

  ts.begin();
  ts.setRotation(1);

  pinMode(PIN_SWITCH, INPUT_PULLUP);  // 内蔵プルアップ


  attachInterrupt(digitalPinToInterrupt(PIN_SWITCH), handleSwitch, CHANGE);
}

void loop() {

  if (switchChanged) {
    switchChanged = false;
    if (currentState == LOW) {
      ScaleOn = false;
    } else {
      ScaleOn = true;
      startTime = millis();
    }
  }

  tft.drawRect(38, 52, 240, 160, 0xFFFFFF);
  tft.setFont(&fonts::Font7);
  tft.setTextSize(0.7, 1);

  tft.setCursor(0, 0);
  if (ScaleOn) {
    tft.setTextColor(0x55CCFF, TFT_BLACK);
    unsigned long elapsed = (millis() - startTime) / 1000;  // 秒単位
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;
    char buf[10];
    sprintf(buf, "%02d:%02d", minutes, seconds);
    tft.print(buf);
  } else {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.print("00:00");
  }



  tft.setFont(&fonts::FreeSerifBold18pt7b);
  tft.setTextSize(1, 1);

  tft.setCursor(108, 0);
  tft.setTextColor(0xFFF71D);
  tft.print("000.0cc");

  tft.setCursor(108, 23);
  tft.setTextColor(0xFCFFAA);
  tft.print("000.0cc");

  tft.setCursor(223, 0);
  tft.setTextColor(0xFFBDBD);
  tft.print("00.0C");

  tft.setCursor(223, 23);
  tft.setTextColor(0xFFEEEE);
  tft.print("00.0C");





  bool isOn = (digitalRead(PIN_SWITCH) == LOW);  // GNDに落ちたらON

  tft.setCursor(80, 80);
  tft.setTextColor(TFT_WHITE);
  tft.print(isOn);
  delay(50);
}
