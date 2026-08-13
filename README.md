# Projetos ESP32

Coleção de projetos pessoais com o ESP32, feitos como trabalhos e experimentos durante a UTFPR. Cada pasta é um sketch independente para a Arduino IDE.

Autor: [github.com/Dirceu06](https://github.com/Dirceu06)

## Hardware comum

A maioria dos projetos usa a mesma base:

- **Placa:** ESP32 Dev Module (DOIT ESP32 DEVKIT V1)
- **Display:** OLED SSD1306 128x64, I2C (geralmente SDA -> GPIO27, SCL -> GPIO26)
- Potenciômetro, botões e sensores variam por projeto (veja o README de cada um)

## Configuração de credenciais

Os projetos que usam WiFi ou APIs externas guardam as credenciais reais num arquivo `config.h` (ou `env.h`, no caso do `relogio`) que **não é versionado**, conforme o `.gitignore`. Para rodar qualquer um desses projetos:

1. Entre na pasta do projeto.
2. Copie o arquivo `config.example.h` (ou `env.example.h`) para `config.h` (ou `env.h`).
3. Preencha com o nome/senha da sua rede WiFi e, se pedido, a chave da API usada.
4. Abra o `.ino` na Arduino IDE normalmente. O `config.h` já fica na mesma pasta e é incluído automaticamente.

## Bibliotecas necessárias

Instale pelo Library Manager da Arduino IDE (`Sketch > Incluir Biblioteca > Gerenciar Bibliotecas`), conforme o projeto:

- `Adafruit SSD1306` + `Adafruit GFX Library` + `Adafruit BusIO`: display OLED
- `ArduinoJson`: parsing de JSON (Spotify, monitor de desempenho, etc.)
- `DHT sensor library` + `Adafruit Unified Sensor`: sensor de temperatura/umidade
- `IRremote`: captura e emissão de sinais infravermelho
- `WebServer`, `HTTPClient`, `WiFi`, `Preferences`: nativas do core ESP32, não precisam instalar

A pasta `libraries/` (código de terceiros baixado localmente) fica fora do git, já que não faz sentido versionar dependências de terceiros dentro do repositório.

## Projetos em destaque

| Projeto | Descrição |
|---|---|
| [`spotify_player/`](spotify_player) | Controle remoto de Spotify com display OLED, dual-core FreeRTOS, potenciômetro e botões físicos |
| [`copiador_sinal/`](copiador_sinal) | Controle universal infravermelho com painel web e armazenamento persistente |
| [`relogio/`](relogio) | Relógio de mesa com sincronização NTP e sensor de temperatura/umidade |
| [`monitora_desem/`](monitora_desem) + [`API_Hardware_stats/`](API_Hardware_stats) | Monitor de desempenho do PC (CPU/RAM/GPU) exibido no OLED via streaming (SSE), com servidor em Python |

