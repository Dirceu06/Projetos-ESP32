#include <IRremote.hpp> 

#include "sinal.h"
#include "telas.h"
#include "auxi.h"

int IR_RECEIVE_PIN = 13;
int IR_SEND_PIN = 12;

Preferences preferences;
std::vector<Dispositivo> listaDispositivos;

void initIR() {
  IrSender.begin(IR_SEND_PIN);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  carregarDados();
}

void carregarDados() {
  preferences.begin("ir_data", false);
  int num_disp = preferences.getInt("num_disp", 0);

  // Se num_disp for 0, é a primeira vez. Fica vazio.
  for (int i = 0; i < num_disp; i++) {
    Dispositivo d;
    d.nome = preferences.getString(("d_n_" + String(i)).c_str(), "Disp");
    int num_cmd = preferences.getInt(("d_c_" + String(i)).c_str(), 0);

    for (int j = 0; j < num_cmd; j++) {
      Comando c;
      c.nome = preferences.getString(("c_n_" + String(i) + "_" + String(j)).c_str(), "Cmd");
      c.temSinal = preferences.getBool(("c_t_" + String(i) + "_" + String(j)).c_str(), false);
      
      if (c.temSinal) {
        preferences.getBytes(("c_s_" + String(i) + "_" + String(j)).c_str(), &c.sinal, sizeof(IRData));
      }
      d.comandos.push_back(c);
    }
    listaDispositivos.push_back(d);
  }
}

void salvarDados() {
  preferences.putInt("num_disp", listaDispositivos.size());
  
  for (int i = 0; i < listaDispositivos.size(); i++) {
    preferences.putString(("d_n_" + String(i)).c_str(), listaDispositivos[i].nome);
    preferences.putInt(("d_c_" + String(i)).c_str(), listaDispositivos[i].comandos.size());

    for (int j = 0; j < listaDispositivos[i].comandos.size(); j++) {
      preferences.putString(("c_n_" + String(i) + "_" + String(j)).c_str(), listaDispositivos[i].comandos[j].nome);
      preferences.putBool(("c_t_" + String(i) + "_" + String(j)).c_str(), listaDispositivos[i].comandos[j].temSinal);
      
      if (listaDispositivos[i].comandos[j].temSinal) {
        preferences.putBytes(("c_s_" + String(i) + "_" + String(j)).c_str(), &listaDispositivos[i].comandos[j].sinal, sizeof(IRData));
      }
    }
  }
}

void receberSinal(Comando &cmd) {
  ExibirOLED("Aguardando IR...");
  while (true) {
    if (IrReceiver.decode()) {
      cmd.sinal = IrReceiver.decodedIRData;
      cmd.temSinal = true;
      
      recebidoBuzz();
      descriSinal(cmd.sinal);
      
      delay(1000);
      IrReceiver.resume();
      break;
    }
  }
}

void enviarSinal(Comando &cmd) {
  if (!cmd.temSinal) {
    ExibirOLED("Vazio!");
    delay(1000);
  } else {
    IrSender.write(&cmd.sinal);
    enviadoBuzz();
  }
}

void descriSinal(IRData sinal){
  Serial.println("========== IR CAPTURADO ==========");
  Serial.print("Protocolo: "); Serial.println(getProtocolString(sinal.protocol));
  Serial.print("Comando: 0x"); Serial.println(sinal.command, HEX);
}