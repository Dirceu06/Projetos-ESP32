#ifndef AUXI_H
#define AUXI_H

#include <Arduino.h>

extern int POT_PIN;
extern int BTT_PIN1;
extern int BTT_PIN2;
extern int BUZZ_PIN;

int lerPot(int totalItens); 
void recebidoBuzz();
void enviadoBuzz();
String lerNomeSerial(String promptOLED);

#endif