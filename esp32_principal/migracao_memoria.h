#pragma once

// Abre o histórico na partição dedicada "rastro_data" (64 KB). Versões antigas
// usavam a NVS padrão: na primeira vez os dados são copiados e conferidos, sem
// apagar o original. O marcador "migrado" é gravado por último, então uma queda
// de energia no meio faz a cópia recomeçar no próximo boot.
bool abrirHistorico(){
  if(!memoriaHistorico.begin("rastro-h2",false,"rastro_data"))return false;
  if(memoriaHistorico.getBool("migrado",false))return true;
  Preferences anterior;
  bool ok=true;
  if(anterior.begin("rastro-h2",true)){
    for(size_t slot=0;slot<LIMITE&&ok;slot++){
      char chave[5];chaveHistorico(slot,chave);
      if(!anterior.isKey(chave))continue;
      uint8_t original[64],copiado[64];
      if(anterior.getBytesLength(chave)!=64||anterior.getBytes(chave,original,64)!=64){ok=false;break;}
      const bool igual=memoriaHistorico.getBytesLength(chave)==64&&
        memoriaHistorico.getBytes(chave,copiado,64)==64&&memcmp(original,copiado,64)==0;
      if(!igual&&memoriaHistorico.putBytes(chave,original,64)!=64){ok=false;break;}
      ok=memoriaHistorico.getBytes(chave,copiado,64)==64&&memcmp(original,copiado,64)==0;
    }
    if(ok&&anterior.isKey("operacao")){
      const uint8_t valor=anterior.getUChar("operacao",0);
      ok=memoriaHistorico.putUChar("operacao",valor)==1&&memoriaHistorico.getUChar("operacao",0)==valor;
    }
    if(ok&&anterior.getBool("importado",false))ok=memoriaHistorico.putBool("importado",true)!=0;
    anterior.end();
  }
  if(ok)ok=memoriaHistorico.putBool("migrado",true)!=0;
  if(!ok)memoriaHistorico.end();
  return ok;
}
