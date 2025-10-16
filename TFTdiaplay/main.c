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

// タッチライブラリ
XPT2046_Touchscreen ts(TOUCH_CS);

// 色配列
uint16_t colors[] = { TFT_RED, TFT_GREEN, TFT_BLUE };
uint16_t colorIndex = 0;

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
}

void loop() {


  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    Serial.printf("X:%d Y:%d Z:%d\n", p.x, p.y, p.z);

    tft.setCursor(10, 10);
    tft.setTextColor(TFT_WHITE);
    tft.printf("X:%d Y:%d Z:%d", p.x, p.y, p.z);
  }

  // 四角形は定期的に表示
  tft.drawRect(38, 52, 240, 160, 0xFFFFFF);

  bool isOn = (digitalRead(PIN_SWITCH) == LOW); // GNDに落ちたらON

  tft.setCursor(80, 80);
  tft.setTextColor(TFT_WHITE);
  tft.print(isOn);
  delay(50);
}
