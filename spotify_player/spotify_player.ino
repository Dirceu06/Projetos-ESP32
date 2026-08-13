#include "config.h" // credenciais reais ficam em config.h (fora do git, veja .gitignore e config.example.h)
#include "SpotifyWifi.h"
#include "telas.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// Declarado no topo do arquivo para o prototipo automatico do Arduino nao
// cair antes da definicao do tipo (o .ino gera os prototipos e insere no
// inicio do arquivo).
struct SnapshotTocando {
  bool tocando;
  bool shuffle;
  int volume;
  String musica;
  String artista;
  unsigned long progresso;
  unsigned long duracao;
};

// Mesmo motivo do struct acima: precisa estar declarado antes dos
// prototipos automaticos do Arduino.
enum TipoAcao { ACAO_TOGGLE_PLAYPAUSE, ACAO_SKIP_NEXT, ACAO_SET_VOLUME };
struct AcaoRede {
  TipoAcao tipo;
  int valor; // ACAO_TOGGLE_PLAYPAUSE: 1 = estava tocando (precisa pausar) | 0 = estava pausado (precisa retomar)
             // ACAO_SET_VOLUME: volume desejado (0-100)
};

// Pinagem atual:
// Display OLED (I2C): SDA -> GPIO27, SCL -> GPIO26 (ver IniciarTelas em telas.cpp)
// Potenciometro: wiper -> GPIO33 (ADC1)
// Botao MENU -> GPIO14, Botao SELECIONAR -> GPIO32, Botao PLAY/PAUSE -> GPIO25
// (o outro terminal de cada botao vai pro GND)

#define PINO_POT            33
#define PINO_BTN_MENU       14
#define PINO_BTN_SELECIONAR 32
#define PINO_BTN_PLAYPAUSE  25

const char* client_id     = SPOTIFY_CLIENT_ID;
const char* client_secret = SPOTIFY_CLIENT_SECRET;
// Obtido via Authorization Code Flow com os escopos: playlist-read-private,
// playlist-read-collaborative, user-modify-playback-state,
// user-read-playback-state, user-read-currently-playing.
// Nao expira sozinho, a menos que o acesso do app seja revogado na conta.
const char* refresh_token = SPOTIFY_REFRESH_TOKEN;

// --------------------------- estado da aplicacao ---------------------------
//
// O ESP32 tem dois nucleos. A tarefa de rede roda no core 0 e cuida so da
// API do Spotify; o loop() (core 1) fica livre para ler botoes/potenciometro
// e redesenhar a tela sem esperar rede. Comunicacao entre os dois: variaveis
// protegidas por mutex e uma fila de acoes que o core 1 manda pro core 0.

enum Tela { TELA_TOCANDO, TELA_PLAYLISTS };
volatile Tela telaAtual = TELA_TOCANDO; // escrita pelo core 1 (UI), lida pelo core 0 (rede)

// Dados protegidos por mutexEstado (sempre acessar via as funcoes helper abaixo).
SemaphoreHandle_t mutexEstado;

// So uma chamada HTTPS por vez (core 0 ou core 1): duas conexoes TLS
// simultaneas deixam pouca RAM livre e causavam falha ao abrir a tela de
// playlists.
SemaphoreHandle_t mutexRede;

String access_token = "";
unsigned long tokenObtidoEm = 0; // so a tarefa de rede mexe nisso, nao precisa de mutex
const unsigned long TOKEN_VALIDADE_MS = 50UL * 60UL * 1000UL; // renova a cada 50 min (o token dura 60)

bool estaTocando = false;
bool shuffleAtivo = false;
int volumeAtual = 50; // atualizado pelo primeiro AtualizarTocando() ainda no setup()
String musicaAtual = "";
String artistaAtual = "";
unsigned long progressoAtual = 0; // posicao (ms) que a API informou na ultima consulta
unsigned long duracaoAtual = 0;
unsigned long momentoDoFetch = 0; // millis() no instante em que progressoAtual foi lido

// --------------------------- fila de acoes (core 1 -> core 0) ---------------------------

QueueHandle_t filaAcoes;

