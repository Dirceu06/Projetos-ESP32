#include "auxi.h"
#include "telas.h"

int POT_PIN = 33;
int BTT_PIN1 = 32;
int BTT_PIN2 = 35;
int BUZZ_PIN = 25;

int lerPot(int totalItens) {
  if (totalItens <= 0) return 0;
  int valor = analogRead(POT_PIN);
  
  int res = map(valor, 0, 4095, totalItens - 1, 0);
  
  if (res < 0) res = 0;
  if (res >= totalItens) res = totalItens - 1;
  return res;
}

void recebidoBuzz() {
  tone(BUZZ_PIN, 392, 120);
}

void enviadoBuzz() {
  tone(BUZZ_PIN, 523, 50);
}

// Aguarda e lê uma string digitada no Monitor Serial do Arduino IDE
String lerNomeSerial(String promptOLED) {
  ExibirOLED(promptOLED + "\nDigite no Serial");
  Serial.println("\n----------------------------------");
  Serial.println(promptOLED);
  Serial.println("Digite o nome e pressione Enter:");
  Serial.println("----------------------------------");
  
  while (Serial.available()) Serial.read(); // Limpa buffer antigo
  
  String nome = "";
  while (nome.length() == 0) {
    if (Serial.available()) {
      nome = Serial.readStringUntil('\n');
      nome.trim();
    }
    delay(10);
  }
  
  Serial.print("Nome recebido: ");
  Serial.println(nome);
  return nome;
}