/* AtomS3 Lite complete sketch - Combined Graph Version
   - UART input (4 values)
   - OLED display
   - Button G41 toggles ScaleOn, G35 drives WS2812
   - Timeseries of totaladd, totalbrew, kettletemp, brewtemp
   - Web page with combined Chart.js graph
   - DHCP WiFi
   - mDNS name: coffeescale01
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

// ===== LED & BUTTON =====
#define BUTTON_PIN 41
#define LED_PIN 35
#define NUM_PIXELS 1
Adafruit_NeoPixel pixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

bool ScaleOn = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// ===== WiFi =====（書き換える）
const char* ssid = "SSID";
const char* password = "********";

// Web server
WebServer server(80);

// ===== Timeseries buffer =====
const int MAX_POINTS = 300;  // at least 60 sec
float buf_totaladd[MAX_POINTS];
float buf_totalbrew[MAX_POINTS];
float buf_kettletemp[MAX_POINTS];
float buf_brewtemp[MAX_POINTS];
unsigned long buf_time[MAX_POINTS];
int buf_head = 0; // number of points currently stored
int buf_idx = 0;  // next write index (circular)

// Data from UART
String inputString = "";
float values[4] = {0,0,0,0};
float w1ref = -725.0;
float w2ref = 3013.0;

// Timers
unsigned long startTime = 0;
unsigned long lastSecondTick = 0;

// ===== Forward declarations =====
void parseData(String data);
void updateLED();
void handleRoot();
void handleData();
void serveNotFound();

void setup() {
  Serial.begin(115200);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

  // OLED init
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.setRotation(3);
  display.setFont(&FreeSansBold9pt7b);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0,20);
  display.println("Waiting UART...");
  display.display();

  // Button & LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pixel.begin();
  pixel.setBrightness(50);
  pixel.clear();
  pixel.show();

  // WiFi DHCP only
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password);
  Serial.print("Connecting WiFi (DHCP) ...");
  unsigned long tstart = millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-tstart<10000){
    delay(200); Serial.print(".");
  }
  Serial.println();
  if(WiFi.status()==WL_CONNECTED){
    Serial.print("WiFi connected. IP: "); Serial.println(WiFi.localIP());
    if(MDNS.begin("coffeescale01")){
      Serial.println("mDNS responder started: coffeescale01");
      MDNS.addService("http","tcp",80);
    } else Serial.println("mDNS start failed");
  } else Serial.println("WiFi not connected.");

  // Web server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.onNotFound(serveNotFound);
  server.begin();

  // init timers
  startTime=millis();
  lastSecondTick=millis();

  // clear buffers
  for(int i=0;i<MAX_POINTS;i++){
    buf_time[i]=0;
    buf_totaladd[i]=buf_totalbrew[i]=buf_kettletemp[i]=buf_brewtemp[i]=NAN;
  }

  updateLED();
}

void loop() {
  // UART input
  while(Serial1.available()){
    char c = Serial1.read();
    if(c=='\n' || c=='\r'){
      if(inputString.length()>0){
        parseData(inputString);
        inputString="";
      }
    } else inputString += c;
  }

  // Button toggle ScaleOn
  if(digitalRead(BUTTON_PIN)==LOW && millis()-lastDebounceTime>debounceDelay){
    ScaleOn = !ScaleOn;
    lastDebounceTime = millis();
    w1ref = values[0];
    w2ref = values[1];
    if(ScaleOn) startTime=millis();
    updateLED();
  }

  // 1-sec sampling
  unsigned long now = millis();
  if(now-lastSecondTick>=1000){
    lastSecondTick+=1000;
    unsigned long elapsed = ScaleOn ? (now-startTime)/1000 : 0;

    float wo1=values[0]-w1ref;
    float wo2=values[1]-w2ref;
    float t1=values[2];
    float t2=values[3];

    float totaladd_v = max(0.0f, wo1+wo2);
    float totalbrew_v = max(0.0f, wo1);
    float kettle_v = t2;
    float brew_v = t1;

    // store circular buffer
    buf_time[buf_idx] = elapsed;
    buf_totaladd[buf_idx]=totaladd_v;
    buf_totalbrew[buf_idx]=totalbrew_v;
    buf_kettletemp[buf_idx]=kettle_v;
    buf_brewtemp[buf_idx]=brew_v;
    buf_idx=(buf_idx+1)%MAX_POINTS;
    if(buf_head<MAX_POINTS) buf_head++;
  }

  // handle web
  server.handleClient();

  // OLED display
  unsigned long elapsed = ScaleOn ? (millis()-startTime)/1000 : 0;
  int minutes = elapsed/60;
  int seconds = elapsed%60;
  float wo1=values[0]-w1ref;
  float wo2=values[1]-w2ref;

  display.clearDisplay();
  display.setCursor(0,20);
  if(ScaleOn) display.printf("%02d:%02d ON\n", minutes, seconds);
  else display.print("--:-- OFF\n");

  display.printf("ADD:%.1f\n", wo1+wo2);
  display.printf("BRW:%.1f\n", wo1);
  display.printf("KET:%.1f\n", values[3]);
  display.printf("BRW T:%.1f\n", values[2]);
  display.display();

  delay(10);
}

// ===== Parse UART =====
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
  if(ScaleOn) pixel.setPixelColor(0,pixel.Color(0,0,255));
  else pixel.setPixelColor(0,pixel.Color(0,255,0));
  pixel.show();
}

// ===== Web Handlers =====
void handleRoot(){
  String page = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Coffee Scale</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
body{font-family:Arial,sans-serif;margin:8px;background:#111;color:#eee}
canvas{background:#000;border:1px solid #444;padding:6px;}
</style>
</head>
<body>
<h3>Coffee Scale Live</h3>
<div>
Total Add: <strong id="totaladd">0.0</strong> cc |
Total Brew: <strong id="totalbrew">0.0</strong> cc |
Kettle Temp: <strong id="kettletemp">0.0</strong> °C |
Brew Temp: <strong id="brewtemp">0.0</strong> °C |
Status: <span id="status">---</span>
</div>
<canvas id="combinedChart" width="800" height="400"></canvas>

<script>
const ctx=document.getElementById('combinedChart').getContext('2d');
let chart=new Chart(ctx,{
  type:'line',
  data:{labels:[],datasets:[
    {label:'Total Add (cc)',data:[],borderColor:'cyan',fill:false,yAxisID:'yVol'},
    {label:'Total Brew (cc)',data:[],borderColor:'lime',fill:false,yAxisID:'yVol'},
    {label:'Kettle Temp (C)',data:[],borderColor:'orange',fill:false,yAxisID:'yTemp'},
    {label:'Brew Temp (C)',data:[],borderColor:'red',fill:false,yAxisID:'yTemp'}
  ]},
  options:{
    animation:false,
    scales:{
      yVol:{type:'linear',position:'left',min:0,suggestedMax:500,title:{display:true,text:'Volume (cc)'}},
      yTemp:{type:'linear',position:'right',min:50,suggestedMax:100,title:{display:true,text:'Temp (C)'}},
      x:{title:{display:true,text:'Seconds'}}
    }
  }
});

async function fetchData(){
  try{
    const res=await fetch('/data');
    const j=await res.json();
    document.getElementById('status').textContent=j.scaleOn?'RUN':'STOP';
    document.getElementById('totaladd').textContent=j.totaladd[j.totaladd.length-1].toFixed(1);
    document.getElementById('totalbrew').textContent=j.totalbrew[j.totalbrew.length-1].toFixed(1);
    document.getElementById('kettletemp').textContent=j.kettle[j.kettle.length-1].toFixed(1);
    document.getElementById('brewtemp').textContent=j.brew[j.brew.length-1].toFixed(1);

    chart.data.labels=j.time;
    chart.data.datasets[0].data=j.totaladd;
    chart.data.datasets[1].data=j.totalbrew;
    chart.data.datasets[2].data=j.kettle;
    chart.data.datasets[3].data=j.brew;
    chart.options.scales.yVol.max=Math.max(j.maxVol,500);
    chart.options.scales.yTemp.max=Math.max(j.maxTemp,100);
    chart.update();
  }catch(e){console.log("Fetch error:",e);}
}
setInterval(fetchData,1000);
fetchData();
</script>
</body>
</html>
)rawliteral";

  server.send(200,"text/html",page);
}

void handleData(){
  int count=buf_head;
  String json="{\"scaleOn\":";
  json+=(ScaleOn?"true":"false");
  json+=",\"time\":[";
  for(int i=0;i<count;i++){
    int idx=(buf_idx+i)%MAX_POINTS;
    if(buf_head<MAX_POINTS) idx=i;
    json+=String(buf_time[idx]);
    if(i<count-1) json+=",";
  }
  json+="],";
  auto appendArray=[&](float* arr,const char* name){
    json+="\"";json+=name;json+="\":[";
    for(int i=0;i<count;i++){
      int idx=(buf_idx+i)%MAX_POINTS;
      if(buf_head<MAX_POINTS) idx=i;
      json+=isnan(arr[idx])?"0.0":String(arr[idx],1);
      if(i<count-1) json+=",";
    }
    json+="],";
  };
  appendArray(buf_totaladd,"totaladd");
  appendArray(buf_totalbrew,"totalbrew");
  appendArray(buf_kettletemp,"kettle");
  appendArray(buf_brewtemp,"brew");

  float maxVol=0,maxTemp=0;
  for(int i=0;i<count;i++){
    int idx=(buf_idx+i)%MAX_POINTS; if(buf_head<MAX_POINTS) idx=i;
    if(buf_totaladd[idx]>maxVol) maxVol=buf_totaladd[idx];
    if(buf_totalbrew[idx]>maxVol) maxVol=buf_totalbrew[idx];
    if(buf_kettletemp[idx]>maxTemp) maxTemp=buf_kettletemp[idx];
    if(buf_brewtemp[idx]>maxTemp) maxTemp=buf_brewtemp[idx];
  }
  json+="\"maxVol\":"+String(maxVol,1)+",";
  json+="\"maxTemp\":"+String(maxTemp,1)+"}";
  server.send(200,"application/json",json);
}

void serveNotFound(){
  server.send(404,"text/plain","Not found");
}
