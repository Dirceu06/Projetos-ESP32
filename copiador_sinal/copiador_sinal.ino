#include "auxi.h"
#include "telas.h"
#include "sinal.h"
#include "servidor.h"
#include <vector>

bool noMenuDispositivos = true; 
int dispositivoAbertoId = -1;

unsigned long tempoUltimaAcao = 0;
const unsigned long TEMPO_STANDBY = 10000; // 15 segundos (em milissegundos)
bool ecraLigado = true;
int ultimoValorPot = -1;

void setup() {
  Serial.begin(115200);
  pinMode(POT_PIN, INPUT);
  pinMode(BTT_PIN1, INPUT);
  pinMode(BTT_PIN2, INPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  
  Wire.begin(27, 26);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("oled Falha ao inicializar");
  }

  initIR();
  initWiFi();
}

void loop() {
  gerirServidor();
  bool b1 = digitalRead(BTT_PIN1);
  bool b2 = digitalRead(BTT_PIN2);

  if (b1 || b2) {
    acordarEcra();
  }

  
  
  if (noMenuDispositivos) {  // NÍVEL 1: MENU DE DISPOSITIVOS
    std::vector<String> nomesDisp;
    for (int i = 0; i < listaDispositivos.size(); i++) {
      nomesDisp.push_back(listaDispositivos[i].nome);
    }
    nomesDisp.push_back("Novo +");

    int atual = lerPot(nomesDisp.size());
    if (ecraLigado) {
      printMenu(true, nomesDisp, atual);
    }

    if (atual != ultimoValorPot) {
      acordarEcra();
      ultimoValorPot = atual;
    }

    if (b1 && !b2) {
      if (atual < listaDispositivos.size()) {
        String novoNome = lerNomeSerial("Novo nome disp:");
        listaDispositivos[atual].nome = novoNome;
        salvarDados();
        delay(300);
      }
    }
    else if (b2 && !b1) {
      if (atual == nomesDisp.size() - 1) {
        String nomeNovo = lerNomeSerial("Nome do Disp:");
        Dispositivo novo;
        novo.nome = nomeNovo;
        
        // Adiciona comandos padrão iniciais
        std::vector<String> padrao = {"Power"};
        for(int i = 0; i < padrao.size(); i++) {
           Comando c;
           c.nome = padrao[i];
           c.temSinal = false;
           novo.comandos.push_back(c);
        }
        
        listaDispositivos.push_back(novo);
        salvarDados();
        delay(300);
      } else { // Clicou num dispositivo existente
        noMenuDispositivos = false;
        dispositivoAbertoId = atual;
        topo = 0;
        delay(300);
      }
    }
  } 
  else { // NÍVEL 2: MENU DE COMANDOS
    std::vector<String> nomesCmd;
    for(int i = 0; i < listaDispositivos[dispositivoAbertoId].comandos.size(); i++){
       nomesCmd.push_back(listaDispositivos[dispositivoAbertoId].comandos[i].nome);
    }
    nomesCmd.push_back("Novo +");
    nomesCmd.push_back("< Voltar");

    int atual = lerPot(nomesCmd.size());
    printMenu(false, nomesCmd, atual);

    int idxNovo = listaDispositivos[dispositivoAbertoId].comandos.size();
    int idxVoltar = idxNovo + 1;

    // B1 + B2 juntos: Renomear comando via Serial
    if (b1 && b2) {
      if (atual < idxNovo) {
        String novoNome = lerNomeSerial("Novo nome cmd:");
        listaDispositivos[dispositivoAbertoId].comandos[atual].nome = novoNome;
        salvarDados();
        delay(300);
      }
    }
    // B1 isolado: Gravar Sinal IR (ou criar comando se estiver no "Novo +")
    else if (b1 && !b2) {
      if (atual == idxVoltar) {
        noMenuDispositivos = true;
        topo = 0;
        delay(300);
      } else if (atual == idxNovo) { // Clicou no "Novo +" com B1
        String nomeCmd = lerNomeSerial("Nome do Comando:");
        Comando c;
        c.nome = nomeCmd;
        c.temSinal = false;
        listaDispositivos[dispositivoAbertoId].comandos.push_back(c);
        salvarDados();
        delay(300);
      } else { // Gravar sinal IR no comando selecionado
        receberSinal(listaDispositivos[dispositivoAbertoId].comandos[atual]);
        salvarDados();
        delay(300);
      }
    } 
    // B2 isolado: Enviar Sinal IR (ou criar comando se estiver no "Novo +")
    else if (b2 && !b1) {
      if (atual == idxVoltar) {
        noMenuDispositivos = true;
        topo = 0;
        delay(300);
      } else if (atual == idxNovo) { // Clicou no "Novo +" com B2
        String nomeCmd = lerNomeSerial("Nome do Comando:");
        Comando c;
        c.nome = nomeCmd;
        c.temSinal = false;
        listaDispositivos[dispositivoAbertoId].comandos.push_back(c);
        salvarDados();
        delay(300);
      } else { // Enviar sinal IR
        enviarSinal(listaDispositivos[dispositivoAbertoId].comandos[atual]);
        delay(300);
      }
    }
  }
  
  if (ecraLigado && (millis() - tempoUltimaAcao > TEMPO_STANDBY)) {
    display.ssd1306_command(SSD1306_DISPLAYOFF); // Desliga o ecrã OLED
    ecraLigado = false;
  }

}



void acordarEcra() {
  tempoUltimaAcao = millis();
  if (!ecraLigado) {
    display.ssd1306_command(SSD1306_DISPLAYON); // Liga o OLED
    ecraLigado = true;
  }
}