// =============================================================================
// RASTRO — firmware da ESP32-CAM (AI-Thinker, câmera OV2640)
// Baseado no exemplo CameraWebServer da Espressif (créditos mantidos nos arquivos).
//
// Papel no projeto: servir imagens na rede local para o painel do RASTRO.
//   * http://IP-DA-CAMERA:81/stream  vídeo MJPEG ao vivo
//   * http://IP-DA-CAMERA/capture    uma foto JPEG
//   * http://IP-DA-CAMERA/status     configuração atual (usada pelo ESP32
//                                    principal para encontrar a câmera na rede)
//   * http://IP-DA-CAMERA/control    ajustes (o painel troca a qualidade JPEG)
// Ajustes do RASTRO: QVGA 320x240, dois buffers na PSRAM com CAMERA_GRAB_LATEST,
// XCLK reduzido para 8 MHz após iniciar o sensor e detector de brownout desligado.
// A câmera não tem fios de dados com o ESP32 principal: tudo passa pelo Wi-Fi.
//
// Placa no Arduino IDE: ESP32 Dev Module · Huge APP · PSRAM Enabled.
// Copie config.exemplo.h para config.h e preencha a rede Wi-Fi (2,4 GHz).
// =============================================================================
#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>

// ===========================
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
// Select camera model in board_config.h
// ===========================
#include "board_config.h"

#include "config.h"  // WIFI_SSID e WIFI_PASS (arquivo fora do Git)
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

void startCameraServer();
void setupLedFlash();

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // desliga o detector de brownout
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000; // Clock padrao: prioriza taxa de quadros
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 18; // Mais compressao para reduzir trafego Wi-Fi
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 18; // Mantem compressao leve para a rede com PSRAM
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

#if CAMERA_TESTE_MEMORIA_INTERNA
  config.frame_size = FRAMESIZE_QVGA;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  Serial.println("Teste de imagem: QVGA, um quadro, memoria interna.");
#endif
#if RASTRO_TESTE_PSRAM_BUFFERS != 1 && RASTRO_TESTE_PSRAM_BUFFERS != 2
#error "RASTRO_TESTE_PSRAM_BUFFERS deve ser 1 ou 2"
#endif
  if (!psramFound()) {
    Serial.println("TESTE CANCELADO: PSRAM nao detectada. Confira PSRAM Enabled no menu e volte ao backup se necessario.");
    return;
  }
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.jpeg_quality = 18;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.fb_count = RASTRO_TESTE_PSRAM_BUFFERS;
  config.grab_mode = RASTRO_TESTE_PSRAM_BUFFERS == 2 ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  Serial.printf("TESTE RASTRO: PSRAM, QVGA, %u buffer(s).\n", (unsigned)config.fb_count);

  Serial.printf("PSRAM detectada: %s; memoria livre: %u bytes\n", psramFound() ? "sim" : "nao", (unsigned)ESP.getFreeHeap());

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    Serial.println("Confira o cabo da camera, o modelo AI-Thinker e a alimentacao 5V.");
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  Serial.printf("Sensor da camera: 0x%04x\n", s->id.PID);
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Keep the tested sensor initialization, then apply the validated runtime clock.
  if(s->set_xclk(s,LEDC_TIMER_0,8)!=0)Serial.println("Nao foi possivel aplicar XCLK 8 MHz.");
  else Serial.println("RASTRO: XCLK 8 MHz; servidor TCP_NODELAY.");

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.printf("Conectando ao Wi-Fi: %s\n", ssid);
  uint32_t inicioTentativa = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - inicioTentativa >= 20000) {
      Serial.printf("\nSem conexao (status %d). Confira rede, senha e hotspot em 2.4 GHz. Tentando novamente...\n", WiFi.status());
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      inicioTentativa = millis();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  Serial.print("Video ao vivo: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");
}

void loop() {
  // Repete o IP para quem abrir o Monitor Serial depois da inicializacao.
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("ESP32-CAM: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.printf("ESP32-CAM sem Wi-Fi (status %d). Aguardando reconexao...\n", WiFi.status());
  }
  delay(10000);
}
