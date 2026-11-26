// relay_functions.h
// Desenvolvido para a placa EK-TM4C1294XL
// Controla reles (representados por leds)
// Edson, Fabio, Joao Victor
// 11/02/2025

#ifndef RELAY_FUNCTIONS_H
#define RELAY_FUNCTIONS_H
#endif

#include <stdint.h>
#include <stdbool.h>

int8_t executaComando(uint8_t *commandBuffer, bool *statusReles);
