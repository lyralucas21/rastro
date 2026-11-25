#pragma once
#include <Preferences.h>
#include <algorithm>
#include "historico_inicial.h"

// Persistência do histórico na flash (biblioteca Preferences / NVS).
// Os eventos não são gravados a cada mudança: ficam marcados como pendentes e
// são salvos em lote para poupar a flash (ver salvarHistorico).
Preferences memoriaHistorico;
bool discoDisponivel=false,discoSaudavel=true,urgenteHistorico=false;
bool pendenteHistorico[LIMITE]={false};
uint32_t geracaoDisco=0,ultimoFlushHistorico=0;
size_t recuperadosHistorico=0,corrompidosHistorico=0;
void chaveHistorico(uint8_t slot,char* chave){snprintf(chave,5,"e%02u",unsigned(slot));}
void marcarHistorico(size_t slot,bool urgente=false){pendenteHistorico[slot]=true;urgenteHistorico|=urgente;}
bool historicoPendente(){for(size_t i=0;i<LIMITE;i++)if(pendenteHistorico[i])return true;return false;}
// Gera o texto da data a partir do epoch salvo (horário de Brasília).
void formatarEventoSalvo(Evento& e){
  if(e.epoch<1700000000){strcpy(e.data,"Sem horario sincronizado");return;}
  time_t local=time_t(e.epoch)-3*3600;struct tm t;gmtime_r(&local,&t);
  strftime(e.data,sizeof(e.data),"%Y-%m-%d %H:%M:%S",&t);
}
#include "migracao_memoria.h"
// Ao ligar: abre a partição, importa a semente inicial (se houver) e lê os
// blocos válidos, reordenando pela numeração. Ocorrências que estavam abertas
// quando a placa desligou ficam marcadas como "interrompidas".
void carregarHistorico(){
  discoDisponivel=abrirHistorico();
  if(!discoDisponivel){discoSaudavel=false;Serial.println("Historico: memoria indisponivel; funcionando em RAM.");return;}
  // Importa a semente historico_inicial.h uma única vez (marcador "importado").
  if(!memoriaHistorico.getBool("importado",false)){
    bool ok=true;
    for(size_t i=0;i<sizeof(HISTORICO_INICIAL)/64;i++){
      char chave[5];chaveHistorico(i,chave);
      if(!memoriaHistorico.isKey(chave)){
        uint8_t b[64];memcpy_P(b,HISTORICO_INICIAL[i],64);
        if(memoriaHistorico.putBytes(chave,b,64)!=64){ok=false;break;}
      }
    }
    if(ok)ok=memoriaHistorico.putBool("importado",true)!=0;
    if(!ok)discoSaudavel=false;
  }
  bool usados[LIMITE]={false};HistoricoDisco::Estado maisRecente;bool encontrou=false;
  for(size_t slot=0;slot<LIMITE;slot++){
    char chave[5];chaveHistorico(slot,chave);if(!memoriaHistorico.isKey(chave))continue;
    uint8_t b[64];Evento e;HistoricoDisco::Estado estado;
    if(memoriaHistorico.getBytesLength(chave)!=64||memoriaHistorico.getBytes(chave,b,64)!=64||!HistoricoDisco::decodificar(b,64,e,estado)){
      ++corrompidosHistorico;continue;
    }
    e.slotFlash=slot;formatarEventoSalvo(e);eventos[quantidade++]=e;usados[slot]=true;
    if(!encontrou||HistoricoDisco::maisNovo(estado.geracao,maisRecente.geracao)){maisRecente=estado;encontrou=true;}
  }
  std::sort(eventos,eventos+quantidade,[](const Evento&a,const Evento&b){return a.ordem<b.ordem;});
  if(encontrou){geracaoDisco=maisRecente.geracao;sequencia=0;totalMovimentos=maisRecente.movimentos;totalOcorrencias=maisRecente.ocorrencias;}
  for(size_t i=0;i<quantidade;i++){
    sequencia=std::max(sequencia,eventos[i].ordem);
    if(eventos[i].aberta){eventos[i].aberta=false;eventos[i].interrompida=true;marcarHistorico(i,true);}
    if(strcmp(eventos[i].tipo,"movimento")==0)ultimo=eventos[i].data;
  }
  size_t livre=0;
  for(size_t i=quantidade;i<LIMITE;i++){while(usados[livre])++livre;eventos[i].slotFlash=livre;usados[livre]=true;}
  recuperadosHistorico=quantidade;proximo=quantidade%LIMITE;
  if(corrompidosHistorico)discoSaudavel=false;
  Serial.printf("Historico: %u registros recuperados; %u invalidos.\n",unsigned(quantidade),unsigned(corrompidosHistorico));
}
// Quando o NTP sincroniza, calcula a hora real dos eventos desta sessão que
// foram registrados antes disso (hora atual - tempo decorrido).
void sincronizarDatasHistorico(){
  const time_t agora=time(nullptr);if(agora<1700000000)return;
  const uint32_t ms=millis();bool mudou=false;
  for(size_t i=0;i<quantidade;i++)if(!eventos[i].epoch&&strcmp(eventos[i].origem,sessao.c_str())==0){
    eventos[i].epoch=uint32_t(agora)-uint32_t(ms-eventos[i].uptime)/1000;
    formatarEventoSalvo(eventos[i]);marcarHistorico(i,true);++revisao;mudou=true;

  }
  if(mudou)for(size_t i=0;i<quantidade;i++){const Evento& e=eventos[(proximo+LIMITE-quantidade+i)%LIMITE];if(strcmp(e.tipo,"movimento")==0)ultimo=e.data;}
}
// Grava os eventos pendentes na flash.
void salvarHistorico(){
  if(!discoDisponivel||!historicoPendente())return;
  const uint32_t agora=millis(),intervalo=uint32_t(agora-ultimoFlushHistorico);
  // Criação/encerramento: grava em até 2 s. Ocorrência ativa: a cada 30 s.
  if(intervalo<2000||(!urgenteHistorico&&intervalo<30000))return;
  ultimoFlushHistorico=agora;urgenteHistorico=false;bool ok=true;
  for(size_t i=0;i<LIMITE;i++)if(pendenteHistorico[i]){
    uint8_t b[64];char chave[5];chaveHistorico(eventos[i].slotFlash,chave);
    const HistoricoDisco::Estado estado{geracaoDisco+1,eventos[i].ordem,totalMovimentos,totalOcorrencias};
    if(!HistoricoDisco::codificar(eventos[i],estado,b)||memoriaHistorico.putBytes(chave,b,64)!=64){ok=false;continue;}
    geracaoDisco=estado.geracao;pendenteHistorico[i]=false;
  }
  discoSaudavel=ok&&!corrompidosHistorico;
  if(!ok)Serial.println("Historico: falha ao salvar; dados mantidos em RAM para nova tentativa.");
}
