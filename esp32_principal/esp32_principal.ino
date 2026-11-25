// =============================================================================
// RASTRO — firmware do ESP32 principal
// Instalação interativa sobre vigilância (Mostra PIC · UFES · 2026)
// Autores: Arthur Gabriel de Souza Costa, Lucas Grillo Lyra, Frederino José de Castro
//
// O que esta placa faz:
//   * lê o sensor PIR (GPIO14) e agrupa os sinais em "ocorrências" de movimento;
//   * aciona o buzzer ativo (GPIO15) conforme o modo de operação escolhido;
//   * mostra estado, modo e IP no LCD 16x2 I2C (endereço 0x27, SDA 21 / SCL 22);
//   * guarda os últimos 100 eventos, o modo e a programação noturna na flash (NVS);
//   * serve o painel web (pagina.h) e a API HTTP: GET /status, GET /eventos, POST /cmd;
//   * procura sozinha a ESP32-CAM na rede local para o painel mostrar o vídeo.
//
// Placa no Arduino IDE: ESP32 Dev Module · Partition Scheme: Huge APP · PSRAM Disabled.
// O partitions.csv desta pasta cria a partição "rastro_data" usada pelo histórico.
// =============================================================================
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
// Módulos do projeto (todos nesta pasta; não separe os .h do .ino):
#include "pagina.h"
#include "pagina_http.h"
#include "ocorrencia.h"
#include "historico.h"
#include "modos.h"
#include "noturno.h"
#include "config.h"  // WIFI_SSID, WIFI_PASS e CAM_IP (arquivo fora do Git)
// Endereço atual da câmera. Começa com o valor do config.h e é trocado
// automaticamente se procurarCamera() achar a ESP32-CAM em outro IP.
String CAM_IP = CAM_IP_PADRAO;

// ---- Pinos do circuito ----
const int PIN_PIR    = 14;
const int PIN_BUZZER = 15;
// LCD I2C: SDA = 21, SCL = 22 (padrão). Endereço 0x27 ou 0x3F.
LiquidCrystal_I2C lcd(0x27, 16, 2);
// ---- LCD em tarefa separada (FreeRTOS) ----
// Escrever no LCD via I2C é lento e pode travar se o módulo for desconectado.
// Por isso o loop principal só deposita a mensagem numa fila de 1 posição e
// uma tarefa própria escreve na tela, sem atrasar a leitura do sensor.
// BEGIN_DISPLAY_TASK
struct MensagemLCD { char linhas[2][17]; };
QueueHandle_t filaLCD=nullptr;
std::atomic<bool> lcdDisponivel{false};
// Tarefa que vive para sempre: pega a última mensagem, confere se o LCD
// responde no barramento e reescreve só as linhas que mudaram.
void tarefaLCD(void*){
  Wire.begin(21,22);Wire.setTimeOut(10);
  MensagemLCD desejada{},mostrada{};bool iniciou=false;
  for(;;){
    MensagemLCD nova;
    if(xQueueReceive(filaLCD,&nova,pdMS_TO_TICKS(1000))==pdPASS)desejada=nova;
    Wire.beginTransmission(0x27);
    if(Wire.endTransmission()!=0){lcdDisponivel.store(false);iniciou=false;continue;}
    if(!iniciou){lcd.init();Wire.setTimeOut(10);lcd.backlight();iniciou=true;memset(&mostrada,0,sizeof(mostrada));}
    for(uint8_t linha=0;linha<2;linha++)if(memcmp(desejada.linhas[linha],mostrada.linhas[linha],17)!=0){
      lcd.setCursor(0,linha);lcd.print(desejada.linhas[linha]);memcpy(mostrada.linhas[linha],desejada.linhas[linha],17);
    }
    Wire.beginTransmission(0x27);const bool ok=Wire.endTransmission()==0;
    lcdDisponivel.store(ok);if(!ok)iniciou=false;
  }
}
// Cria a fila e a tarefa do LCD (chamada uma vez no setup).
void iniciarLCD(){
  filaLCD=xQueueCreate(1,sizeof(MensagemLCD));
  if(filaLCD&&xTaskCreate(tarefaLCD,"rastro-lcd",4096,nullptr,1,nullptr)!=pdPASS){vQueueDelete(filaLCD);filaLCD=nullptr;}
}
// Envia duas linhas (16 colunas cada) para a tarefa do LCD.
// Mensagens repetidas são ignoradas para não ocupar o barramento à toa.
void publicarLCD(const String& primeira,const String& segunda){
  if(!filaLCD)return;
  MensagemLCD mensagem{};const String* linhas[]={&primeira,&segunda};
  for(size_t l=0;l<2;l++){for(size_t c=0;c<16;c++)mensagem.linhas[l][c]=c<linhas[l]->length()?(*linhas[l])[c]:' ';mensagem.linhas[l][16]=0;}
  static MensagemLCD anteriorLCD{};
  if(memcmp(&mensagem,&anteriorLCD,sizeof(mensagem))!=0&&xQueueOverwrite(filaLCD,&mensagem)==pdPASS)anteriorLCD=mensagem;
}
// END_DISPLAY_TASK
// =========================


