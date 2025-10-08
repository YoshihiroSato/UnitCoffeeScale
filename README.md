# UnitCoffeeScale

UnitCoffeeScale/  
├── arduino_slave/                ← コーヒースケール用 Arduino コード  
│   └── UnitCoffeeScale.ino  
│  
├── docs/                          ← 説明資料  
│   └── wiring_diagram.md         ← 君が送った配線図・仕様書  
│  
├── m5stack_master/               ← M5Stack 側のマスターコード  
│   └── master.py  
│  
├── README.md                      ← プロジェクト概要  
├── LICENSE                        ← ライセンス  
└── .gitignore  
  

## 概要
M5STACK用コーヒースケールユニット  
M5STACK用に多機能コーヒースケールユニットを作りました。  
Grove端子でUART接続します。重量計が2つを内蔵、温度計2つを搭載できます。
## 測定できるもの
- ドリッパー内重量
- サーバー内重量
- ケトル内湯温
- ドリップ後のサーバー内の湯温
## 構成
- コア  
  ATmega328P
- 重量センサx2  
  ロードセル10kg  
  HX711(5V駆動)
- 温度計x2  
  DS18B20(JSTPH3Pコネクタ経由)
  プルアップ4.7k
- レベルシフター  
  双方向ロジックレベル変換モジュール 4チャンネル 5V ⇔ 3.3V
- Grove端子
  Grove to 4P
## 配線
### 電源
- GND  
  ATmega328P:GND  
  Grove端子:PIN4  
  HX711-1:GND  
  HX711-2:GND  
  JSTPH-1:PIN3  
  JSTPH-2:PIN3  
  双方向ロジックレベル変換モジュール:GNDx2
- 5V  
  ATmega328P:5V 
  Grove端子:PIN3  
  HX711-1:VCC  
  HX711-2:VCC  
  双方向ロジックレベル変換モジュール:VH  
  JSTPH-1:PIN2  
  JSTPH-2:PIN2  
  プルアップ抵抗4.7k-1:VCC  
  プルアップ抵抗4.7k-2:VCC 
- 3.3V(ATmega328Pで生成)  
  ATmega328P:3V3
  双方向ロジックレベル変換モジュール:VL  
### 信号線
- ロードセル-1信号線  
  赤 to HX711-1:E+    
  黒 to HX711-1:E−  
  白 to HX711-1:S+  
  緑 to HX711-1:S−    
- ロードセル-2信号線  
  赤 to HX711-2:E+    
  黒 to HX711-2:E−  
  白 to HX711-2:S+  
  緑 to HX711-2:S−    
- HX711-1信号線  
  HX711-1:CLK to ATmega328P:D2    
  HX711-1:D to ATmega328P:D3  
- HX711-2信号線  
  HX711-2:CLK to ATmega328P:D4  
  HX711-2:D to ATmega328P:D5  
- DS18B20信号線  
  JSTPH-1:PIN1 to ATmega328P:D6    
  JSTPH-2:PIN1 to ATmega328P:D7
  プルアップ抵抗4.7k-1:D to JSTPH-1:PIN1  
  プルアップ抵抗4.7k-2:D to JSTPH-2:PIN1 
- UART
  Grove端子:PIN1 to 双方向ロジックレベル変換モジュール:DL1  
  Grove端子:PIN2 to 双方向ロジックレベル変換モジュール:DL2  
  双方向ロジックレベル変換モジュール:DH1  to  ATmega328P:11
  双方向ロジックレベル変換モジュール:DH2  to  ATmega328P:12
  
