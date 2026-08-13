# copiador_sinal

Controle remoto universal por infravermelho, construído com ESP32. Captura sinais IR de controles remotos existentes, guarda de forma organizada por dispositivo/comando, e permite reenviá-los tanto pelos botões físicos (com display OLED) quanto por um painel web responsivo servido pelo próprio ESP32, dá pra trocar de canal pelo celular sem precisar de nenhum app.

## Destaques técnicos

- **Armazenamento persistente:** dispositivos e comandos (incluindo os sinais IR capturados) são salvos na flash do ESP32 via `Preferences` (NVS), sobrevivendo a reinicializações e quedas de energia.
- **Painel web full-stack no próprio ESP32:** o `WebServer` serve uma página com CSS e JavaScript embutidos (grid responsiva, feedback visual ao tocar, `fetch()` para enviar comandos sem recarregar a página), sem depender de nenhum servidor externo.
- **Menu por níveis no OLED:** navegação hierárquica (lista de dispositivos -> lista de comandos de cada dispositivo), com combinações de botões distintas para renomear (B1+B2), capturar sinal (B1) e enviar sinal (B2).
- **Standby automático:** a tela desliga sozinha depois de alguns segundos sem uso, e acorda no primeiro toque em qualquer botão.

## Hardware

- ESP32 Dev Module
- Display OLED SSD1306 128x64 (I2C): SDA -> GPIO27, SCL -> GPIO26
- Receptor IR -> GPIO13
- Emissor IR (LED IR) -> GPIO12
- Potenciômetro (navegação dos menus): wiper -> GPIO33
- Botão 1 -> GPIO32
- Botão 2 -> GPIO35
- Buzzer -> GPIO25

## Bibliotecas necessárias

- `Adafruit SSD1306`, `Adafruit GFX Library`, `Adafruit BusIO`
- `IRremote`
- `WiFi`, `WebServer`, `Preferences` (nativas do core ESP32)

## Configuração

1. Copie `config.example.h` para `config.h` (mesma pasta).
2. Preencha `WIFI_SSID` e `WIFI_PASSWORD` com os dados da sua rede.
3. Abra `copiador_sinal.ino` na Arduino IDE e faça o upload.
4. No Monitor Serial (115200 baud), veja o IP que o ESP32 recebeu: é por ele que o painel web fica acessível (`http://<IP-do-ESP32>/`).

## Uso

**No OLED / botões físicos:**
- Potenciômetro: navega entre dispositivos (ou comandos, dentro de um dispositivo)
- Botão 1 sozinho: grava um novo sinal IR no comando selecionado (ou cria um dispositivo/comando, se estiver em "Novo +")
- Botão 2 sozinho: envia o sinal IR do comando selecionado (ou cria um dispositivo/comando, se estiver em "Novo +")
- Botões 1+2 juntos: renomeia o dispositivo/comando selecionado (nome digitado pelo Monitor Serial)

**No painel web:** acesse o IP do ESP32 pelo navegador, escolha o dispositivo e toque no comando desejado para disparar o sinal IR correspondente.
