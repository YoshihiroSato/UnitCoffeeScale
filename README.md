# UnitCoffeeScale
## 概要
M5STACK用コーヒースケールユニット  
M5STACK用に多機能コーヒースケールユニットを作りました。  
Grove端子でUART接続します。
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
- 3.3V(ATmega328Pで生成)  
  ATmega328P:3V3
  双方向ロジックレベル変換モジュール:VL  
  JSTPH-1:PIN2  
  JSTPH-2:PIN2  
  プルアップ抵抗4.7k-1:VCC  
  プルアップ抵抗4.7k-2:VCC
### 信号線
- ロードセル-1信号線  
  赤:HX711-1:E+    
  黒:HX711-1:E−  
  白:HX711-1:S+  
  緑:HX711-1:S−    
- ロードセル-2信号線  
  赤:HX711-2:E+    
  黒:HX711-2:E−  
  白:HX711-2:S+  
  緑:HX711-2:S−    
