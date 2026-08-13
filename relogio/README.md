# relogio

Relógio de mesa com ESP32: sincroniza a hora pela internet (NTP) e mostra a temperatura ambiente (DHT22), num único display OLED, navegado por um botão.

## Funcionalidades

O botão físico alterna entre duas telas:

1. **Relógio:** hora e data sincronizadas por NTP, com a temperatura atual (lida do DHT22) no canto.
2. **Propaganda:** tela com o perfil do GitHub do autor.

## Hardware

- ESP32 Dev Module
- Display OLED SSD1306 128x64 (I2C): SDA -> GPIO27, SCL -> GPIO26
- Sensor DHT22 (temperatura/umidade) -> GPIO14
- Botão de navegação -> GPIO32

## Bibliotecas necessárias

- `Adafruit SSD1306`, `Adafruit GFX Library`, `Adafruit BusIO`
- `DHT sensor library`, `Adafruit Unified Sensor`
- `ArduinoJson`
- `WiFi` (nativa do core ESP32)

## Configuração

1. Copie `env.example.h` para `env.h` (mesma pasta).
2. Preencha `WIFI_NOME` e `WIFI_SENHA` com os dados da sua rede.
3. Abra `relogio.ino` na Arduino IDE e faça o upload.

O fuso horário está fixo em UTC-3 (Brasil) em `NTP.cpp`. Ajuste `gmtOffset_sec` se for usar em outro fuso.

## Uso

Pressione o botão de navegação para alternar entre as duas telas (Relógio -> Propaganda -> Relógio...).

> Notas: este projeto carrega, sem usar atualmente, o mesmo código-cliente de streaming (SSE) usado no par `monitora_desem`/`API_Hardware_stats` (função `desempenho()` em `oled.cpp`). Ele não é chamado no loop principal, mas está disponível para quem quiser reativar a tela de monitoramento de desempenho do PC aqui também.
>
> As funções de captura/emissão de sinal infravermelho (`IV.cpp`/`IV.h`) e o buzzer (`buzz.cpp`/`buzz.h`) foram desativadas neste projeto; use o [`copiador_sinal/`](../copiador_sinal) para controle remoto IR. Os arquivos continuam na pasta, sem uso, e podem ser apagados com segurança.
