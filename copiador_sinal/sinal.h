#ifndef SINAL_H
#define SINAL_H

#include <Preferences.h>
#include <vector>
#include <Arduino.h>

#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>

extern int IR_RECEIVE_PIN;
extern int IR_SEND_PIN;
extern Preferences preferences;

// Estrutura de um Comando individual
struct Comando {
    String nome;
    IRData sinal;
    bool temSinal;
};

// Estrutura de um Dispositivo (que contém vários comandos)
struct Dispositivo {
    String nome;
    std::vector<Comando> comandos;
};

extern std::vector<Dispositivo> listaDispositivos;

void initIR();
void carregarDados();
void salvarDados();
void receberSinal(Comando &cmd);
void enviarSinal(Comando &cmd);
void descriSinal(IRData sinal);

#endif