// ---- Servidor HTTP com fila de conexões ----
// O WebServer padrão atende um cliente por vez e pode ficar preso esperando
// uma conexão que ainda não enviou dados. Esta classe guarda até 4 conexões
// em espera, atende primeiro quem já mandou a requisição (em rodízio) e
// descarta conexões ociosas há mais de 5 s. Assim vários celulares podem
// abrir o painel ao mesmo tempo sem travar a placa.
// BEGIN_HTTP_SCHEDULER
class ServidorRastro:public WebServer {
  NetworkClient esperando[4];uint32_t chegada[4]={};uint8_t turno=0;
  void atender(NetworkClient cliente,uint32_t agora){
    cliente.setNoDelay(true);
    _currentClient=cliente;_currentStatus=HC_WAIT_READ;_statusChange=agora;
    WebServer::handleClient();
  }
public:
  using WebServer::WebServer;
  uint32_t ociosasDescartadas=0;
  void handleClient()override{
    if(_currentStatus!=HC_NONE){WebServer::handleClient();return;}
    const uint32_t agora=millis();
    for(size_t i=0;i<4;i++){
      const int bytes=esperando[i].available();
      if(!esperando[i].connected()&&bytes==0){esperando[i].stop();esperando[i]=NetworkClient();}
      else if(bytes==0&&uint32_t(agora-chegada[i])>=5000){esperando[i].stop();esperando[i]=NetworkClient();++ociosasDescartadas;}
    }
    for(size_t n=0;n<4;n++){
      const size_t i=(turno+n)%4;
      if(esperando[i].available()>0){
        NetworkClient cliente=esperando[i];esperando[i]=NetworkClient();turno=(i+1)%4;
        atender(cliente,agora);return;
      }
    }
    NetworkClient novo=_server.accept();if(!novo)return;
    if(novo.available()>0){atender(novo,agora);return;}
    size_t slot=4;
    for(size_t i=0;i<4;i++)if(!esperando[i].connected()){slot=i;break;}
    if(slot==4){
      slot=0;for(size_t i=1;i<4;i++)if(uint32_t(agora-chegada[i])>uint32_t(agora-chegada[slot]))slot=i;
      esperando[slot].stop();++ociosasDescartadas;
    }
    esperando[slot]=novo;chegada[slot]=agora;
  }
};
// END_HTTP_SCHEDULER
// ---- Estado global do sistema ----
// armado = registro ligado (false = pausado); sirene = buzzer tocando agora;
// movimento = PIR ativo neste instante; anterior = leitura do ciclo anterior.
ServidorRastro server(80);
bool armado=true, sirene=false, movimento=false, anterior=false;
bool automatico=false;
ModoOperacao modo=ModoOperacao::Monitoramento;
ControleSirene alarme;
ProgramacaoNoturna noturno;bool noturnoSalvo=false,noturnoAplicado=true;
bool operacaoSalva=false;
uint32_t ultimaReconexao=0;
// O HC-SR501 precisa de ~30 s após ligar para estabilizar (calibração).
const uint32_t AQUECIMENTO=30000;
const size_t LIMITE=100;

