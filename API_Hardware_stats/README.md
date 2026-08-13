# API_Hardware_stats

Servidor local em Python (FastAPI) que lê o uso de CPU, RAM, bateria e GPU(s) da máquina e transmite os dados continuamente via *Server-Sent Events* (SSE), para serem consumidos por um cliente. Neste repositório, o cliente é o ESP32 do projeto [`monitora_desem/`](../monitora_desem), que exibe os números num display OLED. Veja o README daquele projeto para a explicação completa do par cliente/servidor e o passo a passo de configuração.

## Rodando o servidor

```bash
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

Sobe em `http://0.0.0.0:8000`, com o stream disponível em `GET /combo`. No Windows, pode ser necessário rodar como administrador para o `PyLibreHardwareMonitor` conseguir ler os sensores de GPU/placa-mãe.

## Testando sem o ESP32

Com o servidor rodando, é possível ver o stream direto no navegador ou via `curl`:

```bash
curl http://localhost:8000/combo
```
