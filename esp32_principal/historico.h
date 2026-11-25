#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Um registro do histórico (ocorrência de movimento, troca de modo, sirene...).
// id/ordem: numeração crescente; uptime/ultimoUptime: millis() do início e do
// último movimento; deteccoes: subidas do PIR; aberta: ocorrência em andamento;
// origem: sessão (boot) em que foi criado; epoch: hora real (0 = sem NTP).
struct Evento {
  uint32_t id=0, uptime=0, ordem=0;
  char data[32]={0};
  const char* tipo="inicio";
  uint32_t ultimoUptime=0, deteccoes=0;
  bool aberta=false;
  char origem[17]={0};
  uint32_t epoch=0;
  bool interrompida=false;
  uint8_t slotFlash=0;
};

// Formato de gravação na flash: cada evento vira um bloco fixo de 64 bytes,
// sem ponteiros, com número mágico, versão e CRC32 para detectar corrupção.
// Cada bloco é uma chave separada na NVS (e00..e99).
namespace HistoricoDisco {
constexpr size_t TAMANHO=64;
constexpr uint32_t MAGIC=0x32545352;
// Tabela tipo <-> código numérico (o texto não é gravado, só o número).
inline const char* tipo(uint8_t n) {
  static const char* const tipos[]={"inicio","movimento","armado","desarmado","sirene_on","sirene_off","auto_on","auto_off","modo_monitor","modo_ausente","modo_apresentacao"};
  return n<11?tipos[n]:nullptr;
}
inline int codigo(const char* s) { for(int i=0;i<11;i++)if(strcmp(s,tipo(i))==0)return i;return -1; }
inline void u32(uint8_t* b,uint32_t v){for(int i=0;i<4;i++)b[i]=uint8_t(v>>(8*i));}
inline uint32_t u32(const uint8_t* b){return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
// CRC32 (polinômio 0xEDB88320) para conferir a integridade de cada bloco.
inline uint32_t crc(const uint8_t* b,size_t n){uint32_t c=0xffffffff;for(size_t i=0;i<n;i++){c^=b[i];for(int k=0;k<8;k++)c=(c>>1)^(0xedb88320u&uint32_t(-int32_t(c&1)));}return ~c;}
struct Estado {uint32_t geracao=0,ordem=0,movimentos=0,ocorrencias=0;};
inline bool maisNovo(uint32_t a,uint32_t b){return int32_t(a-b)>0;}
// Evento -> 64 bytes. Bytes 0-39: números; 40-55: sessão; 56: tipo; 57: flags; 60-63: CRC.
inline bool codificar(const Evento& e,const Estado& s,uint8_t* b){
  const int t=codigo(e.tipo);if(t<0||!e.id||!e.origem[0])return false;
  memset(b,0,TAMANHO);
  const uint32_t valores[]={MAGIC,s.geracao,s.ordem,s.movimentos,s.ocorrencias,e.id,e.uptime,e.ultimoUptime,e.deteccoes,e.epoch};
  for(int i=0;i<10;i++)u32(b+4*i,valores[i]);
  memcpy(b+40,e.origem,16);b[56]=uint8_t(t);b[57]=(e.aberta?1:0)|(e.interrompida?2:0);b[58]=1;
  u32(b+60,crc(b,60));return true;
}
// 64 bytes -> Evento. Recusa blocos com CRC, versão ou campos inválidos.
inline bool decodificar(const uint8_t* b,size_t n,Evento& e,Estado& s){
  if(n!=TAMANHO||u32(b)!=MAGIC||b[58]!=1||b[59]!=0||b[57]>3||!tipo(b[56])||u32(b+60)!=crc(b,60)||!u32(b+20))return false;
  if(!b[40])return false;
  for(int i=40;i<56&&b[i];i++)if(!((b[i]>='0'&&b[i]<='9')||(b[i]>='a'&&b[i]<='f')))return false;
  s={u32(b+4),u32(b+8),u32(b+12),u32(b+16)};
  if(!s.ordem)return false;
  e=Evento{};e.ordem=s.ordem;e.id=u32(b+20);e.uptime=u32(b+24);e.ultimoUptime=u32(b+28);e.deteccoes=u32(b+32);e.epoch=u32(b+36);
  memcpy(e.origem,b+40,16);e.origem[16]=0;e.tipo=tipo(b[56]);e.aberta=b[57]&1;e.interrompida=b[57]&2;
  return true;
}
}

