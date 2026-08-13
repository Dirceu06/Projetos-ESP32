#ifndef ICONES_H
#define ICONES_H

#include <Arduino.h>

// Ícones 8x8, 1 bit por pixel, no formato que o Adafruit_GFX::drawBitmap() espera.
// Feitos à mão pra não depender de nada externo, mas dá pra trocar por qualquer
// desenho: gera em https://javl.github.io/image2cpp/ usando canvas 8x8,
// "Draw mode: Horizontal - 1 bit per pixel, MSB first", "Invert image colors"
// se necessário, e cola o array de bytes aqui no lugar de um destes.

// hamburguer (Menu / trocar de tela)
static const unsigned char ICONE_MENU[] PROGMEM = {
  0xFE, 0x00, 0x00, 0xFE, 0x00, 0x00, 0xFE, 0x00
};

// triangulo apontando pra direita (Play)
static const unsigned char ICONE_PLAY[] PROGMEM = {
  0x80, 0xC0, 0xE0, 0xF0, 0xF0, 0xE0, 0xC0, 0x80
};

// duas barras verticais (Pause)
static const unsigned char ICONE_PAUSE[] PROGMEM = {
  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66
};

// triangulo + barra (Proxima faixa)
static const unsigned char ICONE_PROXIMA[] PROGMEM = {
  0x82, 0xC2, 0xE2, 0xF2, 0xF2, 0xE2, 0xC2, 0x82
};

// seta (Selecionar / confirmar)
static const unsigned char ICONE_SELECIONAR[] PROGMEM = {
  0x10, 0x18, 0x1C, 0xFE, 0xFE, 0x1C, 0x18, 0x10
};

// X estilizado (Aleatorio / shuffle), versao pequena 8x8 usada na barrinha
// de dicas do rodape, onde so cabem icones desse tamanho.
static const unsigned char ICONE_ALEATORIO[] PROGMEM = {
  0xC3, 0x66, 0x3C, 0x18, 0x18, 0x3C, 0x66, 0xC3
};

// setas cruzadas (Aleatorio / shuffle), 16x16, mais reconhecivel que a
// versao 8x8 acima. Usada como indicador de status "aleatorio ligado" no
// cabecalho das telas, onde tem espaco de sobra (a faixa amarela tem
// exatamente 16px de altura).
static const unsigned char ICONE_ALEATORIO_16[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x0e, 0x7e, 0x7e, 0x0f, 0xfe, 0x03, 0xcc, 0x03, 0xc0,
  0x03, 0xc0, 0x03, 0xcc, 0x0f, 0xfe, 0x7e, 0x7e, 0x00, 0x0e, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00
};

#endif
