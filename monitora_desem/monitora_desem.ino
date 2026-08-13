#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ArduinoJson.h> // <--- ArduinoJson mantido para o teste
#include "config.h" // credenciais reais ficam em config.h (fora do git, veja .gitignore e config.example.h)

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// ---------- Pinos ----------
#define POT_PIN        34

// ---------- OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Endereço do Servidor do ViaCEP
const char* server = "192.168.200.105";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- TESTE: WiFiClient + ArduinoJson ---");

  // Configura os pinos I2C customizados (SDA, SCL)
  Wire.begin(27, 26);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED] Falha ao inicializar!");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("OLED OK!");
    display.display();
  }

  Serial.print("Conectando ao WiFi");
  WiFi.begin(ssid, password);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 15) {
      delay(500);
      Serial.print(".");
      tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[WiFi] Conectado!");
  } else {
      Serial.println("\n[WiFi] Falha na conexao.");
  }

}

void loop() {
  char c = Serial.read();
  if(c == '1') testOLED_WiFiClient_SSE();

}

void OLED_WiFiClient_SSE() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Conectando SSE...");
  display.display();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Erro] Sem conexao WiFi.");
    return;
  }

  // CORREÇÃO 1: Limpa qualquer lixo ou Enter (\n, \r) que sobrou do menu no Serial
  while(Serial.available() > 0) {
    Serial.read();
  }

  WiFiClient client;

  if (client.connect("192.168.200.105", 8000)) {
    Serial.println("[SSE] Conectado! Solicitando Stream continuo...");

    client.println("GET /combo HTTP/1.1");
    client.println("Host: 192.168.200.105:8000");
    client.println("Accept: text/event-stream");
    client.println("Cache-Control: no-cache");
    client.println("Connection: keep-alive");
    client.println();

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("[SSE] Erro: Timeout inicial!");
        client.stop();
        return;
      }
      delay(10);
    }

    while (client.connected()) {
      delay(10); // Alimenta o Watchdog do ESP32

      if (client.available()) {
        String linha = client.readStringUntil('\n');
        linha.trim();

        if (linha.startsWith("data:")) {
          String jsonBruto = linha.substring(5);
          jsonBruto.trim();

          DynamicJsonDocument doc(1024);
          DeserializationError error = deserializeJson(doc, jsonBruto);

          if (!error) {
            float cpuLoad = doc["CPU"]["Load"] | 0.0;
            float ramLoad = doc["RAM"]["Load"] | 0.0;

            // CORREÇÃO 2: Acessa o índice [0] do array da GPU enviado pelo FastAPI
            float gpuLoad0 = doc["GPU"][0]["Load"] | 0.0;
            float gpuLoad1 = doc["GPU"][1]["Load"] | 0.0;

            display.clearDisplay();

            display.setCursor(0, 0);
            display.print("CPU: "); display.print(cpuLoad); display.println(" %");
            display.setCursor(0, 16);
            display.print("RAM: "); display.print(ramLoad); display.println(" %");

            display.setCursor(0, 28);
            display.print("GPU0: "); display.print(gpuLoad0); display.println(" %");
            display.setCursor(0, 42);
            display.print("GPU1: "); display.print(gpuLoad1); display.println(" %");


            display.display();
          } else {
            Serial.print("[SSE-JSON] Erro no parse: ");
            Serial.println(error.c_str());
          }
        }
      }

      // CORREÇÃO 3: Só encerra o streaming se você digitar 'X' ou 'x' no terminal
      if (Serial.available()) {
        char tecla = Serial.read();
        if (tecla == 'X' || tecla == 'x') {
          Serial.println("[SSE] Conexao encerrada pelo usuario.");
          break;
        }
      }
    }

    client.stop();

  } else {
    Serial.println("[SSE] Falha ao conectar no servidor.");
  }
}
