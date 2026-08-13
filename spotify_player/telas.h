#ifndef TELAS_H
#define TELAS_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern int OLED_RESET;
extern Adafruit_SSD1306 display;

extern int topo;

// Inicializa o barramento I2C e o display OLED. Chamar uma vez no setup().
bool IniciarTelas();

// Tela de lista (playlists). shuffleAtivo so controla o destaque do icone
// de aleatorio na barra de dicas do rodape.
void printMenu(bool ehMenuTocando, std::vector<String> &listaOpcoes, int atual, bool shuffleAtivo);

void ExibirOLED(String msn);

// Tela "Tocando agora": nome da musica (com rolagem se nao couber), artista,
// status, volume, barra de progresso e barra de dicas dos botoes.
void TelaTocando(String musica, String artista, bool tocando, unsigned long progressoMs, unsigned long duracaoMs, bool shuffleAtivo, int volume);

#endif