// Buffer circular com os últimos 100 eventos (ocorrências, mudanças de modo,
// sirene, pausa...). 'proximo' aponta a posição que será sobrescrita.
Evento eventos[LIMITE];
size_t quantidade=0, proximo=0;
uint32_t sequencia=0, revisao=0, totalMovimentos=0, totalOcorrencias=0;
AgrupadorOcorrencia agrupador;
size_t slotOcorrencia=0; uint32_t idOcorrencia=0;
String sessao, ultimo="—";
// Leitura e gravação do histórico na flash (depende das variáveis acima).
#include "persistencia.h"

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

// Data e hora atuais em texto. Antes da sincronização NTP o relógio da
// placa marca 1970, então devolvemos um aviso em vez de uma data falsa.
String dataAgora() {
  time_t agora=time(nullptr);
  if (agora<1700000000) return "Sem horario sincronizado";
  struct tm t; localtime_r(&agora,&t);
  char buf[24]; strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",&t);
  return String(buf);
}
// Acrescenta um evento ao buffer circular e marca para salvar na flash.
void registrar(const char* tipo) {
  Evento &e=eventos[proximo];
  e.id=++sequencia;e.ordem=e.id; ++revisao; e.uptime=millis(); e.tipo=tipo;
  sessao.toCharArray(e.origem,sizeof(e.origem));e.interrompida=false;
  const time_t relogio=time(nullptr);e.epoch=relogio>=1700000000?uint32_t(relogio):0;
  e.ultimoUptime=e.uptime; e.deteccoes=0; e.aberta=false;
  dataAgora().toCharArray(e.data,sizeof(e.data));
  marcarHistorico(proximo,true);
  proximo=(proximo+1)%LIMITE;
  if(quantidade<LIMITE) quantidade++;
  if(String(tipo)=="movimento") { totalOcorrencias++; ultimo=e.data; }
}
// Encerra a ocorrência em andamento (usado ao pausar o registro).
void fecharOcorrencia() {
  if(idOcorrencia && eventos[slotOcorrencia].id==idOcorrencia) {
    Evento &e=eventos[slotOcorrencia];
    e.ultimoUptime=agrupador.ultimo; e.deteccoes=agrupador.deteccoes;
    if(e.aberta) { e.aberta=false; ++revisao; marcarHistorico(slotOcorrencia,true); }
  }
  agrupador.encerrar(); idOcorrencia=0;
}
// Passa a leitura do PIR para o agrupador (ocorrencia.h) e reflete o resultado
// no evento correspondente. Devolve true quando começou uma ocorrência nova
// (é nesse momento que a sirene pode tocar).
bool atualizarOcorrencia(uint32_t agora, bool ativo) {
  const auto mudanca=agrupador.atualizar(agora,ativo);
  if(mudanca==AgrupadorOcorrencia::Inicio) {
    if(idOcorrencia && eventos[slotOcorrencia].id==idOcorrencia) {eventos[slotOcorrencia].aberta=false;marcarHistorico(slotOcorrencia,true);}
    slotOcorrencia=proximo; registrar("movimento"); idOcorrencia=eventos[slotOcorrencia].id;
    eventos[slotOcorrencia].uptime=agrupador.inicio;
  }
  if(mudanca!=AgrupadorOcorrencia::Nenhuma && idOcorrencia && eventos[slotOcorrencia].id==idOcorrencia) {
    Evento &e=eventos[slotOcorrencia];
    e.ultimoUptime=agrupador.ultimo; e.deteccoes=agrupador.deteccoes; e.aberta=agrupador.aberta;
    marcarHistorico(slotOcorrencia,mudanca==AgrupadorOcorrencia::Fim);
    ++revisao;
  }
  if(mudanca==AgrupadorOcorrencia::Fim) idOcorrencia=0;
  return mudanca==AgrupadorOcorrencia::Inicio;
}
// Resposta JSON sem cache, para o painel sempre ver o estado atual.
void responder(int codigo,const String &json) {
  server.sendHeader("Cache-Control","no-store");
  server.send(codigo,"application/json; charset=utf-8",json);
}
// GET /status — estado completo da placa em JSON (consultado pelo painel a
// cada 1,5 s): modo, sensor, contadores, saúde da memória, Wi-Fi, IP da câmera
// e programação noturna.
void status() {
  String j; j.reserve(512);
  j="{\"sessao\":\""+sessao+"\",\"hora\":\""+dataAgora()+"\",\"ultimo\":\""+ultimo+"\"";
  j+=",\"armado\":"+String(armado?"true":"false");
  j+=",\"sirene\":"+String(sirene?"true":"false");
  j+=",\"automatico\":"+String(automatico?"true":"false");
  j+=",\"modo\":\""+String(idModo(modo))+"\",\"modoNome\":\""+String(nomeModo(modo))+"\"";
  j+=",\"modoSalvo\":"+String(operacaoSalva?"true":"false")+",\"tempoSirene\":"+String(tempoSom(modo));
  j+=",\"movimento\":"+String(movimento?"true":"false");
  j+=",\"aquecendo\":"+String(millis()<AQUECIMENTO?"true":"false");
  j+=",\"uptime\":"+String(millis())+",\"total\":"+String(totalOcorrencias);
  j+=",\"deteccoes\":"+String(totalMovimentos)+",\"intervaloOcorrencia\":10000";
  j+=",\"revisao\":"+String(revisao);
  j+=",\"historicoPersistente\":"+String(discoDisponivel&&discoSaudavel?"true":"false");
  j+=",\"historicoPendente\":"+String(historicoPendente()?"true":"false");
  j+=",\"historicoRecuperados\":"+String(recuperadosHistorico)+",\"historicoInvalidos\":"+String(corrompidosHistorico);
  j+=",\"cam\":\""+CAM_IP+"\"";
  j+=",\"rssi\":"+String(WiFi.RSSI())+",\"heap\":"+String(ESP.getFreeHeap());
  j+=",\"lcdDisponivel\":"+String(lcdDisponivel.load()?"true":"false");
  wifi_ps_type_t economiaWiFi;
  if(esp_wifi_get_ps(&economiaWiFi)==ESP_OK)j+=",\"wifiPowerSave\":"+String(int(economiaWiFi));
  j+=statusNoturno();
  j+=",\"httpOciosasDescartadas\":"+String(server.ociosasDescartadas)+"}";
  responder(200,j);
}
// GET /eventos — lista dos eventos guardados, do mais antigo ao mais recente.
void historico() {
  String j; j.reserve(16000); j="{\"sessao\":\""+sessao+"\",\"eventos\":[";
  for(size_t i=0;i<quantidade;i++) {
    const Evento &e=eventos[(proximo+LIMITE-quantidade+i)%LIMITE];
    if(i) j+=",";
    j+="{\"id\":"+String(e.id)+",\"uptime\":"+String(e.uptime)+",\"data\":\""+String(e.data)+"\",\"tipo\":\""+e.tipo+"\",\"ultimoUptime\":"+String(e.ultimoUptime)+",\"deteccoes\":"+String(e.deteccoes)+",\"aberta\":"+String(e.aberta?"true":"false")+",\"sessao\":\""+String(e.origem)+"\",\"interrompida\":"+String(e.interrompida?"true":"false")+"}";
  }
  responder(200,j+"]}");
}
// Salva modo + pausa juntos na flash. Só confirma a mudança se a gravação deu certo.
bool guardarOperacao(ModoOperacao novoModo,bool novoArmado){
  OperacaoSalva config;config.modo=novoModo;config.armado=novoArmado;
  if(!discoDisponivel||memoriaHistorico.putUChar("operacao",config.codificar())!=1){operacaoSalva=false;return false;}
  operacaoSalva=true;return true;
}
// Recupera modo e pausa ao ligar. Sem dado válido: Monitoramento pausado.
void carregarOperacao(){
  OperacaoSalva config;
  if(discoDisponivel){
    if(memoriaHistorico.isKey("operacao"))operacaoSalva=config.carregar(memoriaHistorico.getUChar("operacao",0));
    else operacaoSalva=memoriaHistorico.putUChar("operacao",config.codificar())==1;
  }
  if(!operacaoSalva)config=OperacaoSalva{};
  modo=config.modo;armado=config.armado;automatico=modo!=ModoOperacao::Monitoramento;
}
// Desliga o buzzer se estiver tocando e registra o evento.
void silenciar(){if(alarme.ativa){alarme.parar();sirene=false;digitalWrite(PIN_BUZZER,LOW);registrar("sirene_off");}}
// ---- Programação noturna ----
// Entre 'inicio' e 'fim' (padrão 22:00–07:00) a placa entra sozinha no modo
// Ausente; fora da faixa volta ao modo diurno escolhido. Uma troca manual vale
// até a próxima virada de horário. Tudo roda na placa, sem o navegador.
// BEGIN_NOTURNO
// Minutos desde 1970 no horário de Brasília (UTC-3). false = relógio sem NTP.
bool relogioNoturno(uint32_t& minuto){const time_t agora=time(nullptr);if(agora<1700000000)return false;minuto=uint32_t((int64_t(agora)-3*3600)/60);return true;}
String horarioNoturno(uint16_t minuto){char b[6];snprintf(b,sizeof(b),"%02u:%02u",unsigned(minuto/60),unsigned(minuto%60));return String(b);}
// Grava a programação (20 bytes com CRC) na flash.
bool guardarNoturno(const ProgramacaoNoturna& nova){
  uint8_t b[ProgramacaoNoturna::TAMANHO];nova.codificar(b);
  if(!nova.valida()||!discoDisponivel||memoriaHistorico.putBytes("noturno",b,sizeof(b))!=sizeof(b)){noturnoSalvo=false;return false;}
  noturno=nova;noturnoSalvo=true;return true;
}
// Lê a programação salva; em placa nova grava o padrão (desativado).
void carregarNoturno(){
  if(!discoDisponivel){noturnoSalvo=false;return;}
  if(!memoriaHistorico.isKey("noturno")){guardarNoturno(noturno);return;}
  uint8_t b[ProgramacaoNoturna::TAMANHO];
  noturnoSalvo=memoriaHistorico.getBytesLength("noturno")==sizeof(b)&&memoriaHistorico.getBytes("noturno",b,sizeof(b))==sizeof(b)&&noturno.carregar(b,sizeof(b));
}
// Marca um ajuste manual: a programação não mexe no modo até a próxima virada.
bool suspenderNoturno(){
  if(!noturno.habilitado)return true;
  uint32_t minuto;ProgramacaoNoturna nova=noturno;nova.suspensao=relogioNoturno(minuto)?nova.periodo(minuto):UINT32_MAX;return guardarNoturno(nova);
}
// Chamada a cada 1 s pelo loop: troca o modo se a faixa de horário pedir.
void aplicarNoturno(){
  if(!noturno.habilitado){noturnoAplicado=true;return;}
  uint32_t minuto;if(!relogioNoturno(minuto))return;
  if(noturno.suspensao==UINT32_MAX){ProgramacaoNoturna nova=noturno;nova.suspensao=nova.periodo(minuto);if(!guardarNoturno(nova)){noturnoAplicado=false;return;}}
  if(noturno.manual(minuto)){noturnoAplicado=true;return;}
  const ModoOperacao alvo=noturno.alvo(minuto);
  if(modo==alvo){noturnoAplicado=true;return;}
  if(!guardarOperacao(alvo,armado)){noturnoAplicado=false;return;}
  silenciar();modo=alvo;automatico=modo!=ModoOperacao::Monitoramento;
  registrar(modo==ModoOperacao::Ausente?"modo_ausente":modo==ModoOperacao::Apresentacao?"modo_apresentacao":"modo_monitor");noturnoAplicado=true;
}
// Trecho JSON da programação noturna incluído no /status.
String statusNoturno(){
  uint32_t minuto=0;const bool pronto=relogioNoturno(minuto);
  return String(",\"noturno\":{\"habilitado\":")+(noturno.habilitado?"true":"false")+",\"inicio\":\""+horarioNoturno(noturno.inicio)+"\",\"fim\":\""+horarioNoturno(noturno.fim)+"\",\"modoDia\":\""+idModo(noturno.dia)+"\",\"relogioPronto\":"+(pronto?"true":"false")+",\"faixaNoturna\":"+(pronto&&noturno.noite(minuto)?"true":"false")+",\"ajusteManual\":"+(noturno.habilitado&&((pronto&&noturno.manual(minuto))||noturno.suspensao==UINT32_MAX)?"true":"false")+",\"salvo\":"+(noturnoSalvo?"true":"false")+",\"aplicado\":"+(noturnoAplicado?"true":"false")+"}";
}
// END_NOTURNO

