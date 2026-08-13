#include "oled.h"
#include "wifi.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool conectado = false;

void telaPropaganda(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("github.com/Dirceu06\n");
  display.drawBitmap(0, 16, gitProfile, 128, 48, SSD1306_WHITE);
  display.display();
}

void exibirRelogio(float t) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("Sem hora sincronizada\nTEMPERATURA: %.2f", t);
    display.display();
    return;
  }

  char horaStr[9];  // HH:MM:SS
  char dataStr[11]; // DD/MM/AAAA
  strftime(horaStr, sizeof(horaStr), "%H:%M:%S", &timeinfo);
  strftime(dataStr, sizeof(dataStr), "%d/%m/%Y", &timeinfo);

  display.clearDisplay();

  // Faixa amarela (y=0 a 15) - cabecalho
  display.setTextSize(1);
  display.setCursor(0, 4);
  display.println("RELOGIO");

  display.setTextSize(1);
  display.setCursor(90, 4);
  display.printf("%.1f C", t);


  // Faixa azul (y=16 em diante) - conteudo principal
  display.setTextSize(2);
  display.setCursor(10, 26);
  display.println(horaStr);

  display.setTextSize(1);
  display.setCursor(20, 50);
  display.println(dataStr);

  display.display();
}

void ExibirOLED(char *msn) {
  display.setTextSize(1);
  display.setCursor(0, 55);
  display.printf("%s\n", msn);
  display.display();
}

void desempenho(){
  float* infos;
  if(!conectado){
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("aguarde...");
    display.display();

  }

  infos = OLED_WiFiClient_SSE(&conectado);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("CPU: "); display.print(infos[0]); display.println(" %");
  display.setCursor(0, 16);
  display.print("RAM: "); display.print(infos[1]); display.println(" %");

  display.setCursor(0, 28);
  display.print("GPU0: "); display.print(infos[2]); display.println(" %");
  display.setCursor(0, 42);
  display.print("GPU1: "); display.print(infos[3]); display.println(" %");

  display.display();
}