// --------------------------- playlists (uso exclusivo do core 1) ---------------------------
// A tela de playlists nao anima nada, entao carregar e tocar uma playlist
// podem ser chamadas bloqueantes direto no loop(). Por isso essas variaveis
// nao precisam de mutex: so o core 1 mexe nelas.
std::vector<String> nomesPlaylists;
std::vector<String> urisPlaylists;
int playlistSelecionada = 0;

unsigned long ultimaAtualizacaoTocando = 0; // escrita pelas 2 tarefas
const unsigned long INTERVALO_TOCANDO_MS = 2000; // consulta "tocando agora" a cada 2s

unsigned long ultimaAnimacao = 0;
const unsigned long INTERVALO_ANIMACAO_MS = 100; // redesenha a tela "Tocando" pra animar a rolagem do nome

unsigned long ultimoEnvioVolume = 0;
const unsigned long INTERVALO_VOLUME_MS = 400; // throttle: no maximo 1 pedido de volume a cada 400ms

// O mesmo potenciometro navega playlist e controla volume, dependendo da
// tela. Para nao dar um salto de volume ao trocar de tela, o valor so passa
// a valer quando a posicao do potenciometro se aproxima do volume atual,
// como o pickup mode de mesas de som com fader nao motorizado.
bool potPegouVolume = false;

// ------------------------------- botoes -------------------------------

volatile bool flagMenu = false;
volatile bool flagSelecionar = false;
volatile bool flagPlayPause = false;

volatile unsigned long ultimoInterruptMenu = 0;
volatile unsigned long ultimoInterruptSelecionar = 0;
volatile unsigned long ultimoInterruptPlayPause = 0;

// Tempo minimo entre cliques para nao contar chacoalhada de contato ruim
// (comum em protoboard) como cliques separados.
const unsigned long DEBOUNCE_MS = 350;

// enquanto millis() for menor que isso, cliques sao ignorados (ver setup()).
unsigned long ignorarBotoesAte = 0;

void IRAM_ATTR ISR_Menu() {
  unsigned long agora = millis();
  if (agora - ultimoInterruptMenu > DEBOUNCE_MS) {
    flagMenu = true;
    ultimoInterruptMenu = agora;
  }
}

void IRAM_ATTR ISR_Selecionar() {
  unsigned long agora = millis();
  if (agora - ultimoInterruptSelecionar > DEBOUNCE_MS) {
    flagSelecionar = true;
    ultimoInterruptSelecionar = agora;
  }
}

void IRAM_ATTR ISR_PlayPause() {
  unsigned long agora = millis();
  if (agora - ultimoInterruptPlayPause > DEBOUNCE_MS) {
    flagPlayPause = true;
    ultimoInterruptPlayPause = agora;
  }
}

// Segunda camada de debounce: evita tratar varias chacoalhadas dentro da
// janela da ISR como cliques separados.
volatile unsigned long ultimoConsumoMenu = 0;
volatile unsigned long ultimoConsumoSelecionar = 0;
volatile unsigned long ultimoConsumoPlayPause = 0;

// Terceira camada: ignora cliques que cheguem logo apos o clique de outro
// botao (ruido cruzado entre os fios, comum em protoboard).
volatile unsigned long ultimoCliqueGlobal = 0;
const unsigned long DEBOUNCE_ENTRE_BOTOES_MS = 200;

bool consumirClique(volatile bool &flag, volatile unsigned long &ultimoConsumo) {
  if (millis() < ignorarBotoesAte) {
    flag = false;
    return false;
  }

  if (flag) {
    flag = false;

    unsigned long agora = millis();
    if (agora - ultimoConsumo < DEBOUNCE_MS) {
      return false; // chacoalhada residual do mesmo toque, ignora
    }
    if (agora - ultimoCliqueGlobal < DEBOUNCE_ENTRE_BOTOES_MS) {
      return false; // clique de outro botao ha pouco tempo, provavel ruido cruzado
    }
    ultimoConsumo = agora;
    ultimoCliqueGlobal = agora;
    return true;
  }

  return false;
}

// ------------------------- acesso ao estado compartilhado -------------------------

String ObterAccessToken() {
  xSemaphoreTake(mutexEstado, portMAX_DELAY);
  String tok = access_token;
  xSemaphoreGive(mutexEstado);
  return tok;
}

