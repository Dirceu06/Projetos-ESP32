#include "wifi.h"
#include "env.h"
#include <ArduinoJson.h>
#include "oled.h"
#include <Arduino.h>

void conectarWiFi() {
  Serial.print("Conectando ao WiFi");
  WiFi.begin(WIFI_NOME, WIFI_SENHA);
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nNao foi possivel conectar ao WiFi.");
  }
}




float* OLED_WiFiClient_SSE(bool *conectado) { 
  float infos[4];
  for(int i = 0; i<4; i++){
    infos[i] = 999.9;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Erro] Sem conexao WiFi.");
    *conectado = false;
    return infos;
  }

  WiFiClient client; 

  if (!*conectado) {
    if(client.connect("192.168.200.105", 8000)){
      Serial.println("[SSE] Conectado! Solicitando Stream continuo...");
      client.println("GET /combo HTTP/1.1");
      client.println("Host: 192.168.200.105:8000");
      client.println("Accept: text/event-stream"); 
      client.println("Cache-Control: no-cache");
      client.println("Connection: keep-alive");   
      client.println(); 
      *conectado = true;
    }
  }
  
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("[SSE] Erro: Timeout inicial!");
      client.stop();
      return infos;
    }
    delay(10); 
  }

  if (client.available()) {
    String linha = client.readStringUntil('\n');
    linha.trim(); 

    if (linha.startsWith("data:")) {
      String jsonBruto = linha.substring(5);
      jsonBruto.trim();

      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, jsonBruto);

      if (!error) {
        infos[0] = doc["CPU"]["Load"] | 0.0;
        infos[1] = doc["RAM"]["Load"] | 0.0;
        
        // CORREÇÃO 2: Acessa o índice [0] do array da GPU enviado pelo FastAPI
        infos[2] = doc["GPU"][0]["Load"] | 0.0; 
        infos[3] = doc["GPU"][1]["Load"] | 0.0; 
        *conectado = true;
        return infos;
        
      } else {
        Serial.print("[SSE-JSON] Erro no parse: ");
        Serial.println(error.c_str());
        *conectado = false;
      }
    }

  } else {
    Serial.println("[SSE] Falha ao conectar no servidor.");

  }

  *conectado = false;
  return infos;
}

