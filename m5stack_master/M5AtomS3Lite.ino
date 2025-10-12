/* AtomS3 Lite complete sketch with integrated web graph
   - 600-point buffer
   - ScaleON/OFF resets/starts graph
   - horizontal axis: elapsed seconds
   - vertical axes: volume 0~max500cc, temp 0~100C
   - Canvas height 80% of browser window
   - Digital display of volume/temp with elapsed MM:SS
   - OLED display unchanged
*/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Adafruit_NeoPixel.h>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SDA_PIN 38
#define SCL_PIN 39
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== UART =====
#define RX_PIN 1
#define TX_PIN 2
#define BAUD_RATE 9600

// ===== BUTTON & LED =====
#define BUTTON_PIN 41
#define LED_PIN 35
#define NUM_PIXELS 1
Adafruit_NeoPixel pixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

bool ScaleOn = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// ===== WiFi =====
const char* ssid = "E00EE4BA7CF1-2G";
const char* password = "fp23r4a37c7msx";

// ===== Web server =====
WebServer server(80);

// ===== Timeseries buffer =====
const int MAX_POINTS = 600;
float buf_totaladd[MAX_POINTS];
float buf_totalbrew[MAX_POINTS];
float buf_kettletemp[MAX_POINTS];
float buf_brewtemp[MAX_POINTS];
unsigned long buf_time[MAX_POINTS]; // seconds since start
int buf_idx = 0; // next write index
int buf_head = 0; // number of points stored

// ===== UART data =====
String inputString = "";
float values[4] = {0,0,0,0};
float w1ref = -725.0;
float w2ref = 3013.0;

// ===== timers =====
unsigned long startTime = 0;
unsigned long lastSecondTick = 0;

// ===== forward declarations =====
void parseData(String data);
void updateLED();
void handleRoot();
void handleData();
void serveNotFound();

// ===== setup =====
void setup() {
  Serial.begin(115200);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

  // OLED init
  Wire.begin(SDA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)){
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setRotation(3);
  display.setFont(&FreeSansBold9pt7b);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0,20);
  display.println("Waiting UART...");
  display.display();

  // button & LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pixel.begin();
  pixel.setBrightness(50);
  pixel.clear();
  pixel.show();

  // WiFi (DHCP)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password);
  Serial.print("Connecting WiFi ...");
  unsigned long tstart = millis();
  while(WiFi.status() != WL_CONNECTED && millis()-tstart<10000){
    delay(200); Serial.print(".");
  }
  Serial.println();
  if(WiFi.status()==WL_CONNECTED){
    Serial.print("WiFi IP: "); Serial.println(WiFi.localIP());
    if(MDNS.begin("coffeescale01")){
      Serial.println("mDNS responder started: coffeescale01");
      MDNS.addService("http","tcp",80);
    }else Serial.println("mDNS start failed");
  }else Serial.println("WiFi not connected");

  // Web server endpoints
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.onNotFound(serveNotFound);
  server.begin();

  // timers
  startTime = millis();
  lastSecondTick = millis();

  // clear buffers
  for(int i=0;i<MAX_POINTS;i++){
    buf_time[i]=0;
    buf_totaladd[i]=buf_totalbrew[i]=buf_kettletemp[i]=buf_brewtemp[i]=NAN;
  }

  updateLED();
}