void DefinirAccessToken(const String &novoToken) {
  xSemaphoreTake(mutexEstado, portMAX_DELAY);
  access_token = novoToken;
  xSemaphoreGive(mutexEstado);
}

// A API so e consultada a cada alguns segundos; entre uma consulta e outra a
// posicao da faixa e estimada somando o tempo passado (millis()), e
// corrigida quando a resposta real chega. Chamar sempre com o mutexEstado
// ja tomado (usada dentro de LerEstadoTocando()).
static unsigned long ProgressoEstimadoMs_SemLock() {
  if (!estaTocando || duracaoAtual == 0) {
    return progressoAtual;
  }

  unsigned long decorrido = millis() - momentoDoFetch;
  unsigned long estimado = progressoAtual + decorrido;

  if (estimado > duracaoAtual) estimado = duracaoAtual;
  return estimado;
}

// Le todo o estado que a tela "Tocando" precisa de uma vez, com o mutex
// tomado por pouco tempo, para nao desenhar uma mistura de dados velhos com
// novos.
SnapshotTocando LerEstadoTocando() {
  SnapshotTocando s;
  xSemaphoreTake(mutexEstado, portMAX_DELAY);
  s.tocando = estaTocando;
  s.shuffle = shuffleAtivo;
  s.volume = volumeAtual;
  s.musica = musicaAtual;
  s.artista = artistaAtual;
  s.duracao = duracaoAtual;
  s.progresso = ProgressoEstimadoMs_SemLock();
  xSemaphoreGive(mutexEstado);
  return s;
}

static void DesenharTelaTocandoAgora() {
  SnapshotTocando s = LerEstadoTocando();
  TelaTocando(s.musica, s.artista, s.tocando, s.progresso, s.duracao, s.shuffle, s.volume);
}

// -------------------------------- Spotify --------------------------------
// (chamado so pela tarefa de rede, exceto onde indicado)

void RenovarToken() {
  xSemaphoreTake(mutexRede, portMAX_DELAY);
  String resposta = getSpotifyToken(client_id, client_secret, refresh_token);
  xSemaphoreGive(mutexRede);

  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, resposta);

  if (!erro && doc["access_token"]) {
    DefinirAccessToken(doc["access_token"].as<String>());
    tokenObtidoEm = millis();
    Serial.println("Token renovado.");
  } else {
    Serial.println("Falha ao renovar token: " + resposta);
  }
}

// Chamada direto do loop() (core 1); ok pois a tela de playlists nao anima nada.
void CarregarPlaylists() {
  telaAtual = TELA_PLAYLISTS;
  nomesPlaylists.clear();
  urisPlaylists.clear();
  printMenu(false, nomesPlaylists, 0, shuffleAtivo); // mostra "Carregando..."

  xSemaphoreTake(mutexRede, portMAX_DELAY);
  String resposta = GetPlaylists(ObterAccessToken());
  xSemaphoreGive(mutexRede);

  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, resposta);

  if (erro) {
    Serial.println("Erro ao ler playlists: " + String(erro.c_str()));
    ExibirOLED("Erro ao carregar\nplaylists.");
    return;
  }

  for (JsonObject item : doc["items"].as<JsonArray>()) {
    nomesPlaylists.push_back(item["name"].as<String>());
    urisPlaylists.push_back(item["uri"].as<String>());
  }

  playlistSelecionada = 0;
  Serial.printf("%d playlists carregadas.\n", (int)nomesPlaylists.size());
  printMenu(false, nomesPlaylists, playlistSelecionada, shuffleAtivo);
}

