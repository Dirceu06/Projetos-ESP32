# monitora_desem + API_Hardware_stats

Monitor de desempenho do PC (uso de CPU, RAM e GPU) exibido em tempo real num display OLED conectado ao ESP32. Dois componentes trabalham juntos:

- **`API_Hardware_stats/`** (Python): um servidor local que lê os sensores de hardware do PC e transmite os dados continuamente via *Server-Sent Events* (SSE).
- **`monitora_desem/`** (este projeto, ESP32): um cliente que se conecta ao servidor, recebe o stream e desenha CPU/RAM/GPU no OLED.

## Como funciona

O servidor Python (`API_Hardware_stats/main.py`) usa `psutil` para CPU/RAM e a biblioteca `PyLibreHardwareMonitor` para as GPUs, e expõe um endpoint FastAPI (`GET /combo`) que fica enviando um `data: {json}\n\n` por segundo, o formato padrão de SSE. O ESP32 abre uma conexão TCP crua para esse endpoint (`WiFiClient`), lê o stream linha a linha, filtra as linhas que começam com `data:` e faz o parsing do JSON com `ArduinoJson` para atualizar o display.

Essa mesma lógica de cliente SSE (`OLED_WiFiClient_SSE`) também está duplicada em `relogio/wifi.cpp`, disponível para reaproveitamento ali.

## Hardware (lado ESP32)

- ESP32 Dev Module
- Display OLED SSD1306 128x64 (I2C): SDA -> GPIO27, SCL -> GPIO26

## Bibliotecas necessárias (ESP32)

- `Adafruit SSD1306`, `Adafruit GFX Library`, `Adafruit BusIO`
- `ArduinoJson`
- `WiFi` (nativa do core ESP32)

## Configuração

**1. Servidor (no seu PC Windows, onde estão os sensores de hardware):**

```bash
cd API_Hardware_stats
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

O servidor sobe em `http://0.0.0.0:8000`. Rode como administrador se o `PyLibreHardwareMonitor` não conseguir ler os sensores de GPU/placa-mãe (é comum precisar de privilégios elevados no Windows para acessar esses sensores).

**2. ESP32:**

1. Copie `config.example.h` para `config.h` (mesma pasta).
2. Preencha `WIFI_SSID` e `WIFI_PASSWORD` com os dados da sua rede.
3. Em `monitora_desem.ino`, ajuste o IP `192.168.200.105` (usado tanto na conexão SSE quanto num comentário sobre o ViaCEP, que não é usado por este sketch) para o IP local da máquina onde o servidor Python está rodando.
4. Faça o upload do `.ino`.

## Uso

Este sketch é um cliente de teste controlado pelo Monitor Serial (115200 baud): digite `1` e pressione Enter para iniciar o stream (o OLED passa a mostrar CPU/RAM/GPU0/GPU1 atualizados a cada segundo). Digite `X` para encerrar a conexão.
