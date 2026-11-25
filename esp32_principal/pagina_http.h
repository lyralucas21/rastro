#pragma once
#include <stddef.h>
#include <string.h>

// Envia o painel (guardado na flash) em partes, trocando cada "__CAM__" pelo
// IP da câmera. Assim não é preciso copiar a página inteira (>64 KB) para uma
// String na RAM. Primeiro calcula o tamanho final, depois transmite os trechos.
template <typename Servidor>
void enviarPagina(Servidor &server, const char *pagina, size_t tamanho, const char *camera) {
  constexpr char token[] = "__CAM__";
  constexpr size_t tokenSize = sizeof(token) - 1;
  const size_t cameraSize = strlen(camera);
  size_t total = tamanho;
  const char *cursor = pagina;
  const char *marker;
  while ((marker = strstr(cursor, token)) != nullptr) {
    total = total - tokenSize + cameraSize;
    cursor = marker + tokenSize;
  }
  server.setContentLength(total);
  server.send(200, "text/html; charset=utf-8", "");
  cursor = pagina;
  while ((marker = strstr(cursor, token)) != nullptr) {
    if (marker != cursor) server.sendContent_P(cursor, static_cast<size_t>(marker - cursor));
    if (cameraSize) server.sendContent_P(camera, cameraSize);
    cursor = marker + tokenSize;
  }
  const size_t remaining = tamanho - static_cast<size_t>(cursor - pagina);
  if (remaining) server.sendContent_P(cursor, remaining);
}