// POST /cmd?c=...&v=... — comandos do painel:
//   c=modo&v=monitoramento|ausente|apresentacao  troca o modo
//   c=arm&v=0|1        pausa (0) ou retoma (1) o registro
//   c=sirene&v=0|1     silencia ou testa o som do modo atual
//   c=noturno&v=0|1&inicio=HH:MM&fim=HH:MM&dia=modo   salva a programação noturna
// Responde com o /status atualizado ou com {"erro": ...}.
void comando() {
  const String c=server.arg("c"),v=server.arg("v");
  if(c=="noturno"){
    ProgramacaoNoturna nova=noturno;ModoOperacao dia;
    if((v!="0"&&v!="1")||!lerHorarioNoturno(server.arg("inicio").c_str(),nova.inicio)||!lerHorarioNoturno(server.arg("fim").c_str(),nova.fim)||!lerModo(server.arg("dia").c_str(),dia)) {responder(400,"{\"erro\":\"Programacao invalida\"}");return;}
    nova.habilitado=v=="1";nova.dia=dia;nova.suspensao=0;
    if(!nova.valida()){responder(400,"{\"erro\":\"Horarios devem ser diferentes; escolha um modo diurno valido\"}");return;}
    if(!guardarNoturno(nova)){responder(503,"{\"erro\":\"Nao foi possivel salvar a programacao anterior foi mantida\"}");return;}
    aplicarNoturno();
  }else if(c=="modo"){

    ModoOperacao escolhido;
    if(!lerModo(v.c_str(),escolhido)){responder(400,"{\"erro\":\"Modo invalido\"}");return;}
    const ProgramacaoNoturna noiteAnterior=noturno;
    if(!suspenderNoturno()){responder(503,"{\"erro\":\"Nao foi possivel salvar o ajuste manual\"}");return;}
    if(!guardarOperacao(escolhido,armado)){
      if(noiteAnterior.habilitado)guardarNoturno(noiteAnterior);responder(503,"{\"erro\":\"Nao foi possivel salvar o modo. A escolha anterior continua ativa.\"}");return;}
    silenciar();
    if(modo!=escolhido){modo=escolhido;registrar(modo==ModoOperacao::Ausente?"modo_ausente":modo==ModoOperacao::Apresentacao?"modo_apresentacao":"modo_monitor");}
    automatico=modo!=ModoOperacao::Monitoramento;
  }else if((c=="arm"||c=="sirene")&&(v=="0"||v=="1")){
    const bool ligado=v=="1";
    if(c=="arm"){
      if(!guardarOperacao(modo,ligado)){responder(503,"{\"erro\":\"Nao foi possivel salvar a pausa. A escolha anterior continua ativa.\"}");return;}
      if(armado!=ligado){armado=ligado;if(!armado){fecharOcorrencia();silenciar();movimento=false;anterior=false;}registrar(armado?"armado":"desarmado");}
    }else if(!ligado)silenciar();
    else{
      if(!alarme.manual(modo,millis())){responder(409,"{\"erro\":\"Monitoramento nao emite som. Escolha outro modo para testar.\"}");return;}
      sirene=true;registrar("sirene_on");
    }
  }else{responder(400,"{\"erro\":\"Comando invalido. Atualize o painel para escolher um modo.\"}");return;}
  sirene=alarme.ativa;digitalWrite(PIN_BUZZER,sirene?HIGH:LOW);status();
}
// ---- setup: roda uma vez ao ligar ----
void setup() {
  Serial.begin(115200); pinMode(PIN_PIR,INPUT); pinMode(PIN_BUZZER,OUTPUT);
  digitalWrite(PIN_BUZZER,LOW);
  iniciarLCD();publicarLCD("Conectando WiFi","");
  sessao=String(esp_random(),HEX)+String(esp_random(),HEX);
  // Recupera da flash: últimos eventos, modo/pausa e programação noturna.
  carregarHistorico();carregarOperacao();carregarNoturno();
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
  WiFi.setSleep(false);
  // Rotas HTTP. A página é enviada direto da flash trocando __CAM__ pelo IP da câmera.
  server.on("/",HTTP_GET,[](){
    enviarPagina(server, PAGINA, sizeof(PAGINA)-1, CAM_IP.c_str());
  });
  server.on("/status",HTTP_GET,status);
  server.on("/eventos",HTTP_GET,historico);
  server.on("/cmd",HTTP_POST,comando);
  server.onNotFound([](){responder(404,"{\"erro\":\"Nao encontrado\"}");});
  server.begin(); registrar("inicio");
}
// ---- loop: roda sem parar (a cada ~2 ms) ----
void loop() {
  server.handleClient(); uint32_t agora=millis();
  static uint32_t ultimaProgramacao=0;if(uint32_t(agora-ultimaProgramacao)>=1000){ultimaProgramacao=agora;aplicarNoturno();}
  // Testa um IP por volta até encontrar a ESP32-CAM.
  procurarCamera();
  // Leitura do PIR. Só conta se o registro estiver ligado e o sensor calibrado.
  bool leitura=digitalRead(PIN_PIR)==HIGH;
  bool ativo=leitura&&armado&&agora>=AQUECIMENTO;
  const bool novaOcorrencia=atualizarOcorrencia(agora,ativo);
  if(ativo&&!anterior) {
    ++totalMovimentos;

  }
  anterior=ativo; movimento=ativo;
  // Som: toca no início de cada ocorrência (duração depende do modo) e desliga sozinho.
  if(alarme.movimento(modo,armado,novaOcorrencia,agora))registrar("sirene_on");
  if(alarme.atualizar(agora))registrar("sirene_off");
  sirene=alarme.ativa;
  digitalWrite(PIN_BUZZER,sirene?HIGH:LOW);
  // Wi-Fi: ao conectar, inicia o NTP (UTC-3) e mostra o IP no Serial;
  // se cair, tenta reconectar a cada 15 s.
  static bool avisouIP=false;
  if(WiFi.status()==WL_CONNECTED) {
    if(!avisouIP) {
      avisouIP=true;
      WiFi.setSleep(false);
      configTime(-3*3600,0,"time.google.com","pool.ntp.org","time.cloudflare.com");
      Serial.print("Painel RASTRO: http://"); Serial.println(WiFi.localIP());
    }
  } else {
    avisouIP=false;
    if(uint32_t(agora-ultimaReconexao)>15000) {
      ultimaReconexao=agora;
      Serial.printf("Wi-Fi sem conexao (status %d), tentando de novo...\n",WiFi.status());
      WiFi.disconnect(); WiFi.begin(WIFI_SSID,WIFI_PASS);
    }
  }
  // Atualiza o LCD 1x por segundo: linha 1 = estado/modo, linha 2 = IP do painel.
  static uint32_t lcdAnterior=0;
  if(uint32_t(agora-lcdAnterior)>1000) {
    lcdAnterior=agora;
    String linha=agora<AQUECIMENTO?"Calibrando PIR":(movimento?"MOVIMENTO!":(armado?(modo==ModoOperacao::Ausente?"Modo ausente":modo==ModoOperacao::Apresentacao?"Apresentacao":"Monitoramento"):"Registro pausado"));
    String ip=WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():"Sem WiFi";
    publicarLCD(linha,ip);
  }
  // Corrige datas de eventos registrados antes do NTP e grava pendências na flash.
  sincronizarDatasHistorico();salvarHistorico();
  delay(2);
}
