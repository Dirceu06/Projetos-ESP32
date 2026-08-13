#ifndef NTP_H
#define NTP_H

#include "env.h"
#include <time.h>

extern const char* ntpServer;
extern const long  gmtOffset_sec;
extern const int   daylightOffset_sec;
extern unsigned long ultimaAtualizacao;

void configurarHora();

#endif