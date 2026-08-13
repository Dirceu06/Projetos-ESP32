#include <WiFi.h>
#include <WebServer.h>
#include "servidor.h"
#include "sinal.h"
#include "config.h" // credenciais reais ficam em config.h (fora do git, veja .gitignore e config.example.h)

// Inicia o servidor web na porta 80
WebServer server(80);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

String getHeader() {
    return "<!DOCTYPE html><html><head>"
           "<meta charset=\"UTF-8\">"
           "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0\">"
           "<style>"
           ":root { --bg: #0f0f11; --surface: #1e1e24; --primary: #00adb5; --text: #e1e1e6; --danger: #ef233c; }"
           "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 25px 15px; text-align: center; user-select: none; -webkit-tap-highlight-color: transparent; }"
           "h1 { color: var(--primary); font-size: 24px; font-weight: 600; letter-spacing: 0.5px; margin-bottom: 30px; }"
           ".grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap: 15px; max-width: 500px; margin: 0 auto; }"
           ".btn { background: var(--surface); color: var(--text); padding: 18px 10px; border-radius: 16px; text-decoration: none; font-size: 16px; font-weight: 500; border: 1px solid rgba(255,255,255,0.05); box-shadow: 0 4px 15px rgba(0,0,0,0.2); transition: all 0.15s ease; display: flex; align-items: center; justify-content: center; min-height: 60px; cursor: pointer; font-family: inherit; width: 100%; box-sizing: border-box; outline: none; }"
           ".btn:active { transform: scale(0.92) !important; background: #2a2a32; }"
           ".disabled { opacity: 0.3; border-color: rgba(239, 35, 60, 0.4); pointer-events: none; }"
           ".back { background: rgba(239, 35, 60, 0.1); color: var(--danger); grid-column: 1 / -1; margin-top: 20px; }"
           "</style>"
           "<script>"
           "function enviar(d, c, btn) {"
           "  /* Efeito visual imediato ao tocar */"
           "  btn.style.transform = 'scale(0.92)';"
           "  btn.style.background = 'var(--primary)';"
           "  btn.style.color = '#000';"
           "  setTimeout(() => { btn.style.transform = ''; btn.style.background = ''; btn.style.color = ''; }, 200);"
           "  /* Envia o sinal sem recarregar a página */"
           "  fetch('/enviar?disp=' + d + '&cmd=' + c).catch(e => console.log(e));"
           "}"
           "</script>"
           "</head><body>";
}

String getFooter() {
    return "</body></html>";
}

// Página Inicial: Mostra os Dispositivos
void handleRoot() {
    String html = getHeader();
    html += "<h1>Meus Dispositivos</h1>";
    html += "<div class=\"grid\">"; // Inicia a grelha

    for (int i = 0; i < listaDispositivos.size(); i++) {
        html += "<a class=\"btn\" href=\"/disp?id=" + String(i) + "\">" + listaDispositivos[i].nome + "</a>";
    }

    if (listaDispositivos.size() == 0) {
        html += "<p style=\"grid-column: 1 / -1;\">Ainda não tens dispositivos.<br>Cria um 'Novo +' no ecrã OLED!</p>";
    }

    html += "</div>"; // Fecha a grelha
    html += getFooter();
    server.send(200, "text/html", html);
}

// Página do Dispositivo: Mostra os Comandos (Power, Vol+, etc)
void handleDisp() {
    if (!server.hasArg("id")) {
        server.sendHeader("Location", "/");
        server.send(303);
        return;
    }

    int id = server.arg("id").toInt();
    if (id < 0 || id >= listaDispositivos.size()) {
        server.sendHeader("Location", "/");
        server.send(303);
        return;
    }

    String html = getHeader();
    html += "<h1>" + listaDispositivos[id].nome + "</h1>";
    html += "<div class=\"grid\">"; // Inicia a grelha

    for (int i = 0; i < listaDispositivos[id].comandos.size(); i++) {
        if (listaDispositivos[id].comandos[i].temSinal) {
            // Agora usa um <button> que chama a função JavaScript em vez de recarregar a página
            html += "<button class=\"btn\" onclick=\"enviar(" + String(id) + ", " + String(i) + ", this)\">" + listaDispositivos[id].comandos[i].nome + "</button>";
        } else {
            html += "<div class=\"btn disabled\">" + listaDispositivos[id].comandos[i].nome + "</div>";
        }
    }

    html += "<a class=\"btn back\" href=\"/\">⬅ Voltar</a>";
    html += "</div>"; // Fecha a grelha
    html += getFooter();
    server.send(200, "text/html", html);
}

// Ação de Enviar Sinal IR (agora recebe os comandos em pano de fundo)
void handleEnviar() {
    if (server.hasArg("disp") && server.hasArg("cmd")) {
        int d = server.arg("disp").toInt();
        int c = server.arg("cmd").toInt();

        if (d >= 0 && d < listaDispositivos.size() && c >= 0 && c < listaDispositivos[d].comandos.size()) {
            if (listaDispositivos[d].comandos[c].temSinal) {
                enviarSinal(listaDispositivos[d].comandos[c]);
            }
        }
        // Responde apenas "OK", para a função Fetch no JavaScript saber que funcionou
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Erro");
    }
}

void initWiFi() {
    Serial.println("\n----------------------------------");
    Serial.print("A ligar à rede Wi-Fi: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nLIGADO COM SUCESSO!");
    Serial.print("Abra o navegador e digite o IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("----------------------------------\n");

    server.on("/", handleRoot);
    server.on("/disp", handleDisp);
    server.on("/enviar", handleEnviar);

    server.begin();
}

void gerirServidor() {
    server.handleClient();
}
