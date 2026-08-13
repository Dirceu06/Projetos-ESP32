from fastapi import FastAPI
from fastapi.responses import StreamingResponse
import asyncio
import json
import uvicorn
import psutil
from PyLibreHardwareMonitor import Computer

app = FastAPI()
computer = Computer()

# Gera os dados de hardware continuamente
async def gerar_dados_hardware():
    while True:
        data = dict()
        data['CPU'] = {}
        data['RAM'] = {}
        data['Bateria'] = {}
        data['GPU'] = []

        data['CPU']['Load'] = psutil.cpu_percent()
        data['RAM']['Load'] = psutil.virtual_memory().percent

        bateria = psutil.sensors_battery()
        data['Bateria']['Level'] = bateria.percent if bateria else 0

        qtd = 0
        for nome in computer.gpu:
            data['GPU'].append({'nome': nome, 'Load': max(computer.gpu[nome]['Load'].values())})
            try:
                data['GPU'][qtd]["Temperatura"] = computer.gpu[nome]['Temperature']['GPU Core']
            except:
                data['GPU'][qtd]["Temperatura"] = None
            qtd += 1

        json_texto = json.dumps(data)

        # Formato SSE: "data: {json}\n\n"
        yield f"data: {json_texto}\n\n"

        await asyncio.sleep(1) # intervalo entre cada envio para o ESP32

@app.get("/combo")
def combo():
    return StreamingResponse(gerar_dados_hardware(), media_type="text/event-stream")

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
