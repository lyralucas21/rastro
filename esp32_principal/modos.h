#pragma once
#include <stdint.h>
#include <string.h>

// Modos de operação. Todos registram e permitem fotos; muda só a reação sonora:
//   Monitoramento: sem som · Ausente: sirene de 8 s · Apresentação: toque de 250 ms
enum class ModoOperacao:uint8_t { Monitoramento=0, Ausente=1, Apresentacao=2 };
inline const char* idModo(ModoOperacao m){return m==ModoOperacao::Ausente?"ausente":m==ModoOperacao::Apresentacao?"apresentacao":"monitoramento";}
inline const char* nomeModo(ModoOperacao m){return m==ModoOperacao::Ausente?"Ausente":m==ModoOperacao::Apresentacao?"Apresentação":"Monitoramento";}
inline bool lerModo(const char* s,ModoOperacao& m){for(int i=0;i<3;i++)if(strcmp(s,idModo(ModoOperacao(i)))==0){m=ModoOperacao(i);return true;}return false;}
// Duração do som (ms) em cada modo.
inline uint32_t tempoSom(ModoOperacao m){return m==ModoOperacao::Ausente?8000:m==ModoOperacao::Apresentacao?250:0;}
// Modo + pausa compactados em 1 byte para a flash (0x40 = marca de validade).
struct OperacaoSalva {
  ModoOperacao modo=ModoOperacao::Monitoramento;
  bool armado=false;
  uint8_t codificar()const{return 0x40|uint8_t(modo)|(armado?0x80:0);}
  bool carregar(uint8_t b){if((b&0x7c)!=0x40||(b&3)>2)return false;modo=ModoOperacao(b&3);armado=b&0x80;return true;}
};
// Liga o buzzer por 'duracao' ms e desliga sozinho (sem bloquear o loop com delay).
struct ControleSirene {
  bool ativa=false;
  uint32_t inicio=0,duracao=0;
  void parar(){ativa=false;}
  bool manual(ModoOperacao modo,uint32_t agora){const auto t=tempoSom(modo);if(!t)return false;ativa=true;inicio=agora;duracao=t;return true;}
  bool movimento(ModoOperacao modo,bool armado,bool novaOcorrencia,uint32_t agora){return armado&&novaOcorrencia&&manual(modo,agora);}
  bool atualizar(uint32_t agora){if(ativa&&uint32_t(agora-inicio)>=duracao){parar();return true;}return false;}
};
