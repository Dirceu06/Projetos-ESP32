#include "telas.h"
#include "icones.h"

int SCREEN_WIDTH = 128;
int SCREEN_HEIGHT = 64;
int OLED_RESET = -1;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int topo = 0;
int altura = 17;
String texto[2];

// Endereço I2C mais comum pros modulos OLED SSD1306 128x64. Se o display não
// iniciar, tente 0x3D (o outro endereço comum) e confira a fiação SDA/SCL.
#define ENDERECO_OLED 0x3C

// Esse OLED e bicolor: as primeiras 16 linhas sao amarelas e o resto e azul
// (duas faixas de LED fisicas, fixas). Nenhum texto pode cruzar essa linha,
// senao sai com metade numa cor e metade na outra.
#define ALTURA_FAIXA_AMARELA 16

bool IniciarTelas() {
  Wire.begin(27, 26); // SDA = GPIO27, SCL = GPIO26

  if (!display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED)) {
    Serial.println("Falha ao iniciar o display OLED! Confira o endereco I2C (0x3C/0x3D) e a fiacao.");
    return false;
  }

  display.clearDisplay();
  display.setTextWrap(false); // sem isso, texto que passa da borda pula pra linha de baixo
  display.display();
  return true;
}

// Corta o texto se ele for maior que o numero de caracteres que cabem na tela
// (a fonte tamanho 1 do Adafruit_GFX tem ~6px por caractere; 128px / 6 = ~21).
static String cortar(String texto, int maxChars) {
  if ((int)texto.length() <= maxChars) return texto;
  return texto.substring(0, maxChars - 3) + "...";
}

// Desenha um texto numa linha; se ele nao couber na largura da tela, fica
// rolando da direita pra esquerda em loop. Guarda o progresso em "offset"
// (por referencia) e reinicia sozinho quando o texto muda. Chamar
// periodicamente (ex: a cada ~150ms) pra animar.
static void DesenharTextoRolante(const String &texto, int y, String &textoAnterior, int &offset) {
  const int LARGURA_TELA = 128;
  const int PX_POR_CHAR = 6; // fonte padrao, tamanho 1

  if (texto != textoAnterior) {
    textoAnterior = texto;
    offset = 0;
  }

  int larguraTexto = (int)texto.length() * PX_POR_CHAR;

  if (larguraTexto <= LARGURA_TELA) {
    display.setCursor(0, y);
    display.print(texto);
    return;
  }

  String textoComEspaco = texto + "     "; // "respiro" antes de repetir
  int larguraCompleta = (int)textoComEspaco.length() * PX_POR_CHAR;

  display.setCursor(-offset, y);
  display.print(textoComEspaco);
  display.setCursor(-offset + larguraCompleta, y);
  display.print(texto);

  offset += 2; // velocidade da rolagem, em pixels por chamada
  if (offset > larguraCompleta) offset = 0;
}

// Indicador de "aleatorio" no cabecalho (faixa amarela), usa o icone grande
// 16x16. Quando ativo, desenha um fundo solido atras pra ficar obvio de
// longe se ta ligado ou desligado, sem precisar de texto.
static void DesenharIconeAleatorio16(int x, int y, bool ativo) {
  if (ativo) {
    display.fillRoundRect(x - 2, y, 20, 16, 3, SSD1306_WHITE);
    display.drawBitmap(x, y, ICONE_ALEATORIO_16, 16, 16, SSD1306_BLACK);
  } else {
    display.drawBitmap(x, y, ICONE_ALEATORIO_16, 16, 16, SSD1306_WHITE);
  }
}

// Barra de dicas no rodape: sempre mostra o icone de MENU a esquerda (troca
// de tela); os outros dois icones mudam conforme a tela atual.
// "destacarDireita" desenha um fundo solido atras do icone da direita
// (usado pro estado "aleatorio ativo").
static void DesenharBarraDeAtalhos(const uint8_t* iconeMeio, const uint8_t* iconeDireita, bool destacarDireita = false) {
  const int y = 55;

  display.drawFastHLine(0, 52, SCREEN_WIDTH, SSD1306_WHITE);

  display.drawBitmap(6, y, ICONE_MENU, 8, 8, SSD1306_WHITE);
  display.drawBitmap(60, y, iconeMeio, 8, 8, SSD1306_WHITE);

  if (destacarDireita) {
    display.fillRoundRect(110, y - 1, 12, 10, 2, SSD1306_WHITE);
    display.drawBitmap(112, y, iconeDireita, 8, 8, SSD1306_BLACK);
  } else {
    display.drawBitmap(112, y, iconeDireita, 8, 8, SSD1306_WHITE);
  }
}

