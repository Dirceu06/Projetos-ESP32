#include "IV.h"
#include "oled.h"
#include "buzz.h"
#include <IRremote.h>
#include <Arduino.h>

IRData aux;
bool auxValido = false;

void initIR() {
    IrSender.begin(IR_SEND_PIN);
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
}

void IVleitor() {
  ExibirOLED("esperando...");
  while (true) {
    if (IrReceiver.decode()) {
      recebidoBuzz();
      aux = IrReceiver.decodedIRData;
      auxValido = true;
      float start = millis();
      ExibirOLED("\n[IR-RX] Recebido.");
      delay(1500);
      IrReceiver.resume();
      break;
    }
  }
}

void IVemissor() {
  unsigned long start = millis();
  if (!auxValido) {
    while (millis() - start < 1500) ExibirOLED("Sem sinal guardado!");
  } else {
    IrSender.write(&aux);
    enviadoBuzz();
    while (millis() - start < 1500) ExibirOLED("[IR-TX] Enviado!");
  }
}