// ===== loop =====
void loop(){
  // UART receive
  while(Serial1.available()){
    char c = Serial1.read();
    if(c=='\n'||c=='\r'){
      if(inputString.length()>0){
        parseData(inputString);
        inputString="";
      }
    }else inputString+=c;
  }

  // button
  if(digitalRead(BUTTON_PIN)==LOW && millis()-lastDebounceTime>debounceDelay){
    ScaleOn=!ScaleOn;
    lastDebounceTime=millis();
    w1ref=values[0];
    w2ref=values[1];
    if(ScaleOn) startTime=millis();
    else { // clear buffer on OFF
      buf_idx=0; buf_head=0;
      for(int i=0;i<MAX_POINTS;i++){
        buf_time[i]=0;
        buf_totaladd[i]=buf_totalbrew[i]=buf_kettletemp[i]=buf_brewtemp[i]=NAN;
      }
    }
    updateLED();
  }

  // 1s timer
  unsigned long now=millis();
  if(ScaleOn && now-lastSecondTick>=1000){
    lastSecondTick+=1000;
    unsigned long elapsed=(now-startTime)/1000;
    float wo1=values[0]-w1ref;
    float wo2=values[1]-w2ref;
    float t1=values[2];
    float t2=values[3];
float totaladd_v = max(0.0f, wo1 + wo2);
float totalbrew_v = max(0.0f, wo1);


    // store
    buf_time[buf_idx]=elapsed;
    buf_totaladd[buf_idx]=totaladd_v;
    buf_totalbrew[buf_idx]=totalbrew_v;
    buf_kettletemp[buf_idx]=t2;
    buf_brewtemp[buf_idx]=t1;
    buf_idx=(buf_idx+1)%MAX_POINTS;
    if(buf_head<MAX_POINTS) buf_head++;
  }

  server.handleClient();

  // OLED display (unchanged)
  unsigned long elapsed=(ScaleOn?(millis()-startTime)/1000:0);
  int minutes=elapsed/60;
  int seconds=elapsed%60;
  float wo1=values[0]-w1ref;
  float wo2=values[1]-w2ref;
  float t1=values[2];
  float t2=values[3];

  display.clearDisplay();
  display.setCursor(0,20);
  display.printf("%02d:%02d %s\n",minutes,seconds,ScaleOn?"ON":"OFF");
  display.printf("ADD:%.1f\n",wo1+wo2);
  display.printf("BRW:%.1f\n",wo1);
  display.printf("KET:%.1f\n",t2);
  display.printf("BRE:%.1f\n",t1);
  display.display();

  delay(10);
}

// ===== parseData =====
void parseData(String data){
  int lastIndex=0;
  for(int i=0;i<4;i++){
    int commaIndex=data.indexOf(',',lastIndex);
    String token=(commaIndex==-1)?data.substring(lastIndex):data.substring(lastIndex,commaIndex);
    values[i]=token.toFloat();
    lastIndex=commaIndex+1;
  }
}

// ===== LED =====
void updateLED(){
  pixel.setPixelColor(0,ScaleOn?pixel.Color(0,0,255):pixel.Color(0,255,0));
  pixel.show();
}