// Chamada so pela tarefa de rede (core 0).
void AtualizarTocando() {
  xSemaphoreTake(mutexRede, portMAX_DELAY);
  String resposta = GetSpotify("https://api.spotify.com/v1/me/player", ObterAccessToken());
  xSemaphoreGive(mutexRede);

  if (resposta == "deu ruim") {
    // Erro de rede de verdade (timeout, rate limit, falha de TLS). Mantem o
    // ultimo estado conhecido na tela em vez de zerar a musica.
    Serial.println("Falha ao consultar 'tocando agora', mantendo o ultimo estado conhecido na tela");
    return;
  }

  if (resposta.length() == 0) {
    // Nada tocando de verdade: resposta 2xx sem corpo (204), formato padrao
    // do Spotify quando nao ha dispositivo ativo.
    xSemaphoreTake(mutexEstado, portMAX_DELAY);
    estaTocando = false;
    musicaAtual = "";
    artistaAtual = "";
    progressoAtual = 0;
    duracaoAtual = 0;
    xSemaphoreGive(mutexEstado);
    return;
  }

  // Filtra o JSON pra manter so os campos usados (o payload completo passa
  // de 5-10KB, o que aperta a memoria do ESP32).
  JsonDocument filtro;
  filtro["is_playing"] = true;
  filtro["shuffle_state"] = true;
  filtro["progress_ms"] = true;
  filtro["item"]["name"] = true;
  filtro["item"]["duration_ms"] = true;
  filtro["item"]["artists"][0]["name"] = true;
  filtro["device"]["volume_percent"] = true;

  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, resposta, DeserializationOption::Filter(filtro));

  if (erro) {
    Serial.println("Erro ao ler 'tocando agora': " + String(erro.c_str()));
    return;
  }

  bool novoTocando   = doc["is_playing"] | false;
  String novaMusica  = doc["item"]["name"] | "";
  String novoArtista = doc["item"]["artists"][0]["name"] | "";
  unsigned long novoProgresso = doc["progress_ms"] | 0;
  unsigned long novaDuracao   = doc["item"]["duration_ms"] | 0;

  if (novaMusica.length() == 0) {
    // Resposta sem nome de faixa: pode ser anuncio (Spotify Free) ou
    // transicao entre musicas. Ignora essa consulta e mantem o ultimo
    // estado valido na tela.
    Serial.println("[AtualizarTocando] resposta sem nome de faixa, mantendo o ultimo estado conhecido");
    return;
  }

  xSemaphoreTake(mutexEstado, portMAX_DELAY);
  estaTocando   = novoTocando;
  shuffleAtivo  = doc["shuffle_state"] | shuffleAtivo; // sincroniza com o app (celular, PC etc.)
  musicaAtual   = novaMusica;
  artistaAtual  = novoArtista;
  progressoAtual = novoProgresso;
  duracaoAtual   = novaDuracao;
  volumeAtual    = doc["device"]["volume_percent"] | volumeAtual; // sincroniza com o volume real
  momentoDoFetch = millis();
  xSemaphoreGive(mutexEstado);
}

// Executa uma acao pedida pelo core 1 (botao ou potenciometro). So roda na tarefa de rede.
void ProcessarAcao(const AcaoRede &acao) {
  String tok = ObterAccessToken();

  xSemaphoreTake(mutexRede, portMAX_DELAY);
  switch (acao.tipo) {
    case ACAO_TOGGLE_PLAYPAUSE: {
      bool eraTocando = (acao.valor == 1);
      bool ok = eraTocando ? SpotifyPause(tok) : SpotifyResume(tok);
      if (!ok) {
        // UI ja trocou o icone (feedback otimista); se a chamada falhou, desfaz.
        xSemaphoreTake(mutexEstado, portMAX_DELAY);
        estaTocando = eraTocando;
        xSemaphoreGive(mutexEstado);
        Serial.println("Erro ao pausar/retomar (revertido)");
      }
      break;
    }
    case ACAO_SKIP_NEXT:
      Serial.println("[Botao] SELECIONAR -> proxima faixa");
      SpotifySkipNext(tok);
      break;
    case ACAO_SET_VOLUME:
      if (!SpotifySetVolume(tok, acao.valor)) {
        Serial.println("Erro ao mudar volume (dispositivo ativo suporta controle de volume?)");
      }
      break;
  }
  xSemaphoreGive(mutexRede);

  if (acao.tipo == ACAO_SKIP_NEXT) {
    delay(300); // da tempo do Spotify trocar de faixa antes do AtualizarTocando() seguinte
  }
}

// ------------------------------ leitura do pot ------------------------------

