#include "buzz.h"
#include <Arduino.h>

void recebidoBuzz(){
  tone(BUZZ_PIN,392,120);
  tone(BUZZ_PIN,523,120);
  tone(BUZZ_PIN,659,350);
}

void enviadoBuzz(){
  tone(BUZZ_PIN,523,150);
  tone(BUZZ_PIN,659,150);
  tone(BUZZ_PIN,784,300);
}