// ===== Web =====
void handleRoot() {
  String page = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Coffee Scale</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
body{
  background:#111;
  color:#eee;
  font-family:Arial,Helvetica,sans-serif;
  margin:16px;
  text-align:center;
}
canvas{
  background:#000;
  border:1px solid #555;
}
</style>
</head>
<body>
<h2>Coffee Scale</h2>
<div id="statusLine">---</div>
<canvas id="coffeeChart" width="600" height="300"></canvas>

<script>
let ctx = document.getElementById('coffeeChart').getContext('2d');

let chart = new Chart(ctx, {
  type: 'line',
  data: {
    datasets: [
      { label:'totaladd', yAxisID:'yWeight', borderColor:'cyan', data:[], showLine:true, pointRadius:2 },
      { label:'totalbrew', yAxisID:'yWeight', borderColor:'lime', data:[], showLine:true, pointRadius:2 },
      { label:'kettle', yAxisID:'yTemp', borderColor:'orange', data:[], showLine:true, pointRadius:2 },
      { label:'brew', yAxisID:'yTemp', borderColor:'red', data:[], showLine:true, pointRadius:2 }
    ]
  },
  options: {
    responsive:false,
    animation:false,
    scales:{
      x:{ type:'linear', title:{display:true,text:'Time (sec)'}, grid:{color:'#333'} },
      yWeight:{ position:'left', title:{display:true,text:'Volume (cc)'}, min:0, max:500, grid:{color:'#333'} },
      yTemp:{ position:'right', title:{display:true,text:'Temperature (°C)'}, min:50, max:100, grid:{drawOnChartArea:false} }
    },
    plugins:{
      legend:{labels:{color:'#eee'}},
      title:{display:false}
    }
  }
});

function updateChart(data){
  const t = data.time;
  const add = data.totaladd;
  const brew = data.totalbrew;
  const ket = data.kettle;
  const bre = data.brew;

  // 数値表示は常に最新
  let lastIdx = t.length ? t.length - 1 : 0;
  let latest = t.length ? t[lastIdx] : 0;
  let mm = Math.floor(latest/60);
  let ss = latest%60;
  let txt = 
    (mm.toString().padStart(2,'0')+":"+ss.toString().padStart(2,'0'))+" | "+
    "ADD:"+add[lastIdx].toFixed(1)+"cc | "+
    "BRW:"+brew[lastIdx].toFixed(1)+"cc | "+
    "KET:"+ket[lastIdx].toFixed(1)+"°C | "+
    "BRE:"+bre[lastIdx].toFixed(1)+"°C | "+
    (data.scaleOn ? "START" : "STOP");
  document.getElementById('statusLine').innerText = txt;

  // ScaleON時のみグラフ更新
  if(data.scaleOn){
    // 重量グラフの最大値を算出（100単位切り上げ）
    let maxWeight = Math.max(...add, ...brew);
    let yMax = Math.ceil(maxWeight / 100) * 100;
    if(yMax < 100) yMax = 100;
    if(yMax > 500) yMax = 500;
    chart.options.scales.yWeight.max = yMax;

    chart.data.datasets[0].data = t.map((x,i)=>({x:x,y:add[i]}));
    chart.data.datasets[1].data = t.map((x,i)=>({x:x,y:brew[i]}));
    chart.data.datasets[2].data = t.map((x,i)=>({x:x,y:ket[i]}));
    chart.data.datasets[3].data = t.map((x,i)=>({x:x,y:bre[i]}));

    chart.update();
  }
}


async function fetchData(){
  try{
    const res = await fetch('/data');
    if(!res.ok) throw 'network';
    const j = await res.json();
    updateChart(j);
  }catch(e){
    console.log("fetch error",e);
  }
}

setInterval(fetchData,1000);
fetchData();
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", page);
}


// ===== JSON data endpoint =====
void handleData(){
  String json="{";
  json += "\"scaleOn\":"; json+=(ScaleOn?"true":"false"); json+=",";
  json += "\"time\":[";
  for(int i=0;i<buf_head;i++){
    int idx = (buf_idx + i) % MAX_POINTS;
    if(buf_head<MAX_POINTS) idx=i;
    json += String(buf_time[idx]);
    if(i<buf_head-1) json+=",";
  }
  json+="],";

  auto appendArray = [&](float* arr,const char* name){
    json += "\""; json+=name; json+="\":[";
    for(int i=0;i<buf_head;i++){
      int idx = (buf_idx+i)%MAX_POINTS;
      if(buf_head<MAX_POINTS) idx=i;
      float v = arr[idx];
      if(isnan(v)) json += "0.0";
      else{
        if(strcmp(name,"totaladd")==0 || strcmp(name,"totalbrew")==0){
          v = min(v,500.0f); // 500cc超えない
        }
        json += String(v,1);
      }
      if(i<buf_head-1) json+=",";
    }
    json+="],";
  };

  appendArray(buf_totaladd,"totaladd");
  appendArray(buf_totalbrew,"totalbrew");
  appendArray(buf_kettletemp,"kettle");
  appendArray(buf_brewtemp,"brew");

  // remove last comma
  if(json.endsWith(",")) json.remove(json.length()-1);

  json+="}";
  server.send(200,"application/json",json);
}

void serveNotFound(){
  server.send(404,"text/plain","Not found");
}
