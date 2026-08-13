#include "NTP.h"
#include <WiFi.h>
#include <Arduino.h>


const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600; // brasil (UTC-3).
const int daylightOffset_sec = 0;
unsigned long ultimaAtualizacao = 0;

void configurarHora(){
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("sincronizando hora com NTP...");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("falha ao obter hora do NTP - confira conexao WiFi.");
  } else {
    Serial.println("hora sincronizada!");
  }
}