#ifndef SPOTIFY_WIFI_H
#define SPOTIFY_WIFI_H

#include <Arduino.h>

extern const char* ssid;
extern const char* senha;

// Requisição genérica (GET) autenticada na API do Spotify.
String GetSpotify(String url, String authorization = "");

// Troca o refresh_token por um access_token novo (válido por ~1h).
String getSpotifyToken(String clientId, String clientSecret, String refreshToken);

// Lista as playlists do usuário (GET /v1/me/playlists).
String GetPlaylists(String authorization);

// Manda tocar uma playlist/álbum específico (context_uri no formato "spotify:playlist:...").
// Requer um dispositivo Spotify Connect ativo (celular/PC com o Spotify aberto).
bool SpotifyPlayContext(String authorization, String contextUri);

// Retoma/pausa a reprodução no dispositivo ativo.
bool SpotifyResume(String authorization);
bool SpotifyPause(String authorization);

// Pula pra proxima faixa.
bool SpotifySkipNext(String authorization);

// Liga/desliga o modo aleatorio (shuffle).
bool SpotifyShuffle(String authorization, bool ativo);

// Ajusta o volume do dispositivo ativo (0 a 100).
bool SpotifySetVolume(String authorization, int volumePercent);

#endif
