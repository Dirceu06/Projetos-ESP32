#include "SpotifyWifi.h"
#include "config.h" // credenciais reais ficam em config.h (fora do git, veja .gitignore e config.example.h)

#include "HardwareSerial.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = WIFI_SSID;
const char* senha = WIFI_PASSWORD;

String GetSpotify(String url, String authorization) {
    HTTPClient http;

    http.begin(url);

    if (authorization != "") {
      http.addHeader("Authorization", "Bearer " + authorization);

    }

    int codigo = http.GET();

    if (codigo < 200 || codigo >= 300) {
      http.end();
      Serial.printf("[GetSpotify] erro HTTP %d em %s\n", codigo, url.c_str());
      return "deu ruim";
    }

    String resultado = http.getString();

    http.end();

    // log sempre, nao so em erro: mostra o codigo HTTP e quantos bytes
    // vieram no corpo. Um 204 e "nada tocando" de verdade; um 200 com
    // poucos bytes pode ser um JSON sem "item" (ex: anuncio no Spotify Free).
    Serial.printf("[GetSpotify] HTTP %d, corpo com %d bytes\n", codigo, resultado.length());

    return resultado;
}

// Usa o refresh_token (obtido uma vez via Authorization Code Flow no navegador)
// para conseguir um access_token com contexto de usuário. O client_credentials
// flow NAO funciona para /me/player/... porque o token gerado por ele nao
// pertence a nenhum usuario (a Spotify responde 404 "Invalid username").
String getSpotifyToken(String clientId, String clientSecret, String refreshToken) {
    HTTPClient http;

    http.begin("https://accounts.spotify.com/api/token");

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body =
        "grant_type=refresh_token"
        "&refresh_token=" + refreshToken +
        "&client_id=" + clientId +
        "&client_secret=" + clientSecret;

    int codigo = http.POST(body);

    Serial.println("HTTP: " + String(codigo));

    String resposta = http.getString();

    http.end();

    return resposta;
}

String GetPlaylists(String authorization) {
    // "fields=" pede pra API so devolver nome e uri de cada playlist, sem isso
    // o JSON vem com imagens, dono, links etc., o que pesa demais pra memoria do ESP32.
    return GetSpotify("https://api.spotify.com/v1/me/playlists?limit=20&fields=items(name,uri)", authorization);
}

// Helper interno: manda um PUT autenticado pra API do Spotify.
// body vazio ("") = PUT sem corpo (ex: retomar reprodução no contexto atual).
static int SpotifyPUT(String url, String authorization, String body) {
    HTTPClient http;

    http.begin(url);
    http.addHeader("Authorization", "Bearer " + authorization);
    http.addHeader("Content-Type", "application/json");

    int codigo;
    if (body.length() > 0) {
      codigo = http.PUT(body);
    } else {
      // PUT sem corpo (play/pause) precisa mandar Content-Length: 0
      // explicitamente, senao a API do Spotify responde "411 Length Required".
      http.addHeader("Content-Length", "0");
      codigo = http.sendRequest("PUT");
    }

    if (codigo < 200 || codigo >= 300) {
      Serial.printf("erro PUT: %d\n", codigo);
    }

    http.end();

    return codigo;
}

bool SpotifyPlayContext(String authorization, String contextUri) {
    String body = "{\"context_uri\":\"" + contextUri + "\"}";
    int codigo = SpotifyPUT("https://api.spotify.com/v1/me/player/play", authorization, body);
    // 204 = sucesso sem conteudo (o normal aqui). 202 as vezes tambem aparece.
    return (codigo == 204 || codigo == 202);
}

bool SpotifyResume(String authorization) {
    int codigo = SpotifyPUT("https://api.spotify.com/v1/me/player/play", authorization, "");
    return (codigo == 204 || codigo == 202);
}

bool SpotifyPause(String authorization) {
    int codigo = SpotifyPUT("https://api.spotify.com/v1/me/player/pause", authorization, "");
    return (codigo == 204 || codigo == 202);
}

// Helper interno: manda um POST sem corpo autenticado pra API do Spotify
// (mesmo motivo do Content-Length no SpotifyPUT: sem isso da 411).
static int SpotifyPOSTVazio(String url, String authorization) {
    HTTPClient http;

    http.begin(url);
    http.addHeader("Authorization", "Bearer " + authorization);
    http.addHeader("Content-Length", "0");

    int codigo = http.sendRequest("POST");

    if (codigo < 200 || codigo >= 300) {
      Serial.printf("erro POST: %d\n", codigo);
    }

    http.end();

    return codigo;
}

bool SpotifySkipNext(String authorization) {
    int codigo = SpotifyPOSTVazio("https://api.spotify.com/v1/me/player/next", authorization);
    return (codigo == 204 || codigo == 202);
}

bool SpotifyShuffle(String authorization, bool ativo) {
    String url = "https://api.spotify.com/v1/me/player/shuffle?state=" + String(ativo ? "true" : "false");
    int codigo = SpotifyPUT(url, authorization, "");
    return (codigo == 204 || codigo == 202);
}

bool SpotifySetVolume(String authorization, int volumePercent) {
    if (volumePercent < 0) volumePercent = 0;
    if (volumePercent > 100) volumePercent = 100;
    String url = "https://api.spotify.com/v1/me/player/volume?volume_percent=" + String(volumePercent);
    int codigo = SpotifyPUT(url, authorization, "");
    return (codigo == 204 || codigo == 202);
}