void printMenu(bool ehMenuTocando, std::vector<String> &listaOpcoes, int atual, bool shuffleAtivo) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(ehMenuTocando ? "Tocando" : "Discos");

  if (listaOpcoes.empty()) {
    display.setTextSize(1);
    display.setCursor(0, 25);
    display.print("Carregando...");
    display.display();
    return;
  }

  // contador "atual/total" no canto superior direito, antes do icone de aleatorio
  display.setTextSize(1);
  display.setCursor(74, 4);
  display.print(String(atual + 1) + "/" + String(listaOpcoes.size()));

  DesenharIconeAleatorio16(110, 0, shuffleAtivo);

  // caixa de destaque: alterna entre a posicao de cima e a de baixo pra
  // mostrar sempre 2 opcoes (a atual e a seguinte) com a atual marcada
  if (atual > topo) {
    altura = 35;
    if (atual > topo + 1) {
      topo = atual - 1;
    }
  } else if (atual <= topo) {
    altura = 17;
    topo = atual;
  }

  texto[0] = listaOpcoes[topo];
  if (topo + 1 < (int)listaOpcoes.size()) {
    texto[1] = listaOpcoes[topo + 1];
  } else {
    texto[1] = "";
  }

  display.drawRect(0, altura, 125, 16, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(3, 20);
  display.print(cortar(texto[0], 21));

  display.setCursor(3, 38);
  display.print(cortar(texto[1], 21));

  // botao MENU = trocar de tela | botao SELECIONAR = tocar a opcao marcada |
  // botao PLAY/PAUSE = liga/desliga o modo aleatorio
  DesenharBarraDeAtalhos(ICONE_SELECIONAR, ICONE_ALEATORIO);

  display.display();
}

void ExibirOLED(String msn) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print(msn);
  display.display();
}

void TelaTocando(String musica, String artista, bool tocando, unsigned long progressoMs, unsigned long duracaoMs, bool shuffleAtivo, int volume) {
  static String musicaAnterior = "";
  static String artistaAnterior = "";
  static int offsetMusica = 0;
  static int offsetArtista = 0;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // faixa amarela (y = 0 a 15): status, volume e icone de aleatorio
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(tocando ? "Tocando" : "Pausado");

  display.setTextSize(1);
  display.setCursor(80, 4);
  if (volume < 100) display.print(" ");
  display.print(volume);
  display.print("%");

  DesenharIconeAleatorio16(110, 0, shuffleAtivo);

  // faixa azul (y = 16 em diante): resto do conteudo
  display.setTextSize(1);

  if (musica.length() == 0) {
    display.setCursor(0, 24);
    display.print("Nada tocando");
    display.setCursor(0, 36);
    display.print("no momento...");
  } else {
    DesenharTextoRolante(musica, 17, musicaAnterior, offsetMusica);
    DesenharTextoRolante(artista, 26, artistaAnterior, offsetArtista);

    // barra de progresso
    int x0 = 2, y0 = 36, largura = 124, alturaBarra = 6;
    display.drawRect(x0, y0, largura, alturaBarra, SSD1306_WHITE);
    if (duracaoMs > 0) {
      int preenchido = (int)(((float)progressoMs / (float)duracaoMs) * (largura - 2));
      if (preenchido > largura - 2) preenchido = largura - 2;
      if (preenchido > 0) display.fillRect(x0 + 1, y0 + 1, preenchido, alturaBarra - 2, SSD1306_WHITE);
    }

    // tempo decorrido / duracao total, formato mm:ss
    unsigned long progSeg = progressoMs / 1000;
    unsigned long durSeg = duracaoMs / 1000;
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu / %02lu:%02lu",
             (progSeg / 60) % 60, progSeg % 60,
             (durSeg / 60) % 60, durSeg % 60);

    display.setCursor(0, 44);
    display.print(buffer);
  }

  // botao MENU = trocar de tela | botao SELECIONAR = proxima faixa |
  // botao PLAY/PAUSE = pausa/retoma
  DesenharBarraDeAtalhos(ICONE_PROXIMA, tocando ? ICONE_PAUSE : ICONE_PLAY);

  display.display();
}
