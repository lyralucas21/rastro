#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "modos.h"

// Programação diária: entre 'inicio' e 'fim' (minutos do dia) usa Ausente;
// fora disso usa o modo 'dia'. 'suspensao' guarda um ajuste manual feito pelo
// usuário, válido até a próxima virada de horário.
struct ProgramacaoNoturna {
  bool habilitado=false;
  uint16_t inicio=22*60,fim=7*60;
  ModoOperacao dia=ModoOperacao::Monitoramento;
  uint32_t suspensao=0;
  static constexpr size_t TAMANHO=20;
  bool valida()const{return inicio<1440&&fim<1440&&inicio!=fim&&(dia==ModoOperacao::Monitoramento||dia==ModoOperacao::Apresentacao);}
  // true se o minuto está na faixa noturna (aceita faixas que cruzam a meia-noite).
  bool noite(uint32_t minutoLocal)const{const auto m=minutoLocal%1440;return inicio<fim?(m>=inicio&&m<fim):(m>=inicio||m<fim);}
  // Identifica a última virada de horário (início ou fim) antes deste minuto.
  uint32_t periodo(uint32_t minutoLocal)const{
    const uint32_t base=minutoLocal-minutoLocal%1440;const auto m=minutoLocal%1440;
    if(m>=inicio&&m>=fim)return base+(inicio>fim?inicio:fim);
    if(m>=inicio)return base+inicio;
    if(m>=fim)return base+fim;
    return base-1440+(inicio>fim?inicio:fim);
  }
  bool manual(uint32_t minutoLocal)const{return habilitado&&(suspensao==UINT32_MAX||suspensao==periodo(minutoLocal));}
  ModoOperacao alvo(uint32_t minutoLocal)const{return noite(minutoLocal)?ModoOperacao::Ausente:dia;}
  static uint32_t crc(const uint8_t* b,size_t n){uint32_t c=0xffffffff;for(size_t i=0;i<n;i++){c^=b[i];for(int j=0;j<8;j++)c=(c>>1)^(0xedb88320u&uint32_t(-int32_t(c&1)));}return ~c;}
  static void u32(uint8_t* b,uint32_t n){for(int i=0;i<4;i++)b[i]=uint8_t(n>>(i*8));}
  static uint32_t ler32(const uint8_t* b){uint32_t n=0;for(int i=0;i<4;i++)n|=uint32_t(b[i])<<(i*8);return n;}
  // Formato na flash: "RNT1" + horários + flags + suspensão + CRC32 (20 bytes).
  void codificar(uint8_t* b)const{
    memset(b,0,TAMANHO);memcpy(b,"RNT1",4);b[4]=uint8_t(inicio);b[5]=uint8_t(inicio>>8);b[6]=uint8_t(fim);b[7]=uint8_t(fim>>8);
    b[8]=(habilitado?1:0)|(uint8_t(dia)<<1);b[9]=1;u32(b+12,suspensao);u32(b+16,crc(b,16));
  }
  bool carregar(const uint8_t* b,size_t n){
    if(n!=TAMANHO||memcmp(b,"RNT1",4)||b[9]!=1||b[10]||b[11]||(b[8]&0xf8)||ler32(b+16)!=crc(b,16))return false;
    ProgramacaoNoturna p;p.inicio=uint16_t(b[4])|(uint16_t(b[5])<<8);p.fim=uint16_t(b[6])|(uint16_t(b[7])<<8);
    p.habilitado=b[8]&1;p.dia=ModoOperacao((b[8]>>1)&3);p.suspensao=ler32(b+12);if(!p.valida())return false;*this=p;return true;
  }
};
// Converte "HH:MM" (vindo do painel) em minutos do dia, validando o texto.
inline bool lerHorarioNoturno(const char* s,uint16_t& minuto){
  if(strlen(s)!=5||s[2]!=':')return false;for(int i=0;i<5;i++)if(i!=2&&(s[i]<'0'||s[i]>'9'))return false;
  const unsigned h=(s[0]-'0')*10+s[1]-'0',m=(s[3]-'0')*10+s[4]-'0';if(h>=24||m>=60)return false;minuto=h*60+m;return true;
}
