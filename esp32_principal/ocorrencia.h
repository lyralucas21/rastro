#pragma once
#include <stdint.h>

// Agrupador de ocorrências de movimento.
// O PIR liga e desliga várias vezes enquanto alguém está por perto. Para não
// virar dezenas de registros, sinais próximos formam uma única "ocorrência":
//   * a primeira subida do sinal abre a ocorrência (Inicio);
//   * novas subidas em menos de 10 s somam detecções (Atualizacao);
//   * após 10 s sem sinal ela é encerrada (Fim).
// A subtração com uint32_t continua correta quando millis() dá a volta (~49 dias).
struct AgrupadorOcorrencia {
  enum Mudanca { Nenhuma, Inicio, Atualizacao, Fim };
  static constexpr uint32_t INTERVALO = 10000;
  bool aberta = false, anterior = false;
  uint32_t inicio = 0, ultimo = 0, publicado = 0, deteccoes = 0;
  Mudanca atualizar(uint32_t agora, bool ativo) {
    // Passaram 10 s desde o último sinal: fecha a ocorrência anterior.
    if (aberta && !anterior && uint32_t(agora - ultimo) >= INTERVALO) {
      aberta = false;
      if (!ativo) return Fim;
    }
    if (ativo) {
      if (!aberta) {
        aberta = true; anterior = true;
        inicio = ultimo = publicado = agora; deteccoes = 1;
        return Inicio;
      }
      // Borda de subida = nova detecção dentro da mesma ocorrência.
      // Com o sinal contínuo, publica uma atualização no máximo 1x por segundo.
      const bool borda = !anterior;
      if (borda) ++deteccoes;
      ultimo = agora; anterior = true;
      if (borda || uint32_t(agora - publicado) >= 1000) {
        publicado = agora; return Atualizacao;
      }
    } else {
      if (anterior) { anterior = false; return Atualizacao; }
      anterior = false;
    }
    return Nenhuma;
  }
  void encerrar() { aberta = false; anterior = false; }
};
