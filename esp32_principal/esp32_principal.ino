// RASTRO — ESP32 principal. Sensor PIR + buzzer ativo + LCD I2C.
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include <esp_system.h>
#include "pagina.h"
#include "config.h"  // WIFI_SSID, WIFI_PASS e CAM_IP (arquivo fora do Git)
String CAM_IP = CAM_IP_PADRAO;

const int PIN_PIR    = 14;
const int PIN_BUZZER = 15;
// LCD I2C: SDA = 21, SCL = 22 (padrão). Endereço 0x27 ou 0x3F.
LiquidCrystal_I2C lcd(0x27, 16, 2);
// =========================


WebServer server(80);
bool armado=true, sirene=false, movimento=false, anterior=false;
bool automatico=true;
uint32_t inicioSirene=0, ultimaReconexao=0;
const uint32_t TEMPO_SIRENE=15000, AQUECIMENTO=30000;
const size_t LIMITE=100;
struct Evento { uint32_t id, uptime; char data[32]; const char* tipo; };
Evento eventos[LIMITE];
size_t quantidade=0, proximo=0;
uint32_t sequencia=0, totalMovimentos=0;
String sessao, ultimo="—";

// Procura a ESP32-CAM na rede: testa o IP configurado e depois um IP por vez
// da mesma sub-rede, ate achar quem responda /status como o CameraWebServer.
bool camEncontrada=false; uint8_t camBusca=1; uint32_t ultimaBusca=0;
bool ehCamera(IPAddress ip) {
  WiFiClient c;
  if(!c.connect(ip,80,120)) return false;
  c.print("GET /status HTTP/1.0\r\nHost: cam\r\n\r\n");
  uint32_t t=millis(); String r;
  while(millis()-t<400&&r.length()<800) {
    while(c.available()) r+=(char)c.read();
    if(!c.connected()&&!c.available()) break;
    delay(1);
  }
  c.stop();
  return r.indexOf("framesize")>=0;
}
void procurarCamera() {
  if(camEncontrada||WiFi.status()!=WL_CONNECTED||millis()-ultimaBusca<50) return;
  ultimaBusca=millis();
  static bool testouPadrao=false;
  IPAddress ip;
  if(!testouPadrao) { testouPadrao=true; if(!ip.fromString(CAM_IP)) return; }
  else {
    ip=WiFi.localIP();
    if(camBusca==0||camBusca==255) camBusca=1;
    ip[3]=camBusca++;
    if(ip==WiFi.localIP()) return;
  }
  if(ehCamera(ip)) {
    camEncontrada=true; CAM_IP=ip.toString();
    Serial.println("ESP32-CAM encontrada: http://"+CAM_IP);
  }
}