// O ADC do ESP32 e ruidoso; tira a media de varias leituras para nao mandar
// pedidos de volume por oscilacao do sensor.
static int LerPotenciometroBruto() {
  const int AMOSTRAS = 12;
  long soma = 0;
  for (int i = 0; i < AMOSTRAS; i++) {
    soma += analogRead(PINO_POT);
  }
  return soma / AMOSTRAS;
}

int LerPotenciometro(int quantidadeItens) {
  if (quantidadeItens <= 0) return 0;
  int leitura = LerPotenciometroBruto(); // 0 - 4095, ja filtrado
  int indice = map(leitura, 0, 4095, 0, quantidadeItens - 1);
  return constrain(indice, 0, quantidadeItens - 1);
}

// ---------------------------- tarefa de rede (core 0) ----------------------------

void TarefaRede(void *parametro) {
  for (;;) {
    unsigned long agora = millis();

    if (agora - tokenObtidoEm > TOKEN_VALIDADE_MS) {
      RenovarToken();
    }

    // esvazia a fila de acoes pedidas pelos botoes, sem bloquear
    AcaoRede acao;
    bool processouAcao = false;
    while (xQueueReceive(filaAcoes, &acao, 0) == pdTRUE) {
      ProcessarAcao(acao);
      if (acao.tipo != ACAO_SET_VOLUME) processouAcao = true; // volume ja e otimista, nao precisa reconsultar
    }

    if (processouAcao) {
      AtualizarTocando();
      ultimaAtualizacaoTocando = agora;
    } else if (telaAtual == TELA_TOCANDO && (agora - ultimaAtualizacaoTocando > INTERVALO_TOCANDO_MS)) {
      AtualizarTocando();
      ultimaAtualizacaoTocando = agora;
    }

    vTaskDelay(pdMS_TO_TICKS(50)); // cede a CPU, nao precisa rodar mais rapido que isso
  }
}

// ----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  pinMode(PINO_BTN_MENU, INPUT_PULLUP);
  pinMode(PINO_BTN_SELECIONAR, INPUT_PULLUP);
  pinMode(PINO_BTN_PLAYPAUSE, INPUT_PULLUP);
  // o potenciometro fica em GPIO33, que ja e analogico por padrao (sem pinMode)

  attachInterrupt(digitalPinToInterrupt(PINO_BTN_MENU), ISR_Menu, FALLING);
  attachInterrupt(digitalPinToInterrupt(PINO_BTN_SELECIONAR), ISR_Selecionar, FALLING);
  attachInterrupt(digitalPinToInterrupt(PINO_BTN_PLAYPAUSE), ISR_PlayPause, FALLING);

  mutexEstado = xSemaphoreCreateMutex();
  mutexRede = xSemaphoreCreateMutex();
  filaAcoes = xQueueCreate(8, sizeof(AcaoRede));

  IniciarTelas();
  ExibirOLED("Conectando ao\nWiFi...");

  WiFi.begin(ssid, senha);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");

  ExibirOLED("Autenticando\nno Spotify...");
  RenovarToken();

  AtualizarTocando(); // primeira leitura, ainda sincrona, so acontece uma vez no boot
  ultimaAtualizacaoTocando = millis();
  ultimaAnimacao = millis();

  ignorarBotoesAte = millis() + 1500;

  DesenharTelaTocandoAgora();

  // core 0: mesmo nucleo que o WiFi ja usa, roda tambem a tarefa de rede.
  // core 1 fica livre so para o loop() (botoes, potenciometro, tela).
  xTaskCreatePinnedToCore(TarefaRede, "TarefaRede", 8192, NULL, 1, NULL, 0);
}

