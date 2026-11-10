# Montagem no Wokwi

Simulação do ESP32 principal com PIR, LCD I2C e buzzer.

1. Acesse https://wokwi.com/projects/new/esp32
2. Na aba `diagram.json`, cole o arquivo desta pasta.
3. No *Library Manager*, adicione **LiquidCrystal I2C**.
4. Em `sketch.ino`, cole o firmware de `esp32_principal/` com o conteúdo de `pagina.h` no lugar do `#include` e use a rede `Wokwi-GUEST` (senha vazia).

Ao iniciar, o LCD mostra `Calibrando PIR` e o IP; depois de 30 s, clique no sensor PIR para simular uma passagem.

A ESP32-CAM não existe no Wokwi; ela só precisa de alimentação 5 V e Wi-Fi, sem fios para o ESP32 principal.
