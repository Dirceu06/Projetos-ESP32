#include "telas.h"

int SCREEN_WIDTH = 128;
int SCREEN_HEIGHT = 64;
int OLED_RESET = -1;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int topo = 0;
int altura = 16;
String texto[2];

void printMenu(bool ehMenuDispositivos, std::vector<String> &listaOpcoes, int atual) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (ehMenuDispositivos) display.print("Controles");
  else display.print("Comandos");
  
  if (atual > topo) {
    altura = 33;
    if (atual > topo + 1) {
      topo = atual - 1;
    }
  } else if (atual <= topo) {
    altura = 16;
    topo = atual;
  }

  texto[0] = listaOpcoes[topo];
  if (topo + 1 < listaOpcoes.size()) {
    texto[1] = listaOpcoes[topo + 1];
  } else {
    texto[1] = "";
  }

  display.drawRect(0, altura, 125, 20, SSD1306_WHITE);
  
  display.setCursor(3, 18);
  display.print(texto[0]);
  
  display.setCursor(3, 37);
  display.print(texto[1]);

  // Ícones só aparecem para itens normais (não aparecem em "< Voltar" ou "Novo +")
  String opcaoAtual = listaOpcoes[atual];
  if (opcaoAtual != "< Voltar" && opcaoAtual != "Novo +") {
    if (ehMenuDispositivos) {
      display.drawBitmap(85, altura + 2, lapis, 16, 16, SSD1306_WHITE);
      display.drawBitmap(108, altura + 2, play, 16, 16, SSD1306_WHITE);
    } else {
      display.drawBitmap(85, altura + 2, repetir, 16, 16, SSD1306_WHITE);
      display.drawBitmap(108, altura + 2, play, 16, 16, SSD1306_WHITE);
    }
  }

  display.display();
}

void ExibirOLED(String msn) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print(msn);
  display.display();
}