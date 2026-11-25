# Montagem no Wokwi

Simulação do ESP32 principal com PIR, LCD I2C e buzzer.

1. Acesse https://wokwi.com/projects/new/esp32
2. Na aba `diagram.json`, cole o arquivo desta pasta.
3. No *Library Manager*, adicione **LiquidCrystal I2C**.
4. Em `sketch.ino`, cole `esp32_principal/esp32_principal.ino`. Crie uma aba (botão **+** → *New file*) para cada `.h` da pasta `esp32_principal/` com o mesmo nome, e um `config.h` com `WIFI_SSID "Wokwi-GUEST"`, `WIFI_PASS ""` e `CAM_IP_PADRAO "10.0.0.2"`.

O Wokwi não usa o `partitions.csv` do projeto: sem a partição `rastro_data`, o firmware avisa no Serial que o histórico está funcionando só na RAM, o que é esperado na simulação.

Ao iniciar, o LCD mostra `Calibrando PIR` e o IP; depois de 30 s, clique no sensor PIR para simular uma passagem.

A ESP32-CAM não existe no Wokwi; ela só precisa de alimentação 5 V e Wi-Fi, sem fios para o ESP32 principal.