String dataAgora() {
  time_t agora=time(nullptr);
  if (agora<1700000000) return "Sem horario sincronizado";
  struct tm t; localtime_r(&agora,&t);
  char buf[24]; strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",&t);
  return String(buf);
}
void registrar(const char* tipo) {
  Evento &e=eventos[proximo];
  e.id=++sequencia; e.uptime=millis(); e.tipo=tipo;
  dataAgora().toCharArray(e.data,sizeof(e.data));
  proximo=(proximo+1)%LIMITE;
  if(quantidade<LIMITE) quantidade++;
  if(String(tipo)=="movimento") { totalMovimentos++; ultimo=e.data; }
}
void responder(int codigo,const String &json) {
  server.sendHeader("Cache-Control","no-store");
  server.send(codigo,"application/json; charset=utf-8",json);
}
void status() {
  String j; j.reserve(512);
  j="{\"sessao\":\""+sessao+"\",\"hora\":\""+dataAgora()+"\",\"ultimo\":\""+ultimo+"\"";
  j+=",\"armado\":"+String(armado?"true":"false");
  j+=",\"sirene\":"+String(sirene?"true":"false");
  j+=",\"automatico\":"+String(automatico?"true":"false");
  j+=",\"movimento\":"+String(movimento?"true":"false");
  j+=",\"aquecendo\":"+String(millis()<AQUECIMENTO?"true":"false");
  j+=",\"uptime\":"+String(millis())+",\"total\":"+String(totalMovimentos);
  j+=",\"cam\":\""+CAM_IP+"\"";
  j+=",\"rssi\":"+String(WiFi.RSSI())+",\"heap\":"+String(ESP.getFreeHeap())+"}";
  responder(200,j);
}
void historico() {
  String j; j.reserve(16000); j="{\"sessao\":\""+sessao+"\",\"eventos\":[";
  for(size_t i=0;i<quantidade;i++) {
    const Evento &e=eventos[(proximo+LIMITE-quantidade+i)%LIMITE];
    if(i) j+=",";
    j+="{\"id\":"+String(e.id)+",\"uptime\":"+String(e.uptime)+",\"data\":\""+String(e.data)+"\",\"tipo\":\""+e.tipo+"\"}";
  }
  responder(200,j+"]}");
}
void comando() {
  String c=server.arg("c"),v=server.arg("v");
  if((v!="0"&&v!="1")||(c!="arm"&&c!="sirene"&&c!="automatico")) {
    responder(400,"{\"erro\":\"Comando invalido\"}"); return;
  }
  bool ligado=v=="1";
  if(c=="arm"&&armado!=ligado) {
    armado=ligado;
    if(!armado) { sirene=false; movimento=false; }
    registrar(armado?"armado":"desarmado");
  } else if(c=="sirene"&&sirene!=ligado) {
    sirene=ligado; inicioSirene=millis(); registrar(sirene?"sirene_on":"sirene_off");
  } else if(c=="automatico"&&automatico!=ligado) {
    automatico=ligado; registrar(automatico?"auto_on":"auto_off");
  }
  digitalWrite(PIN_BUZZER,sirene?HIGH:LOW);
  status();
}
void setup() {
  Serial.begin(115200); pinMode(PIN_PIR,INPUT); pinMode(PIN_BUZZER,OUTPUT);
  digitalWrite(PIN_BUZZER,LOW);
  lcd.init(); lcd.backlight(); lcd.print("Conectando WiFi");
  sessao=String(esp_random(),HEX)+String(esp_random(),HEX);
  WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true);
  // Diagnostico: lista as redes que a placa enxerga (so 2.4 GHz)
  int redes=WiFi.scanNetworks();
  Serial.printf("\nRASTRO: %d redes Wi-Fi encontradas\n",redes);
  for(int i=0;i<redes;i++) Serial.printf("  %-32s %4d dBm\n",WiFi.SSID(i).c_str(),WiFi.RSSI(i));
  WiFi.onEvent([](WiFiEvent_t e, WiFiEventInfo_t info){
    Serial.printf("Wi-Fi caiu, motivo %d\n", info.wifi_sta_disconnected.reason);  // 15/202 = senha errada, 201 = rede nao encontrada
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  Serial.printf("Conectando em \"%s\"...\n",WIFI_SSID);
  WiFi.begin(WIFI_SSID,WIFI_PASS);
  configTime(-3*3600,0,"pool.ntp.org");
  server.on("/",HTTP_GET,[](){
    String p=FPSTR(PAGINA); p.replace("__CAM__",CAM_IP);
    server.send(200,"text/html; charset=utf-8",p);
  });
  server.on("/status",HTTP_GET,status);
  server.on("/eventos",HTTP_GET,historico);
  server.on("/cmd",HTTP_POST,comando);
  server.onNotFound([](){responder(404,"{\"erro\":\"Nao encontrado\"}");});
  server.begin(); registrar("inicio");
}
void loop() {
  server.handleClient(); uint32_t agora=millis();
  procurarCamera();
  bool leitura=digitalRead(PIN_PIR)==HIGH;
  bool ativo=leitura&&armado&&agora>=AQUECIMENTO;
  if(ativo&&!anterior) {
    registrar("movimento");
    if(automatico) {sirene=true; inicioSirene=agora;}
  }
  anterior=ativo; movimento=ativo;
  if(sirene&&uint32_t(agora-inicioSirene)>=TEMPO_SIRENE) {sirene=false; registrar("sirene_off");}
  digitalWrite(PIN_BUZZER,sirene?HIGH:LOW);
  static bool avisouIP=false;
  if(WiFi.status()==WL_CONNECTED) {
    if(!avisouIP) { avisouIP=true; Serial.print("Painel RASTRO: http://"); Serial.println(WiFi.localIP()); }
  } else {
    avisouIP=false;
    if(uint32_t(agora-ultimaReconexao)>15000) {
      ultimaReconexao=agora;
      Serial.printf("Wi-Fi sem conexao (status %d), tentando de novo...\n",WiFi.status());
      WiFi.disconnect(); WiFi.begin(WIFI_SSID,WIFI_PASS);
    }
  }
  static uint32_t lcdAnterior=0;
  if(uint32_t(agora-lcdAnterior)>1000) {
    lcdAnterior=agora;
    String linha=agora<AQUECIMENTO?"Calibrando PIR":(movimento?"MOVIMENTO!":(armado?"Sistema armado":"Desarmado"));
    lcd.setCursor(0,0); lcd.print(linha+String("                ").substring(linha.length()));
    String ip=WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():"Sem WiFi";
    lcd.setCursor(0,1); lcd.print(ip+String("                ").substring(ip.length()));
  }
  delay(2);
}
