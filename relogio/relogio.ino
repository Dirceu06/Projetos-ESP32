#include <DHT.h>
#include "pinos.h"
#include "oled.h"
#include "wifi.h"
#include "NTP.h"

DHT dht(DHT_PIN, DHT_TYPE);


//variaveis de controle de telas e botão
int valor;
bool estadoAnterior = LOW;
bool estadoAtual;

// umidade e graus do dht
float h;
float t;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(27, 26);

  dht.begin();

  valor = 0;
  pinMode(BTT_PIN, INPUT_PULLDOWN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED] Falha ao inicializar - confira endereco I2C (0x3C ou 0x3D) e fiacao SDA/SCL");
  }else{
    display.setTextColor(SSD1306_WHITE);
  }

  conectarWiFi();

  configurarHora();

}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  estadoAtual = digitalRead(BTT_PIN);
  if(digitalRead(BTT_PIN) == HIGH && estadoAnterior == LOW){
    valor++;
    valor = valor%2;
  }
  estadoAnterior = estadoAtual;


  if (valor == 0){
    h = dht.readHumidity();
    t = dht.readTemperature();
    if (isnan(h) || isnan(t)) Serial.println("[DHT22] Falha na leitura");
    exibirRelogio(t);
  }else{
    telaPropaganda();
  }

}