void loop() {
  unsigned long agora = millis();

  // botao MENU: alterna entre a tela "Tocando" e a tela "Playlists"
  if (consumirClique(flagMenu, ultimoConsumoMenu)) {
    if (telaAtual == TELA_TOCANDO) {
      CarregarPlaylists();
    } else {
      telaAtual = TELA_TOCANDO;
      potPegouVolume = false; // o potenciometro estava navegando playlists, nao vale como volume ainda
      ultimaAtualizacaoTocando = 0; // forca a tarefa de rede a atualizar quase na hora
      DesenharTelaTocandoAgora(); // desenha o ultimo estado conhecido, sem esperar a rede
    }
  }

  if (telaAtual == TELA_PLAYLISTS) {
    if (!nomesPlaylists.empty()) {
      int novoIndice = LerPotenciometro(nomesPlaylists.size());
      if (novoIndice != playlistSelecionada) {
        playlistSelecionada = novoIndice;
        printMenu(false, nomesPlaylists, playlistSelecionada, shuffleAtivo);
      }
    }

    // botao SELECIONAR: toca a playlist marcada
    if (consumirClique(flagSelecionar, ultimoConsumoSelecionar) && !urisPlaylists.empty()) {
      ExibirOLED("Tocando...");
      xSemaphoreTake(mutexRede, portMAX_DELAY);
      bool ok = SpotifyPlayContext(ObterAccessToken(), urisPlaylists[playlistSelecionada]);
      xSemaphoreGive(mutexRede);

      if (!ok) {
        ExibirOLED("Erro ao tocar.\nAbra o Spotify\nem algum\ndispositivo\nprimeiro.");
        delay(2000);
      }

      telaAtual = TELA_TOCANDO;
      potPegouVolume = false;
      ultimaAtualizacaoTocando = 0; // forca atualizacao rapida pela tarefa de rede
      DesenharTelaTocandoAgora();
    }

    // botao PLAY/PAUSE (sobrando nessa tela): liga/desliga o modo aleatorio.
    if (consumirClique(flagPlayPause, ultimoConsumoPlayPause)) {
      bool novoEstado = !shuffleAtivo;
      xSemaphoreTake(mutexRede, portMAX_DELAY);
      bool ok = SpotifyShuffle(ObterAccessToken(), novoEstado);
      xSemaphoreGive(mutexRede);
      if (ok) {
        shuffleAtivo = novoEstado;
      } else {
        Serial.println("Erro ao mudar o modo aleatorio");
      }
      printMenu(false, nomesPlaylists, playlistSelecionada, shuffleAtivo);
    }
  } else { // TELA_TOCANDO
    // potenciometro nessa tela controla o volume do dispositivo ativo (0-100%)
    int volumeAtualLocal;
    xSemaphoreTake(mutexEstado, portMAX_DELAY);
    volumeAtualLocal = volumeAtual;
    xSemaphoreGive(mutexEstado);

    int volumeDesejado = LerPotenciometro(101); // 101 "opcoes" = indices 0..100

    if (!potPegouVolume) {
      if (abs(volumeDesejado - volumeAtualLocal) <= 3) {
        potPegouVolume = true;
      }
    } else if (volumeDesejado != volumeAtualLocal && (agora - ultimoEnvioVolume) > INTERVALO_VOLUME_MS) {
      ultimoEnvioVolume = agora;

      xSemaphoreTake(mutexEstado, portMAX_DELAY);
      volumeAtual = volumeDesejado; // feedback otimista, instantaneo na tela
      xSemaphoreGive(mutexEstado);

      AcaoRede a = {ACAO_SET_VOLUME, volumeDesejado};
      xQueueSend(filaAcoes, &a, 0);

      DesenharTelaTocandoAgora();
    }

    if (consumirClique(flagPlayPause, ultimoConsumoPlayPause)) {
      bool eraTocando;
      xSemaphoreTake(mutexEstado, portMAX_DELAY);
      eraTocando = estaTocando;
      estaTocando = !estaTocando; // feedback otimista: o icone troca na hora
      xSemaphoreGive(mutexEstado);

      AcaoRede a = {ACAO_TOGGLE_PLAYPAUSE, eraTocando ? 1 : 0};
      xQueueSend(filaAcoes, &a, 0);

      DesenharTelaTocandoAgora();
    }

    // botao SELECIONAR (sobrando nessa tela): pula pra proxima faixa
    if (consumirClique(flagSelecionar, ultimoConsumoSelecionar)) {
      AcaoRede a = {ACAO_SKIP_NEXT, 0};
      xQueueSend(filaAcoes, &a, 0);
    }

    if (agora - ultimaAnimacao > INTERVALO_ANIMACAO_MS) {
      ultimaAnimacao = agora;
      DesenharTelaTocandoAgora();
    }
  }

  delay(15); // suaviza a leitura do potenciometro e dos botoes
}
