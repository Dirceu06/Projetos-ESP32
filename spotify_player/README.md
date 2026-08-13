# spotify_player

Controle remoto físico para o Spotify, construído com um ESP32 e um display OLED. Mostra a faixa/artista atual com rolagem de texto, barra de progresso, volume e estado do modo aleatório, e controla play/pause, próxima faixa, volume e navegação/reprodução de playlists, tudo por hardware.

## Destaques técnicos

- **Dual-core de verdade:** o ESP32 tem dois núcleos, e o projeto usa os dois. O core 1 cuida só da UI (botões, potenciômetro, redesenho da tela); o core 0 roda uma tarefa dedicada do FreeRTOS que fala com a API do Spotify. Isso evita que uma chamada HTTPS (que pode levar centenas de ms) trave a animação da tela ou a leitura dos botões.
- **Comunicação entre núcleos** via mutexes (protegendo o estado compartilhado) e uma fila de "ações": o core 1 enfileira o que quer fazer (pausar, pular faixa, mudar volume) e o core 0 executa.
- **Progresso da música interpolado:** como a API só é consultada a cada 2s, a posição da faixa na tela é estimada localmente com `millis()` entre uma consulta e outra, e corrigida quando a resposta real chega. Fica fluido, sem "saltos".
- **"Pickup mode" no potenciômetro:** o mesmo potenciômetro navega playlists numa tela e controla volume na outra. Para evitar saltos de volume ao trocar de tela, o valor do potenciômetro só passa a valer quando ele se aproxima do volume que já está tocando, o mesmo truque usado em faders não motorizados de mesas de som.
- **Debounce em três camadas** nos botões (ISR, consumo por botão e entre botões diferentes), pensado especificamente para lidar com contato ruim de protoboard e ruído cruzado entre GPIOs.

## Hardware

- ESP32 Dev Module
- Display OLED SSD1306 128x64 (I2C): SDA -> GPIO27, SCL -> GPIO26
- Potenciômetro: pernas nos extremos em 3V3/GND, wiper -> GPIO33
- Botão MENU -> GPIO14 (outro terminal no GND)
- Botão SELECIONAR -> GPIO32 (outro terminal no GND)
- Botão PLAY/PAUSE -> GPIO25 (outro terminal no GND)

## Bibliotecas necessárias

- `Adafruit SSD1306`, `Adafruit GFX Library`, `Adafruit BusIO`
- `ArduinoJson`
- `WiFi`, `HTTPClient` (nativas do core ESP32)

## Configuração

1. Copie `config.example.h` para `config.h` (mesma pasta).
2. Preencha `WIFI_SSID` e `WIFI_PASSWORD` com os dados da sua rede.
3. Crie um app em [developer.spotify.com/dashboard](https://developer.spotify.com/dashboard) e preencha `SPOTIFY_CLIENT_ID` e `SPOTIFY_CLIENT_SECRET`.
4. Obtenha um **refresh token** via Authorization Code Flow, autorizando os escopos:
   - `playlist-read-private`
   - `playlist-read-collaborative`
   - `user-modify-playback-state`
   - `user-read-playback-state`
   - `user-read-currently-playing`

   O fluxo completo é feito uma única vez, pelo navegador (a documentação oficial do Spotify explica o passo a passo em [Authorization Code Flow](https://developer.spotify.com/documentation/web-api/tutorials/code-flow)). Cole o token resultante em `SPOTIFY_REFRESH_TOKEN`.
5. Abra `spotify_player.ino` na Arduino IDE e faça o upload.

O refresh token não expira sozinho, só se o acesso do app for revogado na conta Spotify (em [spotify.com/account/apps](https://www.spotify.com/account/apps)).

## Uso

**Tela "Tocando":**
- Potenciômetro: controla o volume do dispositivo ativo (0 a 100%)
- Botão PLAY/PAUSE: pausa/retoma
- Botão SELECIONAR: pula para a próxima faixa
- Botão MENU: vai para a tela de playlists

**Tela "Playlists":**
- Potenciômetro: navega entre as playlists
- Botão SELECIONAR: toca a playlist marcada
- Botão PLAY/PAUSE: liga/desliga o modo aleatório
- Botão MENU: volta para a tela "Tocando